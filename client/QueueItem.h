/* 
* Copyright (C) 2001-2004 Jacek Sieka, j_s at telia com
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

#if !defined(AFX_QUEUEITEM_H__07D44A33_1277_482D_AFB4_05E3473B4379__INCLUDED_)
#define AFX_QUEUEITEM_H__07D44A33_1277_482D_AFB4_05E3473B4379__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class QueueManager;
class Download;

#include "User.h"
#include "FastAlloc.h"
#include "MerkleTree.h"
#include "FileChunksInfo.h"
#include "HashManager.h"

class QueueItem : public Flags, public FastAlloc<QueueItem> {
public:
	typedef QueueItem* Ptr;
	// Strange, the vc7 optimizer won't take a deque here...
	typedef vector<Ptr> List;
	typedef List::iterator Iter;
	typedef map<string*, Ptr, noCaseStringLess> StringMap;
	//	typedef HASH_MAP<string, Ptr, noCaseStringHash, noCaseStringEq> StringMap;
	typedef StringMap::iterator StringIter;
	typedef HASH_MAP_X(User::Ptr, Ptr, User::HashFunction, equal_to<User::Ptr>, less<User::Ptr>) UserMap;
	typedef UserMap::iterator UserIter;
	typedef HASH_MAP_X(User::Ptr, List, User::HashFunction, equal_to<User::Ptr>, less<User::Ptr>) UserListMap;
	typedef UserListMap::iterator UserListIter;

	enum Status {
		/** The queue item is waiting to be downloaded and can be found in userQueue */
		STATUS_WAITING,
		/** This item is being downloaded and can be found in running */
		STATUS_RUNNING
	};

	enum Priority {
		DEFAULT = -1,
		PAUSED = 0,
		LOWEST,
		LOW,
		NORMAL,
		HIGH,
		HIGHEST,
		LAST
	};

	enum FileFlags {
		/** Normal download, no flags set */
		FLAG_NORMAL = 0x00, 
		/** This download should be resumed if possible */
		FLAG_RESUME = 0x01,
		/** This is a user file listing download */
		FLAG_USER_LIST = 0x02,
		/** The file list is downloaded to use for directory download (used with USER_LIST) */
		FLAG_DIRECTORY_DOWNLOAD = 0x04,
		/** The file is downloaded to be viewed in the gui */
		FLAG_CLIENT_VIEW = 0x08,
		/** Flag to indicate that file should be viewed as a text file */
		FLAG_TEXT = 0x20,
		/** This file exists on the hard disk and should be prioritised */
		FLAG_EXISTS = 0x40,
		/** Match the queue against this list */
		FLAG_MATCH_QUEUE = 0x80,
		/** The source being added has its filename in utf-8 */
		FLAG_SOURCE_UTF8 = 0x100,
		/** The file list downloaded was actually an .xml.bz2 list */
		FLAG_XML_BZLIST = 0x200,
		/** MP3 Info */
		FLAG_MP3_INFO = 0x400,
		FLAG_TESTSUR = 0x800,
		FLAG_CHECK_FILE_LIST = 0x1000,
		FLAG_MULTI_SOURCE = 0x2000
	};

	class Source : public Flags, public FastAlloc<Source> {
	public:
		typedef Source* Ptr;
		typedef vector<Ptr> List;
		typedef List::iterator Iter;
		typedef List::const_iterator ConstIter;
		enum {
			FLAG_NONE = 0x00,
			FLAG_FILE_NOT_AVAILABLE = 0x01,
			FLAG_ROLLBACK_INCONSISTENCY = 0x02,
			FLAG_PASSIVE = 0x04,
			FLAG_REMOVED = 0x08,
			FLAG_CRC_FAILED = 0x10,
			FLAG_CRC_WARN = 0x20,
			FLAG_UTF8 = 0x40,
			FLAG_BAD_TREE = 0x80,
			FLAG_SLOW = 0x100,
			FLAG_NO_TREE = 0x200,
			FLAG_MASK = FLAG_FILE_NOT_AVAILABLE | FLAG_ROLLBACK_INCONSISTENCY 
				| FLAG_PASSIVE | FLAG_REMOVED | FLAG_CRC_FAILED | FLAG_CRC_WARN | FLAG_UTF8 | FLAG_BAD_TREE
				| FLAG_SLOW | FLAG_NO_TREE			
		};

		Source(const User::Ptr& aUser, const string& aPath) : path(aPath), user(aUser) { };
		Source(const Source& aSource) : Flags(aSource), path(aSource.path), user(aSource.user) { }

		User::Ptr& getUser() { return user; };
		const User::Ptr& getUser() const { return user; };
		void setUser(const User::Ptr& aUser) { user = aUser; };
		string getFileName() { return Util::getFileName(path); };

		GETSET(string, path, Path);
	private:
		User::Ptr user;
	};

	QueueItem(const string& aTarget, int64_t aSize, 
		Priority aPriority, int aFlag, int64_t aDownloadedBytes, u_int32_t aAdded, const TTHValue* tth) : 
	Flags(aFlag), target(aTarget), start(0), currentDownload(NULL),
	size(aSize), downloadedBytes(aDownloadedBytes), status(STATUS_WAITING), priority(aPriority), added(aAdded), fastUser(false),
	tthRoot(tth == NULL ? NULL : new TTHValue(*tth)), autoPriority(false), hasTree(false), speed(0), noFreeBlocks(false)
	{ 
		slowDisconnect = BOOLSETTING(DISCONNECTING_ENABLE);
		
		if(isSet(FLAG_USER_LIST) || isSet(FLAG_MP3_INFO) || isSet(FLAG_TESTSUR) || (tth == NULL) || (size < 2*1024*1024)) {
			unsetFlag(FLAG_MULTI_SOURCE);
		}

		if(tth != NULL) {
			TigerTree tree;
			hasTree = HashManager::getInstance()->getTree(*tth, tree);
		}

		speedUsers.push_back(NULL);
		speedUsers.push_back(NULL);
		speedUsers.push_back(NULL);
	};

	QueueItem(const QueueItem& rhs) : 
	Flags(rhs), target(rhs.target), tempTarget(rhs.tempTarget),
		size(rhs.size), downloadedBytes(rhs.downloadedBytes), status(rhs.status), priority(rhs.priority), currents(rhs.currents), activeSegments(rhs.activeSegments),
		added(rhs.added), tthRoot(rhs.tthRoot == NULL ? NULL : new TTHValue(*rhs.tthRoot)), autoPriority(rhs.autoPriority), fileChunksInfo(NULL),
		start(rhs.start), fastUser(rhs.fastUser), speedUsers(rhs.speedUsers), noFreeBlocks(rhs.noFreeBlocks), currentDownload(rhs.currentDownload)
	{
		// Deep copy the source lists
		Source::List::const_iterator i;
		for(i = rhs.sources.begin(); i != rhs.sources.end(); ++i) {
			sources.push_back(new Source(*(*i)));
		}
		for(i = rhs.badSources.begin(); i != rhs.badSources.end(); ++i) {
			badSources.push_back(new Source(*(*i)));
		}
	}

	virtual ~QueueItem() { 
		for_each(sources.begin(), sources.end(), DeleteFunction<Source*>());
		for_each(badSources.begin(), badSources.end(), DeleteFunction<Source*>());
		delete tthRoot;
	};

	int countOnlineUsers() const {
		int n = 0;
		Source::List::const_iterator i = sources.begin();
		for(; i != sources.end(); ++i) {
			if((*i)->getUser()->isOnline())
				n++;
		}
		return n;
	}
	bool hasOnlineUsers() const { return countOnlineUsers() > 0; };

	const string& getSourcePath(const User::Ptr& aUser) { 
		if(!isSource(aUser))
			return Util::emptyString;

		return (*getSource(aUser, sources))->getPath();
	}

	Source::List& getSources() { return sources; };
	Source::List& getBadSources() { return badSources; };

	void getOnlineUsers(User::List& l) const  {
		for(Source::List::const_iterator i = sources.begin(); i != sources.end(); ++i)
			if((*i)->getUser()->isOnline())
				l.push_back((*i)->getUser());
	}

	string getTargetFileName() const { return Util::getFileName(getTarget()); };

	Source::Iter getSource(const User::Ptr& aUser) { return getSource(aUser, sources); };
	Source::Iter getBadSource(const User::Ptr& aUser) { return getSource(aUser, badSources); };

	bool isSource(const User::Ptr& aUser) { return (getSource(aUser, sources) != sources.end()); };
	bool isBadSource(const User::Ptr& aUser) { return (getSource(aUser, badSources) != badSources.end()); };

	bool isSource(const User::Ptr& aUser, const string& aFile) const { return isSource(aUser, aFile, sources); };
	bool isBadSource(const User::Ptr& aUser, const string& aFile) const { return isSource(aUser, aFile, badSources); };
	bool isBadSourceExcept(const User::Ptr& aUser, const string& aFile, Flags::MaskType exceptions) const {
		Source::ConstIter i = getSource(aUser, aFile, badSources);
		if(i != badSources.end())
			return (*i)->isAnySet(exceptions^Source::FLAG_MASK); 
		return false;
	};
	
	void addActiveSegment(const User::Ptr& aUser) {
		if(!isSource(aUser)) return;
		activeSegments.push_back(*getSource(aUser));
	}

	void removeActiveSegment(const User::Ptr& aUser) {
		if(find(activeSegments.begin(), activeSegments.end(), *getSource(aUser)) != activeSegments.end()) {
			activeSegments.erase(find(activeSegments.begin(), activeSegments.end(), *getSource(aUser)));
		}
	}

	void addCurrent(const User::Ptr& aUser) {
		dcassert(isSource(aUser));
		currents.push_back(*getSource(aUser));
	}
	
	bool isCurrent(const User::Ptr& aUser) {
		dcassert(isSource(aUser));
		return find(currents.begin(), currents.end(), *getSource(aUser)) != currents.end();
	
	}

	// All setCurrent(NULL) should be replaced with this
	void removeCurrent(const User::Ptr& aUser) {
		dcassert(isSource(aUser));
		dcassert(find(currents.begin(), currents.end(), *getSource(aUser)) != currents.end());

		currents.erase(find(currents.begin(), currents.end(), *getSource(aUser)));
	}

	int64_t getDownloadedBytes(){
		if(isSet(FLAG_MULTI_SOURCE)){
			FileChunksInfo::Ptr filedatainfo = FileChunksInfo::Get(tempTarget);
			if(filedatainfo)
				return filedatainfo->GetDownloadedSize();
		}

		return downloadedBytes;
	}

	void setDownloadedBytes(int64_t pos) {
		downloadedBytes = pos;
	}
	
	string getListName() {
		dcassert(isSet(QueueItem::FLAG_USER_LIST));
		if(isSet(QueueItem::FLAG_XML_BZLIST)) {
			return getTarget() + ".xml.bz2";
		} else {
			return getTarget() + ".DcLst";
		}
	}

	string getSearchString() const;
	Source::List speedUsers;

	const string& getTempTarget();
	void setTempTarget(const string& aTempTarget) {
		tempTarget = aTempTarget;
	}
	GETSET(string, target, Target);
	string tempTarget;
	int64_t downloadedBytes;
	GETSET(int64_t, size, Size);
	GETSET(Status, status, Status);
	GETSET(Priority, priority, Priority);
	GETSET(Source::List, currents, Currents);
	GETSET(Source::List, activeSegments, ActiveSegments);
	GETSET(Download*, currentDownload, CurrentDownload);
	GETSET(u_int32_t, added, Added);
	GETSET(TTHValue*, tthRoot, TTH);
	GETSET(bool, autoPriority, AutoPriority);
	GETSET(int, maxSegments, MaxSegments);
	GETSET(bool, hasTree, HasTree);
	GETSET(bool, slowDisconnect, SlowDisconnect);
	GETSET(bool, noFreeBlocks, NoFreeBlocks);
	GETSET(bool, fastUser, FastUser);
	GETSET(int64_t, speed, Speed);
	GETSET(FileChunksInfo::Ptr, fileChunksInfo, FileChunksInfo);
	GETSET(u_int32_t, start, Start);

	QueueItem::Priority calculateAutoPriority(){
		QueueItem::Priority p = getPriority();
		if(p == QueueItem::PAUSED) return p;
		if(getAutoPriority()){			
			int percent = getDownloadedBytes() * 10.0 / getSize();
			switch(percent){
					case 0:
					case 1:
						p = QueueItem::LOWEST;
						break;
					case 2:
					case 3:
						p = QueueItem::LOW;
						break;
					case 4:
					case 5:						
					case 6:
					default:
						p = QueueItem::NORMAL;
						break;
					case 7:
					case 8:
						p = QueueItem::HIGH;
						break;
					case 9:
					case 10:
						p = QueueItem::HIGHEST;			
						break;
			}			
		}
		return p;
	}

private:
	friend class QueueManager;
	Source::List sources;
	Source::List badSources;	

	Source* addSource(const User::Ptr& aUser, const string& aPath) {
		dcassert(!isSource(aUser));
		Source* s = NULL;
		Source::Iter i = getSource(aUser, badSources);
		if(i != badSources.end()) {
			s = *i;
			badSources.erase(i);
			s->setPath(aPath);
		} else {
			s = new Source(aUser, aPath);
		}

		sources.push_back(s);
		return s;
	}

	void removeSource(const User::Ptr& aUser, int reason) {
		Source::Iter i = getSource(aUser, sources);
		//dcassert(i != sources.end());
		if(i == sources.end()) return;

		(*i)->setFlag(reason);
		badSources.push_back(*i);
		sources.erase(i);
	}

	static Source::Iter getSource(const User::Ptr& aUser, Source::List& lst) { 
		for(Source::Iter i = lst.begin(); i != lst.end(); ++i)
			if((*i)->getUser() == aUser)
				return i;
		return lst.end();
	}
	static Source::ConstIter getSource(const User::Ptr& aUser, const string& aFile, const Source::List& lst) { 
		for(Source::ConstIter i = lst.begin(); i != lst.end(); ++i) {
			const Source* s = *i;
			if( (s->getUser() == aUser) ||
				((s->getUser()->getNick() == aUser->getNick()) && (s->getPath() == aFile)) )
				return i;
		}

		return lst.end();
	}
	static bool isSource(const User::Ptr& aUser, const string& aFile, const Source::List& lst) {
		for(Source::List::const_iterator i = lst.begin(); i != lst.end(); ++i) {
			Source* s = *i;
			if( (s->getUser() == aUser) ||
				((s->getUser()->getNick() == aUser->getNick()) && (s->getPath() == aFile)) )
				return true;
		}
		return false;
	}

};

#endif // !defined(AFX_QUEUEITEM_H__07D44A33_1277_482D_AFB4_05E3473B4379__INCLUDED_)

/**
* @file
* $Id$
*/