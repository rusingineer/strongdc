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

#include "../client/CID.h"
#include "../client/CriticalSection.h"
#include "../client/FastAlloc.h"
#include "../client/SearchManagerListener.h"
#include "../client/Speaker.h"
#include "../client/User.h"

namespace kademlia
{

class Search :
	public FastAlloc<Search>
{
public:
	enum SearchType { TYPE_FILE = 1, TYPE_NODE = 3, TYPE_STOREFILE = 4 };	// standard types should match ADC protocol

		/** Search type */
	SearchType type;
	
	/** Time when this search has been started */
	uint64_t startTime;
	
	/** File size, -1 = partial file */
	int64_t fileSize;
	
	/** Search identificator */
	string token;
	
	/** Search term (TTH/CID) */
	string term;
	
	/** Nodes where send search request soon to */
	NodeMap possibleNodes;
	
	/** Nodes who responded */
	NodeMap respondedNodes;
	
	/** Nodes where search request was send to to prevent multiple sending */
	typedef std::tr1::unordered_map<CID, OnlineUserPtr> TriedMap;
	TriedMap triedNodes;
	
	/** Complete processing of response to this search */
	void processResponse(const CID& cid, NodeList& results);
	
	/** Process this search */
	void process();
	
	/** Sends this search request to the network */
	void sendRequest(const string& ip, uint16_t port);
};

/** Manages all active Kademlia searches */
class SearchManager :
	public Speaker<SearchManagerListener>, public Singleton<SearchManager>
{
public:
	
	SearchManager(void);
	~SearchManager(void);
		
	/** Find node in the network */
	void findNode(const CID& cid);
	
	/** Searches for a file in the network */
	void findFile(const string& tth);
	
	void publishFile(const string& tth, int64_t size);
		
	/** Process incoming search request */
	void processSearchRequest(const AdcCommand& cmd);
	
	/** Process incoming search response */
	void processSearchResponse(const AdcCommand& cmd);	
	
	/** Starts processing next search request in queue */
	void processSearches();	
		
private:
	/** classes that can access to my private members */
	friend class Singleton<SearchManager>;
	
	/** Last searches */
	typedef std::tr1::unordered_map<string, Search*> SearchMap;
	SearchMap searches;
	
	/** Synchronizes access to searches */
	CriticalSection cs;
	
	/** Searches node/file in the network */
	void search(Search& s);
};

} // namespace kademlia