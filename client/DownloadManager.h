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

#if !defined(AFX_DOWNLOADMANAGER_H__D6409156_58C2_44E9_B63C_B58C884E36A3__INCLUDED_)
#define AFX_DOWNLOADMANAGER_H__D6409156_58C2_44E9_B63C_B58C884E36A3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TimerManager.h"

#include "UserConnection.h"
#include "Singleton.h"
#include "FilteredFile.h"
#include "ZUtils.h"
#include "MerkleTree.h"
#include "LogManager.h"

class QueueItem;
class ConnectionQueueItem;

/**
 * Comes as an argument in the DownloadManagerListener functions.
 * Use it to retrieve information about the ongoing transfer.
 */
class Download : public Transfer, public Flags {
public:
	static const string ANTI_FRAG_EXT;

	typedef Download* Ptr;
	typedef vector<Ptr> List;
	typedef List::iterator Iter;

	enum {
		FLAG_USER_LIST = 0x01,
		FLAG_RESUME = 0x02,
		FLAG_ROLLBACK = 0x04,
		FLAG_ZDOWNLOAD = 0x08,
		FLAG_CALC_CRC32 = 0x10,
		FLAG_CRC32_OK = 0x20,
		FLAG_ANTI_FRAG = 0x40,
		FLAG_UTF8 = 0x80,
		FLAG_TREE_DOWNLOAD = 0x100,
		FLAG_TREE_TRIED = 0x200,		
		FLAG_TESTSUR = 0x400,
		FLAG_TTH_OK = 0x800,
		FLAG_MP3_INFO = 0x1000,
		FLAG_CHECK_FILE_LIST = 0x2000,
		FLAG_MULTI_CHUNK = 0x4000
	};

	Download() throw();
	Download(QueueItem* qi, User::Ptr& aUser) throw();

	/**
	 * @remarks This function is only used from DownloadManager but its
	 * functionality could be useful in TransferView.
	 *
	 * @return Target filename without path.
	 */
	string getTargetFileName() {
		string::size_type i = getTarget().rfind('\\');
		if(i != string::npos) {
			return getTarget().substr(i + 1);
		} else {
			return getTarget();
		}
	};

	int64_t getQueueTotal();
	
	/** @internal */
	string getDownloadTarget() {
		const string& tgt = (getTempTarget().empty() ? getTarget() : getTempTarget());
		return isSet(FLAG_ANTI_FRAG) ? tgt + ANTI_FRAG_EXT : tgt;			
	}

	/** @internal */
	TigerTree& getTigerTree() {
		return tt;
	}

	/** @internal */
	AdcCommand getCommand(bool zlib, bool tthf);

	typedef CalcOutputStream<CRC32Filter, true> CrcOS;
	GETSET(string, source, Source);
	GETSET(string, target, Target);
	GETSET(string, tempTarget, TempTarget);
	GETSET(OutputStream*, file, File);
	GETSET(CrcOS*, crcCalc, CrcCalc);
	GETSET(TTHValue*, tth, TTH);
	GETSET(bool, treeValid, TreeValid);
	GETSET(int64_t, segmentSize, SegmentSize);
	GETSET(QueueItem*, item, Item);
	int64_t quickTick;
	bool finished;
	
private:
	Download(const Download&);

	Download& operator=(const Download&);

	TigerTree tt;
};


/**
 * Use this listener interface to get progress information for downloads.
 *
 * @remarks All methods are sending a pointer to a Download but the receiver
 * (TransferView) is not using any of the methods in Download, only methods
 * from its super class, Transfer. The listener functions should send Transfer
 * objects instead.
 *
 * Changing this will will cause a problem with Download::List which is used
 * in the on Tick function. One solution is reimplement on Tick to call once
 * for every Downloads, sending one Download at a time. But maybe updating the
 * GUI is not DownloadManagers problem at all???
 */
class DownloadManagerListener {
public:
	template<int I>	struct X { enum { TYPE = I };  };

	typedef X<0> Complete;
	typedef X<1> Failed;
	typedef X<2> Starting;
	typedef X<3> Tick;
	typedef X<4> Status;
	typedef X<5> Verifying;

	/** 
	 * This is the first message sent before a download starts. 
	 * No other messages will be sent before.
	 */
	virtual void on(Starting, Download*, bool) throw() { };

	/**
	 * Sent once a second if something has actually been downloaded.
	 */
	virtual void on(Tick, const Download::List&) throw() { };

	/** 
	 * This is the last message sent before a download is deleted. 
	 * No more messages will be sent after it.
	 */
	virtual void on(Complete, Download*, bool) throw() { };

	/** 
	 * This indicates some sort of failure with a particular download.
	 * No more messages will be sent after it.
	 *
	 * @remarks Should send an error code instead of a string and let the GUI
	 * display an error string.
	 */
	virtual void on(Failed, Download*, const string&) throw() { };
	virtual void on(Status, ConnectionQueueItem*, const string&) throw() { };
	virtual void on(Verifying, const string&, int64_t) throw() { };
};


/**
 * Singleton. Use its listener interface to update the download list
 * in the user interface.
 */
class DownloadManager : public Speaker<DownloadManagerListener>, 
	private UserConnectionListener, private TimerManagerListener, 
	public Singleton<DownloadManager>
{
public:

	/** @internal */
	void addConnection(UserConnection::Ptr conn) {
		conn->addListener(this);
		checkDownloads(conn);
	}

	/** @internal */
	void abortDownload(const string& aTarget);
	void abortDownload(const string& aTarget, User::Ptr& aUser);

	/**
	 * @remarks This is only used in the tray icons. In MainFrame this is
	 * calculated instead so there seems to be a little duplication of code.
	 *
	 * @return Average download speed in Bytes/s
	 */
	int getAverageSpeed() {
		Lock l(cs);
		int avg = 0;
		for(Download::Iter i = downloads.begin(); i != downloads.end(); ++i) {
			Download* d = *i;
			avg += (int)d->getRunningAverage();
		}
		return avg;
	}

	/** @return Number of downloads. */ 
	size_t getDownloadCount() {
		Lock l(cs);
		return downloads.size();
	}
	
	bool throttle() { return mThrottleEnable; }
	void throttleReturnBytes(u_int32_t b);
	size_t throttleGetSlice();
	size_t throttleCycleTime();

	int getActiveDownloads() {
		int j = 0;
		for(Download::Iter i = downloads.begin(); i != downloads.end(); ++i) {
			Download* d = *i;
			Transfer* e = d;
			UserConnection* uc = e->getUserConnection();
			if (uc->isSet(UserConnection::FLAG_DOWNLOAD)) {
				++j;
			}
		}
		return j;
	}
private:
	void throttleZeroCounters();
	void throttleBytesTransferred(u_int32_t i);
	void throttleSetup();
	bool mThrottleEnable;
	size_t mBytesSent,
		   mBytesSpokenFor,
		   mDownloadLimit,
		   mCycleTime,
		   mByteSlice;

	enum { MOVER_LIMIT = 10*1024*1024 };
	class FileMover : public Thread {
	public:
		FileMover() : active(false) { };
		virtual ~FileMover() { join(); };

		void moveFile(const string& source, const string& target);
		virtual int run();
	private:
		typedef pair<string, string> FilePair;
		typedef vector<FilePair> FileList;
		typedef FileList::iterator FileIter;

		bool active;

		FileList files;
		CriticalSection cs;
	} mover;
	
	CriticalSection cs;
	Download::List downloads;
	
	bool checkRollback(Download* aDownload, const u_int8_t* aBuf, int aLen) throw(FileException);
	void removeConnection(UserConnection::Ptr aConn, bool reuse = false, bool ntd = false, bool reconn = false);
	void removeDownload(Download* aDown, bool removeSegment = true);
	void fileNotAvailable(UserConnection* aSource);
	void noSlots(UserConnection* aSource);
	
	void moveFile(const string& source, const string&target);
	void logDownload(UserConnection* aSource, Download* d);
	u_int32_t calcCrc32(const string& file) throw(FileException);
	bool checkSfv(UserConnection* aSource, Download* d, u_int32_t crc);
	int64_t getResumePos(const string& file, const TigerTree& tt, int64_t startPos);

	friend class Singleton<DownloadManager>;
	DownloadManager() { 
		TimerManager::getInstance()->addListener(this);
		mDownloadLimit = 0; mBytesSent = 0; mBytesSpokenFor = 0; mCycleTime = 0; mByteSlice = 0;
		mThrottleEnable = BOOLSETTING(THROTTLE_ENABLE);
		throttleZeroCounters();
	};

	virtual ~DownloadManager() throw() {
		TimerManager::getInstance()->removeListener(this);
		while(true) {
			{
				Lock l(cs);
				if(downloads.empty())
					break;
			}
			Thread::sleep(100);
		}
	};
	
	void checkDownloads(UserConnection* aConn, bool reconn = false, QueueItem* q = NULL);
	void handleEndData(UserConnection* aSource);
	void userSpeed(UserConnection* aSource);

	// UserConnectionListener
	virtual void on(Data, UserConnection*, const u_int8_t*, size_t) throw();
	virtual void on(Failed, UserConnection*, const string&) throw();
	virtual void on(Sending, UserConnection*, int64_t) throw();
	virtual void on(FileLength, UserConnection*, int64_t) throw();
	virtual void on(MaxedOut, UserConnection*) throw();
	virtual	void on(FileNotAvailable, UserConnection*) throw();
	void on(ListLength, UserConnection* aSource, const string& aListLength);
	
	virtual void on(AdcCommand::SND, UserConnection*, const AdcCommand&) throw();
	virtual void on(AdcCommand::STA, UserConnection*, const AdcCommand&) throw();

	bool prepareFile(UserConnection* aSource, int64_t newSize, bool z);
	// TimerManagerListener
	virtual void on(TimerManagerListener::Second, u_int32_t aTick) throw();
};

#endif // !defined(AFX_DOWNLOADMANAGER_H__D6409156_58C2_44E9_B63C_B58C884E36A3__INCLUDED_)

/**
 * @file
 * $Id$
 */