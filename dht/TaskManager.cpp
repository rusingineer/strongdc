#include "StdAfx.h"

#include "BootstrapManager.h"
#include "DHT.h"
#include "IndexManager.h"
#include "SearchManager.h"
#include "TaskManager.h"

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
		// bootstrap
		BootstrapManager::getInstance()->bootstrap();
		
		if(IndexManager::getInstance()->getPublishing() < MAX_PUBLISHES_AT_TIME)
		{
			// publish next file
			IndexManager::getInstance()->publishNextFile();
		}
				
		if(aTick >= nextSearchTime)
		{
			SearchManager::getInstance()->processSearches();
			nextSearchTime = aTick + SEARCH_PROCESSTIME;
		}		
	}
	
	void TaskManager::on(TimerManagerListener::Minute, uint64_t aTick) throw()
	{
		if(aTick >= nextPublishTime)
		{
			// republish all files
			if(IndexManager::getInstance()->publishFiles())
				nextPublishTime = aTick + REPUBLISH_TIME;
			else
				nextPublishTime = aTick + (REPUBLISH_TIME / 4);
		}
		
		// remove dead nodes
		DHT::getInstance()->checkExpiration(aTick);
	}
	
}