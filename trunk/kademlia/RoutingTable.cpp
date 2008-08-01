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
#include "KadUtils.h"
#include "RoutingTable.h"

#include <CID.h>
#include <ClientManager.h>
#include <SimpleXML.h>

namespace kademlia
{

RoutingTable::RoutingTable(void) : level(0)
{
	subZones[0] = NULL;
	subZones[1] = NULL;
	nodes = new CIDMap;
	
	uint8_t zero[CID::SIZE] = { 0 };
	zoneIndex = CID(zero);
}

RoutingTable::~RoutingTable(void)
{
}

OnlineUserPtr RoutingTable::add(const CID& cid)
{
	// don't store myself in routing table
	if(cid == ClientManager::getInstance()->getMyCID())
		return NULL; // really NULL???
		
	CID distance = KadUtils::getDistance(cid, ClientManager::getInstance()->getMyCID());
	
	Lock l(cs);
	if(nodes == NULL)
	{
		return subZones[KadUtils::getBit(distance.data(), level)]->add(cid);
	}
	else
	{
		// TODO: split and merge subzones
		CIDMap::const_iterator i = nodes->find(const_cast<CID*>(&cid));
		if(i != nodes->end())
			return i->second;

		// TODO: add only when not full
		// firstly connected node
		UserPtr p = ClientManager::getInstance()->getUser(cid);
		OnlineUserPtr ou = nodes->insert(make_pair(const_cast<CID*>(&p->getCID()), new OnlineUser(p, *reinterpret_cast<Client*>(NULL), 0))).first->second;
		ClientManager::getInstance()->putOnline(ou.get());

		return ou;		
	}
}
	
//OnlineUserPtr RoutingTable::findNode(const CID& cid)
//{
//	Lock l(cs);
//	if(nodes != NULL)
//	{
//		CIDMap::iterator i = nodes.find(const_cast<CID*>(&cid));
//		if(i != nodes.end())
//			return i->second;
//	
//		return NULL;
//	}
//	else
//	{
//		// TODO: should there really be bit from cid???
//		return subZones[KadUtils::getBit(cid.data(), level)]->findNode(cid);
//	}
//}

size_t RoutingTable::getClosestTo(const CID& cid, size_t maximum, NodeMap& results) const
{
	Lock l(cs);
	if(nodes != NULL)
	{
		//results.clear();

		size_t count = 0;
		for(CIDMap::const_iterator i = nodes->begin(); i != nodes->end() && count < maximum; i++, count++)
		{
			CID distance = KadUtils::getDistance(cid, *i->first);
			results[distance] = i->second;
		}
		return count;		
	}
	else
	{
		// otherwise, recurse in the closer-to-the-target subzone first
		uint8_t closer = KadUtils::getBit(cid.data(), level);
		size_t found = subZones[closer]->getClosestTo(cid, maximum, results);

		// if still not enough tokens found, recurse in the other subzone too
		if (found < maximum)
			found += subZones[1 - closer]->getClosestTo(cid, maximum - found, results);

		return found;	
	}
}

void RoutingTable::getAllNodes(NodeList& list) const
{
	Lock l(cs);
	if(nodes != NULL)
	{
		std::transform(nodes->begin(), nodes->end(), std::back_inserter(list),
			select2nd<CIDMap::value_type>());
	}
	else
	{
		subZones[0]->getAllNodes(list);
		subZones[1]->getAllNodes(list);
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
			const string& udpPort	= xml.getChildAttrib("UDP");

			OnlineUserPtr ou = add(cid);
			ou->getIdentity().setIp(ip);
			ou->getIdentity().setUdpPort(udpPort);
			// TODO: nick			
		}
		xml.stepOut();
	}
}

void RoutingTable::saveNodes(SimpleXML& xml)
{
	xml.addTag("Nodes");
	xml.stepIn();
	
	NodeList list;
	getAllNodes(list);

	for(NodeList::const_iterator i = list.begin(); i != list.end(); i++)
	{
		xml.addTag("Node");
		xml.addChildAttrib("CID", (*i)->getUser()->getCID().toBase32());
		xml.addChildAttrib("IP", (*i)->getIdentity().getIp());
		xml.addChildAttrib("UDP", (*i)->getIdentity().getUdpPort());
		// TODO: save features supported by node
	}
	
	xml.stepOut();
}

}