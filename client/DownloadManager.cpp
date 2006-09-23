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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "DownloadManager.h"

#include "ResourceManager.h"
#include "ConnectionManager.h"
#include "QueueManager.h"
#include "CryptoManager.h"
#include "HashManager.h"

#include "LogManager.h"
#include "User.h"
#include "File.h"
#include "FilteredFile.h"
#include "MerkleCheckOutputStream.h"
#include "ClientManager.h"
#include "SharedFileStream.h"
#include "ChunkOutputStream.h"
#include "UploadManager.h"

#include <limits>

// some strange mac definition
#ifdef ff
#undef ff
#endif

static const string DOWNLOAD_AREA = "Downloads";
const string Download::ANTI_FRAG_EXT = ".antifrag";

const string DownloadManager::USER_LIST_NAME = "files.xml";
const string DownloadManager::USER_LIST_NAME_BZ = "files.xml.bz2";

Download::Download() throw() : file(NULL), treeValid(false) { 
}

Download::Download(QueueItem* qi, User::Ptr& aUser, QueueItem::Source* aSource) throw() :
	target(qi->getTarget()), tempTarget(qi->getTempTarget()), file(NULL), tth(qi->getTTH()), treeValid(false),
	quickTick(GET_TICK()) { 
	
	setSize(qi->getSize());
	setFileSize(qi->getSize());

	if(qi->isSet(QueueItem::FLAG_USER_LIST))
		setFlag(Download::FLAG_USER_LIST);
	if(qi->isSet(QueueItem::FLAG_CHECK_FILE_LIST))
		setFlag(Download::FLAG_CHECK_FILE_LIST);
	if(qi->isSet(QueueItem::FLAG_TESTSUR))
		setFlag(Download::FLAG_TESTSUR);
	if(qi->isSet(QueueItem::FLAG_RESUME))
		setFlag(Download::FLAG_RESUME);
	if(qi->isSet(QueueItem::FLAG_MULTI_SOURCE))
		setFlag(Download::FLAG_MULTI_CHUNK);

	if(aSource->isSet(QueueItem::Source::FLAG_PARTIAL))
		setFlag(Download::FLAG_PARTIAL);

	user = aUser;
}

AdcCommand Download::getCommand(bool zlib) {
	AdcCommand cmd(AdcCommand::CMD_GET);
	if(isSet(FLAG_TREE_DOWNLOAD)) {
		cmd.addParam("tthl");
	} else if(isSet(FLAG_PARTIAL_LIST)) {
		cmd.addParam("list");
	} else {
		cmd.addParam("file");
	}
	if(isSet(FLAG_PARTIAL_LIST) || isSet(FLAG_USER_LIST)) {
		cmd.addParam(Util::toAdcFile(getSource()));
	} else {
		cmd.addParam("TTH/" + getTTH().toBase32());
	}
	cmd.addParam(Util::toString(getPos()));
	cmd.addParam(Util::toString(getSize() - getPos()));

	if(zlib && BOOLSETTING(COMPRESS_TRANSFERS)) {
		cmd.addParam("ZL1");
	}

	return cmd;
}

DownloadManager::DownloadManager() {
	TimerManager::getInstance()->addListener(this);
	mDownloadLimit = 0; mBytesSent = 0; mBytesSpokenFor = 0; mCycleTime = 0; mByteSlice = 0;
	mThrottleEnable = BOOLSETTING(THROTTLE_ENABLE);
	throttleZeroCounters();	
}

DownloadManager::~DownloadManager() throw() {
	TimerManager::getInstance()->removeListener(this);
	while(true) {
		{
			Lock l(cs);
			if(downloads.empty())
				break;
		}
		Thread::sleep(100);
	}
}

void DownloadManager::on(TimerManagerListener::Second, u_int32_t aTick) throw() {
	Lock l(cs);

	Download::List tickList;
	throttleSetup();
	throttleZeroCounters();

	// Tick each ongoing download
	for(Download::Iter i = downloads.begin(); i != downloads.end(); ++i) {
		Download* d = *i;

		if (d->getUserConnection() && !d->isSet(Download::FLAG_USER_LIST)) {
			// TODO: merge codes for automatic and manual disconnecting
			if(	(d->getStart() &&  (0 == ((int)(aTick - d->getStart()) / 1000 + 1) % 40)) &&
				(d->getRunningAverage() < (downloads.size() * 100)) &&
				(QueueManager::getInstance()->dropSource(d, true))) {
					d->getUserConnection()->disconnect();
					continue;
			} else if (d->getSize() > (SETTING(MIN_FILE_SIZE) * 1048576)) {
				if((d->getRunningAverage() < SETTING(I_DOWN_SPEED)*1024)) {
					if(	(((aTick - d->quickTick)/1000) > SETTING(DOWN_TIME)) &&
						(!QueueManager::getInstance()->dropSource(d, false))) {
							continue;
					}
				} else {
					d->quickTick = aTick;
				}
			}
		}

		if((*i)->getTotal() > 0) {
			tickList.push_back(*i);
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

void DownloadManager::removeConnection(UserConnection::Ptr aConn) {
	dcassert(aConn->getDownload() == NULL);
	aConn->removeListener(this);
	aConn->disconnect();

 	Lock l(cs);
 	idlers.erase(remove(idlers.begin(), idlers.end(), aConn), idlers.end());
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

void DownloadManager::checkIdle(const User::Ptr& user) {	
	Lock l(cs);	
	for(UserConnection::List::iterator i = idlers.begin(); i != idlers.end(); ++i) {	
		UserConnection* uc = *i;	
		if(uc->getUser() == user) {	
			idlers.erase(i);	
			checkDownloads(uc);	
			return;	
		}	
	}	
}

void DownloadManager::addConnection(UserConnection::Ptr conn) {
	if(!conn->isSet(UserConnection::FLAG_SUPPORTS_TTHF) || !conn->isSet(UserConnection::FLAG_SUPPORTS_ADCGET)) {
		// Can't download from these...
		conn->getUser()->setFlag(User::OLD_CLIENT);
		QueueManager::getInstance()->removeSource(conn->getUser(), QueueItem::Source::FLAG_NO_TTHF);
		removeConnection(conn);
	}

	conn->addListener(this);
	checkDownloads(conn);
}

void DownloadManager::checkDownloads(UserConnection* aConn, bool reconn /*=false*/) {
	dcassert(aConn->getDownload() == NULL);

	bool slotsFull = (SETTING(DOWNLOAD_SLOTS) != 0) && (getDownloadCount() >= (size_t)SETTING(DOWNLOAD_SLOTS));
	bool speedFull = (SETTING(MAX_DOWNLOAD_SPEED) != 0) && (getAverageSpeed() >= (SETTING(MAX_DOWNLOAD_SPEED)*1024));

	if(slotsFull || speedFull) {
		bool extraFull = (SETTING(DOWNLOAD_SLOTS) != 0) && (getDownloadCount() >= (size_t)(SETTING(DOWNLOAD_SLOTS)+SETTING(EXTRA_DOWNLOAD_SLOTS)));
		if(extraFull || !QueueManager::getInstance()->hasDownload(aConn->getUser(), QueueItem::HIGHEST)) {
			removeConnection(aConn);
			return;
		}
	}

	// this happen when download finished, we need reconnect.	
	if(reconn){
		removeConnection(aConn);
		return;
	}

	string message = STRING(WAITING_TO_RETRY);
	Download* d = QueueManager::getInstance()->getDownload(aConn->getUser(), aConn->isSet(UserConnection::FLAG_SUPPORTS_TTHL), 
		!aConn->isSet(UserConnection::FLAG_STEALTH) && (aConn->isSet(UserConnection::FLAG_SUPPORTS_ADCGET) || aConn->isSet(UserConnection::FLAG_SUPPORTS_GETZBLOCK) || aConn->isSet(UserConnection::FLAG_SUPPORTS_XML_BZLIST)),
		message);

	if(!d) {
		fire(DownloadManagerListener::Status(), aConn->getUser(), message);
		aConn->setLastActivity(0);

		Lock l(cs);
 	    idlers.push_back(aConn);
		return;
	}

	d->setUserConnection(aConn);
	aConn->setDownload(d);

	if(d->isSet(Download::FLAG_TESTSUR)) {
		aConn->getListLen();
	}

	aConn->setState(UserConnection::STATE_FILELENGTH);

	if(!d->isSet(Download::FLAG_MULTI_CHUNK)) {
		if(d->isSet(Download::FLAG_RESUME)) {
			dcassert(d->getSize() != -1);

			const string& target = (d->getTempTarget().empty() ? d->getTarget() : d->getTempTarget());
			int64_t start = File::getSize(target);

			// Only use antifrag if we don't have a previous non-antifrag part
			if( BOOLSETTING(ANTI_FRAG) && (start == -1) && (d->getSize() != -1) ) {
				int64_t aSize = File::getSize(target + Download::ANTI_FRAG_EXT);

				if(aSize == d->getSize())
					start = d->getPos();
				else
					start = 0;

				d->setFlag(Download::FLAG_ANTI_FRAG);
			}

			if(BOOLSETTING(ADVANCED_RESUME) && d->getTreeValid() && start > 0 &&
			  (d->getTigerTree().getLeaves().size() > 1 || aConn->isSet(UserConnection::FLAG_SUPPORTS_TTHL)))
			{
				d->setStartPos(getResumePos(d->getDownloadTarget(), d->getTigerTree(), start));
			} else {
				int rollback = SETTING(ROLLBACK);
				if(rollback > start) {
					d->setStartPos(0);
				} else {
					d->setStartPos(start - rollback);
					d->setFlag(Download::FLAG_ROLLBACK);
				}
			}
	
		} else {
			d->setStartPos(0);
		}
	}

	if(d->isSet(Download::FLAG_USER_LIST)) {
		if(aConn->isSet(UserConnection::FLAG_SUPPORTS_XML_BZLIST)) {
			d->setSource(USER_LIST_NAME_BZ);
		} else {
			d->setSource(USER_LIST_NAME);
		}
		d->setStartPos(0);
	}

	{
		Lock l(cs);
		downloads.push_back(d);
	}
	
	aConn->send(d->getCommand(aConn->isSet(UserConnection::FLAG_SUPPORTS_ZLIB_GET)));
}

class DummyOutputStream : public OutputStream {
public:
	virtual size_t write(const void*, size_t n) throw(Exception) { return n; }
	virtual size_t flush() throw(Exception) { return 0; }
};

int64_t DownloadManager::getResumePos(const string& file, const TigerTree& tt, int64_t startPos) {
	// Always discard data until the last block
	if(startPos < tt.getBlockSize())
		return 0;

	startPos -= (startPos % tt.getBlockSize());

	DummyOutputStream dummy;

	vector<u_int8_t> buf((size_t)min((int64_t)1024*1024, tt.getBlockSize()));

	do {
		int64_t blockPos = startPos - tt.getBlockSize();

		try {
			MerkleCheckOutputStream<TigerTree, false> check(tt, &dummy, blockPos);

			File inFile(file, File::READ, File::OPEN);
			inFile.setPos(blockPos);
			int64_t bytesLeft = tt.getBlockSize();
			while(bytesLeft > 0) {
				size_t n = (size_t)min((int64_t)buf.size(), bytesLeft);
				size_t nr = inFile.read(&buf[0], n);
				check.write(&buf[0], nr);
				bytesLeft -= nr;
				if(bytesLeft > 0 && nr == 0) {
					// Huh??
					throw Exception();
				}
			}
			check.flush();
			break;
		} catch(const Exception&) {
			dcdebug("Removed bad block at " I64_FMT "\n", blockPos);
		}
		startPos = blockPos;
	} while(startPos > 0);
	return startPos;
}

void DownloadManager::on(UserConnectionListener::Sending, UserConnection* aSource, int64_t aBytes) throw() {
	if(aSource->getState() != UserConnection::STATE_FILELENGTH) {
		dcdebug("DM::onFileLength Bad state, ignoring\n");
		return;
	}

	if(!aSource->getDownload()) {
		aSource->disconnect(true);
		return;
	}
	
	if(prepareFile(aSource, (aBytes == -1) ? -1 : aSource->getDownload()->getPos() + aBytes, aSource->getDownload()->isSet(Download::FLAG_ZDOWNLOAD))) {
		aSource->setDataMode();
	}
}

void DownloadManager::on(UserConnectionListener::FileLength, UserConnection* aSource, int64_t aFileLength) throw() {

	if(aSource->getState() != UserConnection::STATE_FILELENGTH) {
		dcdebug("DM::onFileLength Bad state, ignoring\n");
		return;
	}

	if(!aSource->getDownload()) {
		aSource->disconnect(true);
		return;
	}

	/** TODO set filelist size
	Download::Ptr download = aSource->getDownload();
	
	if (download && aSource->getDownload()->isSet(Download::FLAG_USER_LIST)) {	
		OnlineUser& ou = ClientManager::getInstance()->getOnlineUser(aSource->getUser());
		if(&ou) {
			ou.getIdentity().set("LS", Util::toString(aFileLength));
			if((aFileLength < 100) && (ou.getIdentity().getBytesShared() > 0)) {
				ou.getIdentity().setCheat(ou.getClient(), "Too small filelist - " + Util::formatBytes(aFileLength) + " for the specified share of " + Util::formatBytes(ou.getIdentity().getBytesShared()), false);
				ou.getIdentity().set("BF", "1");
				ou.getIdentity().sendRawCommand(ou.getClient(), SETTING(FILELIST_TOO_SMALL));
			} else {
				int64_t listLength = Util::toInt64(ou.getIdentity().get("LL"));
				if ( aSource->getUser()->isSet(User::DCPLUSPLUS) && (listLength != -1) && (listLength * 3 < aFileLength) && (ou.getIdentity().getBytesShared() > 0) ) {
					ou.getIdentity().setCheat(ou.getClient(), "Fake file list - ListLen = " + Util::toString(listLength) + " FileLength = " + Util::toString(aFileLength), false);
					ou.getIdentity().sendRawCommand(ou.getClient(), SETTING(LISTLEN_MISMATCH));
				}
			}
		}
	}*/

	if(prepareFile(aSource, aFileLength, aSource->getDownload()->isSet(Download::FLAG_ZDOWNLOAD))) {
		aSource->setDataMode();
		aSource->startSend();
	}
}

void DownloadManager::on(AdcCommand::SND, UserConnection* aSource, const AdcCommand& cmd) throw() {
	if(aSource->getState() != UserConnection::STATE_FILELENGTH) {
		dcdebug("DM::onFileLength Bad state, ignoring\n");
		return;
	}
	if(!aSource->getDownload()) {
		aSource->disconnect(true);
		return;
	}
	const string& type = cmd.getParam(0);
	int64_t bytes = Util::toInt64(cmd.getParam(3));

	if(	!(type == "file" || (type == "tthl" && aSource->getDownload()->isSet(Download::FLAG_TREE_DOWNLOAD)) ||
		(type == "list" && aSource->getDownload()->isSet(Download::FLAG_PARTIAL_LIST))) )
	{
		// Uhh??? We didn't ask for this?
		aSource->disconnect();
		return;
	}

	if(prepareFile(aSource, (bytes == -1) ? -1 : aSource->getDownload()->getPos() + bytes, cmd.hasFlag("ZL", 4))) {
		aSource->setDataMode();
	}
}

class RollbackException : public FileException {
public:
	RollbackException (const string& aError) : FileException(aError) { }
};

template<bool managed>
class RollbackOutputStream : public OutputStream {
public:
	RollbackOutputStream(File* f, OutputStream* aStream, size_t bytes) : s(aStream), pos(0), bufSize(bytes), buf(new u_int8_t[bytes]) {
		size_t n = bytes;
		f->read(buf, n);
		f->movePos(-((int64_t)bytes));
	}
	virtual ~RollbackOutputStream() throw() { delete[] buf; if(managed) delete s; }

	virtual size_t flush() throw(FileException) {
		return s->flush();
	}

	virtual size_t write(const void* b, size_t len) throw(FileException) {
		if(buf != NULL) {
			size_t n = min(len, bufSize - pos);

			u_int8_t* wb = (u_int8_t*)b;
			if(memcmp(buf + pos, wb, n) != 0) {
				throw RollbackException(STRING(ROLLBACK_INCONSISTENCY));
			}
			pos += n;
			if(pos == bufSize) {
				delete buf;
				buf = NULL;
			}
		}
		return s->write(b, len);
	}

private:
	OutputStream* s;
	size_t pos;
	size_t bufSize;
	u_int8_t* buf;
};

bool DownloadManager::prepareFile(UserConnection* aSource, int64_t newSize, bool z) {
	Download* d = aSource->getDownload();
	dcassert(d != NULL);

	if(newSize != -1) {
		d->setSize(newSize);
		if(d->getFileSize() == -1)
			d->setFileSize(newSize);
	}
	
	if(d->getPos() >= d->getSize() || d->getSize() > d->getFileSize()) {
		// Already finished?
		aSource->setDownload(NULL);
		removeDownload(d);
		QueueManager::getInstance()->putDownload(d, false);
		removeConnection(aSource);
		return false;
	}

	dcassert(d->getSize() != -1);

	if(d->isSet(Download::FLAG_PARTIAL_LIST)) {
		d->setFile(new StringOutputStream(d->getPFS()));
	} else if(d->isSet(Download::FLAG_TREE_DOWNLOAD)) {
		d->setFile(new TreeOutputStream(d->getTigerTree()));
	} else {
		string target = d->getDownloadTarget();
		File::ensureDirectory(target);
		if(d->isSet(Download::FLAG_USER_LIST)) {
			if(aSource->isSet(UserConnection::FLAG_SUPPORTS_XML_BZLIST)) {
				target += ".xml.bz2";
			} else {
				target += ".xml";
			}
		}

		SharedFileStream* file = NULL;
		File* f = NULL;
		try {
			if(d->isSet(Download::FLAG_MULTI_CHUNK)) {
				file = new SharedFileStream(target, d->getStartPos(), d->getFileSize());
			} else {
				// Let's check if we can find this file in a any .SFV...
				int trunc = d->isSet(Download::FLAG_RESUME) ? 0 : File::TRUNCATE;
				f = new File(target, File::RW, File::OPEN | File::CREATE | trunc);
				if(d->isSet(Download::FLAG_ANTI_FRAG)) {
					f->setSize(d->getSize());
				}
				f->setPos(d->getPos());
			}
		} catch(const FileException& e) {
			if(d->isSet(Download::FLAG_MULTI_CHUNK))
				delete file;
			else
				delete f;
			failDownload(aSource, STRING(COULD_NOT_OPEN_TARGET_FILE) + e.getError());
			return false;
		} catch(const Exception& e) {
			if(d->isSet(Download::FLAG_MULTI_CHUNK))
				delete file;
			else
				delete f;
			failDownload(aSource, e.getError());
			return false;
		}

		if(d->isSet(Download::FLAG_MULTI_CHUNK))
			d->setFile(file);
		else 
			d->setFile(f);


		if(SETTING(BUFFER_SIZE) > 0 ) {
			try {
				d->setFile(new BufferedOutputStream<true>(d->getFile()));
			} catch(const Exception& e) {
				delete d->getFile();
				d->setFile(NULL);
				failDownload(aSource, e.getError());
				return false;
			}
		}
	
		if(d->isSet(Download::FLAG_MULTI_CHUNK)){
			if(d->getTreeValid()) {
				d->setFile(new MerkleCheckOutputStream<TigerTree, true>(d->getTigerTree(), d->getFile(), d->getPos(), d));
				d->setFlag(Download::FLAG_TTH_CHECK);
			}

			try {
				d->setFile(new ChunkOutputStream<true>(d->getFile(), &d->getTTH(), d->getStartPos()));
			} catch(const FileException&) {
				delete d->getFile();
				d->setFile(NULL);
				failDownload(aSource, STRING(COULD_NOT_OPEN_TARGET_FILE));
				return false;
			}
		} else {
			if(d->getTreeValid()) {
				if((d->getPos() % d->getTigerTree().getBlockSize()) == 0) {
					d->setFile(new MerkleCheckOutputStream<TigerTree, true>(d->getTigerTree(), d->getFile(), d->getPos()));
					d->setFlag(Download::FLAG_TTH_CHECK);
				}
			}
			if(d->isSet(Download::FLAG_ROLLBACK)) {
				d->setFile(new RollbackOutputStream<true>(f, d->getFile(), (size_t)min((int64_t)SETTING(ROLLBACK), d->getSize() - d->getPos())));
			}
		}

	}
	
	if(z) {
		d->setFlag(Download::FLAG_ZDOWNLOAD);
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
			dcassert(d->getFile());
			d->addPos(d->getFile()->write(aData, aLen), aLen);
		} catch(const ChunkDoneException e) {
			dcdebug("ChunkDoneException.....\n");

			if(e.pos > 0 && d->getTreeValid()) {
				FileChunksInfo::Ptr lpFileDataInfo = FileChunksInfo::Get(&d->getTTH());
				lpFileDataInfo->verifyBlock(e.pos - 1, d->getTigerTree(), d->getTempTarget());
			}

			d->setPos(e.pos);
			if(d->getPos() == d->getSize()){
				aSource->setDownload(NULL);
				removeDownload(d);
				QueueManager::getInstance()->putDownload(d, false);
				aSource->setLineMode(0);
				checkDownloads(aSource);
			}else{
				failDownload(aSource, e.getError());
				ClientManager::getInstance()->connect(aSource->getUser());
			}
			return;

		} catch(const FileDoneException e) {
			dcdebug("FileDoneException.....\n");

			if(!d->getTreeValid())
			{
				if(HashManager::getInstance()->getTree(d->getTTH(), d->getTigerTree()))
					d->setTreeValid(true);
			}

			UploadManager::getInstance()->abortUpload(d->getTempTarget(), false);
			abortDownload(d->getTarget(), d);

			if(d->getTreeValid()) {

				FileChunksInfo::Ptr lpFileDataInfo = FileChunksInfo::Get(&d->getTTH());
				if(!(lpFileDataInfo == (FileChunksInfo*)NULL))
				{
					dcdebug("Do last verify.....\n");
					if(!lpFileDataInfo->doLastVerify(d->getTigerTree(), d->getTempTarget())) {
						dcdebug("last verify failed .....\n");

						if ((!SETTING(SOUND_TTH).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
							PlaySound(Text::toT(SETTING(SOUND_TTH)).c_str(), NULL, SND_FILENAME | SND_ASYNC);

						char buf[128];
						snprintf(buf, sizeof(buf), CSTRING(LEAF_CORRUPTED), Util::formatBytes(d->getSize() - lpFileDataInfo->getDownloadedSize()).c_str());

						failDownload(aSource, buf);
						return;
					}
					d->setFlag(Download::FLAG_CRC32_OK);
				}
			}

			// RevConnect : For partial file sharing, abort upload first to move file correctly
			UploadManager::getInstance()->abortUpload(d->getTempTarget());
			
			// wait aborting other downloads
			for(int t = 0; t < 20; t++)
			{
				bool aborting = false;
				{
					Lock l(cs);
					
					for(Download::Iter i = downloads.begin(); i != downloads.end(); ++i) {
						Download* download = *i;
						if(download != d && download->getTarget() == d->getTarget()) {
							download->getUserConnection()->disconnect(true);
							aborting = true;
							break;
						}
					}
				}
				
				if(!aborting) break;
				Thread::sleep(250);
			}

			d->setPos(e.pos);
			if(d->getPos() == d->getSize())
				aSource->setLineMode(0);
			handleEndData(aSource);
			return;	
		}

		if(d->getPos() > d->getSize()) {
			throw Exception(STRING(TOO_MUCH_DATA));
		} else if(d->getPos() == d->getSize()) {
			if(!d->isSet(Download::FLAG_MULTI_CHUNK) || d->isSet(Download::FLAG_TREE_DOWNLOAD)) {
				handleEndData(aSource);
			}
			else{ // peer's partial size < chunk size
				// fire(DownloadManagerListener::ChunkComplete(), d);

				if(d->getTreeValid()) {
					FileChunksInfo::Ptr lpFileDataInfo = FileChunksInfo::Get(&d->getTTH());
					if(!(lpFileDataInfo == (FileChunksInfo*)NULL)){
						dcassert(d->getPos() > 0);
						lpFileDataInfo->verifyBlock(d->getPos() - 1, d->getTigerTree(), d->getTempTarget());
					}else{
						dcassert(0);
					}
				}

				aSource->setDownload(NULL);
				removeDownload(d);
				QueueManager::getInstance()->putDownload(d, false, false);
				checkDownloads(aSource);
			}
			aSource->setLineMode(0);
		}
	} catch(const RollbackException& e) {
		QueueManager::getInstance()->removeSource(d->getTarget(), aSource->getUser(), QueueItem::Source::FLAG_ROLLBACK_INCONSISTENCY);
		d->resetPos();
		failDownload(aSource, e.getError());
	} catch(const FileException& e) {
		if(e.getError().find(STRING(TTH_INCONSISTENCY)) != string::npos){
			QueueManager::getInstance()->removeSource(d->getTarget(), aSource->getUser(), QueueItem::Source::FLAG_TTH_INCONSISTENCY);
		}
		d->resetPos();
		failDownload(aSource, e.getError());
	} catch(const Exception& e) {
		// Nuke the bytes we have written, this is probably a compression error
		d->resetPos();
		failDownload(aSource, e.getError());
	}
}

/** Download finished! */
void DownloadManager::handleEndData(UserConnection* aSource) {

	dcassert(aSource->getState() == UserConnection::STATE_DONE);
	Download* d = aSource->getDownload();
	dcassert(d != NULL);

	bool reconn = true;

	if(d->isSet(Download::FLAG_TREE_DOWNLOAD)) {
		d->getFile()->flush();
		delete d->getFile();
		d->setFile(NULL);

		d->setSize(d->getTigerTree().getFileSize());

		int64_t bl = 1024;
		while(bl * (int64_t)d->getTigerTree().getLeaves().size() < d->getTigerTree().getFileSize())
			bl *= 2;
		d->getTigerTree().setBlockSize(bl);
		d->getTigerTree().calcRoot();

		if(!(d->getTTH() == d->getTigerTree().getRoot())) {
			// This tree is for a different file, remove from queue...
			removeDownload(d);
			fire(DownloadManagerListener::Failed(), d, STRING(INVALID_TREE));

			QueueManager::getInstance()->removeSource(d->getTarget(), aSource->getUser(), QueueItem::Source::FLAG_BAD_TREE, false);

			aSource->setDownload(NULL);
			QueueManager::getInstance()->putDownload(d, false);

			checkDownloads(aSource);
			return;
		}
		d->setTreeValid(true);
		reconn = false;
	} else {

		// First, finish writing the file (flushing the buffers and closing the file...)
		try {
			d->getFile()->flush();
			delete d->getFile();
			d->setFile(NULL);

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
			failDownload(aSource, e.getError());
			return;
		}

		reconn = d->isSet(Download::FLAG_MULTI_CHUNK) && (d->getPos() != d->getSize());
		dcdebug("Download finished: %s, size " I64_FMT ", downloaded " I64_FMT "\n", d->getTarget().c_str(), d->getSize(), d->getTotal());

		if(BOOLSETTING(LOG_DOWNLOADS) && (BOOLSETTING(LOG_FILELIST_TRANSFERS) || !d->isSet(Download::FLAG_USER_LIST)) && !d->isSet(Download::FLAG_TREE_DOWNLOAD)) {
			logDownload(aSource, d);
		}
	
		// Check if we need to move the file
		if( !d->getTempTarget().empty() && (Util::stricmp(d->getTarget().c_str(), d->getTempTarget().c_str()) != 0) ) {
			moveFile(d->getTempTarget(), d->getTarget());
		}
	}

	removeDownload(d);
	fire(DownloadManagerListener::Complete(), d, d->isSet(Download::FLAG_TREE_DOWNLOAD));

	aSource->setDownload(NULL);
	QueueManager::getInstance()->putDownload(d, true);	
	checkDownloads(aSource, reconn);
}

void DownloadManager::logDownload(UserConnection* aSource, Download* d) {
	StringMap params;
	params["target"] = d->getTarget();
	params["userNI"] = aSource->getUser()->getFirstNick();
	params["userI4"] = aSource->getRemoteIp();
	StringList hubNames = ClientManager::getInstance()->getHubNames(aSource->getUser()->getCID());
	if(hubNames.empty())
		hubNames.push_back(STRING(OFFLINE));
	params["hub"] = Util::toString(hubNames);
	StringList hubs = ClientManager::getInstance()->getHubs(aSource->getUser()->getCID());
	if(hubs.empty())
		hubs.push_back(STRING(OFFLINE));
	params["hubURL"] = Util::toString(hubs);
	params["fileSI"] = Util::toString(d->getFileSize());
	params["fileSIshort"] = Util::formatBytes(d->getFileSize());
	params["fileSIchunk"] = Util::toString(d->getTotal());
	params["fileSIchunkshort"] = Util::formatBytes(d->getTotal());
	params["fileSIactual"] = Util::toString(d->getActual());
	params["fileSIactualshort"] = Util::formatBytes(d->getActual());
	params["speed"] = Util::formatBytes(d->getAverageSpeed()) + "/s";
	params["time"] = Text::fromT(Util::formatSeconds((GET_TICK() - d->getStart()) / 1000));
	params["sfv"] = Util::toString(d->isSet(Download::FLAG_CRC32_OK) ? 1 : 0);
	params["fileTR"] = d->getTTH().toBase32();
	LOG(LogManager::DOWNLOAD, params);
}

void DownloadManager::moveFile(const string& source, const string& target) {
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

void DownloadManager::on(UserConnectionListener::MaxedOut, UserConnection* aSource) throw() { 
	noSlots(aSource);
}
void DownloadManager::noSlots(UserConnection* aSource) {
	if(aSource->getState() != UserConnection::STATE_FILELENGTH && aSource->getState() != UserConnection::STATE_TREE) {
		dcdebug("DM::onMaxedOut Bad state, ignoring\n");
		return;
	}

	failDownload(aSource, STRING(NO_SLOTS_AVAILABLE), false);
}

void DownloadManager::on(UserConnectionListener::Failed, UserConnection* aSource, const string& aError) throw() {
	failDownload(aSource, aError);
}

void DownloadManager::failDownload(UserConnection* aSource, const string& reason, bool connectSources) {
	Download* d = aSource->getDownload();

	if(d) {
		removeDownload(d);
		fire(DownloadManagerListener::Failed(), d, reason);

		aSource->setDownload(0);

		if ( d->isSet(Download::FLAG_USER_LIST) ) {
			if (reason.find("File Not Available") != string::npos || reason.find("File non disponibile") != string::npos ) {
				ClientManager::getInstance()->setCheating(aSource->getUser(), "", "filelist not available", SETTING(FILELIST_UNAVAILABLE), false);
				QueueManager::getInstance()->putDownload(d, true);
				removeConnection(aSource);
				return;
			} else if(reason == STRING(DISCONNECTED)) {
				ClientManager::getInstance()->fileListDisconnected(aSource->getUser());
			}
		} else if( d->isSet(Download::FLAG_TESTSUR) ) {
			if(reason == STRING(NO_SLOTS_AVAILABLE))
				ClientManager::getInstance()->setCheating(aSource->getUser(), "MaxedOut", "No slots for TestSUR. User is using slotlocker.", -1, true);
			else
				ClientManager::getInstance()->setCheating(aSource->getUser(), reason, "", -1, true);
			QueueManager::getInstance()->putDownload(d, true);
			removeConnection(aSource);
			return;
		}

		QueueManager::getInstance()->putDownload(d, false, connectSources);
	}
	removeConnection(aSource);
}

void DownloadManager::removeDownload(Download* d) {
	if(d->getFile()) {
		if(d->getActual() > 0) {
			try {
				d->getFile()->flush();
			} catch(const Exception&) {
			}
		}
		delete d->getFile();
		d->setFile(NULL);

		if(d->isSet(Download::FLAG_ANTI_FRAG)) {
			d->unsetFlag(Download::FLAG_ANTI_FRAG);
		} 
	}

	{
		Lock l(cs);
		// Either I'm stupid or the msvc7 optimizer is doing something _very_ strange here...
		// STL-port -D_STL_DEBUG complains that .begin() and .end() don't have the same owner (!),
		// but only in release build

		dcassert(find(downloads.begin(), downloads.end(), d) != downloads.end());

		downloads.erase(remove(downloads.begin(), downloads.end(), d), downloads.end());
		
/*		for(Download::Iter i = downloads.begin(); i != downloads.end(); ++i) {
			if(*i == d) {
				downloads.erase(i);
				break;
			}
		}*/
	}
}

void DownloadManager::abortDownload(const string& aTarget, Download* except) {
	Lock l(cs);
	
	for(Download::Iter i = downloads.begin(); i != downloads.end(); ++i) {
		Download* d = *i;
		if(d != except && d->getTarget() == aTarget) {
			dcassert(d->getUserConnection() != NULL);
			d->getUserConnection()->disconnect(true);
		}
	}
}

void DownloadManager::on(UserConnectionListener::ListLength, UserConnection* aSource, const string& aListLength) {
	ClientManager::getInstance()->setListLength(aSource->getUser(), aListLength);
}

void DownloadManager::on(UserConnectionListener::FileNotAvailable, UserConnection* aSource) throw() {
	if(!aSource->getDownload()) {
		aSource->disconnect(true);
		return;
	}
	fileNotAvailable(aSource);
}

/** @todo Handle errors better */
void DownloadManager::on(AdcCommand::STA, UserConnection* aSource, const AdcCommand& cmd) throw() {
	if(cmd.getParameters().size() < 2) {
		aSource->disconnect();
		return;
	}

	const string& err = cmd.getParameters()[0];
	if(err.length() < 3) {
		aSource->disconnect();
		return;
	}

	switch(Util::toInt(err.substr(0, 1))) {
	case AdcCommand::SEV_FATAL:
		aSource->disconnect();
		return;
	case AdcCommand::SEV_RECOVERABLE:
		switch(Util::toInt(err.substr(1))) {
		case AdcCommand::ERROR_FILE_NOT_AVAILABLE:
			fileNotAvailable(aSource);
			return;
		case AdcCommand::ERROR_SLOTS_FULL:
			noSlots(aSource);
			return;
		}
	}
	aSource->disconnect();
}

void DownloadManager::fileNotAvailable(UserConnection* aSource) {
	Download* d = aSource->getDownload();
	dcassert(d != NULL);
	dcdebug("File Not Available: %s\n", d->getTarget().c_str());

	fire(DownloadManagerListener::Failed(), d, d->getTargetFileName() + ": " + STRING(FILE_NOT_AVAILABLE));
	if( d->isSet(Download::FLAG_TESTSUR) ) {
		dcdebug("TestSUR File not available\n");
		removeDownload(d);

		ClientManager::getInstance()->setCheating(aSource->getUser(), "File Not Available", "", -1, false);
		
		aSource->setDownload(NULL);
		QueueManager::getInstance()->putDownload(d, true);
		removeConnection(aSource);
		return;
	}
	
	QueueManager::getInstance()->removeSource(d->getTarget(), aSource->getUser(), d->isSet(Download::FLAG_TREE_DOWNLOAD) ? QueueItem::Source::FLAG_NO_TREE : QueueItem::Source::FLAG_FILE_NOT_AVAILABLE, false);
	removeDownload(d);
	aSource->setDownload(NULL);

	QueueManager::getInstance()->putDownload(d, false, false);
	checkDownloads(aSource);
}

void DownloadManager::throttleReturnBytes(u_int32_t b) {
	if (b > 0 && b < 2*mByteSlice) {
		mBytesSpokenFor -= b;
		if (mBytesSpokenFor < 0)
			mBytesSpokenFor = 0;
	}
}

size_t DownloadManager::throttleGetSlice() {
	if (mThrottleEnable) {
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

u_int32_t DownloadManager::throttleCycleTime() {
	if (mThrottleEnable)
		return mCycleTime;
	return 0;
}

void DownloadManager::throttleZeroCounters() {
	mBytesSpokenFor = 0;
	mBytesSent = 0;
}

void DownloadManager::throttleBytesTransferred(u_int32_t i) {
	mBytesSent += i;
}

void DownloadManager::throttleSetup() {
// called once a second, plus when a download starts
// from the constructor to BufferedSocket
// with 64k, a few people get winsock error 0x2747
	unsigned int num_transfers = downloads.size();
	mDownloadLimit = (SETTING(MAX_DOWNLOAD_SPEED_LIMIT) * 1024);
	mThrottleEnable = BOOLSETTING(THROTTLE_ENABLE) && (mDownloadLimit > 0) && (num_transfers > 0);
	if (mThrottleEnable) {
		size_t inbufSize = SETTING(SOCKET_IN_BUFFER);
		if (mDownloadLimit <= (inbufSize * 10 * num_transfers)) {
			mByteSlice = mDownloadLimit / (7 * num_transfers);
			if (mByteSlice > inbufSize)
				mByteSlice = inbufSize;
			mCycleTime = 100;
			} else {
			mByteSlice = inbufSize;
			mCycleTime = 1000 * inbufSize / mDownloadLimit;
		}
	}
}

/**
 * @file
 * $Id$
 */
