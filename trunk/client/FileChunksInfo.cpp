/* 
 * Copyright (C) 2003-2004 RevConnect, http://www.revconnect.com
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
#include "DownloadManager.h"
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

FileChunksInfo::FileChunksInfo(const string& name, int64_t size, const vector<int64_t>* blocks) 
	: tempTargetName(name), iFileSize(0), iVerifiedSize(0)
{
	hMutexMapList.enter();
	vecAllFileChunksInfo.push_back(this);
	hMutexMapList.leave();

	SetFileSize(size);

	if(blocks){
		vecFreeBlocks = *blocks;
		sort(vecFreeBlocks.begin(), vecFreeBlocks.end());
	}

    iDownloadedSize = iFileSize;
    for(vector<int64_t>::iterator i = vecFreeBlocks.begin(); i < vecFreeBlocks.end(); i++, i++)
        iDownloadedSize -= ((*(i+1)) - (*i));
}

void FileChunksInfo::SetFileSize(const int64_t& size)
{
	dcassert(iFileSize == 0);
	dcassert(vecFreeBlocks.empty());

	if(size > 0){
		iFileSize = size;
		vecFreeBlocks.push_back(0);
		vecFreeBlocks.push_back(size);

		tthBlockSize = max((size_t)TigerTree::calcBlockSize(iFileSize, 10), (size_t)MIN_BLOCK_SIZE);
		minChunkSize = max((int64_t)1048576, (int64_t)(iFileSize / 100));
		minChunkSize = minChunkSize - (minChunkSize % tthBlockSize);
	}
}

int64_t FileChunksInfo::GetBlockEnd(int64_t begin)
{
	Lock l(cs);

	for(vector<int64_t>::iterator i = vecRunBlocks.begin(); i != vecRunBlocks.end(); i++, i++){
		if((*i) == begin){
			return *(i+1);
		}
	}

	return -1;
}


int FileChunksInfo::ValidBlock(int64_t start, size_t& len)
{
	Lock l(cs);

	for(vector<int64_t>::iterator i = vecRunBlocks.begin(); i != vecRunBlocks.end(); i++, i++){
		if((*i) == start){
			(*i) += len;
			iDownloadedSize += len;

			// block over
			if((*i) >= (*(i+1))){

				iDownloadedSize -= ((*i) - (*(i+1)));
				vecRunBlocks.erase(i, i + 2);

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

int64_t FileChunksInfo::GetUndlStart()
{
	dcdebug("GetUndlStart running = %d free = %d\n", vecRunBlocks.size(), vecFreeBlocks.size());
	Lock l(cs);

	// if have free blocks, return first one
	if(vecFreeBlocks.size() > 1){
		int64_t e = vecFreeBlocks[1];
		int64_t b = vecFreeBlocks[0];

		vecFreeBlocks.erase(vecFreeBlocks.begin(), vecFreeBlocks.begin() + 2);

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

	if((e - b) < (minChunkSize * 2)){
		dcdebug("GetUndlStart return -1 (%I64d)\n", minChunkSize);
		return -1;
	}

	int64_t n = b + (e - b) / 2;

	// align to tiger tree block size
	if(n - b > tthBlockSize)
		n = n - (n % tthBlockSize);

	(* (birr+1)) = n;

	vecRunBlocks.push_back(n);
	vecRunBlocks.push_back(e);

	sort(vecRunBlocks.begin(), vecRunBlocks.end());

	return n;
}

bool FileChunksInfo::hasFreeBlock() {
	Lock l(cs);
	if(vecFreeBlocks.size() > 0)
		return true;

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

	if((e - b) < (minChunkSize * 2)){
		return false;
	}

	return true;
}

void FileChunksInfo::PutUndlStart(int64_t start)
{

	Lock l(cs);

	for(vector<int64_t>::iterator i = vecRunBlocks.begin(); i < vecRunBlocks.end(); i++, i++)
	{
		if((*i) == start){
            if(i != vecRunBlocks.begin() && (*(i-1)) == start){ // continous blocks, concatenate it
				*(i-1) = *(i+1);
				vecRunBlocks.erase(i, i + 2);
			}else{
				addFreeChunk(make_pair(*i, *(i+1)));
				vecRunBlocks.erase(i, i + 2);
			}


			return;
		}
	}

	dcassert(vecRunBlocks.empty() && vecFreeBlocks.empty());
}

string FileChunksInfo::getFreeBlocksString()
{
	ostringstream os;

	Lock l(cs);
	vector<int64_t> tmp = vecFreeBlocks;
	copy(vecRunBlocks.begin(), vecRunBlocks.end(), back_inserter(tmp));
	sort(tmp.begin(), tmp.end());

	// merge
	dcdebug("Before merge : %d\n", tmp.size());
	if(tmp.size() > 2){
		vector<int64_t>::iterator prev = tmp.begin() + 1;
		vector<int64_t>::iterator i = prev + 1;
		while(i < tmp.end()){
			if(*i == *prev){
				*prev = *(i + 1);
				i = tmp.erase(i, i + 1);
			}else{
				prev = i + 1;
				i = i + 2;
			}
		}
	}
	dcdebug("After merge : %d\n", tmp.size());

	copy(tmp.begin(), tmp.end(), ostream_iterator<int64_t>(os, " "));

	return os.str();

}

string FileChunksInfo::getVerifiedBlocksString()
{
	Lock l(cs);

	ostringstream os;

	for(BlockMap::iterator i = verifiedBlocks.begin(); i != verifiedBlocks.end(); i++)
		os << i->first << " " << i->second << " ";

	return os.str();
}


bool FileChunksInfo::DoLastVerify(const TigerTree& aTree)
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

		// This is only called when download finish
		// Because buffer is used during download, the disk data maybe incorrect
  		_ASSERT(vecFreeBlocks.empty() && vecRunBlocks.empty());

		verifiedBlocks.clear();
	}

	::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
	// Open file
	char buf[512*1024];

	SharedFileStream file(tempTargetName, 0, 0);
	TigerTree tth(max((int64_t)TigerTree::calcBlockSize(file.getSize(), 10), (int64_t)tthBlockSize));

	size_t n = 0;
	size_t n2 = 512*1024;
	int64_t hashed = 0;
	while( (n = file.read(buf, n2)) > 0) {
		tth.update(buf, n);
		n2 = 512*1024;
		hashed = hashed + n;
		DownloadManager::getInstance()->fire(DownloadManagerListener::Verifying(), tempTargetName, hashed);
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

	Lock l(cs);
	copy(CorruptedBlocks.begin(), CorruptedBlocks.end(), back_inserter(vecFreeBlocks));

	// double smallest size when last verify failed
	minChunkSize = min(minChunkSize * 2, (int64_t)tthBlockSize);

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

SharedFileStream::SharedFileStream(const string& name, int64_t _pos, int64_t size) 
    : pos(_pos)
{
	{
		Lock l(critical_section);

		if(file_handle_pool.count(name) > 0){

	        shared_handle_ptr = file_handle_pool[name];
			shared_handle_ptr->ref_cnt++;

			return;

		}else{

	        shared_handle_ptr = new SharedFileHandle(name);
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
    lookupPrivilegeValue = (LookupPrivilegeValueFunc)GetProcAddress(hModule, "LookupPrivilegeValue");
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

void FileChunksInfo::addFreeChunk(const Chunk& chunk){
	
	dcassert(chunk.first < chunk.second);
	
	vector<int64_t>::iterator i = vecFreeBlocks.begin();

	while(i < vecFreeBlocks.end() && *i < chunk.first){
		dcassert(chunk.first >= *(i+1));
		i = i + 2;
	}

	dcassert(i == vecFreeBlocks.end() || *i >= chunk.second);
	
	// Todo: Merge with running chunks...

	// *** merge ***
	if(i != vecFreeBlocks.end() && i != vecFreeBlocks.begin() && chunk.first == *(i-1) && chunk.second == *i){
		*i = *(i-2);
		vecFreeBlocks.erase(i-2, i);
	}else if(i != vecFreeBlocks.end() && chunk.second == *i){
		*i = chunk.first;
	}else if(i != vecFreeBlocks.begin() && chunk.first == *(i-1)){
		*(i-1) = chunk.second;
	}else{
		i = vecFreeBlocks.insert(i, chunk.second);
		vecFreeBlocks.insert(i, chunk.first);
	}
}

SharedFileHandle::SharedFileHandle(const string& name)
{
	handle = ::CreateFile(Text::utf8ToWide(name).c_str(), 
					GENERIC_WRITE | GENERIC_READ, 
					FILE_SHARE_READ, 
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
