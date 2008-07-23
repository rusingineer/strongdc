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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "QueueManager.h"

#include "ClientManager.h"
#include "ConnectionManager.h"
#include "DirectoryListing.h"
#include "Download.h"
#include "DownloadManager.h"
#include "HashManager.h"
#include "LogManager.h"
#include "ResourceManager.h"
#include "SearchManager.h"
#include "ShareManager.h"
#include "SimpleXML.h"
#include "StringTokenizer.h"
#include "Transfer.h"
#include "UploadManager.h"
#include "UserConnection.h"
#include "version.h"
#include "SearchResult.h"

#include <limits>

#ifdef ff
#undef ff
#endif

namespace dcpp {

QueueItem* QueueManager::FileQueue::add(const string& aTarget, int64_t aSize, 
						  Flags::MaskType aFlags, QueueItem::Priority p, const string& aTempTarget, 
						  time_t aAdded, const TTHValue& root) throw(QueueException, FileException)
{
	if(p == QueueItem::DEFAULT) {
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
		qi->setMaxSegments(getMaxSegments(qi->getSize()));
		
		if(!aTempTarget.empty()) {
			qi->setTempTarget(aTempTarget);
			if(!Util::fileExists(aTempTarget) && Util::fileExists(aTempTarget + ".antifrag")) {
				// load old antifrag file
				File::renameFile(aTempTarget + ".antifrag", qi->getTempTarget());
	        }
		}
		
		if(p == QueueItem::DEFAULT) {
			if(BOOLSETTING(AUTO_PRIORITY_DEFAULT)) {
				qi->setAutoPriority(true);
				qi->setPriority(QueueItem::LOW);
			} else {
				qi->setPriority(QueueItem::NORMAL);
			}
		}
	} else {
		qi->setPriority(QueueItem::HIGHEST);
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
				stricmp(suffix.c_str(), t.c_str() + (t.length() - suffix.length())) == 0) )
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
		if((cand != NULL) && q->getNextSegment(0, 0, 0, NULL).getSize() == 0) 
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

		if(cand->getNextSegment(0, 0, 0, NULL).getSize() != 0)
			break;
	}

	//check this again, if the first item we pick is running and there are no
	//other suitable items this will be true
	if((cand != NULL) && (cand->getNextSegment(0, 0, 0, NULL).getSize() == 0)) {
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
	} else if(cand->getNextSegment(0, 0, 0, NULL).getSize() == 0) {
		QueueItem* cand2 = findCandidate(queue.begin(), i, recent);
		if(cand2 != NULL && cand2->getNextSegment(0, 0, 0, NULL).getSize() != 0) {
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

QueueItem* QueueManager::UserQueue::getNext(const UserPtr& aUser, QueueItem::Priority minPrio, int64_t wantedSize, int64_t lastSpeed, bool allowRemove) {
	int p = QueueItem::LAST - 1;
	lastError = Util::emptyString;

	do {
		QueueItem::UserListIter i = userQueue[p].find(aUser);
		if(i != userQueue[p].end()) {
			dcassert(!i->second.empty());
			for(QueueItem::Iter j = i->second.begin(); j != i->second.end(); ++j) {
				QueueItem* qi = *j;
				
				QueueItem::SourceConstIter source = qi->getSource(aUser);
				if(source->isSet(QueueItem::Source::FLAG_PARTIAL)) {
					// check partial source
					int64_t blockSize = HashManager::getInstance()->getBlockSize(qi->getTTH());
					if(blockSize == 0)
						blockSize = qi->getSize();
					
					Segment segment = qi->getNextSegment(blockSize, wantedSize, lastSpeed, source->getPartialSource());
					if(allowRemove && segment.getStart() != -1 && segment.getSize() == 0) {
						// no other partial chunk from this user, remove him from queue
						remove(qi, aUser);
						qi->removeSource(aUser, QueueItem::Source::FLAG_NO_NEED_PARTS);
						lastError = STRING(NO_NEEDED_PART);
						p++;
						break;
					}
				}

				if(qi->isWaiting()) {
					// check maximum simultaneous files setting
					if(SETTING(FILE_SLOTS) == 0 || qi->isSet(QueueItem::FLAG_TESTSUR) || qi->isSet(QueueItem::FLAG_USER_LIST) ||
						QueueManager::getInstance()->getRunningFiles().size() < (size_t)SETTING(FILE_SLOTS)) 
					{
						return qi;
					} else {
						lastError = STRING(ALL_FILE_SLOTS_TAKEN);
						continue;
					}
				}
				
				// No segmented downloading when getting the tree
				if(qi->getDownloads()[0]->getType() == Transfer::TYPE_TREE) {
					continue;
				}
				if(!qi->isSet(QueueItem::FLAG_USER_LIST)) {
					int64_t blockSize = HashManager::getInstance()->getBlockSize(qi->getTTH());
					if(blockSize == 0)
						blockSize = qi->getSize();

					Segment segment = qi->getNextSegment(blockSize, wantedSize, lastSpeed, source->getPartialSource());
					if(segment.getSize() == 0) {
						lastError = segment.getStart() == -1 ? STRING(ALL_DOWNLOAD_SLOTS_TAKEN) : STRING(NO_FREE_BLOCK);
						dcdebug("No segment for %s in %s, block " I64_FMT "\n", aUser->getCID().toBase32().c_str(), qi->getTarget().c_str(), blockSize);
						continue;
					}
				}
				return qi;
			}
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

void QueueManager::FileMover::moveFile(const string& source, const string& target) {
	Lock l(cs);
	files.push_back(make_pair(source, target));
	if(!active) {
		active = true;
		start();
	}
}

int QueueManager::FileMover::run() {
	for(;;) {
		FilePair next;
		{
			Lock l(cs);
			if(files.empty()) {
				active = false;
				return 0;
			}
			next = files.back();
			files.pop_back();
		}
		try {
			File::renameFile(next.first, next.second);
		} catch(const FileException&) {
			try {
				// Try to just rename it to the correct name  at least
				string newTarget = Util::getFilePath(next.first) + Util::getFileName(next.second);
				File::renameFile(next.first, newTarget);
				LogManager::getInstance()->message(next.first + STRING(RENAMED_TO) + newTarget);
			} catch(const FileException& e) {
				LogManager::getInstance()->message(STRING(UNABLE_TO_RENAME) + next.first + ": " + e.getError());
			}
		}
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
		std::for_each(filelists.begin(), filelists.end(), &File::deleteFile);
		filelists = File::findFiles(path, "*.DcLst");
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
	vector<const PartsInfoReqParam*> params;

	{
		Lock l(cs);

		//find max 10 pfs sources to exchange parts
		//the source basis interval is 5 minutes
		PFSSourceList sl;
		fileQueue.findPFSSources(sl);

		for(PFSSourceList::const_iterator i = sl.begin(); i != sl.end(); i++){
			QueueItem::PartialSource::Ptr source = (*i->first).getPartialSource();
			const QueueItem* qi = i->second;

			PartsInfoReqParam* param = new PartsInfoReqParam;
			
			int64_t blockSize = HashManager::getInstance()->getBlockSize(qi->getTTH());
			if(blockSize == 0)
				blockSize = qi->getSize();			
			qi->getPartialInfo(param->parts, blockSize);
			
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
	for(vector<const PartsInfoReqParam*>::const_iterator i = params.begin(); i != params.end(); i++){
		const PartsInfoReqParam* param = *i;
		dcassert(param->udpPort > 0);
		
		try {
			AdcCommand cmd = SearchManager::getInstance()->toPSR(true, param->myNick, param->hubIpPort, param->tth, param->parts);
			Socket s;
			s.writeTo(param->ip, param->udpPort, cmd.toString(ClientManager::getInstance()->getMyCID()));
		} catch(...) {
			dcdebug("Partial search caught error\n");		
		}
		
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
		Lock l(cs);
		dirMap[aUser->getCID().toBase32()] = aInitialDir;
	}

	add(target, -1, TTHValue(), aUser, (Flags::MaskType)(QueueItem::FLAG_USER_LIST | aFlags));
}

void QueueManager::addPfs(const UserPtr& aUser, const string& aDir, bool onlyDownload) throw(QueueException) {
	if(aUser == ClientManager::getInstance()->getMe()) {
		throw QueueException(STRING(NO_DOWNLOADS_FROM_SELF));
	}

	if(!aUser->isOnline() || aUser->getCID().isZero())
		return;

	{
		Lock l(cs);
		pair<PfsIter, PfsIter> range = pfsQueue.equal_range(aUser->getCID());
		if(find_if(range.first, range.second, CompareSecondFirst<CID, string, bool>(aDir)) == range.second) {
			pfsQueue.insert(make_pair(aUser->getCID(), make_pair(aDir, onlyDownload)));
		}
	}

	ConnectionManager::getInstance()->getDownloadConnection(aUser);
}

void QueueManager::add(const string& aTarget, int64_t aSize, const TTHValue& root, UserPtr aUser,
					   Flags::MaskType aFlags /* = 0 */, bool addBad /* = true */) throw(QueueException, FileException)
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
		if(q == NULL && !((aFlags & QueueItem::FLAG_USER_LIST) || (aFlags & QueueItem::FLAG_TESTSUR))) {
			QueueItem::List ql;
			fileQueue.find(ql, root);
			if(!ql.empty()){
				dcassert(ql.size() == 1);
				q = ql[0];
			}
		}
				
		if(q == NULL) {
			q = fileQueue.add(target, aSize, aFlags, QueueItem::DEFAULT, Util::emptyString, GET_TIME(), root);
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

			// We don't add any more sources to user list downloads, but we want their flags updated
			if(q->isSet(QueueItem::FLAG_USER_LIST) || q->isSet(QueueItem::FLAG_TESTSUR))
				return;
		}

		wantConnection = aUser && addSource(q, aUser, (Flags::MaskType)(addBad ? QueueItem::Source::FLAG_MASK : 0));
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
	bool wantConnection = (qi->getPriority() != QueueItem::PAUSED) && !userQueue.getRunning(aUser);

	if(qi->isSource(aUser)) {
		throw QueueException(STRING(DUPLICATE_SOURCE) + ": " + Util::getFileName(qi->getTarget()));
	}

	if(qi->isBadSourceExcept(aUser, addBad)) {
		throw QueueException(STRING(DUPLICATE_SOURCE) + ": " + Util::getFileName(qi->getTarget()));
	}

	qi->addSource(aUser);

	if(aUser->isSet(User::PASSIVE) && !ClientManager::getInstance()->isActive(Util::emptyString)) { // TODO: get hub url
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
			if(stricmp(aTarget.c_str(), i->second->getName().c_str()) == 0)
				return;
		}
		
		// Unique directory, fine...
		directories.insert(make_pair(aUser, new DirectoryItem(aUser, aDir, aTarget, p)));
		needList = (dp.first == dp.second);
		setDirty();
	}

	if(needList) {
		try {
			//addList(aUser, QueueItem::FLAG_DIRECTORY_DOWNLOAD);
			addPfs(aUser, aDir, true);
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
typedef unordered_map<TTHValue, const DirectoryListing::File*> TTHMap;

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
		if(qt == NULL || stricmp(aSource, target) == 0) {
			// Good, update the target and move in the queue...
			fire(QueueManagerListener::Moved(), qs, aSource);
			fileQueue.move(qs, target);
			fire(QueueManagerListener::Added(), qs);
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
    QueueItem* qi = userQueue.getNext(aUser);
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
	return 88;
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
		dcdebug("partial %s\n", pi->second.first.c_str());
		return new Download(aSource, pi->second.first);
	}

	QueueItem* q = userQueue.getNext(aUser, QueueItem::LOWEST, aSource.getChunkSize(), aSource.getSpeed(), true);

	if(!q) {
		aMessage = userQueue.getLastError();
		dcdebug("none\n");
		return 0;
	}

	Download* d = new Download(aSource, *q);
	
	userQueue.addDownload(q, d);	

	fire(QueueManagerListener::SourcesUpdated(), q);
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
			if(bufPos == 0 && left >= TigerTree::BYTES) {
				tree.getLeaves().push_back(TTHValue(b + pos));
				pos += TigerTree::BYTES;
			} else {
				size_t bytes = min(TigerTree::BYTES - bufPos, left);
				memcpy(buf + bufPos, b + pos, bytes);
				bufPos += bytes;
				pos += bytes;
				if(bufPos == TigerTree::BYTES) {
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
	uint8_t buf[TigerTree::BYTES];
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
		
		if(d->getOverlapped()) {
			d->setOverlapped(false);

			bool found = false;
			// ok, we got a fast slot, so it's possible to disconnect original user now
			for(DownloadList::const_iterator i = qi->getDownloads().begin(); i != qi->getDownloads().end(); ++i) {
				if((*i) != d && (*i)->getSegment().contains(d->getSegment())) {
					found = true;

					// disconnect slow chunk
					(*i)->getUserConnection().disconnect();
					break;
				}
			}

			if(!found) {
				// slow chunk already finished ???
				throw QueueException(STRING(DOWNLOAD_FINISHED_IDLE));
			}
		}
		
		string target = d->getDownloadTarget();
		File::ensureDirectory(target);

		File* f = new File(target, File::WRITE, File::OPEN | File::CREATE | File::SHARED);

		// Only use antifrag if we don't have a previous non-antifrag part
		if(BOOLSETTING(ANTI_FRAG) && f->getSize() != qi->getSize()) {
			f->setSize(d->getTigerTree().getFileSize());
		}
		
		f->setPos(d->getSegment().getStart());
		d->setFile(f);
	} else if(d->getType() == Transfer::TYPE_FULL_LIST) {
		{
			Lock l(cs);

			QueueItem* qi = fileQueue.find(d->getPath());
			if(!qi) {
				throw QueueException(STRING(TARGET_REMOVED));
			}
		
			// set filelist's size
			qi->setSize(d->getSize());
		}

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
			PfsIter i = find_if(range.first, range.second, CompareSecondFirst<CID, string, bool>(aDownload->getPath()));
			if(i != range.second) {
				if(i->second.second == false) {
					fire(QueueManagerListener::PartialList(), aDownload->getUser(), aDownload->getPFS());
				} else {
					fname = aDownload->getPFS();
					up = aDownload->getUser();
					flag = QueueItem::FLAG_DIRECTORY_DOWNLOAD | QueueItem::FLAG_TEXT;
				}
				pfsQueue.erase(i);
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

						if(aDownload->getType() == Transfer::TYPE_FILE) {
							aDownload->setOverlapped(false);
							q->addSegment(aDownload->getSegment());
						}
						
						if(aDownload->getType() != Transfer::TYPE_FILE || q->isFinished()) {

							if(aDownload->getType() == Transfer::TYPE_FILE) {
								// For partial-share, abort upload first to move file correctly
								UploadManager::getInstance()->abortUpload(q->getTempTarget());
							}
						
							// Check if we need to move the file
							if( !aDownload->getTempTarget().empty() && (stricmp(aDownload->getPath().c_str(), aDownload->getTempTarget().c_str()) != 0) ) {
								moveFile(aDownload->getTempTarget(), aDownload->getPath());
							}

							if(BOOLSETTING(LOG_DOWNLOADS) && (BOOLSETTING(LOG_FILELIST_TRANSFERS) || aDownload->getType() == Transfer::TYPE_FILE)) {
								StringMap params;
								aDownload->getParams(aDownload->getUserConnection(), params);
								LOG(LogManager::DOWNLOAD, params);
							}
							
							fire(QueueManagerListener::Finished(), q, dir, aDownload);
							fire(QueueManagerListener::Removed(), q);
	
							userQueue.remove(q);
							fileQueue.remove(q);
						} else {
							userQueue.removeDownload(q, aDownload->getUser());
							if(aDownload->getType() != Transfer::TYPE_FILE || (reportFinish && q->isWaiting())) {
								fire(QueueManagerListener::StatusUpdated(), q);
							}
						}
						setDirty();
					}
				} else {
					if(aDownload->getType() != Transfer::TYPE_TREE) {
						if(q->getDownloadedBytes() > 0) {
							q->setFlag(QueueItem::FLAG_EXISTS);
						} else {
							q->setTempTarget(Util::emptyString);
						}
						if(q->isSet(QueueItem::FLAG_USER_LIST)) {
							// Blah...no use keeping an unfinished file list...
							File::deleteFile(q->getListName());
						}
						if(aDownload->getType() == Transfer::TYPE_FILE) {
							// mark partially downloaded chunk, but align it to block size
							int64_t downloaded = aDownload->getPos();
							downloaded -= downloaded % aDownload->getTigerTree().getBlockSize();

							if(downloaded > 0) {
								q->addSegment(Segment(aDownload->getStartPos(), downloaded));
							}
						}
					}

					if(q->getPriority() != QueueItem::PAUSED) {
						q->getOnlineUsers(getConn);
					}
	
					userQueue.removeDownload(q, aDownload->getUser());
					fire(QueueManagerListener::StatusUpdated(), q);

					if(aDownload->isSet(Download::FLAG_OVERLAP)) {
						// overlapping segment disconnected, unoverlap original segment
						for(DownloadList::const_iterator i = q->getDownloads().begin(); i != q->getDownloads().end(); ++i) {
							if((*i)->getSegment().contains(aDownload->getSegment())) {
								(*i)->setOverlapped(false);
								break;
							}
						}
					}
				}
			} else if(aDownload->getType() != Transfer::TYPE_TREE) {
				if(!aDownload->getTempTarget().empty() && (aDownload->getType() == Transfer::TYPE_FULL_LIST || aDownload->getTempTarget() != aDownload->getPath())) {
					File::deleteFile(aDownload->getTempTarget());
				}
			}
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
		if(flags & QueueItem::FLAG_TEXT) {
			dirList.loadXML(name, true);
		} else {
			dirList.loadFile(name);
		}
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
		string tmp;
		tmp.resize(BUF_SIZE);
		snprintf(&tmp[0], tmp.size(), CSTRING(MATCHED_FILES), matchListing(dirList));
		LogManager::getInstance()->message(Util::toString(ClientManager::getInstance()->getNicks(user->getCID())) + ": " + tmp);
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
	vector<pair<string, QueueItem::Priority>> priorities;

	{
		Lock l(cs);
	
		QueueItem* q = fileQueue.find(aTarget);
		if( (q != NULL) && (q->getAutoPriority() != ap) ) {
			q->setAutoPriority(ap);
			if(ap) {
				priorities.push_back(make_pair(q->getTarget(), q->calculateAutoPriority()));
			}
			setDirty();
			fire(QueueManagerListener::StatusUpdated(), q);
		}
	}

	for(vector<pair<string, QueueItem::Priority>>::const_iterator p = priorities.begin(); p != priorities.end(); p++) {
		setPriority((*p).first, (*p).second);
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
				f.write(LIT("\" Added=\""));
				f.write(Util::toString(qi->getAdded()));
				b32tmp.clear();
				f.write(LIT("\" TTH=\""));
				f.write(qi->getTTH().toBase32(b32tmp));
				if(!qi->getDone().empty()) {
					f.write(LIT("\" TempTarget=\""));
					f.write(SimpleXML::escape(qi->getTempTarget(), tmp, true));
				}
				f.write(LIT("\" AutoPriority=\""));
				f.write(Util::toString(qi->getAutoPriority()));
				f.write(LIT("\" MaxSegments=\""));
				f.write(Util::toString(qi->getMaxSegments()));

				f.write(LIT("\">\r\n"));

				for(QueueItem::SegmentSet::const_iterator i = qi->getDone().begin(); i != qi->getDone().end(); ++i) {
					f.write(LIT("\t\t<Segment Start=\""));
					f.write(Util::toString(i->getStart()));
					f.write(LIT("\" Size=\""));
					f.write(Util::toString(i->getSize()));
					f.write(LIT("\"/>\r\n"));
				}
				for(QueueItem::SourceConstIter j = qi->sources.begin(); j != qi->sources.end(); ++j) {
					if(j->isSet(QueueItem::Source::FLAG_PARTIAL)) continue;
					f.write(LIT("\t\t<Source CID=\""));
					f.write(j->getUser()->getCID().toBase32());
					f.write(LIT("\" Nick=\""));
					f.write(SimpleXML::escape(ClientManager::getInstance()->getNicks(j->getUser()->getCID())[0], tmp, true));
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
	} catch(...) {
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
static const string sDownloaded = "Downloaded";
static const string sPriority = "Priority";
static const string sSource = "Source";
static const string sNick = "Nick";
static const string sDirectory = "Directory";
static const string sAdded = "Added";
static const string sTTH = "TTH";
static const string sCID = "CID";
static const string sSegment = "Segment";
static const string sStart = "Start";
static const string sAutoPriority = "AutoPriority";
static const string sMaxSegments = "MaxSegments";

void QueueLoader::startTag(const string& name, StringPairList& attribs, bool simple) {
	QueueManager* qm = QueueManager::getInstance();	
	if(!inDownloads && name == "Downloads") {
		inDownloads = true;
	} else if(inDownloads) {
		if(cur == NULL && name == sDownload) {
			Flags::MaskType flags = 0;
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
			QueueItem::Priority p = (QueueItem::Priority)Util::toInt(getAttrib(attribs, sPriority, 3));
			time_t added = static_cast<time_t>(Util::toInt(getAttrib(attribs, sAdded, 4)));
			const string& tthRoot = getAttrib(attribs, sTTH, 5);
			if(tthRoot.empty())
				return;

			string tempTarget = getAttrib(attribs, sTempTarget, 5);
			uint8_t maxSegments = (uint8_t)Util::toInt(getAttrib(attribs, sMaxSegments, 5));
			int64_t downloaded = Util::toInt64(getAttrib(attribs, sDownloaded, 5));
			if (downloaded > size || downloaded < 0)
				downloaded = 0;

			if(added == 0)
				added = GET_TIME();

			QueueItem* qi = qm->fileQueue.find(target);

			if(qi == NULL) {
				qi = qm->fileQueue.add(target, size, flags, p, tempTarget, added, TTHValue(tthRoot));
				if(downloaded > 0) {
					qi->setFlag(QueueItem::FLAG_EXISTS);
					qi->addSegment(Segment(0, downloaded));
					qi->setPriority(qi->calculateAutoPriority());
				}

				bool ap = Util::toInt(getAttrib(attribs, sAutoPriority, 6)) == 1;
				qi->setAutoPriority(ap);
				qi->setMaxSegments(max((uint8_t)1, maxSegments));
				
				qm->fire(QueueManagerListener::Added(), qi);
			}
			if(!simple)
				cur = qi;
		} else if(cur && name == sSegment) {
			int64_t start = Util::toInt64(getAttrib(attribs, sStart, 0));
			int64_t size = Util::toInt64(getAttrib(attribs, sSize, 1));
			
			if(size > 0 && start >= 0 && (start + size) <= cur->getSize()) {
				cur->setFlag(QueueItem::FLAG_EXISTS);
				cur->addSegment(Segment(start, size));
				cur->setPriority(cur->calculateAutoPriority());
			}
		} else if(cur && name == sSource) {
			const string& cid = getAttrib(attribs, sCID, 0);
			if(cid.length() != 39) {
				// Skip loading this source - sorry old users
				return;
			}
			UserPtr user = ClientManager::getInstance()->getUser(CID(cid));
			ClientManager::getInstance()->updateNick(user, getAttrib(attribs, sNick, 1));

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
void QueueManager::on(SearchManagerListener::SR, const SearchResultPtr& sr) throw() {
	bool added = false;
	bool wantConnection = false;
	size_t users = 0;

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
					if(!BOOLSETTING(AUTO_SEARCH_AUTO_MATCH) || (users >= (size_t)SETTING(MAX_AUTO_MATCH_SOURCES)))
						wantConnection = addSource(qi, sr->getUser(), 0);
					added = true;
				} catch(const Exception&) {
					// ...
				}
				break;
			}
		}
	}

	if(added && BOOLSETTING(AUTO_SEARCH_AUTO_MATCH) && (users < (size_t)SETTING(MAX_AUTO_MATCH_SOURCES))) {
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


	vector<pair<string, QueueItem::Priority>> priorities;

	{
		Lock l(cs);

		QueueItem::List um = getRunningFiles();
		for(QueueItem::Iter j = um.begin(); j != um.end(); ++j) {
			QueueItem* q = *j;

			if(q->getAutoPriority()) {
				QueueItem::Priority p1 = q->getPriority();
				if(p1 != QueueItem::PAUSED) {
					QueueItem::Priority p2 = q->calculateAutoPriority();
					if(p1 != p2)
						priorities.push_back(make_pair(q->getTarget(), p2));
				}
			}
			fire(QueueManagerListener::StatusUpdated(), q);
		}
	}

	for(vector<pair<string, QueueItem::Priority>>::const_iterator p = priorities.begin(); p != priorities.end(); p++) {
		setPriority((*p).first, (*p).second);
	}


}

bool QueueManager::dropSource(Download* d) {
	size_t activeSegments = 0, onlineUsers;
	uint64_t overallSpeed;

	{
	    Lock l(cs);

		QueueItem* q = userQueue.getRunning(d->getUser());

		if(!q)
			return false;

   		dcassert(q->isSource(d->getUser()));

		if(!q->isSet(QueueItem::FLAG_AUTODROP))
			return false;

		for(DownloadList::const_iterator i = q->getDownloads().begin(); i != q->getDownloads().end(); i++) {
			if((*i)->getStart() > 0) {
				activeSegments++;
			}

			// more segments won't change anything
			if(activeSegments > 2)
				break;
		}

		onlineUsers = q->countOnlineUsers();
		overallSpeed = q->getAverageSpeed();
	}

	if(!SETTING(DROP_MULTISOURCE_ONLY) || (activeSegments >= 2)) {
		size_t iHighSpeed = SETTING(DISCONNECT_FILE_SPEED);

		if((iHighSpeed == 0 || overallSpeed > iHighSpeed * 1024) && onlineUsers > 2) {
			d->setFlag(Download::FLAG_SLOWUSER);

			if(d->getAverageSpeed() < SETTING(REMOVE_SPEED)*1024) {
				return true;
			} else {
				d->getUserConnection().disconnect();
			}
		}
	}

	return false;
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
		int64_t blockSize = HashManager::getInstance()->getBlockSize(qi->getTTH());
		if(blockSize == 0)
			blockSize = qi->getSize();
		qi->getPartialInfo(outPartialInfo, blockSize);
		
		// Any parts for me?
		wantConnection = qi->isSource(partialSource.getPartialInfo(), blockSize);

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
		if(si->getPartialSource()) {
			si->getPartialSource()->setPartialInfo(partialSource.getPartialInfo());
		}
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

		int64_t blockSize = HashManager::getInstance()->getBlockSize(qi->getTTH());
		if(blockSize == 0)
			blockSize = qi->getSize();
		qi->getPartialInfo(_outPartsInfo, blockSize);
	}

	return !_outPartsInfo.empty();
}

// compare nextQueryTime, get the oldest ones
void QueueManager::FileQueue::findPFSSources(PFSSourceList& sl) 
{
	typedef multimap<time_t, pair<QueueItem::SourceConstIter, const QueueItem*> > Buffer;
	Buffer buffer;
	uint64_t now = GET_TICK();

	for(QueueItem::StringIter i = queue.begin(); i != queue.end(); ++i) {
		const QueueItem* q = i->second;

		if(q->getSize() < PARTIAL_SHARE_MIN_SIZE) continue;

		const QueueItem::SourceList& sources = q->getSources();
		const QueueItem::SourceList& badSources = q->getBadSources();

		for(QueueItem::SourceConstIter j = sources.begin(); j != sources.end(); ++j) {
			if(	(*j).isSet(QueueItem::Source::FLAG_PARTIAL) && (*j).getPartialSource()->getNextQueryTime() <= now &&
				(*j).getPartialSource()->getPendingQueryCount() < 10 && (*j).getPartialSource()->getUdpPort() > 0)
			{
				buffer.insert(make_pair((*j).getPartialSource()->getNextQueryTime(), make_pair(j, q)));
			}
		}

		for(QueueItem::SourceConstIter j = badSources.begin(); j != badSources.end(); ++j) {
			if(	(*j).isSet(QueueItem::Source::FLAG_TTH_INCONSISTENCY) == false && (*j).isSet(QueueItem::Source::FLAG_PARTIAL) &&
				(*j).getPartialSource()->getNextQueryTime() <= now && (*j).getPartialSource()->getPendingQueryCount() < 10 &&
				(*j).getPartialSource()->getUdpPort() > 0)
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

} // namespace dcpp

/**
 * @file
 * $Id$
 */
