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
#include "HashManager.h"

Download::Download(UserConnection& conn, const string& pfsDir) throw() : Transfer(conn, pfsDir, TTHValue()),
	file(0), treeValid(false)
{
	conn.setDownload(this);
	setType(TYPE_PARTIAL_LIST);
}

Download::Download(UserConnection& conn, QueueItem& qi, bool partial) throw() : Transfer(conn, qi.getTarget(), qi.getTTH()),
	tempTarget(qi.getTempTarget()), file(0), lastTick(GET_TICK()), treeValid(false)
{
	conn.setDownload(this);
	
	setSize(qi.getSize());
	setFileSize(qi.getSize());

	if(qi.isSet(QueueItem::FLAG_USER_LIST)) {
		setType(TYPE_FULL_LIST);
	} else if(partial) {
		setType(TYPE_PARTIAL_FILE);
	}

	if(qi.isSet(QueueItem::FLAG_CHECK_FILE_LIST))
		setFlag(Download::FLAG_CHECK_FILE_LIST);
	if(qi.isSet(QueueItem::FLAG_TESTSUR))
		setFlag(Download::FLAG_TESTSUR);
	if(qi.isSet(QueueItem::FLAG_MULTI_SOURCE))
		setFlag(Download::FLAG_MULTI_CHUNK);

	if(qi.getSize() != -1) {
		if(HashManager::getInstance()->getTree(getTTH(), getTigerTree())) {
			setTreeValid(true);
		} else if(conn.isSet(UserConnection::FLAG_SUPPORTS_TTHL) && !qi.getSource(conn.getUser())->isSet(QueueItem::Source::FLAG_NO_TREE) && qi.getSize() > HashManager::MIN_BLOCK_SIZE) {
			// Get the tree unless the file is small (for small files, we'd probably only get the root anyway)
			setType(TYPE_TREE);
			getTigerTree().setFileSize(qi.getSize());
			setPos(0);
			setSize(-1);
		} else if (!isSet(Download::FLAG_MULTI_CHUNK)) {
			// Use the root as tree to get some sort of validation at least...
			getTigerTree() = TigerTree(qi.getSize(), qi.getSize(), getTTH());
			setTreeValid(true);
		}

		if(!isSet(Download::FLAG_MULTI_CHUNK)) {
			if(qi.isSet(QueueItem::FLAG_RESUME) && getType() != TYPE_TREE) {
				const string& target = (getTempTarget().empty() ? getPath() : getTempTarget());
				int64_t start = File::getSize(target);

				// Only use antifrag if we don't have a previous non-antifrag part
				if( BOOLSETTING(ANTI_FRAG) && (start == -1)) {
					int64_t aSize = File::getSize(target + Download::ANTI_FRAG_EXT);

					if(aSize == getSize())
						start = qi.getDownloadedBytes();
					else
						start = 0;

					setFlag(Download::FLAG_ANTI_FRAG);
				}

				setStartPos(std::max<int64_t>(0, start - (start % getTigerTree().getBlockSize())));
			} else {
				setStartPos(0);
			}
		}
	}
}

Download::~Download() {
	getUserConnection().setDownload(0);
}

AdcCommand Download::getCommand(bool zlib) const {
	AdcCommand cmd(AdcCommand::CMD_GET);
	
	cmd.addParam(Transfer::names[getType()]);

	if(getType() == TYPE_PARTIAL_LIST) { 
		cmd.addParam(Util::toAdcFile(getPath()));
	} else if(getType() == TYPE_FULL_LIST) {
		if(isSet(Download::FLAG_XML_BZ_LIST)) {
			cmd.addParam(USER_LIST_NAME_BZ);
		} else {
			cmd.addParam(USER_LIST_NAME);
		}
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
	params["target"] = getPath();
}

