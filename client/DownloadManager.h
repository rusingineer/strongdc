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

#ifndef DCPLUSPLUS_CLIENT_DOWNLOAD_MANAGER_H
#define DCPLUSPLUS_CLIENT_DOWNLOAD_MANAGER_H

#include "forward.h"

#include "DownloadManagerListener.h"
#include "UserConnectionListener.h"
#include "QueueItem.h"
#include "TimerManager.h"
#include "Singleton.h"
#include "MerkleTree.h"
#include "Speaker.h"
#include "File.h"

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
	void addConnection(UserConnectionPtr conn);
	void checkIdle(const UserPtr& user);

	/** @internal */
	void abortDownload(const string& aTarget, const Download* except = NULL);

	/** @return Running average download speed in Bytes/s */
	int64_t getRunningAverage();

	/** @return Number of downloads. */ 
	size_t getDownloadCount() {
		Lock l(cs);
		return downloads.size();
	}

	bool startDownload(QueueItem::Priority prio);

	// the following functions were added to help download throttle
	inline bool throttle() const { return mThrottleEnable; }
	void throttleReturnBytes(uint32_t b);
	size_t throttleGetSlice();
	uint32_t throttleCycleTime() const;

private:
	void throttleBytesTransferred(uint32_t i);
	void throttleSetup();
	bool mThrottleEnable;
	uint32_t mCycleTime;
	size_t mBytesSent,
		   mBytesSpokenFor,
		   mDownloadLimit,
		   mByteSlice;
	
	enum { MOVER_LIMIT = 10*1024*1024 };
	class FileMover : public Thread {
	public:
		FileMover() : active(false) { }
		virtual ~FileMover() { join(); }

		void moveFile(const string& source, const string& target);
		virtual int run();
	private:
		typedef pair<string, string> FilePair;
		typedef vector<FilePair> FileList;
		typedef FileList::const_iterator FileIter;

		bool active;

		FileList files;
		CriticalSection cs;
	} mover;
	
	CriticalSection cs;
	DownloadList downloads;
	UserConnectionList idlers;

	void removeConnection(UserConnectionPtr aConn);
	void removeDownload(Download* aDown);
	void fileNotAvailable(UserConnection* aSource);
	void noSlots(UserConnection* aSource, string param = Util::emptyString);
	
	void moveFile(const string& source, const string&target);
	void logDownload(UserConnection* aSource, Download* d);
	int64_t getResumePos(const string& file, const TigerTree& tt, int64_t startPos);

	void failDownload(UserConnection* aSource, const string& reason, bool connectSources = true);

	friend class Singleton<DownloadManager>;

	DownloadManager();
	virtual ~DownloadManager() throw();

	void checkDownloads(UserConnection* aConn, bool reconn = false);
	void handleEndData(UserConnection* aSource);

	// UserConnectionListener
	virtual void on(Data, UserConnection*, const uint8_t*, size_t) throw();
	virtual void on(Error, UserConnection*, const string&) throw();
	virtual void on(Failed, UserConnection*, const string&) throw();
	virtual void on(Sending, UserConnection*, int64_t) throw();
	virtual void on(FileLength, UserConnection*, int64_t) throw();
	virtual void on(MaxedOut, UserConnection*, string param = Util::emptyString) throw();
	virtual	void on(FileNotAvailable, UserConnection*) throw();
	void on(ListLength, UserConnection* aSource, const string& aListLength);
	
	virtual void on(AdcCommand::SND, UserConnection*, const AdcCommand&) throw();
	virtual void on(AdcCommand::STA, UserConnection*, const AdcCommand&) throw();

	bool prepareFile(UserConnection* aSource, int64_t newSize, bool z);
	// TimerManagerListener
	virtual void on(TimerManagerListener::Second, uint32_t aTick) throw();
};

#endif // !defined(DOWNLOAD_MANAGER_H)

/**
 * @file
 * $Id$
 */
