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

class Upload : public Transfer, public Flags {
public:
	enum Flags {
		FLAG_USER_LIST = 0x01,
		FLAG_TTH_LEAVES = 0x02,
		FLAG_ZUPLOAD = 0x04
	};

	typedef Upload* Ptr;
	typedef vector<Ptr> List;
	typedef List::iterator Iter;
	
	Upload() : file(NULL) { };
	virtual ~Upload() { 
		delete file;
	};

	User::Ptr& getUser() { dcassert(getUserConnection() != NULL); return getUserConnection()->getUser(); };
	
	GETSET(string, fileName, FileName);
	GETSET(string, localFileName, LocalFileName);
	GETSET(InputStream*, file, File);
};

class UploadManagerListener {
public:
	template<int I>	struct X { enum { TYPE = I };  };

	typedef X<0> Complete;
	typedef X<1> Failed;
	typedef X<2> Starting;
	typedef X<3> Tick;
	typedef X<4> QueueAdd;
	typedef X<5> QueueRemove;

	virtual void on(Starting, Upload*) throw() { };
	virtual void on(Tick, const Upload::List&) throw() { };
	virtual void on(Complete, Upload*) throw() { };
	virtual void on(Failed, Upload*, const string&) throw() { };
	virtual void on(QueueAdd, const string&, const string &, const string &, const int64_t, const int64_t) throw() { };
	virtual void on(QueueRemove, const string&) throw() { };

};

class UploadManager : private ClientManagerListener, private UserConnectionListener, public Speaker<UploadManagerListener>, private TimerManagerListener, public Singleton<UploadManager>
{
public:
	static bool m_boFireball;
	static bool m_boFireballLast;
	static bool m_boFileServer;
	static bool m_boFileServerLast;
	static bool getFireballStatus() { return m_boFireball; };
	static bool getFileServerStatus() { return m_boFileServer; };
	static void resetFireballStatus();
	static void resetFileServerStatus();
	
	int getUploads() { Lock l(cs); return uploads.size(); };
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
		return SETTING(SLOTS) + (max(SETTING(HUB_SLOTS),0) * (Client::getTotalCounts() - 1)) ;
	}
	int getRunning() { return running; };
	int getFreeSlots() {return max((getSlots() - running), 0); }
	bool getAutoSlot() {
		if(SETTING(MIN_UPLOAD_SPEED) == 0)
			return false;
		if(getLastGrant() + 30*1000 < GET_TICK())
			return false;
		return (SETTING(MIN_UPLOAD_SPEED)*1024) < UploadManager::getInstance()->getAverageSpeed();
	}
	int getFreeExtraSlots()	{ return max(SETTING(EXTRA_SLOTS) - getExtra(), 0); }
		
	void reserveSlot(const User::Ptr& aUser) {
		Lock l(cs);
		reservedSlots[aUser] = GET_TICK() + 600*1000;
	}

	void reserveSlotHour(const User::Ptr& aUser) {
		Lock l(cs);
		reservedSlots[aUser] = GET_TICK() + 3600*1000;
	}

	void reserveSlotDay(const User::Ptr& aUser) {
		Lock l(cs);
		reservedSlots[aUser] = GET_TICK() + 24*3600*1000;
	}

	void reserveSlotWeek(const User::Ptr& aUser) {
		Lock l(cs);
		reservedSlots[aUser] = GET_TICK() + 7*24*3600*1000;
	}

	void unreserveSlot(const User::Ptr& aUser) {
		SlotIter uis = reservedSlots.find(aUser);
		if(uis != reservedSlots.end())
			reservedSlots.erase(uis);
	}

	typedef deque<User::Ptr> SlotQueue;
	typedef set<string> FileSet;
	typedef hash_map<User::Ptr, FileSet, User::HashFunction> FilesMap;
	string getQueue() const;
	void clearUserFiles(const User::Ptr&);
	const SlotQueue& getQueueVec() const;
	const FileSet& getQueuedUserFiles(const User::Ptr &) const;
	void addConnection(UserConnection::Ptr conn) {
		conn->addListener(this);
		conn->setState(UserConnection::STATE_GET);
	}

	void clearQueue() { waitingUsers.clear(); waitingFiles.clear(); }
	void setRunning(int _running) { running = _running; }
	GETSET(int, extra, Extra);
	GETSET(u_int32_t, lastGrant, LastGrant);

	// Upload throttling
	size_t throttleGetSlice();
	size_t throttleCycleTime();

private:
	int running;
	void throttleZeroCounters();
	void throttleBytesTransferred(u_int32_t i);
	void throttleSetup();
	bool mThrottleEnable;
	size_t mBytesSent,
		   mBytesSpokenFor,
		   mUploadLimit,
		   mCycleTime,
		   mByteSlice;

	Upload::List uploads;
	CriticalSection cs;

	typedef HASH_MAP<User::Ptr, u_int32_t, User::HashFunction> SlotMap;
	typedef SlotMap::iterator SlotIter;
	SlotMap reservedSlots;

	//functions for manipulating waitingFiles and waitingUsers
	SlotQueue waitingUsers;		//this one merely lists the users waiting for slots
	FilesMap waitingFiles;		//set of files which this user's searched for
	void addFailedUpload(UserConnection::Ptr source, string filename, int64_t pos, int64_t size);
	
	friend class Singleton<UploadManager>;
	UploadManager() throw();
	virtual ~UploadManager() throw();

	void removeConnection(UserConnection::Ptr aConn);
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
	virtual void on(Get, UserConnection*, const string&, int64_t) throw();
	virtual void on(GetBlock, UserConnection* conn, const string& line, int64_t resume, int64_t bytes) throw() { onGetBlock(conn, line, resume, bytes, false); }
	virtual void on(GetZBlock, UserConnection* conn, const string& line, int64_t resume, int64_t bytes) throw() { onGetBlock(conn, line, resume, bytes, true); }
	virtual void on(Send, UserConnection*) throw();
	virtual void on(GetListLength, UserConnection* conn) throw();
	virtual void on(TransmitDone, UserConnection*) throw();

	virtual void on(Command::GET, UserConnection*, const Command&) throw();
	//virtual void on(Command::STA, UserConnection*, const Command&) throw();

	void onGetBlock(UserConnection* aSource, const string& aFile, int64_t aResume, int64_t aBytes, bool z);
	bool prepareFile(UserConnection* aSource, const string& aType, const string& aFile, int64_t aResume, int64_t aBytes, bool adc, bool utf8);
};

#endif // !defined(AFX_UPLOADMANAGER_H__B0C67119_3445_4208_B5AA_938D4A019703__INCLUDED_)

/**
 * @file
 * $Id$
 */
