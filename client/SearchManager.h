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

class SearchManager;
class SocketException;

class SearchResult : public FastAlloc<SearchResult> {
public:	

	enum Types {
		TYPE_FILE,
		TYPE_DIRECTORY
	};

	typedef SearchResult* Ptr;
	typedef vector<Ptr> List;
	typedef List::const_iterator Iter;
	
	SearchResult(Types aType, int64_t aSize, const string& name, const TTHValue& aTTH);

	SearchResult(const User::Ptr& aUser, Types aType, uint8_t aSlots, uint8_t aFreeSlots, 
		int64_t aSize, const string& aFile, const string& aHubName, 
		const string& ip, TTHValue aTTH, const string& aToken) :
	file(aFile), hubName(aHubName), user(aUser),
		size(aSize), type(aType), slots(aSlots), freeSlots(aFreeSlots), IP(ip),
		tth(aTTH), token(aToken), ref(1) { }

	string getFileName() const;
	string toSR(const Client& client) const;
	AdcCommand toRES(char type) const;

	const User::Ptr& getUser() const { return user; }
	string getSlotString() const { return Util::toString(getFreeSlots()) + '/' + Util::toString(getSlots()); }

	const string& getFile() const { return file; }
	const string& getHubName() const { return hubName; }
	int64_t getSize() const { return size; }
	Types getType() const { return type; }
	uint8_t getSlots() const { return slots; }
	uint8_t getFreeSlots() const { return freeSlots; }
	TTHValue getTTH() const { return tth; }
	const string& getIP() const { return IP; }
	const string& getToken() const { return token; }

	void incRef() { Thread::safeInc(ref); }
	void decRef() { 
		if(Thread::safeDec(ref) == 0) 
			delete this; 
	}

private:
	friend class SearchManager;

	SearchResult();
	~SearchResult() { }

	SearchResult(const SearchResult& rhs);

	TTHValue tth;
	
	string file;
	string hubName;
	string IP;
	string token;
	
	int64_t size;
	
	User::Ptr user;
	Types type;

	uint8_t slots;
	uint8_t freeSlots;
	
	volatile long ref;
};

class SearchQueueItem {
public:
	SearchQueueItem() { }
	SearchQueueItem(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const int *aWindow, const string& aToken) : 
	  target(aString), size(aSize), typeMode(aFileType), sizeMode(aSizeMode), window(aWindow), token(aToken) { }
	SearchQueueItem(StringList& who, int aSizeMode, int64_t aSize, int aFileType, const string& aString, const int *aWindow, const string& aToken) : 
	  hubs(who), target(aString), size(aSize), typeMode(aFileType), sizeMode(aSizeMode), window(aWindow), token(aToken) { }

	GETSET(string, target, Target);
	GETSET(string, token, Token);
	GETSET(int64_t, size, Size);
	GETSET(int, typeMode, TypeMode);
	GETSET(int, sizeMode, SizeMode);

	StringList& getHubs() { return hubs; }
	void setHubs(StringList aHubs) { hubs = aHubs; }
	const int* getWindow() const { return window; }
private:
	StringList hubs;
	
	const int* window;
};

class SearchManager : public Speaker<SearchManagerListener>, private TimerManagerListener, public Singleton<SearchManager>, public Thread
{
public:
	typedef deque<SearchQueueItem> SearchQueueItemList;
	typedef SearchQueueItemList::iterator SearchQueueIter;
	typedef SearchQueueItemList::const_iterator SearchQueueIterC;

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
	
	void search(const string& aName, int64_t aSize, TypeModes aTypeMode, SizeModes aSizeMode, const string& aToken, const int *aWindow = NULL);
	void search(const string& aName, const string& aSize, TypeModes aTypeMode, SizeModes aSizeMode, const string& aToken, const int *aWindow = NULL) {
		search(aName, Util::toInt64(aSize), aTypeMode, aSizeMode, aToken, aWindow);
	}

	void search(StringList& who, const string& aName, int64_t aSize, TypeModes aTypeMode, SizeModes aSizeMode, const string& aToken, const int *aWindow = NULL);
	void search(StringList& who, const string& aName, const string& aSize, TypeModes aTypeMode, SizeModes aSizeMode, const string& aToken, const int *aWindow = NULL) {
		search(who, aName, Util::toInt64(aSize), aTypeMode, aSizeMode, aToken, aWindow);
 	}
	void stopSearch(const int *aWindow);

	static string clean(const string& aSearchString);
	
	void respond(const AdcCommand& cmd, const CID& cid);

	uint16_t getPort() const
	{
		return port;
	}

	void listen() throw(SocketException);
	void disconnect() throw();
	void onSearchResult(const string& aLine) {
		onData((const uint8_t*)aLine.data(), aLine.length(), Util::emptyString);
	}

	void onRES(const AdcCommand& cmd, const User::Ptr& from, const string& removeIp = Util::emptyString);
	void sendPSR(const string& ip, uint16_t port, bool wantResponse, const string& myNick, const string& hubIpPort, const string& tth, const vector<uint16_t>& partialInfo);

	uint64_t getLastSearch() const { return lastSearch; }
	int getSearchQueueNumber(const int* aWindow);
	

private:
	class ResultsQueue: public Thread {
	public:
		ResultsQueue() : stop(false) {}
		~ResultsQueue() throw() { shutdown(); }

		int run();
		void shutdown() {
			stop = true;
			s.signal();
		}
		void addResult(const string& buf, const string& ip) {
			{
				Lock l(cs);
				resultList.push_back(make_pair(buf, ip));
			}
			s.signal();
		}
	private:
		bool stop;
		CriticalSection cs;
		Semaphore s;
		deque<pair<string, string>> resultList;
	} queue;

	Socket* socket;
	uint16_t port;
	bool stop;
	uint64_t lastSearch;
	friend class Singleton<SearchManager>;
	SearchQueueItemList searchQueue;
	CriticalSection cs;

	SearchManager() : socket(NULL), port(0), stop(false), lastSearch(0) {
		TimerManager::getInstance()->addListener(this);
	}

	virtual void on(TimerManagerListener::Second, uint32_t aTick) throw();

	virtual int run();

	virtual ~SearchManager() throw() {
		TimerManager::getInstance()->removeListener(this);
		if(socket) {
			stop = true;
			socket->disconnect();
#ifdef _WIN32
			join();
#endif
			delete socket;
		}
	}

	void setLastSearch(uint32_t aTime) { lastSearch = aTime; };
	void onData(const uint8_t* buf, size_t aLen, const string& address);

	string getPartsString(const PartsInfo& partsInfo) const;
};

#endif // !defined(SEARCH_MANAGER_H)

/**
 * @file
 * $Id$
 */
