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
 
#include "StdAfx.h"
#include <DCPlusPlus.h>

#include "KadUtils.h"

#include <CID.h>

namespace kademlia
{

CID KadUtils::getDistance(const CID& cid1, const CID& cid2)
{
	uint8_t distance[CID::SIZE];
	
	for(int i = 0; i < sizeof(CID); i++)
	{
		distance[i] = cid1.data()[i] ^ cid2.data()[i];
	}
	
	return CID(distance);
}

bool KadUtils::getBit(const uint8_t* src, uint8_t bit)
{
	if (bit > 191)
		return 0;

    int bytes = bit / 8; 
	int shift = 7 - (bit % 8);
	return ((src[bytes] >> shift) & 1) == 1;
}

uint32_t KadUtils::get32BitChunk(const uint8_t* src)
{
	dcdebug("Variant a) %d\n", (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | (src[3]));
	dcdebug("Variant b) %d\n", (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | (src[0]));
	
	return (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | (src[3]);
}

} // namespace kademlia