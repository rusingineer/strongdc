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

#ifndef USERINFO_H
#define USERINFO_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TypedListViewCtrl.h"
#include "WinUtil.h"

#include "../client/FastAlloc.h"

struct UpdateInfo {
	UpdateInfo() { }
	UpdateInfo(const OnlineUser& ou) : user(ou.getUser()), identity(ou.getIdentity()) { }

	User::Ptr user;
	Identity identity;
};

class UserInfo : public UserInfoBase, public FastAlloc<UserInfo> {
friend struct CompareItems;
public:
	enum {
		COLUMN_FIRST,
		COLUMN_NICK = COLUMN_FIRST, 
		COLUMN_SHARED, 
		COLUMN_EXACT_SHARED, 
		COLUMN_DESCRIPTION, 
		COLUMN_TAG,
		COLUMN_CONNECTION, 
		COLUMN_EMAIL, 
		COLUMN_CLIENTID, 
		COLUMN_VERSION, 
		COLUMN_MODE, 
		COLUMN_HUBS, 
		COLUMN_SLOTS,
		COLUMN_UPLOAD_SPEED, 
		COLUMN_IP,
		COLUMN_PK, 
		COLUMN_LOCK, 
		COLUMN_SUPPORTS,
		COLUMN_LAST
	};
	UserInfo(const UpdateInfo& u, Client* c) : UserInfoBase(u.user), client(c) { 
		update(u.identity, -1); 
	};
	const tstring& getText(int col) const;

	static int compareItems(const UserInfo* a, const UserInfo* b, int col);
	int imageIndex() {
		return WinUtil::getImage(identity, client);
	}

	bool update(const Identity& identity, int sortCol);

	tstring columns[COLUMN_LAST];
	GETSET(Identity, identity, Identity);
	GETSET(int64_t, bytes, Bytes);
	GETSET(Client*, client, Client);
};

#endif //USERINFO_H
