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
#include "SearchManager.h"

#include "Constants.h"
#include "DHT.h"
#include "IndexManager.h"
#include "Utils.h"

#include "../client/ClientManager.h"
#include "../client/SearchResult.h"
#include "../client/SimpleXml.h"

namespace dht
{

	/*
	 * Process this search request 
	 */
	bool Search::process()
	{
		// no node to search
		if(possibleNodes.empty() || respondedNodes.size() >= MAX_SEARCH_RESULTS)
			return false;
			
		// send search request to the first ALPHA closest nodes
		size_t nodesCount = min((size_t)SEARCH_ALPHA, possibleNodes.size());
		Search::NodeMap::iterator it;
		for(size_t i = 0; i < nodesCount; i++)
		{
			it = possibleNodes.begin();
			Node::Ptr id = it->second;
			
			// move to tried and delete from possibles
			triedNodes[it->first] = id;
			possibleNodes.erase(it);

			// send SCH command
			AdcCommand cmd(AdcCommand::CMD_SCH, AdcCommand::TYPE_UDP);
			cmd.addParam("TR", term);
			cmd.addParam("TY", Util::toString(type));
			cmd.addParam("TO", token);
			
			id->setExpires(GET_TICK() + NODE_RESPONSE_TIMEOUT);
			DHT::getInstance()->send(cmd, id->getIp(), static_cast<uint16_t>(Util::toInt(id->getUdpPort())));
		}
		
		return true;
	}
		
	SearchManager::SearchManager(void)
	{
	}

	SearchManager::~SearchManager(void)
	{
	}
	
	/*
	 * Performs node lookup in the network 
	 */
	void SearchManager::findNode(const CID& cid)
	{
		Search* s = new Search();
		s->type = Search::TYPE_NODE;
		s->term = cid.toBase32();
		
		search(*s);
	}
	
	/*
	 * Performs value lookup in the network 
	 */
	void SearchManager::findFile(const string& tth)
	{
		Search* s = new Search();
		s->type = Search::TYPE_FILE;
		s->term = tth;
		
		search(*s);	
	}
	
	/*
	 * Performs node lookup to store key/value pair in the network 
	 */
	void SearchManager::findStore(const string& tth, int64_t size)
	{
		Search* s = new Search();
		s->type = Search::TYPE_STOREFILE;
		s->term = tth;
		s->filesize = size;
		
		search(*s);		
	}
			
	/*
	 * Performs general search operation in the network 
	 */
	void SearchManager::search(Search& s)
	{
		// get nodes closest to requested ID
		DHT::getInstance()->getClosestNodes(CID(s.term), s.possibleNodes, 50);

		if(s.process())
		{
			Lock l(cs);
			// store search
			searches[s.token] = &s;
		}
	}
	
	/*
	 * Process incoming search request 
	 */
	void SearchManager::processSearchRequest(const Node::Ptr& user, const AdcCommand& cmd)
	{
		string token;
		if(!cmd.getParam("TO", 0, token))
			return;	// missing search token?
		
		string term;
		if(!cmd.getParam("TR", 0, term))
			return;	// nothing to search?
			
		string type;
		if(!cmd.getParam("TY", 0, type))
			return;	// type not specified?			
			
		AdcCommand res(AdcCommand::CMD_RES, AdcCommand::TYPE_UDP);
		res.addParam("TO", token);
		
		Search::NodeMap nodes;
		switch(Util::toInt(type))
		{
			case Search::TYPE_FILE:
				// TODO: check file hash in our database
				// if it's there, then select sources else do the same as NODE search
			
			case Search::TYPE_NODE:
			case Search::TYPE_STOREFILE:
				// get nodes closest to requested ID
				DHT::getInstance()->getClosestNodes(CID(term), nodes, SEARCH_ALPHA);
				break;
			
			default:
				// invalid search type
				return;
		}
		
		if(!nodes.empty())
		{
			// add nodelist in XML format
			SimpleXML xml;
			xml.addTag("Nodes");
			xml.stepIn();
			
			for(Search::NodeMap::const_iterator i = nodes.begin(); i != nodes.end(); i++)
			{
				xml.addTag("Node");
				xml.addChildAttrib("CID", i->first.toBase32());
				xml.addChildAttrib("I4", i->second->getIp());
				xml.addChildAttrib("U4", i->second->getUdpPort());			
			}
			
			xml.stepOut();
			
			string nodes;
			StringOutputStream sos(nodes);
			sos.write(SimpleXML::utf8Header);
			xml.toXML(&sos);
			
			res.addParam("NX", nodes);
				
			// send search result
			DHT::getInstance()->send(res, user->getIp(), static_cast<uint16_t>(Util::toInt(user->getUdpPort())));
		}
	}
	
	/*
	 * Process incoming search result 
	 */
	void SearchManager::processSearchResult(const Node::Ptr& user, const AdcCommand& cmd)
	{
		string cid = cmd.getParam(0);
		if(cid.size() != 39)
			return;
			
		string token;
		if(!cmd.getParam("TO", 0, token))
			return;	// missing search token?	
		
		string nodes;
		if(!cmd.getParam("NX", 0, nodes))
			return;	// missing search token?	
				
		Lock l(cs);
		SearchMap::iterator i = searches.find(token);
		if(i == searches.end())
		{
			// we didn't search for this
			return;
		}
		
		Search* s = i->second;
		
		// store this node
		s->respondedNodes.insert(std::make_pair(Utils::getDistance(CID(cid), CID(s->term)), user));
		
		try
		{
			SimpleXML xml;
			xml.fromXML(nodes);
			xml.stepIn();
			while(xml.findChild("Node"))
			{
				const CID cid = CID(xml.getChildAttrib("CID"));
				
				CID distance = Utils::getDistance(cid, CID(s->term));
				
				// don't bother with myself and nodes we've already tried or queued
				if(	ClientManager::getInstance()->getMe()->getCID() == cid || 
					s->possibleNodes.find(distance) != s->possibleNodes.end() ||
					s->triedNodes.find(distance) != s->triedNodes.end())
				{
					continue;
				}
					
				const string& i4 = xml.getChildAttrib("I4");
				const string& u4 = xml.getChildAttrib("U4");
				const string& si = xml.getChildAttrib("SI");

				// don't bother with private IPs
				if(Util::isPrivateIp(i4))
					continue;

				Node::Ptr node = new Node();
				node->setIp(i4);
				node->setUdpPort(u4);

				if(si.empty())
				{
					// update our list of possible nodes
					s->possibleNodes[distance] = node;
				}
				else if(s->type == Search::TYPE_FILE)
				{
					// this is response to TYPE_FILE
					
					// create user as offline (only TCP connected users will be online)
					UserPtr u = ClientManager::getInstance()->getUser(cid);
					u->setFlag(User::DHT);
					
					// TODO: ping node
					
					SearchResultPtr sr(new SearchResult(u, SearchResult::TYPE_FILE, 0, 0, Util::toInt64(si), Util::emptyString, "DHT", Util::emptyString, user->getIp(), TTHValue(s->term), token));
					fire(SearchManagerListener::SR(), sr);
				}
			}
			xml.stepOut();
		}
		catch(const SimpleXMLException&)
		{
			// malformed node list
		}
	}
	
	/*
	 * Sends publishing request 
	 */
	void SearchManager::publishFile(Search::NodeMap nodes, const string& tth, int64_t size)
	{
		// send PUB command to K nodes
		int k = 0;
		for(Search::NodeMap::const_iterator i = nodes.begin(); i != nodes.end(); i++)
		{
			AdcCommand cmd(AdcCommand::CMD_PUB, AdcCommand::TYPE_UDP);
			cmd.addParam("TR", tth);
			cmd.addParam("SI", Util::toString(size));
		
			i->second->setExpires(GET_TICK() + NODE_RESPONSE_TIMEOUT);
			DHT::getInstance()->send(cmd, i->second->getIp(), static_cast<uint16_t>(Util::toInt(i->second->getUdpPort())));
			
			if(k++ == K)
				break;
		}
	}
	
	/*
	 * Processes all running searches and removes long-time ones 
	 */
	void SearchManager::processSearches()
	{
		Lock l(cs);
		
		bool publishing = false;
		SearchMap::iterator it = searches.begin();
		while(it != searches.end())
		{
			Search* s = it->second;
			
			// TODO: check maximum responses to search request
			uint64_t timeout = SEARCHNODE_LIFETIME; // default timeout = 2 minutes
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
			if(s->startTime + timeout < GET_TICK() || !s->process())
			{
				// search time out, stop it
				searches.erase(it++);
					
				if(s->type == Search::TYPE_STOREFILE)
				{
					publishFile(s->respondedNodes, s->term, s->filesize);
				}

				delete s;
			}
			else
			{
				++it;
			}		
		}
		
		IndexManager::getInstance()->setPublishing(publishing);
	}
	
}