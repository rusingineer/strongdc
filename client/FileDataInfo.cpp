#include "stdinc.h"
#include "DCPlusPlus.h"

#include "FileDataInfo.h"
#include "SettingsManager.h"
#include "QueueManager.h"

#define MAPPING_SIZE 2000000

vector<FileDataInfo*> FileDataInfo::vecAllFileDataInfo;
HANDLE FileDataInfo::hMutexMapList = CreateMutex(NULL, FALSE, NULL);

__int64 FileDataInfo::GetBlockEnd(__int64 begin)
{
	WaitForSingleObject(hMutex, INFINITE);

	for(vector<__int64>::iterator i = vecRunBlocks.begin(); i != vecRunBlocks.end(); i++, i++){
		if((*i) == begin){
			ReleaseMutex(hMutex);
			return *(i+1);
		}
	}
	ReleaseMutex(hMutex);
	return -1;
}


FileDataInfo::FileDataInfo(const string& name, __int64 size, const vector<__int64>* blocks) : sFileName(name), iFileSize(size)
{
	WaitForSingleObject(hMutexMapList, INFINITE);
	vecAllFileDataInfo.push_back(this);
	ReleaseMutex(hMutexMapList);

	hMutex = CreateMutex(NULL, FALSE, NULL);

	dcassert(hMutex != NULL);

	if(blocks == NULL){
		vecFreeBlocks.push_back(0);
		vecFreeBlocks.push_back(size);
	} else
		copy(blocks->begin(), blocks->end(), back_inserter(vecFreeBlocks));

    iDownloadedSize = iFileSize;
    for(vector<__int64>::iterator i = vecFreeBlocks.begin(); i < vecFreeBlocks.end(); i++, i++)
        iDownloadedSize -= ((*(i+1)) - (*i));
}



FileDataInfo* FileDataInfo::GetFileDataInfo(string name)
{
	WaitForSingleObject(hMutexMapList, INFINITE);

	for(vector<FileDataInfo*>::iterator i = vecAllFileDataInfo.begin(); i != vecAllFileDataInfo.end(); i++){
		if((*i)->sFileName == name){
			ReleaseMutex(hMutexMapList);
			return (*i);
		}
	}

	ReleaseMutex(hMutexMapList);
	return NULL;
}
                                                                  	
int FileDataInfo::ValidBlock(__int64 start, const void* buf, __int64 len)
{

	WaitForSingleObject(hMutex, INFINITE);

	for(vector<__int64>::iterator i = vecRunBlocks.begin(); i != vecRunBlocks.end(); i++, i++){
		if((*i) == start){
			(*i) += len;
			iDownloadedSize += len;

			// block over
			if((*i) >= (*(i+1))){

				iDownloadedSize -= ((*i) - (*(i+1)));
				vecRunBlocks.erase(i + 1);
				vecRunBlocks.erase(i);

				if(vecFreeBlocks.empty() && vecRunBlocks.empty()){
					ReleaseMutex(hMutex);
					return FILE_OVER;
				}

				ReleaseMutex(hMutex);	
				return BLOCK_OVER;
			}
			ReleaseMutex(hMutex);
			return NORMAL_EXIT;
		}
	}

	ReleaseMutex(hMutex);

	return WRONG_POS;
}

__int64 FileDataInfo::GetUndlStart(int maxSegments)
{
	WaitForSingleObject(hMutex, INFINITE);

	// if have free blocks, return first one
	if(vecFreeBlocks.size() > 1){
		__int64 e = vecFreeBlocks[1];
		__int64 b = vecFreeBlocks[0];

		vecFreeBlocks.erase(vecFreeBlocks.begin());
		vecFreeBlocks.erase(vecFreeBlocks.begin());

		vecRunBlocks.push_back(b);
		vecRunBlocks.push_back(e);

		sort(vecRunBlocks.begin(), vecRunBlocks.end());

		ReleaseMutex(hMutex);
		return b;
	}

	if(vecRunBlocks.empty()){
		ReleaseMutex(hMutex);
		return -1;
	}
	
	// reassign current run blocks

	__int64 blen = 0;
	vector<__int64>::iterator birr = vecRunBlocks.begin();

	// find the biggest block
	for(vector<__int64>::iterator i = vecRunBlocks.begin(); i < vecRunBlocks.end(); i++, i++)
	{
		if((*(i+1)) - (*i) > blen){
			blen = (*(i+1)) - (*i);
			birr = i;
		}
	}

	__int64 b = (*birr);
	__int64 e = (* (birr+1));

	int64_t SMALLEST_BLOCK_SIZE = 65535;
	if(SETTING(MIN_BLOCK_SIZE) == SettingsManager::blockSizes[SettingsManager::SIZE_64]) SMALLEST_BLOCK_SIZE = 65535;
	if(SETTING(MIN_BLOCK_SIZE) == SettingsManager::blockSizes[SettingsManager::SIZE_128]) SMALLEST_BLOCK_SIZE = 131071;
	if(SETTING(MIN_BLOCK_SIZE) == SettingsManager::blockSizes[SettingsManager::SIZE_256]) SMALLEST_BLOCK_SIZE = 262143;
	if(SETTING(MIN_BLOCK_SIZE) == SettingsManager::blockSizes[SettingsManager::SIZE_512]) SMALLEST_BLOCK_SIZE = 524287;
	if(SETTING(MIN_BLOCK_SIZE) == SettingsManager::blockSizes[SettingsManager::SIZE_1024]) SMALLEST_BLOCK_SIZE = 1048575;
	if(SETTING(MIN_BLOCK_SIZE) == SettingsManager::blockSizes[SettingsManager::SIZE_AUTO]) SMALLEST_BLOCK_SIZE = (iFileSize / maxSegments) / 2;
	if(maxSegments == 1) SMALLEST_BLOCK_SIZE = iFileSize;

	if((e - b) < SMALLEST_BLOCK_SIZE){
			ReleaseMutex(hMutex);
			return -1;
		}

	__int64 n = b + (e - b) / 2;

	if(maxSegments == 1) { n = b; }

	(* (birr+1)) = n;

	vecRunBlocks.push_back(n);
	vecRunBlocks.push_back(e);

	sort(vecRunBlocks.begin(), vecRunBlocks.end());

	ReleaseMutex(hMutex);
	return n;
}

void FileDataInfo::PutUndlStart(__int64 start)
{

	WaitForSingleObject(hMutex, INFINITE);

	for(vector<__int64>::iterator i = vecRunBlocks.begin(); i < vecRunBlocks.end(); i++, i++)
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
			ReleaseMutex(hMutex);
			return;
		}
	}

	ReleaseMutex(hMutex);

	// debug output: no block release
}

string FileDataInfo::getFreeBlocksString()
{
	WaitForSingleObject(hMutex, INFINITE);

		ostringstream os;
		copy(vecFreeBlocks.begin(), vecFreeBlocks.end(), ostream_iterator<__int64>(os, " "));			
		copy(vecRunBlocks.begin(), vecRunBlocks.end(), ostream_iterator<__int64>(os, " "));

		ReleaseMutex(hMutex);
		return os.str();

}
