/* 
 * Copyright (C) 2003-2005 RevConnect, http://www.revconnect.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include "stdinc.h"
#include "DCPlusPlus.h"

#include "FileChunksInfo.h"

#include "User.h"
#include "SharedFileStream.h"
#include "ResourceManager.h"
#include "LogManager.h"

FileChunksInfo::tthMap FileChunksInfo::vecAllFileChunksInfo;
CriticalSection FileChunksInfo::hMutexMapList;

// NOTE: THIS MUST EQUAL TO HashManager::Hasher::MIN_BLOCK_SIZE
enum { MIN_BLOCK_SIZE = 65536 };

FileChunksInfo::Ptr FileChunksInfo::Get(const TTHValue* tth)
{
    Lock l(hMutexMapList);
	tthMap::const_iterator i = vecAllFileChunksInfo.find(const_cast<TTHValue*>(tth));
	return (i != vecAllFileChunksInfo.end()) ? i->second : NULL;
}

void FileChunksInfo::Free(const TTHValue* tth)
{
    Lock l(hMutexMapList);
	tthMap::iterator i = vecAllFileChunksInfo.find(const_cast<TTHValue*>(tth));
	vecAllFileChunksInfo.erase(i);
}

FileChunksInfo::FileChunksInfo(const TTHValue* tth, int64_t size, const vector<int64_t>* chunks) 
	: fileSize(size), verifiedSize(0)
{
	{
		Lock l(hMutexMapList);
		vecAllFileChunksInfo.insert(make_pair(const_cast<TTHValue*>(tth), this));
	}

	dcassert(size);

	tthBlockSize = max((size_t)TigerTree::calcBlockSize(fileSize, 10), (size_t)MIN_BLOCK_SIZE);
	minChunkSize = max((int64_t)MIN_CHUNK_SIZE, (int64_t)(fileSize / 100));
	minChunkSize = minChunkSize - (minChunkSize % tthBlockSize);

	if(chunks != NULL){
		for(vector<int64_t>::const_iterator i = chunks->begin(); i < chunks->end(); i++, i++)
			waiting.insert(make_pair(*i, new Chunk(*i, *(i+1))));
	}else{
		waiting.insert(make_pair(0, new Chunk(0, size)));	
	}

	downloadedSize = fileSize;
	for(Chunk::Iter i = waiting.begin(); i != waiting.end(); i++)
        downloadedSize -= (i->second->end - i->second->pos);
}

FileChunksInfo::~FileChunksInfo()
{
	//dcdebug("Delete file chunks info: %s, waiting = %d, running = %d\n", TTH->toBase32().c_str(), waiting.size(), running.size());

	for(Chunk::Iter i = waiting.begin(); i != waiting.end(); i++)
		delete i->second;

	for(Chunk::Iter i = running.begin(); i != running.end(); i++)
		delete i->second;
}

int FileChunksInfo::addChunkPos(int64_t start, int64_t pos, size_t& len)
{
	Lock l(cs);

	Chunk::Iter i = running.find(start);
	if(i == running.end()) {
		dcassert(0);
		return CHUNK_LOST;
	}

	Chunk* chunk = i->second;

	if(chunk->pos != pos) return CHUNK_LOST;

	len = (size_t)min((int64_t)len, chunk->end - chunk->pos);

	chunk->pos += len;
	downloadedSize += len;

	// chunk over
	if(chunk->pos >= chunk->end){
		if(!waiting.empty()) return CHUNK_OVER;
		
		// check unfinished running chunks
		// finished chunk maybe still running, because of overlapped download
		for(Chunk::Iter i = running.begin(); i != running.end(); i++){
			if(i->second->pos < i->second->end)
				return CHUNK_OVER;
		}
		return FILE_OVER;
	}
	return NORMAL_EXIT;
}

void FileChunksInfo::setDownload(int64_t chunk, Download* d, bool noStealth)
{
	Lock l(cs);
	Chunk::Iter i = running.find(chunk);

	if(i == running.end()){
		dcassert(0);
		return;
	}

	i->second->download = d;

	if(noStealth) {
		d->setSize(min(i->second->end, i->second->pos + min(minChunkSize, fileSize - i->second->pos)));
		d->setFlag(Download::FLAG_CHUNKED);
	}
}

int64_t FileChunksInfo::getChunk(bool& useChunks, int64_t _speed)
{
	Chunk* chunk = NULL;

	if(_speed == 0) _speed = DEFAULT_SPEED;

	Lock l(cs);
	dcdebug("getChunk speed = %I64d, running = %d waiting = %d\n", _speed, running.size(), waiting.size());

	useChunks = true;//(waiting.size() > 1) || (running.size() > 0);

	// if there is any waiting chunk, return it
	if(!waiting.empty()){
		chunk = waiting.begin()->second;

		waiting.erase(waiting.begin());
		
		// this chunk maybe already in running because of `overlapped->chunk finish->verify fail'
		// @todo move this to verify() and dolastverify()
		Chunk::Iter i = running.find(chunk->pos);
		if(i != running.end()){
			Chunk* chunk2 = i->second;

			// it must be a finished chunk
			dcassert(chunk2->pos >= chunk2->end);

			// reset the running chunk
			chunk2->overlappedCount++;
			chunk2->pos = chunk->pos;
			chunk2->end = chunk->end;

			// delete the waiting chunk
			delete chunk;
			chunk = chunk2;
		}else{
			running.insert(make_pair(chunk->pos, chunk));
		}

		selfCheck();
		return chunk->pos;
	}

	if(running.empty()){
		dcdebug("running empty, getChunk return -1\n");
		return -1;
	}
	
	// find the max time-left running chunk
	Chunk*  maxChunk = NULL;
	int64_t maxTimeLeft = 0;
	int64_t speed;

	for(Chunk::Iter i = running.begin(); i != running.end(); i++)
	{
		chunk = i->second;

		if((chunk->end - chunk->pos) < minChunkSize*2)
			continue;

		if(chunk->pos > i->first){
			speed = chunk->download ? chunk->download->getAverageSpeed() : 1;
			if(speed == 0) speed = 1;
		}else{
			speed = chunk->download ? chunk->download->getUser()->getLastDownloadSpeed()*1024 : DEFAULT_SPEED;
			if(speed == 0) speed = DEFAULT_SPEED;
		}
		
		int64_t timeLeft = (chunk->end - chunk->pos) / speed;

		if(timeLeft > maxTimeLeft){
			maxTimeLeft = timeLeft;
			maxChunk = chunk;
		}
	}

	// all running chunks are unbreakable (timeleft < 15 sec)
	// try overlapped download the pending chunk
	if(maxTimeLeft < 15){
		for(Chunk::Iter i = running.begin(); i != running.end(); i++)
		{
			chunk = i->second;
			if(	(chunk->pos == i->first) ||
				(chunk->download && (chunk->download->getTotal() > 10) && (chunk->download->getAverageSpeed()*5 < _speed))) {
				chunk->overlappedCount++;
				return i->first;
			}
		}
		
		return -1;
	}

	// split the max time-left running chunk
	int64_t b = maxChunk->pos;
	int64_t e = maxChunk->end;

	speed = maxChunk->download ? maxChunk->download->getAverageSpeed() : DEFAULT_SPEED;

	if(speed == 0 && maxChunk->download){
		speed =  maxChunk->download->getUser()->getLastDownloadSpeed()*1024;
	}

	if(speed == 0){
		speed = DEFAULT_SPEED;
	}

	double x = 1.0 + (double)_speed / (double)speed;

	// too lag than orignal source
	if(x < 1.01) return -1;

	int64_t n = max(b + 1, b + (int64_t)((double)(e - b) / x));

	// align to tiger tree block size
	if(n - b > tthBlockSize * 3 && e - n > tthBlockSize * 3)
		n = n - (n % tthBlockSize);

	maxChunk->end = n;

	running.insert(make_pair(n, new Chunk(n, e)));

	dcdebug("split running chunk (%I64d , %I64d) * %I64d Bytes/s -> (%I64d , %I64d) * %I64d Bytes/s \n", b, e, speed, n, e, _speed);
	selfCheck();
	return n;
}

void FileChunksInfo::putChunk(int64_t start)
{
	Lock l(cs);

	dcdebug("put chunk %I64d\n", start);

	Chunk* prev = NULL;

	for(Chunk::Map::iterator i = running.begin(); i != running.end(); i++)
	{
		Chunk* chunk = i->second;

		if(i->first == start){

			if(i->second->overlappedCount){
				i->second->overlappedCount--;
				chunk->download = NULL;
				return;
			}

			// chunk done
			if(chunk->pos >= chunk->end){
				delete chunk;

			// found continuous running chunk, concatenate it
            }else if(prev != NULL && prev->end == chunk->pos){ 
				prev->end = chunk->end;
				delete chunk;

			// unfinished chunk, waiting ...
			}else{
				chunk->download = NULL;
				waiting.insert(make_pair(chunk->pos, chunk));
			}

			running.erase(i);
			selfCheck();
			return;
		}

		prev = chunk;
	}

	dcassert(0);
}

string FileChunksInfo::getFreeChunksString()
{
	vector<int64_t> tmp;

	{
		Lock l(cs);

		selfCheck();

		Chunk::Map tmpMap = waiting;
		copy(running.begin(), running.end(), inserter(tmpMap, tmpMap.end())); // sort

		tmp.reserve(tmpMap.size() * 2);

		for(Chunk::Iter i = tmpMap.begin(); i != tmpMap.end(); i++){
			if(i->second->pos < i->second->end){
				tmp.push_back(i->second->pos);
				tmp.push_back(i->second->end);
			}
		}
	}

	// merge
	dcdebug("Before merge : %d\n", tmp.size());
	if(tmp.size() > 2){
		vector<int64_t>::iterator prev = tmp.begin() + 1;
		vector<int64_t>::iterator i = prev + 1;
		while(i < tmp.end()){
			if(*i == *prev){
				*prev = *(i + 1);
				i = tmp.erase(i, i + 2);
			}else{
				prev = i + 1;
				i = i + 2;
			}
		}
	}
	dcdebug("After merge : %d\n", tmp.size());

	string ret;
	for(vector<int64_t>::const_iterator i = tmp.begin(); i != tmp.end(); i++)
		ret += Util::toString(*i) + " ";

	return ret;
}

string FileChunksInfo::getVerifiedBlocksString()
{
	Lock l(cs);

	string ret;

	for(BlockMap::const_iterator i = verifiedBlocks.begin(); i != verifiedBlocks.end(); i++)
		ret += Util::toString(i->first) + " " + Util::toString(i->second) + " ";

	return ret;
}

inline void FileChunksInfo::dumpVerifiedBlocks()
{
#ifdef _DEBUG
	dcdebug("verifiedBlocks: (%d)\n", verifiedBlocks.size());

	BlockMap::const_iterator i = verifiedBlocks.begin();
	for(;i != verifiedBlocks.end(); i++)
		dcdebug("   %hu, %hu\n", i->first, i->second);
#endif
}

bool FileChunksInfo::verify(const unsigned char* data, int64_t start, int64_t end, const TigerTree& aTree)
{
	dcassert(end - start <= tthBlockSize);
	size_t len = (size_t)(end - start);

	TigerTree cur(tthBlockSize);
    cur.update(data, len);
	cur.finalize();

	dcassert(cur.getLeaves().size() == 1);

	// verify OK
	uint16_t blockNo = (uint16_t)(start / tthBlockSize);
	if(cur.getLeaves()[0] == aTree.getLeaves()[blockNo]){
		markVerifiedBlock(blockNo, blockNo + 1);
		return true;
	}

	// fail, generate a waiting chunk
	// @todo: merge
	// Note: following code causes a running chunk dupe with waiting chunk
	// Eg. original running chunk: (1000, 1500, 2000), and 800-1500 corruption detected
	//  -> running chunks (1000, 1500, 2000), waiting chunk (800, 1500)
	//
		
	waiting.insert(make_pair(start, new Chunk(start, end)));

	downloadedSize -= (end - start);
	selfCheck();

#ifdef _DEBUG
	try{
		string filename = Util::getDataPath() + aTree.getRoot().toBase32() + "." + Util::toString(start) + "." + Util::toString(end) + "." + "dat";
		File f(filename, File::WRITE, File::CREATE | File::TRUNCATE);
		f.write(data, len);
	}catch(const FileException& e){
		dcdebug("%s\n", e.getError().c_str());
		dcassert(0);
	}
#endif
	const size_t BUF_SIZE = STRING(CORRUPTION_DETECTED).size() + 128;
	AutoArray<char> tmp(BUF_SIZE);
	snprintf(tmp, BUF_SIZE, CSTRING(CORRUPTION_DETECTED), aTree.getRoot().toBase32(), start);
	LogManager::getInstance()->message(string(tmp));
	return false;
}

bool FileChunksInfo::doLastVerify(const TigerTree& aTree, const string& tempTargetName)
{
    dcassert(tthBlockSize == aTree.getBlockSize());
	Lock l(cs);
	
	dcdebug("doLastVerify %I64d bytes %d%% verified\n", verifiedSize , (int)(verifiedSize * 100 / fileSize)); 

	dumpVerifiedBlocks();

	// for exception free
	auto_ptr<unsigned char> buf(new unsigned char[tthBlockSize]);

    // This is only called when download finish
    // Because buffer is used during download, the disk data maybe incorrect
	dcdebug("waiting = %d, running = %d\n", waiting.size(), running.size()); 
    dcassert(waiting.empty());

#ifdef _DEBUG
	for(Chunk::Iter j = running.begin(); j != running.end(); j++)
		dcassert(j->second->pos == j->second->end);
#endif
 
	// Convert to unverified blocks
    map<int64_t, int64_t> unVerifiedBlocks;
	int64_t start = 0;

	BlockMap::iterator i = verifiedBlocks.begin();
	for(; i != verifiedBlocks.end(); i++)
	{
		int64_t first  = (int64_t)i->first  * (int64_t)tthBlockSize;
		int64_t second = (int64_t)i->second * (int64_t)tthBlockSize;
		
		second = min(second, fileSize);

		if(first > start){
            unVerifiedBlocks.insert(make_pair(start, first));
        }else{
            dcassert(start == 0);
        }
		start = second;
	}

	if(start < fileSize)
        unVerifiedBlocks.insert(make_pair(start, fileSize));

	// Open file
	SharedFileStream file(tempTargetName, 0, 0);

	for(map<int64_t, int64_t>::const_iterator i = unVerifiedBlocks.begin();
        								i != unVerifiedBlocks.end();
                                        i++)
	{
        int64_t end = i->first;

		while(end < i->second)
        {
            start = end;
            end = min(start + tthBlockSize, i->second);

            file.setPos(start);
			size_t len = (size_t)(end - start);
			file.read(buf.get(), len);

			dcassert(end - start == len);

			TigerTree cur(tthBlockSize);
            cur.update(buf.get(), len);
			cur.finalize();

			dcassert(cur.getLeaves().size() == 1);

			// Fail!
        	if(!(cur.getLeaves()[0] == aTree.getLeaves()[(size_t)(start / tthBlockSize)]))
        	{

	       		if(!waiting.empty() && waiting.rbegin()->second->end == start){
					waiting.rbegin()->second->end = end;

	            }else{
					bool merged = false;
					// try to merge with running first
					for(Chunk::Iter j = running.begin(); j != running.end(); j++){
						if(j->first == start){
							dcassert(j->second->pos == j->second->end);
							j->second->pos = start;
							j->second->end = end;
							merged = true;
						}else if(j->second->end == start){
							j->second->end = end;
							merged = true;
						}
					}
					
					if(!merged)
						waiting.insert(make_pair(start, new Chunk(start, end)));
				}

				downloadedSize -= (end - start);
        	}else{
				uint16_t s = (uint16_t)(start / tthBlockSize);
				uint16_t e = (uint16_t)((end - 1) / tthBlockSize + 1);
        		markVerifiedBlock(s, e);
        	}

		}

		dcassert(end == i->second);
    }

	if(waiting.empty()){
		dumpVerifiedBlocks();

		dcassert(verifiedBlocks.size() == 1 && 
			    verifiedBlocks.begin()->first == 0 &&
			    verifiedBlocks.begin()->second == (fileSize - 1) / tthBlockSize + 1);

        return true;
    }

	// double smallest size when last verify failed
	minChunkSize = min(minChunkSize * 2, (int64_t)tthBlockSize);

	return false;
}

void FileChunksInfo::markVerifiedBlock(uint16_t start, uint16_t end)
{
	dcassert(start < end);

	dcdebug("markVerifiedBlock(%hu, %hu)\n", start, end);
	Lock l(cs);

	BlockMap::iterator i = verifiedBlocks.begin();

	for(; i != verifiedBlocks.end(); i++)
	{
		// check dupe
		if(start >= i->first && start < i->second){
			//dcassert(0);
			return;
		}

        if(i->second == start){
            i->second = end;
            break;
        }
    }

	if(i == verifiedBlocks.end())
		i = verifiedBlocks.insert(make_pair(start, end)).first;

	BlockMap::iterator j = verifiedBlocks.begin();
	for(; j != verifiedBlocks.end(); j++)
	{
        if(j->first == end)
        {
            i->second = j->second;
            verifiedBlocks.erase(j);
            break;
        }
    }

	verifiedSize = min(verifiedSize + (end - start) * tthBlockSize, fileSize);

}

void FileChunksInfo::abandonChunk(int64_t start)
{
	Lock l(cs);

	Chunk::Iter i = running.find(start);
	dcassert(i != running.end());
	Chunk* chunk = i->second;
	
	dcassert(chunk->pos > start);

	// because verify() may generate a waiting chunk from running chunk
	// the start of running chunk is useless, the real start must be caculated
	for(Chunk::Iter j = waiting.begin(); j != waiting.end(); j++){
		if(j->second->end > start && j->second->end <= chunk->pos){
			dcassert( j->second->end < chunk->pos);
			start = j->second->end;
			break;
		}
	}

	for(Chunk::Iter j = running.begin(); j != running.end(); j++){
		if(j->second != chunk && j->second->end > start && j->second->end <= chunk->pos){
			dcassert( j->second->end < chunk->pos);
			start = j->second->end;
			break;
		}
	}

	int64_t oldPos = chunk->pos;

	// don't abandon verified chunk
	if(chunk->pos - start < tthBlockSize * 2){
		chunk->pos = start;
	}else{
		int64_t firstBlockStart = start;
		
		if(start % tthBlockSize){
			firstBlockStart += (tthBlockSize - (start % tthBlockSize));
		}
		
		int64_t len = chunk->pos - firstBlockStart;
		
		// if the first tth block is verified, only cut the tail
		if(isVerified(firstBlockStart, len)){
			chunk->pos = firstBlockStart + len;
		}else{
			chunk->pos = start;
		}
	}

	// adjust downloade size
	downloadedSize -= (oldPos - chunk->pos);

	selfCheck();
}

bool FileChunksInfo::isSource(const PartsInfo& partsInfo)
{
	Lock l(cs);

	dcassert(partsInfo.size() % 2 == 0);
	
	BlockMap::const_iterator i  = verifiedBlocks.begin();
	for(PartsInfo::const_iterator j = partsInfo.begin(); j != partsInfo.end(); j+=2){
		while(i != verifiedBlocks.end() && i->second <= *j)
			i++;

		if(i == verifiedBlocks.end() || !(i->first <= *j && i->second >= *(j+1)))
			return true;
	}
	
	return false;

}

void FileChunksInfo::getPartialInfo(PartsInfo& partialInfo){
	Lock l(cs);

	size_t maxSize = min(verifiedBlocks.size() * 2, (size_t)510);
	partialInfo.reserve(maxSize);

	BlockMap::const_iterator i = verifiedBlocks.begin();
	for(; i != verifiedBlocks.end() && partialInfo.size() < maxSize; i++){
		partialInfo.push_back(i->first);
		partialInfo.push_back(i->second);
	}
}

/**
 * return first chunk size after split
 */
/*
int64_t splitChunk(int64_t chunkSize, int64_t chunkSpeed, int64_t estimatedSpeed)
{
	if(chunkSpeed <= 0) chunkSpeed = 1;
	if(estimatedSpeed <= 0) estimatedSpeed = DEFAULT_SPEED;

	if(chunkSpeed * 200 < estimatedSpeed) return 0;
	if(chunkSpeed > estimatedSpeed * 200) return chunkSize;

	return chunkSize * chunkSpeed / (chunkSpeed + estimatedSpeed);
}
*/
int64_t FileChunksInfo::getChunk(const PartsInfo& partialInfo, int64_t /*estimatedSpeed*/){
	if(partialInfo.empty()){
		return -1;
	}

	Lock l(cs);

	// Convert block index to file position
	vector<int64_t> posArray;
	posArray.reserve(partialInfo.size());

	for(PartsInfo::const_iterator i = partialInfo.begin(); i != partialInfo.end(); i++)
		posArray.push_back(min(fileSize, (int64_t)(*i) * (int64_t)tthBlockSize));

	// Check waiting chunks
	for(Chunk::Map::iterator i = waiting.begin(); i != waiting.end(); i++){
		Chunk* chunk = i->second;
		for(vector<int64_t>::const_iterator j = posArray.begin(); j < posArray.end(); j+=2){
			if( (*j <= chunk->pos && chunk->pos < *(j+1)) || (chunk->pos <= *j && *j < chunk->end) ){
				int64_t b = max(chunk->pos, *j);
				int64_t e = min(chunk->end, *(j+1));

				if(chunk->pos != b && chunk->end != e){
					int64_t tmp = chunk->end; 
					chunk->end = b;

					waiting.insert(make_pair(e, new Chunk(e, tmp)));

				}else if(chunk->end != e){
					chunk->pos = e;
					waiting.erase(i);
					waiting.insert(make_pair(chunk->pos, chunk));
				}else if(chunk->pos != b){
					chunk->end = b;
				}else{
					waiting.erase(i);
					delete chunk;
				}

				running.insert(make_pair(b, new Chunk(b, e)));
				selfCheck();
				return b;
			}
		}
	}

	// Check running chunks
	Chunk::Iter maxBlockIter = running.end();
	int64_t maxBlockStart = 0;
    int64_t maxBlockEnd   = 0;

	for(Chunk::Iter i = running.begin(); i != running.end(); i++){
		int64_t b = i->second->pos;
		int64_t e = i->second->end;
		
		if(e - b < minChunkSize * 2) continue;
        b = b + (e - b) / 2; //@todo speed

		dcassert(b > i->second->pos);

		for(vector<int64_t>::const_iterator j = posArray.begin(); j < posArray.end(); j+=2){
			int64_t bb = max(*j, b);
			int64_t ee = min(*(j+1), e);

			if(ee - bb > maxBlockEnd - maxBlockStart){
				maxBlockStart = bb;
				maxBlockEnd   = ee;
				maxBlockIter  = i;
			}
		}
	}

	if(maxBlockEnd - maxBlockStart >= minChunkSize){
		int64_t tmp = maxBlockIter->second->end;
		maxBlockIter->second->end = maxBlockStart;

		running.insert(make_pair(maxBlockStart, new Chunk(maxBlockStart, maxBlockEnd)));

		if(tmp > maxBlockEnd){
			waiting.insert(make_pair(maxBlockEnd, new Chunk(maxBlockEnd, tmp)));
		}

		selfCheck();
		return maxBlockStart;
	}

	return -2;

}

bool FileChunksInfo::isVerified(int64_t startPos, int64_t& len){
	if(len <= 0) return false;

	Lock l(cs);

	BlockMap::const_iterator i  = verifiedBlocks.begin();
	for(; i != verifiedBlocks.end(); i++)
	{
		int64_t first  = (int64_t)i->first  * (int64_t)tthBlockSize;
		int64_t second = (int64_t)i->second * (int64_t)tthBlockSize;
		
		second = min(second, fileSize);

		if(first <= startPos && startPos < second){
			len = min(len, second - startPos);
			return true;
		}
	}

	return false;
}

bool FileChunksInfo::verifyBlock(int64_t anyPos, const TigerTree& aTree, const string& tempTargetName)
{
	Lock l(cs);

	// which block the pos belong to?
	uint16_t block = (uint16_t)(anyPos / tthBlockSize);
	int64_t   start = block * tthBlockSize;
	int64_t   end   = min(start + tthBlockSize, fileSize);
	size_t    len   = (size_t)(end - start);

	dcdebug("verifyBlock %I64d - %I64d\n", start, end);
	if(len == 0){
		dcassert(0);
		return true;
	}

	// all block bytes ready?
	for(Chunk::Iter i = waiting.begin(); i != waiting.end(); i++)
		if(start < i->second->end && i->second->pos < end) return true;

	for(Chunk::Iter i = running.begin(); i != running.end(); i++){
		if(i->second->pos < i->second->end && start < i->second->end && i->second->pos < end){
			dcdebug("running %I64d - %I64d\n", i->second->pos, i->second->end);
			return true;
		}
	}

	// block not verified yet?
	int64_t tmp = len;
	if(isVerified(start, tmp)){
		dcassert(tmp == len);
		return true;
	}

	// yield to allow other threads flush their data
	Thread::yield();

	try {
		// for exception free
		AutoArray<unsigned char> buf(len);
		//auto_ptr<unsigned char> buf(new unsigned char[len]);

		// reread all bytes from stream
		SharedFileStream file(tempTargetName, start);

		size_t tmpLen = len;
		file.read(buf, tmpLen);

		dcassert(len == tmpLen);

		// check against tiger tree
		return verify(buf, start, end, aTree);
	} catch(...) {
		// maybe not enough memory
	}
	
	return true;
}

void FileChunksInfo::selfCheck() const
{
#ifdef _DEBUG

	// check running
	for(Chunk::Iter i = running.begin(); i != running.end();){
		dcassert(i->first <= i->second->pos);
		dcassert(i->second->pos <= i->second->end);

		//int64_t tmp = i->second->end;
		i++;

		if(i != running.end()){
			int64_t start = i->first;
			Chunk* x = i->second;
			dcassert(start <= x->pos );
		}
	}

	// check waiting
	for(Chunk::Iter i = waiting.begin(); i != waiting.end();){
		dcassert(i->first == i->second->pos);
		dcassert(i->second->pos <= i->second->end);

		Chunk* c1 = i->second;
		int64_t s1 = i->first;

		for(Chunk::Iter j = running.begin(); j != running.end(); j++){
			Chunk* c2 = j->second;
			int64_t s2 = j->first;

			if(c2->pos == c2->end) continue;

			dcassert(s1 != s2);

			if(s1 > s2)	dcassert(s1 >= c2->end || c1->end <= c2->pos);
			if(s1 < s2)	dcassert(c1->end <= c2->pos);
		}			

		i++;

		if(i != waiting.end()){
			dcassert(c1->end <= i->first);
		}
	}
#endif
}

void FileChunksInfo::getAllChunks(vector<int64_t>& v, int type) // type: 0 - downloaded, 1 - running, 2 - verified
{
	Lock l(cs);

	switch(type) {
	case 0 :
		v.push_back(0);
		for(Chunk::Iter i = waiting.begin(); i != waiting.end(); ++i) {
			v.push_back(i->second->pos);
			v.push_back(i->second->end);
		}
		for(Chunk::Iter i = running.begin(); i != running.end(); ++i) {
			v.push_back(i->second->pos);
			v.push_back(i->second->end);
		}
		v.push_back(fileSize);
		break;
	case 1 :
		for(Chunk::Iter i = running.begin(); i != running.end(); ++i) {
			if(i->second->download == NULL) continue;

			if(!v.empty() && (v.back() > i->first)) {
				v.pop_back();
				v.push_back(i->first);
			}
			v.push_back(i->second->pos);
			v.push_back(i->first + i->second->download->getChunkSize());
		}
		break;
	case 2 :
		for(BlockMap::const_iterator i = verifiedBlocks.begin(); i != verifiedBlocks.end(); ++i) {
			v.push_back((int64_t)i->first * (int64_t)tthBlockSize);
			v.push_back((int64_t)i->second * (int64_t)tthBlockSize);
		}
		break;
	}
	sort(v.begin(), v.end());
}
