/*
 * Copyright (C) 2008 Big Muscle, http://strongdc.sf.net
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

#include "../client/CID.h"
#include "../client/MerkleTree.h"

namespace kademlia
{

// all members must be static!
class KadUtils
{
public:
	/** Returns distance between two nodes */
	static CID getDistance(const CID& cid1, const CID& cid2);
	
	/** Returns distance between node and file */
	static TTHValue getDistance(const CID& cid, const TTHValue& tth)
	{
		return TTHValue(const_cast<uint8_t*>(getDistance(cid, CID(tth.data)).data()));
	}
	
	/** Get the bit in CID/TTH */
	static bool KadUtils::getBit(const uint8_t* src, uint8_t bit);
	
	/** Convert CID/TTH to binary string */
	static uint32_t KadUtils::get32BitChunk(const uint8_t* src);
	
private:
	KadUtils(void) { }
	~KadUtils(void) { }
};

} // namespace kademlia