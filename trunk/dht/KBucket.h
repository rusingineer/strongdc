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

#include "../client/CID.h"
#include "../client/Pointer.h"
#include "../client/User.h"

namespace dht
{

	struct Node :
		public OnlineUser
	{
		typedef boost::intrusive_ptr<Node> Ptr;
		
		Node() : OnlineUser(NULL, *reinterpret_cast<Client*>(NULL), 0) { }
		Node(const UserPtr& u) : OnlineUser(u, *reinterpret_cast<Client*>(NULL), 0) { }
		~Node()	{ }
		
		GETSET(uint64_t, expires, Expires);
	};
		
	class KBucket
	{
	public:
		KBucket(void);
		~KBucket(void);
		
		/** Inserts node to bucket */
		Node::Ptr insert(const UserPtr& u);
		
		/** Finds "max" closest nodes and stores them to the list */
		void getClosestNodes(const CID& cid, std::map<CID, Node::Ptr>& closest, unsigned int max) const;
		
		/** Removes dead nodes */
		unsigned int checkExpiration(uint64_t currentTime);
		
	private:
	
		/** List of nodes in this bucket */
		typedef std::list<Node::Ptr> NodeList;
		NodeList nodes;
		
	};
	
}
