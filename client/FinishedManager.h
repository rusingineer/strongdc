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

#ifndef DCPLUSPLUS_DCPP_FINISHED_MANAGER_H
#define DCPLUSPLUS_DCPP_FINISHED_MANAGER_H

#include "QueueManagerListener.h"
#include "UploadManagerListener.h"

#include "Speaker.h"
#include "CriticalSection.h"
#include "Singleton.h"
#include "FinishedManagerListener.h"
#include "Util.h"
#include "User.h"
#include "MerkleTree.h"
#include "ClientManager.h"

namespace dcpp {

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

	const tstring getText(uint8_t col) const {
		dcassert(col >= 0 && col < COLUMN_LAST);
		switch(col) {
			case COLUMN_FILE: return Text::toT(Util::getFileName(getTarget()));
			case COLUMN_DONE: return Text::toT(Util::formatTime("%Y-%m-%d %H:%M:%S", getTime()));
			case COLUMN_PATH: return Text::toT(Util::getFilePath(getTarget()));
			case COLUMN_NICK: return getUser() ? Text::toT(Util::toString(ClientManager::getInstance()->getNicks(getUser()->getCID()))) : Util::emptyStringT;
			case COLUMN_HUB: return Text::toT(getHub());
			case COLUMN_SIZE: return Util::formatBytesW(getSize());
			case COLUMN_SPEED: return Util::formatBytesW(getAvgSpeed()) + _T("/s");
			default: return Util::emptyStringT;
		}
	}

	static int compareItems(const FinishedItem* a, const FinishedItem* b, uint8_t col) {
		switch(col) {
			case COLUMN_SPEED:	return compare(a->getAvgSpeed(), b->getAvgSpeed());
			case COLUMN_SIZE:	return compare(a->getSize(), b->getSize());
			default:			return lstrcmpi(a->getText(col).c_str(), b->getText(col).c_str());
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

private:
	friend class FinishedManager;

};

class FinishedManager : public Singleton<FinishedManager>,
	public Speaker<FinishedManagerListener>, private QueueManagerListener, private UploadManagerListener
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
	
	FinishedManager();
	~FinishedManager() throw();

	void on(QueueManagerListener::Finished, const QueueItem*, const string&, const Download*) throw();
	void on(UploadManagerListener::Complete, const Upload*) throw();

	CriticalSection cs;
	FinishedItemList downloads, uploads;
};

} // namespace dcpp

#endif // !defined(FINISHED_MANAGER_H)

/**
 * @file
 * $Id$
 */
