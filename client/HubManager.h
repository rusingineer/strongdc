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

#if !defined(AFX_HUBMANAGER_H__75858D5D_F12F_40D0_B127_5DDED226C098__INCLUDED_)
#define AFX_HUBMANAGER_H__75858D5D_F12F_40D0_B127_5DDED226C098__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SettingsManager.h"

#include "CriticalSection.h"
#include "HttpConnection.h"
#include "User.h"
#include "UserCommand.h"
#include "FavoriteUser.h"
#include "Singleton.h"
class ClientProfile {
	public:
	typedef vector<ClientProfile> List;
	typedef List::iterator Iter;

	ClientProfile() : id(0), priority(0), rawToSend(0) { };

	ClientProfile(int aId, const string& aName, const string& aVersion, const string& aTag, 
		const string& aExtendedTag, const string& aLock, const string& aPk, const string& aSupports,
		const string& aTestSUR, const string& aUserConCom, const string& aStatus, const string& aCheatingDescription, 
		int aRawToSend, int aTagVersion, int aUseExtraVersion, int aCheckMismatch,
		const string& aConnection, const string& aComment) 
		throw() : id(aId), name(aName), version(aVersion), tag(aTag), extendedTag(aExtendedTag), 
		lock(aLock), pk(aPk), supports(aSupports), testSUR(aTestSUR), userConCom(aUserConCom),  status(aStatus),
		cheatingDescription(aCheatingDescription), rawToSend(aRawToSend), tagVersion(aTagVersion),
		useExtraVersion(aUseExtraVersion), checkMismatch(aCheckMismatch), 
		connection(aConnection), comment(aComment) { };

	ClientProfile(const ClientProfile& rhs) : id(rhs.id), name(rhs.name), version(rhs.version), tag(rhs.tag), 
		extendedTag(rhs.extendedTag), lock(rhs.lock), pk(rhs.pk), supports(rhs.supports), testSUR(rhs.testSUR), 
		userConCom(rhs.userConCom), status(rhs.status), cheatingDescription(rhs.cheatingDescription), rawToSend(rhs.rawToSend),
		tagVersion(rhs.tagVersion), useExtraVersion(rhs.useExtraVersion), checkMismatch(rhs.checkMismatch),
		connection(rhs.connection), comment(rhs.comment)
	{

	}

	ClientProfile& operator=(const ClientProfile& rhs) {
		id = rhs.id;
		name = rhs.name;
		version = rhs.version;
		tag = rhs.tag;
		extendedTag = rhs.extendedTag;
		lock = rhs.lock;
		pk = rhs.pk;
		supports = rhs.supports;
		testSUR = rhs.testSUR;
		userConCom = rhs.userConCom;
		status = rhs.status;
		rawToSend = rhs.rawToSend;
		cheatingDescription = rhs.cheatingDescription;
		tagVersion = rhs.tagVersion;
		useExtraVersion = rhs.useExtraVersion;
		checkMismatch = rhs.checkMismatch;
		connection = rhs.connection;
		comment = rhs.comment;
		return *this;
	}

		GETSET(int, id, Id);
		GETSET(string, name, Name);
		GETSET(string, version, Version);
		GETSET(string, tag, Tag);
		GETSET(string, extendedTag, ExtendedTag);
		GETSET(string, lock, Lock);
		GETSET(string, pk, Pk);
		GETSET(string, supports, Supports);
		GETSET(string, testSUR, TestSUR);
		GETSET(string, userConCom, UserConCom);
		GETSET(string, status, Status);
		GETSET(string, cheatingDescription, CheatingDescription);
		GETSET(string, connection, Connection);
		GETSET(string, comment, Comment);
		GETSET(int, priority, Priority);
		GETSET(int, rawToSend, RawToSend);
		GETSET(int, tagVersion, TagVersion);
		GETSET(int, useExtraVersion, UseExtraVersion);
		GETSET(int, checkMismatch, CheckMismatch);
};

class HubEntry {
public:
	typedef vector<HubEntry> List;
	typedef List::iterator Iter;
	
	// XML stuff
	HubEntry(const string& aName, const string& aServer, const string& aDescription, const string& aUsers, 
		const string& aShared, const string& aCountry, const string& aStatus, const string& aMinshare, const string& aMinslots, 
		const string& aMaxhubs, const string& aMaxusers, const string& aReliability, const string& aRating, const string& aPort) throw() : 
	name(aName), server(aServer), description(aDescription), users(aUsers), 
		shared(aShared), country(aCountry), status(aStatus), minshare(aMinshare), minslots(aMinslots), 
		maxhubs(aMaxhubs), maxusers(aMaxusers), reliability(aReliability), rating(aRating), port(aPort) { };
	HubEntry(const string& aName, const string& aServer, const string& aDescription, const string& aUsers) throw() : 
	name(aName), server(aServer), description(aDescription), users(aUsers) { };
	HubEntry() throw() { };
	HubEntry(const HubEntry& rhs) throw() : name(rhs.name), server(rhs.server), description(rhs.description), users(rhs.users), 
		shared(rhs.shared), country(rhs.country), status(rhs.status), minshare(rhs.minshare), minslots(rhs.minslots), 
		maxhubs(rhs.maxhubs), maxusers(rhs.maxusers), reliability(rhs.reliability), rating(rhs.rating), port(rhs.port) { }
	virtual ~HubEntry() throw() { };

	GETSET(string, name, Name);
	GETSET(string, server, Server);
	GETSET(string, description, Description);
	GETSET(string, users, Users);
	// XML stuff
	GETSET(string, shared, Shared);
	GETSET(string, country, Country);
	GETSET(string, status, Status);
	GETSET(string, minshare, Minshare);
	GETSET(string, minslots, Minslots);
	GETSET(string, maxhubs, Maxhubs);
	GETSET(string, maxusers, Maxusers);
	GETSET(string, reliability, Reliability);
	GETSET(string, rating, Rating);
	GETSET(string, port, Port);
};

class FavoriteHubEntry {
public:
	typedef FavoriteHubEntry* Ptr;
	typedef vector<Ptr> List;
	typedef List::iterator Iter;

	FavoriteHubEntry() throw() : connect(false), windowposx(0), windowposy(0), windowsizex(0), 
		windowsizey(0), windowtype(0), chatusersplit(0), stealth(false), userliststate(true) { };
	FavoriteHubEntry(const HubEntry& rhs) throw() : name(rhs.getName()), server(rhs.getServer()), 
		description(rhs.getDescription()), connect(false), windowposx(0), windowposy(0), windowsizex(0), 
		windowsizey(0), windowtype(0), chatusersplit(0), stealth(false), userliststate(true) { };
	FavoriteHubEntry(const FavoriteHubEntry& rhs) throw() : userdescription(rhs.userdescription), name(rhs.getName()), 
		server(rhs.getServer()), description(rhs.getDescription()), password(rhs.getPassword()), connect(rhs.getConnect()), 
		nick(rhs.nick), windowposx(rhs.windowposx), windowposy(rhs.windowposy), windowsizex(rhs.windowsizex), 
		windowsizey(rhs.windowsizey), windowtype(rhs.windowtype), chatusersplit(rhs.chatusersplit), stealth(rhs.stealth),
		userliststate(rhs.userliststate)
		, rawOne(rhs.rawOne)
		, rawTwo(rhs.rawTwo)
		, rawThree(rhs.rawThree)
		, rawFour(rhs.rawFour)
		, rawFive(rhs.rawFive)
		 { };
	~FavoriteHubEntry() throw() { };
	
	const string& getNick(bool useDefault = true) const { 
		return (!nick.empty() || !useDefault) ? nick : SETTING(NICK);
	}

	void setNick(const string& aNick) { nick = aNick; };

	GETSET(string, userdescription, UserDescription);
	GETSET(string, name, Name);
	GETSET(string, server, Server);
	GETSET(string, description, Description);
	GETSET(string, password, Password);
	GETSET(bool, connect, Connect);
	GETSET(int, windowposx, WindowPosX);
	GETSET(int, windowposy, WindowPosY);
	GETSET(int, windowsizex, WindowSizeX);
	GETSET(int, windowsizey, WindowSizeY);
	GETSET(int, windowtype, WindowType);
	GETSET(int, chatusersplit, ChatUserSplit);
	GETSET(bool, stealth, Stealth);
	GETSET(bool, userliststate, UserListState);	
	GETSET(string, rawOne, RawOne);
	GETSET(string, rawTwo, RawTwo);
	GETSET(string, rawThree, RawThree);
	GETSET(string, rawFour, RawFour);
	GETSET(string, rawFive, RawFive);
private:
	string nick;
};

class RecentHubEntry {
public:
	typedef RecentHubEntry* Ptr;
	typedef vector<Ptr> List;
	typedef List::iterator Iter;

	~RecentHubEntry() throw() { }	
	
	GETSET(string, name, Name);
	GETSET(string, server, Server);
	GETSET(string, description, Description);
	GETSET(string, users, Users);
	GETSET(string, shared, Shared);	
};


class HubManagerListener {
public:
	template<int I>	struct X { enum { TYPE = I };  };

	typedef X<0> DownloadStarting;
	typedef X<1> DownloadFailed;
	typedef X<2> DownloadFinished;
	typedef X<3> FavoriteAdded;
	typedef X<4> FavoriteRemoved;
	typedef X<5> UserAdded;
	typedef X<6> UserRemoved;
	typedef X<7> RecentAdded;
	typedef X<8> RecentRemoved;
	typedef X<9> RecentUpdated;

	virtual void on(DownloadStarting, const string&) throw() { }
	virtual void on(DownloadFailed, const string&) throw() { }
	virtual void on(DownloadFinished, const string&) throw() { }
	virtual void on(FavoriteAdded, const FavoriteHubEntry*) throw() { }
	virtual void on(FavoriteRemoved, const FavoriteHubEntry*) throw() { }
	virtual void on(UserAdded, const User::Ptr&) throw() { }
	virtual void on(UserRemoved, const User::Ptr&) throw() { }
	virtual void on(RecentAdded, const RecentHubEntry*) throw() { }
	virtual void on(RecentRemoved, const RecentHubEntry*) throw() { }
	virtual void on(RecentUpdated, const RecentHubEntry*) throw() { }
};

class SimpleXML;

/**
 * Public hub list, favorites (hub&user). Assumed to be called only by UI thread.
 */
class HubManager : public Speaker<HubManagerListener>, private HttpConnectionListener, public Singleton<HubManager>,
	private SettingsManagerListener
{
public:
	
	void refresh();

	FavoriteHubEntry::List& getFavoriteHubs() { return favoriteHubs; };

	RecentHubEntry::List& getRecentHubs() { return recentHubs; };

	User::List& getFavoriteUsers() { return users; };
	
	void addFavoriteUser(User::Ptr& aUser);
	void removeFavoriteUser(User::Ptr& aUser);

	void addFavorite(const FavoriteHubEntry& aEntry);
	void removeFavorite(FavoriteHubEntry* entry);

	FavoriteHubEntry* getFavoriteHubEntry(const string& aServer) {
		for(FavoriteHubEntry::Iter i = favoriteHubs.begin(); i != favoriteHubs.end(); ++i) {
			FavoriteHubEntry* hub = *i;
			if(Util::stricmp(hub->getServer(), aServer) == 0) {
				return hub;
			}
		}
		return NULL;
	}
	
	void addRecent(const RecentHubEntry& aEntry);
	void removeRecent(const RecentHubEntry* entry);
	void updateRecent(const RecentHubEntry* entry);

	RecentHubEntry* getRecentHubEntry(const string& aServer) {
		for(RecentHubEntry::Iter i = recentHubs.begin(); i != recentHubs.end(); ++i) {
			RecentHubEntry* r = *i;
			if(Util::stricmp(r->getServer(), aServer) == 0) {
				return r;
			}
		}
		return NULL;
	}

	ClientProfile::List& getClientProfiles() {
		Lock l(cs);
		return clientProfiles;
	}
	
	ClientProfile addClientProfile(
		const string& name, 
		const string& version, 
		const string& tag, 
		const string& extendedTag, 
		const string& lock, 
		const string& pk, 
		const string& supports, 
		const string& testSUR, 
		const string& userConCom, 
		const string& status,
		const string& cheatingdescription, 
		int rawToSend,
		int tagVersion, 
		int useExtraVersion, 
		int checkMismatch,
		const string& connection,
		const string& comment
		) 
	{
		Lock l(cs);
		clientProfiles.push_back(
			ClientProfile(
			lastProfile++, 
			name, 
			version, 
			tag, 
			extendedTag, 
			lock, 
			pk, 
			supports, 
			testSUR, 
			userConCom, 
			status, 
			cheatingdescription,
			rawToSend, 
			tagVersion, 
			useExtraVersion, 
			checkMismatch,
			connection,
			comment
			)
		);
		return clientProfiles.back();
	}

	void addClientProfile( const StringList& sl ) {
		Lock l(cs);
		clientProfiles.push_back(
			ClientProfile(
			lastProfile++, 
			sl[0], 
			sl[1], 
			sl[2], 
			sl[3], 
			sl[4], 
			sl[5], 
			sl[6], 
			sl[7], 
			sl[8], 
			sl[9], 
			"", 
			0, 
			0, 
			0, 
			0
			// FIXME
			, "", ""
			)
		);
		save();
	}

	int getProfileListSize() {
		return clientProfiles.size();
	}

	bool getClientProfile(int id, ClientProfile& cp) {
		Lock l(cs);
		for(ClientProfile::Iter i = clientProfiles.begin(); i != clientProfiles.end(); ++i) {
			if(i->getId() == id) {
				cp = *i;
				return true;
			}
		}
		return false;
	}

	bool getClientProfileByPosition(int pos, ClientProfile& cp) {
		Lock l(cs);
		for(ClientProfile::Iter i = clientProfiles.begin(); i != clientProfiles.end(); ++i) {
			if(i->getPriority() == pos) {
				cp = *i;
				return true;
			}
		}
		return false;
	}

	void removeClientProfile(int id) {
		Lock l(cs);
		for(ClientProfile::Iter i = clientProfiles.begin(); i != clientProfiles.end(); ++i) {
			if(i->getId() == id) {
				clientProfiles.erase(i);
				break;
			}
		}
	}

	void updateClientProfile(const ClientProfile& cp) {
		Lock l(cs);
		for(ClientProfile::Iter i = clientProfiles.begin(); i != clientProfiles.end(); ++i) {
			if(i->getId() == cp.getId()) {
				*i = cp;
				break;
			}
		}
	}

	bool moveClientProfile(int id, int pos, int position) {
		dcassert(pos == -1 || pos == 1);
		Lock l(cs);
		for(ClientProfile::Iter i = clientProfiles.begin(); i != clientProfiles.end(); ++i) {
			if(i->getId() == id) {
				swap(*i, *(i + pos));
				return true;
			}
		}
		return false;
	}

	void loadClientProfiles();

	ClientProfile::List& reloadClientProfiles() { 
		Lock l(cs);
		clientProfiles.clear();
		loadClientProfiles();
		return clientProfiles;
	}
	ClientProfile::List& reloadClientProfilesFromHttp() { 
		Lock l(cs);
		ClientProfile::List oldProfiles = clientProfiles;
		clientProfiles.clear();
		loadClientProfiles();
		for(ClientProfile::Iter j = clientProfiles.begin(); j != clientProfiles.end(); ++j) {
			for(ClientProfile::Iter k = oldProfiles.begin(); k != oldProfiles.end(); ++k) {
				if((*k).getName().compare((*j).getName()) == 0) {
					(*j).setRawToSend((*k).getRawToSend());
					(*j).setCheatingDescription((*k).getCheatingDescription());
				}
			}
		}
		return clientProfiles;
	}

	void removeallRecent() {
		recentHubs.clear();
		recentsave();
	}

	HubEntry::List getPublicHubs() {
		Lock l(cs);
		return publicHubs;
	}

	UserCommand addUserCommand(int type, int ctx, int flags, const string& name, const string& command, const string& hub) {
		// No dupes, add it...
		Lock l(cs);
		userCommands.push_back(UserCommand(lastId++, type, ctx, flags, name, command, hub));
		UserCommand& uc = userCommands.back();
		if(!uc.isSet(UserCommand::FLAG_NOSAVE)) 
			save();
		return userCommands.back();
	}

	bool getUserCommand(int id, UserCommand& uc) {
		Lock l(cs);
		for(UserCommand::Iter i = userCommands.begin(); i != userCommands.end(); ++i) {
			if(i->getId() == id) {
				uc = *i;
				return true;
			}
		}
		return false;
	}

	bool moveUserCommand(int id, int pos) {
		dcassert(pos == -1 || pos == 1);
		Lock l(cs);
		for(UserCommand::Iter i = userCommands.begin(); i != userCommands.end(); ++i) {
			if(i->getId() == id) {
				swap(*i, *(i + pos));
				return true;
			}
		}
		return false;
	}

	void updateUserCommand(const UserCommand& uc) {
		bool nosave = true;
		Lock l(cs);
		for(UserCommand::Iter i = userCommands.begin(); i != userCommands.end(); ++i) {
			if(i->getId() == uc.getId()) {
				*i = uc;
				nosave = uc.isSet(UserCommand::FLAG_NOSAVE);
				break;
			}
		}
		if(!nosave)
			save();
	}

	void removeUserCommnad(int id) {
		bool nosave = true;
		Lock l(cs);
		for(UserCommand::Iter i = userCommands.begin(); i != userCommands.end(); ++i) {
			if(i->getId() == id) {
				nosave = i->isSet(UserCommand::FLAG_NOSAVE);
				userCommands.erase(i);
				break;
			}
	}
		if(!nosave)
			save();
	}
	void removeUserCommand(const string& srv) {
		Lock l(cs);
		for(UserCommand::Iter i = userCommands.begin(); i != userCommands.end(); ) {
			if((i->getHub() == srv) && i->isSet(UserCommand::FLAG_NOSAVE)) {
				i = userCommands.erase(i);
			} else {
				++i;
			}
		}
	}

	void removeHubUserCommands(int ctx, const string& hub) {
		Lock l(cs);
		for(UserCommand::Iter i = userCommands.begin(); i != userCommands.end(); ) {
			if(i->getHub() == hub && i->isSet(UserCommand::FLAG_NOSAVE) && i->getCtx() & ctx) {
				i = userCommands.erase(i);
			} else {
				++i;
			}
		}
	}

	UserCommand::List getUserCommands() { Lock l(cs); return userCommands; };
	UserCommand::List getUserCommands(int ctx, const string& hub, bool op);

	bool isDownloading() { return running; };

	void load();
	void save();
	void recentsave();
	void saveClientProfiles();
private:
	
	enum {
		TYPE_NORMAL,
		TYPE_BZIP2,
		// XML addition
		TYPE_XML,
		TYPE_XMLBZIP2
	} listType;

	HubEntry::List publicHubs;
	FavoriteHubEntry::List favoriteHubs;
	RecentHubEntry::List recentHubs;
	UserCommand::List userCommands;
	User::List users;
	ClientProfile::List clientProfiles;
	int lastProfile;

	RWLock rwcs;
	CriticalSection cs;
	bool running;
	HttpConnection* c;
	int lastServer;
	int lastId;

	/** Used during loading to prevent saving. */
	bool dontSave;

	friend class Singleton<HubManager>;
	
	HubManager() : running(false), c(NULL), lastServer(0), lastId(0), dontSave(false), lastProfile(0) {
		SettingsManager::getInstance()->addListener(this);
	}

	virtual ~HubManager() {
		SettingsManager::getInstance()->addListener(this);
		if(c) {
			c->removeListener(this);
			delete c;
			c = NULL;
		}
		
		for_each(favoriteHubs.begin(), favoriteHubs.end(), DeleteFunction<FavoriteHubEntry*>());
		for_each(recentHubs.begin(), recentHubs.end(), DeleteFunction<RecentHubEntry*>());
	}
	
	string downloadBuf;
	
	FavoriteHubEntry::Iter getFavoriteHub(const string& aServer) {
		for(FavoriteHubEntry::Iter i = favoriteHubs.begin(); i != favoriteHubs.end(); ++i) {
			if(Util::stricmp((*i)->getServer(), aServer) == 0) {
				return i;
			}
		}
		return favoriteHubs.end();
	}

	RecentHubEntry::Iter getRecentHub(const string& aServer) {
		for(RecentHubEntry::Iter i = recentHubs.begin(); i != recentHubs.end(); ++i) {
			if(Util::stricmp((*i)->getServer(), aServer) == 0) {
				return i;
			}
		}
		return recentHubs.end();
	}
	// HttpConnectionListener
	virtual void on(Data, HttpConnection*, const u_int8_t*, size_t) throw();
	virtual void on(Failed, HttpConnection*, const string&) throw();
	virtual void on(Complete, HttpConnection*, const string&) throw();
	virtual void on(Redirected, HttpConnection*, const string&) throw();
	virtual void on(TypeNormal, HttpConnection*) throw();
	virtual void on(TypeBZ2, HttpConnection*) throw();
	virtual void on(TypeXML, HttpConnection*) throw();
	virtual void on(TypeXMLBZ2, HttpConnection*) throw();
	
 	void onHttpFinished() throw();

	// SettingsManagerListener
	virtual void on(SettingsManagerListener::Load, SimpleXML* xml) throw() {
		load(xml);
		recentload(xml);
	}

	void load(SimpleXML* aXml);
	void recentload(SimpleXML* aXml);
	void loadClientProfiles(SimpleXML* aXml);
};

#endif // !defined(AFX_HUBMANAGER_H__75858D5D_F12F_40D0_B127_5DDED226C098__INCLUDED_)

/**
 * @file
 * $Id$
 */
