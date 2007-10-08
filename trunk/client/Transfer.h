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

#ifndef DCPLUSPLUS_CLIENT_TRANSFER_H_
#define DCPLUSPLUS_CLIENT_TRANSFER_H_

#include "forward.h"
#include "MerkleTree.h"
#include "TimerManager.h"
#include "Util.h"

class Transfer {
public:
	enum Type {
		TYPE_FILE,
		TYPE_PARTIAL_FILE,
		TYPE_FULL_LIST,
		TYPE_PARTIAL_LIST,
		TYPE_TREE,
		TYPE_LAST
	};
	
	static const string names[TYPE_LAST];

	static const string USER_LIST_NAME;
	static const string USER_LIST_NAME_BZ;

	Transfer(UserConnection& conn, const string& path, const TTHValue& tth);
	~Transfer() { };

	int64_t getPos() const { return pos; }
	void setPos(int64_t aPos) { pos = aPos; }

	void resetPos() { pos = getStartPos(); }
	void setStartPos(int64_t aPos) { startPos = aPos; pos = aPos; }
	int64_t getStartPos() const { return startPos; }

	void addPos(int64_t aBytes, int64_t aActual) { pos += aBytes; actual+= aActual; }

	enum { AVG_PERIOD = 30000 };
	void updateRunningAverage();

	int64_t getTotal() const { return getPos() - getStartPos(); }
	int64_t getActual() const { return actual; }

	int64_t getSize() const { return size; }
	void setSize(int64_t aSize) { size = aSize; }

	int64_t getAverageSpeed() const {
		int64_t diff = (int64_t)(GET_TICK() - getStart());
		return (diff > 0) ? (getTotal() * (int64_t)1000 / diff) : 0;
	}

	int64_t getSecondsLeft(bool wholeFile = false) {
		updateRunningAverage();
		int64_t avg = getRunningAverage();
		return (avg > 0) ? (((wholeFile ? getFileSize() : getSize()) - getPos()) / avg) : 0;
	}

	int64_t getBytesLeft() const {
		return getSize() - getPos();
	}

	void getParams(const UserConnection& aSource, StringMap& params) const;

	UserPtr getUser();
	const UserPtr getUser() const;
	
	const string& getPath() const { return path; }
	const TTHValue& getTTH() const { return tth; }

	UserConnection& getUserConnection() { return userConnection; }
	const UserConnection& getUserConnection() const { return userConnection; }

	GETSET(Type, type, Type);
	GETSET(uint64_t, start, Start);
	GETSET(uint64_t, lastTick, LastTick);
	GETSET(int64_t, runningAverage, RunningAverage);
	GETSET(int64_t, fileSize, FileSize);
private:
	Transfer(const Transfer&);
	Transfer& operator=(const Transfer&);

	/** The file being transferred */
	string path;
	/** TTH of the file being transferred */
	TTHValue tth;
	/** Bytes on last avg update */
	int64_t last;
	/** Total actual bytes transfered this session (compression?) */
	int64_t actual;
	/** Write position in file */
	int64_t pos;
	/** Starting position */
	int64_t startPos;
	/** Target size of this transfer */
	int64_t size;

	UserConnection& userConnection;
};

#endif /*TRANSFER_H_*/
