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

#include "../client/ClientManager.h"
#include "../client/CriticalSection.h"
#include "../client/Forward.h"
#include "../client/SimpleXML.h"
#include "../client/User.h"

#include "KademliaManager.h"

namespace kademlia
{

class RoutingTable
{
public:
	RoutingTable(void);
	~RoutingTable(void);
	
	///** Returns pointer to node */
	//OnlineUserPtr findNode(const CID& cid);
	
	/** Returns up  "maximum" number of nodes closest to "cid" */
	size_t getClosestTo(const CID& cid, size_t maximum, NodeMap& results) const;
	
	/** Loads existing nodes from disk */
	void loadNodes(SimpleXML& xml);
	
	/** Save all online nodes to disk */
	void saveNodes(SimpleXML& xml);
	
	/** Adds new online node to routing table */
	OnlineUserPtr add(const CID& cid);
	
	/** Returns all online nodes */
	void getAllNodes(NodeList& list) const;
	
private:
	/** Map CID to OnlineUser */
	typedef std::tr1::unordered_map<CID*, OnlineUserPtr> CIDMap;
	typedef CIDMap::const_iterator CIDIter;

	/** Nodes in this zone (if it is a leaf) */
	CIDMap* nodes;
	
	/** Synchronizes access to nodes */
	mutable CriticalSection cs; // TODO: could this be static?
	
	/** Right and left branches */
	RoutingTable* subZones[2];
	
	/** Zone index as a distance from center zone at this level */
	CID zoneIndex;
	
	/** Zone level - max is 192 (sizeof(CID)) */
	uint8_t level;
};

} // namespace kademlia