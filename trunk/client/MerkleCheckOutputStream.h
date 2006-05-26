/*
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(MERKLE_CHECK_OUTPUT_STREAM_H)
#define MERKLE_CHECK_OUTPUT_STREAM_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Streams.h"
#include "MerkleTree.h"

template<class TreeType, bool managed>
class MerkleCheckOutputStream : public OutputStream {
public:
	MerkleCheckOutputStream(const TreeType& aTree, OutputStream* aStream, int64_t start, const string& tempTarget = Util::emptyString) : s(aStream), real(aTree), cur(aTree.getBlockSize()), verified(0), bufPos(0) {
		if(!tempTarget.empty()) {
			skippingBytes = (size_t)(start % aTree.getBlockSize());
			if(skippingBytes > 0)
				skippingBytes = (size_t)(aTree.getBlockSize() - skippingBytes);

			fileChunks = FileChunksInfo::Get(tempTarget);
			dcassert(!(fileChunks == (FileChunksInfo*)NULL));
			bufPos = 0;
			start = start + skippingBytes;
			multiSourceChecking = true;
		} else {
			multiSourceChecking = false;
		}
		// Only start at block boundaries
		dcassert(start % aTree.getBlockSize() == 0);
		// Sanity check
		dcassert(aTree.getLeaves().size() >= (size_t)(start / aTree.getBlockSize()));
		cur.setFileSize(start);
		cur.getLeaves().insert(cur.getLeaves().begin(), aTree.getLeaves().begin(), aTree.getLeaves().begin() + (size_t)(start / aTree.getBlockSize()));
		
		if(multiSourceChecking)
			verified = cur.getLeaves().size();
	}

	virtual ~MerkleCheckOutputStream() throw() { if(managed) delete s; }

	virtual size_t flush() throw(FileException) {
		if(!multiSourceChecking) {
			if (bufPos != 0)
				cur.update(buf, bufPos);
			bufPos = 0;

			cur.finalize();
			if(cur.getLeaves().size() == real.getLeaves().size()) {
				if (cur.getRoot() != real.getRoot())
					throw FileException(STRING(TTH_INCONSISTENCY));
			} else  {
				checkTrees();
			}
		}
		return s->flush();
	}

	virtual size_t write(const void* b, size_t len) throw(FileException) {
		u_int8_t* xb = (u_int8_t*)b;
		size_t pos = 0;
		bool verifyFlag = false;

		
		if(multiSourceChecking && (skippingBytes > 0))
		{
			if(skippingBytes >= len)
			{
				skippingBytes -= len;
				size_t ret = s->write(b, len);
				if(skippingBytes == 0){
					s->flush();
					dcassert(verified > 0);
					int64_t offset = (int64_t)verified * (int64_t)(real.getBlockSize()) - 1;
					fileChunks->verifyBlock(offset, real);
				}
				return ret;
	        }else{
				pos = skippingBytes;
				skippingBytes = 0;
				verifyFlag = true;
			}
		}
		
		if(bufPos != 0) {
			size_t bytes = min(TreeType::BASE_BLOCK_SIZE - bufPos, len);
			memcpy(buf + bufPos, xb, bytes);
			pos = bytes;
			bufPos += bytes;

			if(bufPos == TreeType::BASE_BLOCK_SIZE) {
				cur.update(buf, TreeType::BASE_BLOCK_SIZE);
				bufPos = 0;
			}
		}

		if(pos < len) {
			dcassert(bufPos == 0);
			size_t left = len - pos;
			size_t part = left - (left %  TreeType::BASE_BLOCK_SIZE);
			if(part > 0) {
				cur.update(xb + pos, part);
				pos += part;
			}
			left = len - pos;
			memcpy(buf, xb + pos, left);
			bufPos = left;
		}

		size_t old = verified;
		checkTrees();
		size_t ret = s->write(b, len);
		
		if(multiSourceChecking) {
			// mark verified block
			if(verified > old) {
				s->flush();
				fileChunks->markVerifiedBlock((u_int16_t)old, (u_int16_t)verified);
			}

			if(verifyFlag){
				s->flush();
				dcassert(old > 0);
				int64_t offset = (int64_t)old * (int64_t)(real.getBlockSize()) - 1;
				fileChunks->verifyBlock(offset, real);
			}
		}
		return ret;
	}

	int64_t verifiedBytes() {
		return min(real.getFileSize(), (int64_t)(cur.getBlockSize() * cur.getLeaves().size()));
	}
private:
	OutputStream* s;
	const TreeType& real;
	TreeType cur;
	size_t verified;
	size_t skippingBytes;					// the bytes that will be skipped
	FileChunksInfo::Ptr fileChunks;
	bool multiSourceChecking;

	u_int8_t buf[TreeType::BASE_BLOCK_SIZE];
	size_t bufPos;

	void checkTrees() throw(FileException) {
		while(cur.getLeaves().size() > verified) {
			if(cur.getLeaves().size() > real.getLeaves().size() ||
				!(cur.getLeaves()[verified] == real.getLeaves()[verified])) 
			{
				if(multiSourceChecking) {
					dcassert(cur.getLeaves().size() <= real.getLeaves().size());
					LogManager::getInstance()->message(STRING(CORRUPTION_DETECTED) + " " + Util::toString(verified), true);
				}
				throw FileException(STRING(TTH_INCONSISTENCY));
			}
			verified++;
		}
	}
};

#endif // !defined(MERKLE_CHECK_OUTPUT_STREAM_H)

/**
 * @file
 * $Id$
 */