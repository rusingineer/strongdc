/* 
 * 
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

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"
#include "UserInfo.h"

UserInfo::UserInfo(const User::Ptr& u) : UserInfoBase(u), op(false) { 
	update();
};

const tstring& UserInfo::getText(int col) const {
	return columns[col];
}

int UserInfo::compareItems(const UserInfo* a, const UserInfo* b, int col)  {
	if(a == NULL || b == NULL){
		dcdebug("UserInfo::compareItems NULL >:-[\n");
		return NULL;
	}
	if(col == COLUMN_NICK) {
			if(a->getOp() && !b->getOp()) {
				return -1;
			} else if(!a->getOp() && b->getOp()) {
				return 1;
			}
	}

	switch(col) {
		case COLUMN_SHARED: return compare(a->user->getBytesShared(), b->user->getBytesShared());
		case COLUMN_EXACT_SHARED: return compare(a->user->getBytesShared(), b->user->getBytesShared());
		case COLUMN_HUBS: return compare(Util::toInt(a->user->getHubs()), Util::toInt(b->user->getHubs()));
		case COLUMN_SLOTS: return compare(a->user->getSlots(), b->user->getSlots());
		case COLUMN_UPLOAD_SPEED: return compare(Util::toInt(a->user->getUpload()), Util::toInt(b->user->getUpload()));
	}
	return lstrcmpi(a->columns[col].c_str(), b->columns[col].c_str());	
}

bool UserInfo::update() {
	bool needsSort = (op != user->isSet(User::OP));

	tstring uploadSpeed;

	if(user->getDownloadSpeed()<1) {
		int status = user->getStatus();
		string Omezeni = user->getUpload();
		if (!Omezeni.empty()) {
			uploadSpeed = Text::toT(Util::formatBytes(Util::toInt64(Omezeni)*1024)) + _T("/s");
		} else if( (status == 8) || (status == 9)  || (status == 10) || (status == 11)) {
			uploadSpeed = _T(">= 100 kB/s");
		} else {
			uploadSpeed = _T("N/A");
		}
	} else
		uploadSpeed = Text::toT(Util::formatBytes(user->getDownloadSpeed())) + _T("/s");

	columns[COLUMN_NICK] = Text::toT(user->getNick());
	columns[COLUMN_SHARED] = Text::toT(Util::formatBytes(user->getBytesShared()));
	columns[COLUMN_EXACT_SHARED] = Text::toT(Util::formatExactSize(user->getBytesShared()));
	columns[COLUMN_DESCRIPTION] = Text::toT(user->getDescription());
	columns[COLUMN_TAG] = Text::toT(user->getTag());
	columns[COLUMN_CONNECTION] = Text::toT(user->getConnection());
	columns[COLUMN_UPLOAD_SPEED] = uploadSpeed;
	columns[COLUMN_EMAIL] = Text::toT(user->getEmail());
	columns[COLUMN_CLIENTID] = Text::toT(user->getClientType());
	columns[COLUMN_VERSION] = Text::toT(user->getVersion());
	columns[COLUMN_MODE] = Text::toT(user->getMode());
	columns[COLUMN_HUBS] = Text::toT(user->getHubs());
	columns[COLUMN_SLOTS] = Text::toT(Util::toString(user->getSlots()));
	columns[COLUMN_IP] = Text::toT(user->getIp());
	columns[COLUMN_PK] = Text::toT(user->getPk());
	columns[COLUMN_LOCK] = Text::toT(user->getLock());
	columns[COLUMN_SUPPORTS] = Text::toT(user->getSupports());

	op = user->isSet(User::OP);

	return needsSort;
}
