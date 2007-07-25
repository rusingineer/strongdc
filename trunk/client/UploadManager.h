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

#ifndef DCPLUSPLUS_CLIENT_UPLOAD_MANAGER_H
#define DCPLUSPLUS_CLIENT_UPLOAD_MANAGER_H

#include "forward.h"
#include "UserConnectionListener.h"
#include "Singleton.h"
#include "UploadManagerListener.h"
#include "Client.h"
#include "ClientManager.h"
#include "ClientManagerListener.h"
#include "MerkleTree.h"
#include "FastAlloc.h"

class UploadQueueItem : public FastAlloc<UploadQueueItem>, public PointerBase {
public:
	UploadQueueItem(UserPtr u, const string& file, int64_t p, int64_t sz, uint64_t itime) :
		user(u), file(file), pos(p), size(sz), time(itime) { inc(); }
	
	~UploadQueueItem() throw() { }
	
	typedef vector<UploadQueueItem*> List;
	typedef deque<pair<UserPtr, UploadQueueItem::List>> SlotQueue;

	static int compareItems(const UploadQueueItem* a, const UploadQueueItem* b, uint8_t col) {
		switch(col) {
			case COLUMN_TRANSFERRED: return compare(a->pos, b->pos);
			case COLUMN_SIZE: return compare(a->size, b->size);
			case COLUMN_ADDED:
			case COLUMN_WAITING: return compare(a->time, b->time);
			default: return Util::stricmp(a->getText(col), b->getText(col));
		}
		return 0;
	}

	enum {
		COLUMN_FIRST,
		COLUMN_FILE = COLUMN_FIRST,
		COLUMN_PATH,
		COLUMN_NICK,
		COLUMN_HUB,
		COLUMN_TRANSFERRED,
		COLUMN_SIZE,
		COLUMN_ADDED,
		COLUMN_WAITING,
		COLUMN_LAST
	};
		
	const tstring getText(uint8_t col) const;
	int imageIndex() const;

	const string& getFile() const { return file; }
	const UserPtr& getUser() const { return user; }
	int64_t getSize() const { return size; }
	uint64_t getTime() const { return time; }

	GETSET(int64_t, pos, Pos);
private:
	string file;
	int64_t size;
	uint64_t time;
	
	UserPtr user;	
};

class UploadManager : private ClientManagerListener, private UserConnectionListener, public Speaker<UploadManagerListener>, private TimerManagerListener, public Singleton<UploadManager>
{
public:
	
	/** @return Number of uploads. */ 
	size_t getUploadCount() { Lock l(cs); return uploads.size(); }

	/**
	 * @remarks This is only used in the tray icons. Could be used in
	 * MainFrame too.
	 *
	 * @return Running average download speed in Bytes/s
	 */
	int64_t getRunningAverage();
	
	uint8_t getSlots() const { return (uint8_t)(max(SETTING(SLOTS), max(SETTING(HUB_SLOTS),0) * Client::getTotalCounts())); }

	/** @return Number of free slots. */
	uint8_t getFreeSlots() const { return (uint8_t)max((getSlots() - running), 0); }
	
	/** @internal */
	int getFreeExtraSlots() const { return max(SETTING(EXTRA_SLOTS) - getExtra(), 0); }
	
	/** @param aUser Reserve an upload slot for this user and connect. */
	void reserveSlot(const UserPtr& aUser, uint64_t aTime);
	void unreserveSlot(const UserPtr& aUser);
	void clearUserFiles(const UserPtr&);
	const UploadQueueItem::SlotQueue getWaitingUsers();
	bool hasReservedSlot(const UserPtr& aUser) const { return reservedSlots.find(aUser) != reservedSlots.end(); }
	bool isConnecting(const UserPtr& aUser) const { return connectingUsers.find(aUser) != connectingUsers.end(); }

	bool getFireballStatus() const { return isFireball; }
	bool getFileServerStatus() const { return isFileServer; }

	/** @internal */
	void addConnection(UserConnectionPtr conn);
	void removeDelayUpload(const UserPtr& aUser);
	void abortUpload(const string& aFile, bool waiting = true);
	
	// Upload throttling
	size_t throttleGetSlice();
	inline size_t throttleCycleTime() const { 	return mThrottleEnable ? mCycleTime : 0; }
	
	GETSET(uint8_t, extra, Extra);
	GETSET(uint64_t, lastGrant, LastGrant);

private:
	bool isFireball;
	bool isFileServer;
	bool mThrottleEnable;
	uint8_t running;

	uint64_t m_iHighSpeedStartTick;

	size_t mBytesSpokenFor, mUploadLimit, mCycleTime, mByteSlice;

	UploadList uploads;
	UploadList delayUploads;
	CriticalSection cs;
	
	typedef HASH_MAP<UserPtr, uint64_t, User::HashFunction> SlotMap;
	typedef SlotMap::iterator SlotIter;
	SlotMap reservedSlots;
	SlotMap connectingUsers;
	UploadQueueItem::SlotQueue waitingUsers;

	int addFailedUpload(const UserPtr& aUser, const string& file, int64_t pos, int64_t size);
	void notifyQueuedUsers();

	void throttleSetup();

	friend class Singleton<UploadManager>;
	UploadManager() throw();
	~UploadManager() throw();

	bool getAutoSlot();
	void removeConnection(UserConnection* aConn);
	void removeUpload(Upload* aUpload, bool delay = false);
	void logUpload(Upload* u);

	// ClientManagerListener
	void on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) throw();
	
	// TimerManagerListener
	void on(Second, uint64_t aTick) throw();
	void on(Minute, uint64_t aTick) throw();

	// UserConnectionListener
	void on(BytesSent, UserConnection*, size_t, size_t) throw();
	void on(Failed, UserConnection*, const string&) throw();
	void on(Get, UserConnection*, const string&, int64_t) throw();
	void on(Send, UserConnection*) throw();
	void on(GetListLength, UserConnection* conn) throw();
	void on(TransmitDone, UserConnection*) throw();
	
	void on(AdcCommand::GET, UserConnection*, const AdcCommand&) throw();
	void on(AdcCommand::GFI, UserConnection*, const AdcCommand&) throw();

	bool prepareFile(UserConnection& aSource, const string& aType, const string& aFile, int64_t aResume, int64_t& aBytes, bool listRecursive = false);
};

#endif // !defined(UPLOAD_MANAGER_H)

/**
 * @file
 * $Id$
 */
