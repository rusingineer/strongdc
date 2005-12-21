/* 
 * Copyright (C) 2001-2004 Jacek Sieka, j_s at telia com
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

#include "stdinc.h"
#include "DCPlusPlus.h"
#include "../windows/PopupManager.h"
#include "ConnectionManager.h"
#include "DownloadManager.h"
#include "UploadManager.h"
#include "CryptoManager.h"
#include "ShareManager.h"
#include "SearchManager.h"
#include "QueueManager.h"
#include "ClientManager.h"
#include "HashManager.h"
#include "LogManager.h"
#include "FavoriteManager.h"
#include "SettingsManager.h"
#include "FinishedManager.h"
#include "ADLSearch.h"
#include "SSLSocket.h"
#include "DebugManager.h"
#include "ClientProfileManager.h"

#include "StringTokenizer.h"
#include "WebServerManager.h"

void startup(void (*f)(void*, const string&), void* p) {
	// "Dedicated to the near-memory of Nev. Let's start remembering people while they're still alive."
	// Nev's great contribution to dc++
	while(1) break;

	Util::initialize();

	ResourceManager::newInstance();
	SettingsManager::newInstance();

	LogManager::newInstance();
	TimerManager::newInstance();
	HashManager::newInstance();
	CryptoManager::newInstance();
	SearchManager::newInstance();
	ClientManager::newInstance();
	ConnectionManager::newInstance();
	DownloadManager::newInstance();
	UploadManager::newInstance();
	ShareManager::newInstance();
	FavoriteManager::newInstance();
	QueueManager::newInstance();
	FinishedManager::newInstance();
	ADLSearchManager::newInstance();
	SSLSocketFactory::newInstance();	
	DebugManager::newInstance();
	ClientProfileManager::newInstance();	
	PopupManager::newInstance();

	SettingsManager::getInstance()->load();	

	if(!SETTING(LANGUAGE_FILE).empty()) {
		ResourceManager::getInstance()->loadLanguage(SETTING(LANGUAGE_FILE));
	}

	FavoriteManager::getInstance()->load();
	SSLSocketFactory::getInstance()->loadCertificates();
	ClientProfileManager::getInstance()->load();	
	WebServerManager::newInstance();
	int i;
	for(i = 0; i < SettingsManager::SPEED_LAST; i++) {
		if(SETTING(CONNECTION) == SettingsManager::connectionSpeeds[i])
			break;
	}
	if(i == SettingsManager::SPEED_LAST) {
		SettingsManager::getInstance()->set(SettingsManager::CONNECTION, SettingsManager::connectionSpeeds[0]);
	}

	if(f != NULL)
		(*f)(p, STRING(HASH_DATABASE));
	HashManager::getInstance()->startup();
	if(f != NULL)
		(*f)(p, STRING(SHARED_FILES));
	ShareManager::getInstance()->refresh(true, false, true);
	if(f != NULL)
		(*f)(p, STRING(DOWNLOAD_QUEUE));
	QueueManager::getInstance()->loadQueue();

}

void shutdown() {
	HashManager::getInstance()->shutdown();
	ConnectionManager::getInstance()->shutdown();

	TimerManager::getInstance()->removeListeners();

	QueueManager::getInstance()->saveQueue();
	SettingsManager::getInstance()->save();

	WebServerManager::deleteInstance();
	ClientProfileManager::deleteInstance();	
	PopupManager::deleteInstance();
	SSLSocketFactory::deleteInstance();	
	ADLSearchManager::deleteInstance();
	FinishedManager::deleteInstance();
	ShareManager::deleteInstance();
	CryptoManager::deleteInstance();
	DownloadManager::deleteInstance();
	UploadManager::deleteInstance();
	QueueManager::deleteInstance();
	ConnectionManager::deleteInstance();
	SearchManager::deleteInstance();
	FavoriteManager::deleteInstance();
	ClientManager::deleteInstance();
	HashManager::deleteInstance();
	LogManager::deleteInstance();
	SettingsManager::deleteInstance();
	TimerManager::deleteInstance();
	DebugManager::deleteInstance();
	ResourceManager::deleteInstance();
}

/**
 * @file
 * $Id$
 */
