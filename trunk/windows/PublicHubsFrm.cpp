/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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

#include "PublicHubsFrm.h"
#include "HubFrame.h"
#include "WinUtil.h"

#include "../client/Client.h"

int PublicHubsFrame::columnIndexes[] = { COLUMN_NAME, COLUMN_DESCRIPTION, COLUMN_USERS, COLUMN_SERVER, 
	COLUMN_SHARED, COLUMN_COUNTRY, COLUMN_STATUS, COLUMN_MINSHARE, COLUMN_MINSLOTS, 
    COLUMN_MAXHUBS, COLUMN_MAXUSERS, COLUMN_RELIABILITY, COLUMN_RATING, COLUMN_PORT };

int PublicHubsFrame::columnSizes[] = { 200, 290, 70, 100, 70, 100, 70, 70, 70, 70, 70, 70, 100, 70 };

bool PublicHubsFrame::columnTypes[] = { 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1 };

static ResourceManager::Strings columnNames[] = { ResourceManager::HUB_NAME, ResourceManager::DESCRIPTION, 
ResourceManager::USERS, ResourceManager::HUB_ADDRESS, 
ResourceManager::SHARED, ResourceManager::COUNTRY, ResourceManager::STATUS, ResourceManager::MINSHARE, ResourceManager::MINSLOTS, 
ResourceManager::MAXHUBS, ResourceManager::MAXUSERS, ResourceManager::RELIABILITY, ResourceManager::RATING, ResourceManager::PORT };

LRESULT PublicHubsFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	int w[3] = { 0, 0, 0};
	ctrlStatus.SetParts(3, w);
	
	ctrlHubs.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, IDC_HUBLIST);
	DWORD styles = LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT;
	if (BOOLSETTING(SHOW_INFOTIPS))
		styles |= LVS_EX_INFOTIP;
	ctrlHubs.SetExtendedListViewStyle(styles);
	
	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(PUBLICHUBSFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(PUBLICHUBSFRAME_WIDTHS), COLUMN_LAST);
	
	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = (columnTypes[j]) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlHubs.InsertColumn(j, CSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlHubs.SetColumnOrderArray(COLUMN_LAST, columnIndexes);
	
	ctrlHubs.SetBkColor(WinUtil::bgColor);
	ctrlHubs.SetTextBkColor(WinUtil::bgColor);
	ctrlHubs.SetTextColor(WinUtil::textColor);
	
	ctrlHubs.setSort(COLUMN_USERS, ExListViewCtrl::SORT_INT, false);
	ctrlHubs.SetFocus();

	ctrlHub.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);
	ctrlHub.SetFont(WinUtil::systemFont);
	
	ctrlHubContainer.SubclassWindow(ctrlHub.m_hWnd);
	
	ctrlConnect.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_CONNECT);
	ctrlConnect.SetWindowText(CSTRING(CONNECT));
	ctrlConnect.SetFont(WinUtil::systemFont);

	ctrlRefresh.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_REFRESH);
	ctrlRefresh.SetWindowText(CSTRING(REFRESH));
	ctrlRefresh.SetFont(WinUtil::systemFont);

	ctrlAddress.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_GROUPBOX, WS_EX_TRANSPARENT);
	ctrlAddress.SetWindowText(CSTRING(MANUAL_ADDRESS));
	ctrlAddress.SetFont(WinUtil::systemFont);
	
	ctrlFilter.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);
	filterContainer.SubclassWindow(ctrlFilter.m_hWnd);
	ctrlFilter.SetFont(WinUtil::systemFont);
	
	ctrlFilterDesc.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_GROUPBOX, WS_EX_TRANSPARENT);
	ctrlFilterDesc.SetWindowText(CSTRING(FILTER));
	ctrlFilterDesc.SetFont(WinUtil::systemFont);

	HubManager::getInstance()->addListener(this);

	if(HubManager::getInstance()->isDownloading()) 
		ctrlStatus.SetText(0, CSTRING(DOWNLOADING_HUB_LIST));

	hubs = HubManager::getInstance()->getPublicHubs();
	if(hubs.empty())
		HubManager::getInstance()->refresh();

	updateList();
	
	hubsMenu.CreatePopupMenu();
	hubsMenu.AppendMenu(MF_STRING, IDC_CONNECT, CSTRING(CONNECT));
	hubsMenu.AppendMenu(MF_STRING, IDC_ADD, CSTRING(ADD_TO_FAVORITES));
	hubsMenu.AppendMenu(MF_STRING, IDC_COPY_HUB, CSTRING(COPY_HUB));
	hubsMenu.SetMenuDefaultItem(IDC_CONNECT);

	bHandled = FALSE;
	return TRUE;
}

LRESULT PublicHubsFrame::onDoubleClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	if(!checkNick())
		return 0;
	
		char buf[256];
	int item = -1;
	while((item = ctrlHubs.GetNextItem(item, LVNI_SELECTED)) != -1) {
		RecentHubEntry r;
		ctrlHubs.GetItemText(item, COLUMN_NAME, buf, 256);
		r.setName(buf);
		ctrlHubs.GetItemText(item, COLUMN_DESCRIPTION, buf, 256);
		r.setDescription(buf);
		ctrlHubs.GetItemText(item, COLUMN_USERS, buf, 256);
		r.setUsers(buf);
		ctrlHubs.GetItemText(item, COLUMN_SHARED, buf, 256);
		r.setShared(buf);
		ctrlHubs.GetItemText(item, COLUMN_SERVER, buf, 256);
		r.setServer(buf);
		HubManager::getInstance()->addRecent(r);
		HubFrame::openWindow(buf);
	}

	return 0;
}

LRESULT PublicHubsFrame::onEnter(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
	if(!checkNick())
		return 0;

		char buf[256];
	int item = -1;
	while((item = ctrlHubs.GetNextItem(item, LVNI_SELECTED)) != -1) {
		ctrlHubs.GetItemText(item, COLUMN_SERVER, buf, 256);
		HubFrame::openWindow(buf);
	}

	return 0;
}

LRESULT PublicHubsFrame::onClickedRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlHubs.DeleteAllItems();
	users = 0;
	visibleHubs = 0;
	ctrlStatus.SetText(0, CSTRING(DOWNLOADING_HUB_LIST));
	HubManager::getInstance()->refresh();

	return 0;
}

LRESULT PublicHubsFrame::onClickedConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!checkNick())
		return 0;

	if(ctrlHub.GetWindowTextLength() > 0) {
		char* hub = new char[ctrlHub.GetWindowTextLength()+1];
		ctrlHub.GetWindowText(hub, ctrlHub.GetWindowTextLength()+1);
		ctrlHub.SetWindowText("");
		string tmp = hub;
		delete hub;
		string::size_type i;
		while((i = tmp.find(' ')) != string::npos)
			tmp.erase(i, 1);


		RecentHubEntry r;
		r.setName("***");
		r.setDescription("***");
		r.setUsers("*");
		r.setShared("*");
		r.setServer(tmp);
		HubManager::getInstance()->addRecent(r);

		HubFrame::openWindow(tmp);
			
	} else {
		if(ctrlHubs.GetSelectedCount() >= 1) {
			char buf[256];
			int i = -1;
			while((i = ctrlHubs.GetNextItem(i, LVNI_SELECTED)) != -1) {
			ctrlHubs.GetItemText(i, COLUMN_SERVER, buf, 256);

			RecentHubEntry r;
			ctrlHubs.GetItemText(i, COLUMN_NAME, buf, 256);
			r.setName(buf);
			ctrlHubs.GetItemText(i, COLUMN_DESCRIPTION, buf, 256);
			r.setDescription(buf);
			ctrlHubs.GetItemText(i, COLUMN_USERS, buf, 256);
			r.setUsers(buf);
			ctrlHubs.GetItemText(i, COLUMN_SHARED, buf, 256);
			r.setShared(buf);
			ctrlHubs.GetItemText(i, COLUMN_SERVER, buf, 256);
			r.setServer(buf);
			HubManager::getInstance()->addRecent(r);

			HubFrame::openWindow(buf);
		}
	}
	}

	return 0;
}

LRESULT PublicHubsFrame::onFilterFocus(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	bHandled = true;
	ctrlFilter.SetFocus();
	return 0;
}

LRESULT PublicHubsFrame::onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!checkNick())
		return 0;
	
	char buf[256];
	
	if(ctrlHubs.GetSelectedCount() >= 1) {
		int i = -1;
		while((i = ctrlHubs.GetNextItem(i, LVNI_SELECTED)) != -1) {
		FavoriteHubEntry e;
		ctrlHubs.GetItemText(i, COLUMN_NAME, buf, 256);
		e.setName(buf);
		ctrlHubs.GetItemText(i, COLUMN_DESCRIPTION, buf, 256);
		e.setDescription(buf);
		ctrlHubs.GetItemText(i, COLUMN_SERVER, buf, 256);
		e.setServer(buf);
		HubManager::getInstance()->addFavorite(e);
	}
	}
	return 0;
}

LRESULT PublicHubsFrame::onChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	if(wParam == VK_RETURN && ctrlHub.GetWindowTextLength() > 0) {
		if(!checkNick()) {
			return 0;
		}
		
		char *hub = new char[ctrlHub.GetWindowTextLength()+1];
		ctrlHub.GetWindowText(hub, ctrlHub.GetWindowTextLength()+1);
		ctrlHub.SetWindowText("");
		string tmp = hub;
		delete hub;
		string::size_type i;
		while((i = tmp.find(' ')) != string::npos)
			tmp.erase(i, 1);

		RecentHubEntry r;
		r.setName("***");
		r.setDescription("***");
		r.setUsers("*");
		r.setShared("*");
		r.setServer(tmp);
		HubManager::getInstance()->addRecent(r);
		
		HubFrame::openWindow(tmp);
	} else {
		bHandled = FALSE;
	}
	return 0;
}

LRESULT PublicHubsFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		HubManager::getInstance()->removeListener(this);
		closed = true;
		CZDCLib::setButtonPressed(ID_FILE_CONNECT, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		WinUtil::saveHeaderOrder(ctrlHubs, SettingsManager::PUBLICHUBSFRAME_ORDER,
		SettingsManager::PUBLICHUBSFRAME_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);
		bHandled = FALSE;
		return 0;
	}
}
	
void PublicHubsFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
	
	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[3];
		ctrlStatus.GetClientRect(sr);
		int tmp = (sr.Width()) > 316 ? 216 : ((sr.Width() > 116) ? sr.Width()-100 : 16);
		
		w[0] = sr.right - tmp;
		w[1] = w[0] + (tmp-16)/2;
		w[2] = w[0] + (tmp-16);
		
		ctrlStatus.SetParts(3, w);
	}
	
	CRect rc = rect;
	rc.top += 2;
	rc.bottom -=(56);
	ctrlHubs.MoveWindow(rc);

	rc = rect;
	rc.top = rc.bottom - 52;
	rc.bottom = rc.top + 46;
	rc.right -= 100;
	rc.right -= ((rc.right - rc.left) / 2) + 1;
	ctrlFilterDesc.MoveWindow(rc);

	rc.top += 16;
	rc.bottom -= 8;
	rc.right -= 8;
	rc.left += 8;
	ctrlFilter.MoveWindow(rc);

	rc = rect;
	rc.top = rc.bottom - 52;
	rc.bottom = rc.top + 46;
	rc.right -= 100;
	rc.left += ((rc.right - rc.left) / 2) + 1;
	ctrlAddress.MoveWindow(rc);
	
	rc.top += 16;
	rc.bottom -= 8;
	rc.right -= 8;
	rc.left += 8;
	ctrlHub.MoveWindow(rc);
	
	rc = rect;
	rc.bottom -= 2;
	rc.top = rc.bottom - 22;
	rc.left = rc.right - 96;
	rc.right -= 2;
	ctrlConnect.MoveWindow(rc);

	rc.top -= 24;
	rc.bottom -= 24;
	ctrlRefresh.MoveWindow(rc);
}

bool PublicHubsFrame::checkNick() {
	if(SETTING(NICK).empty()) {
		MessageBox(CSTRING(ENTER_NICK), APPNAME " " VERSIONSTRING, MB_ICONSTOP | MB_OK);
		return false;
	}
	return true;
}

void PublicHubsFrame::updateList() {
	ctrlHubs.DeleteAllItems();
	users = 0;
	visibleHubs = 0;
	
	ctrlHubs.SetRedraw(FALSE);
	
	for(HubEntry::List::const_iterator i = hubs.begin(); i != hubs.end(); ++i) {
		if( filter.getPattern().empty() ||
			filter.match(i->getName()) ||
			filter.match(i->getDescription()) ||
			filter.match(i->getServer()) ) {

			StringList l;
			l.push_back(i->getName());
			l.push_back(i->getDescription());
			l.push_back(i->getUsers());
			l.push_back(i->getServer());
			// xml
			if (i->getShared().empty()) {
				l.push_back("");
			}else{
				l.push_back(Util::formatBytes(i->getShared()));
			}
			l.push_back(i->getCountry());
			l.push_back(i->getStatus());
			if (i->getMinshare().empty()) {
				l.push_back("");
			}else{
				l.push_back(Util::formatBytes(i->getMinshare()));
			}
			l.push_back(i->getMinslots());
			l.push_back(i->getMaxhubs());
			l.push_back(i->getMaxusers());
			l.push_back(i->getReliability()+" %");
			l.push_back(i->getRating());
			l.push_back(i->getPort());

			ctrlHubs.insert(ctrlHubs.GetItemCount(), l);
			visibleHubs++;
			users += Util::toInt(i->getUsers());
		}
	}
	
	ctrlHubs.SetRedraw(TRUE);
	ctrlHubs.resort();

	updateStatus();
}

void PublicHubsFrame::updateStatus() {
	ctrlStatus.SetText(1, (STRING(HUBS) + ": " + Util::toString(visibleHubs)).c_str());
	ctrlStatus.SetText(2, (STRING(USERS) + ": " + Util::toString(users)).c_str());
}

LRESULT PublicHubsFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(wParam == FINISHED) {
		hubs = HubManager::getInstance()->getPublicHubs();
		updateList();
		string* x = (string*)lParam;
		ctrlStatus.SetText(0, (STRING(HUB_LIST_DOWNLOADED) + " (" + (*x) + ")").c_str());
		delete x;
	} else if(wParam == STARTING) {
		string* x = (string*)lParam;
		ctrlStatus.SetText(0, (STRING(DOWNLOADING_HUB_LIST) + " (" + (*x) + ")").c_str());
		delete x;
	} else if(wParam == FAILED) {
		string* x = (string*)lParam;
		ctrlStatus.SetText(0, (STRING(DOWNLOAD_FAILED) + (*x) ).c_str());
		delete x;
	}
	return 0;
}

LRESULT PublicHubsFrame::onFilterChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	char* str;
	
	if(wParam == VK_RETURN) {
		str = new char[ctrlFilter.GetWindowTextLength()+1];
		ctrlFilter.GetWindowText(str, ctrlFilter.GetWindowTextLength()+1);
		filter = string(str, ctrlFilter.GetWindowTextLength());
		delete[] str;
		updateList();
	} else {
		bHandled = FALSE;
	}
	return 0;
}

LRESULT PublicHubsFrame::onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	RECT rc;                    // client area of window 
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 
	
	// Get the bounding rectangle of the client area. 
	if(ctrlHubs.GetSelectedCount() >= 1) {
		ctrlHubs.GetClientRect(&rc);
		ctrlHubs.ScreenToClient(&pt); 

		if (PtInRect(&rc, pt)) 
		{ 
			ctrlHubs.ClientToScreen(&pt);
			hubsMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

			return TRUE; 
		}
	}
	
	return FALSE; 
}

LRESULT PublicHubsFrame::onCopyHub(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlHubs.GetSelectedCount() >= 1) {
		char buf[256];
		string sbuf;
		int i = -1;
		while((i = ctrlHubs.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ctrlHubs.GetItemText(i, COLUMN_SERVER, buf, 256);
			sbuf += buf;
			sbuf += "\r\n";
		}
		WinUtil::setClipboard(sbuf);
	}
	return 0;
}

/**
 * @file
 * $Id$
 */
