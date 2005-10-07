/*
 * Copyright (C) 2001-2005 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(SEARCH_MANAGER_H)
#define SEARCH_MANAGER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SettingsManager.h"

#include "Socket.h"
#include "User.h"
#include "Thread.h"
#include "Client.h"
#include "Singleton.h"
#include "FastAlloc.h"
#include "MerkleTree.h"

#include "SearchManagerListener.h"
#include "TimerManager.h"
#include "AdcCommand.h"
#include "ClientManager.h"
#include "../pme-1.0.4/pme.h"

class SearchManager;

class SearchResult : public FastAlloc<SearchResult> {
public:	

	enum Types {
		TYPE_FILE,
		TYPE_DIRECTORY
	};

	typedef SearchResult* Ptr;
	typedef vector<Ptr> List;
	typedef List::iterator Iter;
	
	SearchResult(Client* aClient, Types aType, int64_t aSize, const string& name, const TTHValue* aTTH, bool aUtf8);
	SearchResult(Types aType, int64_t aSize, const string& name, const TTHValue* aTTH);

	SearchResult(const User::Ptr& aUser, Types aType, int aSlots, int aFreeSlots, 
		int64_t aSize, const string& aFile, const string& aHubName, 
		const string& aHubIpPort, const string& aIp, bool aUtf8) : 
	file(aFile), hubName(isTTH(aHubName) ? Util::emptyString : aHubName), hubIpPort(aHubIpPort), user(aUser), 
		size(aSize), type(aType), slots(aSlots), freeSlots(aFreeSlots), IP(aIp), 
		tth(isTTH(aHubName) ? new TTHValue(aHubName.substr(4)) : NULL), utf8(aUtf8), ref(1) { }

	SearchResult(const User::Ptr& aUser, Types aType, int aSlots, int aFreeSlots, 
		int64_t aSize, const string& aFile, const string& aHubName, 
		const string& aHubIpPort, TTHValue* aTTH, bool aUtf8) :
	file(aFile), hubName(aHubName), hubIpPort(aHubIpPort), user(aUser), 
		size(aSize), type(aType), slots(aSlots), freeSlots(aFreeSlots), 
		tth((aTTH != NULL) ? new TTHValue(*aTTH) : NULL), utf8(aUtf8), ref(1) { }

	string getFileName() const;
	string toSR(const Client& client) const;
	AdcCommand toRES(char type) const;

	User::Ptr& getUser() { return user; }
	string getSlotString() const { return Util::toString(getFreeSlots()) + '/' + Util::toString(getSlots()); }

	const string& getFile() const { return file; }
	const string& getHubIpPort() const { return hubIpPort; }
	/** @todo Return a hub where the user is online? */
	const string& getHubName() const { return hubName.empty() ? user->getClientName() : hubName; }
	int64_t getSize() const { return size; }
	Types getType() const { return type; }
	int getSlots() const { return slots; }
	int getFreeSlots() const { return freeSlots; }
	const string& getIP() const { return IP; }
	TTHValue* getTTH() const { return tth; }
	bool getUtf8() const { return utf8; }

	void incRef() { Thread::safeInc(ref); }
	void decRef() { 
		if(Thread::safeDec(ref) == 0) 
			delete this; 
	};

private:
	friend class SearchManager;

	SearchResult();
	~SearchResult() { delete tth; };

	SearchResult(const SearchResult& rhs);

	string file;
	string hubName;
	string hubIpPort;
	User::Ptr user;
	int64_t size;
	Types type;
	int slots;
	int freeSlots;
	string IP;
	TTHValue* tth;
	
	bool utf8;
	volatile long ref;

	bool isTTH(const string& str) const {
		return str.compare(0, 4, "TTH:") == 0;
	}
};

class SearchQueueItem {
public:
	SearchQueueItem() { }
	SearchQueueItem(int aSizeMode, int64_t aSize, int aFileType, const string& aString, int *aWindow, tstring aSearch, const string& aToken) : 
	  target(aString), size(aSize), typeMode(aFileType), sizeMode(aSizeMode), window(aWindow), search(aSearch), token(aToken) { }
	SearchQueueItem(StringList& who, int aSizeMode, int64_t aSize, int aFileType, const string& aString, int *aWindow, tstring aSearch, const string& aToken) : 
	  hubs(who), target(aString), size(aSize), typeMode(aFileType), sizeMode(aSizeMode), window(aWindow), search(aSearch), token(aToken) { }

	GETSET(string, target, Target);
	GETSET(tstring, search, Search);
	GETSET(string, token, Token);
	GETSET(int64_t, size, Size);
	GETSET(int, typeMode, TypeMode);
	GETSET(int, sizeMode, SizeMode);
	GETSET(int*, window, Window);
	StringList& getHubs() { return hubs; };
	void setHubs(StringList aHubs) { hubs = aHubs; };

private:
	StringList hubs;
};



class SearchManager : public Speaker<SearchManagerListener>, private TimerManagerListener, public Singleton<SearchManager>, public Thread
{
	class ResultsQueue: public Thread
	{
	public:
		bool stop;
		CriticalSection cs;
		Semaphore s;
		SearchResult::List resultList;
		u_int32_t openedSearchFrms;

		ResultsQueue() : stop(false), openedSearchFrms(0) {}
		virtual ~ResultsQueue() throw() {
			shutdown();
		}

		int run() {
			setThreadPriority(Thread::IDLE);
			SearchResult* sr = NULL;
			bool resort = false;
			stop = false;

			for(;;) {
				s.wait();
				if(stop)
					break;

				{
					Lock l(cs);
					if(!resultList.empty()) {
						sr = *resultList.begin();
						resultList.erase(resultList.begin());
						int size = resultList.size();
						resort = (size % 20 == 0);
					} else {
						sr = NULL;
					}
				}

				if(sr != NULL) {
					OnlineUser* ou = sr->getUser()->getOnlineUser();
					if(ou) {
						if (!sr->getIP().empty())
							ou->getIdentity().setIp(sr->getIP());

						if((ou->getIdentity().get("VE") == "++ 0.403") && (ou->getClientType() != "rmDC++ 0.403D[1]")) {
							string path(sr->getFile());
							path = path.substr(0, path.find("\\"));
							PME reg("([A-Z])");
							if(reg.match(path)) {
								ou->setCheat("rmDC++ 0.403D[1] with DC++ emulation" , true);
								ou->setClientType("rmDC++ 0.403D[1]");
								ou->setBadClient(true);
							}
						}
					}

					SearchManager::getInstance()->fire(SearchManagerListener::SR(), sr);
					sr->decRef();
					if(resort)
						SearchManager::getInstance()->fire(SearchManagerListener::Resort());
				}
				sleep((openedSearchFrms != 0) ? 10 : 100);
				resort = false;
			}
			return 0;
		}

		void shutdown() {
			stop = true;
			s.signal();
		}
		void addResult(SearchResult* sr) {
			Lock l(cs);
			resultList.push_back(sr);
		}
	
	};
public:
	typedef list<SearchQueueItem> SearchQueueItemList;
	typedef SearchQueueItemList::iterator SearchQueueIter;

	enum SizeModes {
		SIZE_DONTCARE = 0x00,
		SIZE_ATLEAST = 0x01,
		SIZE_ATMOST = 0x02,
		SIZE_EXACT = 0x03
	};

	enum TypeModes {
		TYPE_ANY = 0,
		TYPE_AUDIO,
		TYPE_COMPRESSED,
		TYPE_DOCUMENT,
		TYPE_EXECUTABLE,
		TYPE_PICTURE,
		TYPE_VIDEO,
		TYPE_DIRECTORY,
		TYPE_TTH
	};
	
	void search(const string& aName, int64_t aSize, TypeModes aTypeMode, SizeModes aSizeMode, const string& aToken, int *aWindow = NULL, tstring aSearch = _T(""));
	void search(const string& aName, const string& aSize, TypeModes aTypeMode, SizeModes aSizeMode, const string& aToken, int *aWindow = NULL, tstring aSearch = _T("")) {
		search(aName, Util::toInt64(aSize), aTypeMode, aSizeMode, aToken, aWindow, aSearch);
	}

	void search(StringList& who, const string& aName, int64_t aSize, TypeModes aTypeMode, SizeModes aSizeMode, const string& aToken, int *aWindow = NULL, tstring aSearch = _T(""));
	void search(StringList& who, const string& aName, const string& aSize, TypeModes aTypeMode, SizeModes aSizeMode, const string& aToken, int *aWindow = NULL, tstring aSearch = _T("")) {
		search(who, aName, Util::toInt64(aSize), aTypeMode, aSizeMode, aToken, aWindow, aSearch);
 	}
	static string clean(const string& aSearchString);
	
	void respond(const AdcCommand& cmd);

	short getPort()
	{
		return port;
	}

	void listen() throw(Exception);
	void disconnect() throw();
	void onSearchResult(const string& aLine) {
		onData((const u_int8_t*)aLine.data(), aLine.length(), Util::emptyString);
	}

	u_int32_t getLastSearch();
	int getSearchQueueNumber(int* aWindow);
	ResultsQueue queue;

private:
	
	Socket* socket;
	short port;
	bool stop;
	u_int32_t lastSearch;
	friend class Singleton<SearchManager>;
	SearchQueueItemList searchQueue;

	SearchManager() : socket(NULL), port(0), stop(false), lastSearch(0) {
		TimerManager::getInstance()->addListener(this);
	};

	virtual void on(TimerManagerListener::Second, u_int32_t aTick) throw();

	virtual int run();

	virtual ~SearchManager() throw() {
		if(socket) {
			stop = true;
			socket->disconnect();
#ifdef _WIN32
			join();
#endif
			delete socket;
		}
		TimerManager::getInstance()->removeListener(this);
	};

	void setLastSearch(u_int32_t aTime) { lastSearch = aTime; };
	void onData(const u_int8_t* buf, size_t aLen, const string& address);	
};

#endif // !defined(SEARCH_MANAGER_H)

/**
 * @file
 * $Id$
 */
