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

#include "UsersFrame.h"
#include "CZDCLib.h"

#include "../client/StringTokenizer.h"
#include "LineDlg.h"

int UsersFrame::columnIndexes[] = { COLUMN_NICK, COLUMN_STATUS, COLUMN_HUB, COLUMN_SEEN, COLUMN_DESCRIPTION };
int UsersFrame::columnSizes[] = { 200, 150, 300, 125, 200 };
static ResourceManager::Strings columnNames[] = { ResourceManager::AUTO_GRANT, ResourceManager::STATUS, ResourceManager::LAST_HUB, ResourceManager::LAST_SEEN, ResourceManager::DESCRIPTION };

LRESULT UsersFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	ctrlUsers.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_USERS);

	DWORD styles = LVS_EX_HEADERDRAGDROP | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT;
	if (BOOLSETTING(SHOW_INFOTIPS))
		styles |= LVS_EX_INFOTIP;
	ctrlUsers.SetExtendedListViewStyle(styles);

	images.CreateFromImage(IDB_FAVSTATES, 16, 3, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	ctrlUsers.SetImageList(images, LVSIL_SMALL);

	ctrlUsers.SetBkColor(WinUtil::bgColor);
	ctrlUsers.SetTextBkColor(WinUtil::bgColor);
	ctrlUsers.SetTextColor(WinUtil::textColor);
	
	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(USERSFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(USERSFRAME_WIDTHS), COLUMN_LAST);
	
	for(int j=0; j<COLUMN_LAST; j++) {
		ctrlUsers.InsertColumn(j, CSTRING_I(columnNames[j]), LVCFMT_LEFT, columnSizes[j], j);
	}
	
	ctrlUsers.SetColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlUsers.setSortColumn(COLUMN_NICK);
	usersMenu.CreatePopupMenu();
	usersMenu.AppendMenu(MF_STRING, IDC_EDIT, CSTRING(PROPERTIES));
	usersMenu.AppendMenu(MF_SEPARATOR);
	appendUserItems(usersMenu);
	usersMenu.AppendMenu(MF_SEPARATOR);
	usersMenu.AppendMenu(MF_STRING, IDC_REMOVE, CSTRING(REMOVE));

	HubManager::getInstance()->addListener(this);
	ClientManager::getInstance()->addListener(this);

	User::List ul = HubManager::getInstance()->getFavoriteUsers();
	ctrlUsers.SetRedraw(FALSE);
	for(User::Iter i = ul.begin(); i != ul.end(); ++i) {
		addUser(*i);
	}
	ctrlUsers.SetRedraw(TRUE);

	startup = false;

	bHandled = FALSE;
	return TRUE;

}

LRESULT UsersFrame::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlUsers.forEachSelected(&UserInfo::remove);
	return 0;
}

LRESULT UsersFrame::onEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlUsers.GetSelectedCount() == 1) {
		int i = ctrlUsers.GetNextItem(-1, LVNI_SELECTED);
		UserInfo* ui = ctrlUsers.getItemData(i);
		dcassert(i != -1);
	LineDlg dlg;
	dlg.description = STRING(DESCRIPTION);
		dlg.title = ui->user->getNick();
		dlg.line = ui->user->getUserDescription();
		if(dlg.DoModal(m_hWnd)) {
			ui->user->setUserDescription(dlg.line);
			ui->update();
			ctrlUsers.updateItem(i);
				HubManager::getInstance()->save();
			}	
	}
	return 0;
}

LRESULT UsersFrame::onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* l = (NMITEMACTIVATE*)pnmh;
	if(!startup && l->iItem != -1 && ((l->uNewState & LVIS_STATEIMAGEMASK) != (l->uOldState & LVIS_STATEIMAGEMASK))) {
		ctrlUsers.getItemData(l->iItem)->user->setAutoExtraSlot(ctrlUsers.GetCheckState(l->iItem) != FALSE);
		HubManager::getInstance()->save();
 	}
  	return 0;
}

LRESULT UsersFrame::onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	RECT rc;                    // client area of window 
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 
	
	// Get the bounding rectangle of the client area. 
	ctrlUsers.GetClientRect(&rc);
	ctrlUsers.ScreenToClient(&pt); 
	
	if (ctrlUsers.GetSelectedCount() > 0 && PtInRect(&rc, pt)) 
	{ 
				string x;
		if (CZDCLib::getFirstSelectedIndex(ctrlUsers) > -1) {
			if (ctrlUsers.GetSelectedCount() == 1)
				x = ctrlUsers.getItemData(CZDCLib::getFirstSelectedIndex(ctrlUsers))->user->getNick();
			else
				x = "";
		} else {
			ctrlUsers.SetRedraw(TRUE);
			return FALSE;
		}
		if (!x.empty())
			usersMenu.InsertSeparatorFirst(x);

		ctrlUsers.ClientToScreen(&pt);
		usersMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

		if (!x.empty())
			usersMenu.RemoveFirstItem();

		ctrlUsers.SetRedraw(TRUE);

		return TRUE; 
	}
	
	return FALSE; 
}

void UsersFrame::addUser(const User::Ptr& aUser) {
	int i = ctrlUsers.insertItem(new UserInfo(aUser), 2);
	bool b = aUser->getAutoExtraSlot();
	ctrlUsers.SetCheckState(i, b);
	updateUser(aUser);
}

void UsersFrame::updateUser(const User::Ptr& aUser) {
	int i = -1;
	while((i = ctrlUsers.findItem(aUser->getNick(), i)) != -1) {
		UserInfo *ui = ctrlUsers.getItemData(i);
		if(ui->user == aUser) {
			ui->update();
			if(aUser->isOnline()) {
				if((aUser->getStatus() == 2) || (aUser->getStatus() == 3) || (aUser->getStatus() == 6) || (aUser->getStatus() == 7) || (aUser->getStatus() == 10) || (aUser->getStatus() == 11))
					ctrlUsers.SetItem(i,0,LVIF_IMAGE, NULL, 1, 0, 0, NULL);
				else ctrlUsers.SetItem(i,0,LVIF_IMAGE, NULL, 0, 0, 0, NULL);
			}
			else ctrlUsers.SetItem(i,0,LVIF_IMAGE, NULL, 2, 0, 0, NULL);
			ctrlUsers.updateItem(i);
		}
	}
}

void UsersFrame::removeUser(const User::Ptr& aUser) {
	int i = -1;
	while((i = ctrlUsers.findItem(aUser->getNick(), i)) != -1) {
		UserInfo *ui = ctrlUsers.getItemData(i);
		if(ui->user == aUser) {
			ctrlUsers.deleteItem(i);
			return;
		}
	}
}

LRESULT UsersFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
	HubManager::getInstance()->removeListener(this);
	ClientManager::getInstance()->removeListener(this);
	
		closed = true;
		CZDCLib::setButtonPressed(IDC_FAVUSERS, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		WinUtil::saveHeaderOrder(ctrlUsers, SettingsManager::USERSFRAME_ORDER, 
			SettingsManager::USERSFRAME_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);
	
		ctrlUsers.DeleteAll();
		bHandled = FALSE;
	return 0;
	}
}

LRESULT UsersFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	CRect rc;
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
		// Let's draw a box if needed...
		if(cd->iSubItem == COLUMN_STATUS) {
			UserInfo *ui = ctrlUsers.getStoredItemAt(cd->nmcd.lItemlParam);
			if(ui->user->isOnline()) {
				cd->clrText = RGB(0, 255, 0);
			} else {
				cd->clrText = RGB(255, 0, 0);
			}
			return CDRF_NEWFONT;
		} else {
			cd->clrText = WinUtil::textColor;
			return CDRF_NEWFONT;
		}

	default:
		return CDRF_DODEFAULT;
	}
}

/**
 * @file
 * $Id$
 */