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
 
#ifndef DCPLUSPLUS_DCPP_DECENTRALIZATION_MANAGER_H
#define DCPLUSPLUS_DCPP_DECENTRALIZATION_MANAGER_H

#include "Socket.h"
#include "Thread.h"
#include "Singleton.h"

#include "AdcCommand.h"
#include "SearchQueue.h"
#include "ResourceManager.h"
#include "TimerManager.h"
#include "User.h"

namespace dcpp
{

#define DPORT SETTING(TLS_PORT)

#define ADC_PACKET_HEADER 'U'
#define ADC_PACKET_FOOTER 0x0a
#define ADC_PACKED_PACKET_HEADER 0xc1

#define NODE_FILE "nodes.dat"

class DecentralizationManager : public Singleton<DecentralizationManager>, private Thread, 
	private CommandHandler<DecentralizationManager>, private TimerManagerListener
{
public:
	DecentralizationManager(void);
	~DecentralizationManager(void);

	/** Disconnects UDP socket */
	void disconnect() throw();
	
	/** Starts listening to UDP socket */
	void listen() throw(SocketException);
		
	/** Get available bytes */
	int64_t getAvailableBytes() const { return availableBytes; }
	
	/** Returns true if bootstrapping process must be done */
	bool needsBootstrap() const;
	
	/** Sends our info to specified user */
	void info(const OnlineUserPtr& ou, bool sendNodeList = false, bool response = false);
	
	/** Adds search request to queue */
	uint64_t search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, void* owner);
	
	/** Requires connection to node */
	void connect(const OnlineUser& ou, const string& token);

	/** Sends private message to connected node */
	void privateMessage(const OnlineUserPtr& user, const string& aMessage, bool thirdPerson = false);
	
	/** Bootstrapping response can occur in SearchManager, so allow receiving it here */
	void onINF(AdcCommand& cmd) { handle(AdcCommand::INF(), cmd); }
	
	/** Returns name of this network */
	string getName() const { return STRING(DECENTRALIZED_NETWORK); }
	
	/** Returns port used to listening to UDP socket */
	uint16_t getPort() const { return port;	}	
	
private:
	/** classes that can access to my private members */
	friend class Singleton<DecentralizationManager>;
	friend class CommandHandler<DecentralizationManager>;
	
	/** Map CID to OnlineUser */
	typedef unordered_map<CID, OnlineUser*> CIDMap;
	typedef CIDMap::const_iterator CIDIter;
	
	/** Stores all online nodes */
	CIDMap nodes;
	
	/** Locks access to "nodes" */
	mutable CriticalSection cs;
	
	/** Queue for sending search requests */
	SearchQueue searchQueue;
	
	/** Indicates to stop socket thread */
	bool stop;
	
	/** Port for communicating in this network */
	uint16_t port;
	
	/** UDP socket */
	std::auto_ptr<Socket> socket;
	
	/** Size of data available through this network */
	int64_t availableBytes;
	
	/** Thread for receiving UDP packets */
	int run();
	
	/** Finds online node by its CID */
	OnlineUserPtr findNode(const CID& cid) const;
	
	/** Creates online node if it doesn't exist else returns existing one */
	OnlineUserPtr getNode(const CID& cid);
	
	/** Get my IP address */
	string getLocalIp() const;
	
	/** Sends command to node */
	void send(AdcCommand& c, const OnlineUserPtr& ou);
	
	/** Loads existing nodes from disk */
	void loadNodes();
	
	/** Save all online nodes to disk */
	void saveNodes();
	
	/** Sends search request to all connected nodes */
	void search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken);
	
	/** All commands supported by this network */
	void handle(AdcCommand::INF, AdcCommand& c) throw(); // node is going online
	void handle(AdcCommand::QUI, AdcCommand& c) throw(); // node is going offline
	void handle(AdcCommand::SCH, AdcCommand& c) throw(); // incoming search request
	void handle(AdcCommand::RES, AdcCommand& c) throw(); // incoming search result
	void handle(AdcCommand::PSR, AdcCommand& c) throw(); // incoming partial search result
	void handle(AdcCommand::MSG, AdcCommand& c) throw(); // received private message

	/** Unsupported command */
	template<typename T> void handle(T, AdcCommand&) { }
	
	// TimerManagerListener
	void on(TimerManagerListener::Second, uint64_t aTick) throw();	
};

} // namespace dcpp

#endif // !defined(DCPLUSPLUS_DCPP_DECENTRALIZATION_MANAGER_H
