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

CriticalSection SharedFileStream::critical_section;
SharedFileStream::SharedFileHandleMap SharedFileStream::file_handle_pool;

vector<FileChunksInfo::Ptr> FileChunksInfo::vecAllFileChunksInfo;
CriticalSection FileChunksInfo::hMutexMapList;

typedef BOOL (__stdcall *SetFileValidDataFunc) (HANDLE, LONGLONG); 
static SetFileValidDataFunc setFileValidData = NULL;

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

	iBlockSize = max((size_t)TigerTree::calcBlockSize(iFileSize, 10), (size_t)SMALLEST_BLOCK_SIZE);

	iSmallestBlockSize = max((int64_t)1048576, (int64_t)(iFileSize / 100));
	iSmallestBlockSize = iSmallestBlockSize - (iSmallestBlockSize % iBlockSize);

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

int64_t FileChunksInfo::GetUndlStart()
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

	if((e - b) < (iSmallestBlockSize * 2)){
		dcdebug("GetUndlStart return -1 (%I64d)\n", iSmallestBlockSize);
		return -1;
	}

	int64_t n = b + (e - b) / 2;

	// align to tiger tree block size
	if(n - b > iBlockSize)
		n = n - (n % iBlockSize);

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
	if(iBlockSize != aTree.getBlockSize())
		return true;

	dcdebug("DoLastVerify %I64d bytes %d%% verified\n", iVerifiedSize , (int)(iVerifiedSize * 100 / iFileSize)); 
	dcdebug("VerifiedBlocks size = %d\n", mapVerifiedBlocks.size());

	// for exception free
	vector<unsigned char> buf;
    buf.reserve(iBlockSize);

	//Lock l(hMutex);

    // This is only called when download finish
    // Because buffer is used during download, the disk data maybe incorrect
    dcassert(vecFreeBlocks.empty() && vecRunBlocks.empty());

	// Convert to unverified blocks
    map<int64_t, int64_t> unVerifiedBlocks;
	int64_t start = 0;

	if(!BOOLSETTING(CHECK_UNVERIFIED_ONLY)) {
		unVerifiedBlocks.insert(make_pair(start, iFileSize));
		mapVerifiedBlocks.clear();
	} else {
	    for(map<int64_t, int64_t>::iterator i = mapVerifiedBlocks.begin();
        									i != mapVerifiedBlocks.end();
        	                                i++)
		{
			if(i->first > start){
    	        dcassert((i->first - start) % iBlockSize == 0);

	            unVerifiedBlocks.insert(make_pair(start, i->first));
        	}else{
    	        dcassert(start == 0);
	        }

			start = i->second;
	
		}

		if(start < iFileSize)
	        unVerifiedBlocks.insert(make_pair(start, iFileSize));
	}

	::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
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

			dcassert(end - start == len);

			TigerTree cur(iBlockSize);
            cur.update(&buf[0], len);
			cur.finalize();

			dcassert(cur.getLeaves().size() == 1);

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
				map<int64_t, int64_t>::iterator i = mapVerifiedBlocks.begin();

				for(; i != mapVerifiedBlocks.end(); i++) {
					if(i->second == start){
						i->second = end;
						break;
					}
				}

				if(i == mapVerifiedBlocks.end())
					i = mapVerifiedBlocks.insert(make_pair(start, end)).first;

				for(map<int64_t, int64_t>::iterator j = mapVerifiedBlocks.begin();
        											j != mapVerifiedBlocks.end();
										            j++) {
					if(j->first == end) {
						i->second = j->second;
						mapVerifiedBlocks.erase(j);
						break;
					}
				}

				iVerifiedSize += (end - start);
        	}
			DownloadManager::getInstance()->fire(DownloadManagerListener::Verifying(), sFilename, end);
		}

		dcassert(end == i->second);
    }

	::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
	if(vecFreeBlocks.empty()){
		dcdebug("VerifiedBlocks size = %d\n", mapVerifiedBlocks.size());


		dcassert(mapVerifiedBlocks.size() == 1 && 
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

    dcassert( ( end - start) % 1024 == 0 || end == iFileSize);

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
