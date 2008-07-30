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

#include "../client/Singleton.h"

#include "KademliaManager.h"

namespace kademlia
{

class IndexManager :
	public Singleton<IndexManager>
{
public:
	IndexManager(void);
	~IndexManager(void);
	
	/** Add new source to tth list */
	void addIndex(const TTHValue& tth, const Identity& source);
	
	/** Finds TTH in known indexes and returns it */
	bool findResult(const TTHValue& tth, SourceList& sources) const;
	
	/** Loads existing indexes from disk */
	void loadIndexes(SimpleXML& xml);
	
	/** Save all indexes to disk */
	void saveIndexes(SimpleXML& xml);	
	
private:

	/** Contains known hashes in the network and their sources */
	typedef std::tr1::unordered_map<TTHValue, SourceList> TTHMap;
	TTHMap tthList;
	
	/** Synchronizes access to tthList */
	mutable CriticalSection cs;
};

} // namespace kademlia