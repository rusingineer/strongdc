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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "DecentralizationManager.h"
#include "SettingsManager.h"
#include "AdcCommand.h"
#include "ConnectionManager.h"
#include "ClientManager.h"
#include "CryptoManager.h"
#include "LogManager.h"
#include "SearchManager.h"
#include "SearchQueue.h"
#include "ShareManager.h"
#include "Socket.h"
#include "StringTokenizer.h"
#include "TimerManager.h"
#include "User.h"
#include "version.h"

#include "../zlib/zlib.h"
#include <mswsock.h>
#include <ws2tcpip.h>

namespace dcpp
{

const string ADCS_FEATURE("ADC0");
const string TCP4_FEATURE("TCP4");
const string UDP4_FEATURE("UDP4");

const uint32_t POLL_TIMEOUT = 250;

DecentralizationManager::DecentralizationManager(void) : stop(false), port(0), availableBytes(0)
{
}

DecentralizationManager::~DecentralizationManager(void)
{
	disconnect();
}

void DecentralizationManager::listen() throw(SocketException)
{
	disconnect();
	
	if(!BOOLSETTING(ENABLE_DECENTRALIZED_NETWORK) || SETTING(INCOMING_CONNECTIONS) == SettingsManager::INCOMING_FIREWALL_PASSIVE)
		return;
	
	try
	{
		socket.reset(new Socket);
		socket->create(Socket::TYPE_UDP);
		
		// TODO: make specific port for this network
		port = socket->bind(static_cast<uint16_t>(DPORT), SETTING(BIND_ADDRESS));
		
		// multicasting will probably work only in local network, but it could be
		// the first try for bootstrapping.	
		struct ip_mreq mreq;
				
		// join multicast group
		mreq.imr_multiaddr.s_addr = inet_addr(MCAST_IP);
		mreq.imr_interface.s_addr = INADDR_ANY;
		setsockopt(socket->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));

		// set multicast time to live
		int iOptVal = 64;
		setsockopt(socket->sock, IPPROTO_IP,  IP_MULTICAST_TTL, (char FAR *)&iOptVal, sizeof (int));
						
		start();
	}
	catch(...)
	{
		socket.reset();
		throw;
	}		
}

void DecentralizationManager::disconnect() throw()
{
	if(socket.get())
	{
		stop = true;
#ifdef _WIN32
		join();
#endif
		socket->disconnect();
		socket.reset();

		port = 0;		
		stop = false;	
	}
}

#define BUFSIZE 32768
/* This thread should manage to send outgoing packets too. */
/* Also there should be done some mechanism for sending acks. */
int DecentralizationManager::run()
{
	// wait 2 sec before continuing
	Thread::sleep(2000);
	
	searchQueue.interval = SETTING(MINIMUM_SEARCH_INTERVAL) * 1000 + 2000;
	
	boost::scoped_array<uint8_t> buf(new uint8_t[BUFSIZE]);

	TimerManager::getInstance()->addListener(this);
	
	// fix receiving when sending fails
	DWORD value = FALSE;
	ioctlsocket(socket->sock, SIO_UDP_CONNRESET, &value);
	
	loadNodes();
	
	string remoteAddr;
	while(!stop)
	{
		try
		{
			Packet* p = NULL;
			{
				Lock l(cs);
				if(!sendQueue.empty())
				{
					// take the first packet in queue
					p = sendQueue.front();
					sendQueue.pop_front();
				}
			}
			
			if(p != NULL)
			{
				// TODO: introduce some delay for sending packets not to flood UDP socket
				try
				{
					socket->writeTo(p->ip, p->port, p->data, p->length);
				}
				catch(SocketException& e)
				{
					dcdebug("DecentralizationManager::run Write error: %s\n", e.getError());
				}
				
				delete[] p->data;
				delete p;
			}
			
			// check for incoming data
			if(socket->wait(POLL_TIMEOUT, Socket::WAIT_READ) == Socket::WAIT_READ)
			{
				// TODO: check flooding
				int len = socket->read(&buf[0], BUFSIZE, remoteAddr);
				
				if(len > 1)
				{
					uLongf destLen = BUFSIZE; // what size should be reserved?
					uint8_t* destBuf;
					
					if(buf[0] == ADC_PACKED_PACKET_HEADER) // is this compressed packet?
					{
						destBuf = new uint8_t[destLen];
						
						// decompress incoming packet
						int result = uncompress(destBuf, &destLen, &buf[0] + 1, len - 1);
						if(result != Z_OK)
						{
							// decompression error!!!
							delete[] destBuf;
							continue;
						}
					}
					else
					{
						destBuf = &buf[0];
						destLen = len;
					}
					
					// process decompressed packet
					string s((char*)destBuf, destLen);
					if(s[0] == ADC_PACKET_HEADER && s[s.length() - 1] == ADC_PACKET_FOOTER)	// is it valid ADC command?
					{
						// TODO: forward remote IP address
						dispatch(s.substr(0, s.length()-1));
						
						// TODO: send ack that packet received successfully
					}
				}
			}
		}
		catch(const SocketException& e)
		{
			dcdebug("DecentralizationManager::run Error: %s\n", e.getError().c_str());
			
			bool failed = false;
			while(!stop)
			{
				try
				{
					socket->disconnect();
					socket->create(Socket::TYPE_UDP);
					socket->bind(port, SETTING(BIND_ADDRESS));
					if(failed)
					{
						LogManager::getInstance()->message("Decentralization network enabled again"); // TODO: translate
						failed = false;
					}
					break;
				}
				catch(const SocketException& e)
				{
					dcdebug("DecentralizationManager::run Stopped listening: %s\n", e.getError().c_str());

					if(!failed)
					{
						LogManager::getInstance()->message("Decentralization network disabled: " + e.getError()); // TODO: translate
						failed = true;
					}

					// Spin for 60 seconds
					for(int i = 0; i < 60 && !stop; ++i)
					{
						Thread::sleep(1000);
					}
				}
			}
		}		
	}
	
	// save all nodes that are online at this time
	saveNodes();
	
	TimerManager::getInstance()->removeListener(this);
	
	return 0;
}

void DecentralizationManager::loadNodes()
{
	try
	{
		File file(Util::getConfigPath() + NODE_FILE, File::READ, File::OPEN);
		
		// load existing node list
		for(size_t i = 0; i < file.getSize(); i += sizeof(CID) + sizeof(uint32_t) + sizeof(uint16_t))
		{
			CID cid;
			uint32_t ipnum;
			uint16_t udpPort;
			
			size_t len = sizeof(CID);
			file.read(&cid, len);
			
			len = sizeof(uint32_t);
			file.read(&ipnum, len);
			
			len = sizeof(uint16_t);
			file.read(&udpPort, len);
			
			// ignore myself in nodelist
			if(cid == ClientManager::getInstance()->getMyCID())
				continue;
				
			char ipstr[16];
			sprintf(ipstr, "%ld.%ld.%ld.%ld", (ipnum >> 24) & 0xFF, (ipnum >> 16) & 0xFF, (ipnum >>  8) & 0xFF, ipnum & 0xFF);
			dcdebug("Loading node %s, IP = %s, port = %d\n", cid.toBase32().c_str(), ipstr, udpPort);
				
			OnlineUserPtr newNode = getNode(cid).onlineUser;
			newNode->getIdentity().setIp(ipstr);				
			newNode->getIdentity().setUdpPort(Util::toString(udpPort));
				
			// notify new node that we're going online
			info(newNode, false, true);
		}
	}
	catch(FileException& e)
	{
		dcdebug("%s\n", e.getError().c_str());
	}
}

void DecentralizationManager::saveNodes()
{
	try
	{
		if(!nodes.empty())
		{
			File file(Util::getConfigPath() + NODE_FILE, File::RW, File::OPEN | File::CREATE);
			file.setSize(0);
			
			// save existing node list
			for(CIDIter i = nodes.begin(); i != nodes.end(); i++)
			{
				const OnlineUserPtr& u = i->second.onlineUser;
				
				// don't include myself and node we are sending to in node list
				if(u->getUser() == ClientManager::getInstance()->getMe())
					continue;
					
				string ipstr = u->getIdentity().getIp();
				uint32_t ipnum = ::ntohl(::inet_addr(ipstr.c_str()));
				dcassert(ipnum != INADDR_NONE);
					
				uint16_t udpPort = static_cast<uint16_t>(Util::toInt(u->getIdentity().getUdpPort()));
					
				file.write(&u->getUser()->getCID(), sizeof(CID));
				file.write(&ipnum, sizeof(uint32_t));
				file.write(&udpPort, sizeof(uint16_t));
				
				dcdebug("Saving node %s\n", u->getUser()->getCID().toBase32().c_str());
			}
		}
	}
	catch(const FileException&)
	{
	}
}

bool DecentralizationManager::needsBootstrap() const
{
	if(!BOOLSETTING(ENABLE_DECENTRALIZED_NETWORK))
		return false;
		
	// TODO: do bootstrapping only when it's really needed
	
	return true;
}

DecentralizationManager::Node* DecentralizationManager::findNode(const CID& cid, bool updateExpiration)
{
	dcdebug("MyCID: %s\n", ClientManager::getInstance()->getMyCID().toBase32().c_str());
	Lock l(cs);
	CIDMap::iterator i = nodes.find(cid);
	
	if(i != nodes.end())
	{
		if(updateExpiration)
		{
			// user is active, update expiration
			i->second.expires = GET_TICK() + NODE_EXPIRATION;
		}
		
		return &i->second;
	}
	
	return NULL;
}

DecentralizationManager::Node& DecentralizationManager::getNode(const CID& cid)
{
	Node* node = findNode(cid);
	
	if(node == NULL)
	{
		// firstly connected node
		UserPtr p = ClientManager::getInstance()->getUser(cid);

		{
			Lock l(cs);
			node = &nodes.insert(make_pair(cid, Node(GET_TICK() + NODE_EXPIRATION, new OnlineUser(p, *reinterpret_cast<Client*>(this), 0)))).first->second;
		}

		node->onlineUser->getIdentity().setStatus(Util::toString(Identity::DSN));
		ClientManager::getInstance()->putOnline(node->onlineUser.get());		
	}
	
	return *node;
}

void DecentralizationManager::send(AdcCommand& cmd, const string& ip, uint16_t port, const CID& cid)
{
	if(!(cid == ClientManager::getInstance()->getMyCID()))
	{
		// when forwarding we must include original IP and port
		// TODO: check that it's not already present
		Node* node = findNode(cid);
		dcassert(node != NULL);
		
		cmd.addParam("I4", node->onlineUser->getIdentity().getIp());
		cmd.addParam("U4", node->onlineUser->getIdentity().getUdpPort());
	}
	
	string command = cmd.toString(cid);
	
	// compress data to have at least some kind of "encryption"
	uLongf destSize = compressBound(command.length()) + 1;
	
	uint8_t* srcBuf = (uint8_t*)command.data();
	uint8_t* destBuf = new uint8_t[destSize];
	
	// packets are small, so compressed them using fast method
	int result = compress2(destBuf + 1, &destSize, srcBuf, command.length(), Z_BEST_SPEED);
	if(result == Z_OK && destSize <= command.length())
	{
		destBuf[0] = ADC_PACKED_PACKET_HEADER;
		destSize += 1;
		
		dcdebug("Packet compressed successfuly, original size = %d bytes, new size %d bytes\n", command.length() + 1, destSize);
	}
	else
	{
		// compression failed, send uncompressed packet

		destSize = command.length();
		memcpy(destBuf, srcBuf, destSize);
				
	}
	
	Lock l(cs);
	sendQueue.push_back(new Packet(ip, port, destBuf, destSize)); 
}

void DecentralizationManager::info(const OnlineUserPtr& ou, bool sendNodeList, bool response)
{
	AdcCommand cmd(AdcCommand::CMD_INF, AdcCommand::TYPE_UDP);
	
	// TODO: incremental update
	cmd.addParam("NI", SETTING(NICK));
	cmd.addParam("I4", getLocalIp());
	cmd.addParam("T4", Util::toString(SETTING(TLS_PORT)));
	cmd.addParam("U4", Util::toString(DPORT));	
	cmd.addParam("SL", Util::toString(SETTING(SLOTS)));
	cmd.addParam("SS", ShareManager::getInstance()->getShareSizeString());
	cmd.addParam("SF", Util::toString(ShareManager::getInstance()->getSharedFiles()));
	cmd.addParam("VE", "StrgDC++ " VERSIONSTRING);
	cmd.addParam("RE", response ? "1" : "0");
	
	if (SETTING(THROTTLE_ENABLE) && SETTING(MAX_UPLOAD_SPEED_LIMIT) != 0)
	{
		cmd.addParam("US", Util::toString(SETTING(MAX_UPLOAD_SPEED_LIMIT) * 1024 * 8));
	}
	else
	{
		cmd.addParam("US", Util::toString((long)(Util::toDouble(SETTING(UPLOAD_SPEED)) * 1024 * 1024)));
	}	
	
	string su;
	if(CryptoManager::getInstance()->TLSOk())
	{
		su += ADCS_FEATURE + ",";
	}

	cmd.addParam("SU", su + TCP4_FEATURE + "," + UDP4_FEATURE);
		
	if(sendNodeList)
	{
		Lock l(cs);
		
		if(nodes.size() > 0)
		{
			// node list contains CID, IP and UDP port
			size_t len = nodes.size() * NODE_ITEM_SIZE;
			
			uint8_t* buf = new uint8_t[len];
			uint8_t* tmp = buf;
			bool added = false;
			
#define writeBuf(a, b, c) \
	memcpy(a, b, c); \
	a += c;
				
			for(CIDIter i = nodes.begin(); i != nodes.end(); i++)
			{
				const OnlineUserPtr& u = i->second.onlineUser;
				
				// don't include myself and node we are sending to in node list
				if(u->getUser() == ClientManager::getInstance()->getMe() || u->getUser() == ou->getUser())
					continue;
				
				string ipstr = u->getIdentity().getIp();
				uint32_t ipnum = ::ntohl(::inet_addr(ipstr.c_str()));
				dcassert(ipnum != INADDR_NONE);
				
				uint16_t udpPort = static_cast<uint16_t>(Util::toInt(u->getIdentity().getUdpPort()));
				
				writeBuf(tmp, &u->getUser()->getCID(), sizeof(CID));
				writeBuf(tmp, &ipnum, sizeof(uint32_t));
				writeBuf(tmp, &udpPort, sizeof(uint16_t));
				
				added = true;
			}

#undef writeBuf
			
			if(added)
				cmd.addParam("NL", string((char*)&buf[0], len));
			
			delete[] buf;
		}
	}
	
	dcdebug("Sending my info to node %s\n", ou->getUser()->getCID().toBase32().c_str());
	send(cmd, ou);
}

uint64_t DecentralizationManager::search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, void* owner)
{
	dcdebug("Queue search %s\n", aString.c_str());

	if(searchQueue.interval)
	{
		Search s;
		s.fileType = aFileType;
		s.size     = aSize;
		s.query    = aString;
		s.sizeType = aSizeMode;
		s.token    = aToken;
		s.owners.insert(owner);

		searchQueue.add(s);

		return searchQueue.getSearchTime(owner) - GET_TICK();
	}

	search(aSizeMode, aSize, aFileType , aString, aToken);
	return 0;

}

// TODO: completely refine this method
void DecentralizationManager::findPerfectNodes(std::vector<OnlineUserPtr>& v, const CID& originCID) const
{
	Lock l(cs);

	std::vector<OnlineUserPtr> tmp;
	
	// select some perfect nodes
	for(CIDIter i = nodes.begin(); i != nodes.end(); i++)
	{
		const OnlineUserPtr& ou = i->second.onlineUser;
		
		// don't sent anything to myself
		if(ou->getUser() == ClientManager::getInstance()->getMe())
			continue;
			
		// don't sent anything to originator of the request
		if(ou->getUser()->getCID() == originCID)
			continue;
		
		tmp.push_back(ou);
	}
	
	if(tmp.size() > MAX_SEARCH_NODES)
	{
		// select random nodes
		std::random_sample_n(tmp.begin(), tmp.end(), v.begin(), MAX_SEARCH_NODES);
	}
	else
	{
		v.swap(tmp);
	}
}

void DecentralizationManager::search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken)
{
	AdcCommand cmd(AdcCommand::CMD_SCH, AdcCommand::TYPE_UDP);
	
	if(aFileType == SearchManager::TYPE_TTH)
	{
		cmd.addParam("TR", aString);
	}
	else
	{
		if(aSizeMode == SearchManager::SIZE_ATLEAST)
		{
			cmd.addParam("GE", Util::toString(aSize));
		}
		else if(aSizeMode == SearchManager::SIZE_ATMOST)
		{
			cmd.addParam("LE", Util::toString(aSize));
		}
		StringTokenizer<string> st(aString, ' ');
		for(StringIter i = st.getTokens().begin(); i != st.getTokens().end(); ++i)
		{
			cmd.addParam("AN", *i);
		}
		if(aFileType == SearchManager::TYPE_DIRECTORY)
		{
			cmd.addParam("TY", "2");
		}
	}

	if(!aToken.empty())
		cmd.addParam("TO", aToken);
		
	cmd.addParam("DI", Util::toString(MAX_SEARCH_DISTANCE));

	vector<OnlineUserPtr> perfectNodes;
	findPerfectNodes(perfectNodes);
	
	for(vector<OnlineUserPtr>::const_iterator i = perfectNodes.begin(); i != perfectNodes.end(); i++)
	{
		send(cmd, *i);
	}
}

void DecentralizationManager::connect(const OnlineUser& ou, const string& token)
{
	// no connections to myself
	if(ou.getUser() == ClientManager::getInstance()->getMe())
		return;
		
	// no connections to passive users
	if(!ou.getIdentity().isTcpActive() || !ou.getIdentity().isSet("T4"))
		return;
	
	ConnectionManager::getInstance()->adcConnect(ou, static_cast<uint16_t>(Util::toInt(ou.getIdentity().get("T4"))), token, ou.getUser()->isSet(User::TLS) && CryptoManager::getInstance()->TLSOk());
}

void DecentralizationManager::privateMessage(const OnlineUserPtr& user, const string& aMessage, bool thirdPerson)
{
	AdcCommand cmd(AdcCommand::CMD_MSG, AdcCommand::TYPE_UDP);
	cmd.addParam(aMessage);
	if(thirdPerson)
		cmd.addParam("ME", "1");	
	send(cmd, user);
}

string DecentralizationManager::getLocalIp() const
{
	if(!SETTING(EXTERNAL_IP).empty())
	{
		return Socket::resolve(SETTING(EXTERNAL_IP));
	}

	return Util::getLocalIp();
}

void DecentralizationManager::handle(AdcCommand::INF, AdcCommand& c) throw()
{
	if(c.getParameters().empty())
		return;
		
	// get node's cid
	string cid = c.getParam(0);
	if(cid.size() != 39)
		return;
		
	Node& node = getNode(CID(cid));
	const OnlineUserPtr& ou = node.onlineUser;
	
	string nodeList;
	uint8_t response = 0;
	
	// get node's properties
	for(StringIterC i = c.getParameters().begin(); i != c.getParameters().end(); ++i)
	{
		if(i->length() < 2)
			continue;

		if(i->substr(0, 2) == "SS")
		{
			availableBytes -= ou->getIdentity().getBytesShared();
			ou->getIdentity().setBytesShared(i->substr(2));
			availableBytes += ou->getIdentity().getBytesShared();
		}
		else if(i->substr(0, 2) == "NL")
		{
			// this is node list
			nodeList = i->substr(2);
		}
		else if(i->substr(0, 2) == "RE")
		{
			// user requires our info too
			response = (i->substr(2) == "1");
		}
		else
		{
			ou->getIdentity().set(i->c_str(), i->substr(2));
		}
	}

	if(ou->getIdentity().supports(ADCS_FEATURE))
	{
		ou->getUser()->setFlag(User::TLS);
	}
	
	if(response)
	{
		info(ou, response == 2);
	}
	
	if(!nodeList.empty() && (nodeList.size() % NODE_ITEM_SIZE == 0))
	{
		dcdebug("nodeList size = %d\n", nodeList.size());
		
		for(size_t i = 0; i < nodeList.size(); i += NODE_ITEM_SIZE)
		{
			CID cid;
			uint32_t ipnum;
			uint16_t udpPort;
			
			memcpy(&cid, (uint8_t*)nodeList.substr(i, sizeof(CID)).c_str(), sizeof(CID));
			memcpy(&ipnum, (uint8_t*)nodeList.substr(i + sizeof(CID), sizeof(uint32_t)).c_str(), sizeof(uint32_t));
			memcpy(&udpPort, (uint8_t*)nodeList.substr(i + sizeof(CID) + sizeof(uint32_t), sizeof(uint16_t)).c_str(), sizeof(uint16_t));
			
			// ignore myself in nodelist
			if(cid == ClientManager::getInstance()->getMyCID())
				continue;
				
			Node* node = findNode(cid, false);
			if(node == NULL)
			{
				char ipstr[16];
				sprintf(ipstr, "%ld.%ld.%ld.%ld", (ipnum >> 24) & 0xFF, (ipnum >> 16) & 0xFF, (ipnum >>  8) & 0xFF, ipnum & 0xFF);
				dcdebug("Found new node %s, IP = %s, port = %d\n", cid.toBase32().c_str(), ipstr, udpPort);
				
				Node& newNode = getNode(cid);
				newNode.onlineUser->getIdentity().setIp(ipstr);				
				newNode.onlineUser->getIdentity().setUdpPort(Util::toString(udpPort));
				
				// notify new node that we exist
				info(newNode.onlineUser, false);
			}
		}
	}
}

void DecentralizationManager::handle(AdcCommand::SCH, AdcCommand& c) throw()
{
	if(c.getParameters().empty())
		return;
	
	CID cid = CID(c.getParam(0));
	
	Node* node = findNode(cid);
	if(node == NULL)
	{
		string ip, udpport;
		if(c.getParam("I4", 0, ip) && c.getParam("U4", 0, udpport))
		{
			// fine, it's forwarded packet
			node = &getNode(cid);
			node->onlineUser->getIdentity().setIp(ip);
			node->onlineUser->getIdentity().setUdpPort(udpport);
			
			// TODO: should we require node's info now?
		}
		else
		{
			return;
		}
	}
	
	c.getParameters().erase(c.getParameters().begin());
	SearchManager::getInstance()->respond(c, cid);
	
	string dist;
	if(c.getParam("DI", 0, dist) && Util::toInt(dist) > 0)
	{
		// fine, this search request can be forwarded to next perfect node
		for(StringIter i = c.getParameters().begin(); i != c.getParameters().end(); ++i)
		{
			if(AdcCommand::toCode((*i).c_str()) == AdcCommand::toCode("DI"))
			{
				c.getParameters().erase(i);
				break;
			}
		}		
		c.addParam("DI", Util::toString(Util::toInt(dist) - 1)); // lower the distance
		
		vector<OnlineUserPtr> perfectNodes;
		findPerfectNodes(perfectNodes, cid);
		
		for(vector<OnlineUserPtr>::const_iterator i = perfectNodes.begin(); i != perfectNodes.end(); i++)
		{
			send(c, *i, cid);
		}		
	}
}

void DecentralizationManager::handle(AdcCommand::RES, AdcCommand& c) throw()
{
	if(c.getParameters().empty())
		return;
		
	Node* node = findNode(CID(c.getParam(0)));
	if(node == NULL) return;
	
	SearchManager::getInstance()->onRES(c, node->onlineUser->getUser());
}

void DecentralizationManager::handle(AdcCommand::PSR, AdcCommand& c) throw()
{
	if(c.getParameters().empty())
		return;
		
	Node* node = findNode(CID(c.getParam(0)));
	if(node == NULL) return;
	
	SearchManager::getInstance()->onPSR(c, node->onlineUser->getUser());		
}

void DecentralizationManager::handle(AdcCommand::MSG, AdcCommand& c) throw()
{
	if(c.getParameters().empty())
		return;
		
	Node* from = findNode(CID(c.getParam(0)));
	if(from == NULL)
		return;
		
	// TODO fire(ClientListener::PrivateMessage(), this, *from, to, replyTo, c.getParam(1), c.hasFlag("ME", 1));
}

void DecentralizationManager::on(TimerManagerListener::Second, uint64_t aTick) throw()
{
	if(!searchQueue.interval) return;

	Search s;
		
	if(searchQueue.pop(s))
	{
		search(s.sizeType, s.size, s.fileType , s.query, s.token);
	}
}

void DecentralizationManager::on(TimerManagerListener::Minute, uint64_t aTick) throw()
{
	vector<OnlineUserPtr> expiredNodes;
	
	{ // check node expiration
		Lock l(cs);
		for(CIDIter i = nodes.begin(); i != nodes.end(); i++)
		{
			if(i->second.expires >= aTick)
				expiredNodes.push_back(i->second.onlineUser);
		}
	}
	
	// now remove expired nodes
	for(vector<OnlineUserPtr>::const_iterator i = expiredNodes.begin(); i != expiredNodes.end(); i++)
	{
		const OnlineUserPtr& ou = *i;
	
		{
			Lock l(cs);
			dcassert(nodes.find(ou->getUser()->getCID()) != nodes.end());
			nodes.erase(nodes.find(ou->getUser()->getCID()));
			
			availableBytes -= ou->getIdentity().getBytesShared();
		}	
	
		ClientManager::getInstance()->putOffline(ou.get(), true);
	}
}

} // namespace dcpp
