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

#include "DHT.h"
#include "BootstrapManager.h"
#include "ConnectionManager.h"
#include "IndexManager.h"
#include "SearchManager.h"
#include "TaskManager.h"
#include "Utils.h"

#include "../client/AdcCommand.h"
#include "../client/CID.h"
#include "../client/ClientManager.h"
#include "../client/CryptoManager.h"
#include "../client/LogManager.h"
#include "../client/SettingsManager.h"
#include "../client/User.h"
#include "../client/Version.h"

namespace dht
{

	DHT::DHT(void) : nodesCount(0), lastPacket(0), dirty(false)
	{
		BootstrapManager::newInstance();
		SearchManager::newInstance();
		IndexManager::newInstance();
		TaskManager::newInstance();
		ConnectionManager::newInstance();
		
		bucket = new KBucket();
		
		loadData();
	}

	DHT::~DHT(void)
	{
		disconnect();
		
		dirty = true;
		saveData();
		
		delete bucket;
		
		ConnectionManager::deleteInstance();		
		TaskManager::deleteInstance();
		IndexManager::deleteInstance();
		SearchManager::deleteInstance();
		BootstrapManager::deleteInstance();
	}
	
	/*
	 * Process incoming command 
	 */
	void DHT::dispatch(const string& aLine, const string& ip, uint16_t port)
	{
		lastPacket = GET_TICK();
		
		// check node's IP address
		if(Util::isPrivateIp(ip))
		{
			socket.send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_BAD_IP, "Your client supplied local IP: " + ip, AdcCommand::TYPE_UDP), ip, port);
			return; // local ip supplied
		}

		try
		{
			AdcCommand cmd(aLine);
			
			// ignore empty commands
			if(cmd.getParameters().empty())
				return;
	
			string cid = cmd.getParam(0);
			if(cid.size() != 39)
				return;
	
			// ignore message from myself
			if(CID(cid) == ClientManager::getInstance()->getMe()->getCID())
				return;
				
			// add user to routing table
			Node::Ptr node = addUser(CID(cid), ip, port);
			
			// bootstrap when we insert the first node into table
			if(nodesCount == 1)
				SearchManager::getInstance()->findNode(ClientManager::getInstance()->getMe()->getCID());
			
#define C(n) case AdcCommand::CMD_##n: handle(AdcCommand::n(), node, cmd); break;
			switch(cmd.getCommand()) 
			{
				C(INF);	// user's info
				C(SCH);	// search request
				C(RES);	// response to SCH
				C(PUB);	// request to publish file
				C(CTM); // connection request
				C(STA);	// status message
				
			default: 
				dcdebug("Unknown ADC command: %.50s\n", aLine.c_str());
				break;
#undef C
	
			}			
		}
		catch(const ParseException&)
		{
			dcdebug("Invalid ADC command: %.50s\n", aLine.c_str());
		}
	}
	
	/*
	 * Sends command to ip and port 
	 */
	void DHT::send(const AdcCommand& cmd, const string& ip, uint16_t port)
	{
		socket.send(cmd, ip, port);
	}
	
	/*
	 * Insert (or update) user into routing table 
	 */
	Node::Ptr DHT::addUser(const CID& cid, const string& ip, uint16_t port)
	{
		Lock l(cs);
		
		// create user as offline (only TCP connected users will be online)
		UserPtr u = ClientManager::getInstance()->getUser(cid);
		u->setFlag(User::DHT);
		
		Node::Ptr node = bucket->insert(u);
		node->getIdentity().setIp(ip);
		node->getIdentity().setUdpPort(Util::toString(port));
		
		nodesCount++;
		
		return node;
	}
	
	/*
	 * Finds "max" closest nodes and stores them to the list 
	 */
	void DHT::getClosestNodes(const CID& cid, std::map<CID, Node::Ptr>& closest, unsigned int max)
	{
		Lock l(cs);
		bucket->getClosestNodes(cid, closest, max);
	}

	/*
	 * Removes dead nodes 
	 */
	void DHT::checkExpiration(uint64_t aTick)
	{
		Lock l(cs);
		nodesCount = bucket->checkExpiration(aTick);
	}
	
	/*
	 * Finds the file in the network 
	 */
	void DHT::findFile(const string& tth, const string& token)
	{
		if(isConnected())
			SearchManager::getInstance()->findFile(tth, token);
	}
	
	/** Sends our info to specified ip:port */
	void DHT::info(const string& ip, uint16_t port, bool wantResponse)
	{
		// TODO: what info is needed?
		AdcCommand cmd(AdcCommand::CMD_INF, AdcCommand::TYPE_UDP);

#ifdef SVNVERSION
#define VER VERSIONSTRING SVNVERSION
#else
#define VER VERSIONSTRING
#endif		
		
		cmd.addParam("VE", ("StrgDC++ " VER));
		cmd.addParam("NI", SETTING(NICK));
		
		if(wantResponse)
			cmd.addParam("RE", "1");
		
		string su;
		if(CryptoManager::getInstance()->TLSOk())
		{
			su += ADCS_FEATURE ",";
		}

		if(SETTING(INCOMING_CONNECTIONS) != SettingsManager::INCOMING_FIREWALL_PASSIVE)
		{
			su += TCP4_FEATURE ",";
			su += UDP4_FEATURE ",";
		}
		
		if(!su.empty()) {
			su.erase(su.size() - 1);
		}
		cmd.addParam("SU", su);
			
		send(cmd, ip, port);		
	}
	
	/*
	 * Sends Connect To Me request to online node 
	 */
	void DHT::connect(const OnlineUser& ou, const string& token)
	{
		ConnectionManager::getInstance()->connect(ou, token);
	}
	
	/*
	 * Sends private message to online node 
	 */
	void DHT::privateMessage(const OnlineUser& ou, const string& aMessage, bool thirdPerson)
	{
	}	
	
	/*
	 * Loads network information from XML file 
	 */
	void DHT::loadData()
	{
		try
		{
			SimpleXML xml;
			xml.fromXML(dcpp::File(Util::getConfigPath() + DHT_FILE, dcpp::File::READ, dcpp::File::OPEN).read());
			
			xml.stepIn();
			
			// load nodes
			if(xml.findChild("Nodes"))
			{
				xml.stepIn();
				while(xml.findChild("Node"))
				{
					CID cid		= CID(xml.getChildAttrib("CID"));
					string i4	= xml.getChildAttrib("I4");
					uint16_t u4	= static_cast<uint16_t>(Util::toInt(xml.getChildAttrib("U4")));;
					
					addUser(cid, i4, u4);
				}
				xml.stepOut();
			}
			
			// load indexes
			IndexManager::getInstance()->loadIndexes(xml);
			xml.stepOut();
		}
		catch(Exception& e)
		{
			dcdebug("%s\n", e.getError().c_str());
		}
	}

	/*
	 * Finds "max" closest nodes and stores them to the list 
	 */
	void DHT::saveData()
	{
		if(!dirty)
			return;
			
		Lock l(cs);
		
		SimpleXML xml;
		xml.addTag("DHT");
		xml.stepIn();
		
		// save nodes
		xml.addTag("Nodes");
		xml.stepIn();

		const KBucket::NodeList& nodes = bucket->getNodes();
		for(KBucket::NodeList::const_iterator j = nodes.begin(); j != nodes.end(); j++)
		{
			const Node::Ptr& node = *j;
					
			if(node->getType() < 2) // save only active nodes
			{
				xml.addTag("Node");
				xml.addChildAttrib("CID", node->getUser()->getCID().toBase32());
				xml.addChildAttrib("I4", node->getIdentity().getIp());
				xml.addChildAttrib("U4", node->getIdentity().getUdpPort());
			}
		}
		
		xml.stepOut();		
		
		// save foreign published files
		IndexManager::getInstance()->saveIndexes(xml);
		
		xml.stepOut();
		
		try
		{
			dcpp::File file(Util::getConfigPath() + DHT_FILE + ".tmp", dcpp::File::WRITE, dcpp::File::CREATE | dcpp::File::TRUNCATE);
			BufferedOutputStream<false> bos(&file);
			bos.write(SimpleXML::utf8Header);
			xml.toXML(&bos);
			bos.flush();
			file.close();
			dcpp::File::deleteFile(Util::getConfigPath() + DHT_FILE);
			dcpp::File::renameFile(Util::getConfigPath() + DHT_FILE + ".tmp", Util::getConfigPath() + DHT_FILE);
		}
		catch(const FileException&)
		{
		}
	}	

	/*
	 * Message processing
	 */

	void DHT::handle(AdcCommand::INF, const Node::Ptr& node, AdcCommand& c) throw()
	{
		string wantResponse;
		c.getParam("RE", 0, wantResponse);
		
		// user send us his info, make him online	
		for(StringIterC i = c.getParameters().begin(); i != c.getParameters().end(); ++i) {
			if(i->length() < 2)
				continue;

			if(i->substr(0, 2) != "RE")
				node->getIdentity().set(i->c_str(), i->substr(2));
		}
		
		if(node->getIdentity().supports(ADCS_FEATURE))
		{
			node->getUser()->setFlag(User::TLS);
		}		
		
		if(!node->isInList)
		{
			node->isInList = true;
			ClientManager::getInstance()->putOnline(node.get());
		}
		
		if(wantResponse == "1")
			info(node->getIdentity().getIp(), static_cast<uint16_t>(Util::toInt(node->getIdentity().getUdpPort())), false);
	}
	
	void DHT::handle(AdcCommand::SCH, const Node::Ptr& node, AdcCommand& c) throw()
	{
		SearchManager::getInstance()->processSearchRequest(node, c);
	}
	
	void DHT::handle(AdcCommand::RES, const Node::Ptr& node, AdcCommand& c) throw()
	{
		SearchManager::getInstance()->processSearchResult(node, c);
	}
	
	void DHT::handle(AdcCommand::PUB, const Node::Ptr& node, AdcCommand& c) throw()
	{
		IndexManager::getInstance()->processPublishRequest(node, c);
	}
	
	void DHT::handle(AdcCommand::CTM, const Node::Ptr& node, AdcCommand& c) throw()
	{
		ConnectionManager::getInstance()->connectToMe(node, c);
	}
	
	void DHT::handle(AdcCommand::STA, const Node::Ptr& node, AdcCommand& c) throw()
	{
		if(c.getParameters().size() < 2)
			return;
			
		// we should disconnect from network when severity is fatal, but it could be possible exploit
		LogManager::getInstance()->message("DHT (" + node->getIdentity().getIp() + "): " + c.getParam(2)); // TODO: translate	
	}
	
}