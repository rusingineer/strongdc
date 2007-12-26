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

#include "FinishedManager.h"

#include "ClientManager.h"
#include "FinishedManagerListener.h"
#include "Download.h"
#include "Upload.h"
#include "QueueManager.h"
#include "UploadManager.h"

#include "LogManager.h"
#include "ResourceManager.h"

FinishedManager::FinishedManager() { 
	QueueManager::getInstance()->addListener(this);
	UploadManager::getInstance()->addListener(this);
}
	
FinishedManager::~FinishedManager() throw() {
	QueueManager::getInstance()->removeListener(this);
	UploadManager::getInstance()->removeListener(this);

	Lock l(cs);
	for_each(downloads.begin(), downloads.end(), DeleteFunction());
	for_each(uploads.begin(), uploads.end(), DeleteFunction());
}

void FinishedManager::remove(FinishedItemPtr item, bool upload /* = false */) {
	{
		Lock l(cs);
		FinishedItemList *listptr = upload ? &uploads : &downloads;
		FinishedItemList::iterator it = find(listptr->begin(), listptr->end(), item);

		if(it != listptr->end())
			listptr->erase(it);
		else
			return;
	}
}
	
void FinishedManager::removeAll(bool upload /* = false */) {
	{
		Lock l(cs);
		FinishedItemList *listptr = upload ? &uploads : &downloads;
		for_each(listptr->begin(), listptr->end(), DeleteFunction());
		listptr->clear();
	}
}

void FinishedManager::on(QueueManagerListener::Finished, const QueueItem* qi, const string&, int64_t speed) throw()
{
	bool isFile = !qi->isSet(QueueItem::FLAG_USER_LIST) && !qi->isSet(QueueItem::FLAG_TESTSUR);

	if(isFile && !SETTING(FINISHFILE).empty() && !BOOLSETTING(SOUNDS_DISABLED)) {
		PlaySound(Text::toT(SETTING(FINISHFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
	}
		
	if(isFile || (qi->isSet(QueueItem::FLAG_USER_LIST) && BOOLSETTING(LOG_FILELIST_TRANSFERS))) {
		uint64_t time = GET_TICK() - qi->getFileBegin();
		UserPtr user = qi->getSources().size() == 1 ? qi->getSources()[0].getUser() : UserPtr();
		
		FinishedItemPtr item = new FinishedItem(
			qi->getTarget(), user,
			user ? Util::toString(ClientManager::getInstance()->getHubNames(user->getCID())) : Util::emptyString,
			qi->getSize(), (int64_t)(((double)time / 1000) * speed), time, GET_TIME(), qi->getTTH().toBase32());
		{
			Lock l(cs);
			downloads.push_back(item);
		}
			
		fire(FinishedManagerListener::AddedDl(), item);
	
		size_t BUF_SIZE = STRING(FINISHED_DOWNLOAD).size() + MAX_PATH + 128;
		char* buf = new char[BUF_SIZE];
		snprintf(buf, BUF_SIZE, CSTRING(FINISHED_DOWNLOAD), Util::getFileName(qi->getTarget()).c_str(), 
			user ? Util::toString(ClientManager::getInstance()->getNicks(user->getCID())).c_str() : Util::emptyString.c_str());

		LogManager::getInstance()->message(buf);
		delete[] buf;
	}
}

void FinishedManager::on(UploadManagerListener::Complete, const Upload* u) throw()
{
	if(u->getType() == Transfer::TYPE_FILE || (u->getType() == Transfer::TYPE_FULL_LIST && BOOLSETTING(LOG_FILELIST_TRANSFERS))) {
		if ((!SETTING(UPLOADFILE).empty() && (!BOOLSETTING(SOUNDS_DISABLED))))
			PlaySound(Text::toT(SETTING(UPLOADFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);

		FinishedItemPtr item = new FinishedItem(
			u->getPath(), u->getUser(),
			Util::toString(ClientManager::getInstance()->getHubNames(u->getUser()->getCID())),
			u->getSize(), u->getPos(), (GET_TICK() - u->getStart()), GET_TIME());
		{
			Lock l(cs);
			uploads.push_back(item);
		}

		fire(FinishedManagerListener::AddedUl(), item);

		size_t BUF_SIZE = STRING(FINISHED_UPLOAD).size() + MAX_PATH + 128;
		char* buf = new char[BUF_SIZE];
		snprintf(buf, BUF_SIZE, CSTRING(FINISHED_UPLOAD), (Util::getFileName(u->getPath())).c_str(), 
			Util::toString(ClientManager::getInstance()->getNicks(u->getUser()->getCID())).c_str());

		LogManager::getInstance()->message(buf);
		delete[] buf;		
	}
}

string FinishedManager::getTarget(const string& aTTH){
	if(aTTH.empty()) return Util::emptyString;

	{
		Lock l(cs);

		for(FinishedItemList::const_iterator i = downloads.begin(); i != downloads.end(); i++)
		{
			if((*i)->getTTH() == aTTH)
				return (*i)->getTarget();
		}
	}

	return Util::emptyString;
}

bool FinishedManager::handlePartialRequest(const TTHValue& tth, vector<uint16_t>& outPartialInfo)
{

	string target = getTarget(tth.toBase32());

	if(target.empty()) return false;

	int64_t fileSize = File::getSize(target);

	if(fileSize < PARTIAL_SHARE_MIN_SIZE)
		return false;

	uint16_t len = TigerTree::calcBlocks(fileSize);
	outPartialInfo.push_back(0);
	outPartialInfo.push_back(len);

	return true;
}

/**
 * @file
 * $Id$
 */
