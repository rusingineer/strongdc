#ifndef _FileDataInfo_H
#define _FileDataInfo_H

/*
 * object create timing: QueueItem is added
 * object delete timing: QueueItem is removed
 */

class FileDataInfo
{
public:
	vector<__int64> vecFreeBlocks;
	vector<__int64> vecRunBlocks;

	string sFileName;
	__int64 iFileSize;

	HANDLE hMutex;

    __int64 iDownloadedSize;

    __int64 GetDownloadedSize()
    {
        return iDownloadedSize;
    }

	FileDataInfo(const string& name, __int64 size, const vector<__int64>* blocks = NULL);

	~FileDataInfo()
	{
		WaitForSingleObject(hMutexMapList, INFINITE);

		WaitForSingleObject(hMutex, INFINITE);
        CloseHandle(hMutex);

		for(vector<FileDataInfo*>::iterator i = vecAllFileDataInfo.begin(); i != vecAllFileDataInfo.end(); i++){
			if((*i) == this ){
				vecAllFileDataInfo.erase(i);
				break;
			}
		}
		ReleaseMutex(hMutexMapList);		
	}

	static vector<FileDataInfo*> vecAllFileDataInfo;
	static HANDLE hMutexMapList;

	// friend class $$$ QueueManager

public:

	static FileDataInfo* GetFileDataInfo(string name);

	__int64 GetUndlStart();

	void PutUndlStart(__int64);

	string getFreeBlocksString();

	enum{
		NORMAL_EXIT,
		BLOCK_OVER,
		FILE_OVER,
        WRONG_POS
	};

	int ValidBlock(__int64, const void*, __int64);

	__int64 GetBlockEnd(__int64);

};

#endif