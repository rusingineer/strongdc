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
#include "SearchManager.h"

#include <ShareManager.h>

namespace kademlia
{

IndexManager::IndexManager(void) : lastPublishTime(0)
{
}

IndexManager::~IndexManager(void)
{
}

void IndexManager::addIndex(const TTHValue& tth, const Identity& source)
{
	Lock l(cs);
	TTHMap::iterator i = tthList.find(tth);
	if(i != tthList.end())
	{
		// TODO: no user duplicites, just update
		SourceList& sources = i->second;
		sources.push_back(source);
		
		if(sources.size() > MAX_FILESOURCES)
			sources.pop_front();
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
	// TODO: publish only 1 file at time
	File f;
	{
		Lock l(cs);
		
		if(publishQueue.empty())
			return;
			
		f = publishQueue.front(); // get the first file in queue
		publishQueue.pop_front(); // and remove it from queue
	}
	SearchManager::getInstance()->publishFile(f.tth, f.size);
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
	lastPublishTime = GET_TICK();
	
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
				identity.getUser()->setFlag(User::KADEMLIA);
				identity.setIp(xml.getChildAttrib("IP"));
				identity.setUdpPort(xml.getChildAttrib("UDP"));
				identity.set("SS", xml.getChildAttrib("size"));
				
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
			xml.addChildAttrib("IP", id.getIp());
			xml.addChildAttrib("UDP", id.getUdpPort());
			xml.addChildAttrib("size", id.get("SS"));		
		}
		xml.stepOut();
	}
	
	xml.stepOut();
}

} // namespace kademlia