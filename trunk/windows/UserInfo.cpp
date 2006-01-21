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

const tstring& UserInfo::getText(int col) const {
	return columns[col];
}

int UserInfo::compareItems(const UserInfo* a, const UserInfo* b, int col)  {
	if(a == NULL || b == NULL){
		dcdebug("UserInfo::compareItems NULL >:-[\n");
		return NULL;
	}
	if(col == COLUMN_NICK) {
		bool aOp = a->getIdentity().isOp();
		bool bOp = b->getIdentity().isOp();
		if(aOp && !bOp) {
			return -1;
		} else if(!aOp && bOp) {
			return 1;
		}
	}

	switch(col) {
		case COLUMN_SHARED:
		case COLUMN_EXACT_SHARED: return compare(a->getBytes(), b->getBytes());
		case COLUMN_SLOTS: return compare(Util::toInt(a->identity.get("SL")), Util::toInt(b->identity.get("SL")));
		case COLUMN_UPLOAD_SPEED: return compare(a->identity.getUser()->getLastDownloadSpeed(), b->identity.getUser()->getLastDownloadSpeed());
	}
	return lstrcmpi(a->columns[col].c_str(), b->columns[col].c_str());	
}

bool UserInfo::update(const Identity& identity, int sortCol) {
	bool needsSort = (getIdentity().isOp() != identity.isOp());

	tstring old;
	if(sortCol != -1)
		old = columns[sortCol];

	if (identity.getUser()->getLastDownloadSpeed() > 0) {
		columns[COLUMN_UPLOAD_SPEED] = Text::toT(Util::formatBytes(identity.getUser()->getLastDownloadSpeed()) + "/s");
	} else if(identity.getUser()->isSet(User::FIREBALL)) {
		columns[COLUMN_UPLOAD_SPEED] = _T(">= 100 kB/s");
	} else {
		columns[COLUMN_UPLOAD_SPEED] = _T("N/A");
	}

	bytes = identity.getBytesShared();

	columns[COLUMN_NICK] = Text::toT(identity.getNick());
	columns[COLUMN_SHARED] = Text::toT(Util::formatBytes(bytes));
	columns[COLUMN_EXACT_SHARED] = Text::toT(Util::formatExactSize(bytes));
	columns[COLUMN_DESCRIPTION] = Text::toT(identity.getDescription());
	columns[COLUMN_TAG] = Text::toT(identity.getTag());
	columns[COLUMN_EMAIL] = Text::toT(identity.getEmail());
	columns[COLUMN_CONNECTION] = Text::toT(identity.getConnection());
	columns[COLUMN_VERSION] = Text::toT(identity.get("VE"));
	columns[COLUMN_MODE] = Text::toT(identity.isTcpActive() ? "A" : "P");
	columns[COLUMN_HUBS] = Text::toT(Util::toString(Util::toInt(identity.get("HN"))+Util::toInt(identity.get("HR"))+Util::toInt(identity.get("HO"))));
	columns[COLUMN_SLOTS] = Text::toT(identity.get("SL"));
	columns[COLUMN_IP] = Text::toT(identity.getIp());
	columns[COLUMN_PK] = Text::toT(identity.getPk());
	columns[COLUMN_LOCK] = Text::toT(identity.getLock());
	columns[COLUMN_SUPPORTS] = Text::toT(identity.getSupports());
	columns[COLUMN_CLIENTID] = Text::toT(identity.getClientType());

	if(sortCol != -1) {
		needsSort = needsSort || (old != columns[sortCol]);
	}

	setIdentity(identity);
	return needsSort;
}
