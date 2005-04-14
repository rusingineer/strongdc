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

#include "FinishedULFrame.h"
#include "WinUtil.h"
#include "CZDCLib.h"	
#include "TextFrame.h"

#include "../client/ClientManager.h"
#include "../client/StringTokenizer.h"

int FinishedULFrame::columnIndexes[] = { COLUMN_DONE, COLUMN_FILE, COLUMN_PATH, COLUMN_NICK, COLUMN_HUB, COLUMN_SIZE, COLUMN_SPEED };
int FinishedULFrame::columnSizes[] = { 100, 110, 290, 125, 80, 80, 80 };
static ResourceManager::Strings columnNames[] = { ResourceManager::FILENAME, ResourceManager::TIME, ResourceManager::PATH, 
ResourceManager::NICK, ResourceManager::HUB, ResourceManager::SIZE, ResourceManager::SPEED
};

LRESULT FinishedULFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	
	ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_FINISHED_UL);
	ctrlList.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | (BOOLSETTING(SHOW_INFOTIPS) ? LVS_EX_INFOTIP : 0));	

	ctrlList.SetImageList(WinUtil::fileImages, LVSIL_SMALL);
	ctrlList.SetBkColor(WinUtil::bgColor);
	ctrlList.SetTextBkColor(WinUtil::bgColor);
	ctrlList.SetTextColor(WinUtil::textColor);

	stateImages.CreateFromImage(IDB_STATE, 16, 2, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	ctrlList.SetImageList(stateImages, LVSIL_STATE);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(FINISHED_UL_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(FINISHED_UL_WIDTHS), COLUMN_LAST);
	
	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_SPEED) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlList.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlList.SetColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlList.setSort(COLUMN_DONE, ExListViewCtrl::SORT_STRING_NOCASE);
	
	UpdateLayout();
	
	FinishedManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);
	updateList(FinishedManager::getInstance()->lockList(true));
	FinishedManager::getInstance()->unlockList();
	
	ctxMenu.CreatePopupMenu();
	ctxMenu.AppendMenu(MF_STRING, IDC_VIEW_AS_TEXT, CTSTRING(VIEW_AS_TEXT));
	ctxMenu.AppendMenu(MF_STRING, IDC_OPEN_FILE, CTSTRING(OPEN));
	ctxMenu.AppendMenu(MF_STRING, IDC_OPEN_FOLDER, CTSTRING(OPEN_FOLDER));
	ctxMenu.AppendMenu(MF_SEPARATOR);
	ctxMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
	ctxMenu.AppendMenu(MF_STRING, IDC_TOTAL, CTSTRING(REMOVE_ALL));
	ctxMenu.SetMenuDefaultItem(IDC_OPEN_FILE);

	tabMenu.CreatePopupMenu();

	if(BOOLSETTING(LOG_UPLOADS)) {
		tabMenu.AppendMenu(MF_STRING, IDC_UPLOAD_LOG, CTSTRING(OPEN_UPLOAD_LOG));
		tabMenu.AppendMenu(MF_SEPARATOR);
	}
	tabMenu.AppendMenu(MF_STRING, IDC_TOTAL, CTSTRING(REMOVE_ALL));
	tabMenu.AppendMenu(MF_SEPARATOR);
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));

	bHandled = FALSE;
	return TRUE;
}

LRESULT FinishedULFrame::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	
	NMITEMACTIVATE * const item = (NMITEMACTIVATE*) pnmh;

	if(item->iItem != -1) {
		FinishedItem* entry = (FinishedItem*)ctrlList.GetItemData(item->iItem);
		WinUtil::openFile(Text::toT(entry->getTarget()));
	}
	return 0;
}

LRESULT FinishedULFrame::onViewAsText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int i;
	if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		FinishedItem * const entry = (FinishedItem*)ctrlList.GetItemData(i);
		TextFrame::openWindow(Text::toT(entry->getTarget()));
	}
	return 0;
}

LRESULT FinishedULFrame::onOpenFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int i;
	if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		FinishedItem * const entry = (FinishedItem*)ctrlList.GetItemData(i);
		WinUtil::openFile(Text::toT(entry->getTarget()));
	}
	return 0;
}

LRESULT FinishedULFrame::onOpenFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int i;
	if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		FinishedItem * const entry = (FinishedItem*)ctrlList.GetItemData(i);
		::ShellExecute(NULL, NULL, Text::toT(Util::getFilePath(entry->getTarget())).c_str(), NULL, NULL, SW_SHOWNORMAL);
	}
	return 0;
}

LRESULT FinishedULFrame::onRemove(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	switch(wID)
	{
	case IDC_REMOVE:
		{
			int i = -1;
			while((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
				FinishedManager::getInstance()->remove((FinishedItem*)ctrlList.GetItemData(i), true);
				ctrlList.DeleteItem(i);
			}
			break;
		}
	case IDC_TOTAL:
		FinishedManager::getInstance()->removeAll(true);
		break;
	}
	return 0;
}

LRESULT FinishedULFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		FinishedManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		WinUtil::saveHeaderOrder(ctrlList, SettingsManager::FINISHED_UL_ORDER, 
			SettingsManager::FINISHED_UL_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);
		CZDCLib::setButtonPressed(IDC_FINISHED_UL, false);

		bHandled = FALSE;
	return 0;
	}
}

LRESULT FinishedULFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(wParam == SPEAK_ADD_LINE) {
		FinishedItem* entry = (FinishedItem*)lParam;

		for(int i = 0, j = ctrlList.GetItemCount(); i < j; ++i) {
			FinishedItem* fi = (FinishedItem*)ctrlList.GetItemData(i);

			if(!fi->mainitem) continue;

			if((fi->getUser() == entry->getUser()) && (fi->getTarget() == entry->getTarget())) {
				fi->subItems.push_back(entry);
				entry->main = fi;
				entry->mainitem = false;

				if(fi->subItems.size() == 1){
					ctrlList.SetItemState(i, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
				}else if(!fi->collapsed){
					addEntry(entry, i + 1);
				}
				return 0;
			}
		}
		entry->mainitem = true;
		entry->collapsed = true;

		addEntry(entry);

		if(BOOLSETTING(FINISHED_DIRTY))
			setDirty();
		updateStatus();
	} else if(wParam == SPEAK_REMOVE) {
		updateStatus();
	} else if(wParam == SPEAK_REMOVE_ALL) {
		ctrlList.DeleteAllItems();
		updateStatus();
	}
	return 0;
}

void FinishedULFrame::addEntry(FinishedItem* entry, int pos) {
	TStringList l;
	l.push_back(Text::toT(Util::getFileName(entry->getTarget())));
	l.push_back(Text::toT(Util::formatTime("%Y-%m-%d %H:%M:%S", entry->getTime())));
	l.push_back(Text::toT(Util::getFilePath(entry->getTarget())));
	l.push_back(Text::toT(entry->getUser()));
	l.push_back(Text::toT(entry->getHub()));
	l.push_back(Text::toT(Util::formatBytes(entry->getSize())));
	l.push_back(Text::toT(Util::formatBytes(entry->getAvgSpeed()) + "/s"));
	totalBytes += entry->getChunkSize();
	totalTime += entry->getMilliSeconds();

	int image = WinUtil::getIconIndex(Text::toT(entry->getTarget()));
	int loc = -1;
	if(pos == -1) {
		loc = ctrlList.insert(l, image, (LPARAM)entry);
	} else {
		LV_ITEM lvi;
		lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE | LVIF_INDENT;
		lvi.iItem = pos;
		lvi.iSubItem = 0;
		lvi.iIndent = 1;
		lvi.pszText = const_cast<TCHAR*>(ctrlList.getSortColumn() == -1 ? l[0].c_str() : l[ctrlList.getSortColumn()].c_str());
		lvi.cchTextMax = ctrlList.getSortColumn() == -1 ? l[0].size() : l[ctrlList.getSortColumn()].size();
		lvi.iImage = image;
		lvi.lParam = (LPARAM)entry;
		lvi.state = 0;
		lvi.stateMask = 0;
		loc = ctrlList.InsertItem(&lvi);
		int k = 0;
		for(TStringIter j = l.begin(); j != l.end(); ++j, k++) {
			ctrlList.SetItemText(loc, k, j->c_str());
		}
	}
	ctrlList.EnsureVisible(loc, FALSE);
}

LRESULT FinishedULFrame::onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 
	tabMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	return TRUE;
}

LRESULT FinishedULFrame::onUploadLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring filename = Text::toT(SETTING(LOG_DIRECTORY)) + _T("Uploads.log");
	if(Util::fileExists(Text::fromT(filename))){
		ShellExecute(NULL, NULL, filename.c_str(), NULL, NULL, SW_SHOWNORMAL);

	} else {
		MessageBox(CTSTRING(NO_UPLOAD_LOG), CTSTRING(NO_UPLOAD_LOG), MB_OK );	  
	}
	return 0;
}

void FinishedULFrame::on(SettingsManagerListener::Save, SimpleXML* /*xml*/) throw() {
	bool refresh = false;
	if(ctrlList.GetBkColor() != WinUtil::bgColor) {
		ctrlList.SetBkColor(WinUtil::bgColor);
		ctrlList.SetTextBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlList.GetTextColor() != WinUtil::textColor) {
		ctrlList.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

LRESULT FinishedULFrame::onLButton(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	bHandled = false;
	LPNMITEMACTIVATE item = (LPNMITEMACTIVATE)pnmh;
	
	if (item->iItem != -1) {
		CRect rect;
		ctrlList.GetItemRect(item->iItem, rect, LVIR_ICON);

		if (item->ptAction.x < rect.left)
		{
			FinishedItem* i = (FinishedItem*)ctrlList.GetItemData(item->iItem);
			if(i->subItems.size() > 0)
				if(i->collapsed) Expand(i,item->iItem); else Collapse(i,item->iItem);
		}
	}
	return 0;
} 

void FinishedULFrame::Collapse(FinishedItem* i, int a) {
	size_t q = 0;
	while(q<i->subItems.size()) {
		FinishedItem* j = i->subItems[q];
		int h = ctrlList.find((LPARAM)j);
		if(h != -1)
			ctrlList.DeleteItem(h);
		q++;
	}

	i->collapsed = true;
	ctrlList.SetItemState(a, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
}

void FinishedULFrame::Expand(FinishedItem* i, int a) {
	size_t q = 0;
	while(q < i->subItems.size()) {
		addEntry(i->subItems[q], a + 1);
		q++;
	}

	i->collapsed = false;
	ctrlList.SetItemState(a, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
	//ctrlResults.resort();
}

/**
 * @file
 * $Id$
 */
