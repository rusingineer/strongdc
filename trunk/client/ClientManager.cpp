/*
 * Copyright (C) 2001-2008 Jacek Sieka, arnetheduck on gmail point com
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
#include "ConnectionManager.h"
#include "CryptoManager.h"
#include "FavoriteManager.h"
#include "SimpleXML.h"
#include "UserCommand.h"
#include "ResourceManager.h"
#include "LogManager.h"
#include "SearchResult.h"

#include "AdcHub.h"
#include "NmdcHub.h"

#include "QueueManager.h"
#include "FinishedManager.h"

namespace dcpp {

Client* ClientManager::getClient(const string& aHubURL) {
	Client* c;
	if(strnicmp("adc://", aHubURL.c_str(), 6) == 0) {
		c = new AdcHub(aHubURL, false);
	} else if(strnicmp("adcs://", aHubURL.c_str(), 7) == 0) {
		c = new AdcHub(aHubURL, true);
	} else if(strnicmp("nmdcs://", aHubURL.c_str(), 8) == 0) {
		c = new NmdcHub(aHubURL, true);
	} else {
		c = new NmdcHub(aHubURL, false);
	}

	{
		Lock l(cs);
		clients.insert(make_pair(const_cast<string*>(&c->getHubUrl()), c));
	}

	c->addListener(this);

	return c;
}

void ClientManager::putClient(Client* aClient) {
	fire(ClientManagerListener::ClientDisconnected(), aClient);
	aClient->removeListeners();

	{
		Lock l(cs);
		clients.erase(const_cast<string*>(&aClient->getHubUrl()));
	}
	aClient->shutdown();
	delete aClient;
}

StringList ClientManager::getHubs(const CID& cid) const {
	Lock l(cs);
	StringList lst;
	OnlinePairC op = onlineUsers.equal_range(const_cast<CID*>(&cid));
	for(OnlineIterC i = op.first; i != op.second; ++i) {
		lst.push_back(i->second->getClient().getHubUrl());
	}
	return lst;
}

StringList ClientManager::getHubNames(const CID& cid) const {
	Lock l(cs);
	StringList lst;
	OnlinePairC op = onlineUsers.equal_range(const_cast<CID*>(&cid));
	for(OnlineIterC i = op.first; i != op.second; ++i) {
		lst.push_back(i->second->getClient().getHubName());
	}
	return lst;
}

StringList ClientManager::getNicks(const CID& cid) const {
	Lock l(cs);
	StringSet ret;
	OnlinePairC op = onlineUsers.equal_range(const_cast<CID*>(&cid));
	for(OnlineIterC i = op.first; i != op.second; ++i) {
		ret.insert(i->second->getIdentity().getNick());
	}
	if(ret.empty()) {
		NickMap::const_iterator i = nicks.find(const_cast<CID*>(&cid));
		if(i != nicks.end()) {
			ret.insert(i->second);
		} else {
			// Offline perhaps?
			ret.insert('{' + cid.toBase32() + '}');
		}
	}
	return StringList(ret.begin(), ret.end());
}

string ClientManager::getConnection(const CID& cid) const {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&cid));
	if(i != onlineUsers.end()) {
		return i->second->getIdentity().getConnection();
	}
	return STRING(OFFLINE);
}

bool ClientManager::isConnected(const string& aUrl) const {
	Lock l(cs);

	Client::Iter i = clients.find(const_cast<string*>(&aUrl));
	return i != clients.end();
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
	for(Client::Iter i = clients.begin(); i != clients.end(); ++i) {
		const Client* c = i->second;
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

	Client::Iter i = clients.find(const_cast<string*>(&aUrl));
	if(i != clients.end()) {
		return *(i->second->getEncoding());
	}
	return Text::systemCharset;
}

UserPtr ClientManager::findLegacyUser(const string& aNick) const throw() {
	Lock l(cs);
	dcassert(aNick.size() > 0);

	// this be slower now, but it's not called so often
	for(NickMap::const_iterator i = nicks.begin(); i != nicks.end(); ++i) {
		if(stricmp(i->second, aNick) == 0) {
			UserMap::const_iterator u = users.find(i->first);
			if(u != users.end() && u->second->getCID() == *i->first)
				return u->second;
		}
	}
	return UserPtr();
}

UserPtr ClientManager::getUser(const string& aNick, const string& aHubUrl) throw() {
	CID cid = makeCid(aNick, aHubUrl);
	Lock l(cs);

	UserMap::const_iterator ui = users.find(const_cast<CID*>(&cid));
	if(ui != users.end()) {
		ui->second->setFlag(User::NMDC);
		return ui->second;
	}

	UserPtr p(new User(cid));
	p->setFlag(User::NMDC);
	users.insert(make_pair(const_cast<CID*>(&p->getCID()), p));

	return p;
}

UserPtr ClientManager::getUser(const CID& cid) throw() {
	Lock l(cs);
	UserMap::const_iterator ui = users.find(const_cast<CID*>(&cid));
	if(ui != users.end()) {
		return ui->second;
	}

	UserPtr p(new User(cid));
	users.insert(make_pair(const_cast<CID*>(&p->getCID()), p));
	return p;
}

UserPtr ClientManager::findUser(const CID& cid) const throw() {
	Lock l(cs);
	UserMap::const_iterator ui = users.find(const_cast<CID*>(&cid));
	if(ui != users.end()) {
		return ui->second;
	}
	return 0;
}

// deprecated
bool ClientManager::isOp(const UserPtr& user, const string& aHubUrl) const {
	Lock l(cs);
	OnlinePairC p = onlineUsers.equal_range(const_cast<CID*>(&user->getCID()));
	for(OnlineIterC i = p.first; i != p.second; ++i) {
		if(i->second->getClient().getHubUrl() == aHubUrl) {
			return i->second->getIdentity().isOp();
		}
	}
	return false;
}

bool ClientManager::isStealth(const string& aHubUrl) const {
	Lock l(cs);
	Client::Iter i = clients.find(const_cast<string*>(&aHubUrl));
	if(i != clients.end()) {
		return i->second->getStealth();
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
		onlineUsers.insert(make_pair(const_cast<CID*>(&ou->getUser()->getCID()), ou));
	}
	
	if(!ou->getUser()->isOnline()) {
		ou->getUser()->setFlag(User::ONLINE);
		fire(ClientManagerListener::UserConnected(), ou->getUser());
	}
}

void ClientManager::putOffline(OnlineUser* ou, bool disconnect) throw() {
	bool lastUser = false;
	{
		Lock l(cs);
		OnlinePair op = onlineUsers.equal_range(const_cast<CID*>(&ou->getUser()->getCID()));
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
		UserPtr& u = ou->getUser();
		u->unsetFlag(User::ONLINE);
		updateNick(*ou);
		if(disconnect)
			ConnectionManager::getInstance()->disconnect(u);
		fire(ClientManagerListener::UserDisconnected(), u);
	}
}

void ClientManager::connect(const UserPtr& p, const string& token) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
	if(i != onlineUsers.end()) {
		OnlineUser* u = i->second;
		u->getClient().connect(*u, token);
	}
}

OnlineUserPtr ClientManager::findOnlineUser(const CID& cid, const Client* client) const throw(){
	Lock l(cs);
	OnlinePairC op = onlineUsers.equal_range(const_cast<CID*>(&cid));
	for(OnlineIterC i = op.first; i != op.second; ++i) {
		const OnlineUserPtr& ou = i->second;
		if(!client || &ou->getClient() == client)
			return ou;
	}
	
	return NULL;
}

void ClientManager::send(AdcCommand& cmd, const CID& cid) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&cid));
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
		Client* c = i->second;
		if(c->isConnected()) {
			c->info(false);
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

	SearchResultList l;
	ShareManager::getInstance()->search(l, aString, aSearchType, aSize, aFileType, aClient, isPassive ? 5 : 10);
	if(l.size() > 0) {
		if(isPassive) {
			string name = aSeeker.substr(4);
			// Good, we have a passive seeker, those are easier...
			string str;
			for(SearchResultList::const_iterator i = l.begin(); i != l.end(); ++i) {
				const SearchResultPtr& sr = *i;
				str += sr->toSR(*aClient);
				str[str.length()-1] = 5;
				str += Text::fromUtf8(name, *(aClient->getEncoding()));
				str += '|';
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
				for(SearchResultList::const_iterator i = l.begin(); i != l.end(); ++i) {
					const SearchResultPtr& sr = *i;
					udp.writeTo(ip, port, sr->toSR(*aClient));
				}
			} catch(...) {
				dcdebug("Search caught error\n");
			}
		}
	} else if(!isPassive && (aFileType == SearchManager::TYPE_TTH) && (aString.compare(0, 4, "TTH:") == 0)) {
		PartsInfo partialInfo;
		TTHValue aTTH(aString.substr(4));
		if(!QueueManager::getInstance()->handlePartialSearch(aTTH, partialInfo)) {
			// if not found, try to find in finished list
			if(!FinishedManager::getInstance()->handlePartialRequest(aTTH, partialInfo)) {
				return;
			}
		}
		
		string ip, file;
		uint16_t port = 0;
		Util::decodeUrl(aSeeker, ip, port, file);
		
		try {
			AdcCommand cmd = SearchManager::getInstance()->toPSR(true, aClient->getMyNick(), aClient->getIpPort(), aTTH.toBase32(), partialInfo);
			Socket s;
			s.writeTo(Socket::resolve(ip), port, cmd.toString(ClientManager::getInstance()->getMyCID()));
		} catch(...) {
			dcdebug("Partial search caught error\n");		
		}
	}
}

void ClientManager::userCommand(const UserPtr& p, const UserCommand& uc, StringMap& params, bool compatibility) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
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

		UserCommand uc = UserCommand(0, 0, 0, 0, "", rawCommand, "");
		userCommand(user, uc, ucParams, true);
	}
}

void ClientManager::on(AdcSearch, const Client*, const AdcCommand& adc, const CID& from) throw() {
	SearchManager::getInstance()->respond(adc, from);
}

void ClientManager::search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, void* aOwner) {
	Lock l(cs);

	for(Client::Iter i = clients.begin(); i != clients.end(); ++i) {
		Client* c = i->second;
		if(c->isConnected()) {
			c->search(aSizeMode, aSize, aFileType, aString, aToken, aOwner);
		}
	}
}

uint64_t ClientManager::search(StringList& who, int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, void* aOwner) {
	Lock l(cs);

	uint64_t estimateSearchSpan = 0;
	
	for(StringIter it = who.begin(); it != who.end(); ++it) {
		const string& client = *it;
		
		Client::Iter i = clients.find(const_cast<string*>(&client));
		if(i != clients.end() && i->second->isConnected()) {
			uint64_t ret = i->second->search(aSizeMode, aSize, aFileType, aString, aToken, aOwner);
			estimateSearchSpan = max(estimateSearchSpan, ret);			
		}
	}
	
	return estimateSearchSpan;
}

void ClientManager::on(TimerManagerListener::Minute, uint64_t /*aTick*/) throw() {
	Lock l(cs);

	// Collect some garbage...
	UserIter i = users.begin();
	while(i != users.end()) {
		if(i->second->unique()) {
			NickMap::iterator n = nicks.find(const_cast<CID*>(&i->second->getCID()));
			if(n != nicks.end()) nicks.erase(n);
			users.erase(i++);
		} else {
			++i;
		}
	}

	for(Client::Iter j = clients.begin(); j != clients.end(); ++j) {
		j->second->info(false);
	}
}

UserPtr& ClientManager::getMe() {
	if(!me) {
		Lock l(cs);
		if(!me) {
			me = new User(getMyCID());
			users.insert(make_pair(const_cast<CID*>(&me->getCID()), me));
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

void ClientManager::updateNick(const OnlineUser& user) throw() {
	updateNick(user.getUser(), user.getIdentity().getNick());
}

void ClientManager::updateNick(const UserPtr& user, const string& nick) throw() {
	Lock l(cs);
	if(nicks.find(const_cast<CID*>(&user->getCID())) != nicks.end()) {
		return;
	}
	
	if(!nick.empty()) {
		nicks.insert(std::make_pair(const_cast<CID*>(&user->getCID()), nick));
	}
}

string ClientManager::getMyNick(const string& hubUrl) const {
	Lock l(cs);
	Client::Iter i = clients.find(const_cast<string*>(&hubUrl));
	if(i != clients.end()) {
		return i->second->getMyIdentity().getNick();
	}
	return Util::emptyString;
}
	
void ClientManager::on(Connected, const Client* c) throw() {
	fire(ClientManagerListener::ClientConnected(), c);
}

void ClientManager::on(UserUpdated, const Client*, const OnlineUserPtr& user) throw() {
	fire(ClientManagerListener::UserUpdated(), *user);
}

void ClientManager::on(UsersUpdated, const Client*, const OnlineUserList& l) throw() {
	for(OnlineUserList::const_iterator i = l.begin(), iend = l.end(); i != iend; ++i) {
		updateNick(*(*i));
		fire(ClientManagerListener::UserUpdated(), *(*i)); 
	}
}

void ClientManager::on(HubUpdated, const Client* c) throw() {
	fire(ClientManagerListener::ClientUpdated(), c);
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
			FavoriteManager::getInstance()->addUserCommand(aType, ctx, UserCommand::FLAG_NOSAVE, name, command, client->getHubUrl());
		}
	}
}

void ClientManager::setListLength(const UserPtr& p, const string& listLen) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
	if(i != onlineUsers.end()) {
		i->second->getIdentity().set("LL", listLen);
	}
}

void ClientManager::fileListDisconnected(const UserPtr& p) {
	string report = Util::emptyString;
	Client* c = NULL;
	{
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
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
		OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
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

		OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
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
	ou->getClient().updated(ou);
	if(!report.empty() && BOOLSETTING(DISPLAY_CHEATS_IN_MAIN_CHAT))
		ou->getClient().cheatMessage(report);
}

void ClientManager::setCheating(const UserPtr& p, const string& aTestSURString, const string& aCheatString, const int aRawCommand, bool aBadClient) {
	OnlineUser* ou = NULL;
	string report = Util::emptyString;
	{
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
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
	ou->getClient().updated(ou);
	if(!report.empty() && BOOLSETTING(DISPLAY_CHEATS_IN_MAIN_CHAT))
		ou->getClient().cheatMessage(report);
}

int ClientManager::getMode(const string& aHubUrl) const {
	
	if(aHubUrl.empty()) 
		return SETTING(INCOMING_CONNECTIONS);

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

void ClientManager::cancelSearch(void* aOwner) {
	Lock l(cs);

	for(Client::Iter i = clients.begin(); i != clients.end(); ++i) {
		i->second->cancelSearch(aOwner);
	}
}

void ClientManager::setPkLock(const UserPtr& p, const string& aPk, const string& aLock) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
	if(i == onlineUsers.end()) return;
	
	i->second->getIdentity().set("PK", aPk);
	i->second->getIdentity().set("LO", aLock);
}

void ClientManager::setSupports(const UserPtr& p, const string& aSupports) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
	if(i == onlineUsers.end()) return;
	
	i->second->getIdentity().set("SU", aSupports);
}

void ClientManager::setGenerator(const UserPtr& p, const string& aGenerator) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
	if(i == onlineUsers.end()) return;
	i->second->getIdentity().set("GE", aGenerator);
}

void ClientManager::setUnknownCommand(const UserPtr& p, const string& aUnknownCommand) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
	if(i == onlineUsers.end()) return;
	i->second->getIdentity().set("UC", aUnknownCommand);
}

} // namespace dcpp

/**
 * @file
 * $Id$
 */
