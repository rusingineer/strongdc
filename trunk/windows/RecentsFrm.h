/* 
 * Copyright (C) 2001-2003 sickb0y, sickb0y@p2pitalia.com
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
#if !defined(__RECENTS_FRAME_H__)
#define __RECENTS_FRAME_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "ExListViewCtrl.h"
#include "WinUtil.h"
#include "../client/HubManager.h"

#define SERVER_MESSAGE_MAP 7

class RecentHubsFrame : public MDITabChildWindowImpl<RecentHubsFrame, RGB(0, 0, 0), IDR_RECENTS>, public StaticFrame<RecentHubsFrame, ResourceManager::RECENT_HUBS, IDC_RECENTS>, private HubManagerListener
{
public:
	RecentHubsFrame() : nosave(true), closed(false) { };
	virtual ~RecentHubsFrame() { };

	DECLARE_FRAME_WND_CLASS_EX("RecentHubsFrame", IDR_RECENTS, 0, COLOR_3DFACE);
		
	virtual void OnFinalMessage(HWND /*hWnd*/) {
		frame = NULL;
		delete this;
	}

	typedef MDITabChildWindowImpl<RecentHubsFrame, RGB(0, 0, 0), IDR_RECENTS> baseClass;
	BEGIN_MSG_MAP(RecentHubsFrame)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		COMMAND_ID_HANDLER(IDC_CONNECT, onClickedConnect)
		COMMAND_ID_HANDLER(IDC_ADD, onAdd)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_REMOVE_ALL, onRemoveAll)
		NOTIFY_HANDLER(IDC_HUBLIST, LVN_COLUMNCLICK, onColumnClickHublist)
		NOTIFY_HANDLER(IDC_HUBLIST, NM_DBLCLK, onDoubleClickHublist)
		NOTIFY_HANDLER(IDC_HUBLIST, NM_RETURN, onEnter)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()
		
	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onDoubleClickHublist(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onEnter(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onClickedConnect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onRemoveAll(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	void UpdateLayout(BOOL bResizeBars = TRUE);
	
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		RECT rc;                    // client area of window 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 
		
		if(ctrlHubs.GetSelectedCount() > 0) {
			// Get the bounding rectangle of the client area. 
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
	
	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlHubs.SetFocus();
		return 0;
	}

	LRESULT onColumnClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLISTVIEW* l = (NMLISTVIEW*)pnmh;
		if(l->iSubItem == ctrlHubs.getSortColumn()) {
			if (!ctrlHubs.isAscending())
				ctrlHubs.setSort(-1, ctrlHubs.getSortType());
			else
				ctrlHubs.setSortDirection(false);
		} else {
			if(l->iSubItem == 2) {
				ctrlHubs.setSort(l->iSubItem, ExListViewCtrl::SORT_INT);
			} else {
				ctrlHubs.setSort(l->iSubItem, ExListViewCtrl::SORT_STRING_NOCASE);
			}
		}
		return 0;
	}
	
	static RecentHubsFrame* frame;
	
private:
	enum {
		COLUMN_FIRST,
		COLUMN_NAME = COLUMN_FIRST,
		COLUMN_DESCRIPTION,
		COLUMN_USERS,
		COLUMN_SERVER,
		COLUMN_LAST
	};
	
	CButton ctrlConnect;
	CButton ctrlRemove;
	CButton ctrlRemoveAll;

	CMenu hubsMenu;
	
	ExListViewCtrl ctrlHubs;

	bool nosave;
	
	static int columnSizes[COLUMN_LAST];
	static int columnIndexes[COLUMN_LAST];
	
	void updateList(const RecentHubEntry::List& fl) {
		ctrlHubs.SetRedraw(FALSE);
		for(RecentHubEntry::List::const_iterator i = fl.begin(); i != fl.end(); ++i) {
			addEntry(*i, ctrlHubs.GetItemCount());
		}
		ctrlHubs.SetRedraw(TRUE);
		ctrlHubs.Invalidate();
	}

	void addEntry(RecentHubEntry* entry, int pos) {
		StringList l;
		l.push_back(entry->getName());
		l.push_back(entry->getDescription());
		l.push_back(entry->getUsers());
		l.push_back(entry->getServer());

		ctrlHubs.insert(pos, l, 0, (LPARAM)entry);
	}
	bool closed;
	virtual void onAction(HubManagerListener::Types type, RecentHubEntry* entry) throw();
};

#endif

/**
 * @file
 * $Id$
 */