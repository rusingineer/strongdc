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

#include "UploadManager.h"

#include "ConnectionManager.h"
#include "LogManager.h"
#include "ShareManager.h"
#include "HubManager.h"
#include "ClientManager.h"
#include "FilteredFile.h"
#include "ZUtils.h"
#include "ResourceManager.h"
#include "HashManager.h"
#include "AdcCommand.h"

#define INBUFSIZE 64*1024
bool UploadManager::m_boFireball = false;
bool UploadManager::m_boFileServer = false;

static const string UPLOAD_AREA = "Uploads";

UploadManager::UploadManager() throw() : running(0), extra(0), lastGrant(0), mUploadLimit(0), 
	mBytesSent(0), mBytesSpokenFor(0), mCycleTime(0), mByteSlice(0), mThrottleEnable(BOOLSETTING(THROTTLE_ENABLE)), 
	m_boLastTickHighSpeed(false), m_iHighSpeedStartTick(0), boFireballSent(false), boFileServerSent(false) {	
	ClientManager::getInstance()->addListener(this);
	TimerManager::getInstance()->addListener(this);
	throttleZeroCounters();
};

UploadManager::~UploadManager() throw() {
	TimerManager::getInstance()->removeListener(this);
	ClientManager::getInstance()->removeListener(this);
	while(true) {
		{
			Lock l(cs);
			if(uploads.empty())
				break;
		}
		Thread::sleep(100);
	}
	{
		Lock l(cs);
		for(UploadQueueItem::UserMapIter ii = UploadQueueItems.begin(); ii != UploadQueueItems.end(); ++ii) {
			for_each(ii->second.begin(), ii->second.end(), DeleteFunction<UploadQueueItem*>());
		}
		UploadQueueItems.clear();
	}
}

bool UploadManager::prepareFile(UserConnection* aSource, const string& aType, const string& aFile, int64_t aStartPos, int64_t aBytes, bool listRecursive) {
	if(aSource->getState() != UserConnection::STATE_GET) {
		dcdebug("UM:prepFile Wrong state, ignoring\n");
		return false;
	}
	
	dcassert(aFile.size() > 0);
	
	InputStream* is = NULL;
	int64_t size = 0;

	bool userlist = false;
	bool free = false;
	bool leaves = false;
	bool partList = false;

	string file;
	try {
		if(aType == "file") {
			file = ShareManager::getInstance()->translateFileName(aFile);
			userlist = (Util::stricmp(aFile.c_str(), "files.xml.bz2") == 0);

			try {
				File* f = new File(file, File::READ, File::OPEN);
				size = f->getSize();

				free = userlist || (size <= (int64_t)(SETTING(SMALL_FILE_SIZE) * 1024));
	
				if(aBytes == -1) {
					aBytes = size - aStartPos;
				}

				if((aBytes < 0) || ((aStartPos + aBytes) > size)) {
					aSource->fileNotAvail();
					delete f;
					return false;
				}

				f->setPos(aStartPos);
			
				is = f;

				if((aStartPos + aBytes) < size) {
					is = new LimitedInputStream<true>(is, aBytes);
				}	

			} catch(const Exception&) {
				aSource->fileNotAvail();
				return false;
			}

		} else if(aType == "tthl") {
			// TTH Leaves...
			MemoryInputStream* mis = ShareManager::getInstance()->getTree(aFile);
			if(mis == NULL) {
				aSource->fileNotAvail();
				return false;
			}

			size = mis->getSize();
			aStartPos = 0;
			is = mis;
			leaves = true;		
			free = true;
		} else if(aType == "list") {
			// Partial file list
			MemoryInputStream* mis = ShareManager::getInstance()->generatePartialList(aFile, listRecursive);
			if(mis == NULL) {
				aSource->fileNotAvail();
				return false;
			}
			size = mis->getSize();
			aStartPos = 0;
			is = mis;
			free = true;
			partList = true;
		} else {
			aSource->fileNotAvail();
			return false;
		}
	} catch(const ShareException&) {
		aSource->fileNotAvail();
		return false;
	}


	Lock l(cs);

	bool extraSlot = false;

	if(!aSource->isSet(UserConnection::FLAG_HASSLOT)) {
		bool hasReserved = (reservedSlots.find(aSource->getUser()) != reservedSlots.end());
		bool isFavorite = false;
		if (aSource->getUser()->isFavoriteUser())
			isFavorite = aSource->getUser()->getAutoExtraSlot();

		if(!(hasReserved || isFavorite || getFreeSlots() > 0 || getAutoSlot())) {
			bool supportsFree = aSource->getUser()->isSet(User::DCPLUSPLUS) || aSource->isSet(UserConnection::FLAG_SUPPORTS_MINISLOTS);
			bool allowedFree = aSource->isSet(UserConnection::FLAG_HASEXTRASLOT) || aSource->getUser()->isSet(User::OP) || getFreeExtraSlots() > 0;
			if(free && supportsFree && allowedFree) {
				extraSlot = true;
			} else {
				delete is;
				aSource->maxedOut();
				addFailedUpload(aSource->getUser(), file, aStartPos, File::getSize(file));
				aSource->disconnect();
				return false;
			}
		}
	
		setLastGrant(GET_TICK());
	}
	clearUserFiles(aSource->getUser());

	Upload* u = new Upload();
	u->setUserConnection(aSource);
	u->setFile(is);
	u->setSize(size);
	u->setStartPos(aStartPos);
	u->setFileName(file);
	u->setLocalFileName(file);

	if(userlist)
		u->setFlag(Upload::FLAG_USER_LIST);
	if(leaves)
		u->setFlag(Upload::FLAG_TTH_LEAVES);
	if(partList)
		u->setFlag(Upload::FLAG_PARTIAL_LIST);

	dcassert(aSource->getUpload() == NULL);
	aSource->setUpload(u);
	uploads.push_back(u);
	throttleSetup();
	
	if(!aSource->isSet(UserConnection::FLAG_HASSLOT)) {
		if(extraSlot) {
				if(!aSource->isSet(UserConnection::FLAG_HASEXTRASLOT)) {
					aSource->setFlag(UserConnection::FLAG_HASEXTRASLOT);
					extra++;
				}
			} else {
			if(aSource->isSet(UserConnection::FLAG_HASEXTRASLOT)) {
				aSource->unsetFlag(UserConnection::FLAG_HASEXTRASLOT);
				extra--;
		}
			aSource->setFlag(UserConnection::FLAG_HASSLOT);
			running++;
		}
	}
	
	return true;
}

void UploadManager::on(UserConnectionListener::Get, UserConnection* aSource, const string& aFile, int64_t aResume, int64_t bytes) throw() {
	if(prepareFile(aSource, "file", Util::toAdcFile(aFile), aResume, bytes)) {
		aSource->setState(UserConnection::STATE_SEND);
		aSource->fileLength(Util::toString(aSource->getUpload()->getSize()));
	}
}

void UploadManager::onGetBlock(UserConnection* aSource, const string& aFile, int64_t aStartPos, int64_t aBytes, bool z) {
	if(!z || BOOLSETTING(COMPRESS_TRANSFERS)) {
		if(prepareFile(aSource, "file", Util::toAdcFile(aFile), aStartPos, aBytes)) {
			Upload* u = aSource->getUpload();
			dcassert(u != NULL);
			if(aBytes == -1)
				aBytes = u->getSize() - aStartPos;

			dcassert(aBytes >= 0);
		
			u->setStart(GET_TICK());

			if(z) {
				u->setFile(new FilteredInputStream<ZFilter, true>(u->getFile()));
			u->setFlag(Upload::FLAG_ZUPLOAD);
			}

			aSource->sending(aBytes);
			aSource->setState(UserConnection::STATE_DONE);
			aSource->transmitFile(u->getFile());
			fire(UploadManagerListener::Starting(), u);
		}
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
	aSource->setState(UserConnection::STATE_DONE);
	aSource->transmitFile(u->getFile());
	fire(UploadManagerListener::Starting(), u);
}

void UploadManager::on(UserConnectionListener::BytesSent, UserConnection* aSource, size_t aBytes, size_t aActual) throw() {
	dcassert(aSource->getState() == UserConnection::STATE_DONE);
	Upload* u = aSource->getUpload();
	dcassert(u != NULL);
	u->addPos(aBytes, aActual);
	throttleBytesTransferred(aActual);
}

void UploadManager::on(UserConnectionListener::Failed, UserConnection* aSource, const string& aError) throw() {
	Upload* u = aSource->getUpload();

	if(u) {
		aSource->setUpload(NULL);
		fire(UploadManagerListener::Failed(), u, aError);

		dcdebug("UM::onFailed: Removing upload\n");
		removeUpload(u);
	}

	removeConnection(aSource, false);
}

void UploadManager::on(UserConnectionListener::TransmitDone, UserConnection* aSource) throw() {
	dcassert(aSource->getState() == UserConnection::STATE_DONE);
	Upload* u = aSource->getUpload();
	dcassert(u != NULL);

	aSource->setUpload(NULL);
	aSource->setState(UserConnection::STATE_GET);

	if(BOOLSETTING(LOG_UPLOADS) && (BOOLSETTING(LOG_FILELIST_TRANSFERS) || !u->isSet(Upload::FLAG_USER_LIST)) && 
		!u->isSet(Upload::FLAG_TTH_LEAVES)) {
		StringMap params;
		params["source"] = u->getFileName();
		params["user"] = aSource->getUser()->getNick();
		params["userip"] = aSource->getRemoteIp();
		params["hub"] = aSource->getUser()->getLastHubName();
		params["hubip"] = aSource->getUser()->getLastHubAddress();
		params["size"] = Util::toString(u->getSize());
		params["sizeshort"] = Util::formatBytes(u->getSize());
		params["chunksize"] = Util::toString(u->getTotal());
		params["chunksizeshort"] = Util::formatBytes(u->getTotal());
		params["actualsize"] = Util::toString(u->getActual());
		params["actualsizeshort"] = Util::formatBytes(u->getActual());
		params["speed"] = Util::formatBytes(u->getAverageSpeed()) + "/s";
		params["time"] = Util::formatSeconds((GET_TICK() - u->getStart()) / 1000);

		if(u->getTTH() != NULL) {
			params["tth"] = u->getTTH()->toBase32();
		}
		LOG(LogManager::UPLOAD, params);
	}

	fire(UploadManagerListener::Complete(), u);
	removeUpload(u);
}

void UploadManager::addFailedUpload(User::Ptr User, string file, int64_t pos, int64_t size) {
	string path = Util::getFilePath(file);
	string filename = Util::getFileName(file);
	int64_t itime = GET_TIME();
	bool found = false;
	UploadQueueItem::UserMapIter j = UploadQueueItems.find(User);
	if(j != UploadQueueItems.end()) {
		for(UploadQueueItem::Iter i = j->second.begin(); i != j->second.end(); ++i) {
			if((*i)->File == file) {
				(*i)->pos = pos;
				found = true;
				break;
			}
		}
	}
	if(found == false) {
		UploadQueueItem* qi = new UploadQueueItem(User, file, path, filename, pos, size, itime);
		{
			UploadQueueItem::UserMapIter i = UploadQueueItems.find(User);
			if(i == UploadQueueItems.end()) {
				UploadQueueItem::List l;
				l.push_back(qi);
				UploadQueueItems.insert(make_pair(User, l));
			} else {
				i->second.push_back(qi);
			}
		}
		fire(UploadManagerListener::QueueAdd(), qi);
	}
}

void UploadManager::clearUserFiles(const User::Ptr& source) {
	UploadQueueItem::UserMapIter ii = UploadQueueItems.find(source);
	if(ii != UploadQueueItems.end()) {
		for(UploadQueueItem::Iter i = ii->second.begin(); i != ii->second.end(); ++i) {
			fire(UploadManagerListener::QueueItemRemove(), (*i));
			*i = NULL;
			delete *i;
		}
		UploadQueueItems.erase(ii);
		fire(UploadManagerListener::QueueRemove(), source);
	}
}

void UploadManager::removeConnection(UserConnection::Ptr aConn, bool ntd) {
	dcassert(aConn->getUpload() == NULL);
	aConn->removeListener(this);
	if(aConn->isSet(UserConnection::FLAG_HASSLOT)) {
			running--;
		aConn->unsetFlag(UserConnection::FLAG_HASSLOT);
	} 
	if(aConn->isSet(UserConnection::FLAG_HASEXTRASLOT)) {
		extra--;
		aConn->unsetFlag(UserConnection::FLAG_HASEXTRASLOT);
	}
	ConnectionManager::getInstance()->putUploadConnection(aConn, ntd);
}

void UploadManager::on(TimerManagerListener::Minute, u_int32_t aTick) throw() {
	Lock l(cs);
	for(SlotIter j = reservedSlots.begin(); j != reservedSlots.end();) {
		if(j->second < aTick) {
			reservedSlots.erase(j++);
		} else {
			++j;
		}
	}
}	

void UploadManager::on(GetListLength, UserConnection* conn) throw() { 
	conn->listLen(ShareManager::getInstance()->getListLenString()); 
}

void UploadManager::on(AdcCommand::NTD, UserConnection* aConn, const AdcCommand&) throw() {
	removeConnection(aConn, true);
}

void UploadManager::on(AdcCommand::GET, UserConnection* aSource, const AdcCommand& c) throw() {
	int64_t aBytes = Util::toInt64(c.getParam(3));
	int64_t aStartPos = Util::toInt64(c.getParam(2));
	const string& fname = c.getParam(1);
	const string& type = c.getParam(0);
	string tmp;

	if(prepareFile(aSource, type, fname, aStartPos, aBytes, c.hasFlag("RE", 4))) {
		Upload* u = aSource->getUpload();
		dcassert(u != NULL);
		if(aBytes == -1)
			aBytes = u->getSize() - aStartPos;

		dcassert(aBytes >= 0);

		u->setStart(GET_TICK());

		AdcCommand cmd(AdcCommand::CMD_SND);
		cmd.addParam(c.getParam(0));
		cmd.addParam(c.getParam(1));
		cmd.addParam(c.getParam(2));
		cmd.addParam(Util::toString(aBytes));

		if(c.hasFlag("ZL", 4)) {
			u->setFile(new FilteredInputStream<ZFilter, true>(u->getFile()));
			u->setFlag(Upload::FLAG_ZUPLOAD);
			cmd.addParam("ZL1");
		}

		aSource->send(cmd);
		aSource->setState(UserConnection::STATE_DONE);
		aSource->transmitFile(u->getFile());
		fire(UploadManagerListener::Starting(), u);
	}
}

/** @todo fixme */
void UploadManager::on(AdcCommand::GFI, UserConnection* aSource, const AdcCommand& c) throw() {

}

// TimerManagerListener
void UploadManager::on(TimerManagerListener::Second, u_int32_t) throw() {
	Lock l(cs);
	Upload::List ticks;

	throttleSetup();
	throttleZeroCounters();
	for(Upload::Iter i = uploads.begin(); i != uploads.end(); ++i) {
		ticks.push_back(*i);
	}
			
	if(ticks.size() > 0)
		fire(UploadManagerListener::Tick(), ticks);

	fire(UploadManagerListener::QueueUpdate());
	int iAvgSpeed = getAverageSpeed();

	if ( iAvgSpeed < 0 ) iAvgSpeed = 0;

#define FILESERVER_UPLOAD 200*1024*1024

	if(m_boFireball == false) {
		if(iAvgSpeed >= 102400) {
			u_int32_t iActTicks = TimerManager::getTick();
			if ( m_boLastTickHighSpeed ) {
				u_int32_t iHighSpeedTicks = 0;
				if ( iActTicks >= m_iHighSpeedStartTick ) 
				iHighSpeedTicks = ( iActTicks - m_iHighSpeedStartTick );
			else
				iHighSpeedTicks = ( iActTicks + 4294967295 - m_iHighSpeedStartTick );

			if ( iHighSpeedTicks > 60*1000 ) {
				m_boFireball = true;
					if(boFireballSent == false) {
						ClientManager::getInstance()->infoUpdated(true);
						boFireballSent = true;
					}
				}
			} else {
				m_iHighSpeedStartTick = TimerManager::getTick();
				m_boLastTickHighSpeed = true;
			}
		} else {
			m_boLastTickHighSpeed = false;
		}

		if(m_boFileServer == false) {
			int64_t iUpload = Socket::getTotalUp();
			int64_t iUpTime = Util::getUptime();
			int64_t iShareSize = ShareManager::getInstance()->getShareSize();
			if((iUpTime > 7200) && 
				(iShareSize > 2147483648) && 
				(iUpload > 209715200)) {
					m_boFileServer = true;
				if(boFireballSent == false && boFileServerSent == false) {
					ClientManager::getInstance()->infoUpdated(true);
					boFileServerSent = true;
				}
			}
		}
	}
}

void UploadManager::on(ClientManagerListener::UserUpdated, const User::Ptr& aUser) throw() {
	Lock l(cs);
	if( (!aUser->isOnline()) && 
		(aUser->isSet(User::QUIT_HUB)) && 
		(BOOLSETTING(AUTO_KICK)) ){

		for(Upload::Iter i = uploads.begin(); i != uploads.end(); ++i) {
			Upload* u = *i;
			if(u->getUser() == aUser) {
				// Oops...adios...
				u->getUserConnection()->disconnect();
				// But let's grant him/her a free slot just in case...
				if (!u->getUserConnection()->isSet(UserConnection::FLAG_HASEXTRASLOT))
					reserveSlot(u->getUser());
				LogManager::getInstance()->message(STRING(DISCONNECTED_USER) + aUser->getFullNick(), true);
			}
		}
	}
	if(aUser->isOnline() == false) {
		clearUserFiles(aUser);
	}
}

size_t UploadManager::throttleGetSlice() {
	if (mThrottleEnable) {
		Lock l(cs);
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

size_t UploadManager::throttleCycleTime() {
	if (mThrottleEnable)
		return mCycleTime;
	return 0;
}

void UploadManager::throttleZeroCounters()  {
	if (mThrottleEnable) {
		Lock l(cs);
		mBytesSpokenFor = 0;
		mBytesSent = 0;
	}
}

void UploadManager::throttleBytesTransferred(u_int32_t i)  {
	Lock l(cs);
	mBytesSent += i;
}

void UploadManager::throttleSetup() {
	Lock l(cs);
	unsigned int num_transfers = getUploadCount();
	mUploadLimit = (SETTING(MAX_UPLOAD_SPEED_LIMIT) * 1024);
	mThrottleEnable = BOOLSETTING(THROTTLE_ENABLE) && (mUploadLimit > 0) && (num_transfers > 0);
	if (mThrottleEnable) {
			if (mUploadLimit <= (INBUFSIZE * 10 * num_transfers)) {
			mByteSlice = mUploadLimit / (5 * num_transfers);
				if (mByteSlice > INBUFSIZE)
					mByteSlice = INBUFSIZE;
				mCycleTime = 1000 / 10;
		}	else {
				mByteSlice = INBUFSIZE;
				mCycleTime = 1000 * INBUFSIZE / mUploadLimit;
			}
		}
}

UploadQueueItem::UserMap UploadManager::getQueue() const {
	return UploadQueueItems;
}

/**
 * @file
 * $Id$
 */