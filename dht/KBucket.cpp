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
	
	// Set all new nodes' type to 1 to avoid spreading dead nodes..
	Node::Node() : 
		OnlineUser(NULL, dht::client, 0), lastTypeSet(GET_TICK()), type(1), expires(0) 
	{
	}
	
	Node::Node(const UserPtr& u) : 
		OnlineUser(u, dht::client, 0), lastTypeSet(GET_TICK()), type(1), expires(0) 
	{
	}

	void Node::setType(uint8_t _type)
	{
		uint64_t time = GET_TICK();
		if(_type != 0 && time - lastTypeSet < 10*1000)
		{
			return;
		}
		
		if(_type > 1 )
		{
			if(expires == 0) // Just in case..
				expires = time + NODE_RESPONSE_TIMEOUT;
			else if(type == 1)
				expires = time + NODE_RESPONSE_TIMEOUT;
			
			type = 2; //Just in case in case again..
			return;
		}

		type = _type;
		if(type == 0)
			expires = time + NODE_EXPIRATION;
		else 
			expires = time + (NODE_EXPIRATION / 2);		
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
	Node::Ptr KBucket::insert(const UserPtr& u)
	{
		bool dirty = false;
		for(NodeList::iterator it = nodes.begin(); it != nodes.end(); it++)
		{
			if(u->getCID() == (*it)->getUser()->getCID())
			{
				// node is already here, move it to the end
				Node::Ptr node = *it;
				node->setType(0);
				
				nodes.erase(it);
				nodes.push_back(node);
				
				DHT::getInstance()->setDirty();
				return node;
			}
		}
		
		Node::Ptr node = new Node(u);
		if(nodes.size() < (K * ID_BITS))
		{
			// bucket still has room to store new node
			nodes.push_back(node);
			dirty = true;
		}
		else
		{
			// TODO: we need to ping the first node in bucket and replace it
		}
			
		if(dirty && DHT::getInstance())
			DHT::getInstance()->setDirty();
			
		return node;	
	}
	
	/*
	 * Finds "max" closest nodes and stores them to the list 
	 */
	void KBucket::getClosestNodes(const CID& cid, std::map<CID, Node::Ptr>& closest, unsigned int max) const
	{
		for(NodeList::const_iterator it = nodes.begin(); it != nodes.end(); it++)
		{
			CID distance = Utils::getDistance(cid, (*it)->getUser()->getCID());
			string ip = (*it)->getIdentity().getIp();
			if(closest.size() < max)
			{
				// just insert
				closest.insert(std::make_pair(distance, *it));
			}
			else
			{
				// not enough room, so insert only closer nodes
				if(distance < closest.rbegin()->first)	// "closest" is sorted map, so just compare with last node
				{
					closest.erase(closest.rbegin()->first);
					closest.insert(std::make_pair(distance, *it));
				}
			}
		}
	}
	
	/*
	 * Remove dead nodes 
	 */
	unsigned int KBucket::checkExpiration(uint64_t currentTime)
	{
		unsigned int count = 0;
		
		NodeList::iterator i = nodes.begin();
		while(i != nodes.end())
		{
			Node::Ptr& node = *i;
			
			if(node->getType() > 1 && node->expires > 0 && node->expires <= currentTime)
			{
				// node is dead, remove it
				if((*i)->getUser()->isOnline())
					ClientManager::getInstance()->putOffline((*i).get());
					
				nodes.erase(i++);
				DHT::getInstance()->setDirty();
			}
			else
			{
				if(node->expires == 0)
					node->expires = currentTime;
					
				count++;
				++i;
			}
		}
		
		return count;
	}
	
	
}
