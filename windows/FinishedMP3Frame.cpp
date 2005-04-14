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

#include "FinishedMP3Frame.h"
#include "WinUtil.h"
#include "CZDCLib.h"
#include "TextFrame.h"

#include "../client/ClientManager.h"
#include "../client/StringTokenizer.h"

int FinishedMP3Frame::columnIndexes[] = { COLUMN_FILE, COLUMN_DONE, COLUMN_NICK, COLUMN_HUB, COLUMN_SIZE, COLUMN_TYPE, COLUMN_MODE, COLUMN_FREQUENCY, COLUMN_BITRATE };
int FinishedMP3Frame::columnSizes[] = { 100, 80, 120, 80, 80, 80, 100, 100, 100 };
static ResourceManager::Strings columnNames[] = { ResourceManager::FILENAME, ResourceManager::TIME,
ResourceManager::NICK, ResourceManager::HUB, ResourceManager::SIZE, ResourceManager::MPEG_VER, ResourceManager::CHANNEL_MODE, ResourceManager::FREQUENCY, ResourceManager::BITRATE
};

LRESULT FinishedMP3Frame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	
	ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_FINISHED);
	ctrlList.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | (BOOLSETTING(SHOW_INFOTIPS) ? LVS_EX_INFOTIP : 0));	

	ctrlList.SetImageList(WinUtil::fileImages, LVSIL_SMALL);
	ctrlList.SetBkColor(WinUtil::bgColor);
	ctrlList.SetTextBkColor(WinUtil::bgColor);
	ctrlList.SetTextColor(WinUtil::textColor);
	
	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(FINISHEDMP3_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(FINISHEDMP3_WIDTHS), COLUMN_LAST);
	
	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_BITRATE || j == COLUMN_FREQUENCY) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlList.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlList.SetColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlList.setSort(COLUMN_DONE, ExListViewCtrl::SORT_STRING_NOCASE);

	UpdateLayout();
	
	FinishedManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);
	updateList(FinishedManager::getInstance()->lockMP3List());
	FinishedManager::getInstance()->unlockList();


	bHandled = FALSE;
	return TRUE;
}

LRESULT FinishedMP3Frame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		FinishedManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);	
		closed = true;
		CZDCLib::setButtonPressed(IDC_FINISHEDMP3, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		WinUtil::saveHeaderOrder(ctrlList, SettingsManager::FINISHEDMP3_ORDER, 
			SettingsManager::FINISHEDMP3_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);
	
		bHandled = FALSE;
		return 0;
	}
}

LRESULT FinishedMP3Frame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(wParam == SPEAK_ADD_LINE) {
		FinishedMP3Item* entry = (FinishedMP3Item*)lParam;
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

void FinishedMP3Frame::on(FinishedManagerListener::Added_MP3Dl(), FinishedMP3Item* entry) throw() {
	PostMessage(WM_SPEAKER, SPEAK_ADD_LINE, (WPARAM)entry);
};

void FinishedMP3Frame::addEntry(FinishedMP3Item* entry) {
	TStringList l;
	l.push_back(Util::getFileName(Text::toT(entry->getTarget())));
	l.push_back(Text::toT(Util::formatTime("%Y-%m-%d %H:%M:%S", entry->getTime())));
	l.push_back(Text::toT(entry->getUser()));
	l.push_back(Text::toT(entry->getHub()));
	l.push_back(Text::toT(Util::formatBytes(entry->getSize())));
	l.push_back(Text::toT(entry->getMPEGVer()));
	l.push_back(Text::toT(entry->getChannels()));
	l.push_back(Text::toT(Util::toString(entry->getSampleRate())+" Hz"));
	l.push_back(Text::toT(Util::toString(entry->getBitRate())+" kbps"));
	int image = WinUtil::getIconIndex(Text::toT(entry->getTarget()));
	int loc = ctrlList.insert(l, image, (LPARAM)entry);
	ctrlList.EnsureVisible(loc, FALSE);
}

void FinishedMP3Frame::on(SettingsManagerListener::Save, SimpleXML* /*xml*/) throw() {
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

/**
 * @file
 * $Id$
 */
