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
#include "Resource.h"
#include "../client/DCPlusPlus.h"
#include "../client/Client.h"
#include "../client/ClientManager.h"
#include "../client/FavoriteManager.h"
#include "../client/QueueManager.h"
#include "UploadQueueFrame.h"
#include "PrivateFrame.h"

#include "BarShader.h"

int UploadQueueFrame::columnSizes[] = { 250, 100, 75, 75, 75, 75, 100, 100 };
int UploadQueueFrame::columnIndexes[] = { COLUMN_FILE, COLUMN_PATH, COLUMN_NICK, COLUMN_HUB, COLUMN_TRANSFERRED, COLUMN_SIZE, COLUMN_ADDED, COLUMN_WAITING };
static ResourceManager::Strings columnNames[] = { ResourceManager::FILENAME, ResourceManager::PATH, ResourceManager::NICK, 
	ResourceManager::HUB, ResourceManager::TRANSFERRED, ResourceManager::SIZE, ResourceManager::ADDED, ResourceManager::WAITING_TIME };

LRESULT UploadQueueFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	showTree = BOOLSETTING(UPLOADQUEUEFRAME_SHOW_TREE); ;

	// status bar
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_UPLOAD_QUEUE);

	ctrlList.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | 0x00010000 | (BOOLSETTING(SHOW_INFOTIPS) ? LVS_EX_INFOTIP : 0));
	ctrlQueued.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
		TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP, 
		 WS_EX_CLIENTEDGE, IDC_DIRECTORIES);
	
	ctrlQueued.SetImageList(WinUtil::fileImages, TVSIL_NORMAL);
	ctrlList.SetImageList(WinUtil::fileImages, LVSIL_SMALL);

	m_nProportionalPos = 2500;
	SetSplitterPanes(ctrlQueued.m_hWnd, ctrlList.m_hWnd);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(UPLOADQUEUEFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(UPLOADQUEUEFRAME_WIDTHS), COLUMN_LAST);

	// column names, sizes
	for (int j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_TRANSFERRED || j == COLUMN_SIZE) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlList.insertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
		
	ctrlList.setColumnOrderArray(COLUMN_LAST, columnIndexes);
    ctrlList.saveHeaderOrder(SettingsManager::UPLOADQUEUEFRAME_ORDER, SettingsManager::UPLOADQUEUEFRAME_WIDTHS,
		SettingsManager::UPLOADQUEUEFRAME_VISIBLE);

	ctrlList.setVisible(SETTING(UPLOADQUEUEFRAME_VISIBLE));
	
	// colors
	ctrlList.SetBkColor(WinUtil::bgColor);
	ctrlList.SetTextBkColor(WinUtil::bgColor);
	ctrlList.SetTextColor(WinUtil::textColor);

	ctrlQueued.SetBkColor(WinUtil::bgColor);
	ctrlQueued.SetTextColor(WinUtil::textColor);
	
	ctrlShowTree.Create(ctrlStatus.m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlShowTree.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowTree.SetCheck(showTree);
	showTreeContainer.SubclassWindow(ctrlShowTree.m_hWnd);

	// Create context menu
	grantMenu.CreatePopupMenu();
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CTSTRING(GRANT_EXTRA_SLOT));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_HOUR, CTSTRING(GRANT_EXTRA_SLOT_HOUR));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_DAY, CTSTRING(GRANT_EXTRA_SLOT_DAY));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_WEEK, CTSTRING(GRANT_EXTRA_SLOT_WEEK));
	grantMenu.AppendMenu(MF_SEPARATOR);
	grantMenu.AppendMenu(MF_STRING, IDC_UNGRANTSLOT, CTSTRING(REMOVE_EXTRA_SLOT));

	contextMenu.CreatePopupMenu();
	contextMenu.AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
	contextMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)grantMenu, CTSTRING(GRANT_SLOTS_MENU));
	contextMenu.AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CTSTRING(SEND_PRIVATE_MESSAGE));
	contextMenu.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
	contextMenu.AppendMenu(MF_SEPARATOR);
	contextMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));

    memset2(statusSizes, 0, sizeof(statusSizes));
	statusSizes[0] = 16;
	ctrlStatus.SetParts(4, statusSizes);
	UpdateLayout();

	UploadManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);
	// Load all searches
	LoadAll();

	bHandled = FALSE;
	return TRUE;
}

LRESULT UploadQueueFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		UploadManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		CZDCLib::setButtonPressed(IDC_UPLOAD_QUEUE, false);
		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		ctrlList.DeleteAllItems();
		ctrlQueued.DeleteAllItems();
		UQFUsers.clear();
		SettingsManager::getInstance()->set(SettingsManager::UPLOADQUEUEFRAME_SHOW_TREE, ctrlShowTree.GetCheck() == BST_CHECKED);
	    ctrlList.saveHeaderOrder(SettingsManager::UPLOADQUEUEFRAME_ORDER, SettingsManager::UPLOADQUEUEFRAME_WIDTHS,
			SettingsManager::UPLOADQUEUEFRAME_VISIBLE);

		bHandled = FALSE;
		return 0;
	}
}

void UploadQueueFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[4];
		ctrlStatus.GetClientRect(sr);
		w[3] = sr.right - 16;
#define setw(x) w[x] = max(w[x+1] - statusSizes[x], 0)
		setw(2); setw(1);

		w[0] = 16;

		ctrlStatus.SetParts(4, w);

		ctrlStatus.GetRect(0, sr);
		ctrlShowTree.MoveWindow(sr);
	}
	if(showTree) {
		if(GetSinglePaneMode() != SPLIT_PANE_NONE) {
			SetSinglePaneMode(SPLIT_PANE_NONE);
		}
	} else {
		if(GetSinglePaneMode() != SPLIT_PANE_RIGHT) {
			SetSinglePaneMode(SPLIT_PANE_RIGHT);
		}
	}
	CRect rc = rect;
	SetSplitterRect(rc);
}

LRESULT UploadQueueFrame::onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(usingUserMenu) {
		User::Ptr User = getSelectedUser();
		if(User) {
			QueueManager::getInstance()->addList(User, QueueItem::FLAG_CLIENT_VIEW);
		}
	} else {
		int i = -1;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			QueueManager::getInstance()->addList(((UploadQueueItem*)ctrlList.getItemData(i))->User, QueueItem::FLAG_CLIENT_VIEW);
		}
	}
	return 0;
}

LRESULT UploadQueueFrame::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(usingUserMenu) {
		User::Ptr User = getSelectedUser();
		if(User) {
			UploadManager::getInstance()->clearUserFiles(User);
		}
	} else {
		int i = -1;
		User::List RemoveUsers;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			// Ok let's cheat here, if you try to remove more users here is not working :(
			RemoveUsers.push_back(((UploadQueueItem*)ctrlList.getItemData(i))->User);
		}
		for(User::Iter i = RemoveUsers.begin(); i != RemoveUsers.end(); ++i) {
			UploadManager::getInstance()->clearUserFiles(*i);
		}
	}
	updateStatus();
	return 0;
}

LRESULT UploadQueueFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	RECT rc;
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	ctrlList.GetHeader().GetWindowRect(&rc);
	if(PtInRect(&rc, pt)){
		ctrlList.showMenu(pt);
		return TRUE;
	}
		
	if(reinterpret_cast<HWND>(wParam) == ctrlList && ctrlList.GetSelectedCount() > 0) {
     	if(pt.x == -1 && pt.y == -1) {
    		WinUtil::getContextMenuPos(ctrlList, pt);
    	}
		usingUserMenu = false;
		contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		return TRUE;
	} else if(reinterpret_cast<HWND>(wParam) == ctrlQueued && ctrlQueued.GetSelectedItem() != NULL) {
     	if(pt.x == -1 && pt.y == -1) {
    		WinUtil::getContextMenuPos(ctrlQueued, pt);
    	} else {
			UINT a = 0;
    		ctrlQueued.ScreenToClient(&pt);
			HTREEITEM ht = ctrlQueued.HitTest(pt, &a);
			if(ht != NULL && ht != ctrlQueued.GetSelectedItem())
				ctrlQueued.SelectItem(ht);
    
			ctrlQueued.ClientToScreen(&pt);
        }
        usingUserMenu = true;
			contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
			return TRUE;
	}	
	return FALSE; 
}

LRESULT UploadQueueFrame::onPrivateMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(usingUserMenu) {
		User::Ptr User = getSelectedUser();
		if(User) {
			PrivateFrame::openWindow(User);
		}
	} else {
		int i = -1;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			PrivateFrame::openWindow(((UploadQueueItem*)ctrlList.getItemData(i))->User);
		}
	}
	return 0;
}

LRESULT UploadQueueFrame::onGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) { 
	if(usingUserMenu) {
		User::Ptr User = getSelectedUser();
		if(User) {
			UploadManager::getInstance()->reserveSlot(User);
		}
	} else {
		int i = -1;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			UploadManager::getInstance()->reserveSlot(((UploadQueueItem*)ctrlList.getItemData(i))->User);
		}
	}
	return 0; 
};

LRESULT UploadQueueFrame::onGrantSlotHour(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(usingUserMenu) {
		User::Ptr User = getSelectedUser();
		if(User) {
			UploadManager::getInstance()->reserveSlotHour(User);
		}
	} else {
		int i = -1;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			UploadManager::getInstance()->reserveSlotHour(((UploadQueueItem*)ctrlList.getItemData(i))->User);
		}
	}
	return 0;
};

LRESULT UploadQueueFrame::onGrantSlotDay(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(usingUserMenu) {
		User::Ptr User = getSelectedUser();
		if(User) {
			UploadManager::getInstance()->reserveSlotDay(User);
		}
	} else {
		int i = -1;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			UploadManager::getInstance()->reserveSlotDay(((UploadQueueItem*)ctrlList.getItemData(i))->User);
		}
	}
	return 0;
};

LRESULT UploadQueueFrame::onGrantSlotWeek(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(usingUserMenu) {
		User::Ptr User = getSelectedUser();
		if(User) {
			UploadManager::getInstance()->reserveSlotWeek(User);
		}
	} else {
		int i = -1;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			UploadManager::getInstance()->reserveSlotWeek(((UploadQueueItem*)ctrlList.getItemData(i))->User);
		}
	}
	return 0;
};

LRESULT UploadQueueFrame::onUnGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(usingUserMenu) {
		User::Ptr User = getSelectedUser();
		if(User) {
			UploadManager::getInstance()->unreserveSlot(User);
		}
	} else {
		int i = -1;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			UploadManager::getInstance()->unreserveSlot(((UploadQueueItem*)ctrlList.getItemData(i))->User);
		}
	}
	return 0;
};

LRESULT UploadQueueFrame::onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(usingUserMenu) {
		User::Ptr User = getSelectedUser();
		if(User) {
			FavoriteManager::getInstance()->addFavoriteUser(User);
		}
	} else {
		int i = -1;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			FavoriteManager::getInstance()->addFavoriteUser(((UploadQueueItem*)ctrlList.getItemData(i))->User);
		}
	}
	return 0;
};

void UploadQueueFrame::LoadAll() {
	ctrlList.DeleteAllItems();
	ctrlQueued.DeleteAllItems();
	UQFUsers.clear();

	// Load queue
	UploadQueueItem::UserMap users = UploadManager::getInstance()->getQueue();
	ctrlList.SetRedraw(FALSE);
	ctrlQueued.SetRedraw(FALSE);
	for(UploadQueueItem::UserMapIter uit = users.begin(); uit != users.end(); ++uit) {
		UQFUsers.push_back(uit->first);
		ctrlQueued.InsertItem(Text::toT(uit->first->getNick()+" ("+uit->first->getLastHubName()+")").c_str(), TVI_ROOT, TVI_LAST);
		for(UploadQueueItem::Iter i = uit->second.begin(); i != uit->second.end(); ++i) {
			AddFile(*i);
		}
	}
	ctrlList.resort();
	ctrlList.SetRedraw(TRUE);
	ctrlQueued.SetRedraw(TRUE);
	ctrlQueued.Invalidate(); 
	updateStatus();
}

void UploadQueueFrame::RemoveUser(const User::Ptr& aUser) {
	HTREEITEM nickNode = ctrlQueued.GetRootItem();

	for(User::Iter i = UQFUsers.begin(); i != UQFUsers.end(); ++i) {
		if(*i == aUser) {
			UQFUsers.erase(i);
			break;
		}
	}

	while(nickNode) {
		TCHAR nickBuf[512];
		ctrlQueued.GetItemText(nickNode, nickBuf, 511);
		if ((aUser->getNick()+" ("+aUser->getLastHubName()+")") == Text::fromT(nickBuf)) {
			ctrlQueued.DeleteItem(nickNode);
			break;
		}
		nickNode = ctrlQueued.GetNextSiblingItem(nickNode);
	}
	updateStatus();
}

LRESULT UploadQueueFrame::onItemChanged(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
	HTREEITEM nickNode = ctrlQueued.GetSelectedItem();

	while(nickNode) {
		ctrlList.DeleteAllItems();
		TCHAR nickBuf[256];
		ctrlQueued.GetItemText(nickNode, nickBuf, 255);
		UploadQueueItem::UserMap users = UploadManager::getInstance()->getQueue();
		for (UploadQueueItem::UserMapIter uit = users.begin(); uit != users.end(); ++uit) {
			if((uit->first->getNick()+" ("+uit->first->getLastHubName()+")") == Text::fromT(nickBuf)) {
				ctrlList.SetRedraw(FALSE);
				ctrlQueued.SetRedraw(FALSE);
				for(UploadQueueItem::Iter i = uit->second.begin(); i != uit->second.end(); ++i) {
					AddFile(*i);
				}
				ctrlList.resort();
				ctrlList.SetRedraw(TRUE);
				ctrlQueued.SetRedraw(TRUE);
				ctrlQueued.Invalidate(); 
				updateStatus();
				return 0;
			}
		}
		nickNode = ctrlQueued.GetNextSiblingItem(nickNode);
	}
	return 0;
}

void UploadQueueFrame::AddFile(UploadQueueItem* aUQI) { 
	HTREEITEM nickNode = ctrlQueued.GetRootItem();
	bool add = true;

	HTREEITEM selNode = ctrlQueued.GetSelectedItem();

	if(nickNode) {
		for(User::Iter i = UQFUsers.begin(); i != UQFUsers.end(); ++i) {
			if(*i == aUQI->User) {
				add = false;
				break;
			}
		}
	}
	if(add) {
			UQFUsers.push_back(aUQI->User);
			nickNode = ctrlQueued.InsertItem(Text::toT(aUQI->User->getNick()+" ("+aUQI->User->getLastHubName()+")").c_str(), TVI_ROOT, TVI_LAST);
	}	
	if(selNode) {
		TCHAR selBuf[256];
		ctrlQueued.GetItemText(selNode, selBuf, 255);
		if(_tcscmp(selBuf, Text::toT(aUQI->User->getNick()+" ("+aUQI->User->getLastHubName()+")").c_str()) != 0) {
			return;
		}
	}
	aUQI->update();
	aUQI->icon = WinUtil::getIconIndex(Text::toT(aUQI->FileName));
	ctrlList.insertItem(ctrlList.GetItemCount(), aUQI, aUQI->icon);
}

HTREEITEM UploadQueueFrame::GetParentItem() {
	HTREEITEM item = ctrlQueued.GetSelectedItem(), parent = ctrlQueued.GetParentItem(item);
	parent = parent?parent:item;
	ctrlQueued.SelectItem(parent);
	return parent;
}

void UploadQueueFrame::updateStatus() {
	if(ctrlStatus.IsWindow()) {

		int cnt = ctrlList.GetItemCount();
		int users = ctrlQueued.GetCount();	

		string tmp[2];
		if(showTree) {
			tmp[0] = STRING(USERS) + ": " + Util::toString(users);
		} else {
			tmp[0] = "";
		}    		  
		tmp[1] = STRING(ITEMS) + ": " + Util::toString(cnt);
		bool u = false;

		for(int i = 1; i < 3; i++) {
			int w = WinUtil::getTextWidth(Text::toT(tmp[i-1]), ctrlStatus.m_hWnd);
				
			if(statusSizes[i] < w) {
				statusSizes[i] = w + 50;
				u = true;
			}
			ctrlStatus.SetText(i+1, Text::toT(tmp[i-1]).c_str());
		}

		if(u)
			UpdateLayout(TRUE);
	}
}


void UploadQueueItem::update() {
	columns[COLUMN_FILE] = Text::toT(FileName);
	columns[COLUMN_PATH] = Text::toT(Path);
	columns[COLUMN_NICK] = Text::toT(User->getNick());
	columns[COLUMN_HUB] = Text::toT(User->getLastHubName());
	columns[COLUMN_TRANSFERRED] = Text::toT(Util::formatBytes(pos)+" ("+Util::toString((double)pos*100.0/(double)size)+"%)");
	columns[COLUMN_SIZE] = Text::toT(Util::formatBytes(size));
	columns[COLUMN_ADDED] = Text::toT(Util::formatTime("%Y-%m-%d %H:%M", iTime));
	columns[COLUMN_WAITING] = Text::toT(Util::formatSeconds(GET_TIME() - iTime));

}

LRESULT UploadQueueFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	ctrlList.SetRedraw(FALSE);
	if(wParam == REMOVE_ITEM) {
		UploadQueueItem* i = (UploadQueueItem*)lParam;
		ctrlList.deleteItem(i);
		updateStatus();
	} else if(wParam == REMOVE) {
		RemoveUser(((UserInfoBase*)lParam)->user);
		delete (UserInfoBase*)lParam;
	} else if(wParam == ADD_ITEM) {
		AddFile((UploadQueueItem*)lParam);
		ctrlList.resort();
	} else if(wParam == UPDATE_ITEMS) {
		int j = ctrlList.GetItemCount();
		int64_t itime = GET_TIME();
		for(int i = 0; i < j; i++) {
			UploadQueueItem* UQI = ctrlList.getItemData(i);
			if(UploadManager::getInstance()->isInUploadQueue(UQI)) {
				UQI->columns[COLUMN_WAITING] = Text::toT(Util::formatSeconds(itime - UQI->iTime));
				ctrlList.updateItem(i);
			}
		}
	}
	ctrlList.SetRedraw(TRUE);
	return 0;
}

void UploadQueueFrame::on(SettingsManagerListener::Save, SimpleXML* /*xml*/) throw() {
	bool refresh = false;
	if(ctrlList.GetBkColor() != WinUtil::bgColor) {
		ctrlList.SetBkColor(WinUtil::bgColor);
		ctrlList.SetTextBkColor(WinUtil::bgColor);
		ctrlQueued.SetBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlList.GetTextColor() != WinUtil::textColor) {
		ctrlList.SetTextColor(WinUtil::textColor);
		ctrlQueued.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

LRESULT UploadQueueFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	if(!BOOLSETTING(SHOW_PROGRESS_BARS)) {
		bHandled = FALSE;
		return 0;
	}

	CRect rc;
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
		// Let's draw a box if needed...
		LVCOLUMN lvc;
		lvc.mask = LVCF_TEXT;
		lvc.pszText = headerBuf;
		lvc.cchTextMax = 128;
		ctrlList.GetColumn(cd->iSubItem, &lvc);
		if(Util::stricmp(headerBuf, CTSTRING_I(columnNames[COLUMN_TRANSFERRED])) == 0 ) {
			// draw something nice...
				TCHAR buf[256];
				UploadQueueItem *ii = (UploadQueueItem*)cd->nmcd.lItemlParam;

				ctrlList.GetItemText((int)cd->nmcd.dwItemSpec, cd->iSubItem, buf, 255);
				buf[255] = 0;

				ctrlList.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);
				// Real rc, the original one.
				CRect real_rc = rc;
				// We need to offset the current rc to (0, 0) to paint on the New dc
				rc.MoveToXY(0, 0);

				// Text rect
				CRect rc2 = rc;
                rc2.left += 6; // indented with 6 pixels
				rc2.right -= 2; // and without messing with the border of the cell

				// Set references
				CDC cdc;
				cdc.CreateCompatibleDC(cd->nmcd.hdc);
				HBITMAP hBmp = CreateCompatibleBitmap(cd->nmcd.hdc,  real_rc.Width(),  real_rc.Height());
				HBITMAP pOldBmp = cdc.SelectBitmap(hBmp);
				HDC& dc = cdc.m_hDC;

				HFONT oldFont = (HFONT)SelectObject(dc, WinUtil::font);
				SetBkMode(dc, TRANSPARENT);

				CBarShader statusBar(rc.bottom - rc.top, rc.right - rc.left, RGB(150, 0, 0), ii->size);
				statusBar.FillRange(0, ii->pos, RGB(222, 160, 0));
				statusBar.Draw(cdc, rc.top, rc.left, SETTING(PROGRESS_3DDEPTH));

				LONG top = rc2.top + (rc2.Height() - WinUtil::getTextHeight(dc) - 1)/2;
				int textcolor = SETTING(PROGRESS_TEXT_COLOR_UP);
			
				SetTextColor(dc, textcolor);
                ::ExtTextOut(dc, rc2.left, top, ETO_CLIPPED, rc2, buf, _tcslen(buf), NULL);

				SelectObject(dc, oldFont);
				
				BitBlt(cd->nmcd.hdc, real_rc.left, real_rc.top, real_rc.Width(), real_rc.Height(), dc, 0, 0, SRCCOPY);

				DeleteObject(cdc.SelectBitmap(pOldBmp));
				return CDRF_SKIPDEFAULT;	
		}
		// Fall through
	default:
		return CDRF_DODEFAULT;
	}
}

/**
 * @file
 * $Id: UploadQueueFrame.cpp,v 1.4 2003/05/13 11:34:07
 */
