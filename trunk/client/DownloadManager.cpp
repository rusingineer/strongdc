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

#include "DownloadManager.h"

#include "ResourceManager.h"
#include "QueueManager.h"
#include "HashManager.h"
#include "Download.h"
#include "LogManager.h"
#include "User.h"
#include "File.h"
#include "FilteredFile.h"
#include "MerkleCheckOutputStream.h"
#include "UserConnection.h"
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

DownloadManager::DownloadManager() : mDownloadLimit(0), mBytesSpokenFor(0),
	mCycleTime(0), mByteSlice(0), mThrottleEnable(BOOLSETTING(THROTTLE_ENABLE)) {
	TimerManager::getInstance()->addListener(this);
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

void DownloadManager::on(TimerManagerListener::Second, uint64_t aTick) throw() {
	Lock l(cs);

	DownloadList tickList;
	throttleSetup();

	// Tick each ongoing download
	for(DownloadList::const_iterator i = downloads.begin(); i != downloads.end(); ++i) {
		Download* d = *i;

		if(d->getTotal() > 0) {
			tickList.push_back(d);
		}

		if (!d->isSet(Download::FLAG_USER_LIST)) {
			// TODO: merge codes for automatic and manual disconnecting
			if(	(d->getStart() &&  (0 == ((int)(aTick - d->getStart()) / 1000 + 1) % 40)) &&
				(d->getRunningAverage() < (downloads.size() * 100)) &&
				(QueueManager::getInstance()->dropSource(d, true))) {
					d->getUserConnection().disconnect();
					continue;
			} else if (d->getSize() > (SETTING(MIN_FILE_SIZE) * 1048576)) {
				if((d->getRunningAverage() < SETTING(I_DOWN_SPEED) * 1024)) {
					if(	(((aTick - d->getLastTick())/1000) > (uint32_t)SETTING(DOWN_TIME)) &&
						(!QueueManager::getInstance()->dropSource(d, false))) {
							continue;
					}
				} else {
					d->setLastTick(aTick);
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

void DownloadManager::removeConnection(UserConnectionPtr aConn) {
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

	size_t write(const void* xbuf, size_t len) throw(Exception) {
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

	size_t flush() throw(Exception) {
		return 0;
	}
private:
	TigerTree& tree;
	uint8_t buf[TigerTree::HASH_SIZE];
	size_t bufPos;
};

void DownloadManager::checkIdle(const UserPtr& user) {	
	Lock l(cs);	
	for(UserConnectionList::iterator i = idlers.begin(); i != idlers.end(); ++i) {	
		UserConnection* uc = *i;	
		if(uc->getUser() == user) {	
			idlers.erase(i);	
			checkDownloads(uc);	
			return;	
		}	
	}	
}

void DownloadManager::addConnection(UserConnectionPtr conn) {
	if(!conn->isSet(UserConnection::FLAG_SUPPORTS_TTHF) || !conn->isSet(UserConnection::FLAG_SUPPORTS_ADCGET)) {
		// Can't download from these...
		conn->getUser()->setFlag(User::OLD_CLIENT);
		QueueManager::getInstance()->removeSource(conn->getUser(), QueueItem::Source::FLAG_NO_TTHF);
		removeConnection(conn);
		return;
	}

	conn->addListener(this);
	checkDownloads(conn);
}

bool DownloadManager::startDownload(QueueItem::Priority prio) {
	size_t downloadCount = getDownloadCount();

	bool full = (SETTING(DOWNLOAD_SLOTS) != 0) && (downloadCount >= (size_t)SETTING(DOWNLOAD_SLOTS));
	full = full || ((SETTING(MAX_DOWNLOAD_SPEED) != 0) && (getRunningAverage() >= (SETTING(MAX_DOWNLOAD_SPEED)*1024)));

	if(full) {
		bool extraFull = (SETTING(DOWNLOAD_SLOTS) != 0) && (getDownloadCount() >= (size_t)(SETTING(DOWNLOAD_SLOTS)+3));
		if(extraFull) {
			return false;
		}
		return prio == QueueItem::HIGHEST;
	}

	if(downloadCount > 0) {
		return prio != QueueItem::LOWEST;
	}

	return true;
}

void DownloadManager::checkDownloads(UserConnection* aConn, bool reconn /*=false*/) {
	dcassert(aConn->getDownload() == NULL);

	if(reconn || !startDownload(QueueManager::getInstance()->hasDownload(aConn->getUser()))) {
		removeConnection(aConn);
		return;
	}

	string aMessage = Util::emptyString;
	Download* d = QueueManager::getInstance()->getDownload(*aConn, aMessage);

	if(!d) {
		if(!aMessage.empty()) fire(DownloadManagerListener::Status(), aConn, aMessage);
		aConn->setLastActivity(0);

		Lock l(cs);
 	    idlers.push_back(aConn);
		return;
	}

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

			if(d->getTreeValid() && start > 0 &&
			  (d->getTigerTree().getLeaves().size() > 1 || aConn->isSet(UserConnection::FLAG_SUPPORTS_TTHL)))
			{
				d->setStartPos(getResumePos(d->getDownloadTarget(), d->getTigerTree(), start));
			} else {
				d->setStartPos(max((int64_t)0, start));
			}
	
		} else {
			d->setStartPos(0);
		}
	}

	if(d->isSet(Download::FLAG_USER_LIST)) {
		if(aConn->isSet(UserConnection::FLAG_SUPPORTS_XML_BZLIST)) {
			d->setSource(const_cast<string*>(&Transfer::USER_LIST_NAME_BZ));
		} else {
			d->setSource(const_cast<string*>(&Transfer::USER_LIST_NAME));
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
	size_t write(const void*, size_t n) throw(Exception) { return n; }
	size_t flush() throw(Exception) { return 0; }
};

int64_t DownloadManager::getResumePos(const string& file, const TigerTree& tt, int64_t startPos) {
	// Always discard data until the last block
	int64_t initPos = startPos, blockSize = tt.getBlockSize();
	if(startPos < blockSize)
		return startPos;

	startPos -= (startPos % blockSize);

	DummyOutputStream dummy;

	vector<uint8_t> buf((size_t)min((int64_t)1024*1024, blockSize));

	do {
		int64_t blockPos = startPos - blockSize;

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

	if (initPos/blockSize == startPos/blockSize) {
		startPos = initPos;
	}

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

	if(!(type == Transfer::TYPE_FILE || (type == Transfer::TYPE_TTHL && aSource->getDownload()->isSet(Download::FLAG_TREE_DOWNLOAD)) ||
		(type == Transfer::TYPE_LIST && aSource->getDownload()->isSet(Download::FLAG_PARTIAL_LIST))) )
	{
		// Uhh??? We didn't ask for this?
		aSource->disconnect(true);
		return;
	}

	if(prepareFile(aSource, (bytes == -1) ? -1 : aSource->getDownload()->getPos() + bytes, cmd.hasFlag("ZL", 4))) {
		aSource->setDataMode();
	}
}

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

		IOStream* file = NULL;

		try {
			if(d->isSet(Download::FLAG_MULTI_CHUNK)) {
				file = new SharedFileStream(target, d->getStartPos(), d->getFileSize());
			} else {
				// Let's check if we can find this file in a any .SFV...
				int trunc = d->isSet(Download::FLAG_RESUME) ? 0 : File::TRUNCATE;
				file = new File(target, File::RW, File::OPEN | File::CREATE | trunc);
				if(d->isSet(Download::FLAG_ANTI_FRAG)) {
					((File*)file)->setSize(d->getSize());
				}
			}
		} catch(const FileException& e) {
			delete file;
			failDownload(aSource, STRING(COULD_NOT_OPEN_TARGET_FILE) + e.getError());
			return false;
		} catch(const Exception& e) {
			delete file;
			failDownload(aSource, e.getError());
			return false;
		}

		d->setFile(file);

		try {
			if(SETTING(BUFFER_SIZE) > 0 ) {
				d->setFile(new BufferedOutputStream<true>(d->getFile()));
			}
			
			if(!d->isSet(Download::FLAG_USER_LIST)) {
				typedef MerkleCheckOutputStream<TigerTree, true> MerkleStream;
				if(d->getTreeValid()) {
					MerkleStream* stream;
					if(d->isSet(Download::FLAG_MULTI_CHUNK)) {
						stream = new MerkleStream(d->getTigerTree(), d->getFile(), d->getPos(), d);
					} else {
						int64_t blockLeft = d->getPos() % d->getTigerTree().getBlockSize();
						int64_t blockPos = d->getPos() - blockLeft;
						
						stream = new MerkleStream(d->getTigerTree(), d->getFile(), blockPos);
						// @todo catch exceptions
						if(blockLeft > 0) {
							std::vector<uint8_t> buf(static_cast<size_t>(std::min(blockLeft, static_cast<int64_t>(64*1024))));
						
							((File*)file)->setPos(blockPos);
							
							while(blockLeft > 0) {
								size_t x = static_cast<size_t>(std::min(blockLeft, static_cast<int64_t>(buf.size())));
								size_t n = file->read(&buf[0], x);
								blockLeft -= n;
								stream->commitBytes(&buf[0], n);
							}
						}					
					}
					d->setFile(stream);
					d->setFlag(Download::FLAG_TTH_CHECK);
				}
			}
				
			if(d->isSet(Download::FLAG_MULTI_CHUNK)) {
				d->setFile(new ChunkOutputStream<true>(d->getFile(), &d->getTTH(), d->getStartPos()));
			} else {
				((File*)file)->setPos(d->getPos());
			}
		} catch(const Exception& e) {
			delete d->getFile();
			d->setFile(NULL);
			failDownload(aSource, e.getError());
			return false;
		} catch(...) {
			delete d->getFile();
			d->setFile(NULL);
			return false;			
		}
	}
	
	if(z) {
		d->setFlag(Download::FLAG_ZDOWNLOAD);
		d->setFile(new FilteredOutputStream<UnZFilter, true>(d->getFile()));
	}

	dcassert(d->getPos() != -1);
	d->setStart(GET_TICK());
	aSource->setState(UserConnection::STATE_RUNNING);

	fire(DownloadManagerListener::Starting(), d);

	return true;
}	

void DownloadManager::on(UserConnectionListener::Data, UserConnection* aSource, const uint8_t* aData, size_t aLen) throw() {
	Download* d = aSource->getDownload();
	dcassert(d != NULL);

	try {
		try{
			dcassert(d->getFile());
			d->addPos(d->getFile()->write(aData, aLen), aLen);
		} catch(const ChunkDoneException e) {
			dcdebug("ChunkDoneException.....\n");

			if(d->getTreeValid() && e.pos > 0) {
				FileChunksInfo::Ptr lpFileDataInfo = FileChunksInfo::Get(&d->getTTH());
				lpFileDataInfo->verifyBlock(e.pos - 1, d->getTigerTree(), d->getTempTarget());
			}
			
			d->setPos(e.pos);
			if(d->getPos() == d->getSize()){
				//fire(DownloadManagerListener::Failed(), d, e.getError());
				aSource->setDownload(NULL);
				removeDownload(d);
				QueueManager::getInstance()->putDownload(d, false, false);
				aSource->setLineMode(0);
				checkDownloads(aSource);
			}else{
				failDownload(aSource, e.getError());
				ClientManager::getInstance()->connect(aSource->getUser(), Util::toString(Util::rand()));
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

						failDownload(aSource, Util::emptyString);
						return;
					}
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
					
					for(DownloadList::const_iterator i = downloads.begin(); i != downloads.end(); ++i) {
						Download* download = *i;
						if(download != d && download->getTarget() == d->getTarget()) {
							download->getUserConnection().disconnect(true);
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
				//fire(DownloadManagerListener::Failed(), d, CSTRING(BLOCK_FINISHED));

				if(d->getTreeValid()) {
					FileChunksInfo::Ptr lpFileDataInfo = FileChunksInfo::Get(&d->getTTH());
					if(!(lpFileDataInfo == (FileChunksInfo*)NULL)) {
						dcassert(d->getPos() > 0);
						lpFileDataInfo->verifyBlock(d->getPos() - 1, d->getTigerTree(), d->getTempTarget());
					}
				}
				
				aSource->setDownload(NULL);
				removeDownload(d);
				QueueManager::getInstance()->putDownload(d, false, false);
				checkDownloads(aSource);
			}
			aSource->setLineMode(0);
		}
	} catch(const FileException& e) {
		failDownload(aSource, e.getError());
	} catch(const Exception& e) {
		// Nuke the bytes we have written, this is probably a compression error
		d->resetPos();
		failDownload(aSource, e.getError());
	}
}

/** Download finished! */
void DownloadManager::handleEndData(UserConnection* aSource) {

	dcassert(aSource->getState() == UserConnection::STATE_RUNNING);
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

	QueueManager::getInstance()->putDownload(d, true);	
	checkDownloads(aSource, reconn);
}

int64_t DownloadManager::getRunningAverage() {
	Lock l(cs);
	int64_t avg = 0;
	for(DownloadList::const_iterator i = downloads.begin(); i != downloads.end(); ++i) {
		Download* d = *i;
		avg += d->getRunningAverage();
	}
	return avg;
}

void DownloadManager::logDownload(UserConnection* aSource, Download* d) {
	StringMap params;
	d->getParams(*aSource, params);
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

void DownloadManager::on(UserConnectionListener::MaxedOut, UserConnection* aSource, string param) throw() { 
	noSlots(aSource, param);
}
void DownloadManager::noSlots(UserConnection* aSource, string param) {
	if(aSource->getState() != UserConnection::STATE_FILELENGTH && aSource->getState() != UserConnection::STATE_TREE) {
		dcdebug("DM::onMaxedOut Bad state, ignoring\n");
		return;
	}

	string extra = param.empty() ? Util::emptyString : " - Queued: " + param;
	failDownload(aSource, STRING(NO_SLOTS_AVAILABLE) + extra, false);
}

void DownloadManager::on(UserConnectionListener::Error, UserConnection* aSource, const string& aError) throw() {
	failDownload(aSource, aError);
}

void DownloadManager::on(UserConnectionListener::Failed, UserConnection* aSource, const string& aError) throw() {
	failDownload(aSource, aError);
}

void DownloadManager::failDownload(UserConnection* aSource, const string& reason, bool connectSources) {
	Download* d = aSource->getDownload();

	if(d) {
		removeDownload(d);
		fire(DownloadManagerListener::Failed(), d, reason);

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

void DownloadManager::abortDownload(const string& aTarget, const Download* except) {
	Lock l(cs);
	
	for(DownloadList::const_iterator i = downloads.begin(); i != downloads.end(); ++i) {
		Download* d = *i;
		if(d != except && d->getTarget() == aTarget) {
			d->getUserConnection().disconnect(true);
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
			string param;
			noSlots(aSource, cmd.getParam("QP", 0, param) ? param : Util::emptyString);
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
		
		QueueManager::getInstance()->putDownload(d, true);
		checkDownloads(aSource);
		return;
	}
	
	QueueManager::getInstance()->removeSource(d->getTarget(), aSource->getUser(), (Flags::MaskType)(d->isSet(Download::FLAG_TREE_DOWNLOAD) ? QueueItem::Source::FLAG_NO_TREE : QueueItem::Source::FLAG_FILE_NOT_AVAILABLE), false);
	removeDownload(d);

	QueueManager::getInstance()->putDownload(d, false, false);
	checkDownloads(aSource);
}

void DownloadManager::throttleReturnBytes(size_t b) {
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

void DownloadManager::throttleSetup() {
	// called once a second
	// with 64k, a few people get winsock error 0x2747
	size_t num_transfers = downloads.size();
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
	mBytesSpokenFor = 0;
}

/**
 * @file
 * $Id$
 */
