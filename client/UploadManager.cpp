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

#include "UploadManager.h"
#include "ConnectionManager.h"
#include "LogManager.h"
#include "ShareManager.h"
#include "HubManager.h"
#include "ClientManager.h"
#include "FilteredFile.h"
#include "ZUtils.h"
#include "ResourceManager.h"

#define FIREBALL_LIMIT 100*1024

#define FILESERVER_ONLINE_TIME 7200
#define FILESERVER_SHARE_SIZE 2*1024*1024*1024
#define FILESERVER_UPLOAD 200*1024*1024

// Main fireball flag
bool UploadManager::m_boFireball = false;
bool UploadManager::m_boFireballLast = false;

// Variables for Fireball detecting
bool m_boLastTickHighSpeed = false;
u_int32_t m_iHighSpeedStartTick = 0;
bool boFireballSent = false;

// Main fileserver flag
bool UploadManager::m_boFileServer = false;
bool UploadManager::m_boFileServerLast = false;
bool boFileServerSent = false;

static const string UPLOAD_AREA = "Uploads";

UploadManager::UploadManager() throw() : running(0), extra(0), lastAutoGrant(0) { 
	ClientManager::getInstance()->addListener(this);
	TimerManager::getInstance()->addListener(this);
	mThrottleEnable = BOOLSETTING(THROTTLE_ENABLE);
	mUploadLimit = 0;
	mBytesSpokenFor = 0;
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
}

bool UploadManager::prepareFile(UserConnection* aSource, const string& aFile, int64_t aResume, int64_t aBytes) {
	if(aSource->getState() != UserConnection::STATE_GET) {
		dcdebug("UM:onGet Wrong state, ignoring\n");
		return false;
	}
	
	Upload* u;
	dcassert(aFile.size() > 0);
	
	bool userlist = false;
	bool smallfile = false;
	bool boAutoExtraSlot = false;

	string file;
	try {
		file = ShareManager::getInstance()->translateFileName(aFile);
	} catch(const ShareException&) {
		aSource->error("File Not Available");
		return false;
	}

	if( (Util::stricmp(aFile.c_str(), "MyList.DcLst") == 0) ||
	 (Util::stricmp(aFile.c_str(), "MyList.bz2") == 0) ||
	 (Util::stricmp(aFile.c_str(), "files.xml.bz2") == 0)) {
		userlist = true;
	}

	int64_t sz = File::getSize(file);
	if(sz == -1) {
		aSource->error("File Not Available");
		return false;
	} else if(aResume >= sz) {
		// Can't do...
		aSource->disconnect();
		return false;
	}

	if( sz < (int64_t)(SETTING(SMALL_FILE_SIZE) * 1024) ) {
		smallfile = true;
	}

	if ( aSource->getUser()->isFavoriteUser() )
		boAutoExtraSlot = aSource->getUser()->getAutoExtraSlot();

	cs.enter();
	SlotIter ui = reservedSlots.find(aSource->getUser());

	if( (!aSource->isSet(UserConnection::FLAG_HASSLOT)) && 
		(getFreeSlots()<=0) &&
		(ui == reservedSlots.end()) && !boAutoExtraSlot) 		
	{
		dcdebug("Average speed: %s/s\n", Util::formatBytes(UploadManager::getInstance()->getAverageSpeed()).c_str());
		if( ((getLastAutoGrant() + 30*1000) > GET_TICK()) || (SETTING(MIN_UPLOAD_SPEED) == 0) || ( (SETTING(MIN_UPLOAD_SPEED)*1024) < UploadManager::getInstance()->getAverageSpeed() ) ) {
			if( !(smallfile || userlist) ||
				!(aSource->isSet(UserConnection::FLAG_HASEXTRASLOT) || (getFreeExtraSlots() > 0) || (aSource->getUser()->isSet(User::OP)) ) || 
				!(aSource->getUser()->isSet(User::DCPLUSPLUS) || aSource->isSet(UserConnection::FLAG_SUPPORTS_MINISLOTS)) 
				) 
			{

				cs.leave();
				aSource->maxedOut();
				addFailedUpload(aSource, aFile, aResume, File::getSize(file));
				removeConnection(aSource);
				return false;
			}
		}
		setLastAutoGrant(GET_TICK());
	}

	clearUserFiles(aSource->getUser());

	File* f;
	try {
		f = new File(file, File::READ, File::OPEN);
	} catch(const FileException&) {
		// Why isn't this possible?...let's reload the directory listing...
		if(Util::stricmp(aFile.c_str(), "MyList.DcLst") == 0 || userlist) {
			
			ShareManager::getInstance()->refresh(true, true, true);
			try {
				f = new File(file, File::READ, File::OPEN);
			} catch(const FileException&) {
				// Still not...very strange...?
				cs.leave();
				aSource->error("File Not Available");
				return false;
			}
		} else {
			cs.leave();
			aSource->error("File Not Available");
			return false;
		}
	}
	
	u = new Upload();
	u->setUserConnection(aSource);
	u->setFile(f);
	u->setSize(f->getSize());
	f->setPos(aResume);
	u->setPos(aResume);
	u->setFileName(aFile);
	u->setLocalFileName(file);

	if(aBytes != -1 && aResume + aBytes < f->getSize()) {
		u->setFile(new LimitedInputStream<true>(u->getFile(), aBytes));
	}

	if(smallfile)
		u->setFlag(Upload::FLAG_SMALL_FILE);
	if(userlist)
		u->setFlag(Upload::FLAG_USER_LIST);

	dcassert(aSource->getUpload() == NULL);
	aSource->setUpload(u);
	uploads.push_back(u);
	throttleSetup();
	if(!aSource->isSet(UserConnection::FLAG_HASSLOT)) {
		if(ui != reservedSlots.end()) {
			aSource->setFlag(UserConnection::FLAG_HASSLOT);
			running++;
		} else {
			if( (getFreeSlots() == 0) && (smallfile || userlist)) {
				if(!aSource->isSet(UserConnection::FLAG_HASEXTRASLOT)) {
					extra++;
					aSource->setFlag(UserConnection::FLAG_HASEXTRASLOT);
				}
			} else {
				running++;
				aSource->setFlag(UserConnection::FLAG_HASSLOT);
			}
		}
	}
	
	if(aSource->isSet(UserConnection::FLAG_HASEXTRASLOT) && aSource->isSet(UserConnection::FLAG_HASSLOT)) {
		extra--;
		aSource->unsetFlag(UserConnection::FLAG_HASEXTRASLOT);
	}
	
	cs.leave();
	return true;

}

void UploadManager::onGet(UserConnection* aSource, const string& aFile, int64_t aResume) {
	if(prepareFile(aSource, aFile, aResume, -1)) {
		aSource->setState(UserConnection::STATE_SEND);
		aSource->fileLength(Util::toString(aSource->getUpload()->getSize()));
	}
}

void UploadManager::onGetBlock(UserConnection* aSource, const string& aFile, int64_t aResume, int64_t aBytes, bool z) {
	if(!z || BOOLSETTING(COMPRESS_TRANSFERS)) {
		if(prepareFile(aSource, aFile, aResume, aBytes)) {
			Upload* u = aSource->getUpload();
			dcassert(u != NULL);
			if(aBytes == -1)
				aBytes = aResume - u->getSize();

			if(aBytes < 0 || (u->getPos() + aBytes) > u->getSize()) {
				// Can't do...
				aSource->disconnect();
				return;
			}
		
			u->setStart(GET_TICK());
			if(z) {
				u->setFile(new FilteredInputStream<ZFilter, true>(u->getFile()));
			u->setFlag(Upload::FLAG_ZUPLOAD);
			}

			aSource->sending(aBytes);
			aSource->setState(UserConnection::STATE_DONE);
			aSource->transmitFile(u->getFile());
			fire(UploadManagerListener::STARTING, u);
		}
	}
}

void UploadManager::onSend(UserConnection* aSource) {
	if(aSource->getState() != UserConnection::STATE_SEND) {
		dcdebug("UM::onSend Bad state, ignoring\n");
		return;
	}

	Upload* u = aSource->getUpload();
	dcassert(u != NULL);

	u->setStart(GET_TICK());
	aSource->setState(UserConnection::STATE_DONE);
	aSource->transmitFile(u->getFile());
	fire(UploadManagerListener::STARTING, u);
}

void UploadManager::onBytesSent(UserConnection* aSource, u_int32_t aBytes, u_int32_t aActual) {
	dcassert(aSource->getState() == UserConnection::STATE_DONE);
	Upload* u = aSource->getUpload();
	dcassert(u != NULL);
	u->addPos(aBytes);
	u->addActual(aActual);
	throttleBytesTransferred(aActual);
}

void UploadManager::onFailed(UserConnection* aSource, const string& aError) {
	Upload* u = aSource->getUpload();

	if(u) {
		aSource->setUpload(NULL);
		fire(UploadManagerListener::FAILED, u, aError);

		dcdebug("UM::onFailed: Removing upload\n");
		removeUpload(u);
	}

	removeConnection(aSource);
}

void UploadManager::onTransmitDone(UserConnection* aSource) {
	dcassert(aSource->getState() == UserConnection::STATE_DONE);
	Upload* u = aSource->getUpload();
	dcassert(u != NULL);

	aSource->setUpload(NULL);
	aSource->setState(UserConnection::STATE_GET);

	if(BOOLSETTING(LOG_UPLOADS)  && (BOOLSETTING(LOG_FILELIST_TRANSFERS) || !u->isSet(Upload::FLAG_USER_LIST))) {
		StringMap params;
		params["source"] = u->getFileName();
		params["user"] = aSource->getUser()->getNick();
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
		LOG(UPLOAD_AREA, Util::formatParams(SETTING(LOG_FORMAT_POST_UPLOAD), params));
	}

	fire(UploadManagerListener::COMPLETE, u);
	removeUpload(u);
}

void UploadManager::addFailedUpload(UserConnection::Ptr source, string filename, int64_t pos, int64_t size)
{
	SlotQueue::iterator userPos = find(waitingUsers.begin(), waitingUsers.end(), source->getUser());
	if (userPos == waitingUsers.end()) waitingUsers.push_back(source->getUser());
	string path = Util::getFilePath(filename);
	filename = Util::getFileName(filename);
	waitingFiles[source->getUser()].insert(filename+"|"+path+"|"+Util::toString(pos)+"|"+Util::toString(size)+"|");		//maintain list of files the user's searched for

	fire(UploadManagerListener::QUEUE_ADD_FILE, source->getUser()->getNick(), filename, path, pos, size);
}

void UploadManager::clearUserFiles(const User::Ptr& source)
{
	//run this when a user's got a slot. It clears the list of files he attempted unsuccessfully download, as well as the user himself from the queue

	SlotQueue::iterator sit = find(waitingUsers.begin(), waitingUsers.end(), source);
	if (sit != waitingUsers.end()) waitingUsers.erase(sit);

	FilesMap::iterator fit = waitingFiles.find(source);
	if (fit != waitingFiles.end()) waitingFiles.erase(source);

	fire(UploadManagerListener::QUEUE_REMOVE_USER, source->getNick());
}

void UploadManager::removeConnection(UserConnection::Ptr aConn) {
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
	ConnectionManager::getInstance()->putUploadConnection(aConn);
}

void UploadManager::onTimerMinute(u_int32_t aTick) {
	Lock l(cs);
	for(SlotIter j = reservedSlots.begin(); j != reservedSlots.end();) {
		if(j->second < aTick) {
			reservedSlots.erase(j++);
		} else {
			++j;
		}
	}
}	

// TimerManagerListener
void UploadManager::onAction(TimerManagerListener::Types type, u_int32_t aTick) throw() {
	switch(type) {
	case TimerManagerListener::SECOND: 
		{
			Lock l(cs);
			Upload::List ticks;

			throttleSetup();
			throttleZeroCounters();
			for(Upload::Iter i = uploads.begin(); i != uploads.end(); ++i) {
				ticks.push_back(*i);
			}
			
			if(ticks.size() > 0)
				fire(UploadManagerListener::TICK, ticks);
			int iAvgSpeed = getAverageSpeed();

			if ( iAvgSpeed < 0 ) iAvgSpeed = 0;

			if ( !m_boFireball ) {
			// Musim orezat narazove falesne velke rychlosti
			// - cekat, zda bude nad limit nejaky souvisly cas, treba 10 vterin
			// - pak teprve nahodit bit FireBall
				if ( iAvgSpeed >= FIREBALL_LIMIT ) {
					u_int32_t iActTicks = TimerManager::getTick();
					if ( m_boLastTickHighSpeed ) {
			// Vysoka rychlost uz trva od driv => spocitat, jak dlouho uz trva
						u_int32_t iHighSpeedTicks = 0;
						if ( iActTicks >= m_iHighSpeedStartTick ) 
						iHighSpeedTicks = ( iActTicks - m_iHighSpeedStartTick );
					else
						iHighSpeedTicks = ( iActTicks + 4294967295 - m_iHighSpeedStartTick );
						if ( iHighSpeedTicks > 60*1000 ) {
			// Dele nez 60 vterin
							m_boFireball = true;
						}
					} else {
			// Prvni tick s vysokou rychlosti
			// Odpamatujeme si tick a nahodime flag pocitani casu
						m_iHighSpeedStartTick = TimerManager::getTick();
						m_boLastTickHighSpeed = true;
					}
				} else {
			// Shodim priznak pro pocitani casu
					m_boLastTickHighSpeed = false;
				}
			}

			if ( !m_boFileServer ) {
				int64_t iUpload = Socket::getTotalUp(); // Celkovy upload od spusteni
				int64_t iUpTime = Util::getUptime();
				int64_t iShareSize = ShareManager::getInstance()->getShareSize();
				if ( ( iUpTime > FILESERVER_ONLINE_TIME ) 
				 && ( iShareSize > (int64_t) FILESERVER_SHARE_SIZE ) 
				 && ( iUpload > (int64_t) FILESERVER_UPLOAD ) ) {
					m_boFileServer = true;
				}
			}

			// Pokud vznikne priznak Fireball nebo FileServer, posleme ho jednorazove vsem hubum
			// Nove pripojenym hubum se posle v ramci pripojovani
			if ( m_boFireball && !boFireballSent ) {
				ClientManager::getInstance()->infoUpdated();
				boFireballSent = true;
				boFileServerSent = true; // Zamezime odeslani MyInfo, kdyz FileServer vznikne po FireBall
			}
			else if ( m_boFileServer && !boFileServerSent ) {
				ClientManager::getInstance()->infoUpdated();
				boFileServerSent = true;
			}
		}
		break;
	case TimerManagerListener::MINUTE: onTimerMinute(aTick);	break;
		break;
	}
}

void UploadManager::onAction(ClientManagerListener::Types type, const User::Ptr& aUser) throw() {
	if( (type == ClientManagerListener::USER_UPDATED) && 
		(!aUser->isOnline()) && 
		(aUser->isSet(User::QUIT_HUB)) && 
		(BOOLSETTING(AUTO_KICK)) ){

		Lock l(cs);
		for(Upload::Iter i = uploads.begin(); i != uploads.end(); ++i) {
			Upload* u = *i;
			if(u->getUser() == aUser) {
				// Oops...adios...
				u->getUserConnection()->disconnect();
				// But let's grant him/her a free slot just in case...
				if (!u->getUserConnection()->isSet(UserConnection::FLAG_HASEXTRASLOT))
					reserveSlot(aUser);
				LogManager::getInstance()->message(STRING(DISCONNECTED_USER) + aUser->getFullNick());
			}
		}
		//They shouldn't wait on the upload queue either anymore.
		clearUserFiles(aUser);
	}
}


// UserConnectionListener
void UploadManager::onAction(UserConnectionListener::Types type, UserConnection* conn) throw() {
	switch(type) {
	case UserConnectionListener::TRANSMIT_DONE:
		onTransmitDone(conn); break;
	case UserConnectionListener::SEND:
		onSend(conn); break;
	case UserConnectionListener::GET_LIST_LENGTH:
		conn->listLen(ShareManager::getInstance()->getListLenString()); break;
	default: 
		break;
	}
}
void UploadManager::onAction(UserConnectionListener::Types type, UserConnection* conn, u_int32_t bytes, u_int32_t actual) throw() {
	switch(type) {
	case UserConnectionListener::BYTES_SENT:
		onBytesSent(conn, bytes, actual); break;
	default: 
		break;
	}
}
void UploadManager::onAction(UserConnectionListener::Types type, UserConnection* conn, const string& line) throw() {
	switch(type) {
	case UserConnectionListener::FAILED:
		onFailed(conn, line); break;
	default: 
		break;
	}
}
void UploadManager::onAction(UserConnectionListener::Types type, UserConnection* conn, const string& line, int64_t resume) throw() {
	switch(type) {
	case UserConnectionListener::GET:
		onGet(conn, line, resume); break;
	default: 
		break;
	}
}

void UploadManager::onAction(UserConnectionListener::Types type, UserConnection* conn, const string& line, int64_t resume, int64_t bytes) throw() {
	switch(type) {
	case UserConnectionListener::GET_ZBLOCK:
		onGetBlock(conn, line, resume, bytes, true); break;
	case UserConnectionListener::GET_BLOCK:
		onGetBlock(conn, line, resume, bytes, false); break;
	default: 
		break;
	}
}

void UploadManager::resetFireballStatus() { 
  m_boFireball = false;
  m_boFireballLast = false; 
  boFireballSent = false;
};

void UploadManager::resetFileServerStatus() {
  m_boFileServer = false;
  m_boFileServerLast = false;
  boFileServerSent = false;
};

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
	} else
		return -1;
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
#define INBUFSIZE 64*1024
	Lock l(cs);
	unsigned int num_transfers = getUploads();
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

string UploadManager::getQueue() const {
	string queue;
	for (SlotQueue::const_iterator sit = waitingUsers.begin(); sit != waitingUsers.end(); ++sit)
		queue += (*sit)->getNick() + " ";
	return queue;
}

const UploadManager::SlotQueue& UploadManager::getQueueVec() const {
	return waitingUsers;
}

const UploadManager::FileSet& UploadManager::getQueuedUserFiles(const User::Ptr &u) const {
	return waitingFiles.find(u)->second;
}

/**
 * @file
 * $Id$
 */
