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

#include "PublicHubsFrm.h"
#include "HubFrame.h"
#include "WinUtil.h"

#include "../client/Client.h"

int PublicHubsFrame::columnIndexes[] = { 
	COLUMN_NAME,
	COLUMN_DESCRIPTION,
	COLUMN_USERS,
	COLUMN_SERVER,
	COLUMN_COUNTRY,
	COLUMN_SHARED,
	COLUMN_MINSHARE,
	COLUMN_MINSLOTS,
	COLUMN_MAXHUBS,
	COLUMN_MAXUSERS,
	COLUMN_RELIABILITY,
	COLUMN_RATING
 };

int PublicHubsFrame::columnSizes[] = { 200, 290, 50, 100, 100, 100, 100, 100, 100, 100, 100, 100 };

static ResourceManager::Strings columnNames[] = { 
	ResourceManager::HUB_NAME, 
	ResourceManager::DESCRIPTION, 
	ResourceManager::USERS, 
	ResourceManager::HUB_ADDRESS,
	ResourceManager::COUNTRY,
	ResourceManager::SHARED,
	ResourceManager::MIN_SHARE,
	ResourceManager::MIN_SLOTS,
	ResourceManager::MAX_HUBS,
	ResourceManager::MAX_USERS,
	ResourceManager::RELIABILITY,
	ResourceManager::RATING,
	
};

LRESULT PublicHubsFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	int w[3] = { 0, 0, 0};
	ctrlStatus.SetParts(3, w);
	
	ctrlHubs.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL, WS_EX_CLIENTEDGE, IDC_HUBLIST);
	DWORD styles = LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT;
	if (BOOLSETTING(SHOW_INFOTIPS))
		styles |= LVS_EX_INFOTIP;
	ctrlHubs.SetExtendedListViewStyle(styles);
	
	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(PUBLICHUBSFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(PUBLICHUBSFRAME_WIDTHS), COLUMN_LAST);
	
	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_USERS) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlHubs.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
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
	ctrlConnect.SetWindowText(CTSTRING(CONNECT));
	ctrlConnect.SetFont(WinUtil::systemFont);

	ctrlRefresh.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_REFRESH);
	ctrlRefresh.SetWindowText(CTSTRING(REFRESH));
	ctrlRefresh.SetFont(WinUtil::systemFont);

	ctrlAddress.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_GROUPBOX, WS_EX_TRANSPARENT);
	ctrlAddress.SetWindowText(CTSTRING(MANUAL_ADDRESS));
	ctrlAddress.SetFont(WinUtil::systemFont);
	
	ctrlFilter.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);
	filterContainer.SubclassWindow(ctrlFilter.m_hWnd);
	ctrlFilter.SetFont(WinUtil::systemFont);
	
	ctrlFilterDesc.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_GROUPBOX, WS_EX_TRANSPARENT);
	ctrlFilterDesc.SetWindowText(CTSTRING(FILTER));
	ctrlFilterDesc.SetFont(WinUtil::systemFont);

	HubManager::getInstance()->addListener(this);

	if(HubManager::getInstance()->isDownloading()) 
		ctrlStatus.SetText(0, CTSTRING(DOWNLOADING_HUB_LIST));

	hubs = HubManager::getInstance()->getPublicHubs();
	if(hubs.empty())
		HubManager::getInstance()->refresh();

	updateList();
	
	hubsMenu.CreatePopupMenu();
	hubsMenu.AppendMenu(MF_STRING, IDC_CONNECT, CTSTRING(CONNECT));
	hubsMenu.AppendMenu(MF_STRING, IDC_ADD, CTSTRING(ADD_TO_FAVORITES));
	hubsMenu.AppendMenu(MF_STRING, IDC_COPY_HUB, CTSTRING(COPY_HUB));
	hubsMenu.SetMenuDefaultItem(IDC_CONNECT);

	bHandled = FALSE;
	return TRUE;
}

LRESULT PublicHubsFrame::onColumnClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLISTVIEW* l = (NMLISTVIEW*)pnmh;
	if(l->iSubItem == ctrlHubs.getSortColumn()) {
		if (!ctrlHubs.isAscending())
			ctrlHubs.setSort(-1, ctrlHubs.getSortType());
		else
			ctrlHubs.setSortDirection(false);
	} else {
		// BAH, sorting on bytes will break of course...oh well...later...
		if(l->iSubItem == COLUMN_USERS || l->iSubItem == COLUMN_MINSLOTS ||l->iSubItem == COLUMN_MAXHUBS || l->iSubItem == COLUMN_MAXUSERS) {
			ctrlHubs.setSort(l->iSubItem, ExListViewCtrl::SORT_INT);
		} else if(l->iSubItem == COLUMN_SHARED || l->iSubItem == COLUMN_MINSHARE || l->iSubItem == COLUMN_RELIABILITY) {
			ctrlHubs.setSort(l->iSubItem, ExListViewCtrl::SORT_FLOAT);
		} else {
			ctrlHubs.setSort(l->iSubItem, ExListViewCtrl::SORT_STRING_NOCASE);
		}
	}
	return 0;
}
	
LRESULT PublicHubsFrame::onDoubleClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	if(!checkNick())
		return 0;
	
	NMITEMACTIVATE* item = (NMITEMACTIVATE*) pnmh;

	if(item->iItem != -1) {
		TCHAR buf[256];
		
		RecentHubEntry r;
		ctrlHubs.GetItemText(item->iItem, COLUMN_NAME, buf, 256);
		r.setName(Text::fromT(buf));
		ctrlHubs.GetItemText(item->iItem, COLUMN_DESCRIPTION, buf, 256);
		r.setDescription(Text::fromT(buf));
		ctrlHubs.GetItemText(item->iItem, COLUMN_USERS, buf, 256);
		r.setUsers(Text::fromT(buf));
		ctrlHubs.GetItemText(item->iItem, COLUMN_SHARED, buf, 256);
		r.setShared(Text::fromT(buf));
		ctrlHubs.GetItemText(item->iItem, COLUMN_SERVER, buf, 256);
		r.setServer(Text::fromT(buf));
		HubManager::getInstance()->addRecent(r);
		HubFrame::openWindow(buf);
	}

	return 0;
}

LRESULT PublicHubsFrame::onEnter(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
	if(!checkNick())
		return 0;

	int item = ctrlHubs.GetNextItem(-1, LVNI_FOCUSED);
	if(item != -1) {
		TCHAR buf[256];

		ctrlHubs.GetItemText(item, COLUMN_SERVER, buf, 256);
		HubFrame::openWindow(buf);
	}

	return 0;
}

LRESULT PublicHubsFrame::onClickedRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlHubs.DeleteAllItems();
	users = 0;
	visibleHubs = 0;
	ctrlStatus.SetText(0, CTSTRING(DOWNLOADING_HUB_LIST));
	HubManager::getInstance()->refresh();

	return 0;
}

LRESULT PublicHubsFrame::onClickedConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!checkNick())
		return 0;

	if(ctrlHub.GetWindowTextLength() > 0) {
		TCHAR* hub = new TCHAR[ctrlHub.GetWindowTextLength()+1];
		ctrlHub.GetWindowText(hub, ctrlHub.GetWindowTextLength()+1);
		ctrlHub.SetWindowText(_T(""));
		tstring tmp = hub;
		delete[] hub;
		string::size_type i;
		while((i = tmp.find(' ')) != string::npos)
			tmp.erase(i, 1);


		RecentHubEntry r;
		r.setName("***");
		r.setDescription("***");
		r.setUsers("*");
		r.setShared("*");
		r.setServer(Text::fromT(tmp));
		HubManager::getInstance()->addRecent(r);

		HubFrame::openWindow(tmp);
			
	} else {
		if(ctrlHubs.GetSelectedCount() == 1) {
			TCHAR buf[256];
			int i = ctrlHubs.GetNextItem(-1, LVNI_SELECTED);
				ctrlHubs.GetItemText(i, COLUMN_SERVER, buf, 256);

				RecentHubEntry r;
				ctrlHubs.GetItemText(i, COLUMN_NAME, buf, 256);
			r.setName(Text::fromT(buf));
				ctrlHubs.GetItemText(i, COLUMN_DESCRIPTION, buf, 256);
			r.setDescription(Text::fromT(buf));
				ctrlHubs.GetItemText(i, COLUMN_USERS, buf, 256);
			r.setUsers(Text::fromT(buf));
				ctrlHubs.GetItemText(i, COLUMN_SHARED, buf, 256);
			r.setShared(Text::fromT(buf));
				ctrlHubs.GetItemText(i, COLUMN_SERVER, buf, 256);
			r.setServer(Text::fromT(buf));
				HubManager::getInstance()->addRecent(r);
	
				HubFrame::openWindow(buf);
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
	
	TCHAR buf[256];
	
	if(ctrlHubs.GetSelectedCount() == 1) {
		int i = ctrlHubs.GetNextItem(-1, LVNI_SELECTED);
			FavoriteHubEntry e;
			ctrlHubs.GetItemText(i, COLUMN_NAME, buf, 256);
		e.setName(Text::fromT(buf));
			ctrlHubs.GetItemText(i, COLUMN_DESCRIPTION, buf, 256);
		e.setDescription(Text::fromT(buf));
			ctrlHubs.GetItemText(i, COLUMN_SERVER, buf, 256);
		e.setServer(Text::fromT(buf));
			HubManager::getInstance()->addFavorite(e);
		}
	return 0;
}

LRESULT PublicHubsFrame::onChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	if(wParam == VK_RETURN && ctrlHub.GetWindowTextLength() > 0) {
		if(!checkNick()) {
			return 0;
		}
		
		TCHAR *hub = new TCHAR[ctrlHub.GetWindowTextLength()+1];
		ctrlHub.GetWindowText(hub, ctrlHub.GetWindowTextLength()+1);
		ctrlHub.SetWindowText(_T(""));
		tstring tmp = hub;
		delete hub;
		string::size_type i;
		while((i = tmp.find(' ')) != string::npos)
			tmp.erase(i, 1);

		RecentHubEntry r;
		r.setName("***");
		r.setDescription("***");
		r.setUsers("*");
		r.setShared("*");
		r.setServer(Text::fromT(tmp));
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
		MessageBox(CTSTRING(ENTER_NICK), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
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

			TStringList l;
			l.resize(COLUMN_LAST);
			l[COLUMN_NAME] = Text::toT(i->getName());
			l[COLUMN_DESCRIPTION] = Text::toT(i->getDescription());
			l[COLUMN_USERS] = Text::toT(Util::toString(i->getUsers()));
			l[COLUMN_SERVER] = Text::toT(i->getServer());
			l[COLUMN_COUNTRY] = Text::toT(i->getCountry());
			l[COLUMN_SHARED] = Text::toT(Util::formatBytes(i->getShared()));
			l[COLUMN_MINSHARE] = Text::toT(Util::formatBytes(i->getMinShare()));
			l[COLUMN_MINSLOTS] = Text::toT(Util::toString(i->getMinSlots()));
			l[COLUMN_MAXHUBS] = Text::toT(Util::toString(i->getMaxHubs()));
			l[COLUMN_MAXUSERS] = Text::toT(Util::toString(i->getMaxUsers()));
			l[COLUMN_RELIABILITY] = Text::toT(Util::toString(i->getReliability()));
			l[COLUMN_RATING] = Text::toT(i->getRating());
			ctrlHubs.insert(ctrlHubs.GetItemCount(), l);
			visibleHubs++;
			users += i->getUsers();
		}
	}
	
	ctrlHubs.SetRedraw(TRUE);
	ctrlHubs.resort();

	updateStatus();
}

void PublicHubsFrame::updateStatus() {
	ctrlStatus.SetText(1, Text::toT(STRING(HUBS) + ": " + Util::toString(visibleHubs)).c_str());
	ctrlStatus.SetText(2, Text::toT(STRING(USERS) + ": " + Util::toString(users)).c_str());
}

LRESULT PublicHubsFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(wParam == FINISHED) {
		hubs = HubManager::getInstance()->getPublicHubs();
		updateList();
		tstring* x = (tstring*)lParam;
		ctrlStatus.SetText(0, (TSTRING(HUB_LIST_DOWNLOADED) + _T(" (") + (*x) + _T(")")).c_str());
		delete x;
	} else if(wParam == STARTING) {
		tstring* x = (tstring*)lParam;
		ctrlStatus.SetText(0, (TSTRING(DOWNLOADING_HUB_LIST) + _T(" (") + (*x) + _T(")")).c_str());
		delete x;
	} else if(wParam == FAILED) {
		tstring* x = (tstring*)lParam;
		ctrlStatus.SetText(0, (TSTRING(DOWNLOAD_FAILED) + (*x) ).c_str());
		delete x;
	}
	return 0;
}

LRESULT PublicHubsFrame::onFilterChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	TCHAR* str;
	
	if(wParam == VK_RETURN) {
		str = new TCHAR[ctrlFilter.GetWindowTextLength()+1];
		ctrlFilter.GetWindowText(str, ctrlFilter.GetWindowTextLength()+1);
		filter = Text::fromT(tstring(str, ctrlFilter.GetWindowTextLength()));
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
	if(ctrlHubs.GetSelectedCount() == 1) {
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
	if(ctrlHubs.GetSelectedCount() == 1) {
		TCHAR buf[256];
		int i = ctrlHubs.GetNextItem(-1, LVNI_SELECTED);
			ctrlHubs.GetItemText(i, COLUMN_SERVER, buf, 256);
		WinUtil::setClipboard(buf);
	}
	return 0;
}

/**
 * @file
 * $Id$
 */