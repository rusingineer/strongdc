#include "stdafx.h"
#include "Resource.h"
#include "../client/DCPlusPlus.h"
#include "../client/Client.h"
#include "../client/ClientManager.h"
#include "../client/HubManager.h"
#include "../client/QueueManager.h"
#include "UploadQueueFrame.h"
#include "PrivateFrame.h"

int UploadQueueFrame::columnSizes[] = { 250, 100, 75, 75, 75, 75, 100, 100 };
int UploadQueueFrame::columnIndexes[] = { COLUMN_FILE, COLUMN_PATH, COLUMN_NICK, COLUMN_HUB, COLUMN_TRANSFERRED, COLUMN_SIZE, COLUMN_ADDED, COLUMN_WAITING };
ResourceManager::Strings UploadQueueFrame::columnNames[] = { ResourceManager::FILENAME, ResourceManager::PATH, ResourceManager::NICK, 
	ResourceManager::HUB, ResourceManager::TRANSFERRED, ResourceManager::SIZE, ResourceManager::ADDED, ResourceManager::WAITING_TIME };

LRESULT UploadQueueFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	showTree = BOOLSETTING(UPLOADQUEUEFRAME_SHOW_TREE); ;

	// status bar
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_UPLOAD_QUEUE);

	DWORD styles = LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT;
	if (BOOLSETTING(SHOW_INFOTIPS))
		styles |= LVS_EX_INFOTIP;
	ctrlList.SetExtendedListViewStyle(styles);

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
		ctrlList.InsertColumn(j, CSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
		
	ctrlList.SetColumnOrderArray(COLUMN_LAST, columnIndexes);
	
	// colors
	ctrlList.SetBkColor(WinUtil::bgColor);
	ctrlList.SetTextBkColor(WinUtil::bgColor);
	ctrlList.SetTextColor(WinUtil::textColor);

	ctrlQueued.SetBkColor(WinUtil::bgColor);
	ctrlQueued.SetTextColor(WinUtil::textColor);
	
	ctrlShowTree.Create(ctrlStatus.m_hWnd, rcDefault, "+/-", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlShowTree.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowTree.SetCheck(showTree);
	showTreeContainer.SubclassWindow(ctrlShowTree.m_hWnd);

	// Create context menu
	grantMenu.CreatePopupMenu();
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CSTRING(GRANT_EXTRA_SLOT));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_HOUR, CSTRING(GRANT_EXTRA_SLOT_HOUR));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_DAY, CSTRING(GRANT_EXTRA_SLOT_DAY));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_WEEK, CSTRING(GRANT_EXTRA_SLOT_WEEK));
	grantMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	grantMenu.AppendMenu(MF_STRING, IDC_UNGRANTSLOT, CSTRING(REMOVE_EXTRA_SLOT));

	contextMenu.CreatePopupMenu();
	contextMenu.AppendMenu(MF_STRING, IDC_GETLIST, CSTRING(GET_FILE_LIST));
	contextMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)grantMenu, CSTRING(GRANT_SLOTS_MENU));
	contextMenu.AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CSTRING(SEND_PRIVATE_MESSAGE));
	contextMenu.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CSTRING(ADD_TO_FAVORITES));
	contextMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	contextMenu.AppendMenu(MF_STRING, IDC_REMOVE, CSTRING(REMOVE));

    memset(statusSizes, 0, sizeof(statusSizes));
	statusSizes[0] = 16;
	ctrlStatus.SetParts(4, statusSizes);
	UpdateLayout();

	// Load all searches
	LoadAll();

	bHandled = FALSE;
	return TRUE;
}

LRESULT UploadQueueFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		UploadManager::getInstance()->removeListener(this);
		CZDCLib::setButtonPressed(IDC_UPLOAD_QUEUE, false);
		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		SettingsManager::getInstance()->set(SettingsManager::UPLOADQUEUEFRAME_SHOW_TREE, ctrlShowTree.GetCheck() == BST_CHECKED);
		WinUtil::saveHeaderOrder(ctrlList, SettingsManager::UPLOADQUEUEFRAME_ORDER, 
			SettingsManager::UPLOADQUEUEFRAME_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);

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
			LoadAll();
		}
	} else {
		if(GetSinglePaneMode() != SPLIT_PANE_RIGHT) {
			SetSinglePaneMode(SPLIT_PANE_RIGHT);
			LoadAll();
		}
	}

	CRect rc = rect;
	SetSplitterRect(rc);
}

LRESULT UploadQueueFrame::onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(showTree) {
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
	if(showTree) {
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
	return 0;
}

LRESULT UploadQueueFrame::onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	// Get the bounding rectangle of the client area. 
	RECT rc;
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	if(showTree) {
		ctrlQueued.GetClientRect(&rc);
		ctrlQueued.ScreenToClient(&pt); 
	} else {
		ctrlList.GetClientRect(&rc);
		ctrlList.ScreenToClient(&pt); 
	}
	// Hit-test
	if(PtInRect(&rc, pt)) {
		if(showTree && ctrlQueued.GetSelectedItem() != NULL) {
			UINT a = 0;
			HTREEITEM ht = ctrlQueued.HitTest(pt, &a);
			if(ht != NULL && ht != ctrlQueued.GetSelectedItem())
				ctrlQueued.SelectItem(ht);
			else if (ht == NULL)
				return FALSE;
			ctrlQueued.ClientToScreen(&pt);
			contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
			return TRUE;
		} else if (ctrlList.GetSelectedCount() > 0) {
			ctrlList.ClientToScreen(&pt);
//		string x = getSelectedNick();
	
//		if(!x.empty())
//			contextMenu.InsertSeparatorFirst(x);

			contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	
//		if(!x.empty())
//			contextMenu.RemoveFirstItem();

			return TRUE; 
		}
	}
	
	return FALSE; 
}

LRESULT UploadQueueFrame::onPrivateMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(showTree) {
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
	if(showTree) {
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
	if(showTree) {
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
	if(showTree) {
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
	if(showTree) {
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
	if(showTree) {
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
	if(showTree) {
		User::Ptr User = getSelectedUser();
		if(User) {
			HubManager::getInstance()->addFavoriteUser(User);
		}
	} else {
		int i = -1;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			HubManager::getInstance()->addFavoriteUser(((UploadQueueItem*)ctrlList.getItemData(i))->User);
		}
	}
	return 0;
};

void UploadQueueFrame::LoadAll() {
	ctrlList.DeleteAllItems();
	ctrlQueued.DeleteAllItems();
	for(User::Iter i = UQFUsers.end(); i != UQFUsers.begin(); --i) {
		UQFUsers.erase(i);
	}

	// Load queue
	UploadQueueItem::UserMap users = UploadManager::getInstance()->getQueue();
	ctrlList.SetRedraw(FALSE);
	ctrlQueued.SetRedraw(FALSE);
	for(UploadQueueItem::UserMapIter uit = users.begin(); uit != users.end(); ++uit) {
		UQFUsers.push_back(uit->first);
		ctrlQueued.InsertItem((uit->first->getNick()+" ("+uit->first->getLastHubName()+")").c_str(), TVI_ROOT, TVI_LAST);
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

//void UploadQueueFrame::on(UploadManagerListener::QueueRemove, const User::Ptr& aUser) {

void UploadQueueFrame::RemoveUser(const User::Ptr& aUser) {
	HTREEITEM nickNode = ctrlQueued.GetRootItem();

	for(User::Iter i = UQFUsers.begin(); i != UQFUsers.end(); ++i) {
		if(*i == aUser) {
			UQFUsers.erase(i);
			break;
		}
	}

	while(nickNode) {
		char nickBuf[512];
		ctrlQueued.GetItemText(nickNode, nickBuf, 511);
		if ((aUser->getNick()+" ("+aUser->getLastHubName()+")") == string(nickBuf)) {
			ctrlQueued.DeleteItem(nickNode);
			break;
		}
		nickNode = ctrlQueued.GetNextSiblingItem(nickNode);
	}

}

LRESULT UploadQueueFrame::onItemChanged(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
	HTREEITEM nickNode = ctrlQueued.GetSelectedItem();

	while(nickNode) {
		ctrlList.DeleteAllItems();
		char nickBuf[256];
		ctrlQueued.GetItemText(nickNode, nickBuf, 255);
		UploadQueueItem::UserMap users = UploadManager::getInstance()->getQueue();
		for (UploadQueueItem::UserMapIter uit = users.begin(); uit != users.end(); ++uit) {
			if((uit->first->getNick()+" ("+uit->first->getLastHubName()+")") == string(nickBuf)) {
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

	HTREEITEM selNode = ctrlQueued.GetSelectedItem();
	char selBuf[256];
	string selNick;
	if(selNode) {
		ctrlQueued.GetItemText(selNode, selBuf, 255);
		selNick = selBuf;
	} else
		selNick = "";
	bool add = true;

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
			nickNode = ctrlQueued.InsertItem((aUQI->User->getNick()+" ("+aUQI->User->getLastHubName()+")").c_str(), TVI_ROOT, TVI_LAST);
	}	
	aUQI->update();
	ctrlList.insertItem(ctrlList.GetItemCount(), aUQI, WinUtil::getIconIndex(aUQI->FileName));
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
		tmp[0] = STRING(USERS) + ": " + Util::toString(users);
		tmp[1] = STRING(ITEMS) + ": " + Util::toString(cnt);
		bool u = false;

		for(int i = 1; i < 3; i++) {
			int w = WinUtil::getTextWidth(tmp[i-1], ctrlStatus.m_hWnd);
				
			if(statusSizes[i] < w) {
				statusSizes[i] = w + 50;
				u = true;
			}
			ctrlStatus.SetText(i+1, tmp[i-1].c_str());
		}

		if(u)
			UpdateLayout(TRUE);
	}
}


void UploadQueueItem::update() {
	columns[COLUMN_FILE] = FileName;
	columns[COLUMN_PATH] = Path;
	columns[COLUMN_NICK] = User->getNick();
	columns[COLUMN_HUB] = User->getLastHubName();
	columns[COLUMN_TRANSFERRED] = Util::formatBytes(pos)+" ("+Util::toString((double)pos*100.0/(double)size)+"%)";
	columns[COLUMN_SIZE] = Util::formatBytes(size);
	columns[COLUMN_ADDED] = Util::formatTime("%Y-%m-%d %H:%M", iTime);
	columns[COLUMN_WAITING] = Util::formatSeconds(GET_TIME() - iTime);
}

LRESULT UploadQueueFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(wParam == REMOVE_ITEM) {
		UploadQueueItem* i = (UploadQueueItem*)lParam;
		ctrlList.deleteItem(i);
	} else if(wParam == REMOVE) {
		UploadQueueItem* i = (UploadQueueItem*)lParam;
		RemoveUser(i->User);
		delete i;
	} else if(wParam == ADD_ITEM) {
		UploadQueueItem* i = (UploadQueueItem*)lParam;
		AddFile(i);
	}
	return 0;
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
		if(cd->iSubItem == COLUMN_TRANSFERRED) {
			// draw something nice...
			char buf[256];
				COLORREF barBase;
				if (SETTING(PROGRESS_OVERRIDE_COLORS)) {
					barBase = SETTING(UPLOAD_BAR_COLOR);
				} else {
					barBase = GetSysColor(COLOR_HIGHLIGHT);
				}
				COLORREF bgBase = WinUtil::bgColor;
				int mod = (HLS_L(RGB2HLS(bgBase)) >= 128) ? -30 : 30;
				//COLORREF barPal[4] = { HLS_TRANSFORM(barBase, -90, 80), HLS_TRANSFORM(barBase, -50, 40), barBase, HLS_TRANSFORM(barBase, 40, -30) };
				COLORREF bgPal[2] = { HLS_TRANSFORM(bgBase, mod, 0), HLS_TRANSFORM(bgBase, mod/2, 0) };

				ctrlList.GetItemText((int)cd->nmcd.dwItemSpec, COLUMN_TRANSFERRED, buf, 255);
				buf[255] = 0;

				ctrlList.GetSubItemRect((int)cd->nmcd.dwItemSpec, COLUMN_TRANSFERRED, LVIR_BOUNDS, rc);
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

				BITMAPINFOHEADER bih;
				bih.biSize = sizeof(BITMAPINFOHEADER);
				bih.biWidth = real_rc.Width();
				bih.biHeight = -real_rc.Height(); // kanske minus
				bih.biPlanes = 1;
				bih.biBitCount = 32;
				bih.biCompression = BI_RGB;
				bih.biSizeImage = 0;
				bih.biXPelsPerMeter = 0;
				bih.biYPelsPerMeter = 0;
				bih.biClrUsed = 32;
				bih.biClrImportant = 0;
				HBITMAP hBmp = CreateDIBitmap(cd->nmcd.hdc, &bih, 0, NULL, NULL, DIB_RGB_COLORS);
				if (hBmp == NULL)
					MessageBox(Util::translateError(GetLastError()).c_str(), "ERROR", MB_OK);
				HBITMAP pOldBmp = cdc.SelectBitmap(hBmp);
				HDC& dc = cdc.m_hDC;

				HFONT oldFont = (HFONT)SelectObject(dc, WinUtil::font);
				SetBkMode(dc, TRANSPARENT);

			// draw background
				HGDIOBJ oldpen = ::SelectObject(dc, CreatePen(PS_SOLID,0,bgPal[0]));
				HGDIOBJ oldbr = ::SelectObject(dc, CreateSolidBrush(bgPal[1]));
				::Rectangle(dc, rc.left, rc.top - 1, rc.right, rc.bottom);			
		        rc.DeflateRect(1, 0, 1, 1); 

				LONG left = rc.left;
				int64_t w = rc.Width();
				// draw start part
				string per = buf;
				per = per.substr(per.find('(')+1, per.find('%')-(per.find('(')+2));
				double percent = Util::toDouble(per) / 100;
				//double percent = (ii->getSize() >0) ? (double)((double)ii->getPos()) / ((double)ii->getSize()) : 0;
				percent = (percent < 0)? 0 : percent;
				
				rc.right = left + (int) (w * percent);

				COLORREF a, b;
				OperaColors::EnlightenFlood(barBase, a, b);
				OperaColors::FloodFill(cdc, rc.left, rc.top, rc.right, rc.bottom, a, b, SETTING(PROGRESS_BUMPED));
				//OperaColors::FloodFill(cdc, rc.left, rc.top, rc.right, rc.bottom, barPal[2], barPal[0], SETTING(PROGRESS_BUMPED));
				rc.left = left;
				
				// draw status text
				DeleteObject(::SelectObject(cd->nmcd.hdc, oldpen));
				DeleteObject(::SelectObject(cd->nmcd.hdc, oldbr));

				LONG right = rc2.right;
				left = rc2.left;
				rc2.right = rc.right;
				LONG top = rc2.top + (rc2.Height() - WinUtil::getTextHeight(dc) - 1)/2;
				int textcolor;
				if (SETTING(PROGRESS_OVERRIDE_COLORS)) {
					textcolor = SETTING(PROGRESS_TEXT_COLOR_UP);
				} else {
					textcolor = OperaColors::TextFromBackground(barBase);
				}
			
				SetTextColor(dc, textcolor);
                ::ExtTextOut(dc, left, top, ETO_CLIPPED, rc2, buf, strlen(buf), NULL);

				rc2.left = rc2.right;
				rc2.right = right;

				SetTextColor(dc, WinUtil::textColor);
				::ExtTextOut(dc, left, top, ETO_CLIPPED, rc2, buf, strlen(buf), NULL);

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
