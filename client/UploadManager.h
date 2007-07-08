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
	typedef List::const_iterator Iter;
	
	typedef HASH_MAP<UserPtr, UploadQueueItem::List, User::HashFunction> UserMap;
	typedef UserMap::const_iterator UserMapIter;

	static int compareItems(const UploadQueueItem* a, const UploadQueueItem* b, uint8_t col) {
		switch(col) {
			case COLUMN_FILE: return Util::stricmp(a->columns[COLUMN_FILE], b->columns[COLUMN_FILE]);
			case COLUMN_PATH: return Util::stricmp(a->columns[COLUMN_PATH], b->columns[COLUMN_PATH]);
			case COLUMN_NICK: return Util::stricmp(a->columns[COLUMN_NICK], b->columns[COLUMN_NICK]);
			case COLUMN_HUB: return Util::stricmp(a->columns[COLUMN_HUB], b->columns[COLUMN_HUB]);
			case COLUMN_TRANSFERRED: return compare(a->pos, b->pos);
			case COLUMN_SIZE: return compare(a->size, b->size);
			case COLUMN_ADDED:
			case COLUMN_WAITING: return compare(a->time, b->time);
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
		
	int imageIndex() const;
	void update(bool onSecond = false);

	inline const tstring& getText(uint8_t col) const { return columns[col]; }

	const string& getFile() const { return file; }
	const UserPtr& getUser() const { return user; }
	int64_t getSize() const { return size; }
	uint64_t getTime() const { return time; }

	GETSET(int64_t, pos, Pos);

private:
	tstring columns[COLUMN_LAST];

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
	void reserveSlot(const UserPtr& aUser, uint32_t aTime);
	void unreserveSlot(const UserPtr& aUser);
	void clearUserFiles(const UserPtr&);
	const UploadQueueItem::UserMap getWaitingFiles();
	bool getFireballStatus() const { return isFireball; }
	bool getFileServerStatus() const { return isFileServer; }
	bool hasReservedSlot(const UserPtr& aUser) const { return reservedSlots.find(aUser) != reservedSlots.end(); }
	bool isConnecting(const UserPtr& aUser) const { return connectingUsers.find(aUser) != connectingUsers.end(); }

	/** @internal */
	void addConnection(UserConnectionPtr conn);
	void removeDelayUpload(const UserPtr& aUser);
	void abortUpload(const string& aFile, bool waiting = true);
	
	// Upload throttling
	size_t throttleGetSlice();
	size_t throttleCycleTime() const;
	
	uint8_t running;
	GETSET(uint8_t, extra, Extra);
	GETSET(uint64_t, lastGrant, LastGrant);

private:
	UploadList uploads;
	UploadList delayUploads;
	CriticalSection cs;
	
	typedef HASH_MAP<UserPtr, uint64_t, User::HashFunction> SlotMap;
	typedef SlotMap::iterator SlotIter;
	SlotMap reservedSlots;
	SlotMap connectingUsers;

	typedef deque<UserPtr> SlotQueue;
	SlotQueue waitingUsers;

	UploadQueueItem::UserMap waitingFiles;
	void addFailedUpload(const UserPtr& User, const string& file, int64_t pos, int64_t size);
	
	void throttleBytesTransferred(uint32_t i);
	void throttleSetup();
	bool mThrottleEnable;
	size_t mBytesSent,
		   mBytesSpokenFor,
		   mUploadLimit,
		   mCycleTime,
		   mByteSlice;
	
	// Variables for Fireball and Fileserver detecting
	bool isFireball;
	bool isFileServer;
	uint32_t m_iHighSpeedStartTick;
	
	friend class Singleton<UploadManager>;
	UploadManager() throw();
	virtual ~UploadManager() throw();

	bool getAutoSlot();
	void removeConnection(UserConnection* aConn);
	void removeUpload(Upload* aUpload, bool delay = false);
	void logUpload(Upload* u);

	void notifyQueuedUsers();

	// ClientManagerListener
	virtual void on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) throw();
	
	// TimerManagerListener
	virtual void on(Second, uint32_t aTick) throw();
	virtual void on(Minute, uint32_t aTick) throw();

	// UserConnectionListener
	virtual void on(BytesSent, UserConnection*, size_t, size_t) throw();
	virtual void on(Failed, UserConnection*, const string&) throw();
	virtual void on(Get, UserConnection*, const string&, int64_t) throw();
	virtual void on(Send, UserConnection*) throw();
	virtual void on(GetListLength, UserConnection* conn) throw();
	virtual void on(TransmitDone, UserConnection*) throw();
	
	virtual void on(AdcCommand::GET, UserConnection*, const AdcCommand&) throw();
	virtual void on(AdcCommand::GFI, UserConnection*, const AdcCommand&) throw();

	bool prepareFile(UserConnection& aSource, const string& aType, const string& aFile, int64_t aResume, int64_t& aBytes, bool listRecursive = false);
};

#endif // !defined(UPLOAD_MANAGER_H)

/**
 * @file
 * $Id$
 */
