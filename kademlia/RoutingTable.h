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
	
	/** Returns pointer to node */
	OnlineUserPtr findNode(const CID& cid);
	
	/** Returns up  "maximum" number of nodes closest to "cid" */
	void getClosestTo(const CID& cid, size_t maximum, NodeMap& results);
	
	/** Loads existing nodes from disk */
	void loadNodes(SimpleXML& xml);
	
	/** Save all online nodes to disk */
	void saveNodes(SimpleXML& xml);
	
	/** Adds new online node to routing table */
	OnlineUserPtr add(const CID& cid, const string& ip, const string& tcpPort, const string& udpPort);
	
private:

/************* temporaty solution ***************/
	/** Map CID to OnlineUser */
	typedef std::tr1::unordered_map<CID*, OnlineUserPtr> CIDMap;
	typedef CIDMap::const_iterator CIDIter;
	
	/** Stores all online nodes */
	CIDMap nodes;
	
	/** Synchronizes access to nodes */
	CriticalSection cs;
};

} // namespace kademlia