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

#ifndef DCPLUSPLUS_CLIENT_USER_H
#define DCPLUSPLUS_CLIENT_USER_H

#include "forward.h"
#include "Util.h"
#include "Pointer.h"
#include "CID.h"
#include "FastAlloc.h"
#include "CriticalSection.h"
#include "Flags.h"

/** A user connected to one or more hubs. */
class User : public FastAlloc<User>, public PointerBase, public Flags
{
public:
	/** Each flag is set if it's true in at least one hub */
	enum UserFlags {
		ONLINE		= 0x01,
		DCPLUSPLUS	= 0x02,
		PASSIVE		= 0x04,
		NMDC		= 0x08,
		BOT			= 0x10,
		TLS			= 0x20,	//< Client supports TLS
		OLD_CLIENT	= 0x40, //< Can't download - old client
		AWAY		= 0x80,
		SERVER		= 0x100,
		FIREBALL	= 0x200
	};

	struct HashFunction {
#ifdef _MSC_VER
		static const size_t bucket_size = 4;
		static const size_t min_buckets = 8;
#endif
		size_t operator()(const UserPtr& x) const { return ((size_t)(&(*x)))/sizeof(User); }
		bool operator()(const UserPtr& a, const UserPtr& b) const { return (&(*a)) < (&(*b)); }
	};

	User(const CID& aCID) : cid(aCID), lastDownloadSpeed(0) { }

	~User() throw() { }

	const CID& getCID() const { return cid; }
	operator const CID&() const { return cid; }

	bool isOnline() const { return isSet(ONLINE); }
	bool isNMDC() const { return isSet(NMDC); }

	GETSET(uint16_t, lastDownloadSpeed, LastDownloadSpeed);
	GETSET(string, firstNick, FirstNick);
private:
	User(const User&);
	User& operator=(const User&);

	CID cid;
};

/** One of possibly many identities of a user, mainly for UI purposes */
class Identity {
public:

	Identity() { }
	Identity(const UserPtr& ptr, uint32_t aSID) : user(ptr) { setSID(aSID); }
	Identity(const Identity& rhs) : user(rhs.user), info(rhs.info) { }
	Identity& operator=(const Identity& rhs) { Lock l(rhs.cs); user = rhs.user; info = rhs.info; return *this; }

#define GS(n, x) string get##n() const { return get(x); } void set##n(const string& v) { set(x, v); }
	GS(Description, "DE")
	GS(Ip, "I4")
	GS(UdpPort, "U4")
	GS(Email, "EM")
	GS(Status, "ST")

	void setNick(const string& aNick) {
		if(!user || !user->isSet(User::NMDC)) {
			set("NI", aNick);
		}
	}

	const string& getNick() const {
		if(user && user->isSet(User::NMDC)) {
			return user->getFirstNick();
		} else {
			Lock l(cs);
			InfMap::const_iterator i = info.find(*(short*)"NI");
			return i == info.end() ? Util::emptyString : i->second;
		}
	}

	void setBytesShared(const string& bs) { set("SS", bs); }
	int64_t getBytesShared() const { return Util::toInt64(get("SS")); }
	
	void setConnection(const string& name) { set("US", name); }
	string getConnection() const;

	void setOp(bool op) { set("OP", op ? "1" : Util::emptyString); }
	void setHub(bool hub) { set("HU", hub ? "1" : Util::emptyString); }
	void setBot(bool bot) { set("BO", bot ? "1" : Util::emptyString); }
	void setHidden(bool hidden) { set("HI", hidden ? "1" : Util::emptyString); }
	const string getTag() const;
	bool supports(const string& name) const;
	bool isHub() const { return !get("HU").empty(); }
	bool isOp() const { return !get("OP").empty(); }
	bool isRegistered() const { return !get("RG").empty(); }
	bool isHidden() const { return !get("HI").empty(); }
	bool isBot() const { return !get("BO").empty(); }
	bool isAway() const { return !get("AW").empty(); }
	bool isTcpActive() const { return (!user->isSet(User::NMDC) && !getIp().empty()) || !user->isSet(User::PASSIVE); }
	bool isUdpActive() const { return !getIp().empty() && !getUdpPort().empty(); }
	const string get(const char* name) const;
	void set(const char* name, const string& val);
	string getSIDString() const { uint32_t sid = getSID(); return string((const char*)&sid, 4); }
	
	uint32_t getSID() const { return Util::toUInt32(get("SI")); }
	void setSID(uint32_t sid) { if(sid != 0) set("SI", Util::toString(sid)); }
	
	const string setCheat(const Client& c, const string& aCheatDescription, bool aBadClient);
	const string getReport() const;
	const string updateClientType(const OnlineUser& ou);
	bool matchProfile(const string& aString, const string& aProfile) const;

	void getParams(StringMap& map, const string& prefix, bool compatibility) const;
	UserPtr& getUser() { return user; }
	GETSET(UserPtr, user, User);
private:
	typedef map<short, string> InfMap;
	typedef InfMap::const_iterator InfIter;
	InfMap info;
	/** @todo there are probably more threading issues here ...*/
	mutable CriticalSection cs;
	
	string getVersion(const string& aExp, const string& aTag) const;
	string splitVersion(const string& aExp, const string& aTag, const int part) const;
};

class NmdcHub;
#include "UserInfoBase.h"

enum {
	COLUMN_FIRST,
	COLUMN_NICK = COLUMN_FIRST, 
	COLUMN_SHARED, 
	COLUMN_EXACT_SHARED, 
	COLUMN_DESCRIPTION, 
	COLUMN_TAG,
	COLUMN_CONNECTION, 
	COLUMN_IP,
	COLUMN_EMAIL, 
	COLUMN_VERSION, 
	COLUMN_MODE, 
	COLUMN_HUBS, 
	COLUMN_SLOTS,
	COLUMN_LAST
};

class OnlineUser : public FastAlloc<OnlineUser>, public PointerBase, public UserInfoBase {
public:
	typedef vector<OnlineUser*> List;
	typedef List::const_iterator Iter;

	OnlineUser(const UserPtr& ptr, Client& client_, uint32_t sid_);
	~OnlineUser() { /*clearData();*/ }

	operator UserPtr&() { return getUser(); }
	operator const UserPtr&() const { return getUser(); }

	inline UserPtr& getUser() { return getIdentity().getUser(); }
	inline const UserPtr& getUser() const { return getIdentity().getUser(); }
	inline Identity& getIdentity() { return identity; }
	Client& getClient() { return client; }
	const Client& getClient() const { return client; }

	/* UserInfo */
	bool update(int sortCol);
	uint8_t imageIndex() const { return UserInfoBase::getImage(identity); }
	static int compareItems(const OnlineUser* a, const OnlineUser* b, uint8_t col);
	const string& getNick() const { return identity.getNick(); }
	bool isHidden() const { return identity.isHidden(); }
	
	void setText(const uint8_t name, const tstring& val);
	void clearData() { info.clear(); }
	
	inline const tstring& getText(const uint8_t col) const {
		InfMap::const_iterator i = info.find((uint8_t)col);
		return i == info.end() ? Util::emptyStringT : i->second;
	}

	GETSET(Identity, identity, Identity);
private:
	friend class NmdcHub;

	OnlineUser(const OnlineUser&);
	OnlineUser& operator=(const OnlineUser&);

	typedef map<uint8_t, tstring> InfMap;
	InfMap info;

	Client& client;
};

#endif // !defined(USER_H)

/**
 * @file
 * $Id$
 */
