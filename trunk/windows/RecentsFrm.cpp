/* 
 * Copyright (C) 2001-2003 sickb0y, sickb0y@p2pitalia.com
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

#include "RecentsFrm.h"
#include "HubFrame.h"

#include "../client/ClientManager.h"
#include "../client/StringTokenizer.h"
#include "WinUtil.h"
//#include "OToolBar.h"

RecentHubsFrame* RecentHubsFrame::frame = NULL;

int RecentHubsFrame::columnIndexes[] = { COLUMN_NAME, COLUMN_DESCRIPTION, COLUMN_USERS, COLUMN_SERVER };

int RecentHubsFrame::columnSizes[] = { 200, 290, 50, 100 };

static ResourceManager::Strings columnNames[] = { ResourceManager::HUB_NAME, ResourceManager::DESCRIPTION, 
ResourceManager::USERS, ResourceManager::HUB_ADDRESS };

LRESULT RecentHubsFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	// Only one of this window please...
	dcassert(frame == NULL);
	frame = this;
	
	SetWindowText(CSTRING(RECENT_HUBS));
	
	ctrlHubs.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL, WS_EX_CLIENTEDGE, IDC_HUBLIST);
	
	DWORD styles = LVS_EX_HEADERDRAGDROP;
	if (BOOLSETTING(FULL_ROW_SELECT))
		styles |= LVS_EX_FULLROWSELECT;
	if (BOOLSETTING(SHOW_INFOTIPS))
		styles |= LVS_EX_INFOTIP;
	ctrlHubs.SetExtendedListViewStyle(styles);
	
	ctrlHubs.SetBkColor(WinUtil::bgColor);
	ctrlHubs.SetTextBkColor(WinUtil::bgColor);
	ctrlHubs.SetTextColor(WinUtil::textColor);
	
	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(RECENTFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(RECENTFRAME_WIDTHS), COLUMN_LAST);
	
	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = LVCFMT_LEFT;
		ctrlHubs.InsertColumn(j, CSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlHubs.SetColumnOrderArray(COLUMN_LAST, columnIndexes);
	
	ctrlConnect.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_CONNECT);
	ctrlConnect.SetWindowText(CSTRING(CONNECT));
	ctrlConnect.SetFont(WinUtil::font);

	ctrlRemove.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_REMOVE);
	ctrlRemove.SetWindowText(CSTRING(REMOVE));
	ctrlRemove.SetFont(WinUtil::font);

	ctrlRemoveAll.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_REMOVE_ALL);
	ctrlRemoveAll.SetWindowText(CSTRING(REMOVE_ALL));
	ctrlRemoveAll.SetFont(WinUtil::font);

	HubManager::getInstance()->addListener(this);
	updateList(HubManager::getInstance()->getRecentHubs());
	
	hubsMenu.CreatePopupMenu();
	hubsMenu.AppendMenu(MF_STRING, IDC_CONNECT, CSTRING(CONNECT));
	hubsMenu.AppendMenu(MF_STRING, IDC_ADD, CSTRING(ADD_TO_FAVORITES));
	hubsMenu.AppendMenu(MF_STRING, IDC_REMOVE, CSTRING(REMOVE));
	hubsMenu.AppendMenu(MF_STRING, IDC_REMOVE_ALL, CSTRING(REMOVE_ALL));

	nosave = false;

	m_hMenu = WinUtil::mainMenu;

	bHandled = FALSE;
	return TRUE;
}

LRESULT RecentHubsFrame::onDoubleClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	
	NMITEMACTIVATE* item = (NMITEMACTIVATE*) pnmh;

	if(item->iItem != -1) {
		RecentHubEntry* entry = (RecentHubEntry*)ctrlHubs.GetItemData(item->iItem);
		HubFrame::openWindow(entry->getServer());
	}
	return 0;
}

LRESULT RecentHubsFrame::onEnter(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
	
	int item = ctrlHubs.GetNextItem(-1, LVNI_FOCUSED);

	if(item != -1) {
		RecentHubEntry* entry = (RecentHubEntry*)ctrlHubs.GetItemData(item);
		HubFrame::openWindow(entry->getName(), entry->getServer(), entry->getDescription(), entry->getUsers());
	}

	return 0;
}

LRESULT RecentHubsFrame::onClickedConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	int i = -1;
	while( (i = ctrlHubs.GetNextItem(i, LVNI_SELECTED)) != -1) {
		RecentHubEntry* entry = (RecentHubEntry*)ctrlHubs.GetItemData(i);
		HubFrame::openWindow(entry->getServer());
	}
	return 0;
}

LRESULT RecentHubsFrame::onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	char buf[256];
	
	if(ctrlHubs.GetSelectedCount() == 1) {
		int i = ctrlHubs.GetNextItem(-1, LVNI_SELECTED);
		FavoriteHubEntry e;
		ctrlHubs.GetItemText(i, COLUMN_NAME, buf, 256);
		e.setName(buf);
		ctrlHubs.GetItemText(i, COLUMN_DESCRIPTION, buf, 256);
		e.setDescription(buf);
		ctrlHubs.GetItemText(i, COLUMN_SERVER, buf, 256);
		e.setServer(buf);
		HubManager::getInstance()->addFavorite(e);
	}
	return 0;
}

LRESULT RecentHubsFrame::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlHubs.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		HubManager::getInstance()->removeRecent((RecentHubEntry*)ctrlHubs.GetItemData(i));
	}
	return 0;
}

LRESULT RecentHubsFrame::onRemoveAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlHubs.DeleteAllItems();
	HubManager::getInstance()->removeallRecent();
	return 0;
}

LRESULT RecentHubsFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		closed = true;		
		HubManager::getInstance()->removeListener(this);

		CZDCLib::setButtonPressed(IDC_RECENTS, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		WinUtil::saveHeaderOrder(ctrlHubs, SettingsManager::RECENTFRAME_ORDER, 
		SettingsManager::RECENTFRAME_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);

		MDIDestroy(m_hWnd);
		return 0;
	}	
}

void RecentHubsFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */)
{
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	CRect rc = rect;
	rc.bottom -=28;
	ctrlHubs.MoveWindow(rc);

	const long bwidth = 90;
	const long bspace = 10;

	rc = rect;
	rc.bottom -= 2;
	rc.top = rc.bottom - 22;

	rc.left = 2;
	rc.right = rc.left + bwidth;
	ctrlConnect.MoveWindow(rc);

	rc.OffsetRect(bspace + bwidth +2, 0);
	ctrlRemove.MoveWindow(rc);

	rc.OffsetRect(bwidth+2, 0);
	ctrlRemoveAll.MoveWindow(rc);
}

void RecentHubsFrame::onAction(HubManagerListener::Types type, RecentHubEntry* entry) throw() {
	switch(type) {
		case HubManagerListener::RECENT_ADDED: addEntry(entry, ctrlHubs.GetItemCount()); break;
		case HubManagerListener::RECENT_REMOVED: ctrlHubs.DeleteItem(ctrlHubs.find((LPARAM)entry)); break;
	}
};

/**
 * @file
 * $Id$
 */
