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
#include "ClientManager.h"
#include "SearchQueue.h"
#include "ResourceManager.h"
#include "TimerManager.h"
#include "User.h"

namespace dcpp
{

#define MCAST_IP					"234.5.6.7"											// IP address for multicasting
#define DPORT						SETTING(TLS_PORT)									// UDP port for listening to

#define ADC_PACKET_HEADER			'U'													// byte which every uncompressed packet must begin with
#define ADC_PACKET_FOOTER			0x0a												// byte which every uncompressed packet must end with
#define ADC_PACKED_PACKET_HEADER	0xc1												// compressed packet detection byte

#define NODE_FILE					"nodes.dat"											// local file with saved known nodes
#define NODE_ITEM_SIZE				sizeof(CID) + sizeof(uint32_t) + sizeof(uint16_t)	// size of one item in nodelist

#define MAX_SEARCH_NODES			3													// maximum perfect nodes to send search request to
#define MAX_SEARCH_DISTANCE			50	// CHECK: is this number ok?					// maximum distance for forwarding search request
#define NODE_EXPIRATION				3600	// 1 hour									// time when node can be marked as offline

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
	
	struct Packet
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
	
	struct Node
	{
		/** Public constructor */
		Node(uint64_t exp_, const OnlineUserPtr& ou_) : expires(exp_), onlineUser(ou_)
		{
		}
		
		/** Time when node should be removed */
		uint64_t expires;
		
		/** All information about node */
		OnlineUserPtr onlineUser;
	};
	
	/** Map CID to OnlineUser */
	typedef unordered_map<CID, Node> CIDMap;
	typedef CIDMap::const_iterator CIDIter;
	
	/** Stores all online nodes */
	CIDMap nodes;
	
	/** Locks access to "nodes" */
	// TODO: 
	// Use Fast critical section, because we don't need locking so often.
	// Almost all shared access is done within one thread.
	mutable CriticalSection cs;
	
	/** Queue for sending search requests */
	SearchQueue searchQueue;
	
	/** Indicates to stop socket thread */
	bool stop;
	
	/** Port for communicating in this network */
	uint16_t port;
	
	/** UDP socket */
	std::auto_ptr<Socket> socket;
	
	/** Queue for sending packets through UDP socket */
	std::deque<Packet*> sendQueue;
	
	/** Size of data available through this network */
	int64_t availableBytes;
	
	/** Thread for receiving UDP packets */
	int run();
	
	/** Finds online node by its CID */
	Node* findNode(const CID& cid, bool updateExpiration = true);
	
	/** Creates online node if it doesn't exist else returns existing one */
	Node& getNode(const CID& cid);
	
	/** Get my IP address */
	string getLocalIp() const;
	
	/** Sends command to node */
	void send(AdcCommand& c, const OnlineUserPtr& ou, const CID& cid = ClientManager::getInstance()->getMyCID())
	{ send(c, ou->getIdentity().getIp(), static_cast<uint16_t>(Util::toInt(ou->getIdentity().getUdpPort())), cid); }
	
	/** Sends command to IP and port */
	void send(AdcCommand& c, const string& ip, uint16_t port, const CID& cid = ClientManager::getInstance()->getMyCID());
	
	/** Loads existing nodes from disk */
	void loadNodes();
	
	/** Save all online nodes to disk */
	void saveNodes();
	
	/** Selects the best nodes for sending search request */
	void findPerfectNodes(std::vector<OnlineUserPtr>& v, const CID& originCID = CID()) const;
	
	/** Sends search request to all connected nodes */
	void search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken);
	
	/** All commands supported by this network */
	void handle(AdcCommand::INF, AdcCommand& c) throw(); // node is going online
	void handle(AdcCommand::SCH, AdcCommand& c) throw(); // incoming search request
	void handle(AdcCommand::RES, AdcCommand& c) throw(); // incoming search result
	void handle(AdcCommand::PSR, AdcCommand& c) throw(); // incoming partial search result
	void handle(AdcCommand::MSG, AdcCommand& c) throw(); // received private message

	/** Unsupported command */
	template<typename T> void handle(T, AdcCommand&) { }
	
	// TimerManagerListener
	void on(TimerManagerListener::Second, uint64_t aTick) throw();
	void on(TimerManagerListener::Minute, uint64_t aTick) throw();	
};

} // namespace dcpp

#endif // !defined(DCPLUSPLUS_DCPP_DECENTRALIZATION_MANAGER_H
