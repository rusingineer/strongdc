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

#if !defined(CLIENT_MANAGER_H)
#define CLIENT_MANAGER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TimerManager.h"

#include "Client.h"
#include "Singleton.h"
#include "SettingsManager.h"
#include "User.h"
#include "DirectoryListing.h"

#include "ClientManagerListener.h"

class UserCommand;

class ClientManager : public Speaker<ClientManagerListener>, 
	private ClientListener, public Singleton<ClientManager>, 
	private TimerManagerListener, private SettingsManagerListener
{
public:
	Client* getClient(const string& aHubURL);
	void putClient(Client* aClient);

	size_t getUserCount() const;
	int64_t getAvailable() const;
	StringList getHubs(const CID& cid) const;
	StringList getHubNames(const CID& cid) const;
	//StringList getNicks(const CID& cid) const;
	string getConnection(const CID& cid) const;

	bool isConnected(const string& aUrl) const;
	
	void search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken);
	void search(StringList& who, int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken);
	void infoUpdated(bool antispam);

	UserPtr getUser(const string& aNick, const string& aHubUrl) throw();
	UserPtr getUser(const CID& cid) throw();

	string findHub(const string& ipPort) const;
	const string& findHubEncoding(const string& aUrl) const;

	UserPtr findUser(const string& aNick, const string& aHubUrl) const throw() { return findUser(makeCid(aNick, aHubUrl)); }
	UserPtr findUser(const CID& cid) const throw();
	UserPtr findLegacyUser(const string& aNick) const throw();

	bool isOnline(const UserPtr& aUser) const {
		Lock l(cs);
		return onlineUsers.find(aUser->getCID()) != onlineUsers.end();
	}
	
	void setIPUser(const string& IP, const UserPtr& user) {
		Lock l(cs);
		OnlinePairC p = onlineUsers.equal_range(user->getCID());
		for (OnlineIterC i = p.first; i != p.second; i++) i->second->getIdentity().setIp(IP);
	}	
	
	const string getMyNMDCNick(const UserPtr& p) const {
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(p->getCID());
		if(i != onlineUsers.end()) {
			return i->second->getClient().getMyNick();
		}
		return Util::emptyString;
	}

	void reportUser(const UserPtr& p) {
		string nick; string report;
		Client* c;
		{
			Lock l(cs);
			OnlineIterC i = onlineUsers.find(p->getCID());
			if(i == onlineUsers.end()) return;

			nick = i->second->getIdentity().getNick();
			report = i->second->getIdentity().getReport();
			c = &i->second->getClient();
		}
		c->cheatMessage("*** Info on " + nick + " ***" + "\r\n" + report + "\r\n");
	}

	void setPkLock(const UserPtr& p, const string& aPk, const string& aLock) {
		OnlineUser* ou;
		{
			Lock l(cs);
			OnlineIterC i = onlineUsers.find(p->getCID());
			if(i == onlineUsers.end()) return;
			
			ou = i->second;
			ou->getIdentity().set("PK", aPk);
			ou->getIdentity().set("LO", aLock);
		}
		//ou->getClient().updated(*ou);
	}

	void setSupports(const UserPtr& p, const string& aSupports) {
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(p->getCID());
		if(i == onlineUsers.end()) return;
		i->second->getIdentity().set("SU", aSupports);
	}

	void setGenerator(const UserPtr& p, const string& aGenerator) {
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(p->getCID());
		if(i == onlineUsers.end()) return;
		i->second->getIdentity().set("GE", aGenerator);
	}

	void setUnknownCommand(const UserPtr& p, const string& aUnknownCommand) {
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(p->getCID());
		if(i == onlineUsers.end()) return;
		i->second->getIdentity().set("UC", aUnknownCommand);
	}

	bool isOp(const UserPtr& aUser, const string& aHubUrl) const;
	bool isStealth(const string& aHubUrl) const;

	/** Constructs a synthetic, hopefully unique CID */
	CID makeCid(const string& nick, const string& hubUrl) const throw();

	void putOnline(OnlineUser* ou) throw();
	void putOffline(OnlineUser* ou) throw();

	UserPtr& getMe();
	
	void connect(const UserPtr& p, const string& token);
	void send(AdcCommand& c, const CID& to);
	void privateMessage(const UserPtr& p, const string& msg);

	void userCommand(const UserPtr& p, const ::UserCommand& uc, StringMap& params, bool compatibility);
	void sendRawCommand(const UserPtr& user, const Client& c, const int aRawCommand);

	int getMode(const string& aHubUrl) const;
	bool isActive(const string& aHubUrl) const { return getMode(aHubUrl) != SettingsManager::INCOMING_FIREWALL_PASSIVE; }

	void lock() throw() { cs.enter(); }
	void unlock() throw() { cs.leave(); }

	const string& getHubUrl(const UserPtr& aUser) const;

	const Client::List& getClients() const { return clients; }

	string getCachedIp() const { Lock l(cs); return cachedIp; }

	CID getMyCID();
	const CID& getMyPID();
	
	// fake detection methods
	void setListLength(const UserPtr& p, const string& listLen);
	void fileListDisconnected(const UserPtr& p);
	void connectionTimeout(const UserPtr& p);
	void checkCheating(const UserPtr& p, DirectoryListing* dl);
	void setCheating(const UserPtr& p, const string& aTestSURString, const string& aCheatString, const int aRawCommand, bool aBadClient);
	void setFakeList(const UserPtr& p, const string& aCheatString);

private:
	typedef HASH_MAP_X(CID, UserPtr, CID::Hash, equal_to<CID>, less<CID>) UserMap;
	typedef UserMap::iterator UserIter;

	typedef HASH_MULTIMAP_X(CID, OnlineUser*, CID::Hash, equal_to<CID>, less<CID>) OnlineMap;
	typedef OnlineMap::iterator OnlineIter;
	typedef OnlineMap::const_iterator OnlineIterC;
	typedef pair<OnlineIter, OnlineIter> OnlinePair;
	typedef pair<OnlineIterC, OnlineIterC> OnlinePairC;

	Client::List clients;
	mutable CriticalSection cs;
	
	UserMap users;
	OnlineMap onlineUsers;

	UserPtr me;
	
	uint64_t quickTick;
	
	string cachedIp;
	CID pid;	

	friend class Singleton<ClientManager>;

	ClientManager() : quickTick(GET_TICK()) { 
		TimerManager::getInstance()->addListener(this); 
		SettingsManager::getInstance()->addListener(this);
	}

	virtual ~ClientManager() throw() { 
		SettingsManager::getInstance()->removeListener(this);
		TimerManager::getInstance()->removeListener(this); 
	}

	void updateCachedIp();

	// SettingsManagerListener
	virtual void on(Load, SimpleXML&) throw();

	// ClientListener
	virtual void on(Connected, const Client* c) throw() { fire(ClientManagerListener::ClientConnected(), c); }
	virtual void on(UserUpdated, const Client*, const OnlineUser& user) throw() { fire(ClientManagerListener::UserUpdated(), user); }
	virtual void on(UsersUpdated, const Client* c, const UserList&) throw() { fire(ClientManagerListener::ClientUpdated(), c); }
	virtual void on(Failed, const Client*, const string&) throw();
	virtual void on(HubUpdated, const Client* c) throw() { fire(ClientManagerListener::ClientUpdated(), c); }
	virtual void on(UserCommand, const Client*, int, int, const string&, const string&) throw();
	virtual void on(NmdcSearch, Client* aClient, const string& aSeeker, int aSearchType, int64_t aSize, 
		int aFileType, const string& aString, bool) throw();
	virtual void on(AdcSearch, const Client* c, const AdcCommand& adc, const CID& from) throw();
	// TimerManagerListener
	virtual void on(TimerManagerListener::Minute, uint32_t aTick) throw();
};

#endif // !defined(CLIENT_MANAGER_H)

/**
 * @file
 * $Id$
 */
