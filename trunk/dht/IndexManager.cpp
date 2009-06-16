/*
 * Copyright (C) 2008-2009 Big Muscle, http://strongdc.sf.net
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
#include "IndexManager.h"
#include "SearchManager.h"

#include "..\client\CID.h"
#include "..\client\ShareManager.h"
#include "..\client\TimerManager.h"

namespace dht
{

	IndexManager::IndexManager(void) :
		publishing(false)
	{
	}

	IndexManager::~IndexManager(void)
	{
	}

	/*
	 * Add new source to tth list 
	 */
	void IndexManager::addIndex(const TTHValue& tth, const Identity& source)
	{
		Lock l(cs);
		TTHMap::iterator i = tthList.find(tth);
		if(i != tthList.end())
		{
			// no user duplicites
			SourceList& sources = i->second;
			for(SourceList::const_iterator s = sources.begin(); s != sources.end(); s++)
			{
				if(source.getUser() == (*s).getUser())
					return;	// TODO: increase lifetime
			}
			
			sources.push_back(source);
			
			if(sources.size() > MAX_SEARCH_RESULTS)
				sources.pop_front();
		}
		else
		{
			// new file
			tthList.insert(std::make_pair(tth, SourceList(1, source)));
			
		}	
	}

	bool IndexManager::findResult(const TTHValue& tth, SourceList& sources) const
	{
		// TODO: remove old indexes
		Lock l(cs);
		TTHMap::const_iterator i = tthList.find(tth);
		if(i != tthList.end())
		{
			sources = i->second;
			return true;
		}
			
		return false;
	}

	void IndexManager::publishNextFile()
	{
		File f;
		{
			Lock l(cs);
						
			if(publishQueue.empty())
				return;

			publishing++;				
				
			f = publishQueue.front(); // get the first file in queue
			publishQueue.pop_front(); // and remove it from queue
		}
		SearchManager::getInstance()->findStore(f.tth.toBase32(), f.size);
	}

	void IndexManager::createPublishQueue(ShareManager::HashFileMap& tthIndex)
	{
	#ifdef _DEBUG
		dcdebug("File list size: %d\n", tthIndex.size());
		uint64_t startTime = GET_TICK();
	#endif
		
		// copy to map to sort by size
		std::map<int64_t, const TTHValue*> sizeMap;
		for(ShareManager::HashFileIter i = tthIndex.begin(); i != tthIndex.end(); i++)
		{
			if(i->second->getSize() > MIN_PUBLISH_FILESIZE)
				sizeMap[i->second->getSize()] = &i->first;
		}
		
		Lock l(cs);
		
		// select the first MAX_PUBLISHED_FILES largest files
		size_t count = 0;
		for(std::map<int64_t, const TTHValue*>::reverse_iterator i = sizeMap.rbegin(); i != sizeMap.rend() && count < MAX_PUBLISHED_FILES; i++, count++)
		{	
			publishQueue.push_back(File(*i->second, i->first));
		}
		
		// shuffle
		random_shuffle(publishQueue.begin(), publishQueue.end());
		
	#ifdef _DEBUG	
		startTime = GET_TICK() - startTime;	
		dcdebug("Create publishing queue took %I64d ms, stored %d files\n", startTime, publishQueue.size());
	#endif
	}

	void IndexManager::loadIndexes(SimpleXML& xml)
	{
		if(xml.findChild("Indexes"))
		{
			xml.stepIn();
			while(xml.findChild("Index"))
			{
				const TTHValue tth = TTHValue(xml.getChildAttrib("TTH"));
				
				xml.stepIn();
				while(xml.findChild("Source"))
				{
					const CID cid = CID(xml.getChildAttrib("CID"));
					
					Identity identity(ClientManager::getInstance()->getUser(cid), 0);
					identity.getUser()->setFlag(User::DHT);
					identity.setIp(xml.getChildAttrib("I4"));
					identity.setUdpPort(xml.getChildAttrib("U4"));
					identity.set("SI", xml.getChildAttrib("SI"));
					
					addIndex(tth, identity);
				}
				
				xml.stepOut();
			}
			xml.stepOut();
		}
	}

	void IndexManager::saveIndexes(SimpleXML& xml)
	{
		xml.addTag("Indexes");
		xml.stepIn();

		Lock l(cs);
		for(TTHMap::const_iterator i = tthList.begin(); i != tthList.end(); i++)
		{
			xml.addTag("Index");
			xml.addChildAttrib("TTH", i->first.toBase32());
			
			xml.stepIn();
			for(SourceList::const_iterator j = i->second.begin(); j != i->second.end(); j++)
			{
				const Identity& id = *j;
				
				xml.addTag("Source");
				xml.addChildAttrib("CID", id.getUser()->getCID().toBase32());
				xml.addChildAttrib("I4", id.getIp());
				xml.addChildAttrib("U4", id.getUdpPort());
				xml.addChildAttrib("SI", id.get("SI"));		
			}
			xml.stepOut();
		}
		
		xml.stepOut();
	}
	
	/*
	 * Processes incoming request to publish file 
	 */
	void IndexManager::processPublishRequest(const Node::Ptr& node, const AdcCommand& cmd)
	{
		string cid = cmd.getParam(0);
		if(cid.size() != 39)
			return;
			
		string tth;
		if(!cmd.getParam("TR", 0, tth))
			return;	// nothing to identify a file?
			
		string size;
		if(!cmd.getParam("SI", 0, size))
			return;	// no file size?
			
		Identity identity(ClientManager::getInstance()->getUser(CID(cid)), 0);
		identity.getUser()->setFlag(User::DHT);
		identity.setIp(node->getIdentity().getIp());
		identity.setUdpPort(node->getIdentity().getUdpPort());
		identity.set("SI", size);
						
		addIndex(TTHValue(tth), identity);
		
		// send response
		DHT::getInstance()->send(AdcCommand(AdcCommand::SEV_SUCCESS, AdcCommand::ERROR_GENERIC, "File published: " + tth, AdcCommand::TYPE_UDP), 
											node->getIdentity().getIp(), static_cast<uint16_t>(Util::toInt(node->getIdentity().getUdpPort())));	
	}

	/*
	 * Publishes all files in queue 
	 */
	bool IndexManager::publishFiles()
	{
		{
			Lock l(cs);
			if(!publishQueue.empty())
				return false;	// don't republish if previous publishing hasn't been finished yet
		}
				
		ShareManager::getInstance()->publish();
		return true;
	}

} // namespace dht