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

#ifndef DCPLUSPLUS_CLIENT_FINISHED_MANAGER_H
#define DCPLUSPLUS_CLIENT_FINISHED_MANAGER_H

#include "DownloadManager.h"
#include "UploadManager.h"

#include "CriticalSection.h"
#include "Singleton.h"
#include "FinishedManagerListener.h"

class FinishedItem
{
public:
	enum {
		COLUMN_FIRST,
		COLUMN_FILE = COLUMN_FIRST,
		COLUMN_DONE,
		COLUMN_PATH,
		COLUMN_NICK,
		COLUMN_HUB,
		COLUMN_SIZE,
		COLUMN_SPEED,
		COLUMN_LAST
	};

	FinishedItem(string const& aTarget, const UserPtr& aUser, string const& aHub, 
		int64_t aSize, int64_t aChunkSize, int64_t aMSeconds, time_t aTime,
		const string& aTTH = Util::emptyString) : 
		target(aTarget), user(aUser), hub(aHub), size(aSize), chunkSize(aChunkSize),
		milliSeconds(aMSeconds), time(aTime), tth(aTTH)
	{
	}

	int64_t getAvgSpeed() const { return milliSeconds > 0 ? (chunkSize * ((int64_t)1000) / milliSeconds) : 0; }

	inline const wchar_t* getText(uint8_t col) const {
		dcassert(col >= 0 && col < COLUMN_LAST);
		return columns[col];
	}

	const wchar_t* copy(uint8_t col) {
		if(col >= 0 && col < COLUMN_LAST)
			return getText(col);

		return Util::emptyStringT.c_str();
	}

	static int compareItems(const FinishedItem* a, const FinishedItem* b, uint8_t col) {
		switch(col) {
			case COLUMN_SPEED:	return compare(a->getAvgSpeed(), b->getAvgSpeed());
			case COLUMN_SIZE:	return compare(a->getSize(), b->getSize());
			default:			return lstrcmpi(a->columns[col], b->columns[col]);
		}
	}
	int imageIndex() const;

	GETSET(string, target, Target);
	GETSET(string, hub, Hub);
	GETSET(string, tth, TTH);

	GETSET(int64_t, size, Size);
	GETSET(int64_t, chunkSize, ChunkSize);
	GETSET(int64_t, milliSeconds, MilliSeconds);
	GETSET(time_t, time, Time);
	GETSET(UserPtr, user, User);

	ColumnBase columns;

private:
	friend class FinishedManager;

};

class FinishedManager : public Singleton<FinishedManager>,
	public Speaker<FinishedManagerListener>, private DownloadManagerListener, private UploadManagerListener
{
public:
	const FinishedItemList& lockList(bool upload = false) { cs.enter(); return upload ? uploads : downloads; }
	void unlockList() { cs.leave(); }

	void remove(FinishedItemPtr item, bool upload = false);
	void removeAll(bool upload = false);

	/** Get file full path by tth to share */
	string getTarget(const string& aTTH);
	bool handlePartialRequest(const TTHValue& tth, vector<uint16_t>& outPartialInfo);


private:
	friend class Singleton<FinishedManager>;
	
	FinishedManager() { 
		DownloadManager::getInstance()->addListener(this);
		UploadManager::getInstance()->addListener(this);
	}
	~FinishedManager() throw();

	void on(DownloadManagerListener::Complete, const Download* d, bool) throw();
	void on(UploadManagerListener::Complete, const Upload*) throw();

	CriticalSection cs;
	FinishedItemList downloads, uploads;
};

#endif // !defined(FINISHED_MANAGER_H)

/**
 * @file
 * $Id$
 */
