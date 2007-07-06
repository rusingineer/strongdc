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

#include "Download.h"

#include "UserConnection.h"
#include "QueueItem.h"

Download::Download(UserConnection& conn) throw() : Transfer(conn), file(0),
treeValid(false) {
	conn.setDownload(this);
}

Download::Download(UserConnection& conn, QueueItem& qi/*, const Source& aSource*/) throw() :
	Transfer(conn), target(qi.getTarget()), tempTarget(qi.getTempTarget()), file(0), 
	quickTick(static_cast<uint32_t>(GET_TICK())), treeValid(false), source(NULL)
{
	conn.setDownload(this);
	
	setTTH(qi.getTTH());
	setSize(qi.getSize());
	setFileSize(qi.getSize());

	if(qi.isSet(QueueItem::FLAG_USER_LIST))
		setFlag(Download::FLAG_USER_LIST);
	if(qi.isSet(QueueItem::FLAG_CHECK_FILE_LIST))
		setFlag(Download::FLAG_CHECK_FILE_LIST);
	if(qi.isSet(QueueItem::FLAG_TESTSUR))
		setFlag(Download::FLAG_TESTSUR);
	if(qi.isSet(QueueItem::FLAG_RESUME))
		setFlag(Download::FLAG_RESUME);
	if(qi.isSet(QueueItem::FLAG_MULTI_SOURCE))
		setFlag(Download::FLAG_MULTI_CHUNK);

	//if(aSource->isSet(QueueItem::Source::FLAG_PARTIAL))
	//	setFlag(Download::FLAG_PARTIAL);
}
Download::~Download() {
	getUserConnection().setDownload(0);
}

AdcCommand Download::getCommand(bool zlib) const {
	AdcCommand cmd(AdcCommand::CMD_GET);
	if(isSet(FLAG_TREE_DOWNLOAD)) {
		cmd.addParam(Transfer::TYPE_TTHL);
	} else if(isSet(FLAG_PARTIAL_LIST)) {
		cmd.addParam(Transfer::TYPE_LIST);
	} else {
		cmd.addParam(Transfer::TYPE_FILE);
	}
	if(isSet(FLAG_PARTIAL_LIST) || isSet(FLAG_USER_LIST)) {
		cmd.addParam(Util::toAdcFile(*getSource()));
	} else {
		cmd.addParam("TTH/" + getTTH().toBase32());
	}
	cmd.addParam(Util::toString(getPos()));
	cmd.addParam(Util::toString(getSize() - getPos()));

	if(zlib && BOOLSETTING(COMPRESS_TRANSFERS)) {
		cmd.addParam("ZL1");
	}

	return cmd;
}

void Download::getParams(const UserConnection& aSource, StringMap& params) {
	Transfer::getParams(aSource, params);
	params["target"] = getTarget();
}

