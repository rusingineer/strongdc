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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "QueueManager.h"

#include "ConnectionManager.h"
#include "SearchManager.h"
#include "ClientManager.h"
#include "DownloadManager.h"
#include "ShareManager.h"
#include "LogManager.h"
#include "ResourceManager.h"
#include "version.h"

#include "UserConnection.h"
#include "SimpleXML.h"
#include "StringTokenizer.h"
#include "DirectoryListing.h"

#include "FileChunksInfo.h"
#include "../pme-1.0.4/pme.h"
#include "FilteredFile.h"
#include "MerkleCheckOutputStream.h"
#include "UploadManager.h"

#include <limits>

#ifdef _WIN32
#define FILELISTS_DIR "FileLists\\"
#else
#define FILELISTS_DIR "filelists/"
#endif

#ifdef ff
#undef ff
#endif

#ifndef _WIN32
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#endif

const string QueueManager::USER_LIST_NAME = "MyList.DcLst";

namespace {
	const string TEMP_EXTENSION = ".dctmp";

	string getTempName(const string& aFileName, const TTHValue* aRoot) {
		string tmp(aFileName);
		if(aRoot != NULL) {
			TTHValue tmpRoot(*aRoot);
			tmp += "." + tmpRoot.toBase32();
		}
		tmp += TEMP_EXTENSION;
		return tmp;
	}

	/**
	* Convert space seperated number string to vector
	*
	* Created by RevConnect
	*/
	template <class T>
	void toIntList(const string& freeBlocks, vector<T>& v)
	{
		if (freeBlocks.empty())
			return;

		StringTokenizer<string> t(freeBlocks, ' ');
		StringList& sl = t.getTokens();

		v.reserve(sl.size() / 2);

		for(StringList::iterator i = sl.begin(); i != sl.end(); ++i) {
			if(!i->empty()) {
				int64_t offset = Util::toInt64(*i);
				if(!v.empty() && offset < v.back()){
					dcassert(0);
					v.clear();
					return;
				}else{
					v.push_back((T)offset);
				}
			}else{
				dcassert(0);
				v.clear();
				return;
			}
		}

		dcassert(!v.empty() && (v.size() % 2 == 0));
	}
}

const string& QueueItem::getTempTarget() {
	if(!isSet(QueueItem::FLAG_USER_LIST) && tempTarget.empty()) {
		if(!SETTING(TEMP_DOWNLOAD_DIRECTORY).empty() /*&& (File::getSize(getTarget()) == -1)*/) {
#ifdef _WIN32
			::StringMap sm;
			if(target.length() >= 3 && target[1] == ':' && target[2] == '\\')
				sm["targetdrive"] = target.substr(0, 3);
			else
				sm["targetdrive"] = Util::getAppPath().substr(0, 3);
			setTempTarget(Util::formatParams(SETTING(TEMP_DOWNLOAD_DIRECTORY), sm) + getTempName(getTargetFileName(), getTTH()));
#else //_WIN32
			setTempTarget(SETTING(TEMP_DOWNLOAD_DIRECTORY) + getTempName(getTargetFileName(), getTTH()));
#endif //_WIN32
		}
	}
	return tempTarget;
}

QueueItem* QueueManager::FileQueue::add(const string& aTarget, int64_t aSize, 
						  int aFlags, QueueItem::Priority p, const string& aTempTarget,
						  int64_t aDownloadedBytes, u_int32_t aAdded, const string& freeBlocks/* = Util::emptyString*/, const string& verifiedBlocks /* = Util::emptyString */, const TTHValue* root) throw(QueueException, FileException) 
{
	if(p == QueueItem::DEFAULT)
		p = (aSize <= 65536) ? QueueItem::HIGHEST : QueueItem::NORMAL;

	if(aFlags & QueueItem::FLAG_USER_LIST) 
		p = QueueItem::HIGHEST;

	QueueItem* qi = new QueueItem(aTarget, aSize, p, aFlags, aDownloadedBytes, aAdded, root);

	qi->setMaxSegments(qi->isSet(QueueItem::FLAG_MULTI_SOURCE) ? getMaxSegments(qi->getTargetFileName(), qi->getSize()) : 1);

	if(!qi->isSet(QueueItem::FLAG_USER_LIST)) {
		if(!aTempTarget.empty()) {
			qi->setTempTarget(aTempTarget);
		}
	}

	// Create FileChunksInfo for this item
	if(qi->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
		FileChunksInfo* pChunksInfo = NULL;

		dcassert(!qi->getTempTarget().empty());
		if ( freeBlocks != Util::emptyString ){
			vector<int64_t> v;
			toIntList<int64_t>(freeBlocks, v);
			pChunksInfo = new FileChunksInfo(qi->getTempTarget(), qi->getSize(), &v);
		}else{
			pChunksInfo = new FileChunksInfo(qi->getTempTarget(), qi->getSize(), NULL);
		}
		qi->chunkInfo = pChunksInfo;

		if(pChunksInfo && verifiedBlocks != Util::emptyString){
			vector<u_int16_t> v;
			toIntList<u_int16_t>(verifiedBlocks, v);

			for(vector<u_int16_t>::iterator i = v.begin(); i + 1 < v.end(); i++, i++)
				pChunksInfo->markVerifiedBlock(*i, *(i+1));
		}
	}

	if((qi->getDownloadedBytes() > 0))
		qi->setFlag(QueueItem::FLAG_EXISTS);

	if(BOOLSETTING(AUTO_PRIORITY_DEFAULT) && !qi->isSet(QueueItem::FLAG_USER_LIST) && (p != QueueItem::HIGHEST) ) {
		qi->setAutoPriority(true);
		qi->setPriority(qi->calculateAutoPriority());
	}

	dcassert(find(aTarget) == NULL);
	add(qi);
	return qi;
}

void QueueManager::FileQueue::add(QueueItem* qi) {
	if(lastInsert == queue.end())
		lastInsert = queue.insert(make_pair(const_cast<string*>(&qi->getTarget()), qi)).first;
	else
		lastInsert = queue.insert(lastInsert, make_pair(const_cast<string*>(&qi->getTarget()), qi));
}

QueueItem* QueueManager::FileQueue::find(const string& target) {
	QueueItem::StringIter i = queue.find(const_cast<string*>(&target));
	return (i == queue.end()) ? NULL : i->second;
}

void QueueManager::FileQueue::find(QueueItem::List& sl, int64_t aSize, const string& suffix) {
	for(QueueItem::StringIter i = queue.begin(); i != queue.end(); ++i) {
		if(i->second->getSize() == aSize) {
			const string& t = i->second->getTarget();
			if(suffix.empty() || (suffix.length() < t.length() &&
				Util::stricmp(suffix.c_str(), t.c_str() + (t.length() - suffix.length())) == 0) )
				sl.push_back(i->second);
		}
	}
}

void QueueManager::FileQueue::find(QueueItem::List& ql, const TTHValue& tth) {
	for(QueueItem::StringIter i = queue.begin(); i != queue.end(); ++i) {
		QueueItem* qi = i->second;
		if(qi->getTTH() != NULL && *qi->getTTH() == tth) {
			ql.push_back(qi);
		}
	}
}

static QueueItem* findCandidate(QueueItem::StringIter start, QueueItem::StringIter end, StringList& recent) {
	QueueItem* cand = NULL;
	for(QueueItem::StringIter i = start; i != end; ++i) {
		QueueItem* q = i->second;

		// No user lists
		if(q->isSet(QueueItem::FLAG_USER_LIST))
			continue;
        // No paused downloads
		if(q->getPriority() == QueueItem::PAUSED)
			continue;

		// No files without TTH
		if(q->getTTH() == NULL)
			continue;
		// Did we search for it recently?
        if(find(recent.begin(), recent.end(), q->getTarget()) != recent.end())
			continue;

		if(q->isSet(QueueItem::FLAG_TESTSUR) || q->isSet(QueueItem::FLAG_CHECK_FILE_LIST))
			continue;

		cand = q;
	}
	return cand;
}

QueueItem* QueueManager::FileQueue::findAutoSearch(StringList& recent) {
	// We pick a start position at random, hoping that we will find something to search for...
	QueueItem::StringMap::size_type start = (QueueItem::StringMap::size_type)Util::rand((u_int32_t)queue.size());

	QueueItem::StringIter i = queue.begin();
	advance(i, start);

	QueueItem* cand = findCandidate(i, queue.end(), recent);
	if(cand == NULL) {
		cand = findCandidate(queue.begin(), i, recent);
	}
	return cand;
}

void QueueManager::FileQueue::move(QueueItem* qi, const string& aTarget) {
	if(lastInsert != queue.end() && Util::stricmp(*lastInsert->first, qi->getTarget()) == 0)
		lastInsert = queue.end();
	queue.erase(const_cast<string*>(&qi->getTarget()));
	qi->setTarget(aTarget);
	add(qi);
}

void QueueManager::UserQueue::add(QueueItem* qi) {
	for(QueueItem::Source::Iter i = qi->getSources().begin(); i != qi->getSources().end(); ++i) {
		add(qi, (*i)->getUser());
	}
}

void QueueManager::UserQueue::add(QueueItem* qi, const User::Ptr& aUser) {
	dcassert(qi->isSource(aUser));

	QueueItem::List& l = userQueue[qi->getPriority()][aUser];
	if(qi->isSet(QueueItem::FLAG_EXISTS)) {
		l.insert(l.begin(), qi);
	} else {
		l.push_back(qi);
	}
}

inline bool hasFreeSegments(QueueItem* qi, const User::Ptr& aUser) {
	return !qi->isSet(QueueItem::FLAG_MULTI_SOURCE) ||
				((qi->getActiveSegments().size() < qi->getMaxSegments()) &&
				(!BOOLSETTING(DONT_BEGIN_SEGMENT) || (SETTING(DONT_BEGIN_SEGMENT_SPEED)*1024 >= qi->getAverageSpeed())) &&
				(qi->chunkInfo->hasFreeBlock() || !(*(qi->getSource(aUser)))->getPartialInfo().empty()));
}

QueueItem* QueueManager::UserQueue::getNext(const User::Ptr& aUser, QueueItem::Priority minPrio, QueueItem* pNext /* = NULL */) {
	int p = QueueItem::LAST - 1;
	bool fNext = false;
#ifdef _DEBUG
	QueueItem* next = pNext;
#endif

	do {
		QueueItem::UserListIter i = userQueue[p].find(aUser);
		if(i != userQueue[p].end()) {
			dcassert(!i->second.empty());
			QueueItem* found = i->second.front();

			bool freeSegments = hasFreeSegments(found, aUser);

			if(freeSegments && (pNext == NULL || fNext)) {
				dcassert(found != next);
				return found;
			}else{
				if(!freeSegments) {
					if(fNext || (pNext == NULL)) {
						pNext = found;
					}
				}

				QueueItem::Iter iQi = find(i->second.begin(), i->second.end(), pNext);

				while(iQi != i->second.end()) {
	                fNext = true;   // found, next is target
	
					iQi++;
					if((iQi != i->second.end()) && hasFreeSegments(*iQi, aUser)) {
						dcassert(*iQi != next);
						return *iQi;
					}
				}
			}
		}
		p--;
	} while(p >= minPrio);

	return NULL;
}

void QueueManager::UserQueue::setRunning(QueueItem* qi, const User::Ptr& aUser) {

	dcassert(qi->isSource(aUser));
	if(qi->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
		QueueItem::UserListMap& ulm = userQueue[qi->getPriority()];
		QueueItem::UserListIter j = ulm.find(aUser);
		dcassert(j != ulm.end());
		QueueItem::List& l = j->second;
		dcassert(find(l.begin(), l.end(), qi) != l.end());
		l.erase(find(l.begin(), l.end(), qi));
	
		if(l.empty()) {
			ulm.erase(j);
		}
	} else {
		// Remove the download from the userQueue...
		remove(qi);
	}

	// Set the flag to running...
	qi->setStatus(QueueItem::STATUS_RUNNING);
    qi->addCurrent(aUser);
    
	// Move the download to the running list...
	dcassert(running.find(aUser) == running.end());
	running[aUser] = qi;

}

void QueueManager::UserQueue::setWaiting(QueueItem* qi, const User::Ptr& aUser, bool removeSegment /* = true */) {
	// This might have been set to wait by removesource already...
	if (running.find(aUser) == running.end()) {
		QueueItem::UserListMap& ulm = userQueue[qi->getPriority()];
		QueueItem::UserListIter j = ulm.find(aUser);
		if((j == ulm.end()) && qi->isSource(aUser))
			add(qi, aUser);
		return;
	}

	dcassert(qi->getStatus() == QueueItem::STATUS_RUNNING);

	// Remove the download from running
	running.erase(aUser);
	
	// Set flag to waiting
	qi->removeCurrent(qi->isSet(QueueItem::FLAG_MULTI_SOURCE) ? aUser : qi->getCurrents()[0]->getUser());
	if(removeSegment) qi->removeActiveSegment(aUser);
	
	if(qi->getCurrents().empty() || !qi->isSet(QueueItem::FLAG_MULTI_SOURCE)){
		qi->setStatus(QueueItem::STATUS_WAITING);
		qi->setAverageSpeed(0);
		if(removeSegment) {
			qi->setStart(0);
		}
	}

   	// Add to the userQueue
	if(qi->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
		add(qi, aUser);
	} else {
		add(qi);
	}
}

QueueItem* QueueManager::UserQueue::getRunning(const User::Ptr& aUser) {
	QueueItem::UserIter i = running.find(aUser);
	return (i == running.end()) ? NULL : i->second;
}

QueueItem* QueueManager::getRunning(const User::Ptr& aUser) {
	Lock l(cs);
	return userQueue.getRunning(aUser);
}

bool QueueManager::setActiveSegment(const User::Ptr& aUser) {
	{
		Lock l(cs);
		QueueItem* qi = userQueue.getRunning(aUser);

		if(qi == NULL) {
			return false;
		}

		unsigned int SegmentsCount = qi->getActiveSegments().size();

		bool isAlreadyActive = find(qi->getActiveSegments().begin(), qi->getActiveSegments().end(), *qi->getSource(aUser)) != qi->getActiveSegments().end();
		if(!isAlreadyActive) {
			if(qi->isSet(QueueItem::FLAG_MULTI_SOURCE) && (qi->getMaxSegments() <= SegmentsCount)) {
				return false;
			}
	
			qi->addActiveSegment(aUser);
			SegmentsCount += 1;
			if(SegmentsCount == 1) {
				qi->setStart(GET_TICK());
			}
		}
	}
	return true;
}

void QueueManager::UserQueue::remove(QueueItem* qi) {
 	if(!qi->isSet(QueueItem::FLAG_MULTI_SOURCE) && (qi->getStatus() == QueueItem::STATUS_RUNNING)) {
		dcassert(qi->getCurrents()[0] != NULL);
		remove(qi, qi->getCurrents()[0]->getUser());
	} else {
		for(QueueItem::Source::Iter i = qi->getSources().begin(); i != qi->getSources().end(); ++i) {
			remove(qi, (*i)->getUser());
		}
	}
}

void QueueManager::UserQueue::remove(QueueItem* qi, const User::Ptr& aUser) {
		// Remove from running...
	if((!qi->isSet(QueueItem::FLAG_MULTI_SOURCE) && (qi->getStatus() == QueueItem::STATUS_RUNNING)) || qi->isCurrent(aUser)){
		dcassert(!qi->getCurrents().empty());
		dcassert(running.find(aUser) != running.end());
		running.erase(aUser);
		
		if(qi->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
			qi->removeCurrent(aUser);

			if(qi->getCurrents().empty())
				qi->setStatus(QueueItem::STATUS_WAITING);
		}
	} else {
		dcassert(qi->isSource(aUser));
		QueueItem::UserListMap& ulm = userQueue[qi->getPriority()];
		QueueItem::UserListIter j = ulm.find(aUser);
		dcassert(j != ulm.end());
		QueueItem::List& l = j->second;
		dcassert(find(l.begin(), l.end(), qi) != l.end());
		l.erase(find(l.begin(), l.end(), qi));

		if(l.empty()) {
			ulm.erase(j);
		}
	}
}

QueueManager::QueueManager() : lastSave(0), queueFile(Util::getAppPath() + SETTINGS_DIR + "Queue.xml"), dirty(true), nextSearch(0) { 
	TimerManager::getInstance()->addListener(this); 
	SearchManager::getInstance()->addListener(this);
	ClientManager::getInstance()->addListener(this);

	File::ensureDirectory(Util::getAppPath() + FILELISTS_DIR);
	ensurePrivilege();
}

QueueManager::~QueueManager() throw() { 
	SearchManager::getInstance()->removeListener(this);
	TimerManager::getInstance()->removeListener(this); 
	ClientManager::getInstance()->removeListener(this);

	saveQueue();

	if(!BOOLSETTING(KEEP_LISTS)) {
		string path = Util::getAppPath() + FILELISTS_DIR;

#ifdef _WIN32
		WIN32_FIND_DATA data;
		HANDLE hFind;
		
		hFind = FindFirstFile(Text::toT(path + "\\*.xml.bz2").c_str(), &data);
		if(hFind != INVALID_HANDLE_VALUE) {
			do {
				File::deleteFile(path + Text::fromT(data.cFileName));			
			} while(FindNextFile(hFind, &data));
			
			FindClose(hFind);
		}
		
		hFind = FindFirstFile(Text::toT(path + "\\*.DcLst").c_str(), &data);
		if(hFind != INVALID_HANDLE_VALUE) {
			do {
				File::deleteFile(path + Text::fromT(data.cFileName));			
			} while(FindNextFile(hFind, &data));
			
			FindClose(hFind);
		}

#else
		DIR* dir = opendir(path.c_str());
		if (dir) {
			while (struct dirent* ent = readdir(dir)) {
				if (fnmatch("*.xml.bz2", ent->d_name, 0) == 0 ||
					fnmatch("*.DcLst", ent->d_name, 0) == 0) {
					File::deleteFile(path + ent->d_name);	
				}
			}
			closedir(dir);
		}
#endif
	}
}

void QueueManager::on(TimerManagerListener::Minute, u_int32_t aTick) throw() {
	string searchString;
	{
		Lock l(cs);
		QueueItem::UserMap& um = userQueue.getRunning();

		for(QueueItem::UserIter j = um.begin(); j != um.end(); ++j) {
			QueueItem* q = j->second;
			if(!q->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
				dcassert(q->getCurrentDownload() != NULL);
				q->setDownloadedBytes(q->getCurrentDownload()->getPos());
			}
			if(q->getAutoPriority())
				setPriority(q->getTarget(), q->calculateAutoPriority());
		}
		if(!um.empty())
			setDirty();

		if(BOOLSETTING(AUTO_SEARCH) && (aTick >= nextSearch) && (fileQueue.getSize() > 0)) {
			// We keep 32 recent searches to avoid duplicate searches
			while((recent.size() >= fileQueue.getSize()) || (recent.size() > 32)) {
				recent.erase(recent.begin());
			}

			QueueItem* qi = fileQueue.findAutoSearch(recent);
			if(qi == NULL && (recent.size() > 0 && fileQueue.getSize() > 0)) {
				recent.erase(recent.begin());
				qi = fileQueue.findAutoSearch(recent);
            }
			if(qi != NULL && qi->getTTH()) {
				searchString = qi->getTTH()->toBase32();
				recent.push_back(qi->getTarget());
				nextSearch = aTick + (SETTING(SEARCH_TIME) * 60000);
				if(BOOLSETTING(REPORT_ALTERNATES))
					LogManager::getInstance()->message(CSTRING(ALTERNATES_SEND) + Util::getFileName(qi->getTargetFileName()), true);		
			} else {
				recent.clear();
			}
		}
	}

	if(!searchString.empty()) {
		SearchManager::getInstance()->search(searchString, 0, SearchManager::TYPE_TTH, SearchManager::SIZE_DONTCARE, "auto");
	}
	if(dirty && ((lastSave + SETTING(AUTOSAVE_QUEUE)*60000) < aTick)) {
		saveQueue();
	}
}

void QueueManager::addList(const User::Ptr& aUser, int aFlags) throw(QueueException, FileException) {
	string target = Util::getAppPath() + "FileLists\\" + Util::validateFileName(aUser->getFirstNick()) + "." + aUser->getCID().toBase32();

	add(target, -1, NULL, aUser, USER_LIST_NAME, true, QueueItem::FLAG_USER_LIST | aFlags);
}

void QueueManager::addPfs(const User::Ptr aUser, const string& aDir) throw() {
	if(!aUser->isOnline() || aUser->getCID().isZero())
		return;

	{
		Lock l(cs);
		pair<PfsIter, PfsIter> range = pfsQueue.equal_range(aUser->getCID());
		if(find_if(range.first, range.second, CompareSecond<CID, string>(aDir)) == range.second) {
			pfsQueue.insert(make_pair(aUser->getCID(), aDir));
		}
	}

	ConnectionManager::getInstance()->getDownloadConnection(aUser);
}

void QueueManager::add(const string& aTarget, int64_t aSize, const TTHValue* root, User::Ptr aUser, const string& aSourceFile,
					   bool utf8, int aFlags /* = QueueItem::FLAG_RESUME */, bool addBad /* = true */) throw(QueueException, FileException) 
{
	bool wantConnection = true;
	bool newItem = false;
	dcassert((aSourceFile != USER_LIST_NAME) || (aFlags &QueueItem::FLAG_USER_LIST));

	// Check that we're not downloading from ourselves...
	if(aUser == ClientManager::getInstance()->getMe()) {
		throw QueueException(STRING(NO_DOWNLOADS_FROM_SELF));
	}

	// Check if we're not downloading something already in our share
	if (BOOLSETTING(DONT_DL_ALREADY_SHARED) && root != NULL){
		if (ShareManager::getInstance()->isTTHShared(*root)){
			throw QueueException(STRING(TTH_ALREADY_SHARED));
		}
	}
    
	string target = checkTarget(aTarget, aSize, aFlags);

	// Check if it's a zero-byte file, if so, create and return...
	if(aSize == 0) {
		if(!BOOLSETTING(SKIP_ZERO_BYTE)) {
			File f(target, File::WRITE, File::CREATE);
		}
		return;
	}

	{
		Lock l(cs);

		QueueItem* q = NULL;
		
		if(root) {
			QueueItem::List ql;
			fileQueue.find(ql, *root);
			if(!ql.empty()){
				q = ql[0];
			}
		}

		if(q == NULL)
			q = fileQueue.find(target);

		if(q == NULL) {
			q = fileQueue.add(target, aSize, aFlags, QueueItem::DEFAULT, Util::emptyString, 0, GET_TIME(), Util::emptyString, Util::emptyString, root);
			fire(QueueManagerListener::Added(), q);

			newItem = true;
		} else {

			if(q->getSources().size() >= (unsigned int)SETTING(MAX_SOURCES)) {
				throw QueueException("Too Many Sources");
			}

			// first source set length
			if(q->getSize() == 0 && root && q->getTTH() && (*root == *q->getTTH())){
				FileChunksInfo::Ptr fi = FileChunksInfo::Get(q->getTempTarget());
				if(fi){
					fi->setFileSize(aSize);
					q->setSize(aSize);
				}
			}

			if(q->getSize() != aSize) {
				throw QueueException(STRING(FILE_WITH_DIFFERENT_SIZE));
			}
			if(root != NULL) {
				if(q->getTTH() == NULL) {
					q->setTTH(new TTHValue(*root));
				} else if(!(*root == *q->getTTH())) {
					throw QueueException(STRING(FILE_WITH_DIFFERENT_TTH));
				}
			}

			aFlags &= ~QueueItem::FLAG_MULTI_SOURCE;
			q->setFlag(aFlags);

			// We don't add any more sources to user list downloads...
			if(q->isSet(QueueItem::FLAG_USER_LIST))
				return;
		}

		wantConnection = addSource(q, aSourceFile, aUser, addBad ? QueueItem::Source::FLAG_MASK : 0, utf8);
	}

	if(wantConnection && aUser->isOnline())
		ConnectionManager::getInstance()->getDownloadConnection(aUser);

	// auto search, prevent DEADLOCK
	if(newItem && root && BOOLSETTING(AUTO_SEARCH)){
		SearchManager::getInstance()->search(TTHValue(*root).toBase32(), 0, SearchManager::TYPE_TTH, SearchManager::SIZE_DONTCARE, "auto");
	}
	
}

void QueueManager::readd(const string& target, User::Ptr aUser) throw(QueueException) {
	bool wantConnection = false;
	{
		Lock l(cs);
		QueueItem* q = fileQueue.find(target);
		if(q != NULL && q->isBadSource(aUser)) {
			QueueItem::Source* s = *q->getBadSource(aUser);
			wantConnection = addSource(q, s->getPath(), aUser, QueueItem::Source::FLAG_MASK, s->isSet(QueueItem::Source::FLAG_UTF8));
		}
	}
	if(wantConnection && aUser->isOnline())
		ConnectionManager::getInstance()->getDownloadConnection(aUser);
}

string QueueManager::checkTarget(const string& aTarget, int64_t aSize, int& flags) throw(QueueException, FileException) {
#ifdef _WIN32
	if(aTarget.length() > MAX_PATH) {
		throw QueueException(STRING(TARGET_FILENAME_TOO_LONG));
	}
	// Check that target starts with a drive or is an UNC path
	if( (aTarget[1] != ':' || aTarget[2] != '\\') &&
		(aTarget[0] != '\\' && aTarget[1] != '\\') ) {
		throw QueueException(STRING(INVALID_TARGET_FILE));
	}
#else
	if(aTarget.length() > PATH_MAX) {
		throw QueueException(STRING(TARGET_FILENAME_TOO_LONG));
	}
	// Check that target contains at least one directory...we don't want headless files...
	if(aTarget[0] != '/') {
		throw QueueException(STRING(INVALID_TARGET_FILE));
	}
#endif

	string target = Util::validateFileName(aTarget);

	// Check that the file doesn't already exist...
	int64_t sz = File::getSize(target);
	if( (aSize != -1) && (aSize <= sz) )  {
		throw FileException(STRING(LARGER_TARGET_FILE_EXISTS));
	}
	if(sz > 0)
		flags |= QueueItem::FLAG_EXISTS;

	return target;
}

/** Add a source to an existing queue item */
bool QueueManager::addSource(QueueItem* qi, const string& aFile, User::Ptr aUser, Flags::MaskType addBad, bool utf8) throw(QueueException, FileException) {
	QueueItem::Source* s = NULL;
	bool wantConnection = (qi->getPriority() != QueueItem::PAUSED);

	if(qi->isSource(aUser)) {
		throw QueueException(STRING(DUPLICATE_SOURCE));
	}

	if(qi->isBadSourceExcept(aUser, addBad)) {
		throw QueueException(STRING(DUPLICATE_SOURCE));
	}

	if(qi->getTTH() && aUser->isSet(User::TTH_GET))
		s = qi->addSource(aUser, Util::emptyString);
	else
		s = qi->addSource(aUser, aFile);

	if(utf8)
		s->setFlag(QueueItem::Source::FLAG_UTF8);

	if(aUser->isSet(User::PASSIVE) && !ClientManager::getInstance()->isActive(aUser->getClient()) ) {
		qi->removeSource(aUser, QueueItem::Source::FLAG_PASSIVE);
		wantConnection = false;
	} else if(qi->isSet(QueueItem::FLAG_MULTI_SOURCE) || (qi->getStatus() != QueueItem::STATUS_RUNNING)) {
		if ((!SETTING(SOURCEFILE).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
			PlaySound(Text::toT(SETTING(SOURCEFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
		userQueue.add(qi, aUser);
	} else {
		wantConnection = false;
	}

	fire(QueueManagerListener::SourcesUpdated(), qi);
	setDirty();

	return wantConnection;
}

void QueueManager::addDirectory(const string& aDir, const User::Ptr& aUser, const string& aTarget, QueueItem::Priority p /* = QueueItem::DEFAULT */) throw() {
	bool needList;
	{
		Lock l(cs);
		
		DirectoryItem::DirectoryPair dp = directories.equal_range(aUser);
		
		for(DirectoryItem::DirectoryIter i = dp.first; i != dp.second; ++i) {
			if(Util::stricmp(aTarget.c_str(), i->second->getName().c_str()) == 0)
				return;
		}
		
		// Unique directory, fine...
		directories.insert(make_pair(aUser, new DirectoryItem(aUser, aDir, aTarget, p)));
		needList = (dp.first == dp.second);
		setDirty();
	}

	if(needList) {
		try {
			addList(aUser, QueueItem::FLAG_DIRECTORY_DOWNLOAD);
		} catch(const Exception&) {
			// Ignore, we don't really care...
		}
	}
}

#define isnum(c) (((c) >= '0') && ((c) <= '9'))

static inline u_int32_t adjustSize(u_int32_t sz, const string& name) {
	if(name.length() > 2) {
		// filename.r32
		u_int8_t c1 = (u_int8_t)name[name.length()-2];
		u_int8_t c2 = (u_int8_t)name[name.length()-1];
		if(isnum(c1) && isnum(c2)) {
			return sz + (c1-'0')*10 + (c2-'0');
		} else if(name.length() > 6) {
			// filename.part32.rar
			c1 = name[name.length() - 6];
			c2 = name[name.length() - 5];
			if(isnum(c1) && isnum(c2)) {
				return sz + (c1-'0')*10 + (c2-'0');
			}
		}
	} 

	return sz;
}

typedef HASH_MULTIMAP<u_int32_t, QueueItem*> SizeMap;
typedef SizeMap::iterator SizeIter;
typedef pair<SizeIter, SizeIter> SizePair;

// *** WARNING *** 
// Lock(cs) makes sure that there's only one thread accessing these,
// I put them here to avoid growing a huge stack...

static const DirectoryListing* curDl = NULL;
static SizeMap sizeMap;

int QueueManager::matchFiles(const DirectoryListing::Directory* dir) throw() {
	int matches = 0;
	for(DirectoryListing::Directory::List::const_iterator j = dir->directories.begin(); j != dir->directories.end(); ++j) {
		if(!(*j)->getAdls())
			matches += matchFiles(*j);
	}

	for(DirectoryListing::File::List::const_iterator i = dir->files.begin(); i != dir->files.end(); ++i) {
		const DirectoryListing::File* df = *i;

		SizePair files = sizeMap.equal_range(adjustSize((u_int32_t)df->getSize(), df->getName()));
		for(SizeIter j = files.first; j != files.second; ++j) {
			QueueItem* qi = j->second;
			bool equal = false;
			if(qi->getTTH() != NULL && df->getTTH() != NULL) {
				equal = (*qi->getTTH() == *df->getTTH()) && (qi->getSize() == df->getSize());
			}
			if(equal) {
				try {
					addSource(qi, curDl->getPath(df) + df->getName(), curDl->getUser(), QueueItem::Source::FLAG_FILE_NOT_AVAILABLE, curDl->getUtf8());
					matches++;
				} catch(const Exception&) {
				}
			}
		}
	}
	return matches;
}

int QueueManager::matchListing(const DirectoryListing& dl) throw() {
	int matches = 0;
	{
		Lock l(cs);
		sizeMap.clear();
		matches = 0;
		curDl = &dl;
		for(QueueItem::StringIter i = fileQueue.getQueue().begin(); i != fileQueue.getQueue().end(); ++i) {
			QueueItem* qi = i->second;
			if(qi->getSize() != -1) {
				sizeMap.insert(make_pair(adjustSize((u_int32_t)qi->getSize(), qi->getTarget()), qi));
			}
		}

		matches = matchFiles(dl.getRoot());
	}
	if(matches > 0)
		ConnectionManager::getInstance()->getDownloadConnection(dl.getUser());
		return matches;
}

void QueueManager::move(const string& aSource, const string& aTarget) throw() {
	string target = Util::validateFileName(aTarget);
	if(Util::stricmp(aSource, target) == 0)
		return;

	bool delSource = false;

	Lock l(cs);
	QueueItem* qs = fileQueue.find(aSource);
	if(qs != NULL) {
		// Don't move running downloads
		if(qs->getStatus() == QueueItem::STATUS_RUNNING) {
			return;
		}
		// Don't move file lists
		if(qs->isSet(QueueItem::FLAG_USER_LIST))
			return;

		// Let's see if the target exists...then things get complicated...
		QueueItem* qt = fileQueue.find(target);
		if(qt == NULL) {
			// Good, update the target and move in the queue...
			fileQueue.move(qs, target);
			fire(QueueManagerListener::Moved(), qs);
			setDirty();
		} else {
			// Don't move to target of different size
			if(qs->getSize() != qt->getSize())
				return;

			try {
				for(QueueItem::Source::Iter i = qs->getSources().begin(); i != qs->getSources().end(); ++i) {
					QueueItem::Source* s = *i;
					addSource(qt, s->getPath(), s->getUser(), QueueItem::Source::FLAG_MASK, s->isSet(QueueItem::Source::FLAG_UTF8));
				}
			} catch(const Exception&) {
			}
			delSource = true;
		}
	}

	if(delSource) {
		remove(aSource);
	}
}

bool QueueManager::getQueueInfo(User::Ptr& aUser, string& aTarget, int64_t& aSize, int& aFlags) throw() {
    Lock l(cs);
    QueueItem* qi = userQueue.getNext(aUser);
	if(qi == NULL)
		return false;

	aTarget = qi->getTarget();
	aSize = qi->getSize();
	aFlags = qi->getFlags();

	return true;
}

int QueueManager::FileQueue::getMaxSegments(string filename, int64_t filesize) {
	int MaxSegments = 1;

	switch(SETTING(SEGMENTS_TYPE)) {
	case SettingsManager::SEGMENT_ON_SIZE : {
		if((filesize >= (SETTING(SET_MIN2)*1048576)) && (filesize < (SETTING(SET_MAX2)*1048576))) {
			MaxSegments = 2;
		} else if((filesize >= (SETTING(SET_MIN3)*1048576)) && (filesize < (SETTING(SET_MAX3)*1048576))) {
			MaxSegments = 3;
		} else if( (filesize >= (SETTING(SET_MIN4)*1048576)) && (filesize < (SETTING(SET_MAX4)*1048576))) {
			MaxSegments = 4;
		} else if( (filesize >= (SETTING(SET_MIN6)*1048576)) && (filesize < (SETTING(SET_MAX6)*1048576))) {
			MaxSegments = 6;
		} else if(filesize >= (SETTING(SET_MIN8)*1048576)) {
			MaxSegments = 8;
		}
			break;
		}
	case SettingsManager::SEGMENT_ON_CONNECTION : {
		if (SETTING(CONNECTION) == SettingsManager::connectionSpeeds[SettingsManager::SPEED_MODEM]) {
			MaxSegments = 2;
		} else if(SETTING(CONNECTION) == SettingsManager::connectionSpeeds[SettingsManager::SPEED_ISDN]) {
			MaxSegments = 3;
		} else if(SETTING(CONNECTION) == SettingsManager::connectionSpeeds[SettingsManager::SPEED_SATELLITE]) {
			MaxSegments = 4;
		} else if(SETTING(CONNECTION) == SettingsManager::connectionSpeeds[SettingsManager::SPEED_WIRELESS]) {
			MaxSegments = 5;
		} else if(SETTING(CONNECTION) == SettingsManager::connectionSpeeds[SettingsManager::SPEED_DSL]) {
			MaxSegments = 6;
		} else if(SETTING(CONNECTION) == SettingsManager::connectionSpeeds[SettingsManager::SPEED_CABLE]) {
			MaxSegments = 6;
		} else if(SETTING(CONNECTION) == SettingsManager::connectionSpeeds[SettingsManager::SPEED_T1]) {
			MaxSegments = 8;
		} else if(SETTING(CONNECTION) == SettingsManager::connectionSpeeds[SettingsManager::SPEED_T3]) {
			MaxSegments = 10;
		}
			break;
		}
	case SettingsManager::SEGMENT_MANUAL : {
			MaxSegments = min(SETTING(NUMBER_OF_SEGMENTS), 10);
			break;
		}
	} 

	if(!SETTING(DONT_EXTENSIONS).empty()) {
		PME reg(SETTING(DONT_EXTENSIONS),"i");
		if(reg.IsValid()) {
			if(reg.match(Util::getFileExt(filename))) {
				MaxSegments = 1;
			}
		}
	}

#ifdef _DEBUG
	return 500;
#else
	return MaxSegments;
#endif
}

void QueueManager::getTargetsBySize(StringList& sl, int64_t aSize, const string& suffix) throw() {
	Lock l(cs);
	QueueItem::List ql;
	fileQueue.find(ql, aSize, suffix);
	for(QueueItem::Iter i = ql.begin(); i != ql.end(); ++i) {
		sl.push_back((*i)->getTarget());
	}
}

void QueueManager::getTargetsByRoot(StringList& sl, const TTHValue& tth) {
	Lock l(cs);
	QueueItem::List ql;
	fileQueue.find(ql, tth);
	for(QueueItem::Iter i = ql.begin(); i != ql.end(); ++i) {
		sl.push_back((*i)->getTarget());
	}
}

Download* QueueManager::getDownload(User::Ptr& aUser, bool supportsTrees, bool supportsChunks, string &message, string aTarget) throw() {
	Lock l(cs);

	// First check PFS's...
	/*PfsIter pi = pfsQueue.find(aUser->getCID());
	if(pi != pfsQueue.end()) {
		Download* d = new Download();
		d->setFlag(Download::FLAG_PARTIAL_LIST);
		d->setFlag(Download::FLAG_UTF8);
		d->setSource(pi->second);
		return d;
	}*/

	QueueItem* q = NULL;
	bool nextChunk = !aTarget.empty();

	if(nextChunk) {
		q = fileQueue.find(aTarget);
	} else {
		q = userQueue.getNext(aUser);

again:

		if(q == NULL)
			return NULL;

		if((SETTING(FILE_SLOTS) != 0) && (q->getStatus() == QueueItem::STATUS_WAITING) && !q->isSet(QueueItem::FLAG_TESTSUR) &&
			!q->isSet(QueueItem::FLAG_MP3_INFO) && !q->isSet(QueueItem::FLAG_USER_LIST) && (getRunningFiles().size() >= (unsigned int)SETTING(FILE_SLOTS))) {
			message = STRING(ALL_FILE_SLOTS_TAKEN);
			q = userQueue.getNext(aUser, QueueItem::LOWEST, q);
			goto again;
		}
	}
	
	int64_t freeBlock = 0;

	QueueItem::Source* source = *(q->getSource(aUser));
	if(q->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
		dcassert(!q->getTempTarget().empty());

		freeBlock = q->chunkInfo->getChunk(source->getPartialInfo(), aUser->getOnlineUser() ? Util::toInt64(aUser->getOnlineUser()->getIdentity().get("US")) : 0);

		if(freeBlock < 0) {
			if(freeBlock == -2) {
				userQueue.remove(q, aUser);
				q->removeSource(aUser, QueueItem::Source::FLAG_NO_NEED_PARTS);
				message = STRING(NO_NEEDED_PART);
			} else
				message = STRING(NO_FREE_BLOCK);
			
			if(nextChunk) {
				nextChunk = false;
				q->removeActiveSegment(aUser);
				q = userQueue.getNext(aUser);
			} else {
				q = userQueue.getNext(aUser, QueueItem::LOWEST, q);
			}
			goto again;
		}
	}

	userQueue.setRunning(q, aUser);

	fire(QueueManagerListener::StatusUpdated(), q);

	Download* d = new Download(q, aUser, source);
	
	if((d->getSize() != -1) && d->getTTH() && !d->isSet(Download::FLAG_PARTIAL)) {
		if(HashManager::getInstance()->getTree(*d->getTTH(), d->getTigerTree())) {
			d->setTreeValid(true);
			q->setHasTree(true);
		} else if(supportsTrees && !source->isSet(QueueItem::Source::FLAG_NO_TREE) && d->getSize() > HashManager::MIN_BLOCK_SIZE) {
			// Get the tree unless the file is small (for small files, we'd probably only get the root anyway)
			d->setFlag(Download::FLAG_TREE_DOWNLOAD);
			d->getTigerTree().setFileSize(d->getSize());
			d->setPos(0);
			d->setSize(-1);
			d->unsetFlag(Download::FLAG_RESUME);
			
			if(q->chunkInfo)
				q->chunkInfo->putChunk(freeBlock);
		} else {
			// Use the root as tree to get some sort of validation at least...
			d->getTigerTree() = TigerTree(d->getSize(), d->getSize(), *d->getTTH());
			d->setTreeValid(true);
		}
	}

	if(q->isSet(QueueItem::FLAG_MULTI_SOURCE) && !d->isSet(Download::FLAG_TREE_DOWNLOAD)) {
		Client* c = aUser->getOnlineUser() ? &aUser->getOnlineUser()->getClient() : NULL;
		supportsChunks = supportsChunks && (!c || !c->getStealth()) && 
			((q->chunkInfo->getDownloadedSize() > 0.85*q->getSize()) || (q->getActiveSegments().size() > 1));
		d->setStartPos(freeBlock);
		q->chunkInfo->setDownload(freeBlock, d, supportsChunks);
	} else {
		if(!d->isSet(Download::FLAG_TREE_DOWNLOAD) && BOOLSETTING(ANTI_FRAG) ) {
			d->setStartPos(q->getDownloadedBytes());
		}		
		q->setCurrentDownload(d);
	}

	return d;
}

void QueueManager::putDownload(Download* aDownload, bool finished, bool removeSegment /* = true */) throw() {
	User::List getConn;
	string fname;
	User::Ptr up;
	int flag = 0;
	bool checkList = false;
	User::Ptr uzivatel;

	{
		Lock l(cs);

		aDownload->setQI(NULL);

		if(aDownload->isSet(Download::FLAG_PARTIAL_LIST)) {
			pair<PfsIter, PfsIter> range = pfsQueue.equal_range(aDownload->getUserConnection()->getUser()->getCID());
			PfsIter i = find_if(range.first, range.second, CompareSecond<CID, string>(aDownload->getSource()));
			if(i != range.second) {
				pfsQueue.erase(i);
				fire(QueueManagerListener::PartialList(), aDownload->getUserConnection()->getUser(), aDownload->getPFS());
			}
		} else {
			if(aDownload->isSet(Download::FLAG_TREE_DOWNLOAD)) {
				removeSegment = false;
			}
			QueueItem* q = fileQueue.find(aDownload->getTarget());

			if(q != NULL) {
				if(aDownload->isSet(Download::FLAG_USER_LIST)) {
					if(aDownload->getSource() == "files.xml.bz2") {
						q->setFlag(QueueItem::FLAG_XML_BZLIST);
					} else {
						q->unsetFlag(QueueItem::FLAG_XML_BZLIST);
					}
				}


				if(finished) {
					dcassert(q->getStatus() == QueueItem::STATUS_RUNNING);
					if(aDownload->isSet(Download::FLAG_TREE_DOWNLOAD)) {
						// Got a full tree, now add it to the HashManager
						dcassert(aDownload->getTreeValid());
						HashManager::getInstance()->addTree(aDownload->getTigerTree());

						q->setHasTree(true);
						q->setCurrentDownload(NULL);
						userQueue.setWaiting(q, aDownload->getUserConnection()->getUser(), removeSegment);
	
						fire(QueueManagerListener::StatusUpdated(), q);
					} else {
						userQueue.remove(q);
						fire(QueueManagerListener::Finished(), q);
						fire(QueueManagerListener::Removed(), q);
						// Now, let's see if this was a directory download filelist...
						if( (q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD) && directories.find(q->getCurrents()[0]->getUser()) != directories.end()) ||
							(q->isSet(QueueItem::FLAG_MATCH_QUEUE)) ) 
						{
							fname = q->getListName();
							up = q->getCurrents()[0]->getUser();
							flag = (q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD) ? QueueItem::FLAG_DIRECTORY_DOWNLOAD : 0)
								| (q->isSet(QueueItem::FLAG_MATCH_QUEUE) ? QueueItem::FLAG_MATCH_QUEUE : 0);
						} 
						fileQueue.remove(q);
						setDirty();
					}
				} else {
					if(!aDownload->isSet(Download::FLAG_TREE_DOWNLOAD)) {
						if(!q->isSet(QueueItem::FLAG_MULTI_SOURCE))
							q->setDownloadedBytes(aDownload->getPos());
	
						if(q->getDownloadedBytes() > 0) {
							q->setFlag(QueueItem::FLAG_EXISTS);
						} else if(!q->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
							q->setTempTarget(Util::emptyString);
						}
						if(q->isSet(QueueItem::FLAG_USER_LIST)) {
							// Blah...no use keeping an unfinished file list...
							File::deleteFile(q->getListName());
						}
					}

					if(removeSegment && (q->getPriority() != QueueItem::PAUSED)) {
						for(QueueItem::Source::Iter j = q->getSources().begin(); j != q->getSources().end(); ++j) {
							if((*j)->getUser()->isOnline() && false == q->isCurrent((*j)->getUser())) {
								getConn.push_back((*j)->getUser());
							}
						}
					}
	
					if(!q->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
						q->setCurrentDownload(NULL);
	
						// This might have been set to wait by removesource already...
						if(q->getStatus() == QueueItem::STATUS_RUNNING) {
							userQueue.setWaiting(q, aDownload->getUserConnection()->getUser(), removeSegment);
							fire(QueueManagerListener::StatusUpdated(), q);						
						}
					} else {
						userQueue.setWaiting(q, aDownload->getUserConnection()->getUser(), removeSegment);
						if(q->getStatus() != QueueItem::STATUS_RUNNING) {
							fire(QueueManagerListener::StatusUpdated(), q);
						}
					}
				}
			} else if(!aDownload->isSet(Download::FLAG_TREE_DOWNLOAD)) {
				if(!aDownload->getTempTarget().empty() && aDownload->getTempTarget() != aDownload->getTarget()) {
					File::deleteFile(aDownload->getTempTarget() + Download::ANTI_FRAG_EXT);
					File::deleteFile(aDownload->getTempTarget());
				}
			}
		}
	
		if(aDownload->isSet(Download::FLAG_MULTI_CHUNK) && 
			!aDownload->isSet(Download::FLAG_TREE_DOWNLOAD)) {
			
			FileChunksInfo::Ptr fileChunks = FileChunksInfo::Get(aDownload->getTempTarget());
			if(!(fileChunks == (FileChunksInfo*)NULL)){
				fileChunks->putChunk(aDownload->getStartPos());
			}
		}

		int64_t speed = aDownload->getAverageSpeed();
		if(aDownload->getUserConnection()->getUser()->getOnlineUser() && (speed > 0 && aDownload->getTotal() > 32768 && speed < 10485760)) {
			aDownload->getUserConnection()->getUser()->getOnlineUser()->getIdentity().set("US", Util::toString(speed));
		}
		
		checkList = aDownload->isSet(Download::FLAG_CHECK_FILE_LIST) && aDownload->isSet(Download::FLAG_TESTSUR);
		uzivatel = aDownload->getUserConnection()->getUser();

		aDownload->setUserConnection(NULL);
		delete aDownload;
	}

	for(User::Iter i = getConn.begin(); i != getConn.end(); ++i) {
		ConnectionManager::getInstance()->getDownloadConnection(*i);
	}

	if(!fname.empty()) {
		processList(fname, up, flag);
	}

	if(checkList) {
		try {
			QueueManager::getInstance()->addList(uzivatel, QueueItem::FLAG_CHECK_FILE_LIST);
			ClientManager::getInstance()->connect(uzivatel);
		} catch(const Exception&) {}
	}
}

void QueueManager::processList(const string& name, User::Ptr& user, int flags) {
	DirectoryListing dirList(user);
	try {
		dirList.loadFile(name);
	} catch(const Exception&) {
		/** @todo Log this */
		return;
	}

	if(flags & QueueItem::FLAG_DIRECTORY_DOWNLOAD) {
		DirectoryItem::List dl;
		{
			Lock l(cs);
			DirectoryItem::DirectoryPair dp = directories.equal_range(user);
			for(DirectoryItem::DirectoryIter i = dp.first; i != dp.second; ++i) {
				dl.push_back(i->second);
			}
			directories.erase(user);
		}

		for(DirectoryItem::Iter i = dl.begin(); i != dl.end(); ++i) {
			DirectoryItem* di = *i;
			dirList.download(di->getName(), di->getTarget(), false);
			delete di;
		}
	}
	if(flags & QueueItem::FLAG_MATCH_QUEUE) {
			AutoArray<char> tmp(STRING(MATCHED_FILES).size() + 16);
		sprintf(tmp, CSTRING(MATCHED_FILES), matchListing(dirList));
		LogManager::getInstance()->message(user->getFirstNick() + ": " + string(tmp), true);			
	}
}

void QueueManager::remove(const string& aTarget) throw() {
	string x;
	{
		Lock l(cs);

		QueueItem* q = fileQueue.find(aTarget);
		if(q != NULL) {
			if(q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD)) {
				dcassert(q->getSources().size() == 1);
				DirectoryItem::DirectoryPair dp = directories.equal_range(q->getSources()[0]->getUser());
				for(DirectoryItem::DirectoryIter i = dp.first; i != dp.second; ++i) {
					delete i->second;
				}
				directories.erase(q->getSources()[0]->getUser());
			}

			string temptarget = q->getTempTarget();

			// For partial-share
			UploadManager::getInstance()->abortUpload(temptarget);

			if(q->getStatus() == QueueItem::STATUS_RUNNING) {
				x = q->getTarget();
			} else if(!q->getTempTarget().empty() && q->getTempTarget() != q->getTarget()) {
				File::deleteFile(q->getTempTarget() + Download::ANTI_FRAG_EXT);
				File::deleteFile(q->getTempTarget());
			}

			userQueue.remove(q);

			fire(QueueManagerListener::Removed(), q);
			fileQueue.remove(q);

			setDirty();
		}
	}
	if(!x.empty()) {
		DownloadManager::getInstance()->abortDownload(x);
	}
}

void QueueManager::removeSource(const string& aTarget, User::Ptr aUser, int reason, bool removeConn /* = true */) throw() {
	string x;
	{
		Lock l(cs);

		QueueItem* q = fileQueue.find(aTarget);
		if(q != NULL && q->isSource(aUser)) { // Updated by RevConnect
			dcassert(q->isSource(aUser));
			if(q->isSet(QueueItem::FLAG_USER_LIST)) {
				remove(q->getTarget());
				return;
			}

			if(reason == QueueItem::Source::FLAG_NO_TREE) {
				QueueItem::Source* s = *q->getSource(aUser);
				s->setFlag(reason);
				return;
			}

			if(reason == QueueItem::Source::FLAG_CRC_WARN) {
				// Already flagged?
				QueueItem::Source* s = *q->getSource(aUser);
				if(s->isSet(QueueItem::Source::FLAG_CRC_WARN)) {
					reason = QueueItem::Source::FLAG_CRC_FAILED;
				} else {
					s->setFlag(reason);
					return;
				}
			}
			if(q->isCurrent(aUser)) {
				if(removeConn)
					x = q->getTarget();
				userQueue.setWaiting(q, aUser);
				userQueue.remove(q, aUser);
			} else {
				userQueue.remove(q, aUser);
			}

			q->removeSource(aUser, reason);
		
			fire(QueueManagerListener::SourcesUpdated(), q);
			setDirty();
		}
	}
	if(!x.empty()) {
		DownloadManager::getInstance()->abortDownload(x, aUser);
	}
}

void QueueManager::removeSource(User::Ptr aUser, int reason) throw() {
	string x;
	{
		Lock l(cs);
		QueueItem* qi = NULL;
		while( (qi = userQueue.getNext(aUser, QueueItem::PAUSED)) != NULL) {
			if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
				remove(qi->getTarget());
			} else {
				userQueue.remove(qi, aUser);
				qi->removeSource(aUser, reason);
				fire(QueueManagerListener::SourcesUpdated(), qi);
				setDirty();
			}
		}
		
		qi = userQueue.getRunning(aUser);
		if(qi != NULL) {
			if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
				remove(qi->getTarget());
			} else {
				userQueue.setWaiting(qi, aUser);
				userQueue.remove(qi, aUser);
				x = qi->getTarget();
				qi->removeSource(aUser, reason);
				fire(QueueManagerListener::SourcesUpdated(), qi);
				setDirty();
			}
		}
	}
	if(!x.empty()) {
		DownloadManager::getInstance()->abortDownload(x, aUser);
	}
}

void QueueManager::setPriority(const string& aTarget, QueueItem::Priority p) throw() {
	User::List ul;
	bool running = false;

	{
		Lock l(cs);
	
		QueueItem* q = fileQueue.find(aTarget);
		if( (q != NULL) && (q->getPriority() != p) ) {
			if( q->getStatus() != QueueItem::STATUS_RUNNING ) {
				if(q->getPriority() == QueueItem::PAUSED || p == QueueItem::HIGHEST) {
					// Problem, we have to request connections to all these users...
					q->getOnlineUsers(ul);
				}

				userQueue.remove(q);
				q->setPriority(p);
				userQueue.add(q);
			} else {
				running = true;
				if(q->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
					for(QueueItem::Source::Iter i = q->getSources().begin(); i != q->getSources().end(); ++i) {
						if(!q->isCurrent((*i)->getUser())) userQueue.remove(q, (*i)->getUser());
					}
					q->setPriority(p);
					for(QueueItem::Source::Iter i = q->getSources().begin(); i != q->getSources().end(); ++i) {
						if(!q->isCurrent((*i)->getUser())) userQueue.add(q, (*i)->getUser());
					}
				} else {
					q->setPriority(p);
				}
			}
			setDirty();
			fire(QueueManagerListener::StatusUpdated(), q);
		}
	}

	if(p == QueueItem::PAUSED) {
		if(running)
			DownloadManager::getInstance()->abortDownload(aTarget);
	} else {
		for(User::Iter i = ul.begin(); i != ul.end(); ++i) {
			ConnectionManager::getInstance()->getDownloadConnection(*i);
		}
	}
}

void QueueManager::setAutoPriority(const string& aTarget, bool ap) throw() {
	User::List ul;

	{
		Lock l(cs);
	
		QueueItem* q = fileQueue.find(aTarget);
		if( (q != NULL) && (q->getAutoPriority() != ap) ) {
			q->setAutoPriority(ap);
			if(ap) {
				QueueManager::getInstance()->setPriority(q->getTarget(), q->calculateAutoPriority());
			}
			setDirty();
			fire(QueueManagerListener::StatusUpdated(), q);
		}
	}
}

void QueueManager::saveQueue() throw() {

	try {
		
#define STRINGLEN(n) n, sizeof(n)-1
#define CHECKESCAPE(n) SimpleXML::escape(n, tmp, true)

		File ff(getQueueFile() + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		BufferedOutputStream<false> f(&ff);
		
		f.write(SimpleXML::utf8Header);
		f.write(STRINGLEN("<Downloads Version=\"" VERSIONSTRING "\">\r\n"));
		string tmp;
		string b32tmp;

		{
			Lock l(cs);
			for(QueueItem::StringIter i = fileQueue.getQueue().begin(); i != fileQueue.getQueue().end(); ++i) {
				QueueItem* qi = i->second;
				if((!qi->isSet(QueueItem::FLAG_USER_LIST)) && (!qi->isSet(QueueItem::FLAG_TESTSUR)) && (!qi->isSet(QueueItem::FLAG_MP3_INFO))) {
					f.write(STRINGLEN("\t<Download Target=\""));
					f.write(CHECKESCAPE(qi->getTarget()));
					f.write(STRINGLEN("\" Size=\""));
					f.write(Util::toString(qi->getSize()));
					f.write(STRINGLEN("\" Priority=\""));
					f.write(Util::toString((int)qi->getPriority()));
					if(qi->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
						f.write(STRINGLEN("\" FreeBlocks=\""));
						f.write(qi->chunkInfo ? qi->chunkInfo->getFreeChunksString() : "");
						f.write(STRINGLEN("\" VerifiedParts=\""));
						f.write(qi->chunkInfo ? qi->chunkInfo->getVerifiedBlocksString() : "");
					}
					f.write(STRINGLEN("\" Added=\""));
					f.write(Util::toString(qi->getAdded()));
					if(qi->getTTH() != NULL) {
						b32tmp.clear();
						f.write(STRINGLEN("\" TTH=\""));
						f.write(qi->getTTH()->toBase32(b32tmp));
					}
					if(qi->getDownloadedBytes() > 0) {
						f.write(STRINGLEN("\" TempTarget=\""));
						f.write(CHECKESCAPE(qi->getTempTarget()));
						f.write(STRINGLEN("\" Downloaded=\""));
						f.write(Util::toString(qi->getDownloadedBytes()));
					}
					f.write(STRINGLEN("\" AutoPriority=\""));
					f.write(Util::toString(qi->getAutoPriority()));
					if(qi->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
						f.write(STRINGLEN("\" MaxSegments=\""));
						f.write(Util::toString(qi->getMaxSegments()));
					}
	
					f.write(STRINGLEN("\">\r\n"));
	
					for(QueueItem::Source::List::const_iterator j = qi->sources.begin(); j != qi->sources.end(); ++j) {
						QueueItem::Source* s = *j;

						if(s->isSet(QueueItem::Source::FLAG_PARTIAL)) continue;

						/*if(!s->getUser()->getCID().isZero()) {
							s->getUser()->setFlag(User::SAVE_NICK);
							f.write(STRINGLEN("\t\t<Source CID=\""));
							f.write(s->getUser()->getCID().toBase32());
						} else {*/
							f.write(STRINGLEN("\t\t<Source Nick=\""));
							f.write(CHECKESCAPE(s->getUser()->getFirstNick()));
						//}
						if(!s->getPath().empty() && (!s->getUser()->isSet(User::TTH_GET) || !qi->getTTH()) ) {
							f.write(STRINGLEN("\" Path=\""));
							f.write(CHECKESCAPE(s->getPath()));
							f.write(STRINGLEN("\" Utf8=\""));
							f.write(s->isSet(QueueItem::Source::FLAG_UTF8) ? "1" : "0", 1);
						}
						f.write(STRINGLEN("\"/>\r\n"));
					}

					f.write(STRINGLEN("\t</Download>\r\n"));
				}
			}
		}
		f.write("</Downloads>\r\n");
		f.flush();
		ff.close();

		File::deleteFile(getQueueFile() + ".bak");
		CopyFile(Text::toT(getQueueFile()).c_str(), Text::toT(getQueueFile() + ".bak").c_str(), FALSE);
		File::deleteFile(getQueueFile());
		File::renameFile(getQueueFile() + ".tmp", getQueueFile());

		dirty = false;
	} catch(const FileException&) {
		// ...
	}
	// Put this here to avoid very many saves tries when disk is full...
	lastSave = GET_TICK();
}

class QueueLoader : public SimpleXMLReader::CallBack {
public:
	QueueLoader() : cur(NULL), inDownloads(false) { };
	virtual ~QueueLoader() { }
	virtual void startTag(const string& name, StringPairList& attribs, bool simple);
	virtual void endTag(const string& name, const string& data);
private:
	string target;

	QueueItem* cur;
	bool inDownloads;
};

void QueueManager::loadQueue() throw() {
	try {
		QueueLoader l;
		SimpleXMLReader(&l).fromXML(File(getQueueFile(), File::READ, File::OPEN).read());
		dirty = false;
		saveQueue(); // ensure old temp file was converted

	} catch(const Exception&) {
		// ...
	}
}

static const string sDownload = "Download";
static const string sTempTarget = "TempTarget";
static const string sTarget = "Target";
static const string sSize = "Size";
static const string sDownloaded = "Downloaded";
static const string sPriority = "Priority";
static const string sSource = "Source";
static const string sNick = "Nick";
static const string sPath = "Path";
static const string sDirectory = "Directory";
static const string sAdded = "Added";
static const string sUtf8 = "Utf8";
static const string sTTH = "TTH";
static const string sCID = "CID";
static const string sFreeBlocks = "FreeBlocks";
static const string sVerifiedBlocks = "VerifiedParts";
static const string sAutoPriority = "AutoPriority";
static const string sMaxSegments = "MaxSegments";

void QueueLoader::startTag(const string& name, StringPairList& attribs, bool simple) {
	QueueManager* qm = QueueManager::getInstance();	
	if(!inDownloads && name == "Downloads") {
		inDownloads = true;
	} else if(inDownloads) {
		if(cur == NULL && name == sDownload) {
			int flags = QueueItem::FLAG_RESUME;
			int64_t size = Util::toInt64(getAttrib(attribs, sSize, 1));
			if(size == 0)
				return;
			try {
				const string& tgt = getAttrib(attribs, sTarget, 0);
				target = QueueManager::checkTarget(tgt, size, flags);
				if(target.empty())
					return;
			} catch(const Exception&) {
				return;
			}
			const string& freeBlocks = getAttrib(attribs, sFreeBlocks, 1);
			const string& verifiedBlocks = getAttrib(attribs, sVerifiedBlocks, 2);

			QueueItem::Priority p = (QueueItem::Priority)Util::toInt(getAttrib(attribs, sPriority, 3));
			u_int32_t added = (u_int32_t)Util::toInt(getAttrib(attribs, sAdded, 4));
			const string& tthRoot = getAttrib(attribs, sTTH, 5);
			string tempTarget = getAttrib(attribs, sTempTarget, 5);
			int64_t downloaded = Util::toInt64(getAttrib(attribs, sDownloaded, 5));
			int maxSegments = Util::toInt(getAttrib(attribs, sMaxSegments, 5));
			if (downloaded > size || downloaded < 0)
				downloaded = 0;

			if(added == 0)
				added = GET_TIME();

			QueueItem* qi = qm->fileQueue.find(target);

			if(qi == NULL) {
				if(tthRoot.empty()) {
					qi = qm->fileQueue.add(target, size, flags, p, tempTarget, downloaded, added, freeBlocks, verifiedBlocks, NULL);
				} else {
					TTHValue root(tthRoot);
					if((maxSegments > 1) || !freeBlocks.empty()) {
						flags |= QueueItem::FLAG_MULTI_SOURCE;
					}
					qi = qm->fileQueue.add(target, size, flags, p, tempTarget, downloaded, added, freeBlocks, verifiedBlocks, &root);
				}
				bool ap = Util::toInt(getAttrib(attribs, sAutoPriority, 6)) == 1;
				qi->setAutoPriority(ap);
				qi->setMaxSegments(max(1, maxSegments));
				
				qm->fire(QueueManagerListener::Added(), qi);
			}
			if(!simple)
				cur = qi;
		} else if(cur != NULL && name == sSource) {
			const string& nick = getAttrib(attribs, sNick, 0);
			const string& cid = getAttrib(attribs, sCID, 1);
			const string& path = getAttrib(attribs, sPath, 1);
			const string& utf8 = getAttrib(attribs, sUtf8, 2);
			bool isUtf8 = (utf8 == "1");
			User::Ptr user;
			if(!cid.empty())
				user = ClientManager::getInstance()->getUser(CID(cid));
			else
				user = ClientManager::getInstance()->getLegacyUser(nick);

			try {
				if(qm->addSource(cur, path, user, 0, isUtf8) && user->isOnline())
				ConnectionManager::getInstance()->getDownloadConnection(user);
			} catch(const Exception&) {
				return;
			}
		}
	}
}

void QueueLoader::endTag(const string& name, const string&) {
	if(inDownloads) {
		if(name == sDownload)
			cur = NULL;
		else if(name == "Downloads")
			inDownloads = false;
	}
}

// SearchManagerListener
void QueueManager::on(SearchManagerListener::SR, SearchResult* sr) throw() {
	bool added = false;
	bool wantConnection = false;
	int users = 0;

	if(BOOLSETTING(AUTO_SEARCH) && sr->getTTH() != NULL) {
		Lock l(cs);
		QueueItem::List matches;

		fileQueue.find(matches, *sr->getTTH());

		for(QueueItem::Iter i = matches.begin(); i != matches.end(); ++i) {
			QueueItem* qi = *i;
			dcassert(qi->getTTH());

			// Size compare to avoid popular spoof
			bool found = (*qi->getTTH() == *sr->getTTH()) && (qi->getSize() == sr->getSize());

			if(found && (qi->getSources().size() <= (unsigned int)SETTING(MAX_SOURCES))) {
				try {
					wantConnection = addSource(qi, sr->getFile(), sr->getUser(), 0, false);
					added = true;
					users = qi->countOnlineUsers();

				} catch(const Exception&) {
					// ...
				}
				break;
			}
		}
	}

	if(added && BOOLSETTING(AUTO_SEARCH_AUTO_MATCH) && (users < SETTING(MAX_AUTO_MATCH_SOURCES))) {
		try {
			addList(sr->getUser(), QueueItem::FLAG_MATCH_QUEUE);
		} catch(const Exception&) {
			// ...
		}
	}
	if(added && sr->getUser()->isOnline() && wantConnection)
		ConnectionManager::getInstance()->getDownloadConnection(sr->getUser());

}

// ClientManagerListener
void QueueManager::on(ClientManagerListener::UserConnected, const User::Ptr& aUser) throw() {
	bool hasDown = false;
	{
		Lock l(cs);
		for(int i = 0; i < QueueItem::LAST; ++i) {
			QueueItem::UserListIter j = userQueue.getList(i).find(aUser);
			if(j != userQueue.getList(i).end()) {
				for(QueueItem::Iter m = j->second.begin(); m != j->second.end(); ++m)
					fire(QueueManagerListener::StatusUpdated(), *m);
				if(i != QueueItem::PAUSED)
					hasDown = true;
			}
		}

		if(pfsQueue.find(aUser->getCID()) != pfsQueue.end()) {
			hasDown = true;
		}		
	}

	if(!aUser->isOnline() /* @todo && aUser->getHasTestSURinQueue() */)
		removeTestSUR(aUser);

	if(hasDown)	
		ConnectionManager::getInstance()->getDownloadConnection(aUser);
}

void QueueManager::on(TimerManagerListener::Second, u_int32_t /*aTick*/) throw() {
	if(BOOLSETTING(REALTIME_QUEUE_UPDATE) && (Speaker<QueueManagerListener>::listeners.size() > 1)) {
		Lock l(cs);
		QueueItem::List um = getRunningFiles();
		for(QueueItem::Iter j = um.begin(); j != um.end(); ++j) {
			QueueItem* q = *j;
			if(!q->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
				dcassert(q->getCurrentDownload() != NULL);
				q->setDownloadedBytes(q->getCurrentDownload()->getPos());
			}
			fire(QueueManagerListener::StatusUpdated(), q);
		}
	}
}


bool QueueManager::add(const string& aFile, int64_t aSize, const string& tth) throw(QueueException, FileException) 
{	
	if(aFile == USER_LIST_NAME) return false;
	if(aSize == 0) return false;

	string target = SETTING(DOWNLOAD_DIRECTORY) + aFile;
	string tempTarget = Util::emptyString;

	int flag = QueueItem::FLAG_RESUME;

	try {
		target = checkTarget(target, aSize, flag);
		if(target.empty()) return false;
	} catch(const Exception&) {
		return false;
	}
	
	{
		Lock l(cs);

		if(fileQueue.find(target)) return false;

		TTHValue root(tth);
		QueueItem::List ql;
		fileQueue.find(ql, root);
		if(ql.size()) return false;

		QueueItem* q = fileQueue.add(target, aSize, flag | (BOOLSETTING(MULTI_CHUNK) ? QueueItem::FLAG_MULTI_SOURCE : 0), QueueItem::DEFAULT, Util::emptyString, 0, GET_TIME(), Util::emptyString, Util::emptyString, &root);
		fire(QueueManagerListener::Added(), q);
	}

	if(BOOLSETTING(AUTO_SEARCH)){
		SearchManager::getInstance()->search(tth, 0, SearchManager::TYPE_TTH, SearchManager::SIZE_DONTCARE, Util::emptyString);
	}

	return true;
}

bool QueueManager::dropSource(Download* d, bool autoDrop) {
	int iHighSpeed = SETTING(H_DOWN_SPEED);

	unsigned int activeSegments, onlineUsers;
	int64_t overallSpeed;
	bool enabledSlowDisconnecting;
	User::Ptr aUser = d->getUserConnection()->getUser();

	{
	    Lock l(cs);

		QueueItem* q = userQueue.getRunning(aUser);
		d->setQI(q);

		if(!q) return false;

		if(autoDrop) {
			// Don't drop only downloading source
			if(q->getActiveSegments().size() < 2) return false;

		    userQueue.setWaiting(q, aUser);
			userQueue.remove(q, aUser);

			q->removeSource(aUser, QueueItem::Source::FLAG_SLOW);

			fire(QueueManagerListener::StatusUpdated(), q);
			setDirty();

			return true;
		}

		activeSegments = q->getActiveSegments().size();
		onlineUsers = q->countOnlineUsers();
		overallSpeed = q->getAverageSpeed();
		enabledSlowDisconnecting = q->isSet(QueueItem::FLAG_AUTODROP);
	}

	if(enabledSlowDisconnecting && !(SETTING(AUTO_DROP_SOURCE) && (activeSegments < 2))) {
		if((overallSpeed > (iHighSpeed*1024)) || (iHighSpeed == 0)) {
			if(onlineUsers > 2) {
				if(d->getRunningAverage() < SETTING(DISCONNECT)*1024) {
					removeSource(d->getTarget(), aUser, QueueItem::Source::FLAG_SLOW);
				} else {
					d->getUserConnection()->disconnect();
				}
				return false;
			}
		}
	}
	return true;
}

bool QueueManager::handlePartialResult(const User::Ptr& aUser, const TTHValue& tth, PartsInfo& partialInfo, PartsInfo& outPartialInfo) {
	bool wantConnection = false;
	dcassert(outPartialInfo.empty());

	{
		Lock l(cs);

		// Locate target QueueItem in download queue
		QueueItem::List ql;
		fileQueue.find(ql, tth);

		if(ql.empty()){
			dcdebug("Not found in download queue\n");
			return false;
		}

		// Check min size
		QueueItem::Ptr qi = ql[0];
		if(qi->getSize() < PARTIAL_SHARE_MIN_SIZE){
			dcassert(0);
			return false;
		}

		// Get my parts info
		FileChunksInfo::Ptr chunksInfo = FileChunksInfo::Get(qi->getTempTarget());
		if(!chunksInfo){
			return false;
		}
		chunksInfo->getPartialInfo(outPartialInfo);
		
		// Any parts for me?
		wantConnection = chunksInfo->isSource(partialInfo);

		// If this user isn't a source and has no parts needed, ignore it
		QueueItem::Source::Iter si = qi->getSource(aUser);
		if(si == qi->getSources().end()){
			si = qi->getBadSource(aUser);

		if(!wantConnection){
				if(si == qi->getBadSources().end())
					return false;
			}else{
				// add this user as partial file sharing source
				qi->addSource(aUser, qi->getTarget());
				si = qi->getSource(aUser);
				(*si)->setFlag(QueueItem::Source::FLAG_PARTIAL);
				userQueue.add(qi, aUser);
				dcassert(si != qi->getSources().end());
				fire(QueueManagerListener::SourcesUpdated(), qi);
			}
		}

		// Update source's parts info
		(*si)->setPartialInfo(partialInfo);
	}
	
	// Connect to this user
	if(wantConnection)
		ConnectionManager::getInstance()->getDownloadConnection(aUser);

	return true;
}

bool QueueManager::handlePartialSearch(const TTHValue& tth, PartsInfo& _outPartsInfo) {
	{
		Lock l(cs);

		// Locate target QueueItem in download queue
		QueueItem::List ql;
		fileQueue.find(ql, tth);

		if(ql.empty()){
			return false;
		}

		QueueItem::Ptr qi = ql[0];
		if(qi->getSize() < PARTIAL_SHARE_MIN_SIZE){
			return false;
		}

		FileChunksInfo::Ptr chunksInfo = FileChunksInfo::Get(qi->getTempTarget());
		if(!chunksInfo){
			return false;
		}

		chunksInfo->getPartialInfo(_outPartsInfo);
	}

	return !_outPartsInfo.empty();
}
/**
 * @file
 * $Id$
 */
