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

#if !defined(AFX_USER_H__26AA222C_500B_4AD2_A5AA_A594E1A6D639__INCLUDED_)
#define AFX_USER_H__26AA222C_500B_4AD2_A5AA_A594E1A6D639__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Util.h"
#include "Pointer.h"
#include "CriticalSection.h"
#include "CID.h"

class Client;
class FavoriteUser;

/**
 * A user connected to a hubs.
 */
class User : public PointerBase, public Flags
{
public:
	enum {
		OP_BIT,
		ONLINE_BIT,
		DCPLUSPLUS_BIT,
		PASSIVE_BIT,
		QUIT_HUB_BIT,
		HIDDEN_BIT,
		HUB_BIT,
		BOT_BIT,
		FORCEZOFF_BIT
	};

	enum {
		OP = 1<<OP_BIT,
		ONLINE = 1<<ONLINE_BIT,
		DCPLUSPLUS = 1<<DCPLUSPLUS_BIT,
		PASSIVE = 1<<PASSIVE_BIT,
		QUIT_HUB = 1<<QUIT_HUB_BIT,
		HIDDEN = 1<<HIDDEN_BIT,
		HUB = 1<<HUB_BIT,
		BOT = 1<<BOT_BIT,
		FORCEZOFF = 1<<FORCEZOFF_BIT,
	};

	typedef Pointer<User> Ptr;
	typedef vector<Ptr> List;
	typedef List::iterator Iter;
	typedef HASH_MAP<string,Ptr> NickMap;
	typedef NickMap::iterator NickIter;
	typedef HASH_MAP_X(CID, Ptr, CID::Hash, equal_to<CID>, less<CID>) CIDMap;
	typedef CIDMap::iterator CIDIter;

	struct HashFunction {
		static const size_t bucket_size = 4;
		static const size_t min_buckets = 8;
		size_t operator()(const Ptr& x) const { return ((size_t)(&(*x)))/sizeof(User); };
		bool operator()(const Ptr& a, const Ptr& b) const { return (&(*a)) < (&(*b)); };
	};

	User(const CID& aCID) : cid(aCID), bytesShared(0), client(NULL), favoriteUser(NULL) { }
	User(const string& aNick) throw() : nick(aNick), bytesShared(0), client(NULL), favoriteUser(NULL), status(0), autoextraslot(false),
			downloadSpeed(0), fileListSize(0), hasTestSURinQueue(false), fakeSharing(false), checked(false), realBytesShared(-1),
			junkBytesShared(-1), fakeShareBytesShared(-1)
			 { unCacheClientInfo(); };
	virtual ~User() throw();

	void setClient(Client* aClient);
	void connect();
	const string& getClientNick() const;
	const string& getClientName() const;
	string getClientAddressPort() const;
	void privateMessage(const string& aMsg);
	void clientMessage(const string& aMsg);
	bool isClientOp() const;
	void send(const string& msg);
	
	string getFullNick() const { 
		string tmp(getNick());
		tmp += " (";
		tmp += getClientName();
		tmp += ")";
		return tmp;
	}
	
	void setBytesShared(const string& aSharing) { setBytesShared(Util::toInt64(aSharing)); };

	bool isOnline() const { return isSet(ONLINE); };
	bool isClient(Client* aClient) const { return client == aClient; };
	
	void getParams(StringMap& ucParams);

	// favorite user stuff
	void setFavoriteUser(FavoriteUser* aUser);
	bool isFavoriteUser() const;
	void setFavoriteLastSeen(u_int32_t anOfflineTime = 0);
	u_int32_t getFavoriteLastSeen() const;
	const string& getUserDescription() const;
	void setUserDescription(const string& aDescription);
	string insertUserData(const string& s, Client* aClient = NULL);

	static void updated(User::Ptr& aUser);
	
	Client* getClient() { return client; }
	
	GETSET(string, connection, Connection);
	GETSET(int, status, Status);
	GETSET(string, nick, Nick);
	GETSET(string, email, Email);
	GETSET(string, description, Description);
	GETSET(string, tag, Tag);
	GETSET(string, version, Version);
	GETSET(string, mode, Mode);
	GETSET(string, hubs, Hubs);
	GETSET(string, slots, Slots);
	GETSET(string, upload, Upload);
	GETSET(string, lastHubAddress, LastHubAddress);
	GETSET(string, lastHubName, LastHubName);
	GETSET(string, ip, Ip);
	GETSET(CID, cid, CID);
	GETSET(string, host, Host);
	GETSET(string, supports, Supports);
	GETSET(string, lock, Lock);
	GETSET(string, pk, Pk);
	GETSET(string, clientType, ClientType);
	GETSET(string, generator, Generator);
	GETSET(int64_t, downloadSpeed, DownloadSpeed);
	GETSET(int64_t, fileListSize, FileListSize);
	GETSET(int64_t, bytesShared, BytesShared);
	GETSET(bool, autoextraslot, AutoExtraSlot);
	GETSET(string, testSUR, TestSUR);
	GETSET(bool, hasTestSURinQueue, HasTestSURinQueue);
	GETSET(int64_t, realBytesShared, RealBytesShared);
	GETSET(int64_t, junkBytesShared, JunkBytesShared);
	GETSET(int64_t, fakeShareBytesShared, FakeShareBytesShared);
	GETSET(string, cheatingString, CheatingString);
	GETSET(bool, fakeSharing, FakeSharing); 
	GETSET(bool, checked, Checked); 
	GETSET(string, unknownCommand, UnknownCommand);
	GETSET(bool, badClient, BadClient);	
	
	void TagParts( char *sTag );
	void updateClientType();
	bool matchProfile(const string& aString, const string& aProfile);
//	Client* getClient() { return client; };
	string getReport();
	string getVersion(const string& aExp, const string& aTag);
	string splitVersion(const string& aExp, const string& aTag);
//	void update();
	void sendRawCommand(const int aRawCommand);
	void unCacheClientInfo() {
		pk = Util::emptyString;
		lock = Util::emptyString;
		host = Util::emptyString;
		supports = Util::emptyString;
		clientType = Util::emptyString;
		downloadSpeed = -1;
		fileListSize = -1;
		status = 0;
		generator = Util::emptyString;
		testSUR = Util::emptyString;
		hasTestSURinQueue = false;
		unknownCommand = Util::emptyString;
		cheatingString = Util::emptyString;
		badClient = false;
	};
	// CDM EXTENSION ENDS
private:
	void updated();
	mutable RWLock cs;
	
	User(const User&);
	User& operator=(const User&);

	Client* client;
	FavoriteUser* favoriteUser;
	StringMap getPreparedFormatedStringMap(Client* aClient = NULL); 
};

#endif // !defined(AFX_USER_H__26AA222C_500B_4AD2_A5AA_A594E1A6D639__INCLUDED_)

/**
 * @file
 * $Id$
 */
