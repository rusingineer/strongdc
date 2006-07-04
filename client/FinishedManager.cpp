/* 
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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
#include "FileChunksInfo.h"
#include "Client.h"

FinishedManager::~FinishedManager() throw() {
	Lock l(cs);
	for_each(downloads.begin(), downloads.end(), DeleteFunction());
	for_each(uploads.begin(), uploads.end(), DeleteFunction());
	DownloadManager::getInstance()->removeListener(this);
	UploadManager::getInstance()->removeListener(this);

}

void FinishedManager::remove(FinishedItem *item, bool upload /* = false */) {
	{
		Lock l(cs);
		FinishedItem::List *listptr = upload ? &uploads : &downloads;
		FinishedItem::List::iterator it = find(listptr->begin(), listptr->end(), item);

		if(it != listptr->end())
			listptr->erase(it);
		else
			return;
	}
	if (!upload)
		fire(FinishedManagerListener::RemovedDl(), item);
	else
		fire(FinishedManagerListener::RemovedUl(), item);
	delete item;		
}
	
void FinishedManager::removeAll(bool upload /* = false */) {
	{
		Lock l(cs);
		FinishedItem::List *listptr = upload ? &uploads : &downloads;
		for_each(listptr->begin(), listptr->end(), DeleteFunction());
		listptr->clear();
	}
	if (!upload)
		fire(FinishedManagerListener::RemovedAllDl());
	else
		fire(FinishedManagerListener::RemovedAllUl());
}

void FinishedManager::on(DownloadManagerListener::Complete, Download* d, bool) throw()
{
	if(!d->isSet(Download::FLAG_USER_LIST) && !SETTING(FINISHFILE).empty() && !BOOLSETTING(SOUNDS_DISABLED)) {
		PlaySound(Text::toT(SETTING(FINISHFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
	}
		
	if(!d->isSet(Download::FLAG_TREE_DOWNLOAD) && (!d->isSet(Download::FLAG_USER_LIST) || BOOLSETTING(LOG_FILELIST_TRANSFERS))) {
		FinishedItem *item = new FinishedItem(
			d->getTarget(), Util::toString(ClientManager::getInstance()->getNicks(d->getUserConnection()->getUser()->getCID())),
			d->getUserConnection()->getUser()->getCID(),
			Util::toString(ClientManager::getInstance()->getHubNames(d->getUserConnection()->getUser()->getCID())),
			d->getSize(), d->getTotal(), (GET_TICK() - d->getStart()), GET_TIME(), d->isSet(Download::FLAG_CRC32_OK), d->getTTH() ? d->getTTH()->toBase32() : Util::emptyString);
		{
			Lock l(cs);
			downloads.push_back(item);
		}
			
		fire(FinishedManagerListener::AddedDl(), item);
	
		char* buf = new char[STRING(FINISHED_DOWNLOAD).size() + MAX_PATH + 128];
		_snprintf(buf, STRING(FINISHED_DOWNLOAD).size() + MAX_PATH + 127, CSTRING(FINISHED_DOWNLOAD), d->getTargetFileName().c_str(), 
			d->getUserConnection()->getUser()->getFirstNick().c_str());
		buf[STRING(FINISHED_DOWNLOAD).size() + MAX_PATH + 127] = 0;
		LogManager::getInstance()->message(buf);
		delete[] buf;
	}
}

void FinishedManager::on(UploadManagerListener::Complete, Upload* u) throw()
{
	if(!u->isSet(Upload::FLAG_TTH_LEAVES) && (!u->isSet(Upload::FLAG_USER_LIST) || BOOLSETTING(LOG_FILELIST_TRANSFERS)) && u->getSize() == u->getFileSize()) {
		if ((!SETTING(UPLOADFILE).empty() && (!BOOLSETTING(SOUNDS_DISABLED))))
			PlaySound(Text::toT(SETTING(UPLOADFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);

		FinishedItem *item = new FinishedItem(
			u->getLocalFileName(), Util::toString(ClientManager::getInstance()->getNicks(u->getUserConnection()->getUser()->getCID())),
			u->getUserConnection()->getUser()->getCID(),
			Util::toString(ClientManager::getInstance()->getHubNames(u->getUserConnection()->getUser()->getCID())),
			u->getSize(), u->getTotal(), (GET_TICK() - u->getStart()), GET_TIME());
		{
			Lock l(cs);
			uploads.push_back(item);
		}

		fire(FinishedManagerListener::AddedUl(), item);

		char* buf = new char[STRING(FINISHED_UPLOAD).size() + MAX_PATH + 128];
		sprintf_s(buf, STRING(FINISHED_UPLOAD).size() + MAX_PATH + 128, CSTRING(FINISHED_UPLOAD), (Util::getFileName(u->getLocalFileName())).c_str(), 
			u->getUserConnection()->getUser()->getFirstNick().c_str());
		LogManager::getInstance()->message(buf);
		delete[] buf;		
	}
}

string FinishedManager::getTarget(const string& aTTH){
	if(aTTH.empty()) return Util::emptyString;

	{
		Lock l(cs);

		for(FinishedItem::Iter i = downloads.begin(); i != downloads.end(); i++)
		{
			if((*i)->getTTH() == aTTH)
				return (*i)->getTarget();
		}
	}

	return Util::emptyString;
}

bool FinishedManager::handlePartialRequest(const TTHValue& tth, vector<u_int16_t>& outPartialInfo)
{

	string target = getTarget(tth.toBase32());

	if(target.empty()) return false;

	int64_t fileSize = File::getSize(target);

	if(fileSize < PARTIAL_SHARE_MIN_SIZE)
		return false;

	u_int16_t len = TigerTree::calcBlocks(fileSize);
	outPartialInfo.push_back(0);
	outPartialInfo.push_back(len);

	return true;
}

/**
 * @file
 * $Id$
 */
