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

#ifndef FINISHEDMP3FRAME_H
#define FINISHEDMP3FRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "ExListViewCtrl.h"

#include "../client/FinishedManager.h"

#define SERVER_MESSAGE_MAP 7

class FinishedMP3Frame : public MDITabChildWindowImpl<FinishedMP3Frame, RGB(0, 0, 0), IDR_FINISHED_MP3>, public StaticFrame<FinishedMP3Frame, ResourceManager::FINISHED_MP3_DOWNLOADS, IDC_FINISHEDMP3>,
	private FinishedManagerListener, private SettingsManagerListener
{
public:
	typedef MDITabChildWindowImpl<FinishedMP3Frame, RGB(0, 0, 0), IDR_FINISHED_MP3> baseClass;
	FinishedMP3Frame() : totalBytes(0), totalTime(0), closed(false) { };
	virtual ~FinishedMP3Frame() { };

	DECLARE_FRAME_WND_CLASS_EX(_T("FinishedMP3Frame"), IDR_FINISHED_MP3, 0, COLOR_3DFACE);
		
	virtual void OnFinalMessage(HWND /*hWnd*/) {
		delete this;
	}

	BEGIN_MSG_MAP(FinishedMP3Frame)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		NOTIFY_HANDLER(IDC_FINISHED, LVN_COLUMNCLICK, onColumnClickFinished)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()
		
	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	
	LRESULT onSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) {
		ctrlList.SetFocus();
		return 0;
	}
	
	static int sortSize(LPARAM a, LPARAM b) {
		FinishedItem* c = (FinishedItem*)a;
		FinishedItem* d = (FinishedItem*)b;
		return compare(c->getSize(), d->getSize());
	}

	static int sortSpeed(LPARAM a, LPARAM b) {
		FinishedItem* c = (FinishedItem*)a;
		FinishedItem* d = (FinishedItem*)b;
		return compare(c->getAvgSpeed(), d->getAvgSpeed());
	}

	LRESULT onColumnClickFinished(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLISTVIEW* const l = (NMLISTVIEW*)pnmh;
		if(l->iSubItem == ctrlList.getSortColumn()) {
			if (!ctrlList.isAscending())
				ctrlList.setSort(-1, ctrlList.getSortType());
			else
				ctrlList.setSortDirection(false);
		} else {
			switch(l->iSubItem) {
			case COLUMN_SIZE:
				ctrlList.setSort(l->iSubItem, ExListViewCtrl::SORT_FUNC, true, sortSize);
				break;
/*			case COLUMN_SPEED:
				ctrlList.setSort(l->iSubItem, ExListViewCtrl::SORT_FUNC, true, sortSpeed);
				break;*/
			default:
				ctrlList.setSort(l->iSubItem, ExListViewCtrl::SORT_STRING_NOCASE);
				break;
			}
		}
		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE)
	{
		RECT rect;
		GetClientRect(&rect);

		// position bars and offset their dimensions
		UpdateBarsPosition(rect, bResizeBars);

		if(ctrlStatus.IsWindow()) {
			CRect sr;
			int w[3];
			ctrlStatus.GetClientRect(sr);
			w[2] = sr.right - 16;
			w[1] = max(w[2] - 100, 0);
			w[0] = max(w[1] - 100, 0);
			
			ctrlStatus.SetParts(3, w);
		}
		
		CRect rc(rect);
		ctrlList.MoveWindow(rc);
	}
	
private:
	enum {
		SPEAK_ADD_LINE,
		SPEAK_REMOVE,
		SPEAK_REMOVE_ALL
	};

	enum {
		COLUMN_FIRST,
		COLUMN_FILE = COLUMN_FIRST,
		COLUMN_DONE,
		COLUMN_NICK,
		COLUMN_HUB,
		COLUMN_SIZE,
		COLUMN_TYPE,
		COLUMN_MODE,
		COLUMN_FREQUENCY,
		COLUMN_BITRATE,
		COLUMN_LAST
	};
	
	CStatusBarCtrl ctrlStatus;
	
	ExListViewCtrl ctrlList;
	
	int64_t totalBytes;
	int64_t totalTime;
	
	bool closed;

	static int columnSizes[COLUMN_LAST];
	static int columnIndexes[COLUMN_LAST];
	
	void updateStatus() {
		ctrlStatus.SetText(1, Text::toT(Util::formatBytes(totalBytes)).c_str());
		ctrlStatus.SetText(2, Text::toT(Util::formatBytes((totalTime > 0) ? totalBytes * ((int64_t)1000) / totalTime : 0) + "/s").c_str());
	}

	void updateList(const FinishedMP3Item::List& fl) {
		ctrlList.SetRedraw(FALSE);
		for(FinishedMP3Item::List::const_iterator i = fl.begin(); i != fl.end(); ++i) {
			addEntry(*i);
		}
		ctrlList.SetRedraw(TRUE);
		ctrlList.Invalidate();
		updateStatus();
	}

	void addEntry(FinishedMP3Item* entry);
	
	virtual void on(FinishedManagerListener::Added_MP3Dl(), FinishedMP3Item* entry) throw();
//	virtual void on(FinishedManagerListener::AddedDl(), FinishedItem* entry) throw();
	virtual void on(SettingsManagerListener::Save, SimpleXML* /*xml*/) throw();

	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}
};

#endif // FINISHEDFRAME_H

/**
 * @file
 * $Id$
 */
