#include "stdinc.h"
#include "DCPlusPlus.h"
#include "File.h"
#include "TraceManager.h"

size_t MappedFile::write(const void* aBuf, size_t len) throw(Exception) {

    dcassert(iPos + len <= iFileSize);
    
    do {
		if(lpMapAddr == NULL){
			iMapPos = (iPos / dwSysGran) * dwSysGran;
			dwMapSize = MAX(dwBuffSize, (DWORD)(len + iPos - iMapPos));
			dwMapSize = (DWORD)MIN(dwMapSize , (iFileSize - iMapPos));

			dcassert(dwMapSize - (iPos - iMapPos) >= len);
			
			lpMapAddr = MapViewOfFile(hMapFile, 
									FILE_MAP_WRITE, 
									HIDWORD(iMapPos), 
									LODWORD(iMapPos), 
									dwMapSize
									);
			
			if(lpMapAddr == NULL){
                DWORD dwErr = GetLastError();
                if(dwErr == ERROR_NOT_ENOUGH_MEMORY){
                    TracePrint("ERROR %d: %s %I64x, %I64x, %I64x, %x, %x, %x",
						dwErr, Util::getOsVersion().c_str(), iFileSize, iPos, iMapPos, dwSysGran, dwBuffSize, dwMapSize);
                    throw FileException("Please report bug with error.log!");
                }else
					throw FileException(Util::translateError(dwErr));
			}
		}else if(iPos < iMapPos || dwMapSize - (iPos - iMapPos) < len){
			UnmapViewOfFile(lpMapAddr);
			lpMapAddr = NULL;
		}
	
	} while (lpMapAddr == NULL);

	UINT iRet = SafeCopyMemory((LPBYTE)lpMapAddr + (iPos - iMapPos), aBuf, len);
	
	if(iRet){
		TCHAR lpBuf[32];
		wsprintf(lpBuf, " 0x%x ", iRet);
		throw FileException(lpBuf);
	}

	iPos += len;

	return len;
    
}
