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

#if !defined(FAVORITE_HUBS_FRM_H)
#define FAVORITE_HUBS_FRM_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "ExListViewCtrl.h"

#include "../client/FavoriteManager.h"

#define SERVER_MESSAGE_MAP 7

class FavoriteHubsFrame : public MDITabChildWindowImpl<FavoriteHubsFrame, RGB(0, 0, 0), IDR_FAVORITES>, public StaticFrame<FavoriteHubsFrame, ResourceManager::FAVORITE_HUBS, IDC_FAVORITES>,
	private FavoriteManagerListener, private SettingsManagerListener
{
public:
	typedef MDITabChildWindowImpl<FavoriteHubsFrame, RGB(0, 0, 0), IDR_FAVORITES> baseClass;

	FavoriteHubsFrame() : nosave(true), closed(false) { }
	~FavoriteHubsFrame() { }

	DECLARE_FRAME_WND_CLASS_EX(_T("FavoriteHubsFrame"), IDR_FAVORITES, 0, COLOR_3DFACE);
		
	BEGIN_MSG_MAP(FavoriteHubsFrame)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		COMMAND_ID_HANDLER(IDC_CONNECT, onClickedConnect)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_EDIT, onEdit)
		COMMAND_ID_HANDLER(IDC_NEWFAV, onNew)
		COMMAND_ID_HANDLER(IDC_MOVE_UP, onMoveUp);
		COMMAND_ID_HANDLER(IDC_MOVE_DOWN, onMoveDown);
		COMMAND_ID_HANDLER(IDC_OPEN_HUB_LOG, onOpenHubLog)
		COMMAND_ID_HANDLER(IDC_MANAGE_GROUPS, onManageGroups)
		NOTIFY_HANDLER(IDC_HUBLIST, NM_DBLCLK, onDoubleClickHublist)
		NOTIFY_HANDLER(IDC_HUBLIST, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_HUBLIST, LVN_ITEMCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_HUBLIST, LVN_COLUMNCLICK, onColumnClickHublist)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()
		
	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onDoubleClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onNew(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onMoveUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onMoveDown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onOpenHubLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onManageGroups(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onColumnClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	bool checkNick();
	void UpdateLayout(BOOL bResizeBars = TRUE);
	
	LRESULT onEnter(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
		openSelected();
		return 0;
	}

	LRESULT onClickedConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		openSelected();
		return 0;
	}
	
	
	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlHubs.SetFocus();
		return 0;
	}

private:

	enum {
		COLUMN_FIRST,
		COLUMN_NAME = COLUMN_FIRST,
		COLUMN_DESCRIPTION,
		COLUMN_NICK,
		COLUMN_PASSWORD,
		COLUMN_SERVER,
		COLUMN_USERDESCRIPTION,
		COLUMN_LAST
	};
	
	CButton ctrlConnect;
	CButton ctrlRemove;
	CButton ctrlNew;
	CButton ctrlProps;
	CButton ctrlUp;
	CButton ctrlDown;
	CButton ctrlManageGroups;
	OMenu hubsMenu;
	
	ExListViewCtrl ctrlHubs;

	bool nosave;
	bool closed;
	
	static int columnSizes[COLUMN_LAST];
	static int columnIndexes[COLUMN_LAST];

	void openSelected();
	
	void fillList() {
		ctrlHubs.SetRedraw(FALSE);

		bool old_nosave = nosave;
		nosave = true;

		ctrlHubs.DeleteAllItems(); 
		
		// sort groups
		set<tstring, noCaseStringLess> sorted_groups;
		const FavHubGroups& favHubGroups = FavoriteManager::getInstance()->getFavHubGroups();
		for(FavHubGroups::const_iterator i = favHubGroups.begin(), iend = favHubGroups.end(); i != iend; ++i)
			sorted_groups.insert(Text::toT(i->first));

		TStringList groups(sorted_groups.begin(), sorted_groups.end());
		groups.insert(groups.begin(), Util::emptyStringT); // default group (otherwise, hubs without group don't show up) 
		
		for(int i = 0; i < groups.size(); ++i) {
			// insert groups
			LVGROUP lg = {0};
			lg.cbSize = sizeof(lg);
			lg.iGroupId = i;
			lg.state = LVGS_NORMAL | (WinUtil::getOsMajor() >= 6 ? LVGS_COLLAPSIBLE : 0);
			lg.mask = LVGF_GROUPID | LVGF_HEADER | LVGF_STATE | LVGF_ALIGN;
			lg.uAlign = LVGA_HEADER_LEFT;

			// Header-title must be unicode (Convert if necessary)
			lg.pszHeader = (LPWSTR)groups[i].c_str();
			lg.cchHeader = groups[i].size();
			ctrlHubs.InsertGroup(i, &lg );
		}

		const FavoriteHubEntryList& fl = FavoriteManager::getInstance()->getFavoriteHubs();
		for(FavoriteHubEntryList::const_iterator i = fl.begin(); i != fl.end(); ++i) {
			const string& group = (*i)->getGroup();

			int index = 0;
			if(!group.empty()) {
				TStringList::const_iterator groupI = find(groups.begin() + 1, groups.end(), Text::toT(group));
				if(groupI != groups.end())
					index = groupI - groups.begin();
			}

			addEntry(*i, ctrlHubs.GetItemCount(), index);
		}

		nosave = old_nosave;
		ctrlHubs.SetRedraw(TRUE);
	}

	void addEntry(const FavoriteHubEntry* entry, int pos, int groupIndex);
	void handleMove(bool up);
	void on(FavoriteAdded, const FavoriteHubEntry* e)  throw() { fillList(); }
	void on(FavoriteRemoved, const FavoriteHubEntry* e) throw() { ctrlHubs.DeleteItem(ctrlHubs.find((LPARAM)e)); }
	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw();
};

#endif // !defined(FAVORITE_HUBS_FRM_H)

/**
 * @file
 * $Id$
 */
