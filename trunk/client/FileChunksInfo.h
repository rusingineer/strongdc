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

#pragma once

#include "Pointer.h"
#include "CriticalSection.h"
#include "File.h"
#include "MerkleTree.h"

#define PARTIAL_SHARE_MIN_SIZE 20971520
#define DEFAULT_SPEED 5120

typedef vector<u_int16_t> PartsInfo;
typedef map<u_int16_t, u_int16_t> BlockMap;

// For SetFileValidData() - antifrag purpose
extern void ensurePrivilege();

class Download;

class ChunkDoneException : public Exception {
public:
	ChunkDoneException(const string& aError, int64_t _pos) throw() : Exception(aError), pos(_pos) { };
	virtual ~ChunkDoneException() { }; 
	int64_t pos;
};

class FileDoneException : public Exception {
public:
	FileDoneException(const string& aError, int64_t _pos) throw() : Exception(aError), pos(_pos) { };
	virtual ~FileDoneException() { }; 
	int64_t pos;
};

class Chunk
{
private:
	friend class FileChunksInfo;

	typedef map<int64_t, Chunk*> Map;
	typedef Map::iterator  Iter;

	Chunk(int64_t _start, int64_t _end) 
		: download(NULL), pos(_start), end(_end), overlappedCount(0), user(NULL)
	{}

	Download* download;
	User::Ptr user;
	int64_t pos;
	int64_t end;

	// allow overlapped download the same pending chunk 
	// when all running chunks are unbreakable
	size_t overlappedCount;
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
		CHUNK_OVER,
		FILE_OVER,
        CHUNK_LOST
	} CHUNK_RESULT;

	/**
     * Get file chunks information by file name
     */
	static Ptr Get(const string& name);

	/**
     * Free the file chunks information, this is called when target is removed from download queue
     * See also ~FileChunksInfo()
     */
    static void Free(const string& name);

	/**
     * Create file chunks infromation with name, size and chunks detail
     */
	FileChunksInfo(const string& name, int64_t size, const vector<int64_t>* blocks = NULL);

	/**
     * Smart pointer based destructor
     */
	virtual ~FileChunksInfo();

	/**
     * Get start offset of a free chunk, the end offset of chunk is unpredictable
     */
	int64_t getChunk(int64_t _speed = DEFAULT_SPEED);

	/**
	 * Compute download start position by source's parts info
	 *
	 * Return the 1st duplicate position both in free and parts info
	 * Or split the biggest duplicate chunk both in running and parts info
	 */
	int64_t getChunk(const PartsInfo& partialInfo, int64_t _speed = DEFAULT_SPEED);

	/**
	 * Check whether there is some free block to avoid unnecessary connection attempts when there's none free
	 */
	bool hasFreeBlock();

	/**
     * Release the chunk with start offset
     */
	void putChunk(int64_t);

	void setDownload(int64_t, Download*, User::Ptr, bool);
	/**
     * Convert all unfinished chunks range data to a string, eg. "0 5000 10000 30000 "
     */
	string getFreeChunksString();

	/**
     * Convert all verified chunks range data to a string, eg. "0 65536 "
     */
	string getVerifiedBlocksString();

	/**
     * Mark a chunk range as download finished
     */
	int addChunkPos(int64_t, int64_t, size_t&);

	/**
     * Because magnet link maybe not contain size information...
     */
	void setFileSize(const int64_t& size);

	void getAllChunks(vector<int64_t>& v, bool verified);

    int64_t getDownloadedSize()
    {
        return iDownloadedSize;
    }

    int64_t getVerifiedSize()
    {
        return iVerifiedSize;
    }

	/**
     * Perform last verifying against TigerTree
     */
	bool doLastVerify(const TigerTree& aTree, string aTarget);

	void markVerifiedBlock(u_int16_t start, u_int16_t end);

	/**
	 * Is specified parts needed by this download?
	 */
	bool isSource(const PartsInfo& partsInfo);

	/**
	 * Get shared parts info, max 255 parts range pairs
	 */
	void getPartialInfo(PartsInfo& partialInfo);

	/**
     * Is specified chunk start verified? If true, the chunk len is set.
	 */
	bool isVerified(int64_t startPos, int64_t& len){
		if(len <= 0) return false;

		Lock l(cs);

		BlockMap::iterator i  = verifiedBlocks.begin();
		for(; i != verifiedBlocks.end(); i++)
		{
			int64_t first  = (int64_t)i->first  * (int64_t)tthBlockSize;
			int64_t second = (int64_t)i->second * (int64_t)tthBlockSize;
			
			second = min(second, iFileSize);

			if(first <= startPos && startPos < second){
				len = min(len, second - startPos);
				return true;
			}
		}

		return false;
	}

	static vector<Ptr> vecAllFileChunksInfo;
	static CriticalSection hMutexMapList;

	// chunk start position must be unique in both waiting and running
	Chunk::Map waiting;
	Chunk::Map running;

    BlockMap verifiedBlocks;

    size_t	tthBlockSize;					// tiger tree hash block size
	string  tempTargetName;					// temp target file name

	int64_t iFileSize;
    int64_t iDownloadedSize;
    int64_t iVerifiedSize;
	int64_t minChunkSize;					// it'll be doubled when last verifying fail

	bool noFreeBlock;

	CriticalSection cs;

private:
};

/******************************************************************************/

struct SharedFileHandle : CriticalSection
{
    HANDLE 				handle;
    int					ref_cnt;

	SharedFileHandle(const string& name, bool shareDelete = false);

	~SharedFileHandle()
    {
        CloseHandle(handle);
    }
};

# pragma warning(disable: 4511) // copy constructor could not be generated
# pragma warning(disable: 4512) // assignment operator could not be generated

class SharedFileStream : public IOStream
{

public:

    typedef map<string, SharedFileHandle*> SharedFileHandleMap;

    SharedFileStream(const string& name, int64_t _pos, int64_t size = 0, bool shareDelete = false);

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

			_ASSERT(0);
        }
    }

	virtual size_t write(const void* buf, size_t len) throw(Exception)
    {
		Lock l(*shared_handle_ptr);

		DWORD x = (DWORD)(pos >> 32);
		DWORD ret = ::SetFilePointer(shared_handle_ptr->handle, (DWORD)(pos & 0xffffffff), (PLONG)&x, FILE_BEGIN);

		if(ret == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
			throw FileException(Util::translateError(GetLastError()));

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

	ChunkOutputStream(OutputStream* _os, const string& name, int64_t _chunk) 
		: os(_os), chunk(_chunk), pos(_chunk)
    {
		fileChunks = FileChunksInfo::Get(name);
		if(fileChunks == (FileChunksInfo*)NULL)
        {
			throw FileException("No chunks data");
		}
    }

	virtual ~ChunkOutputStream()
    {
		if(managed) delete os;
    }

	virtual size_t write(const void* buf, size_t len) throw(Exception)
	{
		if(chunk == -1) return 0;
		if(len == 0) return 0;

    	int iRet = fileChunks->addChunkPos(chunk, pos, len);

		if(iRet == FileChunksInfo::CHUNK_LOST){
			chunk = -1;
			throw ChunkDoneException(Util::emptyString, pos);
		}

		pos += len;

		if(len > 0){
			size_t size = os->write(buf, len);

			if(size != len)
				throw FileException("Internal Error");
		}

		if (iRet == FileChunksInfo::CHUNK_OVER){
			os->flush();
			chunk = -1;
			throw ChunkDoneException(Util::emptyString, pos);

		}else if(iRet == FileChunksInfo::FILE_OVER){			
  			os->flush();
			chunk = -1;
			throw FileDoneException(Util::emptyString, pos);
		}

        return len;
    }

	virtual size_t flush() throw(Exception) 
	{
		return os->flush();
	}

private:

	FileChunksInfo::Ptr fileChunks;
	OutputStream* os;
	int64_t chunk;
	int64_t pos;
};

string GetPartsString(const PartsInfo& partsInfo);