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

#include "NmdcHub.h"

#include "ResourceManager.h"
#include "ClientManager.h"
#include "SearchManager.h"
#include "ShareManager.h"
#include "CryptoManager.h"
#include "ConnectionManager.h"

#include "Socket.h"
#include "UserCommand.h"
#include "StringTokenizer.h"
#include "DebugManager.h"
#include "QueueManager.h"

#include "cvsversion.h"

NmdcHub::NmdcHub(const string& aHubURL) : Client(aHubURL, '|'), supportFlags(0),  
	adapter(this), state(STATE_CONNECT), 
	lastActivity(GET_TICK()), 
	reconnect(true), lastUpdate(0), validatenicksent(false)
{
	TimerManager::getInstance()->addListener(this);

};

NmdcHub::~NmdcHub() throw() {
	TimerManager::getInstance()->removeListener(this);
	socket->removeListener(this);		// Added by RevConnect
	Speaker<NmdcHubListener>::removeListeners();

	Lock l(cs);
	clearUsers();
};

void NmdcHub::connect() {
	setRegistered(false);
	setReconnDelay(120 + Util::rand(0, 60));
	reconnect = true;
	supportFlags = 0;
	lastMyInfoA.clear();
 	lastMyInfoB.clear();
	lastUpdate = 0;
	validatenicksent = false;

	if(socket->isConnected()) {
		disconnect();
	}

	reloadSettings();

	state = STATE_LOCK;

	if(getPort() == 0) {
		setPort(411);
	}
	socket->connect(getAddress(), getPort());
}

void NmdcHub::connect(const User* aUser) {
	checkstate(); 
	dcdebug("NmdcHub::connectToMe %s\n", aUser->getNick().c_str());
	if(getMode() == SettingsManager::CONNECTION_ACTIVE) {
		send("$ConnectToMe " + toNmdc(aUser->getNick()) + " " + getLocalIp() + ":" + Util::toString(SETTING(IN_PORT)) + "|");
		ConnectionManager::iConnToMeCount++;
	} else {
		send("$RevConnectToMe " + toNmdc(getNick()) + " " + toNmdc(aUser->getNick())  + "|");
	}
}

int64_t NmdcHub::getAvailable() const {
	Lock l(cs);
	int64_t x = 0;
	for(User::NickMap::const_iterator i = users.begin(); i != users.end(); ++i) {
		x+=i->second->getBytesShared();
	}
	return x;
}

void NmdcHub::refreshUserList(bool unknownOnly /* = false */) {
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

void NmdcHub::clearUsers() {
	if(ClientManager::getInstance() != NULL) {
		for(User::NickIter i = users.begin(); i != users.end(); ++i) {
			ClientManager::getInstance()->putUserOffline(i->second);
		}
	}
	users.clear();
}

void NmdcHub::onLine(const char* aLine) throw() {
	char *temp;
	lastActivity = GET_TICK();
	
	if(aLine[0] != '$') {
		if ((BOOLSETTING(SUPPRESS_MAIN_CHAT)) && (!getOp())) {
			return;
		}

		// Check if we're being banned...
		if(state != STATE_CONNECTED) {
			if(strstr(aLine, "banned") != 0) {
				reconnect = false;
			}
		}
		Speaker<NmdcHubListener>::fire(NmdcHubListener::Message(), this, Util::validateMessage(fromNmdc(aLine), true));
		return;
	}


	if(strncmp(aLine+1, "Search ", 7) == 0) {
		if(state != STATE_CONNECTED || (temp = strtok((char*)aLine+8, " ")) == NULL)
			return;

		string seeker = fromNmdc(temp);

		bool bPassive = (strnicmp(seeker.c_str(), "Hub:", 4) == 0);

		// We don't wan't to answer passive searches if we're in passive mode...
		if(bPassive == true && getMode() != SettingsManager::CONNECTION_ACTIVE) {
			return;
		}

		// Filter own searches
		if((getMode() == SettingsManager::CONNECTION_ACTIVE) && bPassive == false) {
			if((strcmp(seeker.c_str(), (getLocalIp() + ":" + Util::toString(SETTING(UDP_PORT))).c_str())) == 0) {
				return;
			}
		} else {
			if(strcmp(seeker.c_str() + 4, getNick().c_str()) == 0) {
				return;
			}
		}

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
					if(bPassive == true && strlen(seeker.c_str()) > 4) {
						Speaker<NmdcHubListener>::fire(NmdcHubListener::SearchFlood(), this, seeker.c_str()+4);
					} else {
						Speaker<NmdcHubListener>::fire(NmdcHubListener::SearchFlood(), this, seeker + STRING(NICK_UNKNOWN));
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

		int64_t size = _atoi64(temp);
		if((temp = strtok(NULL, "?")) == NULL)
			return;

		int type = atoi(temp) - 1;
		if((temp = strtok(NULL, "\0")) != NULL) {
			Speaker<NmdcHubListener>::fire(NmdcHubListener::Search(), this, seeker, a, size, type, fromNmdc(temp));
			if(bPassive == true && strlen(seeker.c_str()) > 4) {
				User::Ptr u;
					Lock l(cs);
				User::NickIter ni = users.find(seeker.c_str()+4);
					if(ni != users.end() && !ni->second->isSet(User::PASSIVE)) {
						u = ni->second;
						u->setPassive();
					}

				if(u) {
					updated(u);
				}
			}
		}
	} else if(strncmp(aLine+1, "MyINFO ", 7) == 0) {
		if((temp = strtok((char*)aLine+13, " ")) == NULL)
			return;

		User::Ptr u;
		dcassert(strlen(temp) > 0);
		string nick = fromNmdc(temp);

		{
			Lock l(cs);
			User::NickIter ni = users.find(nick);
			if(ni == users.end()) {
				u = users[nick] = ClientManager::getInstance()->getUser(nick, this);
			} else {
				u  = ni->second;
			}
		}
		// New MyINFO received, delete all user variables... if is bad MyINFO is user problem not my ;-)
		u->setBytesShared(0);
		u->setDescription(Util::emptyString);
		u->setcType(10);
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
				int iLen = strlen(temp);
				if(iLen > 0 && temp[iLen-1] == '>') {
					char *sTag;
					if((sTag = strrchr(temp, '<')) != NULL) {
						u->setTag(fromNmdc(sTag));
						temp[iLen-strlen(sTag)] = '\0';
					} else
						u->setTag(Util::emptyString);
				}
				u->setDescription(fromNmdc(temp));
			}
		}

		if((temp = strtok(NULL, "$")) != NULL) {
			// Support for extended $MyINFO with mode instead space in dolars before connection....
			if(temp[0] != ' ')
				u->setMode(temp);
			if(temp[strlen(temp)+1] != '$') {
				if((temp = strtok(NULL, "$")) != NULL) {
					char status = temp[strlen(temp)-1];
					u->setStatus(status);
					if(status != 1) {
						if(status == 2 || status == 3) {
							u->setFlag(User::AWAY);
							u->unsetFlag(User::SERVER);
							u->unsetFlag(User::FIREBALL);
						} else if(status == 4 || status == 5) {
							u->setFlag(User::SERVER);
							u->unsetFlag(User::AWAY);
							u->unsetFlag(User::FIREBALL);
						} else if(status == 6 || status == 7) {
							u->setFlag(User::SERVER);
							u->setFlag(User::AWAY);
							u->unsetFlag(User::FIREBALL);
						} else if(status == 8 || status == 9) {
							u->setFlag(User::FIREBALL);
							u->unsetFlag(User::AWAY);
							u->unsetFlag(User::SERVER);
						} else if(status == 10 || status == 11) {
							u->setFlag(User::FIREBALL);
							u->setFlag(User::AWAY);
							u->unsetFlag(User::SERVER);
						}
					} else {
						u->unsetFlag(User::AWAY);
						u->unsetFlag(User::SERVER);
						u->unsetFlag(User::FIREBALL);
					}
					temp[strlen(temp)-1] = '\0';
					u->setConnection(fromNmdc(temp));
					if(strcmp(temp, "28.8Kbps") == 0 || strcmp(temp, "33.6Kbps") == 0 ||
						strcmp(temp, "56Kbps") == 0 || strcmp(temp, "Modem") == 0) {
						u->setcType(1);
					} else if(strcmp(temp, "ISDN") == 0) {
						u->setcType(2);
					} else if(strcmp(temp, "Satellite") == 0 || strcmp(temp, "Microwave") == 0) {
						u->setcType(3);
					} else if(strcmp(temp, "Wireless") == 0) {
						u->setcType(4);
					} else if(strcmp(temp, "DSL") == 0) {
						u->setcType(5);
					} else if(strcmp(temp, "Cable") == 0) {
						u->setcType(6);
					} else if(strcmp(temp, "LAN(T1)") == 0 || strcmp(temp, "LAN(T3)") == 0) {
						u->setcType(7);
					} else {
						u->setcType(10);
					}
				}
			} else {
				u->setcType(10);
				u->setConnection(Util::emptyString);
			}
		}

		if(temp != NULL) {
			if(temp[strlen(temp)+2] != '$') {
				if((temp = strtok(NULL, "$")) != NULL)
					u->setEmail(Util::validateMessage(fromNmdc(temp), true));
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

		u->TagParts();

		if(state == STATE_MYINFO) {
			if (u->getNick() == getNick()) {
				setMe(u);
				state = STATE_CONNECTED;
				updateCounts(false);
				u->setFlag(User::DCPLUSPLUS);
				if(getMode() != SettingsManager::CONNECTION_ACTIVE)
					u->setFlag(User::PASSIVE);
				else
					u->unsetFlag(User::PASSIVE);
			}
		}
		Speaker<NmdcHubListener>::fire(NmdcHubListener::MyInfo(), this, u);
	} else if(strncmp(aLine+1, "Quit ", 5) == 0) {
		if((temp = strtok((char*)aLine+6, "\0")) == NULL)
			return;

		User::Ptr u;
		{
			Lock l(cs);
			User::NickIter i = users.find(fromNmdc(temp));
			if(i == users.end()) {
				dcdebug("C::onLine Quitting user %s not found\n", temp);
				return;
			}
			
			u = i->second;
			users.erase(i);
		}
		
		Speaker<NmdcHubListener>::fire(NmdcHubListener::Quit(), this, u);
		ClientManager::getInstance()->putUserOffline(u, true);
	} else if(strncmp(aLine+1, "ConnectToMe ", 12) == 0) {
		if(state != STATE_CONNECTED || (temp = strtok((char*)aLine+13, " ")) == NULL)
			return;

		if(strcmp(fromNmdc(temp).c_str(), getNick().c_str()) != 0) // Check nick... is CTM really for me ? ;o)
			return;

		if((temp = strtok(NULL, ":")) == NULL)
			return;

		char *server = temp;
		if((temp = strtok(NULL, " ")) == NULL)
			return;

		ConnectionManager::getInstance()->nmdcConnect(server, (short)atoi(temp), getNick()); 
		Speaker<NmdcHubListener>::fire(NmdcHubListener::ConnectToMe(), this, fromNmdc(server), (short)atoi(temp));
	} else if(strncmp(aLine+1, "RevConnectToMe ", 15) == 0) {
		if(state != STATE_CONNECTED) {
				return;
		}
		User::Ptr u;
		bool up = false;
		{
			Lock l(cs);
			if((temp = strtok((char*)aLine+16, " ")) == NULL)
				return;

			User::NickIter i = users.find(fromNmdc(temp));
			if((temp = strtok(NULL, "\0")) == NULL)
				return;

			if(strcmp(fromNmdc(temp).c_str(), getNick().c_str()) != 0) // Check nick... is RCTM really for me ? ;-)
				return;

			if(i == users.end()) {
				return;
			}

			u = i->second;
			if(!u->isSet(User::PASSIVE)) {
				u->setPassive();
				up = true;
			}
		}

		if(u) {
			if(getMode() == SettingsManager::CONNECTION_ACTIVE) {
				connectToMe(u);
				Speaker<NmdcHubListener>::fire(NmdcHubListener::RevConnectToMe(), this, u);
			} else {
				// Notify the user that we're passive too...
				if(up)
					revConnectToMe(u);
			}

			if(up)
				updated(u);
		}
	} else if(strncmp(aLine+1, "SR ", 3) == 0) {
		SearchManager::getInstance()->onSearchResult(aLine);
	} else if(strncmp(aLine+1, "HubName ", 8) == 0) {
		if((temp = strtok((char*)aLine+9, "\0")) == NULL)
			return;

		name = fromNmdc(temp);
		Speaker<NmdcHubListener>::fire(NmdcHubListener::HubName(), this);
	} else if(strncmp(aLine+1, "Supports", 8) == 0) {
		bool QuickList = false;
		StringList sl;
		if((temp = strtok((char*)aLine+10, " ")) == NULL) {
			validateNick(getNick());
			return;
		}

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
					myInfo(true);
					getNickList();
					QuickList = true;
				}
			}
			temp = strtok(NULL, " ");
		}
		if (!QuickList) {
			validateNick(getNick());
		}
		Speaker<NmdcHubListener>::fire(NmdcHubListener::Supports(), this, sl);
	} else if(strncmp(aLine+1, "UserCommand ", 12) == 0) {
		if((temp = strtok((char*)aLine+13, " ")) == NULL)
			return;

		int type = atoi(temp);
		if((temp = strtok(NULL, " ")) == NULL)
			return;

		if(type == UserCommand::TYPE_SEPARATOR || type == UserCommand::TYPE_CLEAR) {
			int ctx = atoi(temp);
			Speaker<NmdcHubListener>::fire(NmdcHubListener::UserCommand(), this, type, ctx, Util::emptyString, Util::emptyString);
		} else if(type == UserCommand::TYPE_RAW || type == UserCommand::TYPE_RAW_ONCE) {
			int ctx = atoi(temp);
			if((temp = strtok(NULL, "$")) == NULL)
				return;

			char *name = temp;
			if((temp = strtok(NULL, "\0")) == NULL)
				return;

			Speaker<NmdcHubListener>::fire(NmdcHubListener::UserCommand(), this, type, ctx, Util::validateMessage(fromNmdc(name), true, false), Util::validateMessage(fromNmdc(temp), true, false));
		}
	} else if(strncmp(aLine+1, "Lock ", 5) == 0) {
		char *lock;
		if(state != STATE_LOCK || (lock = strtok((char*)aLine+6, "\0")) == NULL)
			return;

		state = STATE_HELLO;
		if((temp = strstr(lock, " Pk=")) != NULL) { // simply the best ;-)
			lock[strlen(lock)-strlen(temp)] = '\0';
		} else
			lock = temp; // Pk is not usefull, don't waste cpu time to get it ;o)

		if(CryptoManager::getInstance()->isExtended(lock)) {
			StringList feat;
			feat.push_back("UserCommand");
			feat.push_back("NoGetINFO");
			feat.push_back("NoHello");
			feat.push_back("UserIP2");
			feat.push_back("TTHSearch");

			if (getStealth() == false)
				feat.push_back("QuickList");

			if(BOOLSETTING(COMPRESS_TRANSFERS))
				feat.push_back("GetZBlock");
			supports(feat);
			key(CryptoManager::getInstance()->makeKey(lock));
		} else {
			key(CryptoManager::getInstance()->makeKey(lock));
			validateNick(getNick());
		}

		Speaker<NmdcHubListener>::fire(NmdcHubListener::CLock(), this, lock, Util::emptyString);
	} else if(strncmp(aLine+1, "Hello ", 6) == 0) {
		if((temp = strtok((char*)aLine+7, "\0")) == NULL)
			return;

		string nick = fromNmdc(temp);
		User::Ptr u = ClientManager::getInstance()->getUser(nick, this);
		{
			Lock l(cs);
			users[nick] = u;
		}
		
		if(getNick() == nick) {
			setMe(u);
		
			u->setFlag(User::DCPLUSPLUS);
			if(getMode() != SettingsManager::CONNECTION_ACTIVE)
				u->setFlag(User::PASSIVE);
			else
				u->unsetFlag(User::PASSIVE);
		}

		if(state == STATE_HELLO) {
			state = STATE_CONNECTED;
			updateCounts(false);

			version();
			getNickList();
			myInfo(true);
		}
		Speaker<NmdcHubListener>::fire(NmdcHubListener::Hello(), this, u);

	} else if(strncmp(aLine+1, "ForceMove ", 10) == 0) {
		if((temp = strtok((char*)aLine+11, "\0")) == NULL)
			return;

		disconnect();
		Speaker<NmdcHubListener>::fire(NmdcHubListener::Redirect(), this, temp);
	} else if(strcmp(aLine+1, "HubIsFull") == 0) {
		Speaker<NmdcHubListener>::fire(NmdcHubListener::HubFull(), this);
	} else if(strncmp(aLine+1, "ValidateDenide", 14) == 0) {
		disconnect();
		Speaker<NmdcHubListener>::fire(NmdcHubListener::ValidateDenied(), this);
	} else if(strncmp(aLine+1, "UserIP ", 7) == 0) {
		User::List v;
		StringList l;
		if((temp = strtok((char*)aLine+8, "$$")) == NULL)
			return;

		while(temp != NULL) {
			l.push_back(fromNmdc(temp));
			temp = strtok(NULL, "$$");
		}
		string address;
		for(StringIter it = l.begin(); it != l.end(); ++it) {
			string::size_type j = 0;
			if((j = it->find(' ')) == string::npos)
				continue;
			if((j+1) == it->length())
				continue;
			string nick = it->substr(0, j);
			User::Ptr u = ClientManager::getInstance()->getUser(nick, this);
			v.push_back(u);
			string IP = it->substr(j+1);
			v.back()->setIp(IP);
			ClientManager::getInstance()->setIPNick(IP, u);
		}
		Speaker<NmdcHubListener>::fire(NmdcHubListener::UserIp(), this, v);
	} else if(strncmp(aLine+1, "NickList ", 9) == 0) {
		User::List v;
		if((temp = strtok((char*)aLine+10, "$$")) == NULL)
			return;

		while(temp != NULL) {
			v.push_back(ClientManager::getInstance()->getUser(fromNmdc(temp), this));
			temp = strtok(NULL, "$$");
		}

		{
			Lock l(cs);
			for(User::Iter it2 = v.begin(); it2 != v.end(); ++it2) {
				users[(*it2)->getNick()] = *it2;
			}
		}
		
		if(!(getSupportFlags() & SUPPORTS_NOGETINFO)) {
			string tmp;
			// Let's assume 10 characters per nick...
			tmp.reserve(v.size() * (11 + 10 + getNick().length())); 
				string n = ' ' +  toNmdc(getNick()) + '|';
			for(User::List::const_iterator i = v.begin(); i != v.end(); ++i) {
				tmp += "$GetINFO ";
					tmp += toNmdc((*i)->getNick());
					tmp += n;
			}
			if(!tmp.empty()) {
				send(tmp);
			}
		}

		Speaker<NmdcHubListener>::fire(NmdcHubListener::NickList(), this, v);
	} else if(strncmp(aLine+1, "OpList ", 7) == 0) {
		User::List v;
		if((temp = strtok((char*)aLine+8, "$$")) == NULL)
			return;

		while(temp != NULL) {
			v.push_back(ClientManager::getInstance()->getUser(fromNmdc(temp), this));
			v.back()->setFlag(User::OP);
			temp = strtok(NULL, "$$");
		}

		{
			Lock l(cs);
			for(User::Iter it2 = v.begin(); it2 != v.end(); ++it2) {
				users[(*it2)->getNick()] = *it2;
			}
		}
		Speaker<NmdcHubListener>::fire(NmdcHubListener::OpList(), this, v);
		updateCounts(false);
		// Special...to avoid op's complaining that their count is not correctly
		// updated when they log in (they'll be counted as registered first...)
		myInfo(false);
	} else if(strncmp(aLine+1, "To: ", 4) == 0) {
		char *temp1, *from;
		if((temp1 = strstr(aLine+5, "From:")) != NULL) {

			if((temp = strtok(temp1+6, "$")) == NULL)
				return;

			from = temp;
			from[strlen(from)-1] = '\0';
			if((temp = strtok(NULL, "\0")) == NULL)
				return;

			Speaker<NmdcHubListener>::fire(NmdcHubListener::PrivateMessage(), this, ClientManager::getInstance()->getUser(fromNmdc(from), this, false), Util::validateMessage(fromNmdc(temp), true));
		}
	} else if(strcmp(aLine+1, "GetPass") == 0) {
		setRegistered(true);
		Speaker<NmdcHubListener>::fire(NmdcHubListener::GetPassword(), this);
	} else if(strcmp(aLine+1, "BadPass") == 0) {
		Speaker<NmdcHubListener>::fire(NmdcHubListener::BadPassword(), this);
	} else if(strncmp(aLine+1, "LogedIn", 7) == 0) {
		Speaker<NmdcHubListener>::fire(NmdcHubListener::LoggedIn(), this);
	} else {
		dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
	}
}

string NmdcHub::checkNick(const string& aNick) {
	string tmp = aNick;
	string::size_type i = 0;
	while( (i = tmp.find_first_of("|$ ", i)) != string::npos) {
		tmp[i++]='_';
	}
	return tmp;
}

string NmdcHub::getHubURL() {
	return getAddressPort();
}

void NmdcHub::myInfo(bool alwaysSend) {
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
	if(getMode() == SettingsManager::CONNECTION_ACTIVE)
		modeChar = 'A';
	else if(getMode() == SettingsManager::CONNECTION_PASSIVE)
		modeChar = 'P';
	else if(getMode() == SettingsManager::CONNECTION_SOCKS5)
		modeChar = '5';
	
	string VERZE = DCVERSIONSTRING;	
	if (getStealth() == false) {
		tmp0 = "<StrgDC++";

#ifdef isCVS
	VERZE = VERSIONSTRING STRONGDCVERSIONSTRING CVSVERSION;
#else
	VERZE = VERSIONSTRING STRONGDCVERSIONSTRING;
#endif

	}
	string extendedtag = tmp0 + tmp1 + VERZE + tmp2 + modeChar + tmp3 + getCounts() + tmp4 + Util::toString(UploadManager::getInstance()->getSlots());

	string connection = Util::nlfound ? "NetLimiter [" + Util::toString(Util::nlspeed) + " kB/s]" : SETTING(CONNECTION);
	string speedDescription = "";

	if(BOOLSETTING(SHOW_DESCRIPTION_SPEED))
		speedDescription = "["+SETTING(DOWN_SPEED)+"/"+SETTING(UP_SPEED)+"]";

	if (SETTING(THROTTLE_ENABLE) && SETTING(MAX_UPLOAD_SPEED_LIMIT) != 0) {
		int tag = 0;
		tag = SETTING(MAX_UPLOAD_SPEED_LIMIT);
		extendedtag += tmp5 + Util::toString(tag);
	}

	if(getStealth() == false) {
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
	}

	string description = getDescription();

	string::size_type m = description.find_first_not_of(' ');
	string::size_type n = description.find_last_not_of(' ');
	if(m != string::npos)
		description = description.substr(m, n-m+1);
	else
		description = Util::emptyString;

	description = speedDescription + description;

	string myInfoA = 
		"$MyINFO $ALL " + toNmdc(getNick()) + " " + toNmdc(Util::validateMessage(description, false)) + 
		extendedtag +
		">$ $" + connection + StatusMode + "$" + toNmdc(Util::validateMessage(SETTING(EMAIL), false)) + '$';
	string myInfoB = ShareManager::getInstance()->getShareSizeString() + "$|";

 	if(lastMyInfoA != myInfoA || alwaysSend || (lastMyInfoB != myInfoB && lastUpdate + 15*60*1000 < GET_TICK()) ){
 		send(myInfoA + myInfoB);
 		lastMyInfoA = myInfoA;
 		lastMyInfoB = myInfoB;
		lastUpdate = GET_TICK();		
	}
}

void NmdcHub::disconnect() throw() {	
	state = STATE_CONNECT;
	socket->disconnect();
	{ 
		Lock l(cs);
		clearUsers();
	}
}

void NmdcHub::search(int aSizeType, int64_t aSize, int aFileType, const string& aString){
	checkstate(); 
	AutoArray<char> buf((char*)NULL);
	char c1 = (aSizeType == SearchManager::SIZE_DONTCARE || aSizeType == SearchManager::SIZE_EXACT) ? 'F' : 'T';
	char c2 = (aSizeType == SearchManager::SIZE_ATLEAST) ? 'F' : 'T';
	string tmp = toNmdc((aFileType == SearchManager::TYPE_TTH) ? "TTH:" + aString : aString);
	string::size_type i;
	while((i = tmp.find(' ')) != string::npos) {
		tmp[i] = '$';
	}
	int chars = 0;
	if((getMode() == SettingsManager::CONNECTION_ACTIVE) && (!BOOLSETTING(SEARCH_PASSIVE))) {
		string x = getLocalIp();
		buf = new char[x.length() + aString.length() + 64];
		chars = sprintf(buf, "$Search %s:%d %c?%c?%s?%d?%s|", x.c_str(), SETTING(UDP_PORT), c1, c2, Util::toString(aSize).c_str(), aFileType+1, tmp.c_str());
	} else {
		buf = new char[getNick().length() + aString.length() + 64];
		chars = sprintf(buf, "$Search Hub:%s %c?%c?%s?%d?%s|", toNmdc(getNick()).c_str(), c1, c2, Util::toString(aSize).c_str(), aFileType+1, tmp.c_str());
	}
	send(buf, chars);
}

void NmdcHub::kick(const User::Ptr& aUser, const string& aMsg) {
	checkstate(); 
	dcdebug("NmdcHub::kick\n");
	static const char str[] = 
		"$To: %s From: %s $<%s> You are being kicked because: %s|<%s> %s is kicking %s because: %s|";
	string msg2 = Util::validateMessage(aMsg, false);
	
	char* tmp = new char[sizeof(str) + 2*aUser->getNick().length() + 2*msg2.length() + 4*getNick().length()];
	const char* u = aUser->getNick().c_str();
	const char* n = getNick().c_str();
	const char* m = msg2.c_str();
	sprintf(tmp, str, u, n, n, m, n, n, u, m);
	send( toNmdc(tmp) );
	delete[] tmp;
	
	// Short, short break to allow the message to reach the NmdcHub...
	Thread::sleep(200);
	send("$Kick " + toNmdc(aUser->getNick()) + "|");
}

void NmdcHub::kick(const User* aUser, const string& aMsg) {
	checkstate(); 
	dcdebug("NmdcHub::kick\n");
	
	static const char str[] = 
		"$To: %s From: %s $<%s> You are being kicked because: %s|<%s> %s is kicking %s because: %s|";
	string msg2 = Util::validateMessage(aMsg, false);
	
	char* tmp = new char[sizeof(str) + 2*aUser->getNick().length() + 2*msg2.length() + 4*getNick().length()];
	const char* u = aUser->getNick().c_str();
	const char* n = getNick().c_str();
	const char* m = msg2.c_str();
	sprintf(tmp, str, u, n, n, m, n, n, u, m);
	send( toNmdc(tmp) );
	delete[] tmp;
	
	// Short, short break to allow the message to reach the NmdcHub...
	Thread::sleep(100);
	send("$Kick " + toNmdc(aUser->getNick()) + "|");
}

void NmdcHub::redirect(const User* aUser, const string& aServer, const string& aMsg) {
	checkstate(); 
	dcdebug("NmdcHub::opForceMove\n");
	send("$OpForceMove $Who:" + aUser->getNick() + "$Where:" + aServer + "$Msg:" + aMsg + "|");
}

// TimerManagerListener
void NmdcHub::on(TimerManagerListener::Second, u_int32_t aTick) throw() {
	if(socket && (lastActivity + getReconnDelay() * 1000) < aTick) {
		// Nothing's happened for ~120 seconds, check if we're connected, if not, try to connect...
		lastActivity = aTick;
		// Try to send something for the fun of it...
		if(isConnected()) {
			dcdebug("Testing writing...\n");
			socket->write("|", 1);
		} else {
			// Try to reconnect...
			if(reconnect && !getAddress().empty())
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

// BufferedSocketListener
void NmdcHub::on(BufferedSocketListener::Failed, const string& aLine) throw() {
	{
		Lock l(cs);
		clearUsers();
	}

	if(state == STATE_CONNECTED)
		state = STATE_CONNECT;
	Speaker<NmdcHubListener>::fire(NmdcHubListener::Failed(), this, aLine); 
	updateCounts(true);
}

/**
 * @file
 * $Id$
 */