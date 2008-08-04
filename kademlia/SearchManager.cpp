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

#include "IndexManager.h"
#include "KademliaManager.h"
#include "KadUtils.h"
#include "SearchManager.h"

#include <AdcCommand.h>
#include <CID.h>
#include <CryptoManager.h>
#include <MerkleTree.h>
#include <SearchResult.h>
#include <SimpleXML.h>
#include <Speaker.h>
#include <Streams.h>
#include <Util.h>

namespace kademlia
{

SearchManager::SearchManager(void)
{
}

SearchManager::~SearchManager(void)
{
}

void Search::sendRequest(const string& ip, uint16_t port)
{
	AdcCommand cmd(AdcCommand::CMD_REQ, AdcCommand::TYPE_UDP);
	cmd.addParam("TR", term);
	cmd.addParam("TO", token);
	
	size_t returnedNodes = 0;
	switch(type)
	{
		case TYPE_FILE:
			returnedNodes = 2;
			break;
		case TYPE_NODE:
			returnedNodes = 12;
			break;
		case TYPE_STOREFILE:
			returnedNodes = 4;
			break;
	}
	
	cmd.addParam("MN", Util::toString(returnedNodes));
		
	KademliaManager::getInstance()->send(cmd, ip, port);
}

void Search::processResponse(const CID& cid, NodeList& results)
{
	CID* fromDistance = NULL;
	OnlineUserPtr from = NULL;
	
	// find cid in tried nodes
	for(TriedMap::const_iterator tm = triedNodes.begin(); tm != triedNodes.end(); tm++)
	{
		if(tm->second->getUser()->getCID() == cid)
		{
			from = tm->second;
			fromDistance = const_cast<CID*>(&tm->first);
			respondedNodes[tm->first] = from;
			break;
		}
	}
	
	if(fromDistance != NULL)	// is this distance from contacted node?
	{
		size_t bestNodes = 0;
		bool returnedCloser = false;
		for(NodeList::const_iterator i = results.begin(); i != results.end(); i++)
		{
			CID distance = KadUtils::getDistance((*i)->getUser()->getCID(), CID(term));
			
			if(triedNodes.count(distance) > 0)
				return;	// already tried
				
			if(distance < *fromDistance)
			{
				returnedCloser = true;
				if(bestNodes++ < SEARCH_ALPHA)
				{
					// add to tried nodes
					triedNodes[distance] = *i;
					
					// set possibly dead node timeout
					(*i)->getIdentity().set("EX", Util::toString(GET_TICK() + NODE_RESPONSE_TIMEOUT));
					
					// send search request
					sendRequest((*i)->getIdentity().getIp(), static_cast<uint16_t>(Util::toInt((*i)->getIdentity().getUdpPort())));
				}
				else
				{
					// too much nodes, store it for future
					possibleNodes[distance] = *i;
				}
			}
		}
		
		if(type != TYPE_NODE && !returnedCloser && KadUtils::get32BitChunk(fromDistance->data()) > SEARCH_TOLERANCE)
		{
			// if the node is the closest one, ask him for file sources/publishing file
			AdcCommand cmd(0, AdcCommand::TYPE_UDP);
			switch(type)
			{
				case TYPE_FILE:
					cmd = AdcCommand(AdcCommand::CMD_SCH, AdcCommand::TYPE_UDP);
					break;
				case TYPE_STOREFILE:
					cmd = AdcCommand(AdcCommand::CMD_PUB, AdcCommand::TYPE_UDP);
					cmd.addParam("SI", Util::toString(fileSize));
					break;
			}
			cmd.addParam("TR", term);
			cmd.addParam("TO", token);
			KademliaManager::getInstance()->send(cmd, from->getIdentity().getIp(), static_cast<uint16_t>(Util::toInt(from->getIdentity().getUdpPort())));
		}
	}
}

void Search::process()
{
	if(possibleNodes.empty())
	{
		if(startTime + SEARCHNODE_LIFETIME > GET_TICK() + 15*1000)
			startTime = GET_TICK() - SEARCHNODE_LIFETIME - 15*1000;
		return;
	}
	
	// remove obsolete nodes
	if(!respondedNodes.empty())
	{
		CID bestDistance = respondedNodes.begin()->first;
		NodeMap::iterator it = possibleNodes.begin();
		while(it != possibleNodes.end())
		{
			if(it->first < bestDistance)
			{
				NodeMap::iterator tmp = it;
				it++;
				possibleNodes.erase(tmp);
			}
			else
				it++;
		}
	}
	
	if(possibleNodes.empty())
		return;	// nowhere to search
		
	OnlineUserPtr ou = possibleNodes.begin()->second;
	triedNodes[possibleNodes.begin()->first] = ou;
	possibleNodes.erase(possibleNodes.begin());
	
	// set possibly dead node timeout
	ou->getIdentity().set("EX", Util::toString(GET_TICK() + NODE_RESPONSE_TIMEOUT));
	
	// search
	sendRequest(ou->getIdentity().getIp(), static_cast<uint16_t>(Util::toInt(ou->getIdentity().getUdpPort())));
}

void SearchManager::findNode(const CID& cid)
{
	// TODO: check that we are not already searching for this node
	
	Search* s = new Search();
	s->type = Search::TYPE_NODE;
	s->term = cid.toBase32();
	
	search(*s);
}

void SearchManager::findFile(const string& tth)
{
	Search* s = new Search();
	s->type = Search::TYPE_FILE;
	s->term = tth;
	
	search(*s);
}

void SearchManager::publishFile(const TTHValue& tth, int64_t size)
{
	Search* s = new Search();
	s->type = Search::TYPE_STOREFILE;
	s->term = tth.toBase32();
	s->fileSize = size;
	
	search(*s);
}

void SearchManager::search(Search& s)
{
	s.token = Util::toString(Util::rand());
	s.startTime = GET_TICK();
	
	// store this search
	{
		Lock l(cs);
		searches[s.token] = &s;
	}
	
	KademliaManager::getInstance()->getRoutingTable().getClosestTo(CID(s.term), 50, s.possibleNodes);
	
	// no node to search
	if(s.possibleNodes.empty())	return;
	
	size_t nodesCount = min((size_t)SEARCH_ALPHA, s.possibleNodes.size());
	NodeMap::iterator it;
	for(size_t i = 0; i < nodesCount; i++)
	{
		it = s.possibleNodes.begin();
		OnlineUserPtr ou = it->second;
		
		// move to tried and delete from possibles
		s.triedNodes[it->first] = ou;
		s.possibleNodes.erase(it);
		
		// set possibly dead node timeout
		ou->getIdentity().set("EX", Util::toString(GET_TICK() + NODE_RESPONSE_TIMEOUT));
		
		s.sendRequest(ou->getIdentity().getIp(), static_cast<uint16_t>(Util::toInt(ou->getIdentity().getUdpPort())));
	}
}

void SearchManager::processSearchRequest(const AdcCommand& cmd)
{
	if(cmd.getParameters().empty())
		return;
	
	string cid = cmd.getParam(0);
	if(cid.size() != 39)
		return;
	
	in_addr addr; addr.s_addr = cmd.getFrom();
	string senderIp = inet_ntoa(addr);
	uint16_t senderPort = static_cast<uint16_t>(cmd.getTo());
	
	string token;
	if(!cmd.getParam("TO", 0, token))
		return;	// missing search token?
	
	string term;
	if(!cmd.getParam("TR", 0, term))
		return;	// nothing to search?
		
	AdcCommand res(AdcCommand::CMD_RES, AdcCommand::TYPE_UDP);
	res.addParam("TR", term);
	res.addParam("TO", token);

	// get sources for this file
	SourceList sources;
	if(IndexManager::getInstance()->findResult(TTHValue(term), sources)) // TODO: check file size
	{
		// add source to res command in XML format
		SimpleXML xml;
		xml.addTag("Nodes");
		xml.stepIn();
		
		for(SourceList::const_iterator i = sources.begin(); i != sources.end(); i++)
		{
			const Identity& id = *i;
			
			xml.addTag("Node");
			xml.addChildAttrib("CID", id.getUser()->getCID().toBase32());
			xml.addChildAttrib("IP", id.getIp());
			xml.addChildAttrib("UDP", id.getUdpPort());
			xml.addChildAttrib("size", id.get("SS"));
		}
		
		xml.stepOut();
		
		string nodes;
		StringOutputStream sos(nodes);
		sos.write(SimpleXML::utf8Header);
		xml.toXML(&sos);
		
		res.addParam("SX", nodes);
		
		// send result
		KademliaManager::getInstance()->send(res, senderIp, senderPort);
	}
}

void SearchManager::processSearchResponse(const AdcCommand& cmd)
{
	string cid = cmd.getParam(0);
	if(cid.size() != 39)
		return;

	// TODO: set node alive
		
	string token;
	if(!cmd.getParam("TO", 0, token))
		return;	// missing search token?
	
	bool sources = false;
	string tth;
	string nodes;
	if(!cmd.getParam("NX", 0, nodes))
	{
		// is this source list?
		if(!cmd.getParam("SX", 0, nodes))
		{
			return; // missing source list?
		}
		
		if(!cmd.getParam("TR", 0, tth))
			return;	// missing tth?
			
		sources = true;
	}
	
	try
	{	
		NodeList results;
		
		SimpleXML xml;
		xml.fromXML(nodes);
		xml.stepIn();
		while(xml.findChild("Node"))
		{
			const CID cid			= CID(xml.getChildAttrib("CID"));
			
			if(ClientManager::getInstance()->getMyCID() == cid)
				return;
				
			const string& ip		= xml.getChildAttrib("IP");
			const string& udpPort	= xml.getChildAttrib("UDP");

			if(Util::isPrivateIp(ip)) continue;

			OnlineUserPtr ou = KademliaManager::getInstance()->getRoutingTable().add(cid);
			ou->getIdentity().setIp(ip);
			ou->getIdentity().setUdpPort(udpPort);
			// TODO: nick
			
			if(sources)
			{
				// TODO: handle partial sources
				const int64_t size		= Util::toInt64(xml.getChildAttrib("size"));
				dcpp::SearchResultPtr sr(new dcpp::SearchResult(ou->getUser(), SearchResult::TYPE_FILE, 0, 0, size, Util::emptyString,
					"Kademlia", ip, TTHValue(tth), token));
				fire(SearchManagerListener::SR(), sr);
			}
			else
			{
				results.push_back(ou);
			}
		}
		xml.stepOut();
		
		if(!sources)
		{
			// process possible nodes
			Lock l(cs);
			SearchMap::const_iterator sm = searches.find(token);
			if(sm != searches.end())
				sm->second->processResponse(CID(cid), results);
		}
	}
	catch(SimpleXMLException&)
	{ /* node send us damaged node list */ }
}

void SearchManager::processSearches()
{
	Lock l(cs);
	bool publishing = false;
	SearchMap::iterator it = searches.begin();
	while(it != searches.end())
	{
		Search* s = it->second;
		
		// TODO: check maximum responses to search request
		uint64_t timeout = 2 * 60 * 1000; // default timeout = 2 minutes
		switch(s->type)
		{
			case Search::TYPE_FILE:
				timeout = SEARCHFILE_LIFETIME;
				break;
			case Search::TYPE_NODE:
				timeout = SEARCHNODE_LIFETIME;
				break;
			case Search::TYPE_STOREFILE:
				timeout = SEARCHSTOREFILE_LIFETIME;
				publishing = true;
				break;
		}
		
		// remove long search, process active search
		if(s->startTime + timeout < GET_TICK())
		{
			// search time out, stop it
			delete s;
			searches.erase(it++);
		}
		else
		{
			// it's time to process this search
			s->process();
			++it;
		}		
	}
	
	IndexManager::getInstance()->setPublishing(publishing);
}

} // namespace kademlia