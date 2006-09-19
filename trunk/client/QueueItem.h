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

#if !defined(QUEUE_ITEM_H)
#define QUEUE_ITEM_H

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
#include "SettingsManager.h"

class QueueItem : public Flags, public FastAlloc<QueueItem> {
public:
	typedef QueueItem* Ptr;
	typedef deque<Ptr> List;
	typedef List::const_iterator Iter;
	//typedef map<string*, Ptr, noCaseStringLess> StringMap;
	//test the hash_map
	typedef HASH_MAP<string*, Ptr, noCaseStringHash, noCaseStringEq> StringMap;
	typedef StringMap::const_iterator StringIter;
	typedef HASH_MAP_X(User::Ptr, Ptr, User::HashFunction, equal_to<User::Ptr>, less<User::Ptr>) UserMap;
	typedef UserMap::const_iterator UserIter;
	typedef HASH_MAP_X(User::Ptr, List, User::HashFunction, equal_to<User::Ptr>, less<User::Ptr>) UserListMap;
	typedef UserListMap::const_iterator UserListIter;

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
		/** The file list downloaded was actually an .xml.bz2 list */
		FLAG_XML_BZLIST = 0x200,
		/** Test user for slotlocker */
		FLAG_TESTSUR = 0x400,
		/** Test user's file list for fake share */
		FLAG_CHECK_FILE_LIST = 0x800,
		/** The file can be downloaded from multiple sources simultaneously */
		FLAG_MULTI_SOURCE = 0x1000,
		/** Autodrop slow source is enabled for this file */
		FLAG_AUTODROP = 0x2000
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
			FLAG_NO_TTHF = 0x40,
			FLAG_BAD_TREE = 0x80,
			FLAG_SLOW = 0x100,
			FLAG_NO_TREE = 0x200,
			FLAG_NO_NEED_PARTS = 0x400,
			FLAG_PARTIAL = 0x800,
			FLAG_TTH_INCONSISTENCY = 0x1000,
			FLAG_MASK = FLAG_FILE_NOT_AVAILABLE | FLAG_ROLLBACK_INCONSISTENCY 
				| FLAG_PASSIVE | FLAG_REMOVED | FLAG_CRC_FAILED | FLAG_CRC_WARN | FLAG_BAD_TREE
				| FLAG_SLOW | FLAG_NO_TREE | FLAG_TTH_INCONSISTENCY
		};

		Source(const User::Ptr& aUser) : user(aUser) { }
		Source(const Source& aSource) : Flags(aSource), user(aSource.user), partialInfo(aSource.partialInfo) { }

		User::Ptr& getUser() { return user; }

		/**
		 * Source parts info
		 * Meaningful only when FLAG_PARTIAL is set
		 * If this source is not bad source, empty parts info means full file
		 */
		GETSET(PartsInfo, partialInfo, PartialInfo);
		GETSET(User::Ptr, user, User);
	};

	QueueItem(const string& aTarget, int64_t aSize, 
		Priority aPriority, int aFlag, int64_t aDownloadedBytes, u_int32_t aAdded, const TTHValue& tth) :
	Flags(aFlag), target(aTarget), currentDownload(NULL), averageSpeed(0),
	size(aSize), downloadedBytes(aDownloadedBytes), status(STATUS_WAITING), priority(aPriority), added(aAdded),
	tthRoot(tth), autoPriority(false), hasTree(false)
	{ 
		if(BOOLSETTING(DISCONNECTING_ENABLE)) {
			setFlag(FLAG_AUTODROP);
		}
		if(isSet(FLAG_USER_LIST) || isSet(FLAG_TESTSUR) || isSet(FLAG_CHECK_FILE_LIST) || (size < 2097153)) {
			unsetFlag(FLAG_MULTI_SOURCE);
		}

		TigerTree tree;
		hasTree = HashManager::getInstance()->getTree(tth, tree);
	}

	QueueItem(const QueueItem& rhs) : 
	Flags(rhs), target(rhs.target), tempTarget(rhs.tempTarget),
		size(rhs.size), downloadedBytes(rhs.downloadedBytes), status(rhs.status), priority(rhs.priority), currents(rhs.currents),
		currentDownload(rhs.currentDownload), added(rhs.added), tthRoot(rhs.tthRoot),
		averageSpeed(rhs.averageSpeed), autoPriority(rhs.autoPriority)
	{
		// Deep copy the source lists
		Source::ConstIter i;
		for(i = rhs.sources.begin(); i != rhs.sources.end(); ++i) {
			sources.push_back(new Source(*(*i)));
		}
		for(i = rhs.badSources.begin(); i != rhs.badSources.end(); ++i) {
			badSources.push_back(new Source(*(*i)));
		}
	}

	virtual ~QueueItem() { 
		for_each(sources.begin(), sources.end(), DeleteFunction());
		for_each(badSources.begin(), badSources.end(), DeleteFunction());
	}

	int countOnlineUsers() const {
		int n = 0;
		Source::ConstIter i = sources.begin();
		for(; i != sources.end(); ++i) {
			if((*i)->getUser()->isOnline())
				n++;
		}
		return n;
	}
	bool hasOnlineUsers() const { 
		Source::ConstIter i = sources.begin();
		for(; i != sources.end(); ++i) {
			if((*i)->getUser()->isOnline())
				return true;
		}
		return false;
	}

	Source::List& getSources() { return sources; }
	Source::List& getBadSources() { return badSources; }

	void getOnlineUsers(User::List& l) const  {
		for(Source::ConstIter i = sources.begin(); i != sources.end(); ++i)
			if((*i)->getUser()->isOnline())
				l.push_back((*i)->getUser());
	}

	string getTargetFileName() const { return Util::getFileName(getTarget()); }

	Source::Iter getSource(const User::Ptr& aUser) { return getSource(aUser, sources); }
	Source::Iter getBadSource(const User::Ptr& aUser) { return getSource(aUser, badSources); }

	bool isSource(const User::Ptr& aUser) { return (getSource(aUser, sources) != sources.end()); }
	bool isBadSource(const User::Ptr& aUser) { return (getSource(aUser, badSources) != badSources.end()); }

	bool isSource(const User::Ptr& aUser) const { return isSource(aUser, sources); }
	bool isBadSource(const User::Ptr& aUser) const { return isSource(aUser, badSources); }
	bool isBadSourceExcept(const User::Ptr& aUser, Flags::MaskType exceptions) const {
		Source::ConstIter i = getSource(aUser, badSources);
		if(i != badSources.end())
			return (*i)->isAnySet(exceptions^Source::FLAG_MASK); 
		return false;
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
		if(chunkInfo)
			return chunkInfo->getDownloadedSize();
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
			return getTarget() + ".xml";
		}
	}

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
	GETSET(Download*, currentDownload, CurrentDownload);
	GETSET(u_int32_t, added, Added);
	GETSET(TTHValue, tthRoot, TTH);
	GETSET(bool, autoPriority, AutoPriority);
	GETSET(u_int8_t, maxSegments, MaxSegments);
	GETSET(bool, hasTree, HasTree);
	GETSET(size_t, averageSpeed, AverageSpeed);
	FileChunksInfo::Ptr chunkInfo;

	QueueItem::Priority calculateAutoPriority(){
		
		if(getAutoPriority()){
			QueueItem::Priority p;
			int percent = (int)(getDownloadedBytes() * 10.0 / getSize());
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
			return p;			
		}
		return priority;
	}

	bool hasFreeSegments() {
		if(!isSet(QueueItem::FLAG_MULTI_SOURCE)) {
			return (status != STATUS_RUNNING);
		} else {
			return ((currents.size() < maxSegments) &&
					(!BOOLSETTING(DONT_BEGIN_SEGMENT) || ((size_t)(SETTING(DONT_BEGIN_SEGMENT_SPEED)*1024) >= averageSpeed)));
		}
	}

private:
	QueueItem& operator=(const QueueItem&);

	friend class QueueManager;
	Source::List sources;
	Source::List badSources;	

	Source* addSource(const User::Ptr& aUser);

	void removeSource(const User::Ptr& aUser, int reason);

	static Source::Iter getSource(const User::Ptr& aUser, Source::List& lst);
	static Source::ConstIter getSource(const User::Ptr& aUser, const Source::List& lst) ;
	static bool isSource(const User::Ptr& aUser, const Source::List& lst);

};

#endif // !defined(QUEUE_ITEM_H)

/**
* @file
* $Id$
*/
