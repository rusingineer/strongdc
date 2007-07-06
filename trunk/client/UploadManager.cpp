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

#include "UploadManager.h"

#include "ConnectionManager.h"
#include "LogManager.h"
#include "ShareManager.h"
#include "ClientManager.h"
#include "FilteredFile.h"
#include "ZUtils.h"
#include "ResourceManager.h"
#include "HashManager.h"
#include "AdcCommand.h"
#include "FavoriteManager.h"
#include "CryptoManager.h"
#include "Upload.h"
#include "UserConnection.h"
#include "QueueManager.h"
#include "FinishedManager.h"
#include "SharedFileStream.h"

static const string UPLOAD_AREA = "Uploads";

UploadManager::UploadManager() throw() : running(0), extra(0), lastGrant(0), mUploadLimit(0), 
	mBytesSent(0), mBytesSpokenFor(0), mCycleTime(0), mByteSlice(0), mThrottleEnable(BOOLSETTING(THROTTLE_ENABLE)), 
	m_iHighSpeedStartTick(0), isFireball(false), isFileServer(false) {	
	ClientManager::getInstance()->addListener(this);
	TimerManager::getInstance()->addListener(this);
}

UploadManager::~UploadManager() throw() {
	TimerManager::getInstance()->removeListener(this);
	ClientManager::getInstance()->removeListener(this);
	{
		Lock l(cs);
		for(UploadQueueItem::UserMapIter ii = waitingUsers.begin(); ii != waitingUsers.end(); ++ii) {
			for(UploadQueueItem::Iter i = ii->second.begin(); i != ii->second.end(); ++i) {
				(*i)->dec();
			}
		}
		waitingUsers.clear();
	}

	while(true) {
		{
			Lock l(cs);
			if(uploads.empty())
				break;
		}
		Thread::sleep(100);
	}
}

bool UploadManager::prepareFile(UserConnection& aSource, const string& aType, const string& aFile, int64_t aStartPos, int64_t& aBytes, bool listRecursive) {
	if(aFile.empty() || aStartPos < 0 || aBytes < -1 || aBytes == 0) {
		aSource.fileNotAvail("Invalid request");
		return false;
	}
	
	bool partialShare = false;

	InputStream* is = 0;
	int64_t start = 0;
	int64_t bytesLeft = 0;
	int64_t size = 0;

	bool userlist = (aFile == Transfer::USER_LIST_NAME_BZ || aFile == Transfer::USER_LIST_NAME);
	bool free = userlist;
	bool leaves = false;
	bool partList = false;

	string sourceFile;
	try {
		if(aType == Transfer::TYPE_FILE) {
			sourceFile = ShareManager::getInstance()->toReal(aFile);

			if(aFile == Transfer::USER_LIST_NAME) {
				// Unpack before sending...
				string bz2 = File(sourceFile, File::READ, File::OPEN).read();
				string xml;
				CryptoManager::getInstance()->decodeBZ2(reinterpret_cast<const uint8_t*>(bz2.data()), bz2.size(), xml);
				// Clear to save some memory...
				string().swap(bz2);
				is = new MemoryInputStream(xml);
				start = 0;
				bytesLeft = size = xml.size();
			} else {
				File* f = new File(sourceFile, File::READ, File::OPEN);

				start = aStartPos;
				size = f->getSize();
				bytesLeft = (aBytes == -1) ? size : aBytes;

				if(size < (start + bytesLeft)) {
					aSource.fileNotAvail();
					delete f;
					return false;
				}

				free = free || (size <= (int64_t)(SETTING(SET_MINISLOT_SIZE) * 1024) );

				f->setPos(start);

				is = f;
				if((start + bytesLeft) < size) {
					is = new LimitedInputStream<true>(is, aBytes);
				}
			}
		} else if(aType == Transfer::TYPE_TTHL) {
			//sourceFile = ShareManager::getInstance()->toReal(aFile);
			sourceFile = aFile;
			MemoryInputStream* mis = ShareManager::getInstance()->getTree(aFile);
			if(!mis) {
				aSource.fileNotAvail();
				return false;
			}

			start = 0;
			bytesLeft = size = mis->getSize();
			is = mis;
			leaves = true;
			free = true;
		} else if(aType == Transfer::TYPE_LIST) {
			// Partial file list
			MemoryInputStream* mis = ShareManager::getInstance()->generatePartialList(aFile, listRecursive);
			if(mis == NULL) {
				aSource.fileNotAvail();
				return false;
			}
			// Some old dc++ clients err here...
			aBytes = -1;
			start = 0;
			bytesLeft = size = mis->getSize();
	
			is = mis;
			free = true;
			partList = true;
		} else {
			aSource.fileNotAvail("Unknown file type");
			return false;
		}
	} catch(const ShareException& e) {
		// -- Added by RevConnect : Partial file sharing upload
		if(aFile.compare(0, 4, "TTH/") == 0) {

			TTHValue fileHash(aFile.substr(4));

			// find in download queue
			string target;
			string tempTarget;

            if(QueueManager::getInstance()->getTargetByRoot(fileHash, target, tempTarget)){
				if(aType == Transfer::TYPE_FILE) {
					sourceFile = tempTarget;
					// check start position and bytes
					FileChunksInfo::Ptr chunksInfo = FileChunksInfo::Get(&fileHash);
					if(chunksInfo && chunksInfo->isVerified(aStartPos, aBytes)){
						try{
							SharedFileStream* ss = new SharedFileStream(sourceFile, aStartPos);
							is = ss;
							start = aStartPos;
							size = chunksInfo->fileSize;
							bytesLeft = (aBytes == -1) ? size : aBytes;

							if(size < (start + bytesLeft)) {
								aSource.fileNotAvail();
								delete is;
								return false;
							}

							if((aStartPos + bytesLeft) < size) {
								is = new LimitedInputStream<true>(is, aBytes);
							}

							partialShare = true;
							goto ok;
						}catch(const Exception&) {
							aSource.fileNotAvail();
							//aSource.disconnect();
							delete is;
							return false;
						}
					}else{
						// Hit this when user readd partial source without partial info
						//dcassert(0);
					}
				}
			// Share finished file
			}else{
				target = FinishedManager::getInstance()->getTarget(fileHash.toBase32());

				if(!target.empty() && Util::fileExists(target)){
					if(aType == Transfer::TYPE_FILE) {
						sourceFile = target;
						try{
							is = new SharedFileStream(sourceFile, aStartPos, 0, true);
							start = aStartPos;
							size = File::getSize(sourceFile);
							bytesLeft = (aBytes == -1) ? size : aBytes;

							if(size < (start + bytesLeft)) {
								aSource.fileNotAvail();
								delete is;
								return false;
							}

							if((aStartPos + bytesLeft) < size) {
								is = new LimitedInputStream<true>(is, aBytes);
							}

							partialShare = true;
							goto ok;
						}catch(const Exception&){
							aSource.fileNotAvail();
							delete is;
							return false;
						}
					}
				}
			}
		}
		aSource.fileNotAvail(e.getError());
		return false;
	} catch(const Exception& e) {
		LogManager::getInstance()->message(STRING(UNABLE_TO_SEND_FILE) + sourceFile + ": " + e.getError());
		aSource.fileNotAvail();
		return false;
	}

ok:

	Lock l(cs);

	bool extraSlot = false;

	if(!aSource.isSet(UserConnection::FLAG_HASSLOT)) {
		bool hasReserved = (reservedSlots.find(aSource.getUser()) != reservedSlots.end());
		bool isFavorite = FavoriteManager::getInstance()->hasSlot(aSource.getUser());

		if(!(hasReserved || isFavorite || getFreeSlots() > 0 || getAutoSlot())) {
			bool supportsFree = aSource.isSet(UserConnection::FLAG_SUPPORTS_MINISLOTS);
			bool allowedFree = aSource.isSet(UserConnection::FLAG_HASEXTRASLOT) || aSource.isSet(UserConnection::FLAG_OP) || getFreeExtraSlots() > 0;
			if(free && supportsFree && allowedFree) {
				extraSlot = true;
			} else {
				delete is;
				aSource.maxedOut();
				addFailedUpload(aSource.getUser(), sourceFile, aStartPos, size);
				aSource.disconnect();
				return false;
			}
		}

		setLastGrant(GET_TICK());
	}
	clearUserFiles(aSource.getUser());

	bool resumed = false;
	for(UploadList::iterator i = delayUploads.begin(); i != delayUploads.end(); ++i) {
		Upload* up = *i;
		if(&aSource == &up->getUserConnection()) {
			delayUploads.erase(i);
			if(sourceFile != up->getSourceFile()) {
				logUpload(up);
			} else {
				resumed = true;
			}
			dcdebug("Upload from %s removed on next chunk\n", up->getUserConnection().getUser()->getFirstNick());
			delete up;
			break;
		}
	}

	Upload* u = new Upload(aSource);
	u->setStream(is);
	if(aBytes == -1)
		u->setSize(size);
	else
		u->setSize(start + bytesLeft);
		
	if(u->getSize() != size)
		u->setFlag(Upload::FLAG_CHUNKED);

	u->setFileSize(size);
	u->setStartPos(start);
	u->setSourceFile(sourceFile);

	if(userlist)
		u->setFlag(Upload::FLAG_USER_LIST);
	if(leaves)
		u->setFlag(Upload::FLAG_TTH_LEAVES);
	if(partList)
		u->setFlag(Upload::FLAG_PARTIAL_LIST);
	if(partialShare)
		u->setFlag(Upload::FLAG_PARTIAL_SHARE);
	if(resumed)
		u->setFlag(Upload::FLAG_RESUMED);

	uploads.push_back(u);

	throttleSetup();
	if(!aSource.isSet(UserConnection::FLAG_HASSLOT)) {
		if(extraSlot) {
			if(!aSource.isSet(UserConnection::FLAG_HASEXTRASLOT)) {
				aSource.setFlag(UserConnection::FLAG_HASEXTRASLOT);
				extra++;
			}
		} else {
			if(aSource.isSet(UserConnection::FLAG_HASEXTRASLOT)) {
				aSource.unsetFlag(UserConnection::FLAG_HASEXTRASLOT);
				extra--;
			}
			aSource.setFlag(UserConnection::FLAG_HASSLOT);
			running++;
		}
	}

	return true;
}

int64_t UploadManager::getRunningAverage() {
	Lock l(cs);
	int64_t avg = 0;
	for(UploadList::const_iterator i = uploads.begin(); i != uploads.end(); ++i) {
		Upload* u = *i;
		avg += (int)u->getRunningAverage();
	}
	return avg;
}

bool UploadManager::getAutoSlot() {
	/** A 0 in settings means disable */
	if(SETTING(MIN_UPLOAD_SPEED) == 0)
		return false;
	/** Only grant one slot per 30 sec */
	if(GET_TICK() < getLastGrant() + 30*1000)
		return false;
	/** Grant if upload speed is less than the threshold speed */
	return getRunningAverage() < (SETTING(MIN_UPLOAD_SPEED)*1024);
}

void UploadManager::removeUpload(Upload* aUpload, bool delay) {
	Lock l(cs);
	dcassert(find(uploads.begin(), uploads.end(), aUpload) != uploads.end());
	uploads.erase(remove(uploads.begin(), uploads.end(), aUpload), uploads.end());
	throttleSetup();
	
	if(delay) {
		aUpload->setStart(GET_TICK());
		delayUploads.push_back(aUpload);
	} else {
		delete aUpload;
	}
}

void UploadManager::reserveSlot(const UserPtr& aUser, uint32_t aTime) {
	{
		Lock l(cs);
		reservedSlots[aUser] = GET_TICK() + aTime*1000;
	}
	if(aUser->isOnline())
		ClientManager::getInstance()->connect(aUser, Util::toString(Util::rand()));	
}

void UploadManager::unreserveSlot(const UserPtr& aUser) {
	SlotIter uis = reservedSlots.find(aUser);
	if(uis != reservedSlots.end())
		reservedSlots.erase(uis);
}

void UploadManager::on(UserConnectionListener::Get, UserConnection* aSource, const string& aFile, int64_t aResume) throw() {
	if(aSource->getState() != UserConnection::STATE_GET) {
		dcdebug("UM::onGet Bad state, ignoring\n");
		return;
	}
	
	int64_t bytes = -1;
	if(prepareFile(*aSource, Transfer::TYPE_FILE, Util::toAdcFile(aFile), aResume, bytes)) {
		aSource->setState(UserConnection::STATE_SEND);
		aSource->fileLength(Util::toString(aSource->getUpload()->getSize()));
	}
}

void UploadManager::on(UserConnectionListener::Send, UserConnection* aSource) throw() {
	if(aSource->getState() != UserConnection::STATE_SEND) {
		dcdebug("UM::onSend Bad state, ignoring\n");
		return;
	}

	Upload* u = aSource->getUpload();
	dcassert(u != NULL);

	u->setStart(GET_TICK());
	aSource->setState(UserConnection::STATE_RUNNING);
	aSource->transmitFile(u->getStream());
	fire(UploadManagerListener::Starting(), u);
}

void UploadManager::on(AdcCommand::GET, UserConnection* aSource, const AdcCommand& c) throw() {
	int64_t aBytes = Util::toInt64(c.getParam(3));
	int64_t aStartPos = Util::toInt64(c.getParam(2));
	const string& fname = c.getParam(1);
	const string& type = c.getParam(0);

	if(prepareFile(*aSource, type, fname, aStartPos, aBytes, c.hasFlag("RE", 4))) {
		Upload* u = aSource->getUpload();
		dcassert(u != NULL);

		AdcCommand cmd(AdcCommand::CMD_SND);
		cmd.addParam(type).addParam(fname)
			.addParam(Util::toString(u->getPos()))
			.addParam(Util::toString(u->getSize() - u->getPos()));

		if(c.hasFlag("ZL", 4)) {
			u->setStream(new FilteredInputStream<ZFilter, true>(u->getStream()));
			u->setFlag(Upload::FLAG_ZUPLOAD);
			cmd.addParam("ZL1");
		}

		aSource->send(cmd);

		u->setStart(GET_TICK());
		aSource->setState(UserConnection::STATE_RUNNING);
		aSource->transmitFile(u->getStream());
		fire(UploadManagerListener::Starting(), u);
	}
}

void UploadManager::on(UserConnectionListener::BytesSent, UserConnection* aSource, size_t aBytes, size_t aActual) throw() {
	dcassert(aSource->getState() == UserConnection::STATE_RUNNING);
	Upload* u = aSource->getUpload();
	dcassert(u != NULL);
	u->addPos(aBytes, aActual);
	throttleBytesTransferred(aActual);
}

void UploadManager::on(UserConnectionListener::Failed, UserConnection* aSource, const string& aError) throw() {
	Upload* u = aSource->getUpload();

	if(u) {
		fire(UploadManagerListener::Failed(), u, aError);

		dcdebug("UM::onFailed: Removing upload from %s\n", aSource->getUser()->getFirstNick());
		removeUpload(u);
	}

	removeConnection(aSource);
}

void UploadManager::on(UserConnectionListener::TransmitDone, UserConnection* aSource) throw() {
	dcassert(aSource->getState() == UserConnection::STATE_RUNNING);
	Upload* u = aSource->getUpload();
	dcassert(u != NULL);

	aSource->setState(UserConnection::STATE_GET);

	if(!u->isSet(Upload::FLAG_CHUNKED)) {
		logUpload(u);
		removeUpload(u);
	} else {
		removeUpload(u, true);
	}
}

void UploadManager::logUpload(Upload* u) {
	if(BOOLSETTING(LOG_UPLOADS) && (BOOLSETTING(LOG_FILELIST_TRANSFERS) || !u->isSet(Upload::FLAG_USER_LIST)) && 
		!u->isSet(Upload::FLAG_TTH_LEAVES)) {
		StringMap params;
		u->getParams(u->getUserConnection(), params);
		LOG(LogManager::UPLOAD, params);
	}

	fire(UploadManagerListener::Complete(), u);
}

void UploadManager::addFailedUpload(const UserPtr& User, const string& file, int64_t pos, int64_t size) {
	uint64_t itime = GET_TIME();
	bool found = false;
	UploadQueueItem::UserMap::iterator j = waitingUsers.find(User);
	if(j != waitingUsers.end()) {
		for(UploadQueueItem::Iter i = j->second.begin(); i != j->second.end(); ++i) {
			if((*i)->getFile() == file) {
				(*i)->setPos(pos);
				found = true;
				break;
			}
		}
	}
	if(found == false) {
		UploadQueueItem* uqi = new UploadQueueItem(User, file, pos, size, itime);
		if(j == waitingUsers.end()) {
			UploadQueueItem::List l;
			l.push_back(uqi);
			waitingUsers.insert(make_pair(User, l));
		} else {
			j->second.push_back(uqi);
		}
		fire(UploadManagerListener::QueueAdd(), uqi);
	}
}

void UploadManager::clearUserFiles(const UserPtr& source) {
	UploadQueueItem::UserMap::iterator ii = waitingUsers.find(source);
	if(ii != waitingUsers.end()) {
		for(UploadQueueItem::Iter i = ii->second.begin(); i != ii->second.end(); ++i) {
			fire(UploadManagerListener::QueueItemRemove(), (*i));
			(*i)->dec();
		}
		waitingUsers.erase(ii);
		fire(UploadManagerListener::QueueRemove(), source);
	}
}

const UploadQueueItem::UserMap UploadManager::getWaitingUsers() {
	Lock l(cs);
	return waitingUsers;
}

void UploadManager::addConnection(UserConnectionPtr conn) {
	conn->addListener(this);
	conn->setState(UserConnection::STATE_GET);
}
	
void UploadManager::removeConnection(UserConnection* aSource) {
	dcassert(aSource->getUpload() == NULL);
	aSource->removeListener(this);
	if(aSource->isSet(UserConnection::FLAG_HASSLOT)) {
		running--;
		aSource->unsetFlag(UserConnection::FLAG_HASSLOT);

		UploadQueueItem::UserMap u;
		{
			Lock l(cs);
			u = waitingUsers;
		}

		int freeSlots = getFreeSlots()*2;
		for(UploadQueueItem::UserMapIter i = u.begin(); i != u.end(); ++i) {
			UserPtr aUser = i->first;
			if(aUser->isOnline()) {
				ClientManager::getInstance()->connect(aUser, Util::toString(Util::rand()));
				freeSlots--;
			}
			if(freeSlots == 0) break;
		}
	} 
	if(aSource->isSet(UserConnection::FLAG_HASEXTRASLOT)) {
		extra--;
		aSource->unsetFlag(UserConnection::FLAG_HASEXTRASLOT);
	}
}

void UploadManager::on(TimerManagerListener::Minute, uint32_t aTick) throw() {
	UserList disconnects;
	{
		Lock l(cs);
		for(SlotIter j = reservedSlots.begin(); j != reservedSlots.end();) {
			if(j->second < aTick) {
				reservedSlots.erase(j++);
			} else {
				++j;
			}
		}
	
		if( BOOLSETTING(AUTO_KICK) ) {
			for(UploadList::const_iterator i = uploads.begin(); i != uploads.end(); ++i) {
				Upload* u = *i;
				if(u->getUser()->isOnline()) {
					u->unsetFlag(Upload::FLAG_PENDING_KICK);
					continue;
				}

				if(u->isSet(Upload::FLAG_PENDING_KICK)) {
					disconnects.push_back(u->getUser());
					continue;
				}

				if(BOOLSETTING(AUTO_KICK_NO_FAVS) && FavoriteManager::getInstance()->isFavoriteUser(u->getUser())) {
					continue;
				}

				u->setFlag(Upload::FLAG_PENDING_KICK);
			}
		}
	}
		
	for(UserList::const_iterator i = disconnects.begin(); i != disconnects.end(); ++i) {
		LogManager::getInstance()->message(STRING(DISCONNECTED_USER) + (*i)->getFirstNick());
		ConnectionManager::getInstance()->disconnect(*i, false);
	}
}

void UploadManager::on(GetListLength, UserConnection* conn) throw() { 
	conn->listLen("42");
}

void UploadManager::on(AdcCommand::GFI, UserConnection* aSource, const AdcCommand& c) throw() {
	if(c.getParameters().size() < 2) {
		aSource->send(AdcCommand(AdcCommand::SEV_RECOVERABLE, AdcCommand::ERROR_PROTOCOL_GENERIC, "Missing parameters"));
		return;
	}

	const string& type = c.getParam(0);
	const string& ident = c.getParam(1);

	if(type == Transfer::TYPE_FILE) {
		try {
			aSource->send(ShareManager::getInstance()->getFileInfo(ident));
		} catch(const ShareException&) {
			aSource->fileNotAvail();
		}
	} else {
		aSource->fileNotAvail();
	}
}

// TimerManagerListener
void UploadManager::on(TimerManagerListener::Second, uint32_t aTick) throw() {
	{
		Lock l(cs);
		throttleSetup();

		if((aTick / 1000) % 10 == 0) {
			for(UploadList::iterator i = delayUploads.begin(); i != delayUploads.end();) {
				Upload* u = *i;
				if((aTick - u->getStart()) > 15000) {
					logUpload(u);
					delete u;
					delayUploads.erase(i);
					i = delayUploads.begin();
				} else i++;
			}
		}

		if(uploads.size() > 0)
			fire(UploadManagerListener::Tick(), uploads);

		fire(UploadManagerListener::QueueUpdate());
	}
	if(!isFireball) {
		if(getRunningAverage() >= 102400) {
			if (m_iHighSpeedStartTick > 0) {
				if ((aTick - m_iHighSpeedStartTick) > 60000) {
					isFireball = true;
					ClientManager::getInstance()->infoUpdated(true);
					return;
				}
			} else {
				m_iHighSpeedStartTick = aTick;
			}
		} else {
			m_iHighSpeedStartTick = 0;
		}

		if(!isFileServer) {
			if(	(Util::getUptime() > 7200) && // > 2 hours uptime
				(Socket::getTotalUp() > 209715200) && // > 200 MB uploaded
				(ShareManager::getInstance()->getSharedSize() > 2147483648)) { // > 2 GB shared
					isFileServer = true;
					ClientManager::getInstance()->infoUpdated(true);
			}
		}
	}
}

void UploadManager::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) throw() {
	if(!aUser->isOnline()) {
		Lock l(cs);
		clearUserFiles(aUser);
	}
}

size_t UploadManager::throttleGetSlice()  {
	if (mThrottleEnable) {
		size_t left = mUploadLimit - mBytesSpokenFor;
		if (left > 0) {
			if (left > 2*mByteSlice) {
				mBytesSpokenFor += mByteSlice;
				return mByteSlice;
			} else {
				mBytesSpokenFor += left;
				return left;
			}
		} else {
			return 16; // must send > 0 bytes or threadSendFile thinks the transfer is complete
		}
	} else {
		return (size_t)-1;
	}
}

size_t UploadManager::throttleCycleTime() const {
	if (mThrottleEnable)
		return mCycleTime;
	return 0;
}

void UploadManager::throttleBytesTransferred(uint32_t i)  {
	mBytesSent += i;
}

void UploadManager::throttleSetup() {
// called once a second, plus when uploads start
// from the constructor to BufferedSocket
	unsigned int num_transfers = uploads.size();
	mUploadLimit = SETTING(MAX_UPLOAD_SPEED_LIMIT) * 1024;
	mThrottleEnable = BOOLSETTING(THROTTLE_ENABLE) && (mUploadLimit > 0) && (num_transfers > 0);
	if (mThrottleEnable) {
		size_t inbufSize = SETTING(SOCKET_OUT_BUFFER);	
		if (mUploadLimit <= (inbufSize * 10 * num_transfers)) {
			mByteSlice = mUploadLimit / (5 * num_transfers);
			if (mByteSlice > inbufSize)
				mByteSlice = inbufSize;
			mCycleTime = 100;
		} else {
			mByteSlice = inbufSize;
			mCycleTime = 1000 * inbufSize / mUploadLimit;
		}
		mBytesSpokenFor = 0;
		mBytesSent = 0;
	}
}

void UploadManager::removeDelayUpload(const UserPtr& aUser) {
	Lock l(cs);
	for(UploadList::iterator i = delayUploads.begin(); i != delayUploads.end(); ++i) {
		Upload* up = *i;
		if(aUser == up->getUser()) {
			delayUploads.erase(i);
			delete up;
			break;
		}
	}		
}
	
/**
 * Abort upload of specific file
 */
void UploadManager::abortUpload(const string& aFile, bool waiting){
	bool nowait = true;

	{
		Lock l(cs);

		for(UploadList::const_iterator i = uploads.begin(); i != uploads.end(); i++){
			Upload* u = (*i);

			if(u->getSourceFile() == aFile){
				u->getUserConnection().disconnect(true);
				nowait = false;
			}
		}
	}
	
	if(nowait) return;
	if(!waiting) return;
	
	for(int i = 0; i < 20 && nowait == false; i++){
		Thread::sleep(250);
		{
			Lock l(cs);

			nowait = true;
			for(UploadList::const_iterator i = uploads.begin(); i != uploads.end(); i++){
				Upload* u = (*i);

				if(u->getSourceFile() == aFile){
					dcdebug("upload %s is not removed\n", aFile);
					nowait = false;
					break;
				}
			}
		}
	}
	
	if(!nowait)
		dcdebug("abort upload timeout %s\n", aFile);
}

/**
 * @file
 * $Id$
 */
