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
		case COLUMN_EXACT_SHARED: return compare(a->identity.getBytesShared(), b->identity.getBytesShared());
		case COLUMN_SLOTS: return compare(Util::toInt(a->identity.get("SL")), Util::toInt(b->identity.get("SL")));
		case COLUMN_HUBS: return compare(Util::toInt(a->identity.get("HN"))+Util::toInt(a->identity.get("HR"))+Util::toInt(a->identity.get("HO")), Util::toInt(b->identity.get("HN"))+Util::toInt(b->identity.get("HR"))+Util::toInt(b->identity.get("HO")));
		case COLUMN_UPLOAD_SPEED: return compare(a->identity.getUser()->getLastDownloadSpeed(), b->identity.getUser()->getLastDownloadSpeed());
	}
	return lstrcmpi(a->getText(col).c_str(), b->getText(col).c_str());	
}

bool UserInfo::update(const Identity& identity, int sortCol) {
	bool needsSort = (getIdentity().isOp() != identity.isOp());

	tstring old;
	if(sortCol != -1)
		old = getText(sortCol);

	if (identity.getUser()->getLastDownloadSpeed() > 0) {
		setText(COLUMN_UPLOAD_SPEED, Text::toT(Util::formatBytes(identity.getUser()->getLastDownloadSpeed()) + "/s"));
	} else if(identity.getUser()->isSet(User::FIREBALL)) {
		setText(COLUMN_UPLOAD_SPEED, _T(">= 100 kB/s"));
	} else {
		setText(COLUMN_UPLOAD_SPEED, _T("N/A"));
	}

	string hn = identity.get("HN");
	string hr = identity.get("HR");
	string ho = identity.get("HO");

	setText(COLUMN_NICK, Text::toT(identity.getNick()));
	setText(COLUMN_SHARED, Text::toT(Util::formatBytes(identity.getBytesShared())));
	setText(COLUMN_EXACT_SHARED, Text::toT(Util::formatExactSize(identity.getBytesShared())));
	setText(COLUMN_DESCRIPTION, Text::toT(identity.getDescription()));
	setText(COLUMN_TAG, Text::toT(identity.getTag()));
	setText(COLUMN_EMAIL, Text::toT(identity.getEmail()));
	setText(COLUMN_CONNECTION, Text::toT(identity.getConnection()));
	setText(COLUMN_VERSION, Text::toT(identity.get("CT").empty() ? identity.get("VE") : identity.get("CT")));
	setText(COLUMN_MODE, identity.isTcpActive() ? _T("A") : _T("P"));
	setText(COLUMN_HUBS, (hn.empty() && hr.empty() && ho.empty()) ? Util::emptyStringT : Text::toT(hn + "/" + hr + "/" + ho));
	setText(COLUMN_SLOTS, Text::toT(identity.get("SL")));
	setText(COLUMN_IP, Text::toT(identity.getIp()));
	setText(COLUMN_PK, Text::toT(identity.getUser()->getPk()));

	if(sortCol != -1) {
		needsSort = needsSort || (old != getText(sortCol));
	}

	setIdentity(identity);
	return needsSort;
}
