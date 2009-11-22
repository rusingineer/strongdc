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
 
#include "StdAfx.h"

#include "Constants.h"
#include "DHT.h"
#include "KBucket.h"
#include "Utils.h"

#include "../client/Client.h"
#include "../client/ClientManager.h"

namespace dht
{
	const string DHTName = "DHT";
	
	struct DHTClient : public ClientBase
	{
		DHTClient() { type = DHT; }
		
		const string& getHubUrl() const { return DHTName; }
		string getHubName() const { return DHTName; }
		bool isOp() const { return false; }
		void connect(const OnlineUser& user, const string& token) { DHT::getInstance()->connect(user, token); }
		void privateMessage(const OnlineUserPtr& user, const string& aMessage, bool thirdPerson = false) { DHT::getInstance()->privateMessage(*user.get(), aMessage, thirdPerson); }
	};
	
	static DHTClient client;
	
	// Set all new nodes' type to 3 to avoid spreading dead nodes..
	Node::Node(const UserPtr& u) : 
		OnlineUser(u, dht::client, 0), created(GET_TICK()), type(3), expires(0), ipVerified(false)
	{
	}

	CID Node::getUdpKey() const 
	{ 
		// if our external IP changed from the last time, we can't encrypt packet with this key
		if(DHT::getInstance()->getLastExternalIP() == UDPKey.ip)
			return UDPKey.key;
		else
			return CID();
	}
	
	void Node::setUdpKey(const CID& key)
	{ 
		// store key with our current IP address
		UDPKey.ip = DHT::getInstance()->getLastExternalIP();
		UDPKey.key = key;
	}
		
	void Node::setAlive()
	{
		// long existing nodes will probably be there for another long time
		uint64_t hours = (GET_TICK() - created) / 1000 / 60 / 60;
		switch(hours)
		{
			case 0:
				type = 2;
				expires = GET_TICK() + (NODE_EXPIRATION / 2);
				break;
			case 1:
				type = 1;
				expires = GET_TICK() + (uint64_t)(NODE_EXPIRATION / 1.5);
				break;
			default:	// long existing nodes
				type = 0;
				expires = GET_TICK() + NODE_EXPIRATION;			
		}
	}
	
	void Node::setTimeout(uint64_t now) 
	{ 
		if(type == 4) 
			return; 
		
		type++; 
		expires = now + NODE_RESPONSE_TIMEOUT; 
	}
	
	
	KBucket::KBucket(void)
	{
	}

	KBucket::~KBucket(void)
	{
	}
	
	/*
	 * Inserts node to bucket 
	 */
	Node::Ptr KBucket::insert(const UserPtr& u, const string& ip, uint64_t port, bool update, bool isUdpKeyValid)
	{
		if(u->isSet(User::DHT)) // is this user already known in DHT?
		{
			for(NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it)
			{
				Node::Ptr node = *it;
				if(u->getCID() == node->getUser()->getCID())
				{
					// node is already here, move it to the end
					if(update)
					{	
						string oldIp	= node->getIdentity().getIp();
						string oldPort	= node->getIdentity().getUdpPort();
						if(ip != oldIp || static_cast<uint16_t>(Util::toInt(oldPort)) != port)
						{
							node->setIpVerified(false);
							
							// erase old IP and remember new one
							ipMap.erase(oldIp + ":" + oldPort);
							ipMap.insert(ip + ":" + Util::toString(port));
						}
							
						if(!node->isIpVerified())
							node->setIpVerified(isUdpKeyValid);

						node->setAlive();
						node->getIdentity().setIp(ip);
						node->getIdentity().setUdpPort(Util::toString(port));
					}

					nodes.erase(it);
					nodes.push_back(node);
					
					DHT::getInstance()->setDirty();
					return node;
				}
			}
			
			//dcassert(0);			
			// this can happen when we already know this user, but it has not been added to routing table
		}

		// it's new DHT node
		u->setFlag(User::DHT);
		
		Node::Ptr node(new Node(u));
		node->getIdentity().setIp(ip);
		node->getIdentity().setUdpPort(Util::toString(port));	

		// is there already such contact with this IP?
		bool ipExists = ipMap.find(ip + ":" + Util::toString(port)) != ipMap.end();
		
#ifdef _DEBUG
		if(!ipExists)
#else
		if(!ipExists && nodes.size() < (K * ID_BITS))
#endif
		{
			// bucket still has room to store new node
			nodes.push_back(node);
			ipMap.insert(ip + ":" + Util::toString(port));
				
			if(DHT::getInstance())
				DHT::getInstance()->setDirty();
		}
		else
		{
			// when user isn't added to routing table, this line will be processed with every packet got from that user (that' why debugger window can be spammed with a lot of messages)
			dcdebug("DHT node NOT added to our routing table - IP %s exists: %d, table size: %d\n", ip.c_str(), (int)ipExists, nodes.size());
		}
		
		return node;
	}
	
	/*
	 * Finds "max" closest nodes and stores them to the list 
	 */
	void KBucket::getClosestNodes(const CID& cid, Node::Map& closest, unsigned int max, uint8_t maxType) const
	{
		for(NodeList::const_iterator it = nodes.begin(); it != nodes.end(); it++)
		{
			const Node::Ptr& node = *it;
			if(node->getType() <= maxType && node->isIpVerified() && !node->getUser()->isSet(User::PASSIVE))
			{
				CID distance = Utils::getDistance(cid, node->getUser()->getCID());

				if(closest.size() < max)
				{
					// just insert
					closest.insert(std::make_pair(distance, node));
				}
				else
				{
					// not enough room, so insert only closer nodes
					if(distance < closest.rbegin()->first)	// "closest" is sorted map, so just compare with last node
					{
						closest.erase(closest.rbegin()->first);
						closest.insert(std::make_pair(distance, node));
					}
				}
			}
		}
	}
	
	/*
	 * Remove dead nodes 
	 */
	bool KBucket::checkExpiration(uint64_t currentTime)
	{
		bool dirty = false;
		
		Node::Ptr oldest = NULL;
		
		// first, remove dead nodes		
		NodeList::iterator i = nodes.begin();
		while(i != nodes.end())
		{
			Node::Ptr& node = *i;
			
			if(node->getType() == 4 && node->expires > 0 && node->expires <= currentTime)
			{
				if(node->unique())
				{
					// node is dead, remove it
					if(node->isInList)
						ClientManager::getInstance()->putOffline((*i).get());
					
					string ip	= node->getIdentity().getIp();
					string port = node->getIdentity().getUdpPort();
					ipMap.erase(ip + ":" + port);
					i = nodes.erase(i);
					dirty = true;
				}
				else
				{
					++i;
				}
					
				continue;
			}
				
			// select the oldest expired node
			if(oldest == NULL && node->getType() < 4 && node->expires <= currentTime)
				oldest = node;
					
			++i;
		}
		
#ifdef _DEBUG
		int verified = 0; int types[5] = { 0 };
		for(NodeList::const_iterator j = nodes.begin(); j != nodes.end(); j++)
		{
			Node::Ptr n = *j;
			if(n->isIpVerified()) verified++;
			
			dcassert(n->getType() >= 0 && n->getType() <= 4);
			types[n->getType()]++;
		}
			
		dcdebug("DHT Nodes: %d (%d verified)\nTypes: %d/%d/%d/%d/%d\n", nodes.size(), verified, types[0], types[1], types[2], types[3], types[4]);
#endif
		
		if(oldest != NULL)
		{
			// ping the oldest (expired) node
			oldest->setTimeout(currentTime);
			DHT::getInstance()->info(oldest->getIdentity().getIp(), static_cast<uint16_t>(Util::toInt(oldest->getIdentity().getUdpPort())), DHT::PING, oldest->getUser()->getCID(), oldest->getUdpKey());
		}
		
		return dirty;
	}

	/*
	 * Loads existing nodes from disk 
	 */
	void KBucket::loadNodes(SimpleXML& xml)
	{
		xml.resetCurrentChild();
		if(xml.findChild("Nodes"))
		{
			xml.stepIn();
			while(xml.findChild("Node"))
			{
				CID cid		= CID(xml.getChildAttrib("CID"));
				string i4	= xml.getChildAttrib("I4");
				uint16_t u4	= static_cast<uint16_t>(xml.getIntChildAttrib("U4"));
				
				if(Utils::isGoodIPPort(i4, u4))
					//addUser(cid, i4, u4);
					BootstrapManager::getInstance()->addBootstrapNode(i4, u4, cid);
			}
			xml.stepOut();
		}	
	}
		
	/*
	 * Save bootstrap nodes to disk 
	 */
	void KBucket::saveNodes(SimpleXML& xml)
	{
		xml.addTag("Nodes");
		xml.stepIn();

		// get 50 random nodes to bootstrap from them next time
		Node::Map closestToMe;
		getClosestNodes(CID::generate(), closestToMe, 50, 3);
		
		for(Node::Map::const_iterator j = closestToMe.begin(); j != closestToMe.end(); j++)
		{
			const Node::Ptr& node = j->second;
					
			xml.addTag("Node");
			xml.addChildAttrib("CID", node->getUser()->getCID().toBase32());
			xml.addChildAttrib("type", node->getType());
			xml.addChildAttrib("verified", node->isIpVerified());

			StringMap params;
			node->getIdentity().getParams(params, Util::emptyString, false, true);
			
			for(StringMap::const_iterator i = params.begin(); i != params.end(); i++)
				xml.addChildAttrib(i->first, i->second);
		}
		
		xml.stepOut();	
	}
			
	
}
