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
#include "ZUtils.h"
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

		if(d->getPos() > 0) {
			tickList.push_back(d);
			(*i)->tick();
		}

		if (d->getType() == Transfer::TYPE_FILE) {
			if (d->getSize() > (SETTING(DISCONNECT_FILESIZE) * 1048576)) {
				if((d->getAverageSpeed() < SETTING(DISCONNECT_SPEED) * 1024)) {
					if(	(((aTick - d->getLastTick())/1000) > (uint32_t)SETTING(DISCONNECT_TIME)) &&
						(!QueueManager::getInstance()->dropSource(d)))
					{
							fire(DownloadManagerListener::Failed(), d, STRING(SLOW_USER) + " - " + STRING(DISCONNECTED));
							continue;
					}
				} else {
					d->setLastTick(aTick);
				}
			}
		}
		
		if(tickList.size() > 0)
			fire(DownloadManagerListener::Tick(), tickList);
			
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

void DownloadManager::removeConnection(UserConnectionPtr aConn) {
	dcassert(aConn->getDownload() == NULL);
	aConn->removeListener(this);
	aConn->disconnect();

 	Lock l(cs);
 	idlers.erase(remove(idlers.begin(), idlers.end(), aConn), idlers.end());
}

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

void DownloadManager::checkDownloads(UserConnection* aConn) {
	dcassert(aConn->getDownload() == NULL);

	QueueItem::Priority prio = QueueManager::getInstance()->hasDownload(aConn->getUser());
	if(!startDownload(prio)) {
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

	aConn->setState(UserConnection::STATE_SND);

	{
		Lock l(cs);
		downloads.push_back(d);
	}
	if(aConn->isSet(UserConnection::FLAG_SUPPORTS_XML_BZLIST) && d->getType() == Transfer::TYPE_FULL_LIST) {
		d->setFlag(Download::FLAG_XML_BZ_LIST);
	}
	aConn->send(d->getCommand(aConn->isSet(UserConnection::FLAG_SUPPORTS_ZLIB_GET)));
}

void DownloadManager::on(AdcCommand::SND, UserConnection* aSource, const AdcCommand& cmd) throw() {
	if(aSource->getState() != UserConnection::STATE_SND) {
		dcdebug("DM::onFileLength Bad state, ignoring\n");
		return;
	}
	if(!aSource->getDownload()) {
		aSource->disconnect(true);
		return;
	}

	const string& type = cmd.getParam(0);
	int64_t start = Util::toInt64(cmd.getParam(2));
	int64_t bytes = Util::toInt64(cmd.getParam(3));

	if(type != Transfer::names[aSource->getDownload()->getType()]) {
		// Uhh??? We didn't ask for this...
		aSource->disconnect();
		return;
	}

	if(prepareFile(aSource, start, bytes, cmd.hasFlag("ZL", 4))) {
		aSource->setDataMode();
	}
}

bool DownloadManager::prepareFile(UserConnection* aSource, int64_t start, int64_t bytes, bool z) {
	Download* d = aSource->getDownload();
	dcassert(d != NULL);

	dcdebug("Preparing " I64_FMT ":" I64_FMT ", " I64_FMT ":" I64_FMT"\n", d->getStartPos(), start, d->getSize(), bytes);
	if(d->getSize() == -1) {
		if(bytes != -1) {
			d->setSize(bytes);
		} else {
			failDownload(aSource, STRING(INVALID_SIZE));
			return false;
		}
	} else if(d->getSize() != bytes || d->getStartPos() != start) {
		// This is not what we requested...
		failDownload(aSource, STRING(INVALID_SIZE));
		return false;
	}
	
	if(d->getPos() >= d->getSize()) {
		// Already finished? A zero-byte file list could cause this...
		aSource->setDownload(NULL);
		removeDownload(d);
		QueueManager::getInstance()->putDownload(d, true);
		removeConnection(aSource);
		return false;
	}

	try {
		QueueManager::getInstance()->setFile(d);
	} catch(const FileException& e) {
		failDownload(aSource, STRING(COULD_NOT_OPEN_TARGET_FILE) + e.getError());
		return false;
	} catch(const Exception& e) {
		failDownload(aSource, e.getError());
		return false;
	}


	try {
		if((d->getType() == Transfer::TYPE_FILE || d->getType() == Transfer::TYPE_FULL_LIST) && SETTING(BUFFER_SIZE) > 0 ) {
			d->setFile(new BufferedOutputStream<true>(d->getFile()));
		}
	} catch(const Exception& e) {
		failDownload(aSource, e.getError());
		return false;
	} catch(...) {
		delete d->getFile();
		d->setFile(NULL);
		return false;			
	}
			
	if(d->getType() == Transfer::TYPE_FILE) {
		typedef MerkleCheckOutputStream<TigerTree, true> MerkleStream;
		
		d->setFile(new MerkleStream(d->getTigerTree(), d->getFile(), d->getStartPos()));
		d->setFlag(Download::FLAG_TTH_CHECK);
	}
	
	if(z) {
		d->setFlag(Download::FLAG_ZDOWNLOAD);
		d->setFile(new FilteredOutputStream<UnZFilter, true>(d->getFile()));
	}

	d->setStart(GET_TICK());
	aSource->setState(UserConnection::STATE_RUNNING);

	fire(DownloadManagerListener::Starting(), d);

	return true;
}	

void DownloadManager::on(UserConnectionListener::Data, UserConnection* aSource, const uint8_t* aData, size_t aLen) throw() {
	Download* d = aSource->getDownload();
	dcassert(d != NULL);

	try {
		d->addPos(d->getFile()->write(aData, aLen), aLen);

		if(d->getPos() > d->getSize()) {
			throw Exception(STRING(TOO_MUCH_DATA));
		} else if(d->getPos() == d->getSize()) {
			handleEndData(aSource);
			aSource->setLineMode(0);
		}
	} catch(const FileException& e) {
		failDownload(aSource, e.getError());
	} catch(const Exception& e) {
		failDownload(aSource, e.getError());
	}
}

/** Download finished! */
void DownloadManager::handleEndData(UserConnection* aSource) {
	dcassert(aSource->getState() == UserConnection::STATE_RUNNING);
	Download* d = aSource->getDownload();
	dcassert(d != NULL);

	if(d->getType() == Transfer::TYPE_TREE) {
		d->getFile()->flush();

		int64_t bl = 1024;
		while(bl * (int64_t)d->getTigerTree().getLeaves().size() < d->getTigerTree().getFileSize())
			bl *= 2;
		d->getTigerTree().setBlockSize(bl);
		d->getTigerTree().calcRoot();

		if(!(d->getTTH() == d->getTigerTree().getRoot())) {
			// This tree is for a different file, remove from queue...
			removeDownload(d);
			fire(DownloadManagerListener::Failed(), d, STRING(INVALID_TREE));

			QueueManager::getInstance()->removeSource(d->getPath(), aSource->getUser(), QueueItem::Source::FLAG_BAD_TREE, false);

			aSource->setDownload(NULL);
			QueueManager::getInstance()->putDownload(d, false);

			checkDownloads(aSource);
			return;
		}
		d->setTreeValid(true);
	} else {
		// First, finish writing the file (flushing the buffers and closing the file...)
		try {
			d->getFile()->flush();
		} catch(const FileException& e) {
			failDownload(aSource, e.getError());
			return;
		}

		dcdebug("Download finished: %s, size " I64_FMT ", downloaded " I64_FMT "\n", d->getPath().c_str(), d->getSize(), d->getPos());

		if(BOOLSETTING(LOG_DOWNLOADS) && (BOOLSETTING(LOG_FILELIST_TRANSFERS) || d->getType() == Transfer::TYPE_FILE)) {
			logDownload(aSource, d);
		}
	}

	removeDownload(d);
	fire(DownloadManagerListener::Complete(), d, d->getType() == Transfer::TYPE_TREE);

	aSource->setDownload(NULL);
	QueueManager::getInstance()->putDownload(d, true);	
	checkDownloads(aSource);
}

int64_t DownloadManager::getRunningAverage() {
	Lock l(cs);
	int64_t avg = 0;
	for(DownloadList::const_iterator i = downloads.begin(); i != downloads.end(); ++i) {
		Download* d = *i;
		avg += static_cast<int64_t>(d->getAverageSpeed());
	}
	return avg;
}

void DownloadManager::logDownload(UserConnection* aSource, Download* d) {
	StringMap params;
	d->getParams(*aSource, params);
	LOG(LogManager::DOWNLOAD, params);
}

void DownloadManager::on(UserConnectionListener::MaxedOut, UserConnection* aSource, string param) throw() { 
	noSlots(aSource, param);
}
void DownloadManager::noSlots(UserConnection* aSource, string param) {
	if(aSource->getState() != UserConnection::STATE_SND && aSource->getState() != UserConnection::STATE_TREE) {
		dcdebug("DM::onMaxedOut Bad state, ignoring\n");
		return;
	}

	string extra = param.empty() ? Util::emptyString : " - " + STRING(QUEUED) + param;
	failDownload(aSource, STRING(NO_SLOTS_AVAILABLE) + extra);
}

void DownloadManager::on(UserConnectionListener::Error, UserConnection* aSource, const string& aError) throw() {
	failDownload(aSource, aError);
}

void DownloadManager::on(UserConnectionListener::Failed, UserConnection* aSource, const string& aError) throw() {
	failDownload(aSource, aError);
}

void DownloadManager::failDownload(UserConnection* aSource, const string& reason) {
	Download* d = aSource->getDownload();

	if(d) {
		removeDownload(d);
		fire(DownloadManagerListener::Failed(), d, reason);

		if ( d->getType() == Transfer::TYPE_FULL_LIST ) {
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

		QueueManager::getInstance()->putDownload(d, false);
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
	}

	{
		Lock l(cs);
		dcassert(find(downloads.begin(), downloads.end(), d) != downloads.end());

		downloads.erase(remove(downloads.begin(), downloads.end(), d), downloads.end());
	}
}

void DownloadManager::abortDownload(const string& aTarget, const Download* except) {
	Lock l(cs);
	
	for(DownloadList::const_iterator i = downloads.begin(); i != downloads.end(); ++i) {
		Download* d = *i;
		if(d != except && d->getPath() == aTarget) {
			dcdebug("Trying to close connection for download 0x%X\n", d);
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
	if(err.length() != 3) {
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
	case AdcCommand::SEV_SUCCESS:
		// We don't know any messages that would give us these...
		dcdebug("Unknown success message %s %s", err.c_str(), cmd.getParam(1).c_str());
		return;
	}
	aSource->disconnect();
}

void DownloadManager::fileNotAvailable(UserConnection* aSource) {
	Download* d = aSource->getDownload();
	dcassert(d != NULL);
	dcdebug("File Not Available: %s\n", d->getPath().c_str());

	fire(DownloadManagerListener::Failed(), d, d->getTargetFileName() + ": " + STRING(FILE_NOT_AVAILABLE));
	if( d->isSet(Download::FLAG_TESTSUR) ) {
		dcdebug("TestSUR File not available\n");
		removeDownload(d);

		ClientManager::getInstance()->setCheating(aSource->getUser(), "File Not Available", "", -1, false);
		
		QueueManager::getInstance()->putDownload(d, true);
		checkDownloads(aSource);
		return;
	}
	
	QueueManager::getInstance()->removeSource(d->getPath(), aSource->getUser(), (Flags::MaskType)(d->getType() == Transfer::TYPE_TREE ? QueueItem::Source::FLAG_NO_TREE : QueueItem::Source::FLAG_FILE_NOT_AVAILABLE), false);
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
