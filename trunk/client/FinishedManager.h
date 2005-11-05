/*
 * Copyright (C) 2001-2005 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(FINISHED_MANAGER_H)
#define FINISHED_MANAGER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DownloadManager.h"
#include "UploadManager.h"

#include "CriticalSection.h"
#include "Singleton.h"
#include "ResourceManager.h"
#include "LogManager.h"

class FinishedItem
{
public:
	typedef FinishedItem* Ptr;
	typedef vector<Ptr> List;
	typedef List::iterator Iter;

	FinishedItem::List subItems;

	FinishedItem(string const& aTarget, string const& aUser, string const& aHub, 
		int64_t aSize, int64_t aChunkSize, int64_t aMSeconds, time_t aTime,
		bool aCrc32 = false, const string& aTTH = Util::emptyString) : 
		target(aTarget), user(aUser), hub(aHub), size(aSize), chunkSize(aChunkSize),
		milliSeconds(aMSeconds), time(aTime), crc32Checked(aCrc32), tth(aTTH),
		collapsed(true), mainitem(false), needDelete(true), main(NULL)
	{
	}

	int64_t getAvgSpeed() { return milliSeconds > 0 ? (chunkSize * ((int64_t)1000) / milliSeconds) : 0; };

	bool collapsed;
	bool mainitem;
	bool needDelete;
	FinishedItem* main;

	GETSET(string, target, Target);
	GETSET(string, user, User);
	GETSET(string, hub, Hub);
	GETSET(int64_t, size, Size);
	GETSET(int64_t, chunkSize, ChunkSize);
	GETSET(int64_t, milliSeconds, MilliSeconds);
	GETSET(time_t, time, Time);
	GETSET(bool, crc32Checked, Crc32Checked)
	GETSET(string, tth, TTH);	
private:
	friend class FinishedManager;

};

class FinishedMP3Item
{
public:
	typedef FinishedMP3Item* Ptr;
	typedef vector<Ptr> List;
	typedef List::iterator Iter;

	FinishedMP3Item(string const& aTarget, string const& aUser, string const& aHub, 
		int64_t aSize, string aMPEGVer, int aSampleRate, int aBitRate, string aChannels, time_t aTime) : 
		target(aTarget), user(aUser), hub(aHub), size(aSize), MpegVer(aMPEGVer), samplerate(aSampleRate),
		bitrate(aBitRate), channels(aChannels), time(aTime)
	{
	}

	GETSET(string, target, Target);
	GETSET(string, user, User);
	GETSET(string, hub, Hub);
	GETSET(int64_t, size, Size);
	GETSET(string, MpegVer, MPEGVer);
	GETSET(string, channels, Channels);
	GETSET(time_t, time, Time);
	GETSET(int, bitrate, BitRate);
	GETSET(int, samplerate, SampleRate);

private:
	friend class FinishedManager;

};

class FinishedManagerListener {
public:
	template<int I>	struct X { enum { TYPE = I };  };

	typedef X<0> AddedUl;
	typedef X<1> AddedDl;
	typedef X<2> RemovedUl;
	typedef X<3> RemovedDl;
	typedef X<4> RemovedAllUl;
	typedef X<5> RemovedAllDl;
	typedef X<6> Added_MP3Dl;

	virtual void on(AddedDl, FinishedItem*) throw() { }
	virtual void on(Added_MP3Dl, FinishedMP3Item*) throw() { }
	virtual void on(RemovedDl, FinishedItem*) throw() { }
	virtual void on(RemovedAllDl) throw() { }
	virtual void on(AddedUl, FinishedItem*) throw() { }
	virtual void on(RemovedUl, FinishedItem*) throw() { }
	virtual void on(RemovedAllUl) throw() { }

}; 

class FinishedManager : public Singleton<FinishedManager>,
	public Speaker<FinishedManagerListener>, private DownloadManagerListener, private UploadManagerListener
{
public:
	FinishedItem::List& lockList(bool upload = false) { cs.enter(); return upload ? uploads : downloads; };
	FinishedMP3Item::List& lockMP3List() { cs.enter(); return MP3downloads; };
	void unlockList() { cs.leave(); };

	void remove(FinishedItem *item, bool upload = false);
	void removeAll(bool upload = false);

	/** Get file full path by tth to share */
	string getTarget(const string& aTTH);
	bool handlePartialRequest(const TTHValue& tth, vector<u_int16_t>& outPartialInfo);


private:
	friend class Singleton<FinishedManager>;
	
	FinishedManager() { 
		DownloadManager::getInstance()->addListener(this);
		UploadManager::getInstance()->addListener(this);
	}
	virtual ~FinishedManager() throw();

	virtual void on(DownloadManagerListener::Complete, Download* d, bool) throw();
	virtual void on(UploadManagerListener::Complete, Upload*) throw();

	CriticalSection cs;
	FinishedMP3Item::List MP3downloads;
	FinishedItem::List downloads, uploads;
};

#endif // !defined(FINISHED_MANAGER_H)

/**
 * @file
 * $Id$
 */
