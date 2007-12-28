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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "QueueItem.h"
#include "HashManager.h"
#include "Download.h"
#include "File.h"

namespace {
	const string TEMP_EXTENSION = ".dctmp";

	string getTempName(const string& aFileName, const TTHValue& aRoot) {
		string tmp(aFileName);
		tmp += "." + aRoot.toBase32();
		tmp += TEMP_EXTENSION;
		return tmp;
	}
}

size_t QueueItem::countOnlineUsers() const {
	size_t n = 0;
	SourceConstIter i = sources.begin();
	for(; i != sources.end(); ++i) {
		if(i->getUser()->isOnline())
			n++;
	}
	return n;
}

void QueueItem::addSource(const UserPtr& aUser) {
	dcassert(!isSource(aUser));
	SourceIter i = getBadSource(aUser);
	if(i != badSources.end()) {
		sources.push_back(*i);
		badSources.erase(i);
	} else {
		sources.push_back(Source(aUser));
	}
}

void QueueItem::removeSource(const UserPtr& aUser, Flags::MaskType reason) {
	SourceIter i = getSource(aUser);
	dcassert(i != sources.end());
	i->setFlag(reason);
	badSources.push_back(*i);
	sources.erase(i);
}

const string& QueueItem::getTempTarget() {
	if(!isSet(QueueItem::FLAG_USER_LIST) && tempTarget.empty()) {
		if(!SETTING(TEMP_DOWNLOAD_DIRECTORY).empty() && (File::getSize(getTarget()) == -1)) {
#ifdef _WIN32
			::StringMap sm;
			if(target.length() >= 3 && target[1] == ':' && target[2] == '\\')
				sm["targetdrive"] = target.substr(0, 3);
			else
				sm["targetdrive"] = Util::getConfigPath().substr(0, 3);
			setTempTarget(Util::formatParams(SETTING(TEMP_DOWNLOAD_DIRECTORY), sm, false) + getTempName(getTargetFileName(), getTTH()));
#else //_WIN32
			setTempTarget(SETTING(TEMP_DOWNLOAD_DIRECTORY) + getTempName(getTargetFileName(), getTTH()));
#endif //_WIN32
		}
	}
	return tempTarget;
}

int64_t QueueItem::getAverageSpeed() const {
	int64_t totalSpeed = 0;
	
	for(DownloadList::const_iterator i = downloads.begin(); i != downloads.end(); i++) {
		totalSpeed += static_cast<int64_t>((*i)->getAverageSpeed());
	}

	return totalSpeed;
}

Segment QueueItem::getNextSegment(int64_t  blockSize, const PartialSource::Ptr partialSource) const {
	if(downloads.size() >= maxSegments ||
		(BOOLSETTING(DONT_BEGIN_SEGMENT) && (size_t)(SETTING(DONT_BEGIN_SEGMENT_SPEED) * 1024) < getAverageSpeed()))
	{
		// no other segments if we have reached the speed or segment limit
		return Segment(0, 0);
	}

	if(getSize() == -1 || blockSize == 0) {
		return Segment(0, -1);
	}

	/* added for PFS */
	vector<int64_t> posArray;
	vector<Segment> neededParts;

	if(partialSource) {
		posArray.reserve(partialSource->getPartialInfo().size());

		// Convert block index to file position
		for(PartsInfo::const_iterator i = partialSource->getPartialInfo().begin(); i != partialSource->getPartialInfo().end(); i++)
			posArray.push_back(min(getSize(), (int64_t)(*i) * blockSize));
	}

	/***************************/

	int64_t start = 0;
	int64_t maxSize = std::max(blockSize, static_cast<int64_t>(1024 * 1024));
	maxSize = ((maxSize + blockSize - 1) / blockSize) * blockSize; // Make sure we're on an even block boundary
	int64_t curSize = maxSize;
	
	while(start < getSize()) {
		int64_t end = std::min(getSize(), start + curSize);
		Segment block(start, end - start);
		bool overlaps = false;
		for(SegmentIter i = done.begin(); !overlaps && i != done.end(); ++i) {
			if(curSize <= blockSize) {
				int64_t dstart = i->getStart();
				int64_t dend = i->getEnd();
				// We accept partial overlaps, only consider the block done if it is fully consumed by the done block
				if(dstart <= start && dend >= end) {
					overlaps = true;
				}
			} else {
				overlaps = block.overlaps(*i);
			}
		}
		
		for(DownloadList::const_iterator i = downloads.begin(); !overlaps && i !=downloads.end(); ++i) {
			overlaps = block.overlaps((*i)->getSegment());
		}
		
		if(!overlaps) {
			if(partialSource) {
				// store all chunks we could need
				for(vector<int64_t>::const_iterator j = posArray.begin(); j < posArray.end(); j += 2){
					if((*j) <= start && *(j+1) >= end) {					
						neededParts.push_back(block);
					}
				}
			} else {
				return block;
			}
		}
		
		if(curSize > blockSize) {
			curSize -= blockSize;
		} else {
			start = end;
			curSize = maxSize;
		}
	}

	if(!neededParts.empty()) {
		// select random chunk for PFS
		dcdebug("Found partial chunks: %d\n", neededParts.size());
		return neededParts[Util::rand(0, neededParts.size())];
	}
	
	return Segment(0, 0);
}

int64_t QueueItem::getDownloadedBytes() const {
	int64_t total = 0;

	// count done segments
	for(SegmentSet::const_iterator i = done.begin(); i != done.end(); ++i) {
		total += i->getSize();
	}

	// count running segments
	for(DownloadList::const_iterator i = downloads.begin(); i != downloads.end(); ++i) {
		total += (*i)->getPos();
	}

	return total;
}

void QueueItem::addSegment(const Segment& segment) {
	done.insert(segment);

	// Consilidate segments
	if(done.size() == 1)
		return;
	
	for(SegmentSet::iterator i = ++done.begin() ; i != done.end(); ) {
		SegmentSet::iterator prev = i;
		prev--;
		if(prev->getEnd() >= i->getStart()) {
			Segment big(prev->getStart(), i->getEnd() - prev->getStart());
			done.erase(prev);
			done.erase(i++);
			done.insert(big);
		} else {
			++i;
		}
	}
}

bool QueueItem::isSource(const PartsInfo& partsInfo, int64_t blockSize)
{
	dcassert(partsInfo.size() % 2 == 0);
	
	SegmentIter i  = done.begin();
	for(PartsInfo::const_iterator j = partsInfo.begin(); j != partsInfo.end(); j+=2){
		while(i != done.end() && (*i).getEnd() <= (*j) * blockSize)
			i++;

		if(i == done.end() || !((*i).getStart() <= (*j) * blockSize && (*i).getEnd() >= (*(j+1)) * blockSize))
			return true;
	}
	
	return false;

}

void QueueItem::getPartialInfo(PartsInfo& partialInfo, int64_t blockSize) {
	size_t maxSize = min(done.size() * 2, (size_t)510);
	partialInfo.reserve(maxSize);

	SegmentIter i = done.begin();
	for(; i != done.end() && partialInfo.size() < maxSize; i++) {

		uint16_t s = (uint16_t)((*i).getStart() / blockSize);
		uint16_t e = (uint16_t)(((*i).getEnd() - 1) / blockSize + 1);

		partialInfo.push_back(s);
		partialInfo.push_back(e);
	}
}