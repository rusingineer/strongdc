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

#if !defined(SEARCH_MANAGER_H)
#define SEARCH_MANAGER_H

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
#include "ResourceManager.h"

class SearchManager;
class SocketException;

class SearchResult : public FastAlloc<SearchResult>, public UserInfoBase {
public:	

	enum Types {
		TYPE_FILE,
		TYPE_DIRECTORY
	};

	enum {
		COLUMN_FIRST,
		COLUMN_FILENAME = COLUMN_FIRST,
		COLUMN_HITS,
		COLUMN_NICK,
		COLUMN_TYPE,
		COLUMN_SIZE,
		COLUMN_PATH,
		COLUMN_SLOTS,
		COLUMN_CONNECTION,
		COLUMN_HUB,
		COLUMN_EXACT_SIZE,
		COLUMN_UPLOAD,
		COLUMN_IP,		
		COLUMN_TTH,
		COLUMN_LAST
	};

	typedef SearchResult* Ptr;
	typedef vector<Ptr> List;
	typedef List::const_iterator Iter;
	
	SearchResult(Types aType, int64_t aSize, const string& name, const TTHValue& aTTH);

	SearchResult(const UserPtr& aUser, Types aType, uint8_t aSlots, uint8_t aFreeSlots, 
		int64_t aSize, const string& aFile, const string& aHubName, 
		const string& ip, TTHValue aTTH, const string& aToken);

	string getFileName() const;
	string toSR(const Client& client) const;
	AdcCommand toRES(char type) const;

	const UserPtr& getUser() const { return user; }
	string getSlotString() const { return Util::toString(getFreeSlots()) + '/' + Util::toString(getSlots()); }

	const string& getFile() const { return file; }
	const string& getHubName() const { return hubName; }
	int64_t getSize() const { return size; }
	Types getType() const { return type; }
	uint8_t getSlots() const { return slots; }
	uint8_t getFreeSlots() const { return freeSlots; }
	
	const TTHValue& getTTH() const { return tth; }
	
	const string& getIP() const { return IP; }
	const string& getToken() const { return token; }

	void incRef() { Thread::safeInc(ref); }
	void decRef() { 
		if(Thread::safeDec(ref) == 0) 
			delete this; 
	}

	bool collapsed;
	size_t hits;
	SearchResult* parent;

	void getList();
	void browseList();

	void view();
    
	const tstring getText(uint8_t col) const {
		switch(col) {
			case COLUMN_FILENAME:
				if(getType() == SearchResult::TYPE_FILE) {
					if(getFile().rfind(_T('\\')) == tstring::npos) {
						return Text::toT(getFile());
					} else {
    					return Text::toT(Util::getFileName(getFile()));
					}      
				} else {
					return Text::toT(getFileName());
				}
			case COLUMN_HITS: return hits == 0 ? Util::emptyStringT : Util::toStringW(hits + 1) + _T(' ') + TSTRING(USERS);
			case COLUMN_NICK: return Text::toT(Util::toString(ClientManager::getInstance()->getNicks(getUser()->getCID())));
			case COLUMN_TYPE:
				if(getType() == SearchResult::TYPE_FILE) {
					tstring type = Text::toT(Util::getFileExt(Text::fromT(getText(COLUMN_FILENAME))));
					if(!type.empty() && type[0] == _T('.'))
						type.erase(0, 1);
					return type;
				} else {
					return TSTRING(DIRECTORY);
				}
			case COLUMN_SIZE: return getSize() > 0 ? Util::formatBytesW(getSize()) : Util::emptyStringT;
			case COLUMN_PATH:
				if(getType() == SearchResult::TYPE_FILE) {
					return Text::toT(Util::getFilePath(getFile()));
				} else {
					return Text::toT(getFile());
				}
			case COLUMN_SLOTS: return Text::toT(getSlotString());
			case COLUMN_CONNECTION: return Text::toT(ClientManager::getInstance()->getConnection(getUser()->getCID()));
			case COLUMN_HUB: return Text::toT(getHubName());
			case COLUMN_EXACT_SIZE: return getSize() > 0 ? Util::formatExactSize(getSize()) : Util::emptyStringT;
			case COLUMN_UPLOAD:
				if (getUser()->getLastDownloadSpeed() > 0) {
					return Util::formatBytesW(getUser()->getLastDownloadSpeed()) + _T("/s");
				} else if(getUser()->isSet(User::FIREBALL)) {
					return _T(">=100 kB/s");
				} else {
					return _T("N/A");
				}		
			case COLUMN_IP: {
				string ip = getIP();
				if (!ip.empty()) {
					// Only attempt to grab a country mapping if we actually have an IP address
					string tmpCountry = Util::getIpCountry(ip);
					if(!tmpCountry.empty()) {
						ip = tmpCountry + " (" + ip + ")";
					}
				}
				return Text::toT(ip);
			}
			case COLUMN_TTH: return getType() == SearchResult::TYPE_FILE ? Text::toT(getTTH().toBase32()) : Util::emptyStringT;
			default: return Util::emptyStringT;
		}
	}

	static int compareItems(const SearchResult* a, const SearchResult* b, uint8_t col) {
		if(!a || !b)
			return 0;

		switch(col) {
			case COLUMN_TYPE: 
				if(a->getType() == b->getType())
					return lstrcmpi(a->getText(COLUMN_TYPE).c_str(), b->getText(COLUMN_TYPE).c_str());
				else
					return(a->getType() == SearchResult::TYPE_DIRECTORY) ? -1 : 1;
			case COLUMN_HITS: return compare(a->hits, b->hits);
			case COLUMN_SLOTS: 
				if(a->getFreeSlots() == b->getFreeSlots())
					return compare(a->getSlots(), b->getSlots());
				else
					return compare(a->getFreeSlots(), b->getFreeSlots());
			case COLUMN_SIZE:
			case COLUMN_EXACT_SIZE: return compare(a->getSize(), b->getSize());
			case COLUMN_UPLOAD: return compare(a->getText(COLUMN_UPLOAD), b->getText(COLUMN_UPLOAD));
			default: return lstrcmpi(a->getText(col).c_str(), b->getText(col).c_str());
		}
	}

	int imageIndex() const;

	inline SearchResult* createParent() { return this; }
	inline const TTHValue& getGroupCond() const { return getTTH(); }
	inline void deleteSelf() { decRef(); }

	GETSET(uint8_t, flagImage, FlagImage);

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
	
	UserPtr user;
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

	void onRES(const AdcCommand& cmd, const UserPtr& from, const string& removeIp = Util::emptyString);
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
		CriticalSection cs;
		Semaphore s;
		
		deque<pair<string, string>> resultList;
		
		bool stop;
	} queue;

	CriticalSection cs;
	SearchQueueItemList searchQueue;	

	uint64_t lastSearch;
	Socket* socket;
	uint16_t port;
	bool stop;
	friend class Singleton<SearchManager>;

	SearchManager();

	int run();

	~SearchManager() throw();

	void setLastSearch(uint64_t aTime) { lastSearch = aTime; };
	void onData(const uint8_t* buf, size_t aLen, const string& address);

	string getPartsString(const PartsInfo& partsInfo) const;
	void on(TimerManagerListener::Second, uint64_t aTick) throw();
};

#endif // !defined(SEARCH_MANAGER_H)

/**
 * @file
 * $Id$
 */
