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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "QueueManager.h"

#include "ClientManager.h"
#include "ConnectionManager.h"
#include "DirectoryListing.h"
#include "Download.h"
#include "DownloadManager.h"
#include "FileChunksInfo.h"
#include "LogManager.h"
#include "ResourceManager.h"
#include "SearchManager.h"
#include "ShareManager.h"
#include "SharedFileStream.h"
#include "SimpleXML.h"
#include "StringTokenizer.h"
#include "Transfer.h"
#include "UploadManager.h"
#include "UserConnection.h"
#include "version.h"

#include <limits>

#ifdef ff
#undef ff
#endif

#ifndef _WIN32
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#endif

namespace {
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

		v.reserve(sl.size());

		for(StringList::const_iterator i = sl.begin(); i != sl.end(); ++i) {
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

QueueItem* QueueManager::FileQueue::add(const string& aTarget, int64_t aSize, 
						  Flags::MaskType aFlags, QueueItem::Priority p, const string& aTempTarget,
						  time_t aAdded, const string& freeBlocks/* = Util::emptyString*/, const string& verifiedBlocks /* = Util::emptyString */, const TTHValue& root) throw(QueueException, FileException)
{
	if(p == QueueItem::DEFAULT) {
		p = QueueItem::NORMAL;
		if(aSize <= SETTING(PRIO_HIGHEST_SIZE)*1024) {
		p = QueueItem::HIGHEST;
		} else if(aSize <= SETTING(PRIO_HIGH_SIZE)*1024) {
			p = QueueItem::HIGH;
		} else if(aSize <= SETTING(PRIO_NORMAL_SIZE)*1024) {
			p = QueueItem::NORMAL;
		} else if(aSize <= SETTING(PRIO_LOW_SIZE)*1024) {
			p = QueueItem::LOW;
		} else if(SETTING(PRIO_LOWEST)) {
			p = QueueItem::LOWEST;
		}
	}

	QueueItem* qi = new QueueItem(aTarget, aSize, p, aFlags, aAdded, root);

	if(!qi->isSet(QueueItem::FLAG_USER_LIST) && !qi->isSet(QueueItem::FLAG_TESTSUR)) {
		if(!aTempTarget.empty()) {
			qi->setTempTarget(aTempTarget);
		}
		qi->setMaxSegments(getMaxSegments(qi->getSize()));

		// Create FileChunksInfo for this item
		FileChunksInfo* pChunksInfo = NULL;

		dcassert(!qi->getTempTarget().empty());
		vector<int64_t> v;
		bool isMissing = false;
		
		if ( freeBlocks != Util::emptyString ){
			if(File::getSize(qi->getTempTarget()) > 0) {
				toIntList<int64_t>(freeBlocks, v);
			} else {
				v.push_back(0);
				v.push_back(qi->getSize());
				isMissing = true;
			}
		} else {
			v.push_back(0);
			v.push_back(qi->getSize());
		}
		
		if(v.size() < 2 || v.size() % 2 != 0){
			dcassert(0); // wrong freeBlocks
			v.clear();
			
			TigerTree tth;
			if(HashManager::getInstance()->getTree(root, tth)){
				// mark first byte as free, finish and verify this download
				v.push_back(0);
				v.push_back(1);
			}else{
				v.push_back(0);
				v.push_back(qi->getSize());
			}
		}
		
		pChunksInfo = new FileChunksInfo(const_cast<TTHValue*>(&qi->getTTH()), qi->getSize(), &v);
		qi->setChunksInfo(pChunksInfo);

		if(pChunksInfo && !isMissing && verifiedBlocks != Util::emptyString){
			vector<uint16_t> v;
			toIntList<uint16_t>(verifiedBlocks, v);

			for(vector<uint16_t>::iterator i = v.begin(); i < v.end(); i++, i++)
				pChunksInfo->markVerifiedBlock(*i, *(i+1));
		}

		if((qi->getDownloadedBytes() > 0))
			qi->setFlag(QueueItem::FLAG_EXISTS);

	} else {
		qi->setPriority(QueueItem::HIGHEST);
	}

	if(BOOLSETTING(AUTO_PRIORITY_DEFAULT) && !qi->isSet(QueueItem::FLAG_USER_LIST) && (p != QueueItem::HIGHEST) && (p != QueueItem::PAUSED)) {
		qi->setAutoPriority(true);
		qi->setPriority(qi->calculateAutoPriority());
	}

	dcassert(find(aTarget) == NULL);
	add(qi);
	return qi;
}

void QueueManager::FileQueue::add(QueueItem* qi) {
	queue.insert(make_pair(const_cast<string*>(&qi->getTarget()), qi));
}

void QueueManager::FileQueue::remove(QueueItem* qi) {
	queue.erase(const_cast<string*>(&qi->getTarget()));
	qi->dec();
}

QueueItem* QueueManager::FileQueue::find(const string& target) const {
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
		if(qi->getTTH() == tth) {
			ql.push_back(qi);
		}
	}
}

static QueueItem* findCandidate(QueueItem::StringIter start, QueueItem::StringIter end, deque<string>& recent) {
	QueueItem* cand = NULL;
	for(QueueItem::StringIter i = start; i != end; ++i) {
		QueueItem* q = i->second;

		// We prefer to search for things that are not running...
		if((cand != NULL) && !q->hasFreeSegments()) 
			continue;
		// No user lists
		if(q->isSet(QueueItem::FLAG_USER_LIST) || q->isSet(QueueItem::FLAG_TESTSUR) || q->isSet(QueueItem::FLAG_CHECK_FILE_LIST))
			continue;
        // No paused downloads
		if(q->getPriority() == QueueItem::PAUSED)
			continue;
		// No files that already have more than AUTO_SEARCH_LIMIT online sources
		if(q->countOnlineUsers() >= (size_t)SETTING(AUTO_SEARCH_LIMIT))
			continue;
		// Did we search for it recently?
        if(find(recent.begin(), recent.end(), q->getTarget()) != recent.end())
			continue;

		cand = q;

		if(cand->hasFreeSegments())
			break;
	}

	//check this again, if the first item we pick is running and there are no
	//other suitable items this will be true
	if((cand != NULL) && (!cand->hasFreeSegments())) {
		cand = NULL;
	}

	return cand;
}

QueueItem* QueueManager::FileQueue::findAutoSearch(deque<string>& recent) const {
	// We pick a start position at random, hoping that we will find something to search for...
	QueueItem::StringMap::size_type start = (QueueItem::StringMap::size_type)Util::rand((uint32_t)queue.size());

	QueueItem::StringIter i = queue.begin();
	advance(i, start);

	QueueItem* cand = findCandidate(i, queue.end(), recent);
	if(cand == NULL) {
		cand = findCandidate(queue.begin(), i, recent);
	} else if(!cand->hasFreeSegments()) {
		QueueItem* cand2 = findCandidate(queue.begin(), i, recent);
		if(cand2 != NULL && cand2->hasFreeSegments()) {
			cand = cand2;
		}
	}
	return cand;
}

void QueueManager::FileQueue::move(QueueItem* qi, const string& aTarget) {
	queue.erase(const_cast<string*>(&qi->getTarget()));
	qi->setTarget(aTarget);
	add(qi);
}

void QueueManager::UserQueue::add(QueueItem* qi) {
	for(QueueItem::SourceConstIter i = qi->getSources().begin(); i != qi->getSources().end(); ++i) {
		add(qi, i->getUser());
	}
}

void QueueManager::UserQueue::add(QueueItem* qi, const UserPtr& aUser) {
	QueueItem::List& l = userQueue[qi->getPriority()][aUser];
	if(qi->isSet(QueueItem::FLAG_EXISTS)) {
		l.push_front(qi);
	} else {
		l.push_back(qi);
	}
}

QueueItem* QueueManager::UserQueue::getNext(const UserPtr& aUser, QueueItem::Priority minPrio, QueueItem* pNext /* = NULL */) {
	int p = QueueItem::LAST - 1;
	bool fNext = false;

	do {
		QueueItem::UserListIter i = userQueue[p].find(aUser);
		if(i != userQueue[p].end()) {
			dcassert(!i->second.empty());
			QueueItem* found = i->second.front();

			bool freeSegments = found->hasFreeSegments();

			if(freeSegments && (pNext == NULL || fNext)) {
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
					if((iQi != i->second.end()) && (*iQi)->hasFreeSegments()) {
						return *iQi;
					}
				}
			}
		}
		p--;
	} while(p >= minPrio);

	return NULL;
}

QueueItem* QueueManager::UserQueue::getNextAll(const UserPtr& aUser, QueueItem::Priority minPrio) {
	int p = QueueItem::LAST - 1;

	do {
		QueueItem::UserListIter i = userQueue[p].find(aUser);
		if(i != userQueue[p].end()) {
			dcassert(!i->second.empty());
			return i->second.front();
		}
		p--;
	} while(p >= minPrio);

	return NULL;
}

void QueueManager::UserQueue::addDownload(QueueItem* qi, Download* d) {
	qi->getDownloads().push_back(d);

	// Only one download per user...
	dcassert(running.find(d->getUser()) == running.end());
	running[d->getUser()] = qi;
}

void QueueManager::UserQueue::removeDownload(QueueItem* qi, const UserPtr& user) {
	// Remove the download from running
	running.erase(user);

	for(DownloadList::iterator i = qi->getDownloads().begin(); i != qi->getDownloads().end(); ++i) {
		if((*i)->getUser() == user) {
			qi->getDownloads().erase(i);
			break;
		}
	}
}

void QueueManager::UserQueue::setPriority(QueueItem* qi, QueueItem::Priority p) {
	remove(qi, false);
	qi->setPriority(p);
	add(qi);
}

QueueItem* QueueManager::UserQueue::getRunning(const UserPtr& aUser) {
	QueueItem::UserIter i = running.find(aUser);
	return (i == running.end()) ? 0 : i->second;
}

void QueueManager::UserQueue::remove(QueueItem* qi, bool removeRunning) {
	for(QueueItem::SourceConstIter i = qi->getSources().begin(); i != qi->getSources().end(); ++i) {
		remove(qi, i->getUser(), removeRunning);
	}
}

void QueueManager::UserQueue::remove(QueueItem* qi, const UserPtr& aUser, bool removeRunning) {
	if(removeRunning && getRunning(aUser) == qi) {
		removeDownload(qi, aUser);
	}

	dcassert(qi->isSource(aUser));
	QueueItem::UserListMap& ulm = userQueue[qi->getPriority()];
	QueueItem::UserListMap::iterator j = ulm.find(aUser);
	dcassert(j != ulm.end());
	QueueItem::List& l = j->second;
	QueueItem::List::iterator i = find(l.begin(), l.end(), qi);
	dcassert(i != l.end());
	l.erase(i);

	if(l.empty()) {
		ulm.erase(j);
	}
}

QueueManager::QueueManager() : lastSave(0), queueFile(Util::getConfigPath() + "Queue.xml"), dirty(true), nextSearch(0) { 
	TimerManager::getInstance()->addListener(this); 
	SearchManager::getInstance()->addListener(this);
	ClientManager::getInstance()->addListener(this);

	File::ensureDirectory(Util::getListPath());
}

QueueManager::~QueueManager() throw() { 
	SearchManager::getInstance()->removeListener(this);
	TimerManager::getInstance()->removeListener(this); 
	ClientManager::getInstance()->removeListener(this);

	saveQueue();

	if(!BOOLSETTING(KEEP_LISTS)) {
		string path = Util::getListPath();
		StringList filelists = File::findFiles(path, "*.xml.bz2");
		StringList filelists2 = File::findFiles(path, "*.DcLst");
		filelists.insert(filelists.end(), filelists2.begin(), filelists2.end());

		std::for_each(filelists.begin(), filelists.end(), &File::deleteFile);
	}
}

bool QueueManager::getTTH(const string& name, TTHValue& tth) const throw() {
	Lock l(cs);
	QueueItem* qi = fileQueue.find(name);
	if(qi) {
		tth = qi->getTTH();
		return true;
	}
	return false;
}

struct PartsInfoReqParam{
	PartsInfo	parts;
	string		tth;
	string		myNick;
	string		hubIpPort;
	string		ip;
    uint16_t	udpPort;
};

void QueueManager::on(TimerManagerListener::Minute, uint64_t aTick) throw() {
	string searchString;
	vector<PartsInfoReqParam*> params;

	{
		Lock l(cs);

		// find max 10 pfs sources to exchange parts
		// the source basis interval is 5 minutes
		PFSSourceList sl;
		fileQueue.findPFSSources(sl);

		for(PFSSourceList::iterator i = sl.begin(); i != sl.end(); i++){
			QueueItem::PartialSource::Ptr source = (*i->first).getPartialSource();
			QueueItem* qi = i->second;
			FileChunksInfo::Ptr chunks = FileChunksInfo::Get(&qi->getTTH());

			if(!chunks) {
				dcassert(0);
				continue;
			}

			PartsInfoReqParam* param = new PartsInfoReqParam;
			chunks->getPartialInfo(param->parts);
			param->tth = qi->getTTH().toBase32();
			param->ip  = source->getIp();
			param->udpPort = source->getUdpPort();
			param->myNick = source->getMyNick();
			param->hubIpPort = source->getHubIpPort();

			params.push_back(param);

			source->setPendingQueryCount(source->getPendingQueryCount() + 1);
			source->setNextQueryTime(aTick + 300000);		// 5 minutes

		}

		if(BOOLSETTING(AUTO_SEARCH) && (aTick >= nextSearch) && (fileQueue.getSize() > 0)) {
			// We keep 30 recent searches to avoid duplicate searches
			while((recent.size() >= fileQueue.getSize()) || (recent.size() > 30)) {
				recent.pop_front();
			}

			QueueItem* qi;
			while((qi = fileQueue.findAutoSearch(recent)) == NULL && !recent.empty()) {
				recent.pop_front();
			}
			if(qi != NULL) {
				searchString = qi->getTTH().toBase32();
				recent.push_back(qi->getTarget());
				nextSearch = aTick + (SETTING(SEARCH_TIME) * 60000);
				if(BOOLSETTING(REPORT_ALTERNATES))
					LogManager::getInstance()->message(CSTRING(ALTERNATES_SEND) + Util::getFileName(qi->getTargetFileName()));		
			}
		}
	}

	// Request parts info from partial file sharing sources
	for(vector<PartsInfoReqParam*>::const_iterator i = params.begin(); i != params.end(); i++){
		PartsInfoReqParam* param = *i;
		SearchManager::getInstance()->sendPSR(param->ip, param->udpPort, true, param->myNick, param->hubIpPort, param->tth, param->parts);
		delete param;
	}

	if(!searchString.empty()) {
		SearchManager::getInstance()->search(searchString, 0, SearchManager::TYPE_TTH, SearchManager::SIZE_DONTCARE, "auto");
	}
}

void QueueManager::addList(const UserPtr& aUser, Flags::MaskType aFlags, const string& aInitialDir /* = Util::emptyString */) throw(QueueException, FileException) {
	// complete target is checked later, just remove path separators from the nick here
	StringList nicks = ClientManager::getInstance()->getNicks(*aUser);
	string nick = nicks.empty() ? Util::emptyString : Util::cleanPathChars(nicks[0]) + ".";
	string target = Util::getListPath() + nick + aUser->getCID().toBase32();

	if (!aInitialDir.empty()) {
		dirMap[aUser->getCID().toBase32()] = aInitialDir;
	}

	add(target, -1, TTHValue(), aUser, (Flags::MaskType)(QueueItem::FLAG_USER_LIST | aFlags));
}

void QueueManager::addPfs(const UserPtr& aUser, const string& aDir) throw(QueueException) {
	if(aUser == ClientManager::getInstance()->getMe()) {
		throw QueueException(STRING(NO_DOWNLOADS_FROM_SELF));
	}

	if(!aUser->isOnline())
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

void QueueManager::add(const string& aTarget, int64_t aSize, const TTHValue& root, UserPtr aUser,
					   Flags::MaskType aFlags /* = QueueItem::FLAG_RESUME */, bool addBad /* = true */) throw(QueueException, FileException)
{
	bool wantConnection = true;
	bool newItem = false;

	// Check that we're not downloading from ourselves...
	if(aUser == ClientManager::getInstance()->getMe()) {
		throw QueueException(STRING(NO_DOWNLOADS_FROM_SELF));
	}

	// Check if we're not downloading something already in our share
	if (BOOLSETTING(DONT_DL_ALREADY_SHARED)){
		if (ShareManager::getInstance()->isTTHShared(root)){
			throw QueueException(STRING(TTH_ALREADY_SHARED));
		}
	}
    
	string target = checkTarget(aTarget, aSize, aFlags);

	// Check if it's a zero-byte file, if so, create and return...
	if(aSize == 0) {
		if(!BOOLSETTING(SKIP_ZERO_BYTE)) {
			File::ensureDirectory(target);
			File f(target, File::WRITE, File::CREATE);
		}
		return;
	}
	
	{
		Lock l(cs);

		QueueItem* q = fileQueue.find(target);
		//if(q == NULL && root) {
		//	QueueItem::List ql;
		//	fileQueue.find(ql, root);
		//	if(!ql.empty()){
		//		dcassert(ql.size() == 1);
		//		q = ql[0];
		//	}
		//}
				
		if(q == NULL) {
			q = fileQueue.add(target, aSize, aFlags, QueueItem::DEFAULT, Util::emptyString, GET_TIME(), Util::emptyString, Util::emptyString, root);
			fire(QueueManagerListener::Added(), q);

			newItem = !q->isSet(QueueItem::FLAG_USER_LIST) && !q->isSet(QueueItem::FLAG_TESTSUR);
		} else {
			if(q->getSize() != aSize) {
				throw QueueException(STRING(FILE_WITH_DIFFERENT_SIZE));
			}
			if(!(root == q->getTTH())) {
				throw QueueException(STRING(FILE_WITH_DIFFERENT_TTH));
			}
			q->setFlag(aFlags);

			// We don't add any more sources to user list downloads...
			if(q->isSet(QueueItem::FLAG_USER_LIST))
				return;
		}

		wantConnection = addSource(q, aUser, (Flags::MaskType)(addBad ? QueueItem::Source::FLAG_MASK : 0));
	}

	if(wantConnection && aUser->isOnline())
		ConnectionManager::getInstance()->getDownloadConnection(aUser);

	// auto search, prevent DEADLOCK
	if(newItem && BOOLSETTING(AUTO_SEARCH)){
		SearchManager::getInstance()->search(TTHValue(root).toBase32(), 0, SearchManager::TYPE_TTH, SearchManager::SIZE_DONTCARE, "auto");
	}
	
}

void QueueManager::readd(const string& target, const UserPtr& aUser) throw(QueueException) {
	bool wantConnection = false;
	{
		Lock l(cs);
		QueueItem* q = fileQueue.find(target);
		if(q && q->isBadSource(aUser)) {
			wantConnection = addSource(q, aUser, QueueItem::Source::FLAG_MASK);
		}
	}
	if(wantConnection && aUser->isOnline())
		ConnectionManager::getInstance()->getDownloadConnection(aUser);
}

string QueueManager::checkTarget(const string& aTarget, int64_t aSize, Flags::MaskType& flags) throw(QueueException, FileException) {
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
bool QueueManager::addSource(QueueItem* qi, UserPtr aUser, Flags::MaskType addBad) throw(QueueException, FileException) {
	bool wantConnection = (qi->getPriority() != QueueItem::PAUSED) && qi->hasFreeSegments();

	if(qi->isSource(aUser)) {
		throw QueueException(STRING(DUPLICATE_SOURCE) + ": " + Util::getFileName(qi->getTarget()));
	}

	if(qi->isBadSourceExcept(aUser, addBad)) {
		throw QueueException(STRING(DUPLICATE_SOURCE) + ": " + Util::getFileName(qi->getTarget()));
	}

	qi->addSource(aUser);

	if(aUser->isSet(User::PASSIVE) && !ClientManager::getInstance()->isActive(ClientManager::getInstance()->getHubUrl(aUser))) {
		qi->removeSource(aUser, QueueItem::Source::FLAG_PASSIVE);
		wantConnection = false;
	} else {
		if ((!SETTING(SOURCEFILE).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
			PlaySound(Text::toT(SETTING(SOURCEFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
		userQueue.add(qi, aUser);
	}

	fire(QueueManagerListener::SourcesUpdated(), qi);
	setDirty();

	return wantConnection;
}

void QueueManager::addDirectory(const string& aDir, const UserPtr& aUser, const string& aTarget, QueueItem::Priority p /* = QueueItem::DEFAULT */) throw() {
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

QueueItem::Priority QueueManager::hasDownload(const UserPtr& aUser) throw() {
	Lock l(cs);
	if(pfsQueue.find(aUser->getCID()) != pfsQueue.end()) {
		return QueueItem::HIGHEST;
	}
	QueueItem* qi = userQueue.getNext(aUser, QueueItem::LOWEST);
	if(!qi) {
		return QueueItem::PAUSED;
	}
	return qi->getPriority();
}
namespace {
typedef unordered_map<TTHValue, const DirectoryListing::File*, TTHValue::Hash> TTHMap;

// *** WARNING *** 
// Lock(cs) makes sure that there's only one thread accessing this
static TTHMap tthMap;

void buildMap(const DirectoryListing::Directory* dir) throw() {
	for(DirectoryListing::Directory::List::const_iterator j = dir->directories.begin(); j != dir->directories.end(); ++j) {
		if(!(*j)->getAdls())
			buildMap(*j);
	}

	for(DirectoryListing::File::List::const_iterator i = dir->files.begin(); i != dir->files.end(); ++i) {
		const DirectoryListing::File* df = *i;
		tthMap.insert(make_pair(df->getTTH(), df));
	}
}
}

int QueueManager::matchListing(const DirectoryListing& dl) throw() {
	int matches = 0;
	{
		Lock l(cs);
		tthMap.clear();
		buildMap(dl.getRoot());

		for(QueueItem::StringMap::const_iterator i = fileQueue.getQueue().begin(); i != fileQueue.getQueue().end(); ++i) {
			QueueItem* qi = i->second;
			if(qi->isSet(QueueItem::FLAG_USER_LIST))
				continue;
			TTHMap::iterator j = tthMap.find(qi->getTTH());
			if(j != tthMap.end() && i->second->getSize() == qi->getSize()) {
				try {
					addSource(qi, dl.getUser(), QueueItem::Source::FLAG_FILE_NOT_AVAILABLE);
					matches++;
				} catch(...) {
					// Ignore...
				}
			}
		}
	}
	if(matches > 0)
		ConnectionManager::getInstance()->getDownloadConnection(dl.getUser());
		return matches;
}

void QueueManager::move(const string& aSource, const string& aTarget) throw() {
	string target = Util::validateFileName(aTarget);
	if(aSource == target)
		return;

	bool delSource = false;

	Lock l(cs);
	QueueItem* qs = fileQueue.find(aSource);
	if(qs) {
		// Don't move running downloads
		if(qs->isRunning()) {
			return;
		}
		// Don't move file lists
		if(qs->isSet(QueueItem::FLAG_USER_LIST))
			return;

		// Let's see if the target exists...then things get complicated...
		QueueItem* qt = fileQueue.find(target);
		if(qt == NULL || Util::stricmp(aSource, target) == 0) {
			// Good, update the target and move in the queue...
			fileQueue.move(qs, target);
			fire(QueueManagerListener::Moved(), qs, aSource);
			setDirty();
		} else {
			// Don't move to target of different size
			if(qs->getSize() != qt->getSize() || qs->getTTH() != qt->getTTH())
				return;

			for(QueueItem::SourceConstIter i = qs->getSources().begin(); i != qs->getSources().end(); ++i) {
				try {
					addSource(qt, i->getUser(), QueueItem::Source::FLAG_MASK);
				} catch(const Exception&) {
				}
			}
			delSource = true;
		}
	}

	if(delSource) {
		remove(aSource);
	}
}

bool QueueManager::getQueueInfo(const UserPtr& aUser, string& aTarget, int64_t& aSize, int& aFlags) throw() {
    Lock l(cs);
    QueueItem* qi = userQueue.getNextAll(aUser);
	if(qi == NULL)
		return false;

	aTarget = qi->getTarget();
	aSize = qi->getSize();
	aFlags = qi->getFlags();

	return true;
}

uint8_t QueueManager::FileQueue::getMaxSegments(int64_t filesize) const {
	uint8_t MaxSegments = 1;

	if(BOOLSETTING(SEGMENTS_MANUAL)) {
		MaxSegments = min((uint8_t)SETTING(NUMBER_OF_SEGMENTS), (uint8_t)10);
	} else {
		if((filesize >= 2*1048576) && (filesize < 15*1048576)) {
			MaxSegments = 2;
		} else if((filesize >= (int64_t)15*1048576) && (filesize < (int64_t)30*1048576)) {
			MaxSegments = 3;
		} else if((filesize >= (int64_t)30*1048576) && (filesize < (int64_t)60*1048576)) {
			MaxSegments = 4;
		} else if((filesize >= (int64_t)60*1048576) && (filesize < (int64_t)120*1048576)) {
			MaxSegments = 5;
		} else if((filesize >= (int64_t)120*1048576) && (filesize < (int64_t)240*1048576)) {
			MaxSegments = 6;
		} else if((filesize >= (int64_t)240*1048576) && (filesize < (int64_t)480*1048576)) {
			MaxSegments = 7;
		} else if((filesize >= (int64_t)480*1048576) && (filesize < (int64_t)960*1048576)) {
			MaxSegments = 8;
		} else if((filesize >= (int64_t)960*1048576) && (filesize < (int64_t)1920*1048576)) {
			MaxSegments = 9;
		} else if(filesize >= (int64_t)1920*1048576) {
			MaxSegments = 10;
		}
	}

#ifdef _DEBUG
	return 200;
#else
	return MaxSegments;
#endif
}

void QueueManager::getTargets(const TTHValue& tth, StringList& sl) {
	Lock l(cs);
	QueueItem::List ql;
	fileQueue.find(ql, tth);
	for(QueueItem::Iter i = ql.begin(); i != ql.end(); ++i) {
		sl.push_back((*i)->getTarget());
	}
}

Download* QueueManager::getDownload(UserConnection& aSource, string& aMessage) throw() {
	Lock l(cs);

	const UserPtr& aUser = aSource.getUser();
	dcdebug("Getting download for %s...", aUser->getCID().toBase32().c_str());
	// First check PFS's...
	PfsIter pi = pfsQueue.find(aUser->getCID());
	if(pi != pfsQueue.end()) {
		dcdebug("partial %s\n", pi->second.c_str());
		return new Download(aSource, pi->second);
	}

	QueueItem* q = userQueue.getNext(aUser);

again:

	if(!q) {
		dcdebug("none\n");
		return 0;
	}

	if((SETTING(FILE_SLOTS) != 0) && (q->isWaiting()) && !q->isSet(QueueItem::FLAG_TESTSUR) &&
		!q->isSet(QueueItem::FLAG_USER_LIST) && (getRunningFiles().size() >= (size_t)SETTING(FILE_SLOTS))) {
		aMessage = STRING(ALL_FILE_SLOTS_TAKEN);
		q = userQueue.getNext(aUser, QueueItem::LOWEST, q);
		goto again;
	}
	
	int64_t freeBlock = 0;

	QueueItem::SourceConstIter source = q->getSource(aUser);

	if(!q->isSet(QueueItem::FLAG_USER_LIST) && !q->isSet(QueueItem::FLAG_TESTSUR)) {
		if(source->isSet(QueueItem::Source::FLAG_PARTIAL)) {
			freeBlock = q->getChunksInfo()->getChunk(source->getPartialSource()->getPartialInfo(), aUser->getLastDownloadSpeed());
		} else {
			freeBlock = q->getChunksInfo()->getChunk(aUser->getLastDownloadSpeed());
		}

		if(freeBlock < 0) {
			if(freeBlock == -2) {
				dcassert(source->isSet(QueueItem::Source::FLAG_PARTIAL));
				userQueue.remove(q, aUser);
				q->removeSource(aUser, QueueItem::Source::FLAG_NO_NEED_PARTS);
				aMessage = STRING(NO_NEEDED_PART);
			} else {
				aMessage = STRING(NO_FREE_BLOCK);
			}
			
			q = userQueue.getNext(aUser, QueueItem::LOWEST, q);
			goto again;
		}
	}

	Download* d = new Download(aSource, *q, source->isSet(QueueItem::Source::FLAG_PARTIAL));
	
	if(d->getType() == Transfer::TYPE_FILE) {
		d->setStartPos(freeBlock);
		q->getChunksInfo()->setDownload(freeBlock, d, !aSource.isSet(UserConnection::FLAG_STEALTH)/* && q->getMaxSegments() != 1*/);
	} else if(d->getType() == Transfer::TYPE_TREE) {
		q->getChunksInfo()->putChunk(freeBlock);
	}
	
	userQueue.addDownload(q, d);	

	//fire(QueueManagerListener::StatusUpdated(), q);
	dcdebug("found %s\n", q->getTarget().c_str());
	return d;
}

namespace {
class TreeOutputStream : public OutputStream {
public:
	TreeOutputStream(TigerTree& aTree) : tree(aTree), bufPos(0) {
	}

	virtual size_t write(const void* xbuf, size_t len) throw(Exception) {
		size_t pos = 0;
		uint8_t* b = (uint8_t*)xbuf;
		while(pos < len) {
			size_t left = len - pos;
			if(bufPos == 0 && left >= TigerTree::HASH_SIZE) {
				tree.getLeaves().push_back(TTHValue(b + pos));
				pos += TigerTree::HASH_SIZE;
			} else {
				size_t bytes = min(TigerTree::HASH_SIZE - bufPos, left);
				memcpy(buf + bufPos, b + pos, bytes);
				bufPos += bytes;
				pos += bytes;
				if(bufPos == TigerTree::HASH_SIZE) {
					tree.getLeaves().push_back(TTHValue(buf));
					bufPos = 0;
				}
			}
		}
		return len;
	}

	virtual size_t flush() throw(Exception) {
		return 0;
	}
private:
	TigerTree& tree;
	uint8_t buf[TigerTree::HASH_SIZE];
	size_t bufPos;
};

}

void QueueManager::setFile(Download* d) {
	if(d->getType() == Transfer::TYPE_FILE) {
		Lock l(cs);

		QueueItem* qi = fileQueue.find(d->getPath());
		if(!qi) {
			throw QueueException(STRING(TARGET_REMOVED));
		}
		
		string target = d->getDownloadTarget();
		File::ensureDirectory(target);
	
		d->setFile(new SharedFileStream(target, d->getPos(), qi->getSize()));
	} else if(d->getType() == Transfer::TYPE_FULL_LIST) {
		string target = d->getDownloadTarget();
		File::ensureDirectory(target);

		if(d->isSet(Download::FLAG_XML_BZ_LIST)) {
			target += ".xml.bz2";
		} else {
			target += ".xml";
		}
		d->setFile(new File(target, File::WRITE, File::OPEN | File::TRUNCATE | File::CREATE));
	} else if(d->getType() == Transfer::TYPE_PARTIAL_LIST) {
		d->setFile(new StringOutputStream(d->getPFS()));
	} else if(d->getType() == Transfer::TYPE_TREE) {
		d->setFile(new TreeOutputStream(d->getTigerTree()));		
	}
}

void QueueManager::moveFile(const string& source, const string& target) {
	try {
		File::ensureDirectory(target);
		if(File::getSize(source) > MOVER_LIMIT) {
			mover.moveFile(source, target);
		} else {
			File::renameFile(source, target);
		}
	} catch(const FileException&) {
		try {
			if(!SETTING(DOWNLOAD_DIRECTORY).empty()) {
				File::renameFile(source, SETTING(DOWNLOAD_DIRECTORY) + Util::getFileName(target));
			} else {
				File::renameFile(source, Util::getFilePath(source) + Util::getFileName(target));
			}
		} catch(const FileException&) {
			try {
				File::renameFile(source, Util::getFilePath(source) + Util::getFileName(target));
			} catch(const FileException&) {
				// Ignore...
			}
		}
	}
}

void QueueManager::putDownload(Download* aDownload, bool finished, bool reportFinish) throw() {
	UserList getConn;
	string fname;
	UserPtr up = aDownload->getUser();
	int flag = 0;
	bool checkList = aDownload->isSet(Download::FLAG_CHECK_FILE_LIST) && aDownload->isSet(Download::FLAG_TESTSUR);

	{
		Lock l(cs);

		delete aDownload->getFile();
		aDownload->setFile(0);

		if(aDownload->getType() == Transfer::TYPE_PARTIAL_LIST) {
			pair<PfsIter, PfsIter> range = pfsQueue.equal_range(aDownload->getUser()->getCID());
			PfsIter i = find_if(range.first, range.second, CompareSecond<CID, string>(aDownload->getPath()));
			if(i != range.second) {
				pfsQueue.erase(i);
				fire(QueueManagerListener::PartialList(), aDownload->getUser(), aDownload->getPFS());
			}
		} else {
			QueueItem* q = fileQueue.find(aDownload->getPath());

			if(q) {
				if(aDownload->getType() == Transfer::TYPE_FULL_LIST) {
					if(aDownload->isSet(Download::FLAG_XML_BZ_LIST)) {
						q->setFlag(QueueItem::FLAG_XML_BZLIST);
					} else {
						q->unsetFlag(QueueItem::FLAG_XML_BZLIST);
					}
				}

				if(finished) {
					if(aDownload->getType() == Transfer::TYPE_TREE) {
						// Got a full tree, now add it to the HashManager
						dcassert(aDownload->getTreeValid());
						HashManager::getInstance()->addTree(aDownload->getTigerTree());

						userQueue.removeDownload(q, aDownload->getUser());
						fire(QueueManagerListener::StatusUpdated(), q);
					} else {
						// Now, let's see if this was a directory download filelist...
						if( (q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD) && directories.find(aDownload->getUser()) != directories.end()) ||
							(q->isSet(QueueItem::FLAG_MATCH_QUEUE)) ) 
						{
							fname = q->getListName();
							up = aDownload->getUser();
							flag = (q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD) ? QueueItem::FLAG_DIRECTORY_DOWNLOAD : 0)
								| (q->isSet(QueueItem::FLAG_MATCH_QUEUE) ? QueueItem::FLAG_MATCH_QUEUE : 0);
						} 

						string dir;
						if(aDownload->getType() == Transfer::TYPE_FULL_LIST) {
							StringMapIter i = dirMap.find(aDownload->getUser()->getCID().toBase32());
							if (i != dirMap.end()) {
								dir = i->second;
								dirMap.erase(i);
							}
						}

						// Check if we're anti-fragging...
						if(aDownload->isSet(Download::FLAG_ANTI_FRAG)) {
							// Ok, rename the file to what we expect it to be...
							try {
								const string& tgt = aDownload->getTempTarget().empty() ? aDownload->getPath() : aDownload->getTempTarget();
								File::renameFile(aDownload->getDownloadTarget(), tgt);
								aDownload->unsetFlag(Download::FLAG_ANTI_FRAG);
							} catch(const FileException& e) {
								dcdebug("AntiFrag: %s\n", e.getError().c_str());
								// Now what?
							}
						}
						
						// Check if we need to move the file
						if( !aDownload->getTempTarget().empty() && (Util::stricmp(aDownload->getPath().c_str(), aDownload->getTempTarget().c_str()) != 0) ) {
							moveFile(aDownload->getTempTarget(), aDownload->getPath());
						}

						fire(QueueManagerListener::Finished(), q, dir, static_cast<int64_t>(aDownload->getAverageSpeed()));
						fire(QueueManagerListener::Removed(), q);

						userQueue.remove(q);
						fileQueue.remove(q);
						setDirty();
					}
				} else {
					if(aDownload->getType() == Transfer::TYPE_FILE) {
						if(q->getDownloadedBytes() > 0) {
							q->setFlag(QueueItem::FLAG_EXISTS);
						} else {
							q->setTempTarget(Util::emptyString);
						}
					} else if(q->isSet(QueueItem::FLAG_USER_LIST)) {
						// Blah...no use keeping an unfinished file list...
						File::deleteFile(q->getListName());
					}

					if(reportFinish && q->getPriority() != QueueItem::PAUSED) {
						q->getOnlineUsers(getConn);
					}
	
					userQueue.removeDownload(q, aDownload->getUser());
					if(aDownload->getType() != Transfer::TYPE_FILE || (reportFinish && q->isWaiting())) {
						fire(QueueManagerListener::StatusUpdated(), q);						
					}
				}
			} else if(aDownload->getType() != Transfer::TYPE_TREE) {
				if(!aDownload->getTempTarget().empty() && (aDownload->getType() == Transfer::TYPE_FULL_LIST || aDownload->getTempTarget() != aDownload->getPath())) {
					File::deleteFile(aDownload->getTempTarget() + Download::ANTI_FRAG_EXT);
					File::deleteFile(aDownload->getTempTarget());
				}
			}
		}
	
		if(aDownload->getType() == Transfer::TYPE_FILE) {
			FileChunksInfo::Ptr fileChunks = FileChunksInfo::Get(&aDownload->getTTH());
			if(!(fileChunks == (FileChunksInfo*)NULL)){
				fileChunks->putChunk(aDownload->getStartPos());
			}
		}

		size_t speed = static_cast<size_t>(aDownload->getAverageSpeed());
		if(speed > 0 && aDownload->getStart() > 0 && aDownload->getTotal() > 32768 && speed < 10485760){
			UserPtr u = aDownload->getUser();
			u->setLastDownloadSpeed(speed);
		}
		delete aDownload;
	}

	for(UserList::const_iterator i = getConn.begin(); i != getConn.end(); ++i) {
		ConnectionManager::getInstance()->getDownloadConnection(*i);
	}

	if(!fname.empty()) {
		processList(fname, up, flag);
	}

	if(checkList) {
		try {
			QueueManager::getInstance()->addList(up, QueueItem::FLAG_CHECK_FILE_LIST);
		} catch(const Exception&) {}
	}
}

void QueueManager::processList(const string& name, UserPtr& user, int flags) {
	DirectoryListing dirList(user);
	try {
		dirList.loadFile(name);
	} catch(const Exception&) {
		LogManager::getInstance()->message(STRING(UNABLE_TO_OPEN_FILELIST) + name);
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
		const size_t BUF_SIZE = STRING(MATCHED_FILES).size() + 16;
		AutoArray<char> tmp(BUF_SIZE);
		snprintf(tmp, BUF_SIZE, CSTRING(MATCHED_FILES), matchListing(dirList));
		LogManager::getInstance()->message(Util::toString(ClientManager::getInstance()->getNicks(user->getCID())) + ": " + string(tmp));
	}
}

void QueueManager::remove(const string& aTarget) throw() {
	UserList x;

	{
		Lock l(cs);

		QueueItem* q = fileQueue.find(aTarget);
		if(!q)
			return;

		if(q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD)) {
			dcassert(q->getSources().size() == 1);
			DirectoryItem::DirectoryPair dp = directories.equal_range(q->getSources()[0].getUser());
			for(DirectoryItem::DirectoryIter i = dp.first; i != dp.second; ++i) {
				delete i->second;
			}
			directories.erase(q->getSources()[0].getUser());
		}

		// For partial-share
		UploadManager::getInstance()->abortUpload(q->getTempTarget());

		if(q->isRunning()) {
			for(DownloadList::iterator i = q->getDownloads().begin(); i != q->getDownloads().end(); ++i) {
				x.push_back((*i)->getUser());
				dirMap.erase((*i)->getUser()->getCID().toBase32());
			}
		} else if(!q->getTempTarget().empty() && q->getTempTarget() != q->getTarget()) {
			File::deleteFile(q->getTempTarget() + Download::ANTI_FRAG_EXT);
			File::deleteFile(q->getTempTarget());
		}

		fire(QueueManagerListener::Removed(), q);

		userQueue.remove(q);
		fileQueue.remove(q);

		setDirty();
	}

	for(UserList::iterator i = x.begin(); i != x.end(); ++i) {
		ConnectionManager::getInstance()->disconnect(*i, true);
	}
}

void QueueManager::removeSource(const string& aTarget, const UserPtr& aUser, Flags::MaskType reason, bool removeConn /* = true */) throw() {
	bool isRunning = false;
	bool removeCompletely = false;
	{
		Lock l(cs);
		QueueItem* q = fileQueue.find(aTarget);
		if(!q)
			return;

		if(!q->isSource(aUser))
			return;
	
		if(q->isSet(QueueItem::FLAG_USER_LIST)) {
			removeCompletely = true;
			goto endCheck;
		}

		if(reason == QueueItem::Source::FLAG_NO_TREE) {
			q->getSource(aUser)->setFlag(reason);
			return;
		}

		if(reason == QueueItem::Source::FLAG_CRC_WARN) {
			// Already flagged?
			QueueItem::SourceIter s = q->getSource(aUser);
			if(s->isSet(QueueItem::Source::FLAG_CRC_WARN)) {
				reason = QueueItem::Source::FLAG_CRC_FAILED;
			} else {
				s->setFlag(reason);
				return;
			}
		}

		if(q->isRunning() && userQueue.getRunning(aUser) == q) {
			isRunning = true;
			userQueue.removeDownload(q, aUser);
			fire(QueueManagerListener::StatusUpdated(), q);

		}
		userQueue.remove(q, aUser);
		q->removeSource(aUser, reason);
		
		fire(QueueManagerListener::SourcesUpdated(), q);
		setDirty();
	}
endCheck:
	if(isRunning && removeConn) {
		ConnectionManager::getInstance()->disconnect(aUser, true);
	}
	if(removeCompletely) {
		remove(aTarget);
	}	
}

void QueueManager::removeSource(const UserPtr& aUser, Flags::MaskType reason) throw() {
	bool isRunning = false;
	string removeRunning;
	{
		Lock l(cs);
		QueueItem* qi = NULL;
		while( (qi = userQueue.getNextAll(aUser, QueueItem::PAUSED)) != NULL) {
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
		if(qi) {
			if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
				removeRunning = qi->getTarget();
			} else {
				userQueue.removeDownload(qi, aUser);
				userQueue.remove(qi, aUser);
				isRunning = true;
				qi->removeSource(aUser, reason);
				fire(QueueManagerListener::StatusUpdated(), qi);
				fire(QueueManagerListener::SourcesUpdated(), qi);
				setDirty();
			}
		}
	}

	if(isRunning) {
		ConnectionManager::getInstance()->disconnect(aUser, true);
	}
	if(!removeRunning.empty()) {
		remove(removeRunning);
	}	
}

void QueueManager::setPriority(const string& aTarget, QueueItem::Priority p) throw() {
	UserList ul;
	bool running = false;

	{
		Lock l(cs);
	
		QueueItem* q = fileQueue.find(aTarget);
		if( (q != NULL) && (q->getPriority() != p) ) {
			running = q->isRunning();

			if(q->getPriority() == QueueItem::PAUSED || p == QueueItem::HIGHEST) {
				// Problem, we have to request connections to all these users...
				q->getOnlineUsers(ul);
			}
			userQueue.setPriority(q, p);
			setDirty();
			fire(QueueManagerListener::StatusUpdated(), q);
		}
	}

	if(p == QueueItem::PAUSED) {
		if(running)
			DownloadManager::getInstance()->abortDownload(aTarget);
	} else {
		for(UserList::const_iterator i = ul.begin(); i != ul.end(); ++i) {
			ConnectionManager::getInstance()->getDownloadConnection(*i);
		}
	}
}

void QueueManager::setAutoPriority(const string& aTarget, bool ap) throw() {
	UserList ul;

	{
		Lock l(cs);
	
		QueueItem* q = fileQueue.find(aTarget);
		if( (q != NULL) && (q->getAutoPriority() != ap) ) {
			q->setAutoPriority(ap);
			if(ap) {
				setPriority(q->getTarget(), q->calculateAutoPriority());
			}
			setDirty();
			fire(QueueManagerListener::StatusUpdated(), q);
		}
	}
}

void QueueManager::saveQueue() throw() {
	if(!dirty)
		return;
		
	Lock l(cs);	
		
	try {
		
		File ff(getQueueFile() + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		BufferedOutputStream<false> f(&ff);
		
		f.write(SimpleXML::utf8Header);
		f.write(LIT("<Downloads Version=\"" VERSIONSTRING "\">\r\n"));
		string tmp;
		string b32tmp;
		for(QueueItem::StringIter i = fileQueue.getQueue().begin(); i != fileQueue.getQueue().end(); ++i) {
			QueueItem* qi = i->second;
			if(!qi->isSet(QueueItem::FLAG_USER_LIST) && !qi->isSet(QueueItem::FLAG_TESTSUR)) {
				f.write(LIT("\t<Download Target=\""));
				f.write(SimpleXML::escape(qi->getTarget(), tmp, true));
				f.write(LIT("\" Size=\""));
				f.write(Util::toString(qi->getSize()));
				f.write(LIT("\" Priority=\""));
				f.write(Util::toString((int)qi->getPriority()));
				FileChunksInfo::Ptr fileChunksInfo = qi->getChunksInfo();
				if(fileChunksInfo) {
					f.write(LIT("\" FreeBlocks=\""));
					f.write(fileChunksInfo->getFreeChunksString());
					f.write(LIT("\" VerifiedParts=\""));
					f.write(fileChunksInfo->getVerifiedBlocksString());
				}
				f.write(LIT("\" Added=\""));
				f.write(Util::toString(qi->getAdded()));
				b32tmp.clear();
				f.write(LIT("\" TTH=\""));
				f.write(qi->getTTH().toBase32(b32tmp));
				if(qi->getDownloadedBytes() > 0) {
					f.write(LIT("\" TempTarget=\""));
					f.write(SimpleXML::escape(qi->getTempTarget(), tmp, true));
				}
				f.write(LIT("\" AutoPriority=\""));
				f.write(Util::toString(qi->getAutoPriority()));
				f.write(LIT("\" MaxSegments=\""));
				f.write(Util::toString(qi->getMaxSegments()));

				f.write(LIT("\">\r\n"));

				for(QueueItem::SourceConstIter j = qi->sources.begin(); j != qi->sources.end(); ++j) {
					if(j->isSet(QueueItem::Source::FLAG_PARTIAL)) continue;
					f.write(LIT("\t\t<Source CID=\""));
					f.write(j->getUser()->getCID().toBase32());
					f.write(LIT("\"/>\r\n"));
				}

				f.write(LIT("\t</Download>\r\n"));
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
	QueueLoader() : cur(NULL), inDownloads(false) { }
	~QueueLoader() { }
	void startTag(const string& name, StringPairList& attribs, bool simple);
	void endTag(const string& name, const string& data);
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
	} catch(const Exception&) {
		// ...
	}
}

static const string sDownload = "Download";
static const string sTempTarget = "TempTarget";
static const string sTarget = "Target";
static const string sSize = "Size";
static const string sPriority = "Priority";
static const string sSource = "Source";
static const string sNick = "Nick";
static const string sDirectory = "Directory";
static const string sAdded = "Added";
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
			Flags::MaskType flags = QueueItem::FLAG_RESUME;
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
			time_t added = static_cast<time_t>(Util::toInt(getAttrib(attribs, sAdded, 4)));
			const string& tthRoot = getAttrib(attribs, sTTH, 5);
			if(tthRoot.empty())
				return;

			string tempTarget = getAttrib(attribs, sTempTarget, 5);
			uint8_t maxSegments = (uint8_t)Util::toInt(getAttrib(attribs, sMaxSegments, 5));

			if(added == 0)
				added = GET_TIME();

			QueueItem* qi = qm->fileQueue.find(target);

			if(qi == NULL) {
				qi = qm->fileQueue.add(target, size, flags, p, tempTarget, added, freeBlocks, verifiedBlocks, TTHValue(tthRoot));

				bool ap = Util::toInt(getAttrib(attribs, sAutoPriority, 6)) == 1;
				qi->setAutoPriority(ap);
				qi->setMaxSegments(max((uint8_t)1, maxSegments));
				
				qm->fire(QueueManagerListener::Added(), qi);
			}
			if(!simple)
				cur = qi;
		} else if(cur != NULL && name == sSource) {
			const string& cid = getAttrib(attribs, sCID, 0);
			if(cid.length() != 39) {
				// Skip loading this source - sorry old users
				return;
			}
			UserPtr user = ClientManager::getInstance()->getUser(CID(cid));

			try {
				if(qm->addSource(cur, user, 0) && user->isOnline())
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

	if(BOOLSETTING(AUTO_SEARCH)) {
		Lock l(cs);
		QueueItem::List matches;

		fileQueue.find(matches, sr->getTTH());

		for(QueueItem::Iter i = matches.begin(); i != matches.end(); ++i) {
			QueueItem* qi = *i;

			// Size compare to avoid popular spoof
			if(qi->getSize() == sr->getSize() && !qi->isSource(sr->getUser())) {
				try {
					users = qi->countOnlineUsers();
					if(!BOOLSETTING(AUTO_SEARCH_AUTO_MATCH) || (users >= SETTING(MAX_AUTO_MATCH_SOURCES)))
						wantConnection = addSource(qi, sr->getUser(), 0);
					added = true;
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
void QueueManager::on(ClientManagerListener::UserConnected, const UserPtr& aUser) throw() {
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

	if(hasDown)	
		ConnectionManager::getInstance()->getDownloadConnection(aUser);
}

void QueueManager::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) throw() {
	bool hasTestSURinQueue = false;
	{
		Lock l(cs);
		for(int i = 0; i < QueueItem::LAST; ++i) {
			QueueItem::UserListIter j = userQueue.getList(i).find(aUser);
			if(j != userQueue.getList(i).end()) {
				for(QueueItem::Iter m = j->second.begin(); m != j->second.end(); ++m) {
					if((*m)->isSet(QueueItem::FLAG_TESTSUR))  hasTestSURinQueue = true;
					fire(QueueManagerListener::StatusUpdated(), *m);
				}
			}
		}
	}
	
	if(hasTestSURinQueue)
		removeTestSUR(aUser);
}

void QueueManager::on(TimerManagerListener::Second, uint64_t aTick) throw() {
	if(dirty && ((lastSave + 10000) < aTick)) {
		saveQueue();
	}

	Lock l(cs);

	QueueItem::List um = getRunningFiles();
	for(QueueItem::Iter j = um.begin(); j != um.end(); ++j) {
		QueueItem* q = *j;

		if(q->getAutoPriority()) {
			QueueItem::Priority p1 = q->getPriority();
			if(p1 != QueueItem::PAUSED) {
				QueueItem::Priority p2 = q->calculateAutoPriority();
				if(p1 != p2)
					setPriority(q->getTarget(), p2);
			}
		}
		fire(QueueManagerListener::StatusUpdated(), q);
	}
}

bool QueueManager::add(const string& aFile, int64_t aSize, const string& tth) throw(QueueException, FileException) 
{	
	if(aFile == Transfer::USER_LIST_NAME_BZ || aFile == Transfer::USER_LIST_NAME) return false;
	if(aSize == 0) return false;

	string target = SETTING(DOWNLOAD_DIRECTORY) + aFile;
	string tempTarget = Util::emptyString;

	Flags::MaskType flag = QueueItem::FLAG_RESUME;

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

		QueueItem* q = fileQueue.add(target, aSize, flag, QueueItem::DEFAULT, Util::emptyString, GET_TIME(), Util::emptyString, Util::emptyString, root);
		fire(QueueManagerListener::Added(), q);
	}

	if(BOOLSETTING(AUTO_SEARCH)){
		SearchManager::getInstance()->search(tth, 0, SearchManager::TYPE_TTH, SearchManager::SIZE_DONTCARE, Util::emptyString);
	}

	return true;
}

bool QueueManager::dropSource(Download* d) {
	size_t activeSegments, onlineUsers;
	uint64_t overallSpeed;

	{
	    Lock l(cs);

		QueueItem* q = userQueue.getRunning(d->getUser());

		if(!q) return false;

   		dcassert(q->isSource(d->getUser()));

		if(!q->isSet(QueueItem::FLAG_AUTODROP)) return true;

		activeSegments = q->getDownloads().size();
		onlineUsers = q->countOnlineUsers();
		overallSpeed = q->getAverageSpeed();
	}

	if(!SETTING(DROP_MULTISOURCE_ONLY) || (activeSegments >= 2)) {
		size_t iHighSpeed = SETTING(DISCONNECT_FILE_SPEED);
		if((iHighSpeed == 0) || (overallSpeed > iHighSpeed*1024)) {
			if(onlineUsers > 2) {
				d->getUser()->setLastDownloadSpeed(static_cast<size_t>(d->getAverageSpeed()));
				if(d->getAverageSpeed() < SETTING(REMOVE_SPEED)*1024) {
					removeSource(d->getPath(), d->getUser(), QueueItem::Source::FLAG_SLOW);
				} else {
					d->getUserConnection().disconnect();
				}
				return false;
			}
		}
	}
	return true;
}

bool QueueManager::handlePartialResult(const UserPtr& aUser, const TTHValue& tth, const QueueItem::PartialSource& partialSource, PartsInfo& outPartialInfo) {
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
		FileChunksInfo::Ptr chunksInfo = FileChunksInfo::Get(&qi->getTTH());
		if(!chunksInfo){
			return false;
		}
		chunksInfo->getPartialInfo(outPartialInfo);
		
		// Any parts for me?
		wantConnection = chunksInfo->isSource(partialSource.getPartialInfo());

		// If this user isn't a source and has no parts needed, ignore it
		QueueItem::SourceIter si = qi->getSource(aUser);
		if(si == qi->getSources().end()){
			si = qi->getBadSource(aUser);

			if(si != qi->getBadSources().end() && si->isSet(QueueItem::Source::FLAG_TTH_INCONSISTENCY))
				return false;

			if(!wantConnection){
				if(si == qi->getBadSources().end())
					return false;
			}else{
				// add this user as partial file sharing source
				qi->addSource(aUser);
				si = qi->getSource(aUser);
				si->setFlag(QueueItem::Source::FLAG_PARTIAL);

				QueueItem::PartialSource* ps = new QueueItem::PartialSource(partialSource.getMyNick(),
					partialSource.getHubIpPort(), partialSource.getIp(), partialSource.getUdpPort());
				si->setPartialSource(ps);

				userQueue.add(qi, aUser);
				dcassert(si != qi->getSources().end());
				fire(QueueManagerListener::SourcesUpdated(), qi);
			}
		}

		// Update source's parts info
		si->getPartialSource()->setPartialInfo(partialSource.getPartialInfo());
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

		FileChunksInfo::Ptr chunksInfo = FileChunksInfo::Get(&qi->getTTH());
		if(!chunksInfo){
			return false;
		}

		chunksInfo->getPartialInfo(_outPartsInfo);
	}

	return !_outPartsInfo.empty();
}

// compare nextQueryTime, get the oldest ones
void QueueManager::FileQueue::findPFSSources(PFSSourceList& sl) 
{
	typedef multimap<time_t, pair<QueueItem::SourceIter, QueueItem*> > Buffer;
	Buffer buffer;
	uint64_t now = GET_TICK();

	for(QueueItem::StringIter i = queue.begin(); i != queue.end(); ++i) {
		QueueItem* q = i->second;

		if(q->getSize() < PARTIAL_SHARE_MIN_SIZE) continue;

		QueueItem::SourceList& sources = q->getSources();
		QueueItem::SourceList& badSources = q->getBadSources();

		for(QueueItem::SourceIter j = sources.begin(); j != sources.end(); ++j) {
			if(	(*j).isSet(QueueItem::Source::FLAG_PARTIAL) && (*j).getPartialSource()->getNextQueryTime() <= now &&
				(*j).getPartialSource()->getPendingQueryCount() < 10)
			{
				buffer.insert(make_pair((*j).getPartialSource()->getNextQueryTime(), make_pair(j, q)));
			}
		}

		for(QueueItem::SourceIter j = badSources.begin(); j != badSources.end(); ++j) {
			if(	(*j).isSet(QueueItem::Source::FLAG_TTH_INCONSISTENCY) == false && (*j).isSet(QueueItem::Source::FLAG_PARTIAL) &&
				(*j).getPartialSource()->getNextQueryTime() <= now && (*j).getPartialSource()->getPendingQueryCount() < 10 )
			{
				buffer.insert(make_pair((*j).getPartialSource()->getNextQueryTime(), make_pair(j, q)));
			}
		}
	}

	// copy to results
	dcassert(sl.empty());
	const int32_t maxElements = 10;
	sl.reserve(maxElements);
	for(Buffer::iterator i = buffer.begin(); i != buffer.end() && sl.size() < maxElements; i++){
		sl.push_back(i->second);
	}
}

/**
 * @file
 * $Id$
 */
