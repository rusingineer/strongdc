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

class UserListColumns;
friend struct CompareItems;

class UserInfo : public UserInfoBase, public FastAlloc<UserInfo> {

public:
	enum {
		COLUMN_FIRST, 
		COLUMN_NICK = COLUMN_FIRST, 
		COLUMN_SHARED, 
		COLUMN_EXACT_SHARED, 
		COLUMN_DESCRIPTION, 
		COLUMN_TAG,
		COLUMN_CONNECTION,
		COLUMN_UPLOAD_SPEED,
		COLUMN_EMAIL, 
		COLUMN_CLIENTID, 
		COLUMN_VERSION, 
		COLUMN_MODE, 
		COLUMN_HUBS, 
		COLUMN_SLOTS, 
		COLUMN_ISP,
		COLUMN_IP,
		COLUMN_PK,
		COLUMN_LOCK,
		COLUMN_SUPPORTS,
		COLUMN_LAST
	};
	friend struct CompareItems;

public:
	UserInfo(const User::Ptr& u, const UserListColumns* pListColumns);
	UserInfo(const User::Ptr& u) : UserInfoBase(u), op(false) {
		update();
	};

	const tstring& getText(int col) const;

	static int compareItems(const UserInfo* a, const UserInfo* b, int col);

	void update();

	tstring columns[COLUMN_LAST];
	GETSET(bool, op, Op);

protected:
	const UserListColumns* m_pListColumns;
};

class UserListColumns {
public:
	UserListColumns();
	void ReadFromSetup();
	
	static ResourceManager::Strings def_columnNames[];
	void WriteToSetup(TypedListViewCtrl<UserInfo, IDC_USERS>& UserList);
	void SetToList(TypedListViewCtrl<UserInfo, IDC_USERS>& UserList);
	void SwitchColumnVisibility(int nHardColumn, TypedListViewCtrl<UserInfo, IDC_USERS>& UserList);
	void SetColumnVisibility(int nHardColumn, TypedListViewCtrl<UserInfo, IDC_USERS>& UserList, bool bColumnIsOn);

	bool IsColumnUsed(int nHardColumn) const;
	int RemapDataColumnToListColumn(int nDataCol) const;
	int RemapListColumnToDataColumn(int nDataCol) const;
	
protected:
	int m_nColumnSizes[UserInfo::COLUMN_LAST];
	int m_nColumnIndexes[UserInfo::COLUMN_LAST];
	bool m_bColumnUsing[UserInfo::COLUMN_LAST];
	int m_nColumnIdxData[UserInfo::COLUMN_LAST];

	void RecalcIdxData();
};

#endif //USERINFO_H
