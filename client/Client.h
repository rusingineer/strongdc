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

#if !defined(AFX_Client_H__089CBD05_4833_4E30_9A57_BB636231D78E__INCLUDED_)
#define AFX_Client_H__089CBD05_4833_4E30_9A57_BB636231D78E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TimerManager.h"
#include "SettingsManager.h"

#include "BufferedSocket.h"
#include "User.h"
#include "CriticalSection.h"
// StrongDC++ "UploadManager.h"
#include "ConnectionManager.h"

class Client;

class ClientListener  
{
public:
	typedef ClientListener* Ptr;
	typedef vector<Ptr> List;
	typedef List::iterator Iter;
	
	enum Types {
		BAD_PASSWORD,
		CONNECT_TO_ME,
		CONNECTED,
		CONNECTING,
		FAILED,
		FORCE_MOVE,
		GET_PASSWORD,
		HELLO,
		HUB_NAME,
		HUB_FULL,
		SUPPORTS,
		C_LOCK,
		LOGGED_IN,
		MESSAGE,
		MY_INFO,
		NICK_LIST,
		OP_LIST,
		USER_IP,
		PRIVATE_MESSAGE,
		REV_CONNECT_TO_ME,
		SEARCH,
		QUIT,
		UNKNOWN,
		USER_COMMAND,
		VALIDATE_DENIED,
		SEARCH_FLOOD,
		CHEAT_MESSAGE
	};
	
	virtual void onAction(Types, Client*) throw() { };
	virtual void onAction(Types, Client*, const string&) throw() { };
	virtual void onAction(Types, Client*, const string&, const string&) throw() { };
	virtual void onAction(Types, Client*, const User::Ptr&) throw() { };
	virtual void onAction(Types, Client*, const User::List&) throw() { };		// USER_IP
	virtual void onAction(Types, Client*, const User::Ptr&, const string&) throw() { };
	virtual void onAction(Types, Client*, const string&, int, const string&, int, const string&) throw() { };
	virtual void onAction(Types, Client*, int, int, const string&, const string&) throw() { }; // USER_COMMAND
	virtual void onAction(Types, Client*, const StringList&) throw() { };		// SUPPORTS
};

class Client : public Speaker<ClientListener>, private BufferedSocketListener, private TimerManagerListener, private Flags
{
	friend class ClientManager;
public:
	typedef Client* Ptr;
	typedef list<Ptr> List;
	typedef List::iterator Iter;

	enum SupportFlags {
		SUPPORTS_USERCOMMAND = 0x01,
		SUPPORTS_NOGETINFO = 0x02, 
		SUPPORTS_USERIP2 = 0x04,
	};

	User::NickMap& lockUserList() throw() { cs.enter(); return users; };
	void unlockUserList() throw() { cs.leave(); };

	bool isConnected() { return socket->isConnected(); };
	void disconnect() throw();
	void myInfo();
	
	void refreshUserList(bool unknownOnly = false);

#define checkstate() if(state != STATE_CONNECTED) return

	void sendRaw(const string& aRaw) { send(aRaw); }

	void validateNick(const string& aNick) {
		if (validatenicksent != true) {
			send("$ValidateNick " + aNick + "|");
			validatenicksent = true;
		}
	}
	void key(const string& aKey) { send("$Key " + aKey + "|"); };	
	void version(const string& aVersion) { send("$Version " + aVersion + "|"); };
	void getNickList() {
		if(state == STATE_CONNECTED || state == STATE_MYINFO) {
			send("$GetNickList|");
		}
	};
	void password(const string& aPass) { send("$MyPass " + aPass + "|"); };
	void getInfo(User::Ptr aUser) { checkstate(); if(getUserInfo()) send("$GetINFO " + aUser->getNick() + " " + getNick() + "|"); };
	void getInfo(User* aUser) {  checkstate(); if(getUserInfo())send("$GetINFO " + aUser->getNick() + " " + getNick() + "|"); };
	void sendMessage(const string& aMessage) { checkstate(); send("<" + getNick() + "> " + Util::validateChatMessage(aMessage) + "|"); }
	void sendMessageRaw(const string& aMessage) { send(aMessage); }


	void search(int aSizeType, int64_t aSize, int aFileType, const string& aString);
	void searchResults(const string& aResults) { send(aResults); };
	
	void connectToMe(const User::Ptr& aUser) {
		checkstate(); 
		dcdebug("Client::connectToMe %s\n", aUser->getNick().c_str());
		send("$ConnectToMe " + aUser->getNick() + " " + getLocalIp() + ":" + Util::toString(SETTING(IN_PORT)) + "|");
		ConnectionManager::iConnToMeCount++;

	}
	void connectToMe(User* aUser) {
		checkstate(); 
		dcdebug("Client::connectToMe %s\n", aUser->getNick().c_str());
		send("$ConnectToMe " + aUser->getNick() + " " + getLocalIp() + ":" + Util::toString(SETTING(IN_PORT)) + "|");
		ConnectionManager::iConnToMeCount++;
	}
	void privateMessage(const User::Ptr& aUser, const string& aMessage) {
		privateMessage(aUser->getNick(), aMessage);
	}
	void privateMessage(User* aUser, const string& aMessage) {
		privateMessage(aUser->getNick(), aMessage);
	}
	void privateMessage(const string& aNick, const string& aMessage) {
		checkstate(); 
		send("$To: " + aNick + " From: " + getNick() + " $" + Util::validateChatMessage(aMessage) + "|");
	}
	void supports(const StringList& feat) { 
		string x;
		for(StringList::const_iterator i = feat.begin(); i != feat.end(); ++i) {
			x+= ' ' + *i;
		}
		send("$Supports" + x + '|');
	}
	void revConnectToMe(const User::Ptr& aUser) {
		checkstate(); 
		dcdebug("Client::revConnectToMe %s\n", aUser->getNick().c_str());
		send("$RevConnectToMe " + getNick() + " " + aUser->getNick()  + "|");
	}
	void revConnectToMe(User* aUser) {
		checkstate(); 
		dcdebug("Client::revConnectToMe %s\n", aUser->getNick().c_str());
		send("$RevConnectToMe " + getNick() + " " + aUser->getNick()  + "|");
	}
	void sendDebugMessage(const string& aLine);
		string getRawCommand(const int aRawCommand) {
		switch(aRawCommand) {
			case 1: return rawOne;
			case 2: return rawTwo;
			case 3: return rawThree;
			case 4: return rawFour;
			case 5: return rawFive;
		}
		return Util::emptyString;
	};
	void send(const string& a) throw() {
		lastActivity = GET_TICK();
		//dcdebug("Sending %d to %s: %.40s\n", a.size(), getName().c_str(), a.c_str());
		sendDebugMessage("  >> " + a);
		socket->write(a);
	}
	void send(const char* aBuf, int aLen) throw() {
		lastActivity = GET_TICK();
		sendDebugMessage("  >> aBuf ??");
		socket->write(aBuf, aLen);
	}

	void whoip(const string& aMsg);
	void banned(const string& aMsg);
	
	void connect(const string& aAddressPort);
	
	void updated(User::Ptr& aUser) {
		fire(ClientListener::MY_INFO, this, aUser);
	}

	const string& getName() const { return name; };
	const string& getAddress() const { return address; };
	const string& getAddressPort() const { return addressPort; };
	short getPort() const { return port; };

	const string& getIp() {	return socket->getIp().empty() ? getAddress() : socket->getIp(); };
	string getIpPort() { return port == 411 ? getIp() : getIp() + ':' + Util::toString(port); };

	int getUserCount() throw() {
		Lock l(cs);
		return users.size();
	}

	int64_t getAvailable() throw() {
		Lock l(cs);
		int64_t x = 0;
		for(User::NickIter i = users.begin(); i != users.end(); ++i) {
			x+=i->second->getBytesShared();
		}
		return x;
	}

	static string getCounts() {
		char buf[128];
		return string(buf, sprintf(buf, "%ld/%ld/%ld", counts.normal, counts.registered, counts.op));
	
	}

	static int getTotalCounts(){
		return counts.normal + counts.registered + counts.op;
		
	}

	string getLocalIp() { 
		if(!SETTING(SERVER).empty()) {
			return Socket::resolve(SETTING(SERVER));
		}
		if(me && !me->getIp().empty())
			return me->getIp();

		if(socket == NULL)
			return Util::getLocalIp();
		string tmp = socket->getLocalIp();
		if(tmp.empty())
			return Util::getLocalIp();
		return tmp;
	}

	const string& getDescription() const { return description.empty() ? SETTING(DESCRIPTION) : description; };
	void setDescription(const string& aDesc) { description = aDesc; };

	bool getOp() { return me ? me->isSet(User::OP) : false; };

	GETSET(User::Ptr, me, Me);
	GETSET(string, nick, Nick);
	GETSET(string, defpassword, Password);
	GETSET(int, supportFlags, SupportFlags);
	GETSET(bool, userInfo, UserInfo);
	GETSET(bool, registered, Registered);
	GETSET(bool, firstHello, FirstHello);
	GETSET(bool, stealth, Stealth);
	GETSET(string, rawOne, RawOne);
	GETSET(string, rawTwo, RawTwo);
	GETSET(string, rawThree, RawThree);
	GETSET(string, rawFour, RawFour);
	GETSET(string, rawFive, RawFive);
private:
	enum States {
		STATE_CONNECT,
		STATE_LOCK,
		STATE_HELLO,
		STATE_MYINFO,
		STATE_CONNECTED
	} state;

	enum {
		COUNT_UNCOUNTED,
		COUNT_NORMAL,
		COUNT_REGISTERED,
		COUNT_OP
	};

	string address;
	string addressPort;
	string lastmyinfo;
	bool validatenicksent;
	int64_t lastbytesshared;
	short port;

	string name;
	string description;

	BufferedSocket* socket;
	u_int32_t lastActivity;

	CriticalSection cs;

	User::NickMap users;

	struct Counts {
		Counts(long n = 0, long r = 0, long o = 0) : normal(n), registered(r), op(o) { };
		long normal;
		long registered;
		long op;
		bool operator !=(const Counts& rhs) { return normal != rhs.normal || registered != rhs.registered || op != rhs.op; };
	};

	static Counts counts;

	Counts lastCounts;

	int countType;
	bool reconnect;
	u_int32_t lastUpdate;
	
	char *cmd, *param, *dscrptn, *temp;
	int paramlen, dscrptnlen;

	typedef list<pair<string, u_int32_t> > FloodMap;
	typedef FloodMap::iterator FloodIter;
	FloodMap seekers;
	FloodMap flooders;

	void updateCounts(bool aRemove);

	Client();	
	virtual ~Client() throw();

	// Dummy
	Client(const Client&);
	Client& operator=(const Client&);

	void connect();

	void clearUsers();
	void onLine(const char *aLine) throw();
	
	// TimerManagerListener
	virtual void onAction(TimerManagerListener::Types type, u_int32_t aTick) throw();

	// BufferedSocketListener
	virtual void onAction(BufferedSocketListener::Types type, const string& aLine) throw();
	virtual void onAction(BufferedSocketListener::Types type) throw();

};

#endif // !defined(AFX_Client_H__089CBD05_4833_4E30_9A57_BB636231D78E__INCLUDED_)

/**
 * @file
 * $Id$
 */
