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

#include "FinishedULFrame.h"
#include "WinUtil.h"
#include "CZDCLib.h"	
#include "TextFrame.h"

#include "../client/ClientManager.h"
#include "../client/StringTokenizer.h"

int FinishedULFrame::columnIndexes[] = { COLUMN_DONE, COLUMN_FILE, COLUMN_PATH, COLUMN_NICK, COLUMN_HUB, COLUMN_SIZE, COLUMN_SPEED };
int FinishedULFrame::columnSizes[] = { 100, 110, 290, 125, 80, 80, 80 };
static ResourceManager::Strings columnNames[] = { ResourceManager::TIME, ResourceManager::FILENAME, ResourceManager::PATH, 
ResourceManager::NICK, ResourceManager::HUB, ResourceManager::SIZE, ResourceManager::SPEED
};

LRESULT FinishedULFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	
	ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_FINISHED_UL);

	DWORD styles = LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT;
	if (BOOLSETTING(SHOW_INFOTIPS))
		styles |= LVS_EX_INFOTIP;
	ctrlList.SetExtendedListViewStyle(styles);
	
	ctrlList.SetImageList(WinUtil::fileImages, LVSIL_SMALL);
	ctrlList.SetBkColor(WinUtil::bgColor);
	ctrlList.SetTextBkColor(WinUtil::bgColor);
	ctrlList.SetTextColor(WinUtil::textColor);
	
	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(FINISHED_UL_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(FINISHED_UL_WIDTHS), COLUMN_LAST);
	
	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_SPEED) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlList.InsertColumn(j, CSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlList.SetColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlList.setSort(COLUMN_DONE, ExListViewCtrl::SORT_STRING_NOCASE);
	
	UpdateLayout();
	
	FinishedManager::getInstance()->addListener(this);
	updateList(FinishedManager::getInstance()->lockList(true));
	FinishedManager::getInstance()->unlockList();
	
	ctxMenu.CreatePopupMenu();
	ctxMenu.AppendMenu(MF_STRING, IDC_VIEW_AS_TEXT, CSTRING(VIEW_AS_TEXT));
	ctxMenu.AppendMenu(MF_STRING, IDC_OPEN_FILE, CSTRING(OPEN));
	ctxMenu.AppendMenu(MF_STRING, IDC_OPEN_FOLDER, CSTRING(OPEN_FOLDER));
	ctxMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	ctxMenu.AppendMenu(MF_STRING, IDC_REMOVE, CSTRING(REMOVE));
	ctxMenu.AppendMenu(MF_STRING, IDC_TOTAL, CSTRING(REMOVE_ALL));
	ctxMenu.SetMenuDefaultItem(IDC_OPEN_FILE);

	tabMenu.CreatePopupMenu();

	if(BOOLSETTING(LOG_UPLOADS)) {
		tabMenu.AppendMenu(MF_STRING, IDC_UPLOAD_LOG, CSTRING(OPEN_UPLOAD_LOG));
		tabMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	}
	tabMenu.AppendMenu(MF_STRING, IDC_TOTAL, CSTRING(REMOVE_ALL));
	tabMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CSTRING(CLOSE));

	bHandled = FALSE;
	return TRUE;
}

LRESULT FinishedULFrame::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	
	NMITEMACTIVATE * const item = (NMITEMACTIVATE*) pnmh;

	if(item->iItem != -1) {
		FinishedItem* entry = (FinishedItem*)ctrlList.GetItemData(item->iItem);
		WinUtil::openFile(entry->getTarget());
	}
	return 0;
}

LRESULT FinishedULFrame::onViewAsText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int i;
	if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		FinishedItem * const entry = (FinishedItem*)ctrlList.GetItemData(i);
		TextFrame::openWindow(entry->getTarget());
	}
	return 0;
}

LRESULT FinishedULFrame::onOpenFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int i;
	if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		FinishedItem * const entry = (FinishedItem*)ctrlList.GetItemData(i);
		WinUtil::openFile(entry->getTarget());
	}
	return 0;
}

LRESULT FinishedULFrame::onOpenFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int i;
	if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		FinishedItem * const entry = (FinishedItem*)ctrlList.GetItemData(i);
		::ShellExecute(NULL, NULL, Util::getFilePath(entry->getTarget()).c_str(), NULL, NULL, SW_SHOWNORMAL);
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
	if(!closed){
	FinishedManager::getInstance()->removeListener(this);
	
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

void FinishedULFrame::addEntry(FinishedItem* entry) {
	StringList l;

	l.push_back(Util::formatTime("%Y-%m-%d %H:%M:%S", entry->getTime()));
	l.push_back(Util::getFileName(entry->getTarget()));

	l.push_back(Util::getFilePath(entry->getTarget()));
	l.push_back(entry->getUser());
	l.push_back(entry->getHub());
	l.push_back(Util::formatBytes(entry->getSize()));
	l.push_back(Util::formatBytes(entry->getAvgSpeed()) + "/s");
	totalBytes += entry->getChunkSize();
	totalTime += entry->getMilliSeconds();

	int image = WinUtil::getIconIndex(entry->getTarget());
	int loc = ctrlList.insert(l, image, (LPARAM)entry);
	ctrlList.EnsureVisible(loc, FALSE);
}

LRESULT FinishedULFrame::onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 
	tabMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	return TRUE;
}

LRESULT FinishedULFrame::onUploadLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	string filename = SETTING(LOG_DIRECTORY) + "Uploads.log";
	if(File::existsFile(filename)){
		ShellExecute(NULL, NULL, filename.c_str(), NULL, NULL, SW_SHOWNORMAL);

	} else {
		MessageBox(CSTRING(NO_UPLOAD_LOG),CSTRING(NO_UPLOAD_LOG), MB_OK );	  
	}
	return 0;
}

/**
 * @file
 * $Id$
 */