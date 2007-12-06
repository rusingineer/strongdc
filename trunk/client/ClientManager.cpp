/*
 * Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
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

#include "ClientManager.h"

#include "ShareManager.h"
#include "SearchManager.h"
#include "CryptoManager.h"
#include "FavoriteManager.h"
#include "SimpleXML.h"
#include "UserCommand.h"
#include "ResourceManager.h"
#include "LogManager.h"

#include "AdcHub.h"
#include "NmdcHub.h"

#include "QueueManager.h"
#include "FinishedManager.h"

Client* ClientManager::getClient(const string& aHubURL) {
	Client* c;
	if(Util::strnicmp("adc://", aHubURL.c_str(), 6) == 0) {
		c = new AdcHub(aHubURL, false);
	} else if(Util::strnicmp("adcs://", aHubURL.c_str(), 7) == 0) {
		c = new AdcHub(aHubURL, true);
	} else {
		c = new NmdcHub(aHubURL);
	}

	{
		Lock l(cs);
		clients.push_front(c);
	}

	c->addListener(this);

	return c;
}

void ClientManager::putClient(Client* aClient) {
	aClient->shutdown();

	fire(ClientManagerListener::ClientDisconnected(), aClient);
	aClient->removeListeners();

	{
		Lock l(cs);
		clients.remove(aClient);
	}
	delete aClient;
}

size_t ClientManager::getUserCount() const {
	Lock l(cs);
	return onlineUsers.size();
}

StringList ClientManager::getHubs(const CID& cid) const {
	Lock l(cs);
	StringList lst;
	OnlinePairC op = onlineUsers.equal_range(cid);
	for(OnlineIterC i = op.first; i != op.second; ++i) {
		lst.push_back(i->second->getClient().getHubUrl());
	}
	return lst;
}

StringList ClientManager::getHubNames(const CID& cid) const {
	Lock l(cs);
	StringList lst;
	OnlinePairC op = onlineUsers.equal_range(cid);
	for(OnlineIterC i = op.first; i != op.second; ++i) {
		lst.push_back(i->second->getClient().getHubName());
	}
	return lst;
}

StringList ClientManager::getNicks(const CID& cid) const {
	Lock l(cs);
	StringSet nicks;
	OnlinePairC op = onlineUsers.equal_range(cid);
	for(OnlineIterC i = op.first; i != op.second; ++i) {
		nicks.insert(i->second->getIdentity().getNick());
	}
	if(nicks.empty()) {
		// Offline perhaps?
		nicks.insert('{' + cid.toBase32() + '}');
	}
	return StringList(nicks.begin(), nicks.end());
}

string ClientManager::getConnection(const CID& cid) const {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(cid);
	if(i != onlineUsers.end()) {
		return i->second->getIdentity().getConnection();
	}
	return STRING(OFFLINE);
}

int64_t ClientManager::getAvailable() const {
	Lock l(cs);
	int64_t bytes = 0;
	for(OnlineIterC i = onlineUsers.begin(); i != onlineUsers.end(); ++i) {
		bytes += i->second->getIdentity().getBytesShared();
	}

	return bytes;
}

bool ClientManager::isConnected(const string& aUrl) const {
	Lock l(cs);

	for(Client::List::const_iterator i = clients.begin(); i != clients.end(); ++i) {
		if((*i)->getHubUrl() == aUrl) {
			return true;
		}
	}
	return false;
}

string ClientManager::findHub(const string& ipPort) const {
	Lock l(cs);

	string ip;
	uint16_t port = 411;
	string::size_type i = ipPort.find(':');
	if(i == string::npos) {
		ip = ipPort;
	} else {
		ip = ipPort.substr(0, i);
		port = static_cast<uint16_t>(Util::toInt(ipPort.substr(i+1)));
	}

	string url;
	for(Client::List::const_iterator i = clients.begin(); i != clients.end(); ++i) {
		const Client* c = *i;
		if(c->getIp() == ip) {
			// If exact match is found, return it
			if(c->getPort() == port)
				return c->getHubUrl();

			// Port is not always correct, so use this as a best guess...
			url = c->getHubUrl();
		}
	}

	return url;
}

const string& ClientManager::findHubEncoding(const string& aUrl) const {
	Lock l(cs);

	for(Client::List::const_iterator i = clients.begin(); i != clients.end(); ++i) {
		if((*i)->getHubUrl() == aUrl) {
			return *((*i)->getEncoding());
		}
	}
	return Text::systemCharset;
}

UserPtr ClientManager::findLegacyUser(const string& aNick) const throw() {
	Lock l(cs);
	dcassert(aNick.size() > 0);

	for(OnlineMap::const_iterator i = onlineUsers.begin(); i != onlineUsers.end(); ++i) {
		const OnlineUser* ou = i->second;
		if(ou->getUser()->isSet(User::NMDC) && Util::stricmp(ou->getIdentity().getNick(), aNick) == 0)
			return ou->getUser();
	}
	return UserPtr();
}

UserPtr ClientManager::getUser(const string& aNick, const string& aHubUrl) throw() {
	CID cid = makeCid(aNick, aHubUrl);
	Lock l(cs);

	UserIter ui = users.find(cid);
	if(ui != users.end()) {
		ui->second->setFlag(User::NMDC);
		return ui->second;
	}

	UserPtr p(new User(cid));
	p->setFlag(User::NMDC);
	users.insert(make_pair(cid, p));

	return p;
}

UserPtr ClientManager::getUser(const CID& cid) throw() {
	Lock l(cs);
	UserMap::const_iterator ui = users.find(cid);
	if(ui != users.end()) {
		return ui->second;
	}

	UserPtr p(new User(cid));
	users.insert(make_pair(cid, p));
	return p;
}

UserPtr ClientManager::findUser(const CID& cid) const throw() {
	Lock l(cs);
	UserMap::const_iterator ui = users.find(cid);
	if(ui != users.end()) {
		return ui->second;
	}
	return 0;
}

bool ClientManager::isOp(const UserPtr& user, const string& aHubUrl) const {
	Lock l(cs);
	OnlinePairC p = onlineUsers.equal_range(user->getCID());
	for(OnlineIterC i = p.first; i != p.second; ++i) {
		if(i->second->getClient().getHubUrl() == aHubUrl) {
			return i->second->getIdentity().isOp();
		}
	}
	return false;
}

bool ClientManager::isStealth(const string& aHubUrl) const {
	Lock l(cs);
	for(Client::Iter i = clients.begin(); i != clients.end(); ++i) {
		const Client* c = *i;
		if(c->getHubUrl() == aHubUrl) {
			return c->getStealth();
		}
	}
	return false;
}

CID ClientManager::makeCid(const string& aNick, const string& aHubUrl) const throw() {
	string n = Text::toLower(aNick);
	TigerHash th;
	th.update(n.c_str(), n.length());
	th.update(Text::toLower(aHubUrl).c_str(), aHubUrl.length());
	// Construct hybrid CID from the bits of the tiger hash - should be
	// fairly random, and hopefully low-collision
	return CID(th.finalize());
}

void ClientManager::putOnline(OnlineUser* ou) throw() {
	{
		Lock l(cs);
		onlineUsers.insert(make_pair(ou->getUser()->getCID(), ou));
	}

	if(!ou->getUser()->isOnline()) {
		ou->getUser()->setFlag(User::ONLINE);
		fire(ClientManagerListener::UserConnected(), ou->getUser());
	}
}

void ClientManager::putOffline(OnlineUser* ou) throw() {
	bool lastUser = false;
	{
		Lock l(cs);
		OnlinePair op = onlineUsers.equal_range(ou->getUser()->getCID());
		dcassert(op.first != op.second);
		for(OnlineIter i = op.first; i != op.second; ++i) {
			OnlineUser* ou2 = i->second;
			if(ou == ou2) {
				lastUser = (distance(op.first, op.second) == 1);
				onlineUsers.erase(i);
				break;
			}
		}
	}

	if(lastUser) {
		ou->getUser()->unsetFlag(User::ONLINE);
		fire(ClientManagerListener::UserDisconnected(), ou->getUser());
	}
}

void ClientManager::connect(const UserPtr& p, const string& token) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(p->getCID());
	if(i != onlineUsers.end()) {
		OnlineUser* u = i->second;
		u->getClient().connect(*u, token);
	}
}

void ClientManager::privateMessage(const UserPtr& p, const string& msg) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(p->getCID());
	if(i != onlineUsers.end()) {
		OnlineUser* u = i->second;
		u->getClient().privateMessage(*u, msg);
	}
}

void ClientManager::send(AdcCommand& cmd, const CID& cid) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(cid);
	if(i != onlineUsers.end()) {
		OnlineUser& u = *i->second;
		if(cmd.getType() == AdcCommand::TYPE_UDP && !u.getIdentity().isUdpActive()) {
			cmd.setType(AdcCommand::TYPE_DIRECT);
			cmd.setTo(u.getIdentity().getSID());
			u.getClient().send(cmd);
		} else {
			try {
				Socket udp;
				udp.writeTo(u.getIdentity().getIp(), static_cast<uint16_t>(Util::toInt(u.getIdentity().getUdpPort())), cmd.toString(getMe()->getCID()));
			} catch(const SocketException&) {
				dcdebug("Socket exception sending ADC UDP command\n");
			}
		}
	}
}

void ClientManager::infoUpdated() {
	Lock l(cs);
	for(Client::Iter i = clients.begin(); i != clients.end(); ++i) {
		if((*i)->isConnected()) {
			(*i)->info(false);
		}
	}
}

void ClientManager::on(NmdcSearch, Client* aClient, const string& aSeeker, int aSearchType, int64_t aSize, 
									int aFileType, const string& aString, bool isPassive) throw() 
{
	Speaker<ClientManagerListener>::fire(ClientManagerListener::IncomingSearch(), aString);

	// We don't wan't to answer passive searches if we're in passive mode...
	if(isPassive && !ClientManager::getInstance()->isActive(aClient->getHubUrl())) {
		return;
	}

	SearchResult::List l;
	ShareManager::getInstance()->search(l, aString, aSearchType, aSize, aFileType, aClient, isPassive ? 5 : 10);
	if(l.size() > 0) {
		if(isPassive) {
			string name = aSeeker.substr(4);
			// Good, we have a passive seeker, those are easier...
			string str;
			for(SearchResult::Iter i = l.begin(); i != l.end(); ++i) {
				SearchResult* sr = *i;
				str += sr->toSR(*aClient);
				str[str.length()-1] = 5;
				str += Text::fromUtf8(name, *(aClient->getEncoding()));
				str += '|';

				sr->decRef();
			}
			
			if(str.size() > 0)
				aClient->send(str);
			
		} else {
			try {
				Socket udp;
				string ip, file;
				uint16_t port = 0;
				Util::decodeUrl(aSeeker, ip, port, file);
				ip = Socket::resolve(ip);
				
				if(port == 0) 
					port = 412;
				for(SearchResult::Iter i = l.begin(); i != l.end(); ++i) {
					SearchResult* sr = *i;
					udp.writeTo(ip, port, sr->toSR(*aClient));
					sr->decRef();
				}
			} catch(...) {
				for(SearchResult::Iter i = l.begin(); i != l.end(); ++i) {
					SearchResult* sr = *i;
					sr->decRef();
				}
				dcdebug("Search caught error\n");
			}
		}
	} else if(!isPassive && (aFileType == SearchManager::TYPE_TTH) && (aString.compare(0, 4, "TTH:") == 0)) {
		PartsInfo partialInfo;
		TTHValue aTTH(aString.substr(4));
		if(!QueueManager::getInstance()->handlePartialSearch(aTTH, partialInfo)) {
			// if not found, try to find in finished list
			if(!FinishedManager::getInstance()->handlePartialRequest(aTTH, partialInfo)){
				return;
			}
		}
		
		string ip, file;
		uint16_t port = 0;
		Util::decodeUrl(aSeeker, ip, port, file);
		SearchManager::getInstance()->sendPSR(ip, port, true, aClient->getMyNick(), aClient->getIpPort(), aTTH.toBase32(), partialInfo);
	}
}

void ClientManager::userCommand(const UserPtr& p, const ::UserCommand& uc, StringMap& params, bool compatibility) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(p->getCID());
	if(i == onlineUsers.end())
		return;

	OnlineUser& ou = *i->second;
	ou.getIdentity().getParams(params, "user", compatibility);
	ou.getClient().getHubIdentity().getParams(params, "hub", false);
	ou.getClient().getMyIdentity().getParams(params, "my", compatibility);
	ou.getClient().escapeParams(params);
	ou.getClient().sendUserCmd(Util::formatParams(uc.getCommand(), params, false));
}

void ClientManager::sendRawCommand(const UserPtr& user, const Client& c, const int aRawCommand) {
	string rawCommand = c.getRawCommand(aRawCommand);
	if (!rawCommand.empty()) {
		StringMap ucParams;

		::UserCommand uc = ::UserCommand(0, 0, 0, 0, "", rawCommand, "");
		userCommand(user, uc, ucParams, true);
	}
}

void ClientManager::on(AdcSearch, const Client*, const AdcCommand& adc, const CID& from) throw() {
	SearchManager::getInstance()->respond(adc, from);
}

const string& ClientManager::getHubUrl(const UserPtr& aUser) const {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(aUser->getCID());
	if(i != onlineUsers.end()) {
		return i->second->getClient().getHubUrl();
	}
	return Util::emptyString;
}

void ClientManager::search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken) {
	Lock l(cs);

	updateCachedIp(); // no point in doing a resolve for every single hub we're searching on

	for(Client::Iter i = clients.begin(); i != clients.end(); ++i) {
		if((*i)->isConnected()) {
			(*i)->search(aSizeMode, aSize, aFileType, aString, aToken);
		}
	}
}

void ClientManager::search(StringList& who, int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken) {
	Lock l(cs);

	updateCachedIp(); // no point in doing a resolve for every single hub we're searching on

	for(StringIter it = who.begin(); it != who.end(); ++it) {
		string& client = *it;
		for(Client::Iter j = clients.begin(); j != clients.end(); ++j) {
			Client* c = *j;
			if(c->isConnected() && c->getHubUrl() == client) {
				c->search(aSizeMode, aSize, aFileType, aString, aToken);
			}
		}
	}
}

void ClientManager::on(TimerManagerListener::Minute, uint64_t /*aTick*/) throw() {
	Lock l(cs);

	// Collect some garbage...
	UserIter i = users.begin();
	while(i != users.end()) {
		if(i->second->unique()) {
			users.erase(i++);
		} else {
			++i;
		}
	}

	for(Client::Iter j = clients.begin(); j != clients.end(); ++j) {
		(*j)->info(false);
	}
}

UserPtr& ClientManager::getMe() {
	if(!me) {
		Lock l(cs);
		if(!me) {
			me = new User(getMyCID());
			users.insert(make_pair(me->getCID(), me));
		}
	}
	return me;
}

const CID& ClientManager::getMyPID() {
	if(pid.isZero())
		pid = CID(SETTING(PRIVATE_ID));
	return pid;
}

CID ClientManager::getMyCID() {
	TigerHash tiger;
	tiger.update(getMyPID().data(), CID::SIZE);
	return CID(tiger.finalize());
}

void ClientManager::on(Failed, const Client* client, const string&) throw() { 
	fire(ClientManagerListener::ClientDisconnected(), client);
}

void ClientManager::on(HubUserCommand, const Client* client, int aType, int ctx, const string& name, const string& command) throw() { 
	if(BOOLSETTING(HUB_USER_COMMANDS)) {
		if(aType == UserCommand::TYPE_REMOVE) {
			int cmd = FavoriteManager::getInstance()->findUserCommand(name, client->getHubUrl());
			if(cmd != -1)
				FavoriteManager::getInstance()->removeUserCommand(cmd);
		} else if(aType == UserCommand::TYPE_CLEAR) {
 			FavoriteManager::getInstance()->removeHubUserCommands(ctx, client->getHubUrl());
 		} else {
			FavoriteManager::getInstance()->addUserCommand(aType, ctx, ::UserCommand::FLAG_NOSAVE, name, command, client->getHubUrl());
		}
	}
}

void ClientManager::updateCachedIp() {
	// Best case - the server detected it
	if((!BOOLSETTING(NO_IP_OVERRIDE) || SETTING(EXTERNAL_IP).empty())) {
		for(Client::Iter i = clients.begin(); i != clients.end(); ++i) {
			if(!(*i)->getMyIdentity().getIp().empty()) {
				cachedIp = (*i)->getMyIdentity().getIp();
				return;
			}
		}
	}

	if(!SETTING(EXTERNAL_IP).empty()) {
		cachedIp = Socket::resolve(SETTING(EXTERNAL_IP));
		return;
	}

	//if we've come this far just use the first client to get the ip.
	if(clients.size() > 0)
		cachedIp = (*clients.begin())->getLocalIp();
}

void ClientManager::setListLength(const UserPtr& p, const string& listLen) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(p->getCID());
	if(i != onlineUsers.end()) {
		i->second->getIdentity().set("LL", listLen);
	}
}

void ClientManager::fileListDisconnected(const UserPtr& p) {
	string report = Util::emptyString;
	Client* c = NULL;
	{
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(p->getCID());
		if(i != onlineUsers.end()) {
			OnlineUser& ou = *i->second;
	
			int fileListDisconnects = Util::toInt(ou.getIdentity().get("FD")) + 1;
			ou.getIdentity().set("FD", Util::toString(fileListDisconnects));

			if(SETTING(ACCEPTED_DISCONNECTS) == 0)
				return;

			if(fileListDisconnects == SETTING(ACCEPTED_DISCONNECTS)) {
				c = &ou.getClient();
				report = ou.getIdentity().setCheat(ou.getClient(), "Disconnected file list " + Util::toString(fileListDisconnects) + " times", false);
				ClientManager::getInstance()->sendRawCommand(ou.getUser(), ou.getClient(), SETTING(DISCONNECT_RAW));
			}
		}
	}
	if(c && !report.empty() && BOOLSETTING(DISPLAY_CHEATS_IN_MAIN_CHAT)) {
		c->cheatMessage(report);
	}
}

void ClientManager::connectionTimeout(const UserPtr& p) {
	string report = Util::emptyString;
	bool remove = false;
	Client* c = NULL;
	{
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(p->getCID());
		if(i != onlineUsers.end()) {
			OnlineUser& ou = *i->second;
	
			int connectionTimeouts = Util::toInt(ou.getIdentity().get("TO")) + 1;
			ou.getIdentity().set("TO", Util::toString(connectionTimeouts));
	
			if(SETTING(ACCEPTED_TIMEOUTS) == 0)
				return;
	
			if(connectionTimeouts == SETTING(ACCEPTED_TIMEOUTS)) {
				c = &ou.getClient();
				report = ou.getIdentity().setCheat(ou.getClient(), "Connection timeout " + Util::toString(connectionTimeouts) + " times", false);
				remove = true;
				sendRawCommand(ou.getUser(), ou.getClient(), SETTING(TIMEOUT_RAW));
			}
		}
	}
	if(remove) {
		try {
			QueueManager::getInstance()->removeTestSUR(p);
		} catch(...) {
		}
	}
	if(c && !report.empty() && BOOLSETTING(DISPLAY_CHEATS_IN_MAIN_CHAT)) {
		c->cheatMessage(report);
	}
}

void ClientManager::checkCheating(const UserPtr& p, DirectoryListing* dl) {
	string report = Util::emptyString;
	OnlineUser* ou = NULL;
	{
		Lock l(cs);

		OnlineIterC i = onlineUsers.find(p->getCID());
		if(i == onlineUsers.end())
			return;

		ou = i->second;

		int64_t statedSize = ou->getIdentity().getBytesShared();
		int64_t realSize = dl->getTotalSize();
	
		double multiplier = ((100+(double)SETTING(PERCENT_FAKE_SHARE_TOLERATED))/100); 
		int64_t sizeTolerated = (int64_t)(realSize*multiplier);
		string detectString = Util::emptyString;
		string inflationString = Util::emptyString;
		ou->getIdentity().set("RS", Util::toString(realSize));
		bool isFakeSharing = false;
	
		if(statedSize > sizeTolerated) {
			isFakeSharing = true;
		}

		if(isFakeSharing) {
			ou->getIdentity().set("BF", "1");
			detectString += STRING(CHECK_MISMATCHED_SHARE_SIZE);
			if(realSize == 0) {
				detectString += STRING(CHECK_0BYTE_SHARE);
			} else {
				double qwe = (double)((double)statedSize / (double)realSize);
				char buf[128];
				snprintf(buf, sizeof(buf), CSTRING(CHECK_INFLATED), Util::toString(qwe).c_str());
				inflationString = buf;
				detectString += inflationString;
			}
			detectString += STRING(CHECK_SHOW_REAL_SHARE);

			report = ou->getIdentity().setCheat(ou->getClient(), detectString, false);
			sendRawCommand(ou->getUser(), ou->getClient(), SETTING(FAKESHARE_RAW));
		}
		ou->getIdentity().set("FC", "1");
	}
	ou->getClient().updated(*ou);
	if(!report.empty() && BOOLSETTING(DISPLAY_CHEATS_IN_MAIN_CHAT))
		ou->getClient().cheatMessage(report);
}

void ClientManager::setCheating(const UserPtr& p, const string& aTestSURString, const string& aCheatString, const int aRawCommand, bool aBadClient) {
	OnlineUser* ou = NULL;
	string report = Util::emptyString;
	{
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(p->getCID());
		if(i == onlineUsers.end()) return;
		
		ou = i->second;
		
		if(!aTestSURString.empty()) {
			ou->getIdentity().set("TS", aTestSURString);
			ou->getIdentity().set("TC", "1");
			report = ou->getIdentity().updateClientType(*ou);
		}
		if(!aCheatString.empty()) {
			report = ou->getIdentity().setCheat(ou->getClient(), aCheatString, aBadClient);
		}
		if(aRawCommand != -1)
			sendRawCommand(ou->getUser(), ou->getClient(), aRawCommand);
	}
	ou->getClient().updated(*ou);
	if(!report.empty() && BOOLSETTING(DISPLAY_CHEATS_IN_MAIN_CHAT))
		ou->getClient().cheatMessage(report);
}

int ClientManager::getMode(const string& aHubUrl) const {
	if(aHubUrl.empty()) return SETTING(INCOMING_CONNECTIONS);

	int mode = 0;
	const FavoriteHubEntry* hub = FavoriteManager::getInstance()->getFavoriteHubEntry(aHubUrl);
	if(hub) {
		switch(hub->getMode()) {
			case 1 :
				mode = SettingsManager::INCOMING_DIRECT;
				break;
			case 2 :
				mode = SettingsManager::INCOMING_FIREWALL_PASSIVE;
				break;
			default:
				mode = SETTING(INCOMING_CONNECTIONS);
		}
	} else {
		mode = SETTING(INCOMING_CONNECTIONS);
	}
	return mode;
}

/**
 * @file
 * $Id$
 */
