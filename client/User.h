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

#ifndef DCPLUSPLUS_DCPP_USER_H
#define DCPLUSPLUS_DCPP_USER_H

#include "Util.h"
#include "Pointer.h"
#include "CID.h"
#include "FastAlloc.h"
#include "CriticalSection.h"
#include "Flags.h"
#include "forward.h"

namespace dcpp {

/** A user connected to one or more hubs. */
class User : public FastAlloc<User>, public intrusive_ptr_base, public Flags
{
public:
	/** Each flag is set if it's true in at least one hub */
	enum UserFlags {
		ONLINE					=  0x01,
		DCPLUSPLUS				=  0x02,
		PASSIVE					=  0x04,
		NMDC					=  0x08,
		BOT						=  0x10,
		TLS						=  0x20,	//< Client supports TLS
		OLD_CLIENT				=  0x40,	//< Can't download - old client
		NO_ADC_1_0_PROTOCOL		=  0x80,	//< Doesn't support "ADC/1.0" (dc++ <=0.703)
		NO_ADC_0_10_PROTOCOL	= 0x100,	//< Doesn't support "ADC/0.10"
		NO_ADCS_0_10_PROTOCOL	= 0x200		//< Doesn't support "ADCS/0.10"
	};

	struct Hash {
		size_t operator()(const UserPtr& x) const { return ((size_t)(&(*x)))/sizeof(User); }
	};

	User(const CID& aCID) : cid(aCID) { }

	~User() throw() { }

	const CID& getCID() const { return cid; }
	operator const CID&() const { return cid; }

	bool isOnline() const { return isSet(ONLINE); }
	bool isNMDC() const { return isSet(NMDC); }

private:
	User(const User&);
	User& operator=(const User&);

	CID cid;
};

/** One of possibly many identities of a user, mainly for UI purposes */
class Identity {
public:
	enum ClientType {
		CT_BOT = 1,
		CT_REGGED = 2,
		CT_OP = 4,
		CT_SU = 8,
		CT_OWNER = 16,
		CT_HUB = 32
	};
	
	enum StatusFlags {
		NORMAL		= 0x01,
		AWAY		= 0x02,
		SERVER		= 0x04,
		FIREBALL	= 0x08,
		TLS			= 0x10
	};
	
	Identity() { }
	Identity(const UserPtr& ptr, uint32_t aSID) : user(ptr) { setSID(aSID); }
	Identity(const Identity& rhs) { *this = rhs; } // Use operator= since we have to lock before reading...
	Identity& operator=(const Identity& rhs) { FastLock l(cs); user = rhs.user; info = rhs.info; return *this; }
	~Identity() { FastLock l(cs); }

#define GS(n, x) string get##n() const { return get(x); } void set##n(const string& v) { set(x, v); }
	GS(Nick, "NI")
	GS(Description, "DE")
	GS(Ip, "I4")
	GS(UdpPort, "U4")
	GS(Email, "EM")

	void setBytesShared(const string& bs) { set("SS", bs); }
	int64_t getBytesShared() const { return Util::toInt64(get("SS")); }
	
	void setConnection(const string& name) { set("US", name); }
	string getConnection() const;
	
	void setStatus(const string& st) { set("ST", st); }
	StatusFlags getStatus() const { return static_cast<StatusFlags>(Util::toInt(get("ST"))); }

	void setOp(bool op) { set("OP", op ? "1" : Util::emptyString); }
	void setHub(bool hub) { set("HU", hub ? "1" : Util::emptyString); }
	void setBot(bool bot) { set("BO", bot ? "1" : Util::emptyString); }
	void setHidden(bool hidden) { set("HI", hidden ? "1" : Util::emptyString); }
	string getTag() const;
	bool supports(const string& name) const;
	bool isHub() const { return isClientType(CT_HUB) || isSet("HU"); }
	bool isOp() const { return isClientType(CT_OP) || isSet("OP"); }
	bool isRegistered() const { return isClientType(CT_REGGED) || isSet("RG"); }
	bool isHidden() const { return isSet("HI"); }
	bool isBot() const { return isClientType(CT_BOT) || isSet("BO"); }
	bool isAway() const { return (getStatus() & AWAY) || isSet("AW"); }
	bool isTcpActive() const { return (!user->isSet(User::NMDC) && !getIp().empty()) || !user->isSet(User::PASSIVE); }
	bool isUdpActive() const { return !getIp().empty() && !getUdpPort().empty(); }
	string get(const char* name) const;
	void set(const char* name, const string& val);
	bool isSet(const char* name) const;	
	string getSIDString() const { uint32_t sid = getSID(); return string((const char*)&sid, 4); }
	
	uint32_t getSID() const { return Util::toUInt32(get("SI")); }
	void setSID(uint32_t sid) { if(sid != 0) set("SI", Util::toString(sid)); }
	
	bool isClientType(ClientType ct) const;
		
	string setCheat(const Client& c, const string& aCheatDescription, bool aBadClient);
	string getReport() const;
	string updateClientType(const OnlineUser& ou);
	bool matchProfile(const string& aString, const string& aProfile) const;

	static string getVersion(const string& aExp, string aTag);
	static string splitVersion(const string& aExp, string aTag, size_t part);
	
	void getParams(StringMap& map, const string& prefix, bool compatibility) const;
	UserPtr& getUser() { return user; }
	GETSET(UserPtr, user, User);
private:
	typedef std::tr1::unordered_map<short, string> InfMap;
	typedef InfMap::const_iterator InfIter;
	InfMap info;

	static FastCriticalSection cs;
};

class NmdcHub;
#include "UserInfoBase.h"

class OnlineUser : public FastAlloc<OnlineUser>, public intrusive_ptr_base, public UserInfoBase {
public:
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

	typedef vector<OnlineUser*> List;
	typedef List::const_iterator Iter;

	OnlineUser(const UserPtr& ptr, Client& client_, uint32_t sid_);
	~OnlineUser() { }

	operator UserPtr&() { return getUser(); }
	operator const UserPtr&() const { return getUser(); }

	UserPtr& getUser() { return getIdentity().getUser(); }
	const UserPtr& getUser() const { return getIdentity().getUser(); }
	Identity& getIdentity() { return identity; }
	Client& getClient() { return client; }
	const Client& getClient() const { return client; }

	/* UserInfo */
	bool update(int sortCol, const tstring& oldText = Util::emptyStringT);
	uint8_t imageIndex() const { return UserInfoBase::getImage(identity); }
	static int compareItems(const OnlineUser* a, const OnlineUser* b, uint8_t col);
	bool isHidden() const { return identity.isHidden(); }
	
	tstring getText(uint8_t col) const;

	bool isInList;
	GETSET(Identity, identity, Identity);
private:
	friend class NmdcHub;

	OnlineUser(const OnlineUser&);
	OnlineUser& operator=(const OnlineUser&);

	Client& client;
};

} // namespace dcpp

#endif // !defined(USER_H)

/**
 * @file
 * $Id$
 */
