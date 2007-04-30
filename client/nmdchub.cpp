/* 
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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
#include "ZUtils.h"

#include "svnversion.h"

NmdcHub::NmdcHub(const string& aHubURL) : Client(aHubURL, '|', false), supportFlags(0),
	lastbytesshared(0)
{
}

NmdcHub::~NmdcHub() throw() {
	clearUsers();
}


#define checkstate() if(state != STATE_NORMAL) return

void NmdcHub::connect(const OnlineUser& aUser, const string&) {
	checkstate(); 
	dcdebug("NmdcHub::connect %s\n", aUser.getIdentity().getNick().c_str());
	if(isActive()) {
		connectToMe(aUser);
	} else {
		revConnectToMe(aUser);
	}
}

void NmdcHub::refreshUserList(bool refreshOnly) {
	if(refreshOnly) {
		OnlineUser::List v;
		{
			Lock l(cs);
			for(NickIter i = users.begin(); i != users.end(); ++i) {
				v.push_back(i->second);
			}
			fire(ClientListener::UsersUpdated(), this, v);
		}
	} else {
		clearUsers();
		getNickList();
	}
}

OnlineUser& NmdcHub::getUser(const string& aNick) {
	OnlineUser* u = NULL;
	{
		Lock l(cs);

		NickIter i = users.find(aNick);
		if(i != users.end())
			return *i->second;
	}

	User::Ptr p;
	if(aNick == getCurrentNick()) {
		p = ClientManager::getInstance()->getMe();
	} else {
		p = ClientManager::getInstance()->getUser(aNick, getHubUrl());
	}

	{
		Lock l(cs);
		u = users.insert(make_pair(aNick, new OnlineUser(p, *this, 0))).first->second;
		u->getIdentity().setNick(aNick);
		if(u->getUser() == getMyIdentity().getUser()) {
			setMyIdentity(u->getIdentity());
		}
	}
	ClientManager::getInstance()->putOnline(u);
	return *u;
}

void NmdcHub::supports(const StringList& feat) { 
	string x;
	for(StringList::const_iterator i = feat.begin(); i != feat.end(); ++i) {
		x+= *i + ' ';
	}
	send("$Supports " + x + '|');
}

OnlineUser* NmdcHub::findUser(const string& aNick) const {
	Lock l(cs);
	NickIter i = users.find(aNick);
	return i == users.end() ? NULL : i->second;
}

void NmdcHub::putUser(const string& aNick) {
	OnlineUser* ou = NULL;
	{
		Lock l(cs);
		NickMap::iterator i = users.find(aNick);
		if(i == users.end())
			return;
		ou = i->second;
		users.erase(i);
	}
	availableBytes -= ou->getIdentity().getBytesShared();

	ClientManager::getInstance()->putOffline(ou);
	ou->dec();
}

void NmdcHub::clearUsers() {
	NickMap u2;
	
	{
		Lock l(cs);
		u2.swap(users);
		availableBytes = 0;
	}

	for(NickIter i = u2.begin(); i != u2.end(); ++i) {
		ClientManager::getInstance()->putOffline(i->second);
		if(!i->second->unique()) {
			i->second->dec();
		}
		i->second->dec();
	}
}

void NmdcHub::updateFromTag(Identity& id, const string& tag) {
	StringTokenizer<string> tok(tag, ',');
	string::size_type j;
	size_t slots = 1;
	for(StringIter i = tok.getTokens().begin(); i != tok.getTokens().end(); ++i) {
		if(i->length() < 2)
			continue;

		if(i->compare(0, 2, "H:") == 0) {
			StringTokenizer<string> t(i->substr(2), '/');
			if(t.getTokens().size() != 3)
				continue;
			id.set("HN", t.getTokens()[0]);
			id.set("HR", t.getTokens()[1]);
			id.set("HO", t.getTokens()[2]);
		} else if(i->compare(0, 2, "S:") == 0) {
			id.set("SL", i->substr(2));
			slots = Util::toUInt32(i->substr(2));			
		} else if((j = i->find("V:")) != string::npos) {
			i->erase(i->begin(), i->begin() + j + 2);
			id.set("VE", *i);
		} else if(i->compare(0, 2, "M:") == 0) {
			if(i->size() == 3) {
				if((*i)[2] == 'A')
					id.getUser()->unsetFlag(User::PASSIVE);
				else
					id.getUser()->setFlag(User::PASSIVE);
			}
		} else if((j = i->find("L:")) != string::npos) {
			i->erase(i->begin() + j, i->begin() + j + 2);
			if(slots > 0)
				id.getUser()->setLastDownloadSpeed((uint16_t)(Util::toInt(*i) / slots));
		}
	}
	/// @todo Think about this
	id.set("TA", '<' + tag + '>');
}

void NmdcHub::onLine(const string& aLine) throw() {
	if(aLine.length() == 0)
		return;

	if(aLine[0] != '$') {
		if (BOOLSETTING(SUPPRESS_MAIN_CHAT) && !isOp()) return;

		// Check if we're being banned...
		if(state != STATE_NORMAL) {
			if(Util::findSubString(aLine, "banned") != string::npos) {
				setAutoReconnect(false);
			}
		}
		string line = toUtf8(aLine);
		if(line[0] != '<') {
			fire(ClientListener::Message(), this, *(OnlineUser*)NULL, unescape(line));
			return;
		}
		string::size_type i = line.find('>', 2);
		if(i == string::npos) {
			fire(ClientListener::Message(), this, *(OnlineUser*)NULL, unescape(line));
			return;
		}
		string nick = line.substr(1, i-1);
		OnlineUser* ou = findUser(nick);

		if(line.size() > i+5 && Util::stricmp(line.substr(i+2, 3), "/me") == 0) {
			line = "* " + nick + line.substr(i+5);
		}

		if(ou) {
			fire(ClientListener::Message(), this, *ou, unescape(line));
		} else {
			OnlineUser& o = getUser(nick);
			// Assume that messages from unknown users come from the hub
			o.getIdentity().setHub(true);
			o.getIdentity().setHidden(true);
			fire(ClientListener::UserUpdated(), this, o);

			fire(ClientListener::Message(), this, o, unescape(line));
		}
		return;
    }

 	string cmd;
	string param;
	string::size_type x;
	
	if( (x = aLine.find(' ')) == string::npos) {
		cmd = aLine;
	} else {
		cmd = aLine.substr(0, x);
		param = toUtf8(aLine.substr(x+1));
	}

	if(cmd == "$Search") {
		if(state != STATE_NORMAL) {
			return;
		}
		string::size_type i = 0;
		string::size_type j = param.find(' ', i);
		if(j == string::npos || i == j)
			return;
		
		string seeker = param.substr(i, j-i);

		bool bPassive = (seeker.compare(0, 4, "Hub:") == 0);
		bool meActive = isActive();

		// We don't wan't to answer passive searches if we're in passive mode...
		if((bPassive == true) && !meActive) {
			return;
		}
		// Filter own searches
		if(meActive && bPassive == false) {
			if(seeker == ((getFavIp().empty() ? ClientManager::getInstance()->getCachedIp() : getFavIp()) + ":" + Util::toString(SearchManager::getInstance()->getPort()))) {
				return;
			}
		} else {
			// Hub:seeker
			if(Util::stricmp(seeker.c_str() + 4, getMyNick().c_str()) == 0) {
				return;
			}
		}

		i = j + 1;
		
		uint64_t tick = GET_TICK();
		clearFlooders(tick);

		seekers.push_back(make_pair(seeker, tick));

		// First, check if it's a flooder
		for(FloodIter fi = flooders.begin(); fi != flooders.end(); ++fi) {
			if(fi->first == seeker) {
				return;
			}
		}

		int count = 0;
		for(FloodIter fi = seekers.begin(); fi != seekers.end(); ++fi) {
			if(fi->first == seeker)
				count++;

			if(count > 7) {
			    if(isOp()) {
					if(bPassive)
						fire(ClientListener::SearchFlood(), this, seeker.substr(4));
					else
						fire(ClientListener::SearchFlood(), this, seeker + STRING(NICK_UNKNOWN));
				}
				
				flooders.push_back(make_pair(seeker, tick));
				return;
			}
		}

		int a;
		if(param[i] == 'F') {
			a = SearchManager::SIZE_DONTCARE;
		} else if(param[i+2] == 'F') {
			a = SearchManager::SIZE_ATLEAST;
		} else {
			a = SearchManager::SIZE_ATMOST;
		}
		i += 4;
		j = param.find('?', i);
		if(j == string::npos || i == j)
			return;
		string size = param.substr(i, j-i);
		i = j + 1;
		j = param.find('?', i);
		if(j == string::npos || i == j)
			return;
		int type = Util::toInt(param.substr(i, j-i)) - 1;
		i = j + 1;
		string terms = unescape(param.substr(i));

		if(terms.size() > 0) {
			if(seeker.compare(0, 4, "Hub:") == 0) {
				OnlineUser* u = findUser(seeker.substr(4));

				if(u == NULL) {
					return;
				}

				if(!u->getUser()->isSet(User::PASSIVE)) {
					u->getUser()->setFlag(User::PASSIVE);
					updated(*u);
				}
			}

			fire(ClientListener::NmdcSearch(), this, seeker, a, Util::toInt64(size), type, terms, bPassive);
		}
	} else if(cmd == "$MyINFO") {
		string::size_type i, j;
		i = 5;
		j = param.find(' ', i);
		if( (j == string::npos) || (j == i) )
			return;
		string nick = param.substr(i, j-i);
		
		if(nick.empty())
			return;

		i = j + 1;

		OnlineUser& u = getUser(nick);

		j = param.find('$', i);
		if(j == string::npos)
			return;

		string tmpDesc = unescape(param.substr(i, j-i));
		// Look for a tag...
		if(tmpDesc.size() > 0 && tmpDesc[tmpDesc.size()-1] == '>') {
			x = tmpDesc.rfind('<');
			if(x != string::npos) {
				// Hm, we have something...disassemble it...
				updateFromTag(u.getIdentity(), tmpDesc.substr(x + 1, tmpDesc.length() - x - 2));
				tmpDesc.erase(x);
			}
		}
		u.getIdentity().setDescription(tmpDesc);

		i = j + 3;
		j = param.find('$', i);
		if(j == string::npos)
			return;

		string connection = (i == j) ? Util::emptyString : param.substr(i, j-i-1);
		if(connection.empty()) {
			// No connection = bot...
			u.getUser()->setFlag(User::BOT);
			u.getIdentity().setBot(true);
		} else {
			u.getUser()->unsetFlag(User::BOT);
			u.getIdentity().setBot(false);
		}

		u.getIdentity().setHub(false);
		u.getIdentity().setHidden(false);

		u.getIdentity().setConnection(connection);

		char status = param[j-1];
		switch(status) {
			case 1:
				u.getUser()->unsetFlag(User::AWAY);
      			u.getUser()->unsetFlag(User::SERVER);
       			u.getUser()->unsetFlag(User::FIREBALL);
				break;
            case 2:
            case 3:
				u.getUser()->setFlag(User::AWAY);
				u.getUser()->unsetFlag(User::SERVER);
				u.getUser()->unsetFlag(User::FIREBALL);
           		break;
       		case 4:
            case 5:
				u.getUser()->setFlag(User::SERVER);
				u.getUser()->unsetFlag(User::AWAY);
				u.getUser()->unsetFlag(User::FIREBALL);
           		break;
       		case 6:
            case 7:
				u.getUser()->setFlag(User::SERVER);
				u.getUser()->setFlag(User::AWAY);
				u.getUser()->unsetFlag(User::FIREBALL);
           		break;
           	case 8:
            case 9:
				u.getUser()->setFlag(User::FIREBALL);
				u.getUser()->unsetFlag(User::AWAY);
				u.getUser()->unsetFlag(User::SERVER);
				if(u.getUser()->getLastDownloadSpeed() == 0)
					u.getUser()->setLastDownloadSpeed(100);
           		break;
           	case 10:
            case 11:
				u.getUser()->setFlag(User::FIREBALL);
				u.getUser()->setFlag(User::AWAY);
				u.getUser()->unsetFlag(User::SERVER);
				if(u.getUser()->getLastDownloadSpeed() == 0)
					u.getUser()->setLastDownloadSpeed(100);
           		break;
           	default:
				u.getUser()->unsetFlag(User::AWAY);
				u.getUser()->unsetFlag(User::SERVER);
				u.getUser()->unsetFlag(User::FIREBALL);
				break;
		}
		u.getIdentity().setStatus(Util::toString(status));

		i = j + 1;
		j = param.find('$', i);

		if(j == string::npos)
			return;

		u.getIdentity().setEmail(unescape(param.substr(i, j-i)));

		i = j + 1;
		j = param.find('$', i);
		if(j == string::npos)
			return;

		availableBytes -= u.getIdentity().getBytesShared();
		u.getIdentity().setBytesShared(param.substr(i, j-i));
		availableBytes += u.getIdentity().getBytesShared();

		if(u.getUser() == getMyIdentity().getUser()) {
			setMyIdentity(u.getIdentity());
		}
		
		fire(ClientListener::UserUpdated(), this, u);
	} else if(cmd == "$Quit") {
		if(!param.empty()) {
			const string& nick = param;
			OnlineUser* u = findUser(nick);
			if(!u)
				return;

			fire(ClientListener::UserRemoved(), this, *u);

			putUser(nick);
		}
	} else if(cmd == "$ConnectToMe") {
		if(state != STATE_NORMAL) {
			return;
		}
		string::size_type i = param.find(' ');
		string::size_type j;
		if( (i == string::npos) || ((i + 1) >= param.size()) ) {
			return;
		}
		i++;
		j = param.find(':', i);
		if(j == string::npos) {
			return;
		}
		string server = param.substr(i, j-i);
		if(j+1 >= param.size()) {
			return;
		}
		string port = param.substr(j+1);
		// For simplicity, we make the assumption that users on a hub have the same character encoding
		ConnectionManager::getInstance()->nmdcConnect(server, static_cast<uint16_t>(Util::toInt(port)), getMyNick(), getHubUrl(), getEncoding(), getStealth());
	} else if(cmd == "$RevConnectToMe") {
		if(state != STATE_NORMAL) {
			return;
		}

		string::size_type j = param.find(' ');
		if(j == string::npos) {
			return;
		}

		OnlineUser* u = findUser(param.substr(0, j));
		if(u == NULL)
			return;

		if(isActive()) {
			connectToMe(*u);
		} else {
			if(!u->getUser()->isSet(User::PASSIVE)) {
				u->getUser()->setFlag(User::PASSIVE);
				// Notify the user that we're passive too...
				revConnectToMe(*u);
				updated(*u);

				return;
			}
		}
	} else if(cmd == "$SR") {
		SearchManager::getInstance()->onSearchResult(aLine);
	} else if(cmd == "$HubName") {
		// If " - " found, the first part goes to hub name, rest to description
		// If no " - " found, first word goes to hub name, rest to description

		string::size_type i = param.find(" - ");
		if(i == string::npos) {
			i = param.find(' ');
			if(i == string::npos) {
			getHubIdentity().setNick(unescape(param));
			getHubIdentity().setDescription(Util::emptyString);			
		} else {
			getHubIdentity().setNick(unescape(param.substr(0, i)));
				getHubIdentity().setDescription(unescape(param.substr(i+1)));
			}
		} else {
			getHubIdentity().setNick(unescape(param.substr(0, i)));
			getHubIdentity().setDescription(unescape(param.substr(i+3)));
		}
		fire(ClientListener::HubUpdated(), this);
	} else if(cmd == "$Supports") {
		StringTokenizer<string> st(param, ' ');
		StringList& sl = st.getTokens();
		for(StringIter i = sl.begin(); i != sl.end(); ++i) {
			if(*i == "UserCommand") {
				supportFlags |= SUPPORTS_USERCOMMAND;
			} else if(*i == "NoGetINFO") {
				supportFlags |= SUPPORTS_NOGETINFO;
			} else if(*i == "UserIP2") {
				supportFlags |= SUPPORTS_USERIP2;
			}
		}
	} else if(cmd == "$UserCommand") {
		string::size_type i = 0;
		string::size_type j = param.find(' ');
		if(j == string::npos)
			return;

		int type = Util::toInt(param.substr(0, j));
		i = j+1;
 		if(type == UserCommand::TYPE_SEPARATOR || type == UserCommand::TYPE_CLEAR) {
			int ctx = Util::toInt(param.substr(i));
			fire(ClientListener::UserCommand(), this, type, ctx, Util::emptyString, Util::emptyString);
		} else if(type == UserCommand::TYPE_RAW || type == UserCommand::TYPE_RAW_ONCE) {
			j = param.find(' ', i);
			if(j == string::npos)
				return;
			int ctx = Util::toInt(param.substr(i));
			i = j+1;
			j = param.find('$');
			if(j == string::npos)
				return;
			string name = unescape(param.substr(i, j-i));
			i = j+1;
			string command = unescape(param.substr(i, param.length() - i));
			fire(ClientListener::UserCommand(), this, type, ctx, name, command);
		}
	} else if(cmd == "$Lock") {
		if(state != STATE_PROTOCOL) {
			return;
		}
		state = STATE_IDENTIFY;

		// Param must not be toUtf8'd...
		param = aLine.substr(6);

		if(!param.empty()) {
			string::size_type j = param.find(" Pk=");
			string lock, pk;
			if( j != string::npos ) {
				lock = param.substr(0, j);
				pk = param.substr(j + 4);
			} else {
				// Workaround for faulty linux hubs...
				j = param.find(" ");
				if(j != string::npos)
					lock = param.substr(0, j);
				else
					lock = param;
			}

			if(CryptoManager::getInstance()->isExtended(lock)) {
				StringList feat;
				feat.push_back("UserCommand");
				feat.push_back("NoGetINFO");
				feat.push_back("NoHello");
				feat.push_back("UserIP2");
				feat.push_back("TTHSearch");
				feat.push_back("ZPipe0");

				if(BOOLSETTING(COMPRESS_TRANSFERS))
					feat.push_back("GetZBlock");
				supports(feat);
			}

			key(CryptoManager::getInstance()->makeKey(lock));
			OnlineUser& ou = getUser(getCurrentNick());
			validateNick(ou.getIdentity().getNick());
		}
	} else if(cmd == "$Hello") {
		if(!param.empty()) {
			OnlineUser& u = getUser(param);

			if(u.getUser() == getMyIdentity().getUser()) {
				u.getUser()->setFlag(User::DCPLUSPLUS);
				if(isActive())
					u.getUser()->unsetFlag(User::PASSIVE);
				else
					u.getUser()->setFlag(User::PASSIVE);
			}

			if(state == STATE_IDENTIFY && u.getUser() == getMyIdentity().getUser()) {
				state = STATE_NORMAL;
				updateCounts(false);

				version();
				getNickList();
				myInfo();
			}

			fire(ClientListener::UserUpdated(), this, u);
		}
	} else if(cmd == "$ForceMove") {
		socket->disconnect(false);
		fire(ClientListener::Redirect(), this, param);
	} else if(cmd == "$HubIsFull") {
		fire(ClientListener::HubFull(), this);
	} else if(cmd == "$ValidateDenide") {		// Mind the spelling...
		socket->disconnect(false);
		fire(ClientListener::NickTaken(), this);
	} else if(cmd == "$UserIP") {
		if(!param.empty()) {
			OnlineUser::List v;
			StringTokenizer<string> t(param, "$$");
			StringList& l = t.getTokens();
			for(StringIter it = l.begin(); it != l.end(); ++it) {
				string::size_type j = 0;
				if((j = it->find(' ')) == string::npos)
					continue;
				if((j+1) == it->length())
					continue;

				OnlineUser* u = findUser(it->substr(0, j));
				
				if(!u)
					continue;

				u->getIdentity().setIp(it->substr(j+1));
				if(u->getUser() == getMyIdentity().getUser()) {
					setMyIdentity(u->getIdentity());
				}
				v.push_back(u);
			}

			fire(ClientListener::UsersUpdated(), this, v);
		}
	} else if(cmd == "$NickList") {
		if(!param.empty()) {
			OnlineUser::List v;
			StringTokenizer<string> t(param, "$$");
			StringList& sl = t.getTokens();

			for(StringIter it = sl.begin(); it != sl.end(); ++it) {
				if(it->empty())
					continue;
				
				v.push_back(&getUser(*it));
			}

			if(!(supportFlags & SUPPORTS_NOGETINFO)) {
				string tmp;
				// Let's assume 10 characters per nick...
				tmp.reserve(v.size() * (11 + 10 + getMyNick().length())); 
				string n = ' ' + fromUtf8(getMyNick()) + '|';
				for(OnlineUser::List::const_iterator i = v.begin(); i != v.end(); ++i) {
					tmp += "$GetINFO ";
					tmp += fromUtf8((*i)->getIdentity().getNick());
					tmp += n;
				}
				if(!tmp.empty()) {
					send(tmp);
				}
			} 

			fire(ClientListener::UsersUpdated(), this, v);
		}
	} else if(cmd == "$OpList") {
		if(!param.empty()) {
			OnlineUser::List v;
			StringTokenizer<string> t(param, "$$");
			StringList& sl = t.getTokens();
			for(StringIter it = sl.begin(); it != sl.end(); ++it) {
				if(it->empty())
					continue;
				OnlineUser& ou = getUser(*it);
				ou.getIdentity().setOp(true);
				if(ou.getUser() == getMyIdentity().getUser()) {
					setMyIdentity(ou.getIdentity());
				}
				v.push_back(&ou);
			}

			fire(ClientListener::UsersUpdated(), this, v);
			updateCounts(false);

			// Special...to avoid op's complaining that their count is not correctly
			// updated when they log in (they'll be counted as registered first...)
			myInfo();
		}
	} else if(cmd == "$To:") {
		string::size_type i = param.find("From:");
		if(i == string::npos)
			return;

		i+=6;
		string::size_type j = param.find('$', i);
		if(j == string::npos)
			return;

		string rtNick = param.substr(i, j - 1 - i);
		if(rtNick.empty())
			return;
		i = j + 1;

		if(param.size() < i + 3 || param[i] != '<')
			return;

		j = param.find('>', i);
		if(j == string::npos)
			return;

		string fromNick = param.substr(i+1, j-i-1);
		if(fromNick.empty())
			return;

        OnlineUser* replyTo = findUser(rtNick);
		OnlineUser* from = findUser(fromNick);

		string msg = param.substr(i);
		if(replyTo == NULL || from == NULL) {
			if(replyTo == 0) {
				// Assume it's from the hub
				replyTo = &getUser(rtNick);
				replyTo->getIdentity().setHub(true);
				replyTo->getIdentity().setHidden(true);
				fire(ClientListener::UserUpdated(), this, *replyTo);
			}
			if(from == 0) {
				// Assume it's from the hub
				from = &getUser(fromNick);
				from->getIdentity().setHub(true);
				from->getIdentity().setHidden(true);
				fire(ClientListener::UserUpdated(), this, *from);
			}
			
			// Update pointers just in case they've been invalidated
			replyTo = findUser(rtNick);
			from = findUser(fromNick);
		}

		OnlineUser& to = getUser(getMyNick());
		fire(ClientListener::PrivateMessage(), this, *from, to, *replyTo, unescape(msg));
	} else if(cmd == "$GetPass") {
		OnlineUser& ou = getUser(getMyNick());
		ou.getIdentity().set("RG", "1");
		setMyIdentity(ou.getIdentity());
		fire(ClientListener::GetPassword(), this);
	} else if(cmd == "$BadPass") {
		setPassword(Util::emptyString);
	} else if(cmd == "$ZOn") {
		socket->setMode(BufferedSocket::MODE_ZPIPE);
	} else if(cmd == "$HubTopic") {
		fire(ClientListener::HubTopic(), this, param);
	} else {
		dcassert(cmd[0] == '$');
		dcdebug("NmdcHub::onLine Unknown command %s\n", aLine.c_str());
	} 
}

string NmdcHub::checkNick(const string& aNick) {
	string tmp = aNick;
	for(size_t i = 0; i < aNick.size(); ++i) {
		if(static_cast<uint8_t>(tmp[i]) <= 32 || tmp[i] == '|' || tmp[i] == '$' || tmp[i] == '<' || tmp[i] == '>') {
			tmp[i] = '_';
		}
	}
	return tmp;
}

void NmdcHub::connectToMe(const OnlineUser& aUser) {
	checkstate();
	string userNick = aUser.getIdentity().getNick();
	dcdebug("NmdcHub::connectToMe %s\n", userNick.c_str());
	ConnectionManager::getInstance()->nmdcExpect(userNick, getMyNick(), getHubUrl());
	send("$ConnectToMe " + fromUtf8(userNick) + " " + getLocalIp() + ":" + Util::toString(ConnectionManager::getInstance()->getPort()) + "|");
}

void NmdcHub::revConnectToMe(const OnlineUser& aUser) {
	checkstate(); 
	dcdebug("NmdcHub::revConnectToMe %s\n", aUser.getIdentity().getNick().c_str());
	send("$RevConnectToMe " + fromUtf8(getMyNick()) + " " + fromUtf8(aUser.getIdentity().getNick()) + "|");
}

void NmdcHub::hubMessage(const string& aMessage) { 
	checkstate(); 
	char buf[256];
	snprintf(buf, sizeof(buf), "<%s> ", getMyNick().c_str());
	send(fromUtf8(string(buf)+ escape(aMessage) + "|"));
}

void NmdcHub::myInfo() {
	checkstate();
	
	reloadSettings(false);
	
	dcdebug("MyInfo %s...\n", getMyNick().c_str());
	char StatusMode = '\x01';

	char modeChar = '?';
	if(SETTING(OUTGOING_CONNECTIONS) == SettingsManager::OUTGOING_SOCKS5)
		modeChar = '5';
	else if(isActive())
		modeChar = 'A';
	else 
		modeChar = 'P';
	
	char tag[256];
	string dc;
	string version = DCVERSIONSTRING;
	int NetLimit = Util::getNetLimiterLimit();
	string connection = (NetLimit > -1) ? "NetLimiter [" + Util::toString(NetLimit) + " kB/s]" : SETTING(UPLOAD_SPEED);

	if (getStealth() == false) {
		dc = "<StrgDC++";
#ifdef SVNVERSION
		version = VERSIONSTRING SVNVERSION;
#else
		version = VERSIONSTRING;
#endif
		if (UploadManager::getInstance()->getFireballStatus()) {
			StatusMode += 8;
		} else if (UploadManager::getInstance()->getFileServerStatus()) {
			StatusMode += 4;
		}

		if(Util::getAway()) {
			StatusMode += 2;
		}
	} else {
		dc = "<++";
		if (connection == "Modem") { connection = "56Kbps"; }
		else if (connection == "Wireless") { connection = "Satellite"; }
	}

	if (SETTING(THROTTLE_ENABLE) && SETTING(MAX_UPLOAD_SPEED_LIMIT) != 0) {
		snprintf(tag, sizeof(tag), "%s V:%s,M:%c,H:%s,S:%d,L:%d>", dc.c_str(), version.c_str(), modeChar, getCounts().c_str(), UploadManager::getInstance()->getSlots(), SETTING(MAX_UPLOAD_SPEED_LIMIT));
	} else {
		snprintf(tag, sizeof(tag), "%s V:%s,M:%c,H:%s,S:%d>", dc.c_str(), version.c_str(), modeChar, getCounts().c_str(), UploadManager::getInstance()->getSlots());
	}

	char myinfo[512];
	snprintf(myinfo, sizeof(myinfo), "$MyINFO $ALL %s %s%s$ $%s%c$%s$", fromUtf8(getCurrentNick()).c_str(),
		fromUtf8(escape(getCurrentDescription())).c_str(), tag, connection.c_str(), StatusMode, 
		fromUtf8(escape(SETTING(EMAIL))).c_str());
	int64_t newbytesshared = ShareManager::getInstance()->getShareSize();
	if (strcmp(myinfo, lastmyinfo.c_str()) != 0 || newbytesshared < (lastbytesshared - 1048576) || newbytesshared > (lastbytesshared + 1048576)){
		lastmyinfo = myinfo;
		lastbytesshared = newbytesshared;
		tag[0] = NULL;
		snprintf(tag, sizeof(tag), "%s$|", Util::toString(newbytesshared).c_str());
		strcat(myinfo, tag);
		send(myinfo);	
	}
}

void NmdcHub::search(int aSizeType, int64_t aSize, int aFileType, const string& aString, const string&){
	checkstate(); 
	AutoArray<char> buf((char*)NULL);
	char c1 = (aSizeType == SearchManager::SIZE_DONTCARE || aSizeType == SearchManager::SIZE_EXACT) ? 'F' : 'T';
	char c2 = (aSizeType == SearchManager::SIZE_ATLEAST) ? 'F' : 'T';
	string tmp = ((aFileType == SearchManager::TYPE_TTH) ? "TTH:" + aString : fromUtf8(escape(aString)));
	string::size_type i;
	while((i = tmp.find(' ')) != string::npos) {
		tmp[i] = '$';
	}
	int chars = 0;
	size_t BUF_SIZE;	
	if(isActive() && !BOOLSETTING(SEARCH_PASSIVE)) {
		string x = getFavIp().empty() ? ClientManager::getInstance()->getCachedIp() : getFavIp();
		BUF_SIZE = x.length() + aString.length() + 64;
		buf = new char[BUF_SIZE];
		chars = snprintf(buf, BUF_SIZE, "$Search %s:%d %c?%c?%I64d?%d?%s|", x.c_str(), (int)SearchManager::getInstance()->getPort(), c1, c2, aSize, aFileType+1, tmp.c_str());
	} else {
		BUF_SIZE = getMyNick().length() + aString.length() + 64;
		buf = new char[BUF_SIZE];
		chars = snprintf(buf, BUF_SIZE, "$Search Hub:%s %c?%c?%I64d?%d?%s|", fromUtf8(getMyNick()).c_str(), c1, c2, aSize, aFileType+1, tmp.c_str());
	}
	send(buf, chars);
}

string NmdcHub::validateMessage(string tmp, bool reverse) {
	string::size_type i = 0;

	if(reverse) {
		while( (i = tmp.find("&#36;", i)) != string::npos) {
			tmp.replace(i, 5, "$");
			i++;
		}
		i = 0;
		while( (i = tmp.find("&#124;", i)) != string::npos) {
			tmp.replace(i, 6, "|");
			i++;
		}
		i = 0;
		while( (i = tmp.find("&amp;", i)) != string::npos) {
			tmp.replace(i, 5, "&");
			i++;
		}
	} else {
		i = 0;
		while( (i = tmp.find("&amp;", i)) != string::npos) {
			tmp.replace(i, 1, "&amp;");
			i += 4;
		}
		i = 0;
		while( (i = tmp.find("&#36;", i)) != string::npos) {
			tmp.replace(i, 1, "&amp;");
			i += 4;
		}
		i = 0;
		while( (i = tmp.find("&#124;", i)) != string::npos) {
			tmp.replace(i, 1, "&amp;");
			i += 4;
		}
		i = 0;
		while( (i = tmp.find('$', i)) != string::npos) {
			tmp.replace(i, 1, "&#36;");
			i += 4;
		}
		i = 0;
		while( (i = tmp.find('|', i)) != string::npos) {
			tmp.replace(i, 1, "&#124;");
			i += 5;
		}
	}
	return tmp;
}

void NmdcHub::privateMessage(const OnlineUser& aUser, const string& aMessage) { 
	checkstate();

	send("$To: " + fromUtf8(aUser.getIdentity().getNick()) + " From: " + fromUtf8(getMyNick()) + " $" + fromUtf8(escape("<" + getMyNick() + "> " + aMessage)) + "|");
	// Emulate a returning message...
	Lock l(cs);
	OnlineUser* ou = findUser(getMyNick());
	if(ou) {
		fire(ClientListener::PrivateMessage(), this, *ou, aUser, *ou, "<" + getMyNick() + "> " + aMessage);
	}
}

void NmdcHub::clearFlooders(uint64_t aTick) {
	while(!seekers.empty() && seekers.front().second + (5 * 1000) < aTick) {
		seekers.pop_front();
	}

	while(!flooders.empty() && flooders.front().second + (120 * 1000) < aTick) {
		flooders.pop_front();
	}
}

void NmdcHub::on(Connected) throw() {
	Client::on(Connected());

	supportFlags = 0;
	lastmyinfo.clear();
	lastbytesshared = 0;
}

void NmdcHub::on(Line, const string& aLine) throw() {
	Client::on(Line(), aLine);
	onLine(aLine);
}

void NmdcHub::on(Failed, const string& aLine) throw() {
	clearUsers();
	Client::on(Failed(), aLine);
	updateCounts(true);	
}

void NmdcHub::on(Second, uint32_t aTick) throw() {
	Client::on(Second(), aTick);

	if(state == STATE_NORMAL && (aTick > (getLastActivity() + 120*1000)) ) {
		send("|", 1);
	}
}

/**
 * @file
 * $Id$
 */
