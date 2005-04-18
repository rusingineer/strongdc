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

#include "Client.h"

#include "BufferedSocket.h"
#include "DebugManager.h"

#include "FavoriteManager.h"

Client::Counts Client::counts;

Client::Client(const string& hubURL, char separator) : 
	socket(BufferedSocket::getSocket(separator)), reconnDelay(120), me(NULL),
	lastActivity(0), registered(false), hubUrl(hubURL), port(0), 
	countType(COUNT_UNCOUNTED)
{
	string file;
	Util::decodeUrl(hubURL, address, port, file);
	socket->addListener(this);
}

Client::~Client() throw() {
	socket->removeListener(this);

	updateCounts(true);
}

void Client::reloadSettings() {
	FavoriteHubEntry* hub = FavoriteManager::getInstance()->getFavoriteHubEntry(getHubUrl());
	if(hub) {
		setNick(checkNick(hub->getNick(true)));
		setDescription(hub->getUserDescription());
		setPassword(hub->getPassword());
		setStealth(hub->getStealth());
		switch(hub->getMode()) {
			case 1 : setIP(hub->getIP()); break;
			default: setIP(SETTING(EXTERNAL_IP));
		}
	} else {
		setNick(checkNick(SETTING(NICK)));
		setStealth(true);
		setIP(SETTING(EXTERNAL_IP)); // @todo: override ip option
	}
}

int Client::getMode() {
	int mode = 0;
	FavoriteHubEntry* hub = FavoriteManager::getInstance()->getFavoriteHubEntry(getHubUrl());
	if(hub) {
		switch(hub->getMode()) {
			case 1 :
			{
				mode = SettingsManager::INCOMING_DIRECT;
				setIP(hub->getIP());
				break;
			}
			case 2 :
			{
				mode = SettingsManager::INCOMING_FIREWALL_PASSIVE;
				break;
			}
			default:
			{
				mode = SETTING(INCOMING_CONNECTIONS);
				setIP(SETTING(EXTERNAL_IP)); // @todo: override ip option
			}
		}
	} else {
		mode = SETTING(INCOMING_CONNECTIONS);
		setIP(SETTING(EXTERNAL_IP)); // @todo: override ip option
	}
	return mode;
}

void Client::connect() {
	socket->disconnect();

	setReconnDelay(120 + Util::rand(0, 60));
	reloadSettings();
	setRegistered(false);

	socket->connect(address, port);

	updateActivity();
}

void Client::updateActivity() {
	lastActivity = GET_TICK();
}

void Client::updateCounts(bool aRemove) {
	// We always remove the count and then add the correct one if requested...
	if(countType == COUNT_NORMAL) {
		Thread::safeDec(counts.normal);
	} else if(countType == COUNT_REGISTERED) {
		Thread::safeDec(counts.registered);
	} else if(countType == COUNT_OP) {
		Thread::safeDec(counts.op);
	}

	countType = COUNT_UNCOUNTED;

	if(!aRemove) {
		if(getOp()) {
			Thread::safeInc(counts.op);
			countType = COUNT_OP;
		} else if(registered) {
			Thread::safeInc(counts.registered);
			countType = COUNT_REGISTERED;
		} else {
			Thread::safeInc(counts.normal);
			countType = COUNT_NORMAL;
		}
	}
}

string Client::getLocalIp() const { 
	// Best case - the server detected it
	if((!BOOLSETTING(NO_IP_OVERRIDE) || SETTING(EXTERNAL_IP).empty()) && !getIP().empty()) {
		return getIP();
	}

	if(!SETTING(EXTERNAL_IP).empty()) {
		return Socket::resolve(SETTING(EXTERNAL_IP));
	}

	string lip = socket->getLocalIp();

	if(lip.empty())
		return Util::getLocalIp();
	return lip;
}

/**
 * @file
 * $Id$
 */
