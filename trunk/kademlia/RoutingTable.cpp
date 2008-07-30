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
 
#include "StdAfx.h"
#include <DCPlusPlus.h>

#include "KademliaManager.h"
#include "RoutingTable.h"

#include <SimpleXML.h>

namespace kademlia
{

RoutingTable::RoutingTable(void)
{
}

RoutingTable::~RoutingTable(void)
{
}

OnlineUserPtr RoutingTable::add(const CID& cid, const string& ip, const string& tcpPort, const string& udpPort)
{
	Lock l(cs);
	OnlineUserPtr ou;
	CIDMap::const_iterator i = nodes.find(const_cast<CID*>(&cid));
	if(i != nodes.end())
	{
		ou = i->second;
	}
	else
	{
		// firstly connected node
		UserPtr p = ClientManager::getInstance()->getUser(cid);
	
		// TODO: nodes must be stored according to their zone
		ou = nodes.insert(make_pair(const_cast<CID*>(&p->getCID()), new OnlineUser(p, *reinterpret_cast<Client*>(NULL), 0))).first->second;
		ClientManager::getInstance()->putOnline(ou.get());
	}

	ou->getIdentity().setIp(ip);
	ou->getIdentity().set("T4", tcpPort);
	ou->getIdentity().setUdpPort(udpPort);
	
	return ou;
}
	
OnlineUserPtr RoutingTable::findNode(const CID& cid)
{
	Lock l(cs);
	CIDMap::iterator i = nodes.find(const_cast<CID*>(&cid));
	
	if(i != nodes.end())
		return i->second;
	
	return NULL;	
}

void RoutingTable::getClosestTo(const CID& cid, size_t maximum, NodeMap& results)
{
	// TODO:	following code is only temporary solution until whole routing table
	//			is divided to subzones
	results.clear();
	
	size_t count = 0;
	for(CIDMap::const_iterator i = nodes.begin(); i != nodes.end() && count < maximum; i++, count++)
	{
		results[*i->first] = i->second;
	}
}

void RoutingTable::loadNodes(SimpleXML& xml)
{
	if(xml.findChild("Nodes"))
	{
		xml.stepIn();
		while(xml.findChild("Node"))
		{
			const CID cid			= CID(xml.getChildAttrib("CID"));
			
			if(ClientManager::getInstance()->getMyCID() == cid)
				continue;
				
			const string& ip		= xml.getChildAttrib("IP");
			const string& tcpPort	= xml.getChildAttrib("TCP");
			const string& udpPort	= xml.getChildAttrib("UDP");

			add(cid, ip, tcpPort, udpPort);
		}
		xml.stepOut();
	}
}

void RoutingTable::saveNodes(SimpleXML& xml)
{
	xml.addTag("Nodes");
	xml.stepIn();

	for(CIDIter i = nodes.begin(); i != nodes.end(); i++)
	{
		const OnlineUserPtr& ou = i->second;
		if(ou->getUser() == ClientManager::getInstance()->getMe())
			continue;
					
		xml.addTag("Node");
		xml.addChildAttrib("CID", ou->getUser()->getCID().toBase32());
		xml.addChildAttrib("IP", ou->getIdentity().getIp());
		xml.addChildAttrib("TCP", ou->getIdentity().get("T4"));
		xml.addChildAttrib("UDP", ou->getIdentity().getUdpPort());
		// TODO: save features supported by node
	}
	
	xml.stepOut();
}

}