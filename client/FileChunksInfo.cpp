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

#include "DownloadManager.h"
#include "FileChunksInfo.h"

#include "SettingsManager.h"
#include "Winioctl.h"

CriticalSection SharedFileStream::critical_section;
SharedFileStream::SharedFileHandleMap SharedFileStream::file_handle_pool;

vector<FileChunksInfo::Ptr> FileChunksInfo::vecAllFileChunksInfo;
CriticalSection FileChunksInfo::hMutexMapList;

typedef BOOL (__stdcall *SetFileValidDataFunc) (HANDLE, LONGLONG); 
static SetFileValidDataFunc setFileValidData = NULL;

// NOTE: THIS MUST EQUAL TO HashManager::Hasher::MIN_BLOCK_SIZE
enum { MIN_BLOCK_SIZE = 65536 };

FileChunksInfo::Ptr FileChunksInfo::Get(const string& name)
{
    Lock l(hMutexMapList);

	for(vector<Ptr>::iterator i = vecAllFileChunksInfo.begin(); i != vecAllFileChunksInfo.end(); i++){
		if((*i)->tempTargetName == name){
			return (*i);
		}
	}

	return NULL;
}

void FileChunksInfo::Free(const string& name)
{
    Lock l(hMutexMapList);

	for(vector<FileChunksInfo::Ptr>::iterator i = vecAllFileChunksInfo.begin(); i != vecAllFileChunksInfo.end(); i++){
		if((*i)->tempTargetName == name ){
			vecAllFileChunksInfo.erase(i);
			return;
		}
	}

	dcassert(0);
}

FileChunksInfo::FileChunksInfo(const string& name, int64_t size, const vector<int64_t>* chunks) 
	: tempTargetName(name), iFileSize(0), iVerifiedSize(0), noFreeBlock(false)
{
	hMutexMapList.enter();
	vecAllFileChunksInfo.push_back(this);
	hMutexMapList.leave();

	setFileSize(size);

	if(chunks != NULL){
		waiting.clear();
		for(vector<int64_t>::const_iterator i = chunks->begin(); i < chunks->end(); i++, i++)
			waiting.insert(make_pair(*i, new Chunk(*i, *(i+1))));
	}

    iDownloadedSize = iFileSize;
	for(Chunk::Iter i = waiting.begin(); i != waiting.end(); i++)
        iDownloadedSize -= (i->second->end - i->second->pos);
}

void FileChunksInfo::setFileSize(const int64_t& size)
{
	dcassert(iFileSize == 0);
	dcassert(waiting.empty());

	if(size > 0){
		iFileSize = size;
		waiting.insert(make_pair(0, new Chunk(0, size)));

		tthBlockSize = max((size_t)TigerTree::calcBlockSize(iFileSize, 10), (size_t)MIN_BLOCK_SIZE);
		minChunkSize = max((int64_t)1048576, (int64_t)(iFileSize / 100));
		minChunkSize = minChunkSize - (minChunkSize % tthBlockSize);
	}
}

FileChunksInfo::~FileChunksInfo()
{
	dcdebug("Delete file chunks info: %s, waiting = %d, running = %d\n", tempTargetName.c_str(), waiting.size(), running.size());

	for(Chunk::Iter i = waiting.begin(); i != waiting.end(); i++)
		delete i->second;

	for(Chunk::Iter i = running.begin(); i != running.end(); i++)
		delete i->second;
}

int FileChunksInfo::addChunkPos(int64_t start, int64_t pos, size_t& len)
{
	Lock l(cs);

	Chunk::Iter i = running.find(start);
	dcassert(i != running.end());
	Chunk* chunk = i->second;

	if(chunk->pos != pos) return CHUNK_LOST;
	
	len = (size_t)min((int64_t)len, chunk->end - chunk->pos);

	chunk->pos += len;
	iDownloadedSize += len;

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
	if(chunk->pos >= i->first + chunk->download->getSegmentSize()){
		return CHUNK_OVER;
	}
	return NORMAL_EXIT;
}

void FileChunksInfo::setDownload(int64_t chunk, Download* d, User::Ptr u, bool noStealth)
{
	map<int64_t, Chunk*>::iterator i = running.find(chunk);

	if(i == running.end()){
		dcassert(0);
		return;
	}

	int64_t chunkEnd = i->second->end;
	i->second->download = d;
	i->second->user = u;
	if(noStealth) {
		i->second->download->setFlag(Download::FLAG_CHUNK_TRANSFER);
		chunkEnd = min(i->second->end, i->second->pos + min(minChunkSize, iFileSize - i->second->pos));
	}
	i->second->download->setSegmentSize(chunkEnd - i->second->pos);
}

int64_t FileChunksInfo::getChunk(int64_t _speed)
{
	Chunk* chunk = NULL;

	if(_speed == 0) _speed = DEFAULT_SPEED;

	Lock l(cs);
	dcdebug("getChunk speed = %I64d, running = %d waiting = %d\n", _speed, running.size(), waiting.size());

	// if there is any waiting chunk, return it
	if(!waiting.empty()){
		chunk = waiting.begin()->second;

		waiting.erase(waiting.begin());
		running.insert(make_pair(chunk->pos, chunk));

		noFreeBlock = false;
		return chunk->pos;
	}

	if(running.empty()){
		noFreeBlock = true;
		dcdebug("running empty, getChunk return -1\n");
		return -1;
	}
	
	// find the max time-left running chunk
	Chunk*  maxChunk = NULL;
	int64_t maxTimeLeft = 0;
	int64_t speed;

	for(map<int64_t, Chunk*>::iterator i = running.begin(); i != running.end(); i++)
	{
		chunk = i->second;

		if((chunk->end - chunk->pos) < minChunkSize*2)
			continue;

		if(chunk->pos > i->first){
			speed = chunk->download->getAverageSpeed();
			if(speed == 0) speed = 1;
		}else{
			speed =  chunk->user->getLastDownloadSpeed();
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
	/*if(maxTimeLeft < 15){
		for(map<int64_t, Chunk*>::iterator i = running.begin(); i != running.end(); i++)
		{
			if(chunk->pos == i->first){
				chunk->overlappedCount++;
				noFreeBlock = false;
				return i->first;
			}
		}
	}*/

	if(maxChunk == NULL) {
		noFreeBlock = true;
		return -1;
	}

	// split the max time-left running chunk
	int64_t b = maxChunk->pos;
	int64_t e = maxChunk->end;

	speed = maxChunk->download ? maxChunk->download->getAverageSpeed() : DEFAULT_SPEED;

	if(speed == 0){
		speed =  chunk->user->getLastDownloadSpeed();
	}

	if(speed == 0){
		speed = DEFAULT_SPEED;
	}

	double x = 1.0 + (double)_speed / (double)speed;

	// too lag than orignal source
	if(x < 1.01) { 
		noFreeBlock = true;
		return -1;
	}

	int64_t n = max(b + 1, b + (int64_t)((double)(e - b) / x));

	// align to tiger tree block size
	if(n - b > tthBlockSize * 3 && e - n > tthBlockSize * 3)
		n = n - (n % tthBlockSize);

	maxChunk->end = n;

	running.insert(make_pair(n, new Chunk(n, e)));

	dcdebug("split running chunk (%I64d , %I64d) * %I64d Bytes/s -> (%I64d , %I64d) * %I64d Bytes/s \n", b, e, speed, n, e, _speed);
	noFreeBlock = false;
	return n;
}

bool FileChunksInfo::hasFreeBlock() { return !noFreeBlock; }

void FileChunksInfo::putChunk(int64_t start)
{
	Lock l(cs);

	dcdebug("put chunk %I64d\n", start);

	Chunk* prev = NULL;

	for(map<int64_t, Chunk*>::iterator i = running.begin(); i != running.end(); i++)
	{
		Chunk* chunk = i->second;

		if(i->first == start){

			if(i->second->overlappedCount){
				i->second->overlappedCount--;
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
				noFreeBlock = false;
				waiting.insert(make_pair(chunk->pos, chunk));				
			}

			running.erase(i);
			return;
		}

		prev = chunk;
	}

//	dcassert(0);
}

string FileChunksInfo::getFreeChunksString()
{
	vector<int64_t> tmp;

	{
		Lock l(cs);

		Chunk::Map tmpMap = waiting;
		copy(running.begin(), running.end(), inserter(tmpMap, tmpMap.end())); // sort

		tmp.reserve(tmpMap.size() * 2);

		for(Chunk::Iter i = tmpMap.begin(); i != tmpMap.end(); i++){
			tmp.push_back(i->second->pos);
			tmp.push_back(i->second->end);
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
	for(vector<int64_t>::iterator i = tmp.begin(); i != tmp.end(); i++)
		ret += Util::toString(*i) + " ";

	return ret;
}

string FileChunksInfo::getVerifiedBlocksString()
{
	Lock l(cs);

	string ret;

	for(BlockMap::iterator i = verifiedBlocks.begin(); i != verifiedBlocks.end(); i++)
		ret += Util::toString(i->first) + " " + Util::toString(i->second) + " ";

	return ret;
}


bool FileChunksInfo::doLastVerify(const TigerTree& aTree, string aTarget)
{
	if(tthBlockSize != aTree.getBlockSize())
		return true;

	dcdebug("DoLastVerify %I64d bytes %d%% verified\n", iVerifiedSize , (int)(iVerifiedSize * 100 / iFileSize)); 
	dcdebug("VerifiedBlocks size = %d\n", verifiedBlocks.size());

	// Convert to unverified blocks
	vector<int64_t> CorruptedBlocks;
	int64_t start = 0;

	{
		Lock l(cs);

		if(BOOLSETTING(CHECK_UNVERIFIED_ONLY) && (iVerifiedSize*4/3 >= iFileSize))
				return true;

		// This is only called when download finish
		// Because buffer is used during download, the disk data maybe incorrect
	    dcassert(waiting.empty() && running.size() == 1 && running.begin()->second->pos == running.begin()->second->end);


		verifiedBlocks.clear();
	}

	::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
	// Open file
	char buf[512*1024];

	SharedFileStream file(tempTargetName, 0, 0);
	TigerTree tth(max((int64_t)TigerTree::calcBlockSize(file.getSize(), 10), (int64_t)tthBlockSize));

	size_t n = 0;
	size_t n2 = 524288;
	int64_t hashed = 0;
	while( (n = file.read(buf, n2)) > 0) {
		tth.update(buf, n);
		n2 = 524288;
		hashed = hashed + n;
		DownloadManager::getInstance()->fire(DownloadManagerListener::Verifying(), aTarget, hashed);
	}
	tth.finalize();

	int64_t end;
	for(int i = 0; i < tth.getLeaves().size(); i++) {
		end = min(start + tth.getBlockSize(), file.getSize());
		if(!(tth.getLeaves()[i] == aTree.getLeaves()[i])) {
       		if(!CorruptedBlocks.empty() && *(CorruptedBlocks.rbegin()) == start) {
				*(CorruptedBlocks.rbegin()) = end;
	        } else {
				CorruptedBlocks.push_back(start);
	        	CorruptedBlocks.push_back(end);
	        }
			iDownloadedSize -= (end - start);
        } else {
			u_int16_t s = (u_int16_t)(start / tthBlockSize);
			u_int16_t e = (u_int16_t)((end - 1) / tthBlockSize + 1);
        	markVerifiedBlock(s, e);
		}
		start = end;
	}

	::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

	if(CorruptedBlocks.empty()){
		dcdebug("VerifiedBlocks size = %d\n", verifiedBlocks.size());


		dcassert(verifiedBlocks.size() == 1 && 
			    verifiedBlocks.begin()->first == 0 &&
			    verifiedBlocks.begin()->second == (iFileSize - 1) / tthBlockSize + 1);

        return true;
    }

	{
		Lock l(cs);
		for(vector<int64_t>::iterator i = CorruptedBlocks.begin(); i != CorruptedBlocks.end(); i++, i++) {
			waiting.insert(make_pair(*i, new Chunk(*i, *(i+1))));
		}

		// double smallest size when last verify failed
		minChunkSize = min(minChunkSize * 2, (int64_t)tthBlockSize);
		noFreeBlock = false;
	}

	return false;
}

void FileChunksInfo::markVerifiedBlock(u_int16_t start, u_int16_t end)
{
	dcassert(start < end);

	dcdebug("markVerifiedBlock(%hu, %hu)\n", start, end);
	Lock l(cs);

	BlockMap::iterator i = verifiedBlocks.begin();

	for(; i != verifiedBlocks.end(); i++)
	{
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

	iVerifiedSize += (end - start) * tthBlockSize;

}

SharedFileStream::SharedFileStream(const string& name, int64_t _pos, int64_t size, bool shareDelete) 
    : pos(_pos)
{
	{
		Lock l(critical_section);

		if(file_handle_pool.count(name) > 0){

	        shared_handle_ptr = file_handle_pool[name];
			shared_handle_ptr->ref_cnt++;

			return;

		}else{

	        shared_handle_ptr = new SharedFileHandle(name, shareDelete);
	        shared_handle_ptr->ref_cnt = 1;
			file_handle_pool[name] = shared_handle_ptr;

		}
	}

	if((size > 0) && BOOLSETTING(ANTI_FRAG)){
		Lock l(*shared_handle_ptr);

		DWORD x = (DWORD)(size >> 32);
		::SetFilePointer(shared_handle_ptr->handle, (DWORD)(size & 0xffffffff), (PLONG)&x, FILE_BEGIN);
		::SetEndOfFile(shared_handle_ptr->handle);

		if(setFileValidData != NULL){
			if(!setFileValidData(shared_handle_ptr->handle, size))
				dcdebug("SetFileValidData error %d\n", GetLastError());
		}
    }

}

void ensurePrivilege()
{
	HANDLE			 hToken;
	TOKEN_PRIVILEGES privilege;
	LUID			 luid;
	DWORD			 dwRet;
	OSVERSIONINFO    osVersionInfo;

	typedef BOOL (__stdcall *OpenProcessTokenFunc) (HANDLE, DWORD, PHANDLE);
	typedef BOOL (__stdcall *LookupPrivilegeValueFunc) (LPCTSTR, LPCTSTR, PLUID);
	typedef BOOL (__stdcall *AdjustTokenPrivilegesFunc) (HANDLE, BOOL, PTOKEN_PRIVILEGES, DWORD, PTOKEN_PRIVILEGES, PDWORD);

	OpenProcessTokenFunc openProcessToken = NULL;
	LookupPrivilegeValueFunc lookupPrivilegeValue = NULL;
	AdjustTokenPrivilegesFunc adjustTokenPrivileges = NULL;

	osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
	dwRet = GetVersionEx(&osVersionInfo);

	if(!dwRet){
		dcdebug("GetVersionEx error : %d\n", GetLastError());
		return;
	}

	if(osVersionInfo.dwMajorVersion < 5) return;
	if(osVersionInfo.dwMajorVersion == 5 && osVersionInfo.dwMinorVersion < 1) return;

	HMODULE hModule;

	hModule = GetModuleHandle(_T("advapi32"));
    if(hModule == NULL) return;

    openProcessToken = (OpenProcessTokenFunc)GetProcAddress(hModule, "OpenProcessToken");

#ifdef  UNICODE
    lookupPrivilegeValue = (LookupPrivilegeValueFunc)GetProcAddress(hModule, "LookupPrivilegeValueW");
#else
	lookupPrivilegeValue = (LookupPrivilegeValueFunc)GetProcAddress(hModule, "LookupPrivilegeValueA");
#endif
    adjustTokenPrivileges = (AdjustTokenPrivilegesFunc)GetProcAddress(hModule, "AdjustTokenPrivileges");

	if(openProcessToken == NULL || lookupPrivilegeValue == NULL || adjustTokenPrivileges == NULL)
		return;

	dcdebug("Os version %d.%d\n", osVersionInfo.dwMajorVersion, osVersionInfo.dwMinorVersion);

	dwRet = openProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken);

	if(!dwRet){
		dcdebug("OpenProcessToken error : %d\n", GetLastError());
		return;
	}

	dwRet = lookupPrivilegeValue(NULL, SE_MANAGE_VOLUME_NAME, &luid);
	if(!dwRet){
		dcdebug("LookupPrivilegeValue error : %d\n", GetLastError());
		goto cleanup;
	}

	privilege.PrivilegeCount = 1;
	privilege.Privileges[0].Luid = luid;
	privilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	dwRet = adjustTokenPrivileges(hToken, FALSE, &privilege, 0, NULL, NULL);

	if(!dwRet){
		dcdebug("AdjustTokenPrivileges error : %d\n", GetLastError());
		goto cleanup;
	}

	hModule = GetModuleHandle(_T("kernel32"));
    if(hModule)
        setFileValidData = (SetFileValidDataFunc)GetProcAddress(hModule, "SetFileValidData");

	dcdebug("ensurePrivilege done.\n");
cleanup:
	CloseHandle(hToken);

}

bool FileChunksInfo::isSource(const PartsInfo& partsInfo)
{
	Lock l(cs);

	dcassert(partsInfo.size() % 2 == 0);
	
	BlockMap::iterator i  = verifiedBlocks.begin();
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

	BlockMap::iterator i = verifiedBlocks.begin();
	for(; i != verifiedBlocks.end() && partialInfo.size() < maxSize; i++){
		partialInfo.push_back(i->first);
		partialInfo.push_back(i->second);
	}
}

// TODO: speed
int64_t FileChunksInfo::getChunk(const PartsInfo& partialInfo, int64_t _speed){
	if(partialInfo.empty()){
		return getChunk(_speed);
	}

	Lock l(cs);

	// Convert block index to file position
	vector<int64_t> posArray;
	posArray.reserve(partialInfo.size());

	for(PartsInfo::const_iterator i = partialInfo.begin(); i != partialInfo.end(); i++)
		posArray.push_back(min(iFileSize, (int64_t)(*i) * (int64_t)tthBlockSize));

	// Check free blocks
	for(Chunk::Iter i = waiting.begin(); i != waiting.end(); i++){
		Chunk* chunk = i->second;
		for(vector<int64_t>::iterator j = posArray.begin(); j < posArray.end(); j+=2){
			if( (*j <= chunk->pos && chunk->pos < *(j+1)) || (chunk->pos <= *j && *j < chunk->end) ){
				int64_t b = max(chunk->pos, *j);
				int64_t e = min(chunk->end, *(j+1));

				if(chunk->pos != b && chunk->end != e){
					int64_t tmp = chunk->end; 
					chunk->end = b;

					waiting.insert(make_pair(e, new Chunk(e, tmp)));

				}else if(chunk->end != e){
					waiting.erase(i);
					chunk->pos = e;
					waiting.insert(make_pair(e, chunk));
				}else if(chunk->pos != b){
					chunk->end = b;
				}else{
					waiting.erase(i);
				}

				running.insert(make_pair(b, new Chunk(b, e)));
				noFreeBlock = false;
				return b;
			}
		}
	}

	// Check running blocks
	Chunk::Iter maxBlockIter = running.end();
	int64_t maxBlockStart = 0;
    int64_t maxBlockEnd   = 0;

	for(Chunk::Iter i = running.begin(); i != running.end(); i++){
		int64_t b = i->second->pos;
		int64_t e = i->second->end;
		
		if(e - b < MIN_BLOCK_SIZE * 3) continue;

        b = b + (e - b) / 2;
		b = b - (b % MIN_BLOCK_SIZE);

		dcassert(b > i->second->pos);

		for(vector<int64_t>::iterator j = posArray.begin(); j < posArray.end(); j+=2){
			if(*j <= b  && *(j+1) > b ){
				int64_t e = min(i->second->end, e);

				if(e - b > maxBlockEnd - maxBlockStart){
					maxBlockStart = b;
					maxBlockEnd   = e;
					maxBlockIter  = i;
				}
			}
		}
	}

	if(maxBlockEnd > 0){
		int64_t tmp = maxBlockIter->second->end;
		maxBlockIter->second->end = maxBlockStart;

		running.insert(make_pair(maxBlockStart, new Chunk(maxBlockStart, maxBlockEnd)));

		if(tmp > maxBlockEnd){
			waiting.insert(make_pair(maxBlockEnd, new Chunk(maxBlockEnd, tmp)));
		}

		noFreeBlock = false;
		return maxBlockStart;
	}

	return -2;

}

SharedFileHandle::SharedFileHandle(const string& name, bool shareDelete)
{
	DWORD dwShareMode = FILE_SHARE_READ;
	DWORD dwDesiredAccess = GENERIC_READ;

	if(shareDelete){
		dwShareMode |= (FILE_SHARE_DELETE | FILE_SHARE_WRITE);
	}else{
		dwDesiredAccess |= GENERIC_WRITE;
	}

	handle = ::CreateFile(Text::utf8ToWide(name).c_str(), 
					dwDesiredAccess, 
					dwShareMode, 
					NULL, 
					OPEN_ALWAYS, 
					0,
					NULL);
	
	if(handle == INVALID_HANDLE_VALUE) {
		throw FileException(Util::translateError(GetLastError()));
	}

	if(!SETTING(ANTI_FRAG)){
		DWORD bytesReturned;
		DeviceIoControl(handle, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &bytesReturned, NULL);
	}
}

string GetPartsString(const PartsInfo& partsInfo)
{
	string ret;

	for(PartsInfo::const_iterator i = partsInfo.begin(); i < partsInfo.end(); i+=2){
		ret += Util::toString(*i) + "," + Util::toString(*(i+1)) + ",";
	}

	return ret.substr(0, ret.size()-1);
}

void FileChunksInfo::getAllChunks(vector<int64_t>& v, bool verified)
{
	Lock l(cs);

	if(verified) {
		for(BlockMap::iterator i = verifiedBlocks.begin(); i != verifiedBlocks.end(); i++) {
			v.push_back(i->first * tthBlockSize);
			v.push_back(i->second * tthBlockSize);
		}
	} else {
		for(Chunk::Iter i = waiting.begin(); i != waiting.end(); i++) {
			v.push_back(i->second->pos);
			v.push_back(i->second->end);
		}
		for(Chunk::Iter i = running.begin(); i != running.end(); i++) {
			v.push_back(i->second->pos);
			v.push_back(i->second->end);
		}
	}
	sort(v.begin(), v.end());
}