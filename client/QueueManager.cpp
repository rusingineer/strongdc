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

#ifndef _WIN32
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#endif

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "QueueManager.h"

#include "ConnectionManager.h"
#include "SearchManager.h"
#include "ClientManager.h"
#include "DownloadManager.h"
#include "CryptoManager.h"
#include "ShareManager.h"
#include "LogManager.h"

#include "UserConnection.h"
#include "SimpleXML.h"
#include "StringTokenizer.h"
#include "DirectoryListing.h"

#include "FileChunksInfo.h"
#include "../pme-1.0.4/pme.h"

#ifdef _WIN32
#define FILELISTS_DIR "FileLists\\"
#else
#define FILELISTS_DIR "filelists/"
#endif

const string QueueManager::USER_LIST_NAME = "MyList.DcLst";

QueueItem* QueueManager::FileQueue::add(const string& aTarget, int64_t aSize, const string& aSearchString, 
						  int aFlags, QueueItem::Priority p, const string& aTempTarget,
						  int64_t aDownloadedBytes, u_int32_t aAdded, const string& freeBlocks/* = Util::emptyString*/, const string& verifiedBlocks /* = Util::emptyString */, const TTHValue* root) throw(QueueException, FileException) 
{
	if(p == QueueItem::DEFAULT)
		p = (aSize <= 64*1024) ? QueueItem::HIGHEST : QueueItem::NORMAL;

	QueueItem* qi = new QueueItem(aTarget, aSize, aSearchString, p, aFlags, aDownloadedBytes, aAdded, root);

	if(BOOLSETTING(AUTO_PRIORITY_DEFAULT) && !qi->isSet(QueueItem::FLAG_USER_LIST) && p != QueueItem::HIGHEST ) {
		qi->setAutoPriority(true);
		qi->setPriority(qi->calculateAutoPriority());
	}

	qi->setMaxSegments(getMaxSegments(qi->getTargetFileName(), qi->getSize()));
	qi->setMaxSegmentsInitial(getMaxSegments(qi->getTargetFileName(), qi->getSize()));

	if(!qi->isSet(QueueItem::FLAG_USER_LIST)) {
		if(aTempTarget.empty()) {
			if(!SETTING(TEMP_DOWNLOAD_DIRECTORY).empty()) {
				qi->setTempTarget(SETTING(TEMP_DOWNLOAD_DIRECTORY) + getTempName(qi->getTargetFileName()));
				if (File::getSize(qi->getTarget()) > 0 ) {
					Util::ensureDirectory(qi->getTempTarget());
					File::renameFile(qi->getTarget(), qi->getTempTarget());
				}
			}
		} else {
			qi->setTempTarget(aTempTarget);
		}
	}

	// create filedatainfo for this item
	if(!qi->isSet(QueueItem::FLAG_USER_LIST) && !qi->isSet(QueueItem::FLAG_MP3_INFO) && !qi->isSet(QueueItem::FLAG_TESTSUR)){
		FileChunksInfo* pChunksInfo = NULL;

		dcassert(!qi->getTempTarget().empty());
		if ( freeBlocks != Util::emptyString ){
			vector<int64_t> v;	
			istringstream is(freeBlocks);
			copy(istream_iterator<int64_t>(is),
			istream_iterator<int64_t>(),
			back_inserter(v)); 
			pChunksInfo = new FileChunksInfo(qi->getTempTarget(), qi->getSize(), &v);
		}else{
			int64_t tmpSize = File::getSize(qi->getTempTarget());
			if(tmpSize > 0){
				vector<int64_t> v;
				v.push_back(tmpSize);
				v.push_back(qi->getSize());
				pChunksInfo = new FileChunksInfo(qi->getTempTarget(), qi->getSize(), &v);
			}else
				new FileChunksInfo(qi->getTempTarget(), qi->getSize(), NULL);
			
		}

		if(pChunksInfo && verifiedBlocks != Util::emptyString){
			vector<int64_t> v;
			istringstream is(verifiedBlocks);
			copy(istream_iterator<int64_t>(is), 
				istream_iterator<int64_t>(), 
				back_inserter(v)); 

			for(vector<int64_t>::iterator i = v.begin(); i + 1< v.end(); i++, i++)
				pChunksInfo->MarkVerifiedBlock(*i, *(i+1));
		}
	}

	if((qi->getDownloadedBytes() > 0))
		qi->setFlag(QueueItem::FLAG_EXISTS);


	dcassert(find(aTarget) == NULL);
	add(qi);

	return qi;
}

void QueueManager::FileQueue::add(QueueItem* qi) {
	if(lastInsert == queue.end())
		lastInsert = queue.insert(make_pair(qi->getTarget(), qi)).first;
	else
		lastInsert = queue.insert(lastInsert, make_pair(qi->getTarget(), qi));
}

QueueItem* QueueManager::FileQueue::find(const string& target) {
	QueueItem::StringIter i = queue.find(target);
	return (i == queue.end()) ? NULL : i->second;
}

QueueItem* QueueManager::FileQueue::findByHash(const string& hash) {
	for(QueueItem::StringIter i = queue.begin(); i != queue.end(); ++i) {
		if (i->second->getSources().size()>=SETTING(MAX_SOURCES))
			continue;
		if(i->second->getTTH() && i->second->getTTH()->toBase32() == hash)
			return i->second;
	}
	return NULL;
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

void QueueManager::FileQueue::find(QueueItem::List& ql, TTHValue* tth) {
	for(QueueItem::StringIter i = queue.begin(); i != queue.end(); ++i) {
		QueueItem* qi = i->second;
		if(qi->getTTH() != NULL && *qi->getTTH() == *tth) {
			ql.push_back(qi);
		}
	}
}

static QueueItem* findCandidate(QueueItem::StringIter start, QueueItem::StringIter end, StringList& recent) {
	QueueItem* cand = NULL;
	for(QueueItem::StringIter i = start; i != end; ++i) {
		QueueItem* q = i->second;

		// We prefer to search for things that are not running...	

		// No user lists
		if(q->isSet(QueueItem::FLAG_USER_LIST))
			continue;

        // No paused downloads
		if(q->getPriority() == QueueItem::PAUSED)
			continue;

		// No files that already have more than 5 online sources

		// Check that we have a search string

		if(!q->getTTH())
			continue;

		// Did we search for it recently?
        if(find(recent.begin(), recent.end(), /*q->getSearchString()*/ q->getTTH()->toBase32()) != recent.end())
			continue;

		cand = q;

	}
	return cand;
}

QueueItem* QueueManager::FileQueue::findAutoSearch(StringList& recent) {
	// We pick a start position at random, hoping that we will find something to search for...
	QueueItem::StringMap::size_type start = (QueueItem::StringMap::size_type)Util::rand(queue.size());

	QueueItem::StringIter i = queue.begin();
	advance(i, start);

	QueueItem* cand = findCandidate(i, queue.end(), recent);
	if(cand == NULL) {
		cand = findCandidate(queue.begin(), i, recent);
	}
	return cand;
}

QueueItem* QueueManager::FileQueue::findHighest(){
	QueueItem* cand = NULL;
	for(QueueItem::StringIter i = queue.begin(); i != queue.end(); ++i) {
		QueueItem* q = i->second;

		if(!q->getTTH())
			continue;

		if(q->getPriority() == QueueItem::HIGHEST)
			return q;

		if(q->getPriority() == QueueItem::PAUSED)
			continue;

		if(!cand){
			cand = q;
			continue;
	}

		if(cand->getPriority() < q->getPriority())
			cand = q;
	}

	return cand;
}

void QueueManager::FileQueue::move(QueueItem* qi, const string& aTarget) {
	if(lastInsert != queue.end() && lastInsert->first == qi->getTarget())
		lastInsert = queue.end();
	queue.erase(qi->getTarget());
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

QueueItem* QueueManager::UserQueue::getNext(const User::Ptr& aUser, QueueItem::Priority minPrio, QueueItem* pNext /* = NULL */) {
	int p = QueueItem::LAST - 1;
	bool fNext = false;

	do {
		QueueItem::UserListIter i = userQueue[p].find(aUser);
		if(i != userQueue[p].end()) {
			dcassert(!i->second.empty());
			if(pNext == NULL || fNext){
			return i->second.front();
			}else{
				QueueItem::Iter iQi = find(i->second.begin(), i->second.end(), pNext);

                if(iQi != i->second.end()){
                    fNext = true;   // found, next is target

					iQi++;
                    if(iQi != i->second.end())
					return *iQi;
				}

			}
		}
		p--;
	} while(p >= minPrio);

	return NULL;
}

void QueueManager::UserQueue::setRunning(QueueItem* qi, const User::Ptr& aUser) {

	if(!qi->isSource(aUser)) return;
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

	// Set the flag to running...
	if(qi->getStatus() == QueueItem::STATUS_WAITING)
		qi->setStatus(QueueItem::STATUS_RUNNING);
	qi->addCurrent(aUser);

	// Move the download to the running list...
	dcassert(running.find(aUser) == running.end());
	running[aUser] = qi;

}

void QueueManager::UserQueue::setWaiting(QueueItem* qi, const User::Ptr& aUser) {
	// This might have been set to wait by removesource already...
	if (running.find(aUser) == running.end()) return;

	dcassert(qi->getStatus() == QueueItem::STATUS_RUNNING);

	// Remove the download from running
	running.erase(aUser);

	// Set flag to waiting
	qi->removeCurrent(aUser);

	if(qi->getCurrents().empty()){
		qi->setStatus(QueueItem::STATUS_WAITING);
	}

	// Add to the userQueue
	add(qi, aUser);
}

QueueItem* QueueManager::UserQueue::getRunning(const User::Ptr& aUser) {
	QueueItem::UserIter i = running.find(aUser);
	return (i == running.end()) ? NULL : i->second;
}

QueueItem* QueueManager::getRunning(const User::Ptr& aUser) {
	return userQueue.getRunning(aUser);
}

void QueueManager::UserQueue::remove(QueueItem* qi) {
		for(QueueItem::Source::Iter i = qi->getSources().begin(); i != qi->getSources().end(); ++i) {
			remove(qi, (*i)->getUser());
		}
}

void QueueManager::UserQueue::remove(QueueItem* qi, const User::Ptr& aUser) {
		// Remove from running...
	if(qi->isCurrent(aUser)){
		dcassert(!qi->getCurrents().empty());
		dcassert(running.find(aUser) != running.end());
		running.erase(aUser);
		qi->removeCurrent(aUser);

		if(qi->getCurrents().empty())
			qi->setStatus(QueueItem::STATUS_WAITING);

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

QueueManager::QueueManager() : lastSave(0), queueFile(Util::getAppPath() + "Queue.xml"), dirty(false), nextSearch(0) { 
	TimerManager::getInstance()->addListener(this); 
	SearchManager::getInstance()->addListener(this);
	ClientManager::getInstance()->addListener(this);
	Util::ensureDirectory(Util::getAppPath() + FILELISTS_DIR);
};

QueueManager::~QueueManager() { 
	SearchManager::getInstance()->removeListener(this);
	TimerManager::getInstance()->removeListener(this); 
	ClientManager::getInstance()->removeListener(this);

	saveQueue();

	if(!BOOLSETTING(KEEP_LISTS)) {
		string path = Util::getAppPath() + FILELISTS_DIR;

#ifdef _WIN32
		WIN32_FIND_DATA data;
		HANDLE hFind;
		
		hFind = FindFirstFile((path + "\\*.bz2").c_str(), &data);
		if(hFind != INVALID_HANDLE_VALUE) {
			do {
				File::deleteFile(path + data.cFileName);			
			} while(FindNextFile(hFind, &data));
			
			FindClose(hFind);
		}
		
		hFind = FindFirstFile((path + "\\*.DcLst").c_str(), &data);
		if(hFind != INVALID_HANDLE_VALUE) {
			do {
				File::deleteFile(path + data.cFileName);			
			} while(FindNextFile(hFind, &data));
			
			FindClose(hFind);
		}

#else
		DIR* dir = opendir(path.c_str());
		if (dir) {
			while (struct dirent* ent = readdir(dir)) {
				if (fnmatch("*.bz2", ent->d_name, 0) == 0 ||
					fnmatch("*.DcLst", ent->d_name, 0) == 0) {
					File::deleteFile(path + ent->d_name);	
				}
			}
			closedir(dir);
		}
#endif
	}
};

void QueueManager::on(TimerManagerListener::Minute, u_int32_t aTick) throw() {
	string searchString;
	string fname;


	{
		Lock l(cs);

 
		if(BOOLSETTING(AUTO_SEARCH) && (aTick >= nextSearch) && (fileQueue.getSize() > 0)) {
			// We keep 30 recent searches to avoid duplicate searches
			while((recent.size() > fileQueue.getSize()) || (recent.size() > 30)) {
				recent.erase(recent.begin());
			}

			QueueItem* qi = fileQueue.findAutoSearch(recent);
			if(qi != NULL && qi->getTTH()) {
				searchString = qi->getTTH()->toBase32();
				fname = Util::getFileName(qi->getTargetFileName());
				recent.push_back(searchString);
			} else
				recent.clear();
		}
	}

	if(!searchString.empty()){
		SearchManager::getInstance()->search("TTH:" + searchString, 0, SearchManager::TYPE_HASH, SearchManager::SIZE_DONTCARE, true);
		nextSearch = aTick + (SETTING(SEARCH_TIME) * 60000);
		if(BOOLSETTING(REPORT_ALTERNATES))
			LogManager::getInstance()->message(CSTRING(ALTERNATES_SEND) + fname, true);		
	}

}

enum { TEMP_LENGTH = 8 };
string QueueManager::getTempName(const string& aFileName) {
	string tmp;
	tmp.reserve(aFileName.length() + TEMP_LENGTH);
	string::size_type j = aFileName.rfind('.');
	
	if(j == string::npos) {
		tmp.append(aFileName);
	} else {
		tmp.append(aFileName.data(), j);
	}
	for(int i = 0; i < TEMP_LENGTH; i++) {
		tmp.append(1, (char)Util::rand('a', 'z'));
	}
	if(j != string::npos) {
		tmp.append(aFileName.data() + j, aFileName.length() - j);
	}
	return tmp;
}

void QueueManager::add(const string& aFile, int64_t aSize, User::Ptr aUser, const string& aTarget, 
					   const TTHValue* root, const string& aSearchString /* = Util::emptyString */,
					   int aFlags /* = QueueItem::FLAG_RESUME */, QueueItem::Priority p /* = QueueItem::DEFAULT */,
					   const string& aTempTarget /* = Util::emptyString */, bool addBad /* = true */) throw(QueueException, FileException) 
{
	bool wantConnection = true;
	bool newItem = false;

	dcassert((aFile != USER_LIST_NAME) || (aFlags &QueueItem::FLAG_USER_LIST));

	// Check that we're not downloading from ourselves...
	if(aUser->getClientNick() == aUser->getNick()) {
		throw QueueException(STRING(NO_DOWNLOADS_FROM_SELF));
	}

	bool utf8 = (aFlags & QueueItem::FLAG_SOURCE_UTF8) > 0;
	aFlags &= ~QueueItem::FLAG_SOURCE_UTF8;

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

		QueueItem* q = fileQueue.find(target);
		if(q == NULL) {
			q = fileQueue.add(target, aSize, aSearchString, aFlags, p, aTempTarget, 0, GET_TIME(), Util::emptyString, Util::emptyString, root);
			fire(QueueManagerListener::Added(), q);

			newItem = true;
		} else {
			// first source set length
			if(q->getSize() == 0 && root && q->getTTH() && (*root == *q->getTTH())){
				FileChunksInfo::Ptr fi = FileChunksInfo::Get(q->getTempTarget());
				if(fi){
					fi->SetFileSize(aSize);
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
			q->setFlag(aFlags);

			// We don't add any more sources to user list downloads...
			if(q->isSet(QueueItem::FLAG_USER_LIST))
				return;
		}

		wantConnection = addSource(q, aFile, aUser, addBad, utf8);
	}

	if(wantConnection && aUser->isOnline())
	ConnectionManager::getInstance()->getDownloadConnection(aUser);

	// auto search, prevent DEAKLOCK
	if(newItem && root){
		SearchManager::getInstance()->search("TTH:" + TTHValue(*root).toBase32(), 0, SearchManager::TYPE_HASH, SearchManager::SIZE_DONTCARE);
	}
}

void QueueManager::readd(const string& target, User::Ptr& aUser) throw(QueueException) {
	bool wantConnection = false;
	{
		Lock l(cs);
		QueueItem* q = fileQueue.find(target);
		if(q != NULL && q->isBadSource(aUser)) {
			QueueItem::Source* s = *q->getBadSource(aUser);
			wantConnection = addSource(q, s->getPath(), aUser, true, s->isSet(QueueItem::Source::FLAG_UTF8));
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
bool QueueManager::addSource(QueueItem* qi, const string& aFile, User::Ptr aUser, bool addBad, bool utf8) throw(QueueException, FileException) {
	QueueItem::Source* s = NULL;
	bool wantConnection = (qi->getPriority() != QueueItem::PAUSED);

	if(qi->isSource(aUser, aFile)) {
		throw QueueException(STRING(DUPLICATE_SOURCE));
	}

	if(!addBad && qi->isBadSource(aUser, aFile)) {
		throw QueueException(STRING(DUPLICATE_SOURCE));
	}

	s = qi->addSource(aUser, aFile);
	if(utf8)
		s->setFlag(QueueItem::Source::FLAG_UTF8);

	if(aUser->isSet(User::PASSIVE) && (SETTING(CONNECTION_TYPE) != SettingsManager::CONNECTION_ACTIVE) ) {
		qi->removeSource(aUser, QueueItem::Source::FLAG_PASSIVE);
		wantConnection = false;
	} else {//if((getMaxSegments(qi->getTargetFileName(), qi->getSize()) > qi->getCurrents().size()) && (qi->getStatus() != QueueItem::STATUS_RUNNING)) {
		if ((!SETTING(SOURCEFILE).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
			PlaySound(SETTING(SOURCEFILE).c_str(), NULL, SND_FILENAME | SND_ASYNC);
		userQueue.add(qi, aUser);
	} /*else {
		wantConnection = false;
	}*/

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

static DirectoryListing* curDl = NULL;
static SizeMap sizeMap;
static string utfTmp;

static const string& utfEscaper(const string& x) {
	return curDl->getUtf8() ? Util::toAcp(x, utfTmp) : x;
}

int QueueManager::matchFiles(DirectoryListing::Directory* dir) throw() {
	int matches = 0;
	for(DirectoryListing::Directory::Iter j = dir->directories.begin(); j != dir->directories.end(); ++j) {
		if(!(*j)->getAdls())
			matches += matchFiles(*j);
	}

	for(DirectoryListing::File::Iter i = dir->files.begin(); i != dir->files.end(); ++i) {
		DirectoryListing::File* df = *i;

		SizePair files = sizeMap.equal_range(adjustSize((u_int32_t)df->getSize(), df->getName()));
		for(SizeIter j = files.first; j != files.second; ++j) {
			QueueItem* qi = j->second;
			bool equal = false;
			if(qi->getTTH() != NULL && df->getTTH() != NULL) {
				equal = (*qi->getTTH() == *df->getTTH());
			} else {
			if(Util::stricmp(utfEscaper(df->getName()), qi->getTargetFileName()) == 0) {
				dcassert(df->getSize() == qi->getSize());			
					equal = true;

					if(df->getTTH() != NULL) {
						dcassert(qi->getTTH() == NULL);
						qi->setTTH(new TTHValue(*df->getTTH()));
					}
				}
			}
			if(equal) {
				try {
					addSource(qi, curDl->getPath(df) + df->getName(), curDl->getUser(), false, curDl->getUtf8());
					matches++;
				} catch(const Exception&) {
				}
			}
		}
	}
	return matches;
}

int QueueManager::matchListing(DirectoryListing* dl) throw() {
	int matches = 0;
	{
		Lock l(cs);
		sizeMap.clear();
		matches = 0;
		curDl = dl;
		for(QueueItem::StringIter i = fileQueue.getQueue().begin(); i != fileQueue.getQueue().end(); ++i) {
			QueueItem* qi = i->second;
			if(qi->getSize() != -1) {
				sizeMap.insert(make_pair(adjustSize((u_int32_t)qi->getSize(), qi->getTarget()), qi));
			}
		}

		matches = matchFiles(dl->getRoot());
	}
	if(matches > 0)
	ConnectionManager::getInstance()->getDownloadConnection(dl->getUser());
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
					addSource(qt, s->getPath(), s->getUser(), true, s->isSet(QueueItem::Source::FLAG_UTF8));
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

QueueItem* QueueManager::lookupNext(User::Ptr& aUser) throw() {
    Lock l(cs);
    return userQueue.getNext(aUser);
}

bool QueueManager::FileQueue::matchExtension(const string& aString, const string& aExt) {
	PME reg(aExt,"i");
	if(reg.match(aString)) { return true; }
	return false;
}

int QueueManager::FileQueue::getMaxSegments(string filename, int64_t filesize) {
	int MaxSegments = 1;

	string s1 = filename;
	s1.erase(0,s1.find_last_of('.'));

	if(SETTING(SEGMENTS_TYPE)==SettingsManager::SEGMENT_ON_SIZE) {
		if( (filesize>=(SETTING(SET_MIN2)*1024*1024)) && (filesize<(SETTING(SET_MAX2)*1024*1024))) { MaxSegments = 2;}
		else
			if( (filesize>=(SETTING(SET_MIN3)*1024*1024)) && (filesize<(SETTING(SET_MAX3)*1024*1024))) { MaxSegments = 3;}
		else
			if( (filesize>=(SETTING(SET_MIN4)*1024*1024)) && (filesize<(SETTING(SET_MAX4)*1024*1024))) { MaxSegments = 4;}
		else
			if( (filesize>=(SETTING(SET_MIN6)*1024*1024)) && (filesize<(SETTING(SET_MAX6)*1024*1024))) { MaxSegments = 6;}
		else
			if(filesize>=(SETTING(SET_MIN8)*1024*1024)) { MaxSegments = 8;}
	} else
	if(SETTING(SEGMENTS_TYPE)==SettingsManager::SEGMENT_ON_CONNECTION) { 
		if (SETTING(CONNECTION) == SettingsManager::connectionSpeeds[SettingsManager::SPEED_MODEM]) { MaxSegments = 3;
		} else if(SETTING(CONNECTION) == SettingsManager::connectionSpeeds[SettingsManager::SPEED_ISDN]) {  MaxSegments = 4;
		} else if(SETTING(CONNECTION) == SettingsManager::connectionSpeeds[SettingsManager::SPEED_SATELLITE]) { MaxSegments = 5;
		} else if(SETTING(CONNECTION) == SettingsManager::connectionSpeeds[SettingsManager::SPEED_WIRELESS]) { MaxSegments = 6;
		} else if(SETTING(CONNECTION) == SettingsManager::connectionSpeeds[SettingsManager::SPEED_DSL]) { MaxSegments = 8;
		} else if(SETTING(CONNECTION) == SettingsManager::connectionSpeeds[SettingsManager::SPEED_CABLE]) { MaxSegments = 8;
		} else if(SETTING(CONNECTION) == SettingsManager::connectionSpeeds[SettingsManager::SPEED_T1]) { MaxSegments = 10;
		} else if(SETTING(CONNECTION) == SettingsManager::connectionSpeeds[SettingsManager::SPEED_T3]) { MaxSegments = 15;
		}			
	} else
	if(SETTING(SEGMENTS_TYPE)==SettingsManager::SEGMENT_MANUAL) {
		MaxSegments = SETTING(NUMBER_OF_SEGMENTS);
	} 

	if(matchExtension(s1,SETTING(DONT_EXTENSIONS))) {  MaxSegments = 1; }

 return MaxSegments;
}

Download* QueueManager::getDownload(User::Ptr& aUser, UserConnection* aConn) throw() {
	Lock l(cs);

	string message = "";
	QueueItem* q = userQueue.getNext(aUser);
	Download *d;

again:

	if(q == NULL) {
		dcdebug(("Message : "+message+"\n").c_str());
		if(message != "")
			ConnectionManager::getInstance()->fire(ConnectionManagerListener::Failed(), aConn->getCQI(), message);
		return NULL;
	}

	if(((q->getActiveSegments().size() + q->getCurrents().size()) / 2) >= q->getMaxSegments()) {
		message = STRING(ALL_SEGMENTS_TAKEN);		
		q = userQueue.getNext(aUser, QueueItem::LOWEST, q);
		goto again;
	}

	if(!q->isSet(QueueItem::FLAG_USER_LIST) && !q->isSet(QueueItem::FLAG_TESTSUR) && !q->isSet(QueueItem::FLAG_MP3_INFO) &&
		BOOLSETTING(DONT_BEGIN_SEGMENT) && (SETTING(DONT_BEGIN_SEGMENT_SPEED) > 0)) {
		if(DownloadManager::getInstance()->getWholeFileSpeed(q->getTarget()) > SETTING(DONT_BEGIN_SEGMENT_SPEED)*1024) {
			message = STRING(ALL_SEGMENTS_TAKEN);		
			q = userQueue.getNext(aUser, QueueItem::LOWEST, q);
			goto again;
		}
	}

	int64_t freeBlock = 0;

	if(!q->isSet(QueueItem::FLAG_USER_LIST) && !q->isSet(QueueItem::FLAG_TESTSUR) && !q->isSet(QueueItem::FLAG_MP3_INFO)){
		dcassert(!q->getTempTarget().empty());
		freeBlock = FileChunksInfo::Get(q->getTempTarget())->GetUndlStart(q->getMaxSegments());

		if(freeBlock == -1){
			message = STRING(NO_FREE_BLOCK);
			q = userQueue.getNext(aUser, QueueItem::LOWEST, q);
			goto again;
		}


	}

	userQueue.setRunning(q, aUser);

	fire(QueueManagerListener::StatusUpdated(), q);
	
	d = new Download(q, aUser);
/*
	if( BOOLSETTING(ANTI_FRAG) ) {
		d->setStartPos(q->getDownloadedBytes());
	}
*/
	if(!q->isSet(QueueItem::FLAG_USER_LIST)){
		d->setStartPos(freeBlock);
	}

	return d;
}


void QueueManager::putDownload(Download* aDownload, bool finished /* = false */) throw() {
	User::List getConn;
	string fname;
	User::Ptr up;
	int flag = 0;

	{
		Lock l(cs);
		QueueItem* q = fileQueue.find(aDownload->getTarget());

		if(q != NULL) {

			if(finished) {
				dcassert(q->getStatus() == QueueItem::STATUS_RUNNING);

				userQueue.remove(q);
				if(aDownload->isSet(Download::FLAG_USER_LIST)) {
					if(aDownload->getSource() == "files.xml.bz2") {
						q->setFlag(QueueItem::FLAG_XML_BZLIST);
					} else if(aDownload->getSource() == "MyList.bz2") {
						q->setFlag(QueueItem::FLAG_BZLIST);
					}
				}
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
			} else {
				//if(!aDownload->isSet(Download::FLAG_TREE_DOWNLOAD))
				//	 q->setDownloadedBytes(aDownload->getPos());
				//q->setCurrentDownload(NULL);
				if(q->getDownloadedBytes() > 0)
					q->setFlag(QueueItem::FLAG_EXISTS);

				if(q->getPriority() != QueueItem::PAUSED) {
					for(QueueItem::Source::Iter j = q->getSources().begin(); j != q->getSources().end(); ++j) {
						if((*j)->getUser()->isOnline()) {
							getConn.push_back((*j)->getUser());
						}
					}
				}

				// This might have been set to wait by removesource already...
				userQueue.setWaiting(q, aDownload->getUserConnection()->getUser());
				if(q->getStatus() != QueueItem::STATUS_RUNNING) {
					fire(QueueManagerListener::StatusUpdated(), q);
				}

				if(q->isSet(QueueItem::FLAG_USER_LIST)) {
					// Blah...no use keeping an unfinished file list...
					File::deleteFile(q->getListName());
				}
			}
		} else {
			if(!aDownload->getTempTarget().empty() && aDownload->getTempTarget() != aDownload->getTarget()) {
				File::deleteFile(aDownload->getTempTarget() + Download::ANTI_FRAG_EXT);
				File::deleteFile(aDownload->getTempTarget());
			}
		}
		aDownload->setUserConnection(NULL);

		delete aDownload;
	}

	for(User::Iter i = getConn.begin(); i != getConn.end(); ++i) {
	ConnectionManager::getInstance()->getDownloadConnection(*i);
	}

	if(!fname.empty()) {
		DirectoryListing dirList(up);
		try {
			dirList.loadFile(fname, false);
		} catch(const Exception&) {
			addList(up, flag);
			return;
		}

		if(flag & QueueItem::FLAG_DIRECTORY_DOWNLOAD) {
			DirectoryItem::List dl;
			{
				Lock l(cs);
				DirectoryItem::DirectoryPair dp = directories.equal_range(up);
				for(DirectoryItem::DirectoryIter i = dp.first; i != dp.second; ++i) {
					dl.push_back(i->second);
				}
				directories.erase(up);
			}

			for(DirectoryItem::Iter i = dl.begin(); i != dl.end(); ++i) {
				DirectoryItem* di = *i;
				dirList.download(di->getName(), di->getTarget());
				delete di;
			}
		}
		if(flag & QueueItem::FLAG_MATCH_QUEUE) {
			AutoArray<char> tmp(STRING(MATCHED_FILES).size() + 16);
			sprintf(tmp, CSTRING(MATCHED_FILES), matchListing(&dirList));
			LogManager::getInstance()->message(up->getNick() + ": " + string(tmp), true);			
		}
	}
}

void QueueManager::remove(const string& aTarget) throw() {
	string x;
	string temptarget;
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

			temptarget = q->getTempTarget();
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

void QueueManager::removeSource(const string& aTarget, User::Ptr& aUser, int reason, bool removeConn /* = true */) throw() {
	Lock l(cs);
	QueueItem* q = fileQueue.find(aTarget);
	string x;
	if(q != NULL) {
		dcassert(q->isSource(aUser));
		if(q->isSet(QueueItem::FLAG_USER_LIST)) {
			remove(q->getTarget());
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
		if((q->getStatus() == QueueItem::STATUS_RUNNING) && q->isCurrent(aUser)) {
			if(removeConn)
				x = q->getTarget();
			userQueue.setWaiting(q, aUser);
			userQueue.remove(q, aUser);
		} else if(q->getStatus() == QueueItem::STATUS_WAITING) {
			userQueue.remove(q, aUser);
		}

		q->removeSource(aUser, reason);
		
		fire(QueueManagerListener::SourcesUpdated(), q);
		setDirty();
	}
	if(!x.empty()) {
		DownloadManager::getInstance()->abortDownload(x, aUser);
	}
}

void QueueManager::removeSources(User::Ptr& aUser, int reason) throw() {
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

	{
		Lock l(cs);
	
		QueueItem* q = fileQueue.find(aTarget);
		if( (q != NULL) && (q->getPriority() != p) ) {
			if( q->getStatus() != QueueItem::STATUS_RUNNING ) {
				if(q->getPriority() == QueueItem::PAUSED) {
					// Problem, we have to request connections to all these users...
					q->getOnlineUsers(ul);
				}

				userQueue.remove(q);
				q->setPriority(p);
				userQueue.add(q);
			} else {
				for(QueueItem::Source::Iter i = q->getSources().begin(); i != q->getSources().end(); ++i) {
					if(!q->isCurrent((*i)->getUser())) userQueue.remove(q, (*i)->getUser());
				}
				q->setPriority(p);
				for(QueueItem::Source::Iter i = q->getSources().begin(); i != q->getSources().end(); ++i) {
					if(!q->isCurrent((*i)->getUser())) userQueue.add(q, (*i)->getUser());
				}
			}
			setDirty();
			fire(QueueManagerListener::StatusUpdated(), q);
		}
	}

	for(User::Iter i = ul.begin(); i != ul.end(); ++i) {
	ConnectionManager::getInstance()->getDownloadConnection(*i);
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

void QueueManager::setSearchString(const string& aTarget, const string& searchString) throw()
{
	Lock l(cs);

	QueueItem* q = fileQueue.find(aTarget);
	if( (q != NULL) && (q->getSearchString() != searchString) ) {
		q->setSearchString(searchString);
		setDirty();
		fire(QueueManagerListener::SearchStringUpdated(), q);
	}
}

void QueueManager::saveQueue() throw() {

	Lock l(cs);

	try {
		
#define STRINGLEN(n) n, sizeof(n)-1
#define CHECKESCAPE(n) SimpleXML::escape(n, tmp, true)

		File ff(getQueueFile() + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		BufferedOutputStream<false> f(&ff);
		
		f.write(SimpleXML::w1252Header);
		f.write(STRINGLEN("<Downloads>\r\n"));
		string tmp;
		string b32tmp;
		for(QueueItem::StringIter i = fileQueue.getQueue().begin(); i != fileQueue.getQueue().end(); ++i) {
			QueueItem* d = i->second;
			if((!d->isSet(QueueItem::FLAG_USER_LIST)) && (!d->isSet(QueueItem::FLAG_TESTSUR)) && (!d->isSet(QueueItem::FLAG_MP3_INFO))) {
				f.write(STRINGLEN("\t<Download Target=\""));
				f.write(CHECKESCAPE(d->getTarget()));
				f.write(STRINGLEN("\" Size=\""));
				f.write(Util::toString(d->getSize()));
				f.write(STRINGLEN("\" Priority=\""));
				f.write(Util::toString((int)d->getPriority()));
				f.write(STRINGLEN("\" TempTarget=\""));
				f.write(CHECKESCAPE(d->getTempTarget()));
				f.write(STRINGLEN("\" FreeBlocks=\""));
				f.write(FileChunksInfo::Get(d->getTempTarget())->getFreeBlocksString());
				f.write(STRINGLEN("\" VerifiedBlocks=\""));
				f.write(FileChunksInfo::Get(d->getTempTarget())->getVerifiedBlocksString());
				f.write(STRINGLEN("\" Added=\""));
				f.write(Util::toString(d->getAdded()));
				if(d->getTTH() != NULL) {
					b32tmp.clear();
					f.write(STRINGLEN("\" TTH=\""));
					f.write(d->getTTH()->toBase32(b32tmp));
				}
				if(!d->getSearchString().empty()) {
					f.write(STRINGLEN("\" SearchString=\""));
					f.write(CHECKESCAPE(d->getSearchString()));
				}
				if(d->getDownloadedBytes() != 0) {
					f.write(STRINGLEN("\" Downloaded=\""));
					f.write(Util::toString(d->getDownloadedBytes()));
				}
				f.write(STRINGLEN("\" AutoPriority=\""));
				f.write(Util::toString(d->getAutoPriority()));

				f.write(STRINGLEN("\" MaxSegments=\""));
				f.write(Util::toString(d->getMaxSegments()));

				f.write(STRINGLEN("\" MaxSegmentsInitial=\""));
				f.write(Util::toString(d->getMaxSegmentsInitial()));

				f.write(STRINGLEN("\">\r\n"));

				for(QueueItem::Source::List::const_iterator j = d->sources.begin(); j != d->sources.end(); ++j) {
					QueueItem::Source* s = *j;
					f.write(STRINGLEN("\t\t<Source Nick=\""));
					f.write(CHECKESCAPE(s->getUser()->getNick()));
					f.write(STRINGLEN("\" Path=\""));
					f.write(CHECKESCAPE(s->getPath()));
					f.write(STRINGLEN("\" Utf8=\""));
					f.write(s->isSet(QueueItem::Source::FLAG_UTF8) ? "1" : "0", 1);
					f.write(STRINGLEN("\"/>\r\n"));
				}

				f.write(STRINGLEN("\t</Download>\r\n"));
			}
		}

		for(DirectoryItem::DirectoryIter j = directories.begin(); j != directories.end(); ++j) {
			DirectoryItem::Ptr d = j->second;
			f.write(STRINGLEN("\t<Directory Target=\""));
			f.write(CHECKESCAPE(d->getTarget()));
			f.write(STRINGLEN("\" Nick=\""));
			f.write(CHECKESCAPE(d->getUser()->getNick()));
			f.write(STRINGLEN("\" Priority=\""));
			f.write(Util::toString((int)d->getPriority()));
			f.write(STRINGLEN("\" Source=\""));
			f.write(CHECKESCAPE(d->getName()));
			f.write(STRINGLEN("\"/>\r\n"));
		}
		
		f.write("</Downloads>\r\n");
		f.flush();
		ff.close();

		File::deleteFile(getQueueFile() + ".bak");
		CopyFile(getQueueFile().c_str(), (getQueueFile() + ".bak").c_str(), FALSE);
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
static const string sSearchString = "SearchString";
static const string sAdded = "Added";
static const string sUtf8 = "Utf8";
static const string sTTH = "TTH";
static const string sFreeBlocks = "FreeBlocks";
static const string sVerifiedBlocks = "VerifiedBlocks";
static const string sAutoPriority = "AutoPriority";
static const string sMaxSegments = "MaxSegments";
static const string sMaxSegmentsInitial = "MaxSegmentsInitial";

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
				target = QueueManager::checkTarget(getAttrib(attribs, sTarget, 0), size, flags);
				if(target.empty())
					return;
			} catch(const Exception&) {
				return;
			}
			const string& freeBlocks = getAttrib(attribs, sFreeBlocks, 1);
			const string& verifiedBlocks = getAttrib(attribs, sVerifiedBlocks, 2);

			QueueItem::Priority p = (QueueItem::Priority)Util::toInt(getAttrib(attribs, sPriority, 3));
			const string& tempTarget = getAttrib(attribs, sTempTarget, 4);
			u_int32_t added = (u_int32_t)Util::toInt(getAttrib(attribs, sAdded, 5));
			const string& tthRoot = getAttrib(attribs, sTTH, 6);
			const string& searchString = getAttrib(attribs, sSearchString, 6);
			int64_t downloaded = Util::toInt64(getAttrib(attribs, sDownloaded, 6));
			int maxsegments = Util::toInt(getAttrib(attribs, sMaxSegments, 6));
			int maxsegmentsinitial = Util::toInt(getAttrib(attribs, sMaxSegmentsInitial, 6));
			if (downloaded > size || downloaded < 0)
				downloaded = 0;

			if(added == 0)
				added = GET_TIME();

			QueueItem* qi = qm->fileQueue.find(target);

			if(qi == NULL) {
				if(tthRoot.empty())	
					qi = qm->fileQueue.add(target, size, searchString, flags, p, tempTarget, downloaded, added, freeBlocks, verifiedBlocks, NULL);
				else {
					TTHValue root(tthRoot);
					qi = qm->fileQueue.add(target, size, searchString, flags, p, tempTarget, downloaded, added, freeBlocks, verifiedBlocks, &root);
				}
				bool ap = Util::toInt(getAttrib(attribs, sAutoPriority, 6)) == 1;
				qi->setAutoPriority(ap);

				if(maxsegmentsinitial > 0) qi->setMaxSegmentsInitial(maxsegmentsinitial);
				if(maxsegments > 1) qi->setMaxSegments(maxsegments);
					else qi->setMaxSegments(1);
				
				qm->fire(QueueManagerListener::Added(), qi);
			}
			if(!simple)
				cur = qi;
		} else if(cur != NULL && name == sSource) {
			const string& nick = getAttrib(attribs, sNick, 0);
			if(nick.empty())
				return;
			const string& path = getAttrib(attribs, sPath, 1);
			if(path.empty())
				return;
			const string& utf8 = getAttrib(attribs, sUtf8, 2);
			bool isUtf8 = (utf8 == "1");
			User::Ptr user = ClientManager::getInstance()->getUser(nick);
			try {
				if(qm->addSource(cur, path, user, false, isUtf8) && user->isOnline())
				ConnectionManager::getInstance()->getDownloadConnection(user);
			} catch(const Exception&) {
				return;
			}
		} else if(cur == NULL && name == sDirectory) {
			const string& targetd = getAttrib(attribs, sTarget, 0);
			if(targetd.empty())
				return;
			const string& nick = getAttrib(attribs, sNick, 1);
			if(nick.empty())
				return;
			QueueItem::Priority p = (QueueItem::Priority)Util::toInt(getAttrib(attribs, sPriority, 2));
			const string& source = getAttrib(attribs, sSource, 3);
			if(source.empty())
				return;
			
			qm->addDirectory(source, ClientManager::getInstance()->getUser(nick), targetd, p);
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
	//bool found = false;
	string target;
	if(BOOLSETTING(AUTO_SEARCH) && sr->getTTH()) {
		Lock l(cs);
		QueueItem::List matches;

		if(QueueItem* qi = fileQueue.findByHash(sr->getTTH()->toBase32())) {
		// Wow! found a new source that seems to match...add it...
			target = qi->getTarget();
		}
	}

	if(!target.empty()) {
		try {
			add(sr->getFile(), sr->getSize(), sr->getUser(), target, sr->getTTH(), Util::emptyString, QueueItem::FLAG_RESUME, QueueItem::DEFAULT, Util::emptyString, false);
			//dcdebug("QueueManager::onAction New source %s for target %s found\n", sr->getUser()->getNick().c_str(), qi->getTarget().c_str());
		} catch(const Exception&) {
			// ...
		}
	}
}

// ClientManagerListener
void QueueManager::on(ClientManagerListener::UserUpdated, const User::Ptr& aUser) throw() {
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
		}

	if(aUser->isOnline() && hasDown)	
	ConnectionManager::getInstance()->getDownloadConnection(aUser);
}

void QueueManager::on(TimerManagerListener::Second, u_int32_t aTick) throw() {
		if(dirty && ((lastSave + 10000) < aTick)) {
			saveQueue();
		}
		if((lastSave + SETTING(AUTOSAVE_QUEUE)*1000 + 1000) < aTick) {
			saveQueue();
		}
}


// add to queue by hash without source, TODO:size=0 is allowed
bool QueueManager::add(const string& aFile, int64_t aSize, const string& tth) throw(QueueException, FileException) 
{	
	bool newItem = false;
	dcassert(aFile != USER_LIST_NAME);

	string target = SETTING(DOWNLOAD_DIRECTORY) + aFile;
	string tempTarget = SETTING(TEMP_DOWNLOAD_DIRECTORY) + aFile;

	int flag = QueueItem::FLAG_RESUME;

	try{
		target = checkTarget(target, aSize, flag);
		if(target.empty())
			return false;
	} catch(const Exception&) {
		return false;
	}
	
	//// Check if it's a zero-byte file, if so, create and return...

	//if(aSize == 0) {
	//	if(!BOOLSETTING(SKIP_ZERO_BYTE)) {
	//		File f(target, File::WRITE, File::CREATE);
	//	}
	//	return false;
	//}

	{
		Lock l(cs);

		QueueItem* q = fileQueue.find(target);
		if(q == NULL) {
			q = fileQueue.findByHash(tth);
			if(q == NULL){
				TTHValue root(tth);
				q = fileQueue.add(target, aSize, "", flag, QueueItem::DEFAULT, tempTarget, 0, GET_TIME(), Util::emptyString, Util::emptyString, &root);
				fire(QueueManagerListener::Added(), q);
				newItem = true;
			}
		}
	}

	if(newItem){
		SearchManager::getInstance()->search("TTH:" + tth, 0, SearchManager::TYPE_HASH, SearchManager::SIZE_DONTCARE);
		return true;
	}

	return false;
}

void QueueManager::autoDropSource(User::Ptr& aUser)
{
    Lock l(cs);

    QueueItem* q = userQueue.getRunning(aUser);

    if(!q) return;

    dcassert(q->isSource(aUser));

    // Don't drop only downloading source
    if(q->currents.size() < 2) return;

    userQueue.setWaiting(q, aUser);
    userQueue.remove(q, aUser);

    q->removeSource(aUser, QueueItem::Source::FLAG_REMOVED);

	fire(QueueManagerListener::StatusUpdated(), q);
    setDirty();

    DownloadManager::getInstance()->abortDownload(q->getTarget(), aUser);
}

void QueueManager::sendAutoSearch(Client* c)
{
    Lock l(cs);
	QueueItem::StringMap& queue = fileQueue.getQueue();

	for(QueueItem::StringMap::iterator i = queue.begin(); i != queue.end(); i++)
	{
		if(i->second->getTTH())
		{
			c->search(SearchManager::SIZE_DONTCARE, 0, SearchManager::TYPE_HASH, "TTH:" + i->second->getTTH()->toBase32(), true);
		}
	}
}

/**
 * @file
 * $Id$
 */