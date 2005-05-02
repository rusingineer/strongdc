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

#pragma once

#include "Pointer.h"
#include "CriticalSection.h"
#include "File.h"
#include "MerkleTree.h"

typedef map<u_int16_t, u_int16_t> BlockMap;

// For SetFileValidData() - antifrag purpose
extern void ensurePrivilege();

class TargetInfo : public PointerBase
{
public:
	typedef Pointer<TargetInfo> Ptr;
	
	TargetInfo(string aTarget, string aTempTarget) : target(aTarget), temptarget(aTempTarget) { }
	virtual ~TargetInfo() { }

	GETSET(string, target, Target);
	GETSET(string, temptarget, TempTarget);
};

class BlockDLException : public Exception {
public:
	BlockDLException(const string& aError, int64_t _pos) throw() : Exception(aError), pos(_pos) { };
	virtual ~BlockDLException() { }; 
	int64_t pos;
};

class FileDLException : public Exception {
public:
	FileDLException(const string& aError, int64_t _pos) throw() : Exception(aError), pos(_pos) { };
	virtual ~FileDLException() { }; 
	int64_t pos;
};

/**
 * Holds multi-sources downloading file information
 * including downloaded chunks and verified parts/blocks
 */

class FileChunksInfo : public PointerBase
{
public:
	typedef Pointer<FileChunksInfo> Ptr;

	enum{
		NORMAL_EXIT,
		BLOCK_OVER,
		FILE_OVER,
        WRONG_POS
	} VALID_RESULT;

	/**
     * Get file chunks information by file name
     */
	static Ptr Get(const TargetInfo::Ptr& ti);

	/**
     * Free the file chunks information, this is called when target is removed from download queue
     * See also ~FileChunksInfo()
     */
    static void Free(const TargetInfo::Ptr& ti);

	/**
     * Create file chunks infromation with name, size and chunks detail
     */
	FileChunksInfo(const TargetInfo::Ptr& ti, int64_t size, const vector<int64_t>* blocks = NULL);

	/**
     * Smart pointer based destructor
     */
	virtual ~FileChunksInfo()
    {

	}

	/**
     * Get start offset of a free chunk, the end offset of chunk is unpredictable
     */
	int64_t GetUndlStart();

	/**
	 * Check whether there is some free block to avoid unnecessary connection attempts when there's none free
	 */
	bool hasFreeBlock();

	/**
     * Release the chunk with start offset
     */
	void PutUndlStart(int64_t);

	/**
     * Convert all unfinished chunks range data to a string, eg. "0 5000 10000 30000 "
     */
	string getFreeBlocksString();

	/**
     * Convert all verified chunks range data to a string, eg. "0 65536 "
     */
	string getVerifiedBlocksString();

	/**
     * Mark a chunk range as download finished
     */
	int ValidBlock(int64_t, size_t&);

	int64_t GetBlockEnd(int64_t);

	/**
     * Because magnet link maybe not contain size information...
     */
	void SetFileSize(const int64_t& size);

    int64_t GetDownloadedSize()
    {
        return iDownloadedSize;
    }

    int64_t GetVerifiedSize()
    {
        return iVerifiedSize;
    }

	/**
     * Perform last verifying against TigerTree
     */
	bool DoLastVerify(const TigerTree& aTree, string aTarget);

	void markVerifiedBlock(u_int16_t start, u_int16_t end);



//private:

	static vector<Ptr> vecAllFileChunksInfo;
	static CriticalSection hMutexMapList;

	vector<int64_t> vecFreeBlocks;
	vector<int64_t> vecRunBlocks;
    BlockMap verifiedBlocks;

    size_t	tthBlockSize;					// tiger tree hash block size
//	string  tempTargetName;					// temp target file name
	TargetInfo::Ptr tInfo;

	int64_t iFileSize;
    int64_t iDownloadedSize;
    int64_t iVerifiedSize;
	int64_t minChunkSize;					// it'll be doubled when last verifying fail

	CriticalSection cs;

private:
	typedef pair<int64_t, int64_t> Chunk;

	void addFreeChunk(const Chunk& chunk);	// Merge free chunks when a running chunk becomes free

};

/******************************************************************************/

struct SharedFileHandle : CriticalSection
{
    HANDLE 				handle;
    int					ref_cnt;

	SharedFileHandle(const string& name);

	~SharedFileHandle()
    {
        CloseHandle(handle);
    }
};

class SharedFileStream : public IOStream
{

public:

    typedef map<string, SharedFileHandle*> SharedFileHandleMap;

    SharedFileStream(const string& name, int64_t _pos, int64_t size = 0);

    virtual ~SharedFileStream()
    {
		Lock l(critical_section);

		shared_handle_ptr->ref_cnt--;
		
		if(!shared_handle_ptr->ref_cnt)
		{
            for(SharedFileHandleMap::iterator i = file_handle_pool.begin();
            							i != file_handle_pool.end();
                                        i++)
			{
				if(i->second == shared_handle_ptr)
				{
                	file_handle_pool.erase(i);
					delete shared_handle_ptr;
                    return;
                }
            }

			dcassert(0);
        }
    }

	virtual size_t write(const void* buf, size_t len) throw(Exception)
    {
		Lock l(*shared_handle_ptr);

		DWORD x = (DWORD)(pos >> 32);
		::SetFilePointer(shared_handle_ptr->handle, (DWORD)(pos & 0xffffffff), (PLONG)&x, FILE_BEGIN);

		if(!::WriteFile(shared_handle_ptr->handle, buf, len, &x, NULL)) {
			throw FileException(Util::translateError(GetLastError()));
		}

        pos += len;
		return len;
    }

	virtual size_t read(void* buf, size_t& len) throw(Exception) {
		Lock l(*shared_handle_ptr);

		DWORD x = (DWORD)(pos >> 32);
		::SetFilePointer(shared_handle_ptr->handle, (DWORD)(pos & 0xffffffff), (PLONG)&x, FILE_BEGIN);

		if(!::ReadFile(shared_handle_ptr->handle, buf, len, &x, NULL)) {
			throw(FileException(Util::translateError(GetLastError())));
		}
		len = x;

        pos += len;

		return x;
	}

	virtual size_t flush() throw(Exception) 
	{
		Lock l(*shared_handle_ptr);

		if(!FlushFileBuffers(shared_handle_ptr->handle))
			throw FileException(Util::translateError(GetLastError()));
		return 0;
	}

	virtual void setPos(int64_t _pos) 
	{ pos = _pos; }

	virtual int64_t getSize() throw() {
		DWORD x;
		DWORD l = ::GetFileSize(shared_handle_ptr->handle, &x);

		if( (l == INVALID_FILE_SIZE) && (GetLastError() != NO_ERROR))
			return -1;

		return (int64_t)l | ((int64_t)x)<<32;
	}

    static CriticalSection critical_section;
	static SharedFileHandleMap file_handle_pool;

private:
	SharedFileHandle* shared_handle_ptr;
	int64_t pos;


};

/******************************************************************************/

// Note: should be used before BufferedOutputStream
template<bool managed>
class ChunkOutputStream : public OutputStream
{

public:

	ChunkOutputStream(OutputStream* _os, const TargetInfo::Ptr& name, int64_t _pos) 
		: os(_os), pos(_pos)
    {
		file_chunks_info_ptr = FileChunksInfo::Get(name);
		if(file_chunks_info_ptr == (FileChunksInfo*)NULL)
        {
			throw FileException("No chunks data");
		}
    }

	virtual ~ChunkOutputStream()
    {
        if(pos >=0)
	        file_chunks_info_ptr->PutUndlStart(pos);

		if(managed) delete os;
    }

	virtual size_t write(const void* buf, size_t len) throw(Exception)
	{
		if(pos == -1) return 0;

		size_t blockRemain = file_chunks_info_ptr->GetBlockEnd(pos) - pos; 
		size_t size = os->write(buf, min(len, blockRemain)); 
		int iRet = file_chunks_info_ptr->ValidBlock(pos, size);

		if (iRet == FileChunksInfo::BLOCK_OVER){
			int64_t oldPos = pos;
            pos = -1;
			os->flush();
			throw BlockDLException(Util::emptyString, oldPos + len);

		}else if(iRet == FileChunksInfo::FILE_OVER){			
			int64_t oldPos = pos;
            pos = -1;
   			os->flush();
			throw FileDLException(Util::emptyString, oldPos + len);

		}else if(iRet == FileChunksInfo::WRONG_POS){
			throw FileException(string("Error:") + Util::toString(pos) + "," + Util::toString(pos + len));

		}

		pos += len;
        return size;
    }

	virtual size_t flush() throw(Exception) 
	{
		return os->flush();
	}

	int64_t getPos(){return pos;}

private:

	FileChunksInfo::Ptr file_chunks_info_ptr;
	OutputStream* os;
	int64_t pos;
};
