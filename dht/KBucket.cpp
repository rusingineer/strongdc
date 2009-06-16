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
#include "KBucket.h"
#include "Utils.h"

#include "../client/ClientManager.h"

namespace dht
{

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
		for(NodeList::iterator it = nodes.begin(); it != nodes.end(); it++)
		{
			if(u->getCID() == (*it)->getUser()->getCID())
			{
				// node is already here, move it to the end
				Node::Ptr node = *it;
				node->setExpires(GET_TICK() + NODE_EXPIRATION);
				
				nodes.erase(it);
				nodes.push_back(node);
				return node;
			}
		}
		
		Node::Ptr node = new Node(u);
		node->setExpires(GET_TICK() + NODE_EXPIRATION);
		
		if(nodes.size() < K)
		{
			// bucket still has room to store new node
			nodes.push_back(node);
		}
		else
		{
			// TODO: we need to ping the first node in bucket and replace it
		}
			
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
	
	/** Remove dead nodes */
	unsigned int KBucket::checkExpiration(uint64_t currentTime)
	{
		unsigned int count = 0;
		
		NodeList::iterator i = nodes.begin();
		while(i != nodes.end())
		{
			uint64_t expires = (*i)->getExpires();
			if(expires > 0 && expires <= currentTime)
			{
				// node is dead, remove it
				if((*i)->getUser()->isOnline())
					ClientManager::getInstance()->putOffline((*i).get());
					
				nodes.erase(i++);
			}
			else
			{
				count++;
				++i;
			}
		}
		
		return count;
	}
	
	
}
