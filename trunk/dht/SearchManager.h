/*
 * Copyright (C) 2009 Big Muscle, http://strongdc.sf.net
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

#include "KBucket.h"

#include "..\client\CID.h"
#include "..\client\CriticalSection.h"
#include "..\client\FastAlloc.h"
#include "..\client\MerkleTree.h"
#include "..\client\SearchManagerListener.h"
#include "..\client\Singleton.h"
#include "..\client\Speaker.h"
#include "..\client\TimerManager.h"
#include "..\client\User.h"

namespace dht
{

	struct Search :
		public FastAlloc<Search>
	{
		
		Search() : token(Util::toString(Util::rand())), startTime(GET_TICK())
		{
		}

		enum SearchType { TYPE_FILE = 1, TYPE_NODE = 3, TYPE_STOREFILE = 4 };	// standard types should match ADC protocol

		/** Search type */
		SearchType type;
		
		/** Time when this search has been started */
		uint64_t startTime;		
		
		/** Search identificator */
		string token;
	
		/** Search term (TTH/CID) */
		string term;
		
		/** Nodes where send search request soon to */
		typedef std::map<CID, Node::Ptr> NodeMap;
		NodeMap possibleNodes;
		
		/** Nodes where search request has already been sent to */
		NodeMap triedNodes;
		
		/** Nodes who responded to this search request */
		NodeMap respondedNodes;
		
		/** File size */
		int64_t filesize;
		
		/** Process this search request */
		bool process();
	};
		
	class SearchManager :
		public Speaker<SearchManagerListener>, public Singleton<SearchManager>
	{
	public:
		SearchManager(void);
		~SearchManager(void);
		
		/** Performs node lookup in the network */
		void findNode(const CID& cid);
		
		/** Performs value lookup in the network */
		void findFile(const string& tth);
		
		/** Performs node lookup to store key/value pair in the network */
		void findStore(const string& tth, int64_t size);		
		
		/** Process incoming search request */
		void processSearchRequest(const Node::Ptr& node, const AdcCommand& cmd);

		/** Process incoming search result */
		void processSearchResult(const Node::Ptr& node, const AdcCommand& cmd);
		
		/** Processes all running searches and removes long-time ones */
		void processSearches();
		
	private:
	
		/** Running search operations */
		typedef std::tr1::unordered_map<string, Search*> SearchMap;
		SearchMap searches;
		
		/** Locks access to "searches" */
		CriticalSection cs;
		
		/** Performs general search operation in the network */
		void search(Search& s);
		
		/** Sends publishing request */
		void publishFile(Search::NodeMap nodes, const string& tth, int64_t size);
		
	};

}