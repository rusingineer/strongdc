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

#if !defined(AFX_UPLOADMANAGER_H__B0C67119_3445_4208_B5AA_938D4A019703__INCLUDED_)
#define AFX_UPLOADMANAGER_H__B0C67119_3445_4208_B5AA_938D4A019703__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "UserConnection.h"
#include "Singleton.h"

#include "Client.h"
#include "ClientManagerListener.h"
#include "File.h"
#include "MerkleTree.h"
#include "FastAlloc.h"

class Upload : public Transfer, public Flags {
public:
	enum Flags {
		FLAG_USER_LIST = 0x01,
		FLAG_TTH_LEAVES = 0x02,
		FLAG_ZUPLOAD = 0x04,
		FLAG_PARTIAL_LIST = 0x08
	};

	typedef Upload* Ptr;
	typedef vector<Ptr> List;
	typedef List::iterator Iter;
	
	Upload() : tth(NULL), file(NULL) { };
	virtual ~Upload() { 
		delete file;
		delete tth;
	};

	User::Ptr& getUser() { dcassert(getUserConnection() != NULL); return getUserConnection()->getUser(); };
	
	GETSET(string, fileName, FileName);
	GETSET(string, localFileName, LocalFileName);
	GETSET(TTHValue*, tth, TTH);
	GETSET(InputStream*, file, File);
};

class UploadManagerListener {
	friend class UploadQueueItem; 
public:
	template<int I>	struct X { enum { TYPE = I };  };

	typedef X<0> Complete;
	typedef X<1> Failed;
	typedef X<2> Starting;
	typedef X<3> Tick;
	typedef X<4> QueueAdd;
	typedef X<5> QueueRemove;
	typedef X<6> QueueItemRemove;
	typedef X<7> QueueUpdate;

	virtual void on(Starting, Upload*) throw() { };
	virtual void on(Tick, const Upload::List&) throw() { };
	virtual void on(Complete, Upload*) throw() { };
	virtual void on(Failed, Upload*, const string&) throw() { };
	virtual void on(QueueAdd, UploadQueueItem*) throw() { };
	virtual void on(QueueRemove, const User::Ptr&) throw() { };
	virtual void on(QueueItemRemove, UploadQueueItem*) throw() { };
	virtual void on(QueueUpdate) throw() { };

};

class UploadQueueItem : public FastAlloc<UploadQueueItem> {
	public:
		UploadQueueItem(User::Ptr u, string file, string path, string filename, int64_t p, int64_t sz, int64_t itime) :
			User(u), File(file), Path(path), FileName(filename), pos(p), size(sz), iTime(itime) { };
		virtual ~UploadQueueItem() throw() { };
		typedef UploadQueueItem* Ptr;
		typedef vector<Ptr> List;
		typedef List::iterator Iter;
		typedef HASH_MAP<User::Ptr, UploadQueueItem::List, User::HashFunction> UserMap;
		typedef UserMap::iterator UserMapIter;

		const tstring& getText(int col) const {
			return columns[col];
		}
		static int compareItems(UploadQueueItem* a, UploadQueueItem* b, int col) {
			switch(col) {
				case COLUMN_FILE: return Util::stricmp(a->FileName, b->FileName);
				case COLUMN_PATH: return Util::stricmp(a->Path, b->Path);
				case COLUMN_NICK: return Util::stricmp(a->User->getNick(), b->User->getNick());
				case COLUMN_HUB: return Util::stricmp(a->User->getLastHubName(), b->User->getLastHubName());
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

		void update();

		User::Ptr User;
		string File;
		string Path;
		string FileName;
		int64_t pos;
		int64_t size;
		int64_t iTime;
		tstring columns[COLUMN_LAST];
};

class UploadManager : private ClientManagerListener, private UserConnectionListener, public Speaker<UploadManagerListener>, private TimerManagerListener, public Singleton<UploadManager>
{
public:

	/** @return Number of uploads. */ 
	size_t getUploadCount() { Lock l(cs); return uploads.size(); };

	/**
	 * @remarks This is only used in the tray icons. Could be used in
	 * MainFrame too.
	 *
	 * @return Average download speed in Bytes/s
	 */
	int getAverageSpeed() {
		Lock l(cs);
		int avg = 0;
		for(Upload::Iter i = uploads.begin(); i != uploads.end(); ++i) {
			Upload* u = *i;
			avg += (int)u->getRunningAverage();
		}
		return avg;
	}
	
	int getSlots() {
		//return SETTING(SLOTS) + (max(SETTING(HUB_SLOTS),0) * (Client::getTotalCounts() - 1)) ;
		int slots = 0;
		if (SETTING(HUB_SLOTS) * Client::getTotalCounts() <= SETTING(SLOTS)) {
			slots = SETTING(SLOTS);
		} else {
			slots = max(SETTING(HUB_SLOTS),0) * (Client::getTotalCounts());
		}
		return slots;
	}

	/** @return Number of free slots. */
	int getFreeSlots() { return max((getSlots() - running), 0); }

	/** @internal */
	bool getAutoSlot() {
		/** A 0 in settings means disable */
		if(SETTING(MIN_UPLOAD_SPEED) == 0)
			return false;
		/** Only grant one slot per 30 sec */
		if(GET_TICK() < getLastGrant() + 30*1000)
			return false;
		/** Grant if uploadspeed is less than the threshold speed */
		return UploadManager::getInstance()->getAverageSpeed() < (SETTING(MIN_UPLOAD_SPEED)*1024);
	}

	/** @internal */
	int getFreeExtraSlots()	{ return max(SETTING(EXTRA_SLOTS) - getExtra(), 0); };
		
	/** @param aUser Reserve an upload slot for this user and connect. */
	void reserveSlot(User::Ptr& aUser) {
		{
			Lock l(cs);
			reservedSlots[aUser] = GET_TICK() + 600*1000;
		}
		if(aUser->isOnline())
			aUser->connect();	
	}

	/** @param aUser Reserve an upload slot for this user. */
	void reserveSlot(const User::Ptr& aUser) {
		{
			Lock l(cs);
			reservedSlots[aUser] = GET_TICK();
		}
	}
	
	void reserveSlotHour(User::Ptr& aUser) {
		{
			Lock l(cs);
			reservedSlots[aUser] = GET_TICK() + 3600*1000;
		}
		if(aUser->isOnline())
			aUser->connect();				
	}

	void reserveSlotDay(User::Ptr& aUser) {
		{
			Lock l(cs);
			reservedSlots[aUser] = GET_TICK() + 24*3600*1000;
		}
		if(aUser->isOnline())
			aUser->connect();	
	}

	void reserveSlotWeek(User::Ptr& aUser) {
		{
			Lock l(cs);
			reservedSlots[aUser] = GET_TICK() + 7*24*3600*1000;
		}
		if(aUser->isOnline())
			aUser->connect();	
}

	void unreserveSlot(const User::Ptr& aUser) {
		SlotIter uis = reservedSlots.find(aUser);
		if(uis != reservedSlots.end())
			reservedSlots.erase(uis);
	}

	/** @internal */
	void addConnection(UserConnection::Ptr conn) {
		conn->addListener(this);
		conn->setState(UserConnection::STATE_GET);
	}

	GETSET(int, running, Running);
	GETSET(int, extra, Extra);
	GETSET(u_int32_t, lastGrant, LastGrant);

	// Upload throttling
	size_t throttleGetSlice();
	size_t throttleCycleTime();

	UploadQueueItem::UserMap getQueue() const;
	void clearUserFiles(const User::Ptr&);
	UploadQueueItem::UserMap UploadQueueItems;
	static bool getFireballStatus() { return m_boFireball; };
	static bool getFileServerStatus() { return m_boFileServer; };
	bool hasReservedSlot(const User::Ptr& aUser) { return reservedSlots.find(aUser) != reservedSlots.end(); }
private:
	void throttleZeroCounters();
	void throttleBytesTransferred(u_int32_t i);
	void throttleSetup();
	bool mThrottleEnable;
	size_t mBytesSent,
		   mBytesSpokenFor,
		   mUploadLimit,
		   mCycleTime,
		   mByteSlice;

	static bool m_boFireball;
	static bool m_boFileServer;
	
	// Variables for Fireball detecting
	bool m_boLastTickHighSpeed;
	u_int32_t m_iHighSpeedStartTick;
	bool boFireballSent;

	// Main fileserver flag
	bool boFileServerSent;

	
	Upload::List uploads;
	CriticalSection cs;

	typedef HASH_MAP<User::Ptr, u_int32_t, User::HashFunction> SlotMap;
	typedef SlotMap::iterator SlotIter;
	SlotMap reservedSlots;

	void addFailedUpload(User::Ptr& User, string file, int64_t pos, int64_t size);
	
	friend class Singleton<UploadManager>;
	UploadManager() throw();
	virtual ~UploadManager() throw();

	void removeConnection(UserConnection::Ptr aConn, bool ntd);
	void removeUpload(Upload* aUpload) {
		Lock l(cs);
		dcassert(find(uploads.begin(), uploads.end(), aUpload) != uploads.end());
		uploads.erase(find(uploads.begin(), uploads.end(), aUpload));
		throttleSetup();
		aUpload->setUserConnection(NULL);
		delete aUpload;
	}

	// ClientManagerListener
	virtual void on(ClientManagerListener::UserUpdated, const User::Ptr& aUser) throw();
	
	// TimerManagerListener
	virtual void on(TimerManagerListener::Minute, u_int32_t aTick) throw();
	virtual void on(TimerManagerListener::Second, u_int32_t aTick) throw();

	// UserConnectionListener
	virtual void on(BytesSent, UserConnection*, size_t, size_t) throw();
	virtual void on(Failed, UserConnection*, const string&) throw();
	virtual void on(Get, UserConnection*, const string&, int64_t, int64_t) throw();
	virtual void on(GetBlock, UserConnection* conn, const string& line, int64_t resume, int64_t bytes) throw() { onGetBlock(conn, line, resume, bytes, false); }
	virtual void on(GetZBlock, UserConnection* conn, const string& line, int64_t resume, int64_t bytes) throw() { onGetBlock(conn, line, resume, bytes, true); }
	virtual void on(Send, UserConnection*) throw();
	virtual void on(GetListLength, UserConnection* conn) throw();
	virtual void on(TransmitDone, UserConnection*) throw();

	virtual void on(AdcCommand::GET, UserConnection*, const AdcCommand&) throw();
	virtual void on(AdcCommand::GFI, UserConnection*, const AdcCommand&) throw();
	virtual void on(AdcCommand::NTD, UserConnection*, const AdcCommand&) throw();

	void onGetBlock(UserConnection* aSource, const string& aFile, int64_t aResume, int64_t aBytes, bool z);
	bool prepareFile(UserConnection* aSource, const string& aType, const string& aFile, int64_t aResume, int64_t aBytes, bool listRecursive = false);
};

#endif // !defined(AFX_UPLOADMANAGER_H__B0C67119_3445_4208_B5AA_938D4A019703__INCLUDED_)

/**
 * @file
 * $Id$
 */
