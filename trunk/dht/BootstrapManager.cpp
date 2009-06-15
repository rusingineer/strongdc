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
#include "BootstrapManager.h"

#include "Constants.h"
#include "DHT.h"
#include "SearchManager.h"

#include "../client/ClientManager.h"
#include "../client/HttpConnection.h"
#include "../client/LogManager.h"

namespace dht
{

	BootstrapManager::BootstrapManager(void)
	{
		httpConnection.addListener(this);
	}

	BootstrapManager::~BootstrapManager(void)
	{
		httpConnection.removeListener(this);
	}

	void BootstrapManager::start()
	{
		dcdebug("MyCID: %s\n", ClientManager::getInstance()->getMe()->getCID().toBase32().c_str());
		// TODO: make URL settable
		// TODO: add supported features
		string url = BOOTSTRAP_URL "?cid=" + ClientManager::getInstance()->getMe()->getCID().toBase32() +
			"&u4=" + Util::toString(DHT_UDPPORT);
		
		httpConnection.setCoralizeState(HttpConnection::CST_NOCORALIZE);
		httpConnection.downloadFile(url);		
	}
	
	void BootstrapManager::on(HttpConnectionListener::Data, HttpConnection*, const uint8_t* buf, size_t len) throw()
	{
		nodesXML += string((const char*)buf, len);
	}

	#define BUFSIZE 16384
	void BootstrapManager::on(HttpConnectionListener::Complete, HttpConnection*, string const&) throw()
	{
		if(!nodesXML.empty())
		{
			try
			{
				uLongf destLen = BUFSIZE;
				boost::scoped_array<uint8_t> destBuf;
				
				// decompress incoming packet
				int result;
				
				do
				{
					destLen *= 2;
					destBuf.reset(new uint8_t[destLen]);
					
					result = uncompress(&destBuf[0], &destLen, (Bytef*)nodesXML.data(), nodesXML.length());
				} while (result == Z_BUF_ERROR);
				
				if(result != Z_OK)
				{
					// decompression error!!!
					throw Exception("Decompress error.");
				}
							
				SimpleXML remoteXml;
				remoteXml.fromXML(string((char*)&destBuf[0], destLen));
				remoteXml.stepIn();
					
				while(remoteXml.findChild("Node"))
				{
					CID cid = CID(remoteXml.getChildAttrib("CID"));
					string i4 = remoteXml.getChildAttrib("I4");
					string u4 = remoteXml.getChildAttrib("U4");
					
					Identity id;
					id.setIp(i4);
					id.setUdpPort(u4);
					
					nodes.push_back(std::make_pair(cid, id));
				}

				remoteXml.stepOut();
			}
			catch(Exception& e)
			{
				LogManager::getInstance()->message("DHT bootstrap error: " + e.getError());
			}
		}
	}

	void BootstrapManager::on(HttpConnectionListener::Failed, HttpConnection*, const string& aLine) throw()
	{
		LogManager::getInstance()->message("DHT bootstrap error: " + aLine);
	}
	
	void BootstrapManager::bootstrap()
	{
		if(!nodes.empty() && DHT::getInstance()->getNodesCount() == 0)
		{
			//LogManager::getInstance()->message("DHT bootstrapping started");
			
			// it's time to bootstrap
			NodeList bootstrapNodes(SEARCH_ALPHA > nodes.size() ? nodes.size() : SEARCH_ALPHA);
			
			// insert ALPHA random nodes to routing table
			std::random_sample_n(nodes.begin(), nodes.end(), bootstrapNodes.begin(), SEARCH_ALPHA);
			
			for(NodeList::const_iterator i = bootstrapNodes.begin(); i != bootstrapNodes.end(); i++)
			{
				CID cid		= i->first;
				string i4	= i->second.getIp();
				uint16_t u4	= static_cast<uint16_t>(Util::toInt(i->second.getUdpPort()));
				
				DHT::getInstance()->addUser(cid, i4, u4);
			}
			
			// find myself in the network
			SearchManager::getInstance()->findNode(ClientManager::getInstance()->getMe()->getCID());			
		}
	}
	
}