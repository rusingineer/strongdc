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

#if !defined(UPLOAD_MANAGER_H)
#define UPLOAD_MANAGER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "UserConnection.h"
#include "Singleton.h"

#include "Client.h"
#include "ClientManager.h"
#include "ClientManagerListener.h"
#include "MerkleTree.h"
#include "FastAlloc.h"

class InputStream;

class Upload : public Transfer, public Flags {
public:
	enum Flags {
		FLAG_USER_LIST = 0x01,
		FLAG_TTH_LEAVES = 0x02,
		FLAG_ZUPLOAD = 0x04,
		FLAG_PARTIAL_LIST = 0x08,
		FLAG_PENDING_KICK = 0x10,
		FLAG_PARTIAL_SHARE = 0x20,
		FLAG_RESUMED = 0x40,
		FLAG_CHUNKED = 0x80
	};

	typedef Upload* Ptr;
	typedef vector<Ptr> List;
	typedef List::const_iterator Iter;
	
	Upload(UserConnection& conn);
	virtual ~Upload();
	
	virtual void getParams(const UserConnection& aSource, StringMap& params);
	
	GETSET(string, sourceFile, SourceFile);
	GETSET(InputStream*, stream, Stream);
};

class UploadManagerListener {
	friend class UploadQueueItem; 
public:
	virtual ~UploadManagerListener() { }
	template<int I>	struct X { enum { TYPE = I };  };
	
	typedef X<0> Complete;
	typedef X<1> Failed;
	typedef X<2> Starting;
	typedef X<3> Tick;
	typedef X<4> QueueAdd;
	typedef X<5> QueueRemove;
	typedef X<6> QueueItemRemove;
	typedef X<7> QueueUpdate;

	virtual void on(Starting, Upload*) throw() { }
	virtual void on(Tick, const Upload::List&) throw() { }
	virtual void on(Complete, Upload*) throw() { }
	virtual void on(Failed, Upload*, const string&) throw() { }
	virtual void on(QueueAdd, UploadQueueItem*) throw() { }
	virtual void on(QueueRemove, const User::Ptr&) throw() { }
	virtual void on(QueueItemRemove, UploadQueueItem*) throw() { }
	virtual void on(QueueUpdate) throw() { }

};

class UploadQueueItem : public FastAlloc<UploadQueueItem>, public PointerBase, public ColumnBase {
public:
	UploadQueueItem(User::Ptr u, string file, int64_t p, int64_t sz, uint32_t itime) :
		User(u), File(file), pos(p), size(sz), iTime(itime), icon(0) { inc(); }
	virtual ~UploadQueueItem() throw() { }
	typedef UploadQueueItem* Ptr;
	typedef vector<Ptr> List;
	typedef List::const_iterator Iter;
	typedef HASH_MAP<User::Ptr, UploadQueueItem::List, User::HashFunction> UserMap;
	typedef UserMap::const_iterator UserMapIter;

	static int compareItems(const UploadQueueItem* a, const UploadQueueItem* b, int col) {
		switch(col) {
			case COLUMN_FILE: return Util::stricmp(a->getText(COLUMN_FILE), b->getText(COLUMN_FILE));
			case COLUMN_PATH: return Util::stricmp(a->getText(COLUMN_PATH), b->getText(COLUMN_PATH));
			case COLUMN_NICK: return Util::stricmp(a->getText(COLUMN_NICK), b->getText(COLUMN_NICK));
			case COLUMN_HUB: return Util::stricmp(a->getText(COLUMN_HUB), b->getText(COLUMN_HUB));
			case COLUMN_TRANSFERRED: return compare(a->pos, b->pos);
			case COLUMN_SIZE: return compare(a->size, b->size);
			case COLUMN_ADDED: return compare(a->iTime, b->iTime);
			case COLUMN_WAITING: return compare(a->iTime, b->iTime);
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
		
	int imageIndex() const { return icon; }
	void update();

	User::Ptr User;
	string File;
	int64_t pos;
	int64_t size;
	uint32_t iTime;
	int icon;
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
	
	int getSlots() const {
		int slots = 0;
		if (SETTING(HUB_SLOTS) * Client::getTotalCounts() <= SETTING(SLOTS)) {
			slots = SETTING(SLOTS);
		} else {
			slots = max(SETTING(HUB_SLOTS),0) * (Client::getTotalCounts());
		}
		return slots;
	}

	/** @return Number of free slots. */
	int getFreeSlots() const { return max((getSlots() - running), 0); }
	
	/** @internal */
	int getFreeExtraSlots() const { return max(SETTING(EXTRA_SLOTS) - getExtra(), 0); }
	
	/** @param aUser Reserve an upload slot for this user and connect. */
	void reserveSlot(const User::Ptr& aUser, uint32_t aTime);
	void unreserveSlot(const User::Ptr& aUser);
	void clearUserFiles(const User::Ptr&);
	UploadQueueItem::UserMap getWaitingUsers();
	bool getFireballStatus() const { return isFireball; }
	bool getFileServerStatus() const { return isFileServer; }
	bool hasReservedSlot(const User::Ptr& aUser) const { return reservedSlots.find(aUser) != reservedSlots.end(); }

	/** @internal */
	void addConnection(UserConnection::Ptr conn) {
		conn->addListener(this);
		conn->setState(UserConnection::STATE_GET);
	}
	void removeDelayUpload(const User::Ptr& aUser) {
		Lock l(cs);
		for(Upload::List::iterator i = delayUploads.begin(); i != delayUploads.end(); ++i) {
			Upload* up = *i;
			if(aUser == up->getUser()) {
				delayUploads.erase(i);
				delete up;
				break;
			}
		}		
	}

	void abortUpload(const string& aFile, bool waiting = true);
	
	// Upload throttling
	size_t throttleGetSlice();
	size_t throttleCycleTime() const;
	
	GETSET(uint16_t, running, Running);
	GETSET(uint16_t, extra, Extra);
	GETSET(uint32_t, lastGrant, LastGrant);
private:
	Upload::List uploads;
	Upload::List delayUploads;
	CriticalSection cs;
	
	typedef HASH_MAP<User::Ptr, uint32_t, User::HashFunction> SlotMap;
	typedef SlotMap::iterator SlotIter;
	SlotMap reservedSlots;
	
	UploadQueueItem::UserMap waitingUsers;
	void addFailedUpload(const User::Ptr& User, string file, int64_t pos, int64_t size);
	
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

	// ClientManagerListener
	virtual void on(ClientManagerListener::UserDisconnected, const User::Ptr& aUser) throw();
	
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
