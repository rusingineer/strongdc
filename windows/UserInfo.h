#ifndef USERINFO_H
#define USERINFO_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\client\FastAlloc.h"
#include "TypedListViewCtrl.h"
#include "WinUtil.h"

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
		//COLUMN_LIMITER, 
		COLUMN_ISP,
		COLUMN_IP,
		COLUMN_PK,
		COLUMN_LOCK,
		COLUMN_SUPPORTS,
		COLUMN_LAST
	};


public:
	UserInfo(const User::Ptr& u, const UserListColumns* pListColumns);

	const string& getText(int col) const;

	static int compareItems(const UserInfo* a, const UserInfo* b, int col);

	void update();

	GETSET(string, shared, Shared)
	GETSET(string, exactshare, ExactShare)
	GETSET(string, uuploadSpeed, UuploadSpeed);
	GETSET(bool, op, Op);

protected:
	const UserListColumns* m_pListColumns;
};

class UserListColumns {
public:
	UserListColumns();
	void ReadFromSetup();

	void WriteToSetup(TypedListViewCtrlCleanup<UserInfo, IDC_USERS>& UserList);
	void SetToList(TypedListViewCtrlCleanup<UserInfo, IDC_USERS>& UserList);
	void SwitchColumnVisibility(int nHardColumn, TypedListViewCtrlCleanup<UserInfo, IDC_USERS>& UserList);
	void SetColumnVisibility(int nHardColumn, TypedListViewCtrlCleanup<UserInfo, IDC_USERS>& UserList, bool bColumnIsOn);

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
