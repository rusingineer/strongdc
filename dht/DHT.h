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

#include "BootstrapManager.h"
#include "Constants.h"
#include "KBucket.h"
#include "UDPSocket.h"

#include "../client/AdcCommand.h"
#include "../client/CID.h"
#include "../client/Singleton.h"

namespace dht
{

	class DHT :	
		public Singleton<DHT>
	{
	public:
		DHT(void);
		~DHT(void);
		
		/** Socket functions */
		void listen() { socket.listen(); BootstrapManager::getInstance()->start(); }
		void disconnect() { socket.disconnect(); }
		uint16_t getPort() const { return socket.getPort(); }
		
		/** Process incoming command */
		void dispatch(const string& aLine, const string& ip, uint16_t port);
		
		/** Sends command to ip and port */
		void send(const AdcCommand& cmd, const string& ip, uint16_t port);
		
		/** Insert (or update) user in routing table */
		Node::Ptr addUser(const CID& cid, const string& ip, uint16_t port);
		
		/** Returns counts of nodes available in k-buckets */
		unsigned int getNodesCount() const { return nodesCount; }
		
		/** Removes dead nodes */
		void checkExpiration(uint64_t aTick);
		
	private:
		/** Classes that can access to my private members */
		friend class Singleton<DHT>;
		friend class SearchManager;
		
		void handle(AdcCommand::SCH, const Node::Ptr& user, AdcCommand& c) throw();	// incoming search request
		void handle(AdcCommand::RES, const Node::Ptr& user, AdcCommand& c) throw();	// incoming search result
		void handle(AdcCommand::PUB, const Node::Ptr& user, AdcCommand& c) throw();	// incoming publish request
		void handle(AdcCommand::STA, const Node::Ptr& user, AdcCommand& c) throw();	// status message
			
		/** Unsupported command */
		template<typename T> void handle(T, const Node::Ptr&user, AdcCommand&) { }
		
		/** UDP socket */
		UDPSocket	socket;
		
		/** Routing table */
		KBucket*	bucket[ID_BITS];
		
		/** Lock to routing table */
		CriticalSection	cs;
		
		/** Counts of nodes available in k-buckets */
		unsigned int	nodesCount;
				
		/** Find bucket for storing node */
		uint8_t findBucket(const CID& cid) const;

		/** Finds "max" closest nodes and stores them to the list */
		void getClosestNodes(const CID& cid, std::map<CID, Node::Ptr>& closest, unsigned int max);
	};

}