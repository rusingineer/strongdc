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

#include "Constants.h"

#include "../client/Socket.h"
#include "../client/Thread.h"
#include "../client/Singleton.h"

#include "../client/AdcCommand.h"
#include "../client/ClientManager.h"
#include "../client/FastAlloc.h"
#include "../client/Forward.h"
#include "../client/HttpConnection.h"
#include "../client/MerkleTree.h"
#include "../client/SearchQueue.h"
#include "../client/TimerManager.h"
#include "../client/User.h"

namespace kademlia
{
	// TODO: move this to somewhere
	typedef std::vector<OnlineUserPtr> NodeList;
	typedef std::map<CID, OnlineUserPtr> NodeMap;
	typedef std::deque<Identity> SourceList;
}

#include "RoutingTable.h"

namespace kademlia
{

class KademliaManager :
	public Singleton<KademliaManager>, private Thread, 
	private CommandHandler<KademliaManager>, private TimerManagerListener,
	private HttpConnectionListener
{
public:

	KademliaManager(void);
	~KademliaManager(void);

	/** Disconnects UDP socket */
	void disconnect() throw();
	
	/** Starts listening to UDP socket */
	void listen() throw(SocketException);
		
	/** Searches for a file in the network */
	uint64_t search(const TTHValue& tth);
	
	/** Returns port used to listening to UDP socket */
	uint16_t getPort() const { return port;	}
	
	/** Returns reference to routing table */
	RoutingTable& getRoutingTable() { return *routingTable; }
	
	/** Sends command to ip and port */
	void send(const AdcCommand& cmd, const string& ip, uint16_t port);
	
private:
	/** classes that can access to my private members */
	friend class Singleton<KademliaManager>;
	friend class CommandHandler<KademliaManager>;
	
	struct Packet : FastAlloc<Packet>
	{
		/** Public constructor */
		Packet(const string& ip_, uint16_t port_, const uint8_t* data_, size_t length_) : 
			ip(ip_), port(port_), data(data_), length(length_)
		{
		}
		
		/** IP where send this packet to */
		string ip;
		
		/** To which port this packet should be sent */
		uint16_t port;
		
		/** Data to sent */
		const uint8_t* data;
		
		/** Data's length */
		size_t length;
	};

	/** Locks access to sending queue */
	// TODO: 
	// Use Fast critical section, because we don't need locking so often.
	// Almost all shared access is done within one thread.
	mutable CriticalSection cs;
	
	/** Manages all known nodes */
	RoutingTable* routingTable;
	
	/** Indicates to stop socket thread */
	bool stop;
	
	/** Port for communicating in this network */
	uint16_t port;
	
	/** UDP socket */
	std::auto_ptr<Socket> socket;
	
	/** Queue for sending packets through UDP socket */
	std::deque<Packet*> sendQueue;
	
	/** Queue for sending search requests */
	SearchQueue searchQueue;
	
	/** HTTP connection for bootstrapping */ 
	HttpConnection httpConnection;
	
	/** Downloaded node list */
	string nodesXML;
	
	/** The IP of last received packet */
	string lastIP;
	
	/** next timer operations */
	uint64_t nextSelfLookup;
	uint64_t nextSearchJumpStart;
	
#ifdef _DEBUG
	// debug constants to optimize bandwidth
	size_t sentBytes;
	size_t receivedBytes;
#endif

	/** Thread for receiving UDP packets */
	int run();
	
	/** Loads network information from XML file */
	void loadData();
	
	/** Saves network information to XML file */
	void saveData();
	
	void handle(AdcCommand::REQ, AdcCommand& c) throw();	// incoming kademlia request
	void handle(AdcCommand::SCH, AdcCommand& c) throw();	// incoming search request
	void handle(AdcCommand::PUB, AdcCommand& c) throw();	// incoming publish request	
	void handle(AdcCommand::RES, AdcCommand& c) throw();	// incoming search/kademlia response
	
	/** Unsupported command */
	template<typename T> void handle(T, AdcCommand&) { }
	
	// HttpConnectionListener
	void on(HttpConnectionListener::Data, HttpConnection* conn, const uint8_t* buf, size_t len) throw();
	void on(HttpConnectionListener::Complete, HttpConnection* conn, string const& aLine) throw();
	void on(HttpConnectionListener::Failed, HttpConnection* conn, const string& aLine) throw();
	
	// TimerManagerListener
	void on(TimerManagerListener::Second, uint64_t aTick) throw();
	void on(TimerManagerListener::Minute, uint64_t aTick) throw();
};

} // namespace kademlia
