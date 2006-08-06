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

/*
 * Terms:
 *     Chunk  : file bytes range
 *     Block  : TTH level-10 blocks range
 *     Running: a chunk status of asking slot or downloading, occupied
 *     Waiting: a chunk not running
 *     Pending: running chunk without any progress
 *     Split  : when no waiting chunk available, a running chunk is split 
 *              into two chunks
 *     PFS    : partial file sharing
 */

#pragma once

#include "Pointer.h"
#include "CriticalSection.h"
#include "MerkleTree.h"

// minimum file size to be PFS : 20M
#define PARTIAL_SHARE_MIN_SIZE 20971520

// it's used when a source's download speed is unknown
#define DEFAULT_SPEED 5120

// PFS purpose
typedef vector<u_int16_t> PartsInfo;

// ...
typedef map<u_int16_t, u_int16_t> BlockMap;
typedef BlockMap::const_iterator BlockIter;

class Download;


/**
 * Hold basic information of a chunk
 * Features: 
 *     . download speed awareness when chunk split
 *     . overlapped
 */
class Chunk
{
private:
	friend class FileChunksInfo;

	typedef map<int64_t, Chunk*> Map;
	typedef Map::const_iterator  Iter;

	Chunk(int64_t startOffset, int64_t endOffset) 
		: download(NULL), pos(startOffset), end(endOffset), overlappedCount(0)
	{}

	Download* download;
	int64_t pos;
	int64_t end;

	// allow overlapped download the same pending chunk 
	// when all running chunks could not be split
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
	static Ptr Get(const TTHValue* tth);

	/**
     * Free the file chunks information, this is called when target is removed from download queue
     * See also ~FileChunksInfo()
     */
    static void Free(const TTHValue* tth);

	/**
     * Create file chunks infromation with name, size and chunks detail
     */
	FileChunksInfo(TTHValue* tth, int64_t size, const vector<int64_t>* blocks = NULL);

	/**
     * Smart pointer based destructor
     */
	virtual ~FileChunksInfo();

	/**
     * Get start offset of a free chunk, the end offset of chunk is unpredictable
     */
	int64_t getChunk(bool& useChunks, int64_t _speed = DEFAULT_SPEED);

	/**
	 * Compute download start position by source's parts info
	 *
	 * Return the 1st duplicate position both in free and parts info
	 * Or split the biggest duplicate chunk both in running and parts info
	 */
	int64_t getChunk(const PartsInfo& partialInfo, int64_t _speed = DEFAULT_SPEED);

	/**
	 * Detects whether new chunk can be started
	 */
	int findChunk(const PartsInfo& partialInfo, int64_t _speed = DEFAULT_SPEED);

	/**
     * Abandon all unverified bytes received by a chunk
     */
	void abandonChunk(int64_t);
	
	/**
     * Release the chunk with start offset
     */
	void putChunk(int64_t);

	/** 
	 * Specify a Download to a running chunk
	 */
	 void setDownload(int64_t, Download*, bool);

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

	void getAllChunks(vector<int64_t>& v, int type);

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
	bool doLastVerify(const TigerTree& aTree, const string& tempTargetName);

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
	bool isVerified(int64_t startPos, int64_t& len);

	/**
	 * Verify a block	
 	 */	
 	bool verifyBlock(int64_t anyPos, const TigerTree& aTree, const string& tempTargetName);
 	 	
 	/**
	 * Debug
	 */

	inline void selfCheck();
	
	static vector<Ptr> vecAllFileChunksInfo;
	static CriticalSection hMutexMapList;

	// chunk start position must be unique in both waiting and running
	Chunk::Map waiting;
	Chunk::Map running;

    BlockMap verifiedBlocks;

    size_t	tthBlockSize;					// tiger tree hash block size
	TTHValue* TTH;

	int64_t iFileSize;
    int64_t iDownloadedSize;
    int64_t iVerifiedSize;
	int64_t minChunkSize;					// it'll be doubled when last verifying fail

	CriticalSection cs;

private:
	bool verify(const unsigned char* data, int64_t start, int64_t end, const TigerTree& aTree);


	// for debug purpose
	void dumpVerifiedBlocks();
};

string GetPartsString(const PartsInfo& partsInfo);
