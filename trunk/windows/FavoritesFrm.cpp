/* 
 * Copyright (C) 2001-2004 Jacek Sieka, j_s at telia com
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

#include "FavoritesFrm.h"
#include "HubFrame.h"
#include "FavHubProperties.h"
#include "CZDCLib.h"

#include "../client/ClientManager.h"
#include "../client/StringTokenizer.h"
#include "../client/version.h"

int FavoriteHubsFrame::columnIndexes[] = { COLUMN_NAME, COLUMN_DESCRIPTION, COLUMN_NICK, COLUMN_PASSWORD, COLUMN_SERVER, COLUMN_USERDESCRIPTION };
int FavoriteHubsFrame::columnSizes[] = { 200, 290, 125, 100, 100, 125 };
static ResourceManager::Strings columnNames[] = { ResourceManager::AUTO_CONNECT, ResourceManager::DESCRIPTION, 
ResourceManager::NICK, ResourceManager::PASSWORD, ResourceManager::SERVER, ResourceManager::USER_DESCRIPTION
};

LRESULT FavoriteHubsFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	ctrlHubs.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, IDC_HUBLIST);

	DWORD styles = LVS_EX_CHECKBOXES | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT;
	if (BOOLSETTING(SHOW_INFOTIPS))
		styles |= LVS_EX_INFOTIP;
	ctrlHubs.SetExtendedListViewStyle(styles);
	
	ctrlHubs.SetBkColor(WinUtil::bgColor);
	ctrlHubs.SetTextBkColor(WinUtil::bgColor);
	ctrlHubs.SetTextColor(WinUtil::textColor);
	
	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(FAVORITESFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(FAVORITESFRAME_WIDTHS), COLUMN_LAST);
	
	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = LVCFMT_LEFT;
		ctrlHubs.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlHubs.SetColumnOrderArray(COLUMN_LAST, columnIndexes);
	
	ctrlConnect.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_CONNECT);
	ctrlConnect.SetWindowText(CTSTRING(CONNECT));
	ctrlConnect.SetFont(WinUtil::font);

	ctrlNew.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_NEWFAV);
	ctrlNew.SetWindowText(CTSTRING(NEW));
	ctrlNew.SetFont(WinUtil::font);

	ctrlProps.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_EDIT);
	ctrlProps.SetWindowText(CTSTRING(PROPERTIES));
	ctrlProps.SetFont(WinUtil::font);

	ctrlRemove.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_REMOVE);
	ctrlRemove.SetWindowText(CTSTRING(REMOVE));
	ctrlRemove.SetFont(WinUtil::font);

	ctrlUp.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_MOVE_UP);
	ctrlUp.SetWindowText(CTSTRING(MOVE_UP));
	ctrlUp.SetFont(WinUtil::font);

	ctrlDown.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_MOVE_DOWN);
	ctrlDown.SetWindowText(CTSTRING(MOVE_DOWN));
	ctrlDown.SetFont(WinUtil::font);

	HubManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);
	updateList(HubManager::getInstance()->getFavoriteHubs());
	
	hubsMenu.CreatePopupMenu();
	hubsMenu.AppendMenu(MF_STRING, IDC_OPEN_HUB_LOG, CTSTRING(OPEN_HUB_LOG));
	hubsMenu.AppendMenu(MF_SEPARATOR);
	hubsMenu.AppendMenu(MF_STRING, IDC_CONNECT, CTSTRING(CONNECT));
	hubsMenu.AppendMenu(MF_STRING, IDC_NEWFAV, CTSTRING(NEW));
	hubsMenu.AppendMenu(MF_STRING, IDC_MOVE_UP, CTSTRING(MOVE_UP));
	hubsMenu.AppendMenu(MF_STRING, IDC_MOVE_DOWN, CTSTRING(MOVE_DOWN));
	hubsMenu.AppendMenu(MF_SEPARATOR);
	hubsMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
	hubsMenu.AppendMenu(MF_SEPARATOR);
	hubsMenu.AppendMenu(MF_STRING, IDC_EDIT, CTSTRING(PROPERTIES));
	hubsMenu.SetMenuDefaultItem(IDC_CONNECT);

	nosave = false;

	bHandled = FALSE;
	return TRUE;
}

void FavoriteHubsFrame::openSelected() {
	if(!checkNick())
		return;
	
	int i = -1;
	while( (i = ctrlHubs.GetNextItem(i, LVNI_SELECTED)) != -1) {
		FavoriteHubEntry* entry = (FavoriteHubEntry*)ctrlHubs.GetItemData(i);
		RecentHubEntry r;
		r.setName(entry->getName());
		r.setDescription(entry->getDescription());
		r.setUsers("*");
		r.setShared("*");
		r.setServer(entry->getServer());
		HubManager::getInstance()->addRecent(r);
		HubFrame::openWindow(Text::toT(entry->getServer())
			, Text::toT(entry->getRawOne())
			, Text::toT(entry->getRawTwo())
			, Text::toT(entry->getRawThree())
			, Text::toT(entry->getRawFour())
			, Text::toT(entry->getRawFive())
			, entry->getWindowPosX(), entry->getWindowPosY(), entry->getWindowSizeX(), entry->getWindowSizeY(), entry->getWindowType(), 
			entry->getChatUserSplit(), entry->getStealth(), entry->getUserListState());
	}
	return;
}

void FavoriteHubsFrame::addEntry(const FavoriteHubEntry* entry, int pos) {
	TStringList l;
	l.push_back(Text::toT(entry->getName()));
	l.push_back(Text::toT(entry->getDescription()));
	l.push_back(Text::toT(entry->getNick(false)));
	l.push_back(tstring(entry->getPassword().size(), '*'));
	l.push_back(Text::toT(entry->getServer()));
	l.push_back(Text::toT(entry->getUserDescription()));	
	l.push_back(Text::toT(entry->getRawOne()));
	l.push_back(Text::toT(entry->getRawTwo()));
	l.push_back(Text::toT(entry->getRawThree()));
	l.push_back(Text::toT(entry->getRawFour()));
	l.push_back(Text::toT(entry->getRawFive()));
	bool b = entry->getConnect();
	int i = ctrlHubs.insert(pos, l, 0, (LPARAM)entry);
	ctrlHubs.SetCheckState(i, b);
}

LRESULT FavoriteHubsFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	if((HWND)wParam == ctrlHubs) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		if(pt.x == -1 && pt.y == -1) {
			pt.x = pt.y = 0;
			ctrlHubs.ClientToScreen(&pt);
		}

		int status = ctrlHubs.GetSelectedCount() > 0 ? MFS_ENABLED : MFS_DISABLED;
		hubsMenu.EnableMenuItem(IDC_CONNECT, status);
		hubsMenu.EnableMenuItem(IDC_EDIT, status);
		hubsMenu.EnableMenuItem(IDC_MOVE_UP, status);
		hubsMenu.EnableMenuItem(IDC_MOVE_DOWN, status);

		string x;
		if (ctrlHubs.GetSelectedCount() == 1) {
			FavoriteHubEntry* f = (FavoriteHubEntry*)ctrlHubs.GetItemData(CZDCLib::getFirstSelectedIndex(ctrlHubs));
			x = f->getName();
		} else {
			x = "";
		}
		if (!x.empty())
			hubsMenu.InsertSeparatorFirst(x);

		hubsMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

		if (!x.empty())
			hubsMenu.RemoveFirstItem();

		return TRUE; 
	}
	
	bHandled = FALSE;
	return FALSE; 
}

LRESULT FavoriteHubsFrame::onDoubleClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	if(!checkNick())
		return 0;
	
	NMITEMACTIVATE* item = (NMITEMACTIVATE*) pnmh;

	if(item->iItem != -1) {
		FavoriteHubEntry* entry = (FavoriteHubEntry*)ctrlHubs.GetItemData(item->iItem);
		RecentHubEntry r;
		r.setName(entry->getName());
		r.setDescription(entry->getDescription());
		r.setUsers("*");
		r.setShared("*");
		r.setServer(entry->getServer());
		HubManager::getInstance()->addRecent(r);
		HubFrame::openWindow(Text::toT(entry->getServer())
			, Text::toT(entry->getRawOne())
			, Text::toT(entry->getRawTwo())
			, Text::toT(entry->getRawThree())
			, Text::toT(entry->getRawFour())
			, Text::toT(entry->getRawFive())	
			, entry->getWindowPosX(), entry->getWindowPosY(), entry->getWindowSizeX(), entry->getWindowSizeY(), entry->getWindowType(), 
			entry->getChatUserSplit(), entry->getStealth(), entry->getUserListState());
	}

	return 0;
}

LRESULT FavoriteHubsFrame::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	if(!BOOLSETTING(CONFIRM_HUB_REMOVAL) || MessageBox(CTSTRING(REALLY_REMOVE), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
		while( (i = ctrlHubs.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			HubManager::getInstance()->removeFavorite((FavoriteHubEntry*)ctrlHubs.GetItemData(i));
		}
	}
	return 0;
}

LRESULT FavoriteHubsFrame::onEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	if((i = ctrlHubs.GetNextItem(i, LVNI_SELECTED)) != -1)
	{
		FavoriteHubEntry* e = (FavoriteHubEntry*)ctrlHubs.GetItemData(i);
		dcassert(e != NULL);
		FavHubProperties dlg(e);
		if(dlg.DoModal(m_hWnd) == IDOK)
		{
			ctrlHubs.SetItemText(i, COLUMN_NAME, Text::toT(e->getName()).c_str());
			ctrlHubs.SetItemText(i, COLUMN_DESCRIPTION, Text::toT(e->getDescription()).c_str());
			ctrlHubs.SetItemText(i, COLUMN_SERVER, Text::toT(e->getServer()).c_str());
			ctrlHubs.SetItemText(i, COLUMN_NICK, Text::toT(e->getNick(false)).c_str());
			ctrlHubs.SetItemText(i, COLUMN_PASSWORD, tstring(e->getPassword().size(), '*').c_str());
			ctrlHubs.SetItemText(i, COLUMN_USERDESCRIPTION, Text::toT(e->getUserDescription()).c_str());
		}
	}
	return 0;
}

LRESULT FavoriteHubsFrame::onNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	FavoriteHubEntry e;
	FavHubProperties dlg(&e);

	if(dlg.DoModal((HWND)*this) == IDOK)
		HubManager::getInstance()->addFavorite(e);
	return 0;
}

bool FavoriteHubsFrame::checkNick() {
	if(SETTING(NICK).empty()) {
		MessageBox(CTSTRING(ENTER_NICK), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
		return false;
	}
	return true;
}

LRESULT FavoriteHubsFrame::onMoveUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	nosave = true;
	int j = ctrlHubs.GetItemCount();
	FavoriteHubEntry::List& fh = HubManager::getInstance()->getFavoriteHubs();
	ctrlHubs.SetRedraw(FALSE);
	for(int i = 1; i < j; ++i) {
		if(ctrlHubs.GetItemState(i, LVIS_SELECTED)) {
			FavoriteHubEntry* e = fh[i];
			swap(fh[i], fh[i-1]);
			ctrlHubs.DeleteItem(i);
			addEntry(e, i-1);
			ctrlHubs.SetItemState(i-1, LVIS_SELECTED, LVIS_SELECTED);
			ctrlHubs.EnsureVisible(i-1, FALSE);
		}
	}
	ctrlHubs.SetRedraw(TRUE);
	nosave = false;
	HubManager::getInstance()->save();
	return 0;
}

LRESULT FavoriteHubsFrame::onMoveDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int j = ctrlHubs.GetItemCount() - 2;
	FavoriteHubEntry::List& fh = HubManager::getInstance()->getFavoriteHubs();

	nosave = true;
	ctrlHubs.SetRedraw(FALSE);
	for(int i = j; i >= 0; --i) {
		if(ctrlHubs.GetItemState(i, LVIS_SELECTED)) {
			FavoriteHubEntry* e = fh[i];
			swap(fh[i], fh[i+1]);
			addEntry(e, i+2);
			ctrlHubs.SetItemState(i+2, LVIS_SELECTED, LVIS_SELECTED);
			ctrlHubs.DeleteItem(i);
			ctrlHubs.EnsureVisible(i, FALSE);
		}
	}
	ctrlHubs.SetRedraw(TRUE);
	nosave = false;
	HubManager::getInstance()->save();
	return 0;
}

LRESULT FavoriteHubsFrame::onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* l = (NMITEMACTIVATE*)pnmh;
	if(!nosave && l->iItem != -1 && ((l->uNewState & LVIS_STATEIMAGEMASK) != (l->uOldState & LVIS_STATEIMAGEMASK))) {
		FavoriteHubEntry* f = (FavoriteHubEntry*)ctrlHubs.GetItemData(l->iItem);
		f->setConnect(ctrlHubs.GetCheckState(l->iItem) != FALSE);
		HubManager::getInstance()->save();
	}
	return 0;
}

LRESULT FavoriteHubsFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		HubManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		closed = true;		
		CZDCLib::setButtonPressed(IDC_FAVORITES, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		WinUtil::saveHeaderOrder(ctrlHubs, SettingsManager::FAVORITESFRAME_ORDER, 
		SettingsManager::FAVORITESFRAME_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);

		bHandled = FALSE;
		return 0;
	}	
}

void FavoriteHubsFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */)
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
	ctrlNew.MoveWindow(rc);

	rc.OffsetRect(bwidth+2, 0);
	ctrlProps.MoveWindow(rc);

	rc.OffsetRect(bwidth+2, 0);
	ctrlRemove.MoveWindow(rc);

	rc.OffsetRect(bspace + bwidth +2, 0);
	ctrlUp.MoveWindow(rc);

	rc.OffsetRect(bwidth+2, 0);
	ctrlDown.MoveWindow(rc);

	rc.OffsetRect(bspace + bwidth +2, 0);
	ctrlConnect.MoveWindow(rc);

}

LRESULT FavoriteHubsFrame::onOpenHubLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlHubs.GetSelectedCount() == 1) {
		int i = ctrlHubs.GetNextItem(-1, LVNI_SELECTED);
		FavoriteHubEntry* entry = (FavoriteHubEntry*)ctrlHubs.GetItemData(i);
		StringMap params;
		params["hub"] = entry->getName();
		params["hubaddr"] = entry->getServer();
		params["mynick"] = entry->getNick(); 
		tstring filename = Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_MAIN_CHAT), params)));
		if(Util::fileExists(Text::fromT(filename))){
			ShellExecute(NULL, NULL, filename.c_str(), NULL, NULL, SW_SHOWNORMAL);
		} else {
			MessageBox(CTSTRING(NO_LOG_FOR_HUB), CTSTRING(NO_LOG_FOR_HUB), MB_OK );	  
		}
	}
	return 0;
}

void FavoriteHubsFrame::on(SettingsManagerListener::Save, SimpleXML* /*xml*/) throw() {
	bool refresh = false;
	if(ctrlHubs.GetBkColor() != WinUtil::bgColor) {
		ctrlHubs.SetBkColor(WinUtil::bgColor);
		ctrlHubs.SetTextBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlHubs.GetTextColor() != WinUtil::textColor) {
		ctrlHubs.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

/**
 * @file
 * $Id$
 */
