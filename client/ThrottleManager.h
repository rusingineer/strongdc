/* 
 * Copyright (C) 2009 Big Muscle
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

#ifndef _THROTTLEMANAGER_H
#define _THROTTLEMANAGER_H

#include "DownloadManager.h"
#include "Singleton.h"
#include "Socket.h"
#include "TimerManager.h"
#include "UploadManager.h"

namespace dcpp
{

	#define POLL_TIMEOUT 250
	
	class ThrottleManager :
		public Singleton<ThrottleManager>, private TimerManagerListener
	{
	public:

		/*
		 * Limits a traffic and reads a packet from the network
		 */
		int read(Socket* sock, void* buffer, size_t len)
		{
			size_t downs = DownloadManager::getInstance()->getDownloadCount();
			if(!BOOLSETTING(THROTTLE_ENABLE) || downs == 0)
				return sock->read(buffer, len);
			
			{
				Lock l(downCS);
				
				if(downTokens > 0)
				{
					size_t slice = (SETTING(MAX_DOWNLOAD_SPEED_LIMIT) * 1024) / downs;
					size_t readSize = min(slice, min(len, static_cast<size_t>(downTokens)));
					
					// read from socket
					readSize = sock->read(buffer, readSize);
					
					if(readSize > 0)
						downTokens -= readSize;
				
					return readSize;
				}
			}
			
			WaitForSingleObject(hEvent, POLL_TIMEOUT);
			return -1;
		}
		
		/*
		 * Limits a traffic and writes a packet to the network
		 * We must handle this a little bit differently than downloads, because that stupidity in OpenSSL
		 */		
		int write(Socket* sock, void* buffer, size_t& len)
		{
			size_t ups = UploadManager::getInstance()->getUploadCount();
			if(!BOOLSETTING(THROTTLE_ENABLE) || ups == 0)
				return sock->write(buffer, len);
			
			{
				Lock l(upCS);
				
				if(upTokens > 0)
				{
					size_t slice = (SETTING(MAX_UPLOAD_SPEED_LIMIT) * 1024) / ups;
					len = min(slice, min(len, static_cast<size_t>(upTokens)));
					upTokens -= len;
					
					// write to socket
					return sock->write(buffer, len);
				}
			}
			
			WaitForSingleObject(hEvent, POLL_TIMEOUT);
			return 0;		
		}
		
	private:
		
		// download limiter
		CriticalSection downCS;
		int64_t			downTokens;
		
		// upload limiter
		CriticalSection upCS;
		int64_t			upTokens;
		
		HANDLE			hEvent;
		
		friend class Singleton<ThrottleManager>;
		
		// constructor
		ThrottleManager(void) : downTokens(0), upTokens(0)
		{
			hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			
			TimerManager::getInstance()->addListener(this);
		}

		// destructor
		~ThrottleManager(void)
		{
			TimerManager::getInstance()->removeListener(this);
			
			CloseHandle(hEvent);
		}		
		
		// TimerManagerListener
		void on(TimerManagerListener::Second, uint64_t aTick) throw()
		{
			if(!BOOLSETTING(THROTTLE_ENABLE))
				return;
				
			int64_t downLimit	= SETTING(MAX_DOWNLOAD_SPEED_LIMIT);
			int64_t upLimit		= SETTING(MAX_UPLOAD_SPEED_LIMIT);
			
			// limiter restriction
			if(SETTING(MAX_UPLOAD_SPEED_LIMIT) > 0) 
			{
				if(SETTING(MAX_UPLOAD_SPEED_LIMIT) < 5 * UploadManager::getInstance()->getSlots() + 4)
				{
					SettingsManager::getInstance()->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT, 5 * UploadManager::getInstance()->getSlots() + 4);
				}
				if((SETTING(MAX_DOWNLOAD_SPEED_LIMIT) > 7 * SETTING(MAX_UPLOAD_SPEED_LIMIT)) || (SETTING(MAX_DOWNLOAD_SPEED_LIMIT) == 0))
				{
					SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT, 7 * SETTING(MAX_UPLOAD_SPEED_LIMIT));
				}
			}

			if(SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME) > 0) 
			{
				if(SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME) < 5 * UploadManager::getInstance()->getSlots() + 4)
				{
					SettingsManager::getInstance()->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT_TIME, 5 * UploadManager::getInstance()->getSlots() + 4);
				}
				if((SETTING(MAX_DOWNLOAD_SPEED_LIMIT_TIME) > 7 * SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME)) || (SETTING(MAX_DOWNLOAD_SPEED_LIMIT_TIME) == 0)) 
				{
					SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT_TIME, 7 * SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME));
				}
			}

			// alternative limiter
			time_t currentTime;
			time(&currentTime);
			int currentHour = localtime(&currentTime)->tm_hour;
			if (SETTING(TIME_DEPENDENT_THROTTLE) &&
				((SETTING(BANDWIDTH_LIMIT_START) < SETTING(BANDWIDTH_LIMIT_END) &&
					currentHour >= SETTING(BANDWIDTH_LIMIT_START) && currentHour < SETTING(BANDWIDTH_LIMIT_END)) ||
				(SETTING(BANDWIDTH_LIMIT_START) > SETTING(BANDWIDTH_LIMIT_END) &&
					(currentHour >= SETTING(BANDWIDTH_LIMIT_START) || currentHour < SETTING(BANDWIDTH_LIMIT_END)))))
			{
				downLimit	= SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME);
				upLimit		= SETTING(MAX_DOWNLOAD_SPEED_LIMIT_TIME);
			} 
			
			// readd tokens			
			if(downLimit > 0)
			{
				Lock l(downCS);
				downTokens = downLimit * 1024;
			}
				
			if(upLimit > 0)
			{
				Lock l(upCS);
				upTokens = upLimit * 1024;
			}
			
			PulseEvent(hEvent);
		}
				
	};

}	// namespace dcpp
#endif	// _THROTTLEMANAGER_H