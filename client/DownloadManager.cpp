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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "DownloadManager.h"

#include "ConnectionManager.h"
#include "QueueManager.h"
#include "CryptoManager.h"
#include "HashManager.h"

#include "LogManager.h"
#include "SFVReader.h"
#include "User.h"
#include "File.h"
#include "FilteredFile.h"
#include "TraceManager.h"
#include "Client.h"

#include <limits>

static const string DOWNLOAD_AREA = "Downloads";
const string Download::ANTI_FRAG_EXT = ".antifrag";

Download::Download() throw() : file(NULL),
crcCalc(NULL), treeValid(false), oldDownload(false), tth(NULL) { 
}

Download::Download(QueueItem* qi, User::Ptr& aUser) throw() : source(qi->getSourcePath(aUser)),
	target(qi->getTarget()), tempTarget(qi->getTempTarget()), file(NULL), finished(false),
	crcCalc(NULL), treeValid(false), oldDownload(false), quickTick(GET_TICK()), tth(qi->getTTH()), 
	maxSegmentsInitial(qi->getMaxSegmentsInitial()), userNick(aUser->getNick()) { 
	
	setSize(qi->getSize());
	if(qi->isSet(QueueItem::FLAG_USER_LIST))
		setFlag(Download::FLAG_USER_LIST);
	if(qi->isSet(QueueItem::FLAG_MP3_INFO))
		setFlag(Download::FLAG_MP3_INFO);
	if(qi->isSet(QueueItem::FLAG_CHECK_FILE_LIST))
		setFlag(Download::FLAG_CHECK_FILE_LIST);
	if(qi->isSet(QueueItem::FLAG_TESTSUR)) {
		setFlag(Download::FLAG_TESTSUR);
	} else {
	if(qi->isSet(QueueItem::FLAG_RESUME))
		setFlag(Download::FLAG_RESUME);
	if((*(qi->getSource(aUser)))->isSet(QueueItem::Source::FLAG_UTF8))
		setFlag(Download::FLAG_UTF8);
}
};

void DownloadManager::on(TimerManagerListener::Second, u_int32_t /*aTick*/) throw() {
	Lock l(cs);

	Download::List tickList;
	iSpeed = SETTING(I_DOWN_SPEED);
	iHighSpeed = SETTING(H_DOWN_SPEED);
	iTime = SETTING(DOWN_TIME) * 60;
	throttleSetup();
	throttleZeroCounters();
	// Tick each ongoing download
	for(Download::Iter i = downloads.begin(); i != downloads.end(); ++i) {
        if((*i)->getStart() &&  0 == ((int)(GET_TICK() - (*i)->getStart()) / 1000 + 1) % 20 && BOOLSETTING(AUTO_DROP_SOURCE) ) // check every 20 sec
        {
            if((*i)->getRunningAverage() < 1230){
                QueueManager::getInstance()->autoDropSource((*i)->getUserConnection()->getUser());
                continue;
            }
        }

		if(((*i)->getTotal() > 0) && (!(*i)->finished)) {
			tickList.push_back(*i);
			QueueManager::getInstance()->updateSource(QueueManager::getInstance()->getRunning((*i)->getUserConnection()->getUser()));
		}

		if(BOOLSETTING(DISCONNECTING_ENABLE)) {
			if(getAverageSpeed() < (iHighSpeed*1024)) {
				Download* d = *i;
				dcassert(d->getUserConnection() != NULL);
				if (d->getSize() > (SETTING(MIN_FILE_SIZE) * (1024*1024))) {
					QueueItem* q = QueueManager::getInstance()->getRunning(d->getUserConnection()->getUser());
					if(q != NULL) {		
						if(d->getRunningAverage() < (iSpeed*1024) && (q->countOnlineUsers() >= 2) && (!d->isSet(Download::FLAG_USER_LIST))) {
							if(((GET_TICK() - d->quickTick)/1000) > iTime){
								d->getUserConnection()->disconnect();
								if(d->getRunningAverage() < (SETTING(DISCONNECT)*1024)) { 
									QueueManager::getInstance()->removeSources(d->getUserConnection()->getUser(),QueueItem::Source::FLAG_SLOW);
								}
							}
						} else {
								d->quickTick = GET_TICK();
						}
					}
				}
			}
		}
	}

	if(tickList.size() > 0)
		fire(DownloadManagerListener::Tick(), tickList);
}

void DownloadManager::FileMover::moveFile(const string& source, const string& target) {
	Lock l(cs);
	files.push_back(make_pair(source, target));
	if(!active) {
		active = true;
		start();
	}
}

int DownloadManager::FileMover::run() {
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
			// Too bad...
		}
	}
}

void DownloadManager::removeConnection(UserConnection::Ptr aConn, bool reuse /* = false */, bool reconnect /* = false */) {
	dcassert(aConn->getDownload() == NULL);
	aConn->removeListener(this);
	ConnectionManager::getInstance()->putDownloadConnection(aConn, reuse, reconnect);
}

class TreeOutputStream : public OutputStream {
public:
	TreeOutputStream(TigerTree& aTree) : tree(aTree), bufPos(0) {
	}

	virtual size_t write(const void* xbuf, size_t len) throw(Exception) {
		size_t pos = 0;
		u_int8_t* b = (u_int8_t*)xbuf;
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
	u_int8_t buf[TigerTree::HASH_SIZE];
	size_t bufPos;
};

void DownloadManager::checkDownloads(UserConnection* aConn, bool reconn /*=false*/) {

	Download* d = aConn->getDownload();

	bool firstTry = false;
	if(d == NULL) {
		firstTry = true;

	bool slotsFull = (SETTING(DOWNLOAD_SLOTS) != 0) && (getDownloads() >= SETTING(DOWNLOAD_SLOTS));
	bool speedFull = (SETTING(MAX_DOWNLOAD_SPEED) != 0) && (getAverageSpeed() >= (SETTING(MAX_DOWNLOAD_SPEED)*1024));
	if( slotsFull || speedFull ) {
		bool extraFull = (SETTING(DOWNLOAD_SLOTS) != 0) && (getDownloads() >= (SETTING(DOWNLOAD_SLOTS)+SETTING(EXTRA_DOWNLOAD_SLOTS)));
		if(extraFull || !QueueManager::getInstance()->hasDownload(aConn->getUser(), QueueItem::HIGHEST)) {
			removeConnection(aConn);
			return;
		}
	}
	
	// this happen when download finished, we need reconnect.	
	if(reconn && QueueManager::getInstance()->hasDownload(aConn->getUser())) {
		removeConnection(aConn, false, true);
		return;
	}

		d = QueueManager::getInstance()->getDownload(aConn->getUser(), aConn);

	if(d == NULL) {
		removeConnection(aConn, false);
		return;
	}

		{
			Lock l(cs);
			downloads.push_back(d);
		}

		d->setUserConnection(aConn);
		aConn->setDownload(d);
		}

	if(firstTry && !d->getTreeValid() && 
		!d->isSet(Download::FLAG_USER_LIST) && d->getTTH() != NULL)
	{
			if(HashManager::getInstance()->getTree(d->getTarget(), d->getTigerTree())) {
				d->setTreeValid(true);
		} else if(!d->isSet(Download::FLAG_TREE_TRIED) && 
			aConn->isSet(UserConnection::FLAG_SUPPORTS_TTHL)) 
		{
				// So, we need to download the tree...
				Download* tthd = new Download();
				tthd->setOldDownload(d);
				tthd->setFlag(Download::FLAG_TREE_DOWNLOAD);
				tthd->setTarget(d->getTarget());
			tthd->setSource(d->getSource());

				tthd->setUserConnection(aConn);
				aConn->setDownload(tthd);

				aConn->setState(UserConnection::STATE_TREE);
			// Hack to get by TTH if possible
			tthd->setTTH(d->getTTH());
			fire(DownloadManagerListener::Failed(), d, STRING(DOWNLOADING_TTHL));
			aConn->send(tthd->getCommand(false, aConn->isSet(UserConnection::FLAG_SUPPORTS_TTHF)));
			tthd->setTTH(NULL);
				return;
			}
		}

		aConn->setState(UserConnection::STATE_FILELENGTH);
		/*QueueItem* q = QueueManager::getInstance()->fileQueue.find(d->getTarget());
		if(d->isSet(Download::FLAG_RESUME) && (q->getMaxSegmentsInitial() == 1)) {
			dcassert(d->getSize() != -1);

			const string& target = (d->getTempTarget().empty() ? d->getTarget() : d->getTempTarget());
			//int64_t start = File::getSize(target);
			int64_t start = d->getPos();

			// Only use antifrag if we don't have a previous non-antifrag part
			if( BOOLSETTING(ANTI_FRAG) && (start == -1) && (d->getSize() != -1) ) {
				int64_t aSize = File::getSize(target + Download::ANTI_FRAG_EXT);

				if(aSize == d->getSize())
					start = d->getPos();
				else
					start = 0;

				d->setFlag(Download::FLAG_ANTI_FRAG);
			}
		
			int rollback = SETTING(ROLLBACK);
			if(rollback > start) {
				d->setStartPos(0);
			} else {
				d->setStartPos(start - rollback);
				d->setFlag(Download::FLAG_ROLLBACK);
			}
		} else {
			d->setStartPos(0);
		}*/


		if(d->isSet(Download::FLAG_USER_LIST)) {
			if(aConn->isSet(UserConnection::FLAG_SUPPORTS_XML_BZLIST)) {
				d->setSource("files.xml.bz2");
			}
			d->setStartPos(0);
		}

	if(aConn->isSet(UserConnection::FLAG_SUPPORTS_ADCGET) && d->isSet(Download::FLAG_UTF8)) {
		aConn->send(d->getCommand(
			aConn->isSet(UserConnection::FLAG_SUPPORTS_ZLIB_GET),
			aConn->isSet(UserConnection::FLAG_SUPPORTS_TTHF)
			));
	} else {
		if(BOOLSETTING(COMPRESS_TRANSFERS) && aConn->isSet(UserConnection::FLAG_SUPPORTS_GETZBLOCK) && d->getSize() != -1  && !aConn->getUser()->isSet(User::FORCEZOFF)) {
			// This one, we'll download with a zblock download instead...
			d->setFlag(Download::FLAG_ZDOWNLOAD);
			aConn->getZBlock(d->getSource(), d->getPos(), d->getBytesLeft(), d->isSet(Download::FLAG_UTF8));
		} else if(d->isSet(Download::FLAG_UTF8)) {
			aConn->getBlock(d->getSource(), d->getPos(), d->getBytesLeft(), true);
		} else{
			aConn->get(d->getSource(), d->getPos());
		}
		aConn->getUser()->unsetFlag(User::FORCEZOFF);
	}
}

void DownloadManager::on(UserConnectionListener::Sending, UserConnection* aSource, int64_t aBytes) throw() {
	if(aSource->getState() != UserConnection::STATE_FILELENGTH) {
		dcdebug("DM::onFileLength Bad state, ignoring\n");
		return;
	}
	
	if(prepareFile(aSource, (aBytes == -1) ? -1 : aSource->getDownload()->getPos() + aBytes)) {
		aSource->setDataMode();
	}
}

void DownloadManager::on(UserConnectionListener::FileLength, UserConnection* aSource, int64_t aFileLength) throw() {
	if(aSource->getState() != UserConnection::STATE_FILELENGTH) {
		dcdebug("DM::onFileLength Bad state, ignoring\n");
		return;
	}

	Download::Ptr download = aSource->getDownload();
	User::Ptr user = aSource->getUser();
	if (download != NULL) {	
		if ( aSource->getDownload()->isSet(Download::FLAG_USER_LIST) ) {
			Client* client = NULL;
			if (user) {
				client = user->getClient();
				user->setFileListSize(aFileLength);
				if (client != NULL) {
					if((aFileLength < 100) && (user != (User::Ptr)NULL) && (user->getBytesShared() > 0)) {
						user->setCheat(Util::validateMessage("Too small filelist - " + Util::formatBytes(aFileLength) + " for the specified share of " + Util::formatBytes(user->getBytesShared()), false), false);
						user->setFakeSharing(true);
					}
				client->updated(user);
				}
			}
		}
	}

	if(prepareFile(aSource, aFileLength)) {
		aSource->setDataMode(/*aFileLength - aSource->getDownload()->getStartPos()*/);
		aSource->startSend();
	}
}

void DownloadManager::on(Command::SND, UserConnection* aSource, const Command& cmd) throw() {
	int64_t bytes = Util::toInt64(cmd.getParam(3));

	if(cmd.getParam(0) == "tthl") {
		if(aSource->getState() != UserConnection::STATE_TREE) {
			dcdebug("DM::SND Bad state, ignoring\n");
			return;
		}
		Download* d = aSource->getDownload();
		d->setFile(new TreeOutputStream(d->getOldDownload()->getTigerTree()));
		d->setSize(bytes);
		d->setPos(0);
		dcassert(d->isSet(Download::FLAG_TREE_DOWNLOAD));
		aSource->setState(UserConnection::STATE_DONE);

		if(cmd.hasFlag("ZL", 4)) {
			d->setFile(new FilteredOutputStream<UnZFilter, true>(d->getFile()));
		}

		aSource->setDataMode();
	} else if(cmd.getParam(0) == "file") {
		if(aSource->getState() != UserConnection::STATE_FILELENGTH) {
			dcdebug("DM::onFileLength Bad state, ignoring\n");
			return;
		}

	if(prepareFile(aSource, (bytes == -1) ? -1 : aSource->getDownload()->getPos() + bytes)) {
			aSource->setDataMode();
		}
	}
}

template<bool managed>
class RollbackOutputStream : public OutputStream {
public:
	RollbackOutputStream(File* f, OutputStream* aStream, size_t bytes) : s(aStream), pos(0), bufSize(bytes), buf(new u_int8_t[bytes]) {
		size_t n = bytes;
		f->read(buf, n);
		f->movePos(-((int64_t)bytes));
	}
	virtual ~RollbackOutputStream() { if(managed) delete s; };

	virtual size_t flush() throw(FileException) {
		return s->flush();
	}

	virtual size_t write(const void* b, size_t len) throw(FileException) {
		u_int8_t* wb = (u_int8_t*)b;
		if(buf != NULL) {
			size_t n = len < (bufSize - pos) ? len : bufSize - pos;

			if(memcmp(buf + pos, wb, n) != 0) {
				throw FileException(STRING(ROLLBACK_INCONSISTENCY));
			}
			pos += n;
			if(pos == bufSize) {
				delete buf;
				buf = NULL;
			}
		}
		return s->write(wb, len);
	}

private:
	OutputStream* s;
	size_t pos;
	size_t bufSize;
	u_int8_t* buf;
};

template<bool managed>
class TigerCheckOutputStream : public OutputStream {
public:
	TigerCheckOutputStream(const TigerTree& aTree, OutputStream* aStream) : s(aStream), real(aTree), cur(aTree.getBlockSize()), verified(0), bufPos(0) {
	}
	virtual ~TigerCheckOutputStream() { if(managed) delete s; };

	virtual size_t flush() throw(FileException) {
		if (bufPos != 0)
			cur.update(buf, bufPos);
		bufPos = 0;

		//cur.finalize();
		checkTrees();
		return s->flush();
	}

	virtual size_t write(const void* b, size_t len) throw(FileException) {
		u_int8_t* xb = (u_int8_t*)b;
		size_t pos = 0;

		if(bufPos != 0) {
			size_t bytes = min(TigerTree::BASE_BLOCK_SIZE - bufPos, len);
			memcpy(buf + bufPos, xb, bytes);
			pos = bytes;
			bufPos += bytes;

			if(bufPos == TigerTree::BASE_BLOCK_SIZE) {
				cur.update(buf, TigerTree::BASE_BLOCK_SIZE);
				bufPos = 0;
			}
		}

		if(pos < len) {
			dcassert(bufPos == 0);
			size_t left = len - pos;
			size_t part = left - (left %  TigerTree::BASE_BLOCK_SIZE);
			if(part > 0) {
				cur.update(xb + pos, part);
				pos += part;
			}
			left = len - pos;
			memcpy(buf, xb + pos, left);
			bufPos = left;
		}

		checkTrees();
		return s->write(b, len);
	}
	
	virtual int64_t verifiedBytes() {
		return min(real.getFileSize(), cur.getBlockSize() * (int64_t)cur.getLeaves().size());
	}
private:
	OutputStream* s;
	const TigerTree& real;
	TigerTree cur;
	size_t verified;

	u_int8_t buf[TigerTree::BASE_BLOCK_SIZE];
	size_t bufPos;

	void checkTrees() throw(FileException) {
		while(cur.getLeaves().size() > verified) {
			if(cur.getLeaves().size() > real.getLeaves().size() ||
				!(cur.getLeaves()[verified] == real.getLeaves()[verified])) 
			{
				throw FileException(STRING(TTH_INCONSISTENCY));
			}
			verified++;
		}
	}
};

bool DownloadManager::prepareFile(UserConnection* aSource, int64_t newSize /* = -1 */) {
	Download* d = aSource->getDownload();
	dcassert(d != NULL);
	
	QueueItem* q = QueueManager::getInstance()->getRunning(aSource->getUser());
	if(q != NULL) {
		if(q->getMaxSegments() <= q->getActiveSegments().size()) {
			aSource->setDownload(NULL);
			removeDownload(d, true);
			removeConnection(aSource);
			return false;
		}
	q->addActiveSegment(aSource->getUser());
	dcdebug(("Active segments : "+Util::toString((int)q->getActiveSegments().size())).c_str());
	}

	if(newSize != -1) {
		d->setSize(newSize);
	}
	if(d->getPos() >= d->getSize()) {
		// Already finished?
		aSource->setDownload(NULL);
		removeDownload(d, true, true);
		removeConnection(aSource);
		return false;
	}

	dcassert(d->getSize() != -1);

	string target = d->getDownloadTarget();
	Util::ensureDirectory(target);
	if(d->isSet(Download::FLAG_USER_LIST)) {
		if(aSource->isSet(UserConnection::FLAG_SUPPORTS_XML_BZLIST)) {
			target += ".xml.bz2";
		} else {
			target += ".DcLst";
		}
	}

	File* file = NULL;

	try {
		// Let's check if we can find this file in a any .SFV...
		int trunc = d->isSet(Download::FLAG_RESUME) ? 0 : File::TRUNCATE;
		if(!d->isSet(Download::FLAG_USER_LIST) && BOOLSETTING(MEMORY_MAPPED_FILE)){
			file = new MappedFile(target, d->getSize(), File::RW, File::OPEN | File::CREATE | trunc);			
		} else {
		file = new File(target, File::RW, File::OPEN | File::CREATE | trunc);
		}
/*		if(d->isSet(Download::FLAG_ANTI_FRAG)) {
						file->setSize(d->getSize());
		}*/
		file->setPos(d->getPos());		
	} catch(const FileException& e) {
		delete file;
		fire(DownloadManagerListener::Failed(), d, STRING(COULD_NOT_OPEN_TARGET_FILE) + e.getError());
		aSource->setDownload(NULL);
		removeDownload(d, true);
		removeConnection(aSource);
		return false;
	} catch(const Exception& e) {
		delete file;
		fire(DownloadManagerListener::Failed(), d, e.getError());
		aSource->setDownload(NULL);
		removeDownload(d, true);
		removeConnection(aSource);
		return false;
	}

	d->setFile(file);
	
	if(d->isSet(Download::FLAG_ROLLBACK)) {
		d->setFile(new RollbackOutputStream<true>(file, d->getFile(), SETTING(ROLLBACK)/*(size_t)min((int64_t)SETTING(ROLLBACK), d->getSize() - d->getPos())*/));
	}

	if(SETTING(BUFFER_SIZE) != 0) {
		d->setFile(new BufferedOutputStream<true>(d->getFile()));
	}

	bool sfvcheck = BOOLSETTING(SFV_CHECK) && (d->getPos() == 0) && (SFVReader(d->getTarget()).hasCRC());

	if(sfvcheck) {
		d->setFlag(Download::FLAG_CALC_CRC32);
		Download::CrcOS* crc = new Download::CrcOS(d->getFile());
		d->setCrcCalc(crc);
		d->setFile(crc);
	}
	
	if((BOOLSETTING(ENABLE403FEATURES)) && (d->getPos() == 0) && (!d->isSet(Download::FLAG_MP3_INFO))) {
		if(!d->getTreeValid() && d->getTTH() != NULL && d->getSize() < numeric_limits<size_t>::max()) {
			// We make a single node tree...
			d->getTigerTree().setFileSize(d->getSize());
			d->getTigerTree().setBlockSize((size_t)d->getSize());

			d->getTigerTree().getLeaves().push_back(*d->getTTH());
			d->getTigerTree().calcRoot();
			d->setTreeValid(true);
		}
		if(d->getTreeValid()) {
		d->setFile(new TigerCheckOutputStream<true>(d->getTigerTree(), d->getFile()));
	}
	}

	if(d->isSet(Download::FLAG_ZDOWNLOAD)) {
		d->setFile(new FilteredOutputStream<UnZFilter, true>(d->getFile()));
	}
	dcassert(d->getPos() != -1);
	d->setStart(GET_TICK());
	aSource->setState(UserConnection::STATE_DONE);

	fire(DownloadManagerListener::Starting(), d);
	
	return true;
}	

void DownloadManager::on(UserConnectionListener::Data, UserConnection* aSource, const u_int8_t* aData, size_t aLen) throw() {
	Download* d = aSource->getDownload();
	dcassert(d != NULL);
	
	try {
		try{
			if(!d->isSet(Download::FLAG_USER_LIST) && BOOLSETTING(LOG_SEGMENT)) {
				string log = Util::toString(d->getPos())+" => ";
					log += Util::toString(d->getPos()+aLen)+" - ";
					log += aSource->getUser()->getNick()+" @ ";
					log += Util::formatBytes(d->getRunningAverage())+"/s ";
					log += d->isSet(Download::FLAG_TREE_DOWNLOAD) ? "TreeDownload, " : "";
					log += d->isSet(Download::FLAG_TREE_TRIED) ? "TreeTried, " : "";
					log += d->isSet(Download::FLAG_ZDOWNLOAD) ? "GetZBlock, " : "";
					log += d->isSet(Download::FLAG_UTF8) ? "Utf8, " : "";
//					log += d->treeValid ? "TTHL, " : "";
//					log += getTTH() ? "TTH leaf: "+getTigerTree().getLeaves()[1] : "TTH: None";
				LogManager::getInstance()->log(d->getTargetFileName()+".segment",log);
			}

			d->addPos(d->getFile()->write(aData, aLen));			
		} catch(const BlockDLException) {
			d->getFile()->flush();
			fire(DownloadManagerListener::Failed(), d, CSTRING(BLOCK_FINISHED));			
			d->finished = true;
			aSource->setDownload(NULL);
			removeDownload(d, true);
			removeConnection(aSource, false, true);
			return;

		} catch(const FileDLException) {			
			handleEndData(aSource);
			return;	
		}


		d->addActual(aLen);
		if(d->getPos() == d->getSize()) {
			handleEndData(aSource);
			aSource->setLineMode();
		}


	} catch(const FileException& e) {
		fire(DownloadManagerListener::Failed(), d, e.getError());
		//d->resetPos();
		aSource->setDownload(NULL);
		removeDownload(d, true);
		removeConnection(aSource);
		return;
	} catch(const Exception& e) {
		fire(DownloadManagerListener::Failed(), d, e.getError());
		// Nuke the bytes we have written, this is probably a compression error
		//d->resetPos();
		aSource->setDownload(NULL);
		removeDownload(d, true);
		removeConnection(aSource);
		return;
	}
}

/** Download finished! */
void DownloadManager::handleEndData(UserConnection* aSource) {

	dcassert(aSource->getState() == UserConnection::STATE_DONE);
	Download* d = aSource->getDownload();
	dcassert(d != NULL);
	
	d->finished = true;

	if(d->isSet(Download::FLAG_TREE_DOWNLOAD)) {
		d->getFile()->flush();
		delete d->getFile();
		d->setFile(NULL);

		Download* old = d->getOldDownload();

		size_t bl = 1024;
		while(bl * old->getTigerTree().getLeaves().size() < old->getSize())
			bl *= 2;
		old->getTigerTree().setBlockSize(bl);
		dcassert(old->getSize() != -1);
		old->getTigerTree().setFileSize(old->getSize());

		old->getTigerTree().calcRoot();

		if(!(*old->getTTH() == old->getTigerTree().getRoot())) {
			// This tree is for a different file, remove from queue...
			fire(DownloadManagerListener::Failed(), old, STRING(INVALID_TREE));

			string target = old->getTarget();

			aSource->setDownload(NULL);
			removeDownload(old, true);

			QueueManager::getInstance()->removeSource(target, aSource->getUser(), QueueItem::Source::FLAG_BAD_TREE, false);
			checkDownloads(aSource, true);
			return;
		}

		d->getOldDownload()->setTreeValid(true);

		HashManager::getInstance()->addTree(old->getTarget(), old->getTigerTree());

		aSource->setDownload(d->getOldDownload());

		delete d;

		// Ok, now we can continue to the actual file...
		checkDownloads(aSource, false);
		return;
	}

	u_int32_t crc = 0;
	bool hasCrc = (d->getCrcCalc() != NULL);

	// First, finish writing the file (flushing the buffers and closing the file...)
	try {
		d->getFile()->flush();
		if(hasCrc)
			crc = d->getCrcCalc()->getFilter().getValue();
		delete d->getFile();
		d->setFile(NULL);
		d->setCrcCalc(NULL);

		// Check if we're anti-fragging...
		if(d->isSet(Download::FLAG_ANTI_FRAG)) {
			// Ok, rename the file to what we expect it to be...
			try {
				const string& tgt = d->getTempTarget().empty() ? d->getTarget() : d->getTempTarget();
				File::renameFile(d->getDownloadTarget(), tgt);
				d->unsetFlag(Download::FLAG_ANTI_FRAG);
			} catch(const FileException& e) {
				dcdebug("AntiFrag: %s\n", e.getError().c_str());
				// Now what?
			}
		}
	} catch(const FileException& e) {
		fire(DownloadManagerListener::Failed(), d, e.getError());
		
		aSource->setDownload(NULL);
		removeDownload(d, true);
		removeConnection(aSource, false);
		return;
	}

	//dcassert(d->getPos() == d->getSize());
	dcdebug("Download finished: %s, size " I64_FMT ", downloaded " I64_FMT "\n", d->getTarget().c_str(), d->getSize(), d->getTotal());

	QueueItem* q = QueueManager::getInstance()->fileQueue.find(d->getTarget());

	// Check if we have some crc:s...
	if(BOOLSETTING(SFV_CHECK)) {
		SFVReader sfv(d->getTarget());
		if(sfv.hasCRC()) {
			bool crcMatch;
			string tgt = d->getDownloadTarget();
			if(hasCrc) {
				crcMatch = (crc == sfv.getCRC());
			} else {
				// More complicated, we have to reread the file
				try {
					
					File ff(tgt, File::READ, File::OPEN);
					CalcInputStream<CRC32Filter, false> f(&ff);

					const size_t BUF_SIZE = 16 * 65536;
					AutoArray<u_int8_t> b(BUF_SIZE);
					size_t n = BUF_SIZE;
					while(f.read((u_int8_t*)b, n) > 0)
						;		// Keep on looping...

					crcMatch = (f.getFilter().getValue() == sfv.getCRC());
				} catch (FileException&) {
					// Nope; read failed...
					goto noCRC;
				}
			}

			if(!crcMatch) {
				//File::deleteFile(tgt);
				dcdebug("DownloadManager: CRC32 mismatch for %s\n", d->getTarget().c_str());
				LogManager::getInstance()->message(STRING(SFV_INCONSISTENCY) + " (" + STRING(FILE) + ": " + d->getTarget() + ")", true);
				fire(DownloadManagerListener::Failed(), d, STRING(SFV_INCONSISTENCY));
				
				string target = d->getTarget();
				Download* old = d->getOldDownload();

				aSource->setDownload(NULL);
				delete FileDataInfo::GetFileDataInfo(d->getTempTarget());

				vector<int64_t> v;
				v.push_back(0);
				v.push_back(d->getSize());
				new FileDataInfo(d->getTempTarget(), d->getSize(), &v);

				removeDownload(d, true);				
				QueueManager::getInstance()->removeSource(target, aSource->getUser(), QueueItem::Source::FLAG_CRC_WARN, false);

				aSource->setDownload(old);
				checkDownloads(aSource, true);
				return;
			} 

			d->setFlag(Download::FLAG_CRC32_OK);
			
			dcdebug("DownloadManager: CRC32 match for %s\n", d->getTarget().c_str());
		}
	}
noCRC:
	if(BOOLSETTING(LOG_DOWNLOADS) && (BOOLSETTING(LOG_FILELIST_TRANSFERS) || !d->isSet(Download::FLAG_USER_LIST))) {
		StringMap params;
		params["target"] = d->getTarget();
		params["user"] = aSource->getUser()->getNick();
		params["hub"] = aSource->getUser()->getLastHubName();
		params["hubip"] = aSource->getUser()->getLastHubAddress();
		params["size"] = Util::toString(d->getSize());
		params["sizeshort"] = Util::formatBytes(d->getSize());
		params["chunksize"] = Util::toString(d->getTotal());
		params["chunksizeshort"] = Util::formatBytes(d->getTotal());
		params["actualsize"] = Util::toString(d->getActual());
		params["actualsizeshort"] = Util::formatBytes(d->getActual());
		params["speed"] = Util::formatBytes(d->getAverageSpeed()) + "/s";
		params["time"] = Util::formatSeconds((GET_TICK() - d->getStart()) / 1000);
		params["sfv"] = Util::toString(d->isSet(Download::FLAG_CRC32_OK) ? 1 : 0);
		LOG(DOWNLOAD_AREA, Util::formatParams(SETTING(LOG_FORMAT_POST_DOWNLOAD), params));
	}
	
	// Check hash
	if((BOOLSETTING(CHECK_TTH)) && (!d->isSet(Download::FLAG_USER_LIST)) && (!d->isSet(Download::FLAG_MP3_INFO))) {
		fire(DownloadManagerListener::Failed(), d, STRING(CHECKING_TTH));
		TTHValue* hash1 = new TTHValue(HashManager::getInstance()->hasher.getTTfromFile(d->getTempTarget()).getRoot());
		TTHValue* hash2 = q->getTTH();

		bool hashMatch = true;

		if((hash1 != NULL) && (hash2 != NULL)) { 
			if (*hash1 == *hash2) hashMatch = true; else hashMatch = false;

			if(!hashMatch) {		
				if ((!SETTING(SOUND_TTH).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
					PlaySound(SETTING(SOUND_TTH).c_str(), NULL, SND_FILENAME | SND_ASYNC);
	
				fire(DownloadManagerListener::Failed(), d, STRING(DOWNLOAD_CORRUPTED));
	
				string target = d->getTarget();
				Download* old = d->getOldDownload();			

				aSource->setDownload(NULL);

				for(int i = 10; i>0; --i) {
					char buf[64];
					sprintf(buf, CSTRING(DOWNLOAD_CORRUPTED), i);
					fire(DownloadManagerListener::Failed(), d, buf);
					Thread::sleep(1000);
				}

				delete FileDataInfo::GetFileDataInfo(d->getTempTarget());

				fire(DownloadManagerListener::Failed(), d, STRING(CONNECTING));
				vector<int64_t> v;
				v.push_back(0);
				v.push_back(d->getSize());
				new FileDataInfo(d->getTempTarget(), d->getSize(), &v);

				removeDownload(d, true);
				QueueManager::getInstance()->removeSource(target, aSource->getUser(), QueueItem::Source::FLAG_TTH_INCONSISTENCY, false);

				aSource->setDownload(old);
				checkDownloads(aSource, true);
				return;
			}
		d->setFlag(Download::FLAG_TTH_OK);
		}
	delete hash1;
	}

	// Check if we need to move the file
	if( !d->getTempTarget().empty() && (Util::stricmp(d->getTarget().c_str(), d->getTempTarget().c_str()) != 0) ) {
		try {
			Util::ensureDirectory(d->getTarget());
			if(File::getSize(d->getTempTarget()) > MOVER_LIMIT) {
				mover.moveFile(d->getTempTarget(), d->getTarget());
			} else {
			File::renameFile(d->getTempTarget(), d->getTarget());
			}
			d->setTempTarget(Util::emptyString);
		} catch(const FileException&) {
			// Huh??? Now what??? Oh well...let it be...
		}
	}
	fire(DownloadManagerListener::Complete(), d);

	aSource->setDownload(NULL);
	removeDownload(d, true, true);	
	checkDownloads(aSource, true);

}

void DownloadManager::on(UserConnectionListener::MaxedOut, UserConnection* aSource) throw() { 
	if(aSource->getState() != UserConnection::STATE_FILELENGTH && aSource->getState() != UserConnection::STATE_TREE) {
		dcdebug("DM::onMaxedOut Bad state, ignoring\n");
		return;
	}

	Download* d = aSource->getDownload();
	dcassert(d != NULL);

	fire(DownloadManagerListener::Failed(), d, STRING(NO_SLOTS_AVAILABLE));

	User::Ptr user = aSource->getUser();

			if( d->isSet(Download::FLAG_TESTSUR) && user->getConnection().size() > 0) {
				dcdebug("No slots for TestSUR %s\n", user->getNick());
				user->setTestSUR("MaxedOut");
				user->setHasTestSURinQueue(false);
				user->updateClientType();
				aSource->setDownload(NULL);
				removeDownload(d, true, true);
				removeConnection(aSource);
				user->setCheatingString(Util::validateMessage("No slots for TestSUR. User is using slotlocker.", false));
				user->setFakeSharing(true);
				User::updated(user);
				user->getClient()->fire(ClientListener::CheatMessage(), user->getClient(), user->getNick()+": "+user->getCheatingString());
				return;
			}

	aSource->setDownload(NULL);
	removeDownload(d, true);
	removeConnection(aSource);
}

void DownloadManager::on(UserConnectionListener::Failed, UserConnection* aSource, const string& aError) throw() {
	Download* d = aSource->getDownload();

	if(d == NULL) {
		removeConnection(aSource);
		return;
	}
	
	fire(DownloadManagerListener::Failed(), d, aError);

	if ( d->isSet(Download::FLAG_USER_LIST) ) {
		User::Ptr user = aSource->getUser();
		if (user) {
			if (aError.find("File Not Available") != string::npos || aError.find("File non disponibile") != string::npos ) {
				//user->addLine("*** User " + user->getNick() + " filelist not available");
				user->setCheat("filelist not available", false);
				user->updated();
				//user->sendRawCommand(SETTING(FILELIST_UNAVAILABLE));
				aSource->setDownload(NULL);
				removeDownload(d, true, true);
				removeConnection(aSource);
				return;
			} else if(aError == STRING(DISCONNECTED)) {
				//user->addLine("*** User " + user->getNick() + " filelist disconnected");
				if(user->fileListDisconnected()) {
					aSource->setDownload(NULL);
					removeDownload(d, true, true);
					removeConnection(aSource);
					return;
				}
			}
		} 
	}
	else if( d->isSet(Download::FLAG_TESTSUR) ) {
		dcdebug("TestSUR Error: %s\n", aError);
		User::Ptr user = aSource->getUser();
		user->setTestSUR(aError);
		user->setHasTestSURinQueue(false);
		user->updateClientType();
		aSource->setDownload(NULL);
		removeDownload(d, true, true);
		removeConnection(aSource);
		return;
	}

	string target = d->getTarget();
	aSource->setDownload(NULL);
	removeDownload(d, true);
	
	if(aError.find("File Not Available") != string::npos || aError.find(" no more exists") != string::npos) { //solved DCTC
		QueueManager::getInstance()->removeSource(target, aSource->getUser(), QueueItem::Source::FLAG_FILE_NOT_AVAILABLE, false);
	}

	removeConnection(aSource);
}

void DownloadManager::removeDownload(Download* d, bool full, bool finished /* = false */) {
	bool checkList = d->isSet(Download::FLAG_CHECK_FILE_LIST) && d->isSet(Download::FLAG_TESTSUR);
	User::Ptr uzivatel = d->getUserConnection()->getUser();

	if(d->getOldDownload() != NULL) {
		if(d->getFile()) {
			try {
				d->getFile()->flush();
			} catch(const Exception&) {
				finished = false;
			}
			delete d->getFile();
			d->setFile(NULL);
			d->setCrcCalc(NULL);

			if(d->isSet(Download::FLAG_ANTI_FRAG)) {
				// Ok, set the pos to whereever it was last writing and hope for the best...
				d->unsetFlag(Download::FLAG_ANTI_FRAG);
			} 
		}
		Download* old = d;
		d = d->getOldDownload();
		if(!full) {
		old->getUserConnection()->setDownload(d);
		}

		old->setUserConnection(NULL);
		delete old;

		if(!full) {
			return;
		}
	}

	if(d->getFile()) {
		try {
			d->getFile()->flush();
		} catch(const Exception&) {
			finished = false;
		}
		delete d->getFile();
		d->setFile(NULL);
		d->setCrcCalc(NULL);

		if(d->isSet(Download::FLAG_ANTI_FRAG)) {
			// Ok, set the pos to whereever it was last writing and hope for the best...
			d->unsetFlag(Download::FLAG_ANTI_FRAG);
		} 
	}

	{
		Lock l(cs);
		// Either I'm stupid or the msvc7 optimizer is doing something _very_ strange here...
		// STL-port -D_STL_DEBUG complains that .begin() and .end() don't have the same owner (!),
		// but only in release build

		dcassert(find(downloads.begin(), downloads.end(), d) != downloads.end());

		//		downloads.erase(find(downloads.begin(), downloads.end(), d));
		
		for(Download::Iter i = downloads.begin(); i != downloads.end(); ++i) {
			if(*i == d) {
				downloads.erase(i);
				break;
			}
		}
	}
	QueueManager::getInstance()->putDownload(d, finished);
	if(checkList) {
		try {
			QueueManager::getInstance()->addList(uzivatel, QueueItem::FLAG_CHECK_FILE_LIST);
		} catch(const Exception&) {}
		uzivatel->connect();
	}
}

void DownloadManager::abortDownload(const string& aTarget) {
	Lock l(cs);
	for(Download::Iter i = downloads.begin(); i != downloads.end(); ++i) {
		Download* d = *i;
		if(d->getTarget() == aTarget) {
			dcassert(d->getUserConnection() != NULL);
			d->getUserConnection()->disconnect();
		}
	}
}

void DownloadManager::abortDownload(const string& aTarget, User::Ptr& aUser) {
	Lock l(cs);
	for(Download::Iter i = downloads.begin(); i != downloads.end(); ++i) {
		Download* d = *i;
		if(d->getTarget() == aTarget) {
			dcassert(d->getUserConnection() != NULL);
			if(d->getUserConnection()->getUser() == aUser){
				d->getUserConnection()->disconnect();	
						break;
			}	
		}
	}
}

void DownloadManager::on(UserConnectionListener::FileNotAvailable, UserConnection* aSource) throw() {
	Download* d = aSource->getDownload();
	dcassert(d != NULL);

	if(d->isSet(Download::FLAG_TREE_DOWNLOAD)) {
		// No tree, too bad...
		aSource->setDownload(d->getOldDownload());
		delete d->getFile();
		d->setFile(NULL);
		delete d;
		checkDownloads(aSource, false);
		return;
	}

	dcdebug("File Not Available: %s\n", d->getTarget().c_str());

	if(d->getFile()) {
		delete d->getFile();
		d->setFile(NULL);
		d->setCrcCalc(NULL);
	}

	fire(DownloadManagerListener::Failed(), d, d->getTargetFileName() + ": " + STRING(FILE_NOT_AVAILABLE));
	if( d->isSet(Download::FLAG_TESTSUR) ) {
		dcdebug("TestSUR File not available\n");
		User::Ptr user = aSource->getUser();
		user->setTestSUR("File Not Available");
		user->setHasTestSURinQueue(false);
		user->updateClientType();
		aSource->setDownload(NULL);
		removeDownload(d, false, true);
		removeConnection(aSource);
		return;
	}

	aSource->setDownload(NULL);

	QueueManager::getInstance()->removeSource(d->getTarget(), aSource->getUser(), QueueItem::Source::FLAG_FILE_NOT_AVAILABLE, false);
	removeDownload(d, false, false);
	checkDownloads(aSource, false);
}

void DownloadManager::throttleReturnBytes(u_int32_t b) {
	Lock l(cs);
	if (b > 0 && b < 2*mByteSlice) {
		mBytesSpokenFor -= b;
		if (mBytesSpokenFor < 0)
			mBytesSpokenFor = 0;
	}
}

size_t DownloadManager::throttleGetSlice() {
	if (mThrottleEnable) {
		Lock l(cs);
		size_t left = mDownloadLimit - mBytesSpokenFor;
		if (left > 0) {
			if (left > 2*mByteSlice) {
				mBytesSpokenFor += mByteSlice;
				return mByteSlice;
			} else {
				mBytesSpokenFor += left;
				return left;
			}
		} else
			return 0;
	} else {
		return (size_t)-1;
	}
}

size_t DownloadManager::throttleCycleTime() {
	if (mThrottleEnable)
		return mCycleTime;
	return 0;
}

void DownloadManager::throttleZeroCounters() {
	Lock l(cs);
	mBytesSpokenFor = 0;
	mBytesSent = 0;
}

void DownloadManager::throttleBytesTransferred(u_int32_t i) {
	Lock l(cs);
	mBytesSent += i;
}

void DownloadManager::throttleSetup() {
// called once a second, plus when a download starts
// from the constructor to BufferedSocket
// with 64k, a few people get winsock error 0x2747
#define INBUFSIZE 64*1024
	Lock l(cs);
	unsigned int num_transfers = getDownloads();
	mDownloadLimit = (SETTING(MAX_DOWNLOAD_SPEED_LIMIT) * 1024);
	mThrottleEnable = BOOLSETTING(THROTTLE_ENABLE) && (mDownloadLimit > 0) && (num_transfers > 0);
	if (mThrottleEnable) {
			if (mDownloadLimit <= (INBUFSIZE * 10 * num_transfers)) {
				mByteSlice = mDownloadLimit / (7 * num_transfers);
				if (mByteSlice > INBUFSIZE)
					mByteSlice = INBUFSIZE;
				mCycleTime = 1000 / 10;
				} else {
				mByteSlice = INBUFSIZE;
				mCycleTime = 1000 * INBUFSIZE / mDownloadLimit;
			}
		}
}

/**
 * @file
 * $Id$
 */
