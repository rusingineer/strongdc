/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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

#include "ResourceManager.h"
#include "ClientManager.h"
#include "SearchManager.h"
#include "ShareManager.h"
#include "UploadManager.h"

#include "Socket.h"
#include "UserCommand.h"
#include "StringTokenizer.h"

Client::Counts Client::counts;

Client::Client() : supportFlags(0), userInfo(true), 
	registered(false), firstHello(true), state(STATE_CONNECT), 
	socket(BufferedSocket::getSocket('|')), lastActivity(GET_TICK()), 
	countType(COUNT_UNCOUNTED), reconnect(true), lastUpdate(0)
{
	TimerManager::getInstance()->addListener(this);
	socket->addListener(this);
	dscrptn = (char *) calloc(96, sizeof(char));
	dscrptn[0] = NULL;
	dscrptnlen = 95;
};

Client::~Client() throw() {
	TimerManager::getInstance()->removeListener(this);
	socket->removeListener(this);
	removeListeners();

	clearUsers();
	updateCounts(true);

		BufferedSocket::putSocket(socket);
		socket = NULL;
		delete[] dscrptn;
};

void Client::updateCounts(bool aRemove) {
	// We always remove the count and then add the correct one if requested...

	if(countType == COUNT_NORMAL) {
		Thread::safeDec(&counts.normal);
	} else if(countType == COUNT_REGISTERED) {
		Thread::safeDec(&counts.registered);
	} else if(countType == COUNT_OP) {
		Thread::safeDec(&counts.op);
	}
	countType = COUNT_UNCOUNTED;

	if(!aRemove) {
		if(getOp()) {
			Thread::safeInc(&counts.op);
			countType = COUNT_OP;
		} else if(registered) {
			Thread::safeInc(&counts.registered);
			countType = COUNT_REGISTERED;
		} else {
			Thread::safeInc(&counts.normal);
			countType = COUNT_NORMAL;
		}
	}
}

void Client::connect(const string& aAddressPort) {
	addressPort = aAddressPort;
	string tmp;
	port = 411;
	Util::decodeUrl(aAddressPort, address, port, tmp);

	connect();
}

void Client::connect() {
	registered = false;
	reconnect = true;
	firstHello = true;
	supportFlags = 0;
	lastmyinfo = "";
	lastbytesshared = 0;
	validatenicksent = false;

	if(socket->isConnected()) {
		disconnect();
	}

	state = STATE_LOCK;

	socket->connect(address, port);
}

void Client::refreshUserList(bool unknownOnly /* = false */) {
	Lock l(cs);
	if(unknownOnly) {
		for(User::NickIter i = users.begin(); i != users.end(); ++i) {
			if(i->second->getConnection().empty()) {
				getInfo(i->second);
			}
		}
	} else {
		clearUsers();
		getNickList();
	}
}

void Client::clearUsers() {
	for(User::NickIter i = users.begin(); i != users.end(); ++i) {
		ClientManager::getInstance()->putUserOffline(i->second);		
	}
	users.clear();
}

void Client::onLine(const char *aLine) throw() {
	lastActivity = GET_TICK();
	
	if(aLine[0] != '$') {
		// Check if we're being banned...
		if(state != STATE_CONNECTED) {
			if(strstr(aLine, "banned") != 0) {
				reconnect = false;
			}
		}
		fire(ClientListener::MESSAGE, this, Util::validateMessage(aLine, true));
		return;
	}

	if((temp = strtok((char*) aLine, " ")) == NULL) {
		cmd = (char*) aLine;
	} else {
		cmd = temp;
		param = strtok(NULL, "\0");
	}

	if(strcmp(cmd, "$Search") == 0) {
		if(state != STATE_CONNECTED || param == NULL)
			return;

		if((temp = strtok(param, " ")) == NULL)
			return;

		char *seeker = temp;
		{
			Lock l(cs);
			u_int32_t tick = GET_TICK();

			seekers.push_back(make_pair(seeker, tick));

			// First, check if it's a flooder
			FloodIter fi;
			for(fi = flooders.begin(); fi != flooders.end(); ++fi) {
				if(fi->first == seeker)
					return;
			}

			int count = 0;
			for(fi = seekers.begin(); fi != seekers.end(); ++fi) {
				if(fi->first == seeker)
					count++;

				if(count > 7) {
					if( strnicmp(seeker, "Hub:", 4) == 0 ) {
						fire(ClientListener::SEARCH_FLOOD, this, strtok(seeker+4, "\0"));
					} else {
						fire(ClientListener::SEARCH_FLOOD, this, seeker + STRING(NICK_UNKNOWN));
					}

					flooders.push_back(make_pair(seeker, tick));
					return;
				}
			}
		}
		int a;
		if((temp = strtok(NULL, "?")) == NULL)
			return;

		if(strnicmp(temp, "F", 1) == 0 ) {
			a = SearchManager::SIZE_DONTCARE;
			if((temp = strtok(NULL, "?")) == NULL)
				return;
		} else {
			if((temp = strtok(NULL, "?")) == NULL)
				return;

			if(strnicmp(temp, "F", 1) == 0 ) {
				a = SearchManager::SIZE_ATLEAST;
			} else {
				a = SearchManager::SIZE_ATMOST;
			}
		}

		if((temp = strtok(NULL, "?")) == NULL)
			return;

		char *size = temp;
		if((temp = strtok(NULL, "?")) == NULL)
			return;

		int type = Util::toInt(temp) - 1;
		if((temp = strtok(NULL, "\0")) != NULL) {
			fire(ClientListener::SEARCH, this, seeker, a, size, type, temp);
			if( strnicmp(seeker, "Hub:", 4) == 0 ) {
				User::Ptr u;
				{
					Lock l(cs);
					User::NickIter ni = users.find(strtok(seeker+4, "\0"));
					if(ni != users.end() && !ni->second->isSet(User::PASSIVE)) {
						u = ni->second;
						u->setFlag(User::PASSIVE);
					}
				}

				if(u) {
					updated(u);
				}
			}
		}
	} else if(strcmp(cmd, "$MyINFO") == 0) {
		if(param == NULL)
			return;

		if((temp = strtok(param+5, " ")) == NULL)
			return;

		User::Ptr u;
		dcassert(strlen(temp) > 0);

		{
			Lock l(cs);
			User::NickIter ni = users.find(temp);
			if(ni == users.end()) {
				u = users[temp] = ClientManager::getInstance()->getUser(temp, this);
			} else {
				u  = ni->second;
			}
		}
		// New MyINFO received, delete all user variables... if is bad MyINFO is user problem not my ;-)
		u->setBytesShared(0);
		u->setDescription(Util::emptyString);
		u->setStatus(1);
		u->setConnection(Util::emptyString);
		u->setEmail(Util::emptyString);
		u->setTag(Util::emptyString);
		u->setClientType(Util::emptyString);
		u->setVersion(Util::emptyString);
		u->setMode(Util::emptyString);
		u->setHubs(Util::emptyString);
		u->setSlots(Util::emptyString);
		u->setUpload(Util::emptyString);

		if(temp[strlen(temp)+1] != '$') {
			if((temp = strtok(NULL, "$")) != NULL) {
				paramlen = strlen(temp);
				if(paramlen > dscrptnlen) {
					dscrptn = (char *) realloc(dscrptn, paramlen+1);
					dscrptnlen = paramlen;
				}
				strcpy(dscrptn, temp);
			}
		}

		if((temp = strtok(NULL, "$")) != NULL) {
			if(temp[strlen(temp)+1] != '$') {
				if((temp = strtok(NULL, "$")) != NULL) {
					char status;
					status = temp[strlen(temp)-1];
					u->setStatus(status);
					temp[strlen(temp)-1] = '\0';
					u->setConnection(temp);
				}
			} else {
				u->setStatus(1);
				u->setConnection(Util::emptyString);
			}
		}

		if(temp != NULL) {
			if(temp[strlen(temp)+2] != '$') {
				if((temp = strtok(NULL, "$")) != NULL)
					u->setEmail(Util::validateMessage(temp, true));
			} else {
				u->setEmail(Util::emptyString);
			}
		}

		if(temp != NULL) {
			if(temp[strlen(temp)+1] != '$') {
				if((temp = strtok(NULL, "$")) != NULL)
					u->setBytesShared(temp);
			}
		}

		if(dscrptn != NULL) {
			if(strlen(dscrptn) > 0 && dscrptn[strlen(dscrptn)-1] == '>') {
				char *sTag;
				if((sTag = strrchr(dscrptn, '<')) != NULL) {
					u->TagParts(sTag);
					dscrptn[strlen(dscrptn)-strlen(sTag)] = '\0';
				}
			}
			u->setDescription(dscrptn);
			dscrptn[0] = NULL;
		}

		if (u->getNick() == getNick()) {
			if(state == STATE_MYINFO) {
				state = STATE_CONNECTED;
				updateCounts(false);
			}	

			u->setFlag(User::DCPLUSPLUS);
			if(SETTING(CONNECTION_TYPE) != SettingsManager::CONNECTION_ACTIVE)
				u->setFlag(User::PASSIVE);
		}
		fire(ClientListener::MY_INFO, this, u);
	} else if(strcmp(cmd, "$Quit") == 0) {
		if(param == NULL)
			return;

		User::Ptr u;
		{
			Lock l(cs);
			User::NickIter i = users.find(param);
			if(i == users.end()) {
				dcdebug("C::onLine Quitting user %s not found\n", param);
				return;
			}
			
			u = i->second;
			users.erase(i);
		}
		
		fire(ClientListener::QUIT, this, u);
		ClientManager::getInstance()->putUserOffline(u, true);
	} else if(strcmp(cmd, "$ConnectToMe") == 0) {
		if(state != STATE_CONNECTED || param == NULL)
			return;

		if((temp = strtok(param, " ")) == NULL)
			return;

		if(strcmp(temp, getNick().c_str()) != 0) // Check nick... is CTM really for me ? ;o)
			return;

		if((temp = strtok(NULL, ":")) == NULL)
			return;

		char *server = temp;
		if((temp = strtok(NULL, " ")) == NULL)
			return;

		fire(ClientListener::CONNECT_TO_ME, this, server, temp);
	} else if(strcmp(cmd, "$RevConnectToMe") == 0) {
		if(state != STATE_CONNECTED || param == NULL) {
				return;
		}
		User::Ptr u;
		bool up = false;
		{
			Lock l(cs);
			if((temp = strtok(param, " ")) == NULL)
				return;

			User::NickIter i = users.find(temp);
			if((temp = strtok(NULL, "\0")) == NULL)
				return;

			if(strcmp(temp, getNick().c_str()) != 0) // Check nick... is RCTM really for me ? ;-)
				return;

			if(i == users.end()) {
				return;
			}

			u = i->second;
			if(!u->isSet(User::PASSIVE)) {
				u->setFlag(User::PASSIVE);
				up = true;
			}
		}

		if(u) {
			if(SETTING(CONNECTION_TYPE) == SettingsManager::CONNECTION_ACTIVE) {
				fire(ClientListener::REV_CONNECT_TO_ME, this, u);
			} else {
				// Notify the user that we're passive too...
				if(up)
					revConnectToMe(u);
			}

			if(up)
				updated(u);
		}
	} else if(strcmp(cmd, "$SR") == 0) {
		dcdebug(param);
		if(param != NULL)
			SearchManager::getInstance()->onSearchResult(param);
	} else if(strcmp(cmd, "$HubName") == 0) {
		if(param == NULL)
			return;

		name = param;
		fire(ClientListener::HUB_NAME, this);
	} else if(strcmp(cmd, "$Supports") == 0) {
		if(param == NULL)
			return;

		bool QuickList = false;
		StringList sl;
		if((temp = strtok(param, " ")) == NULL)
			return;

		while(temp != NULL) {
			sl.push_back(temp);
			if((strcmp(temp, "UserCommand")) == 0) {
				supportFlags |= SUPPORTS_USERCOMMAND;
			} else if((strcmp(temp, "NoGetINFO")) == 0) {
				supportFlags |= SUPPORTS_NOGETINFO;
			} else if((strcmp(temp, "UserIP2")) == 0) {
				supportFlags |= SUPPORTS_USERIP2;
			} else if((strcmp(temp, "QuickList")) == 0) {
				if(state == STATE_HELLO) {
					state = STATE_MYINFO;
					updateCounts(false);
					myInfo();
					getNickList();
					QuickList = true;
				}
			}
			temp = strtok(NULL, " ");
		}
		if (!QuickList) {
			validateNick(getNick());
		}
		fire(ClientListener::SUPPORTS, this, sl);
	} else if(strcmp(cmd, "$UserCommand") == 0) {
		if(param == NULL)
			return;

		if((temp = strtok(param, " ")) == NULL)
			return;

		int type = Util::toInt(temp);
		if((temp = strtok(NULL, " ")) == NULL)
			return;

		if(type == UserCommand::TYPE_SEPARATOR) {
			int ctx = Util::toInt(temp);
			fire(ClientListener::USER_COMMAND, this, type, ctx, Util::emptyString, Util::emptyString);
		} else if(type == UserCommand::TYPE_RAW || type == UserCommand::TYPE_RAW_ONCE) {
			int ctx = Util::toInt(temp);
			if((temp = strtok(NULL, "$")) == NULL)
				return;

			char *name = temp;
			if((temp = strtok(NULL, "\0")) == NULL)
				return;

			fire(ClientListener::USER_COMMAND, this, type, ctx, Util::validateMessage(name, true, false), Util::validateMessage(temp, true, false));
		}
	} else if(strcmp(cmd, "$Lock") == 0) {
		if(state != STATE_LOCK || param == NULL)
			return;

		state = STATE_HELLO;
		char *lock;
		if((temp = strstr(param, " Pk=")) != NULL) { // simply the best ;-)
			param[strlen(param)-strlen(temp)] = 0;
			lock = param;
		} else
			lock = param; // Pk is not usefull, don't waste cpu time to get it ;o)
		fire(ClientListener::C_LOCK, this, lock, Util::emptyString);
	} else if(strcmp(cmd, "$Hello") == 0) {
		if(param == NULL)
			return;

		User::Ptr u = ClientManager::getInstance()->getUser(param, this);
		{
			Lock l(cs);
			users[param] = u;
		}
		
		if(getNick() == param)
			setMe(u);
		
		if(u == getMe()) {
			if(state == STATE_HELLO) {
				state = STATE_CONNECTED;
				updateCounts(false);

				u->setFlag(User::DCPLUSPLUS);
				if(SETTING(CONNECTION_TYPE) != SettingsManager::CONNECTION_ACTIVE)
					u->setFlag(User::PASSIVE);
			}
			fire(ClientListener::HELLO, this, u);
			setFirstHello(false);
		} else {
			fire(ClientListener::HELLO, this, u);
		}
	} else if(strcmp(cmd, "$ForceMove") == 0) {
		if(param == NULL)
			return;

		disconnect();
		fire(ClientListener::FORCE_MOVE, this, param);
	} else if(strcmp(cmd, "$HubIsFull") == 0) {
		fire(ClientListener::HUB_FULL, this);
	} else if(strcmp(cmd, "$ValidateDenide") == 0) {
		disconnect();
		fire(ClientListener::VALIDATE_DENIED, this);
	} else if(strcmp(cmd, "$UserIP") == 0) {
		if(param == NULL)
			return;

		User::List v;
		StringList l;
		if((temp = strtok(param, "$$")) == NULL)
			return;

		while(temp != NULL) {
			l.push_back(temp);
			temp = strtok(NULL, "$$");
		}
		for(StringIter it = l.begin(); it != l.end(); ++it) {
			string::size_type j = 0;
			if((j = it->find(' ')) == string::npos)
				continue;
			if((j+1) == it->length())
				continue;
			v.push_back(ClientManager::getInstance()->getUser(it->substr(0, j), this));
			v.back()->setIp(it->substr(j+1));
		}
		fire(ClientListener::USER_IP, this, v);
	} else if(strcmp(cmd, "$NickList") == 0) {
		if(param == NULL)
			return;

		User::List v;
		if((temp = strtok(param, "$$")) == NULL)
			return;

		while(temp != NULL) {
			v.push_back(ClientManager::getInstance()->getUser(temp, this));
			temp = strtok(NULL, "$$");
		}

		{
			Lock l(cs);
			for(User::Iter it2 = v.begin(); it2 != v.end(); ++it2) {
				users[(*it2)->getNick()] = *it2;
			}
		}
		
		fire(ClientListener::NICK_LIST, this, v);
	} else if(strcmp(cmd, "$OpList") == 0) {
		if(param == NULL)
			return;

		User::List v;
		if((temp = strtok(param, "$$")) == NULL)
			return;

		while(temp != NULL) {
			v.push_back(ClientManager::getInstance()->getUser(temp, this));
			v.back()->setFlag(User::OP);
			temp = strtok(NULL, "$$");
		}

		{
			Lock l(cs);
			for(User::Iter it2 = v.begin(); it2 != v.end(); ++it2) {
				users[(*it2)->getNick()] = *it2;
			}
		}
		fire(ClientListener::OP_LIST, this, v);
		updateCounts(false);
		if(lastCounts != counts) {
			myInfo();
		}
	} else if(strcmp(cmd, "$To:") == 0) {
		if(param == NULL)
			return;

		char *temp1, *from;
		if((temp1 = strstr(param, "From:")) != NULL) {

			if((temp = strtok(temp1+6, "$")) == NULL)
				return;

			from = temp;
			from[strlen(from)-1] = '\0';
			if((temp = strtok(NULL, "\0")) == NULL)
				return;

			fire(ClientListener::PRIVATE_MESSAGE, this, ClientManager::getInstance()->getUser(from, this, false), Util::validateMessage(temp, true));
		}
	} else if(strcmp(cmd, "$GetPass") == 0) {
			registered = true;
			fire(ClientListener::GET_PASSWORD, this);
	} else if(strcmp(cmd, "$BadPass") == 0) {
			fire(ClientListener::BAD_PASSWORD, this);
	} else if(strcmp(cmd, "$LogedIn") == 0) {
		fire(ClientListener::LOGGED_IN, this);
	} else {
		dcassert(cmd[0] == '$');
		if(param != NULL) {
			strcat(cmd, " ");
			strcat(cmd, param);
		}
		dcdebug("Client::onLine Unknown command %s\n", cmd);
	}
}

void Client::myInfo() {
	if(state != STATE_CONNECTED && state != STATE_MYINFO) {
		return;
	}

	dcdebug("MyInfo %s...\n", getNick().c_str());
	lastCounts = counts;
	char StatusMode = '\x01';
	string tmp0 = "<++";
	string tmp1 = "\x1fU9";
	string tmp2 = "+L9";
	string tmp3 = "+G9";
	string tmp4 = "+R9";
	string tmp5 = "+K9";
	string::size_type i;
	
	for(i = 0; i < 6; i++) {
		tmp1[i]++;
	}
	for(i = 0; i < 3; i++) {
		tmp2[i]++; tmp3[i]++; tmp4[i]++; tmp5[i]++;
	}
	char modeChar = '?';
	if(SETTING(CONNECTION_TYPE) == SettingsManager::CONNECTION_ACTIVE)
		modeChar = 'A';
	else if(SETTING(CONNECTION_TYPE) == SettingsManager::CONNECTION_PASSIVE)
		modeChar = 'P';
	else if(SETTING(CONNECTION_TYPE) == SettingsManager::CONNECTION_SOCKS5)
		modeChar = '5';
	
	string VERZE = DCVERSIONSTRING;
	


	if ((!stealth) && (SETTING(CLIENT_EMULATION) != SettingsManager::CLIENT_DC) && (SETTING(CLIENT_EMULATION) != SettingsManager::CLIENT_CZDC)) {
		tmp0 = "<StrgDC++";
		VERZE = VERSIONSTRING CZDCVERSIONSTRING;
	}
	string extendedtag = tmp0 + tmp1 + VERZE + tmp2 + modeChar + tmp3 + getCounts() + tmp4 + Util::toString(UploadManager::getInstance()->getSlots());

	string connection = SETTING(CONNECTION);
	string speedDescription = "";

	if((!stealth) && (SETTING(CLIENT_EMULATION) != SettingsManager::CLIENT_DC)) {
	if (SETTING(THROTTLE_ENABLE) && SETTING(MAX_UPLOAD_SPEED_LIMIT) != 0) {
		int tag = 0;
		tag = SETTING(MAX_UPLOAD_SPEED_LIMIT);
		extendedtag += tmp5 + Util::toString(tag);
	}

	if (UploadManager::getFireballStatus()) {
		StatusMode += 8;
	} else if (UploadManager::getFileServerStatus()) {
		StatusMode += 4;
	}
	
	if(Util::getAway()) {
		StatusMode += 2;
	}
	} else {
		if (connection == "Modem") { connection = "56Kbps"; }
		if (connection == "Wireless") { connection = "Satellite"; }

		if (SETTING(THROTTLE_ENABLE) && SETTING(MAX_UPLOAD_SPEED_LIMIT) != 0) {
			speedDescription = Util::toString(SETTING(MAX_UPLOAD_SPEED_LIMIT)*8) + "kbps ";
		}

	}

	extendedtag += ">";

	string newmyinfo = ("$MyINFO $ALL " + Util::validateNick(getNick()) + " " + Util::validateMessage(speedDescription+getDescription(), false));
	if(BOOLSETTING(SEND_EXTENDED_INFO) || (((counts.normal) + (counts.registered) + (counts.op)) > 10) ) {
		newmyinfo += extendedtag;
	}
	int64_t newbytesshared = ShareManager::getInstance()->getShareSize();
	newmyinfo += ("$ $" + connection + StatusMode + "$" + Util::validateMessage(SETTING(EMAIL), false) + '$');
		if ( (newmyinfo != lastmyinfo) || ( (newbytesshared < (lastbytesshared - 1048576) ) || (newbytesshared > (lastbytesshared + 1048576) ) ) ){
		send(newmyinfo + Util::toString(newbytesshared) + "$|");
		lastmyinfo = newmyinfo;
		lastbytesshared = newbytesshared;
	}
}

void Client::disconnect() throw() {	
	state = STATE_CONNECT;
	socket->disconnect();
	{ 
		Lock l(cs);
		clearUsers();
	}
}

void Client::search(int aSizeType, int64_t aSize, int aFileType, const string& aString){
	checkstate(); 
	char* buf;
	char c1 = (aSizeType == SearchManager::SIZE_DONTCARE || aSizeType == SearchManager::SIZE_EXACT) ? 'F' : 'T';
	char c2 = (aSizeType == SearchManager::SIZE_ATLEAST) ? 'F' : 'T';
	string tmp = aString;
	string::size_type i;
	while((i = tmp.find(' ')) != string::npos) {
		tmp[i] = '$';
	}
	int chars = 0;
	if((SETTING(CONNECTION_TYPE) == SettingsManager::CONNECTION_ACTIVE) && (!BOOLSETTING(SEARCH_PASSIVE))) {
		string x = getLocalIp();
		buf = new char[x.length() + aString.length() + 64];
		chars = sprintf(buf, "$Search %s:%d %c?%c?%s?%d?%s|", x.c_str(), SETTING(IN_PORT), c1, c2, Util::toString(aSize).c_str(), aFileType+1, tmp.c_str());
	} else {
		buf = new char[getNick().length() + aString.length() + 64];
		chars = sprintf(buf, "$Search Hub:%s %c?%c?%s?%d?%s|", getNick().c_str(), c1, c2, Util::toString(aSize).c_str(), aFileType+1, tmp.c_str());
	}
	send(buf, chars);
	delete[] buf;
}

void Client::whoip(const string& aMsg) {
	checkstate(); 
	dcdebug("Client::whoip\n");

	send("$WhoIP " + Util::validateMessage(aMsg, false) + "|");
}

void Client::banned(const string& aMsg) {
	checkstate(); 
	dcdebug("Client::banned\n");

	send("$Banned " + Util::validateMessage(aMsg, false) + "|");
}

// TimerManagerListener
void Client::onAction(TimerManagerListener::Types type, u_int32_t aTick) throw() {
	if(type == TimerManagerListener::SECOND) {
		if(socket && (lastActivity + (120+Util::rand(0, 60)) * 1000) < aTick) {
			// Nothing's happened for ~120 seconds, check if we're connected, if not, try to connect...
			lastActivity = aTick;
			// Try to send something for the fun of it...
			if(isConnected()) {
				dcdebug("Testing writing...\n");
				socket->write("|", 1);
			} else {
				// Try to reconnect...
				if(reconnect && !address.empty())
					connect();
			}
		}
		{
			Lock l(cs);
			
			while(!seekers.empty() && seekers.front().second + (5 * 1000) < aTick) {
				seekers.pop_front();
			}
			
			while(!flooders.empty() && flooders.front().second + (120 * 1000) < aTick) {
				flooders.pop_front();
			}
		}
	} 
}

// BufferedSocketListener
void Client::onAction(BufferedSocketListener::Types type, const string& aLine) throw() {
	switch(type) {
	case BufferedSocketListener::LINE:
		onLine(aLine.c_str()); break;
	case BufferedSocketListener::FAILED:
		{
			Lock l(cs);
			clearUsers();
		}
		if(state == STATE_CONNECTED)
			state = STATE_CONNECT;
		fire(ClientListener::FAILED, this, aLine); break;
	default:
		dcassert(0);
	}
}

void Client::onAction(BufferedSocketListener::Types type) throw() {
	switch(type) {
	case BufferedSocketListener::CONNECTING:
		fire(ClientListener::CONNECTING, this); break;
	case BufferedSocketListener::CONNECTED:
		lastActivity = GET_TICK();
		fire(ClientListener::CONNECTED, this);
		break;
	default:
		break;
	}
}

/**
 * @file
 * $Id$
 */

