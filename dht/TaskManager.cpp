#include "StdAfx.h"

#include "BootstrapManager.h"
#include "DHT.h"
#include "IndexManager.h"
#include "SearchManager.h"
#include "TaskManager.h"

#include "../client/SettingsManager.h"
#include "../client/TimerManager.h"

namespace dht
{

	TaskManager::TaskManager(void) :
		nextPublishTime(GET_TICK()), nextSearchTime(GET_TICK())
	{
		TimerManager::getInstance()->addListener(this);
	}

	TaskManager::~TaskManager(void)
	{
		TimerManager::getInstance()->removeListener(this);
	}
	
	// TimerManagerListener
	void TaskManager::on(TimerManagerListener::Second, uint64_t aTick) throw()
	{	
		if(DHT::getInstance()->isConnected())
		{
			if(IndexManager::getInstance()->getPublishing() < MAX_PUBLISHES_AT_TIME)
			{
				// publish next file
				IndexManager::getInstance()->publishNextFile();
			}
		}
		else
		{
			// bootstrap
			BootstrapManager::getInstance()->bootstrap();
		}
		
		if(aTick >= nextSearchTime)
		{
			SearchManager::getInstance()->processSearches();
			nextSearchTime = aTick + SEARCH_PROCESSTIME;
		}		
	}
	
	void TaskManager::on(TimerManagerListener::Minute, uint64_t aTick) throw()
	{
		if((SETTING(INCOMING_CONNECTIONS) != SettingsManager::INCOMING_FIREWALL_PASSIVE) && DHT::getInstance()->isConnected() && aTick >= nextPublishTime)
		{
			// republish all files
			if(IndexManager::getInstance()->publishFiles())
				nextPublishTime = aTick + REPUBLISH_TIME;
			else
				nextPublishTime = aTick + (REPUBLISH_TIME / 4);
		}
		
		// remove dead nodes
		DHT::getInstance()->checkExpiration(aTick);
		IndexManager::getInstance()->checkExpiration(aTick);
	}
	
}