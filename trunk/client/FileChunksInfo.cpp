#include "stdinc.h"
#include "DCPlusPlus.h"

#include "FileChunksInfo.h"
#include "SettingsManager.h"

CriticalSection SharedFileStream::critical_section;
SharedFileStream::SharedFileHandleMap SharedFileStream::file_handle_pool;


vector<FileChunksInfo::Ptr> FileChunksInfo::vecAllFileChunksInfo;
CriticalSection FileChunksInfo::hMutexMapList;

FileChunksInfo::Ptr FileChunksInfo::Get(const string& name)
{
    Lock l(hMutexMapList);

	for(vector<Ptr>::iterator i = vecAllFileChunksInfo.begin(); i != vecAllFileChunksInfo.end(); i++){
		if((*i)->sFilename == name){
			return (*i);
		}
	}

	return NULL;
}

void FileChunksInfo::Free(const string& name)
{
    Lock l(hMutexMapList);

	for(vector<FileChunksInfo::Ptr>::iterator i = vecAllFileChunksInfo.begin(); i != vecAllFileChunksInfo.end(); i++){
		if((*i)->sFilename == name ){
			vecAllFileChunksInfo.erase(i);
			delete i;
			return;
		}
	}

	_ASSERT(0);
}

FileChunksInfo::FileChunksInfo(const string& name, int64_t size, const vector<int64_t>* blocks) 
	: sFilename(name), iFileSize(size)
{
	hMutexMapList.enter();
	vecAllFileChunksInfo.push_back(this);
	hMutexMapList.leave();

	if(blocks == NULL){
		vecFreeBlocks.push_back(0);
		vecFreeBlocks.push_back(size);
	}else{
		dcdebug("Freeblocks %d\n", blocks->size());
		copy(blocks->begin(), blocks->end(), back_inserter(vecFreeBlocks));
	}
    iDownloadedSize = iFileSize;
    for(vector<int64_t>::iterator i = vecFreeBlocks.begin(); i < vecFreeBlocks.end(); i++, i++)
        iDownloadedSize -= ((*(i+1)) - (*i));

	iBlockSize = max(TigerTree::calcBlockSize(iFileSize, 10), (size_t)SMALLEST_BLOCK_SIZE);
	iSmallestBlockSize = SMALLEST_BLOCK_SIZE;

	iVerifiedSize = 0;
}

int64_t FileChunksInfo::GetBlockEnd(int64_t begin)
{
	Lock l(hMutex);

	for(vector<int64_t>::iterator i = vecRunBlocks.begin(); i != vecRunBlocks.end(); i++, i++){
		if((*i) == begin){
			return *(i+1);
		}
	}

	return -1;
}


int FileChunksInfo::ValidBlock(int64_t start, size_t& len)
{
	Lock l(hMutex);

	for(vector<int64_t>::iterator i = vecRunBlocks.begin(); i != vecRunBlocks.end(); i++, i++){
		if((*i) == start){
			(*i) += len;
			iDownloadedSize += len;

			// block over
			if((*i) >= (*(i+1))){

				iDownloadedSize -= ((*i) - (*(i+1)));
				vecRunBlocks.erase(i + 1);
				vecRunBlocks.erase(i);

				if(vecFreeBlocks.empty() && vecRunBlocks.empty()){
					return FILE_OVER;
				}

				return BLOCK_OVER;
			}

			return NORMAL_EXIT;
		}
	}

	return WRONG_POS;
}

int64_t FileChunksInfo::GetUndlStart(int maxSegments)
{
	dcdebug("GetUndlStart running = %d free = %d\n", vecRunBlocks.size(), vecFreeBlocks.size());
	Lock l(hMutex);

	// if have free blocks, return first one
	if(vecFreeBlocks.size() > 1){
		int64_t e = vecFreeBlocks[1];
		int64_t b = vecFreeBlocks[0];

		vecFreeBlocks.erase(vecFreeBlocks.begin());
		vecFreeBlocks.erase(vecFreeBlocks.begin());

		vecRunBlocks.push_back(b);
		vecRunBlocks.push_back(e);

		sort(vecRunBlocks.begin(), vecRunBlocks.end());

		return b;
	}

	if(vecRunBlocks.empty()){
		dcdebug("GetUndlStart return -1\n");
		return -1;
	}
	
	// reassign current run blocks

	int64_t blen = 0;
	vector<int64_t>::iterator birr = vecRunBlocks.begin();

	// find the biggest block
	for(vector<int64_t>::iterator i = vecRunBlocks.begin(); i < vecRunBlocks.end(); i++, i++)
	{
		if((*(i+1)) - (*i) > blen){
			blen = (*(i+1)) - (*i);
			birr = i;
		}
	}

	int64_t b = (*birr);
	int64_t e = (* (birr+1));

	int64_t SMALLEST_BLOCK_SIZE = 64*1024;
	if(SETTING(MIN_BLOCK_SIZE) == SettingsManager::blockSizes[SettingsManager::SIZE_64]) SMALLEST_BLOCK_SIZE = 64*1024;
	if(SETTING(MIN_BLOCK_SIZE) == SettingsManager::blockSizes[SettingsManager::SIZE_128]) SMALLEST_BLOCK_SIZE = 128*1024;
	if(SETTING(MIN_BLOCK_SIZE) == SettingsManager::blockSizes[SettingsManager::SIZE_256]) SMALLEST_BLOCK_SIZE = 256*1024;
	if(SETTING(MIN_BLOCK_SIZE) == SettingsManager::blockSizes[SettingsManager::SIZE_512]) SMALLEST_BLOCK_SIZE = 512*1024;
	if(SETTING(MIN_BLOCK_SIZE) == SettingsManager::blockSizes[SettingsManager::SIZE_1024]) SMALLEST_BLOCK_SIZE = 1024*1024;
	if(SETTING(MIN_BLOCK_SIZE) == SettingsManager::blockSizes[SettingsManager::SIZE_AUTO]) SMALLEST_BLOCK_SIZE = (iFileSize / maxSegments) / 2;
	if(maxSegments == 1) SMALLEST_BLOCK_SIZE = iFileSize;

	if((e - b) < SMALLEST_BLOCK_SIZE){
		dcdebug("GetUndlStart return -1 (SMALLEST_BLOCK_SIZE)\n");
		return -1;
	}

	int64_t n = b + (e - b) / 2;

	if(maxSegments == 1) { n = b; }

	(* (birr+1)) = n;

	vecRunBlocks.push_back(n);
	vecRunBlocks.push_back(e);

	sort(vecRunBlocks.begin(), vecRunBlocks.end());

	return n;
}

void FileChunksInfo::PutUndlStart(int64_t start)
{

	Lock l(hMutex);

	for(vector<int64_t>::iterator i = vecRunBlocks.begin(); i < vecRunBlocks.end(); i++, i++)
	{
		if((*i) == start){
            if(i != vecRunBlocks.begin() && (*(i-1)) == start){ // continous blocks, concatenate it
				*(i-1) = *(i+1);
				vecRunBlocks.erase(i+1);
				vecRunBlocks.erase(i);
			}else{
				vecFreeBlocks.push_back(*i);
				vecFreeBlocks.push_back(*(i+1));

				sort(vecFreeBlocks.begin(), vecFreeBlocks.end());

				vecRunBlocks.erase(i+1);
				vecRunBlocks.erase(i);
			}
			return;
		}
	}

	_ASSERT(0);
}

string FileChunksInfo::getFreeBlocksString()
{
	Lock l(hMutex);

	ostringstream os;
	copy(vecFreeBlocks.begin(), vecFreeBlocks.end(), ostream_iterator<int64_t>(os, " "));
	copy(vecRunBlocks.begin(), vecRunBlocks.end(), ostream_iterator<int64_t>(os, " "));

	return os.str();

}

string FileChunksInfo::getVerifiedBlocksString()
{
	Lock l(hMutex);

	ostringstream os;

	for(map<int64_t, int64_t>::iterator i = mapVerifiedBlocks.begin(); i != mapVerifiedBlocks.end(); i++)
		os << i->first << " " << i->second << " ";

	return os.str();
}


bool FileChunksInfo::DoLastVerify(const TigerTree& aTree)
{
    _ASSERT(iBlockSize == aTree.getBlockSize());

	dcdebug("DoLastVerify %I64d bytes %d%% verified\n", iVerifiedSize , (int)(iVerifiedSize * 100 / iFileSize)); 
	dcdebug("VerifiedBlocks size = %d\n", mapVerifiedBlocks.size());
	for(map<int64_t, int64_t>::iterator i = mapVerifiedBlocks.begin();
										i != mapVerifiedBlocks.end();
										i++)
	{
		dcdebug("   %I64d, %I64d\n", i->first, i->second);
	}

	// for exception free
	vector<unsigned char> buf;
    buf.reserve(iBlockSize);

	Lock l(hMutex);

    // This is only called when download finish
    // Because buffer is used during download, the disk data maybe incorrect
    _ASSERT(vecFreeBlocks.empty() && vecRunBlocks.empty());

	// Convert to unverified blocks
    map<int64_t, int64_t> unVerifiedBlocks;
	int64_t start = 0;
    for(map<int64_t, int64_t>::iterator i = mapVerifiedBlocks.begin();
        								i != mapVerifiedBlocks.end();
                                        i++)
	{
		if(i->first > start){
            _ASSERT((i->first - start) % iBlockSize == 0);

            unVerifiedBlocks.insert(make_pair(start, i->first));
        }else{
            _ASSERT(start == 0);
        }

		start = i->second;
	
	}

	if(start < iFileSize)
        unVerifiedBlocks.insert(make_pair(start, iFileSize));

	// Open file
	SharedFileStream file(sFilename, 0, 0);

	for(map<int64_t, int64_t>::iterator i = unVerifiedBlocks.begin();
        								i != unVerifiedBlocks.end();
                                        i++)
	{
        int64_t end = i->first;

		while(end < i->second)
        {
            start = end;
            end = min(start + iBlockSize, i->second);

            file.setPos(start);
			size_t len = iBlockSize;
			file.read(&buf[0], len);

			_ASSERT(end - start == len);

			TigerTree cur(iBlockSize);
            cur.update(&buf[0], len);
			cur.finalize();

			_ASSERT(cur.getLeaves().size() == 1);

			// Fail!
        	if(!(cur.getLeaves()[0] == aTree.getLeaves()[(size_t)(start / iBlockSize)]))
        	{
	       		if(!vecFreeBlocks.empty() && *(vecFreeBlocks.rbegin()) == start)
	        	{
	                *(vecFreeBlocks.rbegin()) = end;
	            }
	            else{
	        		vecFreeBlocks.push_back(start);
	        		vecFreeBlocks.push_back(end);
	            }

				iDownloadedSize -= (end - start);
        	}else{
        		MarkVerifiedBlock(start, end);
        	}

		}

		_ASSERT(end == i->second);
    }

	if(vecFreeBlocks.empty()){
		dcdebug("VerifiedBlocks size = %d\n", mapVerifiedBlocks.size());
		for(map<int64_t, int64_t>::iterator i = mapVerifiedBlocks.begin();
											i != mapVerifiedBlocks.end();
											i++)
		{
			dcdebug("   %I64d, %I64d\n", i->first, i->second);
		}

		_ASSERT(mapVerifiedBlocks.size() == 1 && 
			    mapVerifiedBlocks.begin()->first == 0 &&
			    mapVerifiedBlocks.begin()->second == iFileSize);

        return true;
    }

	// double smallest size when last verify failed
	iSmallestBlockSize = min<int64_t>(iSmallestBlockSize * 2, iBlockSize);

	return false;
}

void FileChunksInfo::MarkVerifiedBlock(int64_t start, int64_t end)
{

    _ASSERT( ( end - start) % 1024 == 0 || end == iFileSize);

	dcdebug("MarkVerifiedBlock %I64d, %I64d\n", start, end);
	Lock l(hMutex);

	map<int64_t, int64_t>::iterator i = mapVerifiedBlocks.begin();

	for(;
		i != mapVerifiedBlocks.end();
        i++)
	{
        if(i->second == start){
            i->second = end;
            break;
        }
    }

	if(i == mapVerifiedBlocks.end())
		i = mapVerifiedBlocks.insert(make_pair(start, end)).first;

	for(map<int64_t, int64_t>::iterator j = mapVerifiedBlocks.begin();
        								j != mapVerifiedBlocks.end();
                                        j++)
	{
        if(j->first == end)
        {
            i->second = j->second;
            mapVerifiedBlocks.erase(j);
            break;
        }
    }

	iVerifiedSize += (end - start);
}

