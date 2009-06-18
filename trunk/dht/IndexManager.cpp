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
#include "..\client\LogManager.h"
#include "..\client\ShareManager.h"
#include "..\client\TimerManager.h"

namespace dht
{

	IndexManager::IndexManager(void) :
		publishing(0)
	{
	}

	IndexManager::~IndexManager(void)
	{
	}

	/*
	 * Add new source to tth list 
	 */
	void IndexManager::addIndex(const TTHValue& tth, const Node::Ptr& node, uint64_t size)
	{
		Source source;
		source.setCID(node->getUser()->getCID());
		source.setIp(node->getIdentity().getIp());
		source.setUdpPort(static_cast<uint16_t>(Util::toInt(node->getIdentity().getUdpPort())));
		source.setSize(size);
		source.setExpires(GET_TICK() + REPUBLISH_TIME);
			
		Lock l(cs);
		
		TTHMap::iterator i = tthList.find(tth);
		if(i != tthList.end())
		{
			// no user duplicites
			SourceList& sources = i->second;
			for(SourceList::iterator s = sources.begin(); s != sources.end(); s++)
			{
				if(node->getUser()->getCID() == (*s).getCID())
				{
					// delete old item
					sources.erase(s);
					break;
				}
			}
			
			// old items in front, new items in back
			sources.push_back(source);
			
			// if maximum sources reached, remove the oldest one
			if(sources.size() > MAX_SEARCH_RESULTS)
				sources.pop_front();
		}
		else
		{
			// new file		
			tthList.insert(std::make_pair(tth, SourceList(1, source)));
		}	
	}

	/*
	 * Finds TTH in known indexes and returns it 
	 */
	bool IndexManager::findResult(const TTHValue& tth, SourceList& sources) const
	{
		// TODO: remove old indexes
		// TODO: does file exist in my own sharelist?
		Lock l(cs);
		TTHMap::const_iterator i = tthList.find(tth);
		if(i != tthList.end())
		{
			sources = i->second;
			return true;
		}
			
		return false;
	}

	/*
	 * Try to publish next file in queue 
	 */
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

	/*
	 * Create publish queue from local file list 
	 */
	void IndexManager::createPublishQueue(ShareManager::HashFileMap& tthIndex)
	{
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
		
		LogManager::getInstance()->message("DHT: Publishing " + Util::toString(publishQueue.size()) + " files...");
	}

	/*
	 * Loads existing indexes from disk 
	 */
	void IndexManager::loadIndexes(SimpleXML& xml)
	{
		if(xml.findChild("Indexes"))
		{
			xml.stepIn();
			while(xml.findChild("Index"))
			{
				const TTHValue tth = TTHValue(xml.getChildAttrib("TTH"));
				SourceList sources;
				
				xml.stepIn();
				while(xml.findChild("Source"))
				{
					Source source;
					source.setCID(CID(xml.getChildAttrib("CID")));
					source.setIp(xml.getChildAttrib("I4"));
					source.setUdpPort(static_cast<uint16_t>(Util::toInt(xml.getChildAttrib("U4"))));
					source.setSize(Util::toInt64(xml.getChildAttrib("SI")));
					source.setExpires(Util::toInt64(xml.getChildAttrib("expires")));
					
					sources.push_back(source);
				}
				
				tthList.insert(std::make_pair(tth, sources));
				xml.stepOut();
			}
			xml.stepOut();
		}
	}

	/*
	 * Save all indexes to disk 
	 */
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
				const Source& source = *j;
				
				xml.addTag("Source");
				xml.addChildAttrib("CID", source.getCID().toBase32());
				xml.addChildAttrib("I4", source.getIp());
				xml.addChildAttrib("U4", source.getUdpPort());
				xml.addChildAttrib("SI", source.getSize());
				xml.addChildAttrib("expires", source.getExpires());
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
		string tth;
		if(!cmd.getParam("TR", 0, tth))
			return;	// nothing to identify a file?
			
		string size;
		if(!cmd.getParam("SI", 0, size))
			return;	// no file size?
	
		addIndex(TTHValue(tth), node, Util::toInt64(size));
		
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
	
	/*
	 * Removes old sources 
	 */
	void IndexManager::checkExpiration(uint64_t aTick)
	{
		Lock l(cs);

		TTHMap::iterator i = tthList.begin();
		while(i != tthList.end())
		{
			SourceList::iterator j = i->second.begin();
			while(j != i->second.end())
			{
				const Source& source = *j;
				if(source.getExpires() <= aTick)
					j = i->second.erase(j);
				else
					break;	// list is sorted, so finding first non-expired can stop iteration
				
			}
			
			if(i->second.empty())
				tthList.erase(i++);
			else
				++i;
		}		
	}
	

} // namespace dht