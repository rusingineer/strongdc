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

#if !defined(AFX_SPYFRAME_H__19A67830_B811_4672_BBC2_3D793E0342E8__INCLUDED_)
#define AFX_SPYFRAME_H__19A67830_B811_4672_BBC2_3D793E0342E8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/ClientManager.h"
#include "../client/TimerManager.h"
#include "CZDCLib.h"
#include "../client/SearchManager.h"

#include "FlatTabCtrl.h"
#include "ExListViewCtrl.h"

class SpyFrame : public MDITabChildWindowImpl<SpyFrame, RGB(0, 0, 0), IDR_SPY>, public StaticFrame<SpyFrame, ResourceManager::SEARCH_SPY, IDC_SEARCH_SPY>,
	private ClientManagerListener, private TimerManagerListener
{
public:
	SpyFrame() : total(0), cur(0), closed(false) {
		ZeroMemory(perSecond, sizeof(perSecond));
		ClientManager::getInstance()->addListener(this);
		TimerManager::getInstance()->addListener(this);
	}

	virtual ~SpyFrame() {
	}

	enum {
		COLUMN_FIRST,
		COLUMN_STRING = COLUMN_FIRST,
		COLUMN_COUNT,
		COLUMN_USERS,
		COLUMN_LAST
	};

	DECLARE_FRAME_WND_CLASS_EX("SpyFrame", IDR_SPY, 0, COLOR_3DFACE)

	virtual void OnFinalMessage(HWND /*hWnd*/) { delete this; }

	typedef MDITabChildWindowImpl<SpyFrame, RGB(0, 0, 0), IDR_SPY> baseClass;
	BEGIN_MSG_MAP(SpyFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		COMMAND_ID_HANDLER(IDC_SEARCH, onSearch)
		NOTIFY_HANDLER(IDC_RESULTS, LVN_COLUMNCLICK, onColumnClickResults)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		if(!closed){
		ClientManager::getInstance()->removeListener(this);
		TimerManager::getInstance()->removeListener(this);

			bHandled = TRUE;
			closed = true;
			PostMessage(WM_CLOSE);
			return 0;
		} else {
			CZDCLib::setButtonPressed(IDC_SEARCH_SPY, false);
			m_hMenu = NULL;
			MDIDestroy(m_hWnd);
		return 0;
	}
	}

	LRESULT onColumnClickResults(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLISTVIEW* l = (NMLISTVIEW*)pnmh;
		if(l->iSubItem == ctrlSearches.getSortColumn()) {
			if (!ctrlSearches.isAscending())
				ctrlSearches.setSort(-1, ctrlSearches.getSortType());
			else
				ctrlSearches.setSortDirection(false);
		} else {
			if(l->iSubItem == COLUMN_COUNT) {
				ctrlSearches.setSort(l->iSubItem, ExListViewCtrl::SORT_INT);
			} else {
				ctrlSearches.setSort(l->iSubItem, ExListViewCtrl::SORT_STRING_NOCASE);
			}
		}
		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE);
	
private:

	enum { AVG_TIME = 60 };
	enum {
		SEARCH,
		TICK_AVG
	};

	ExListViewCtrl ctrlSearches;
	CStatusBarCtrl ctrlStatus;
	int total;
	int perSecond[AVG_TIME];
	int cur;
	string searchString;
	
	bool closed;
	
	CriticalSection cs;

	struct SearchData
	{
		SearchData() : curpos(0) { }
		int i;
		string seekers[3];

		void AddSeeker(const string& s)
		{
			seekers[curpos++] = s;
			curpos = curpos % 3;
		}
	private:
		int curpos;
	};

	typedef HASH_MAP<string, SearchData> SearchMap;
  	typedef SearchMap::iterator SearchIter;

  	SearchMap searches;

  	// ClientManagerListener
	struct SearchInfo
	{
		SearchInfo(const string& _user, const string& _s) : seeker(_user), s(_s) { }
		string seeker, s;
	};

	void onSearchResult(string aSearchString);

	// ClientManagerListener
	virtual void on(ClientManagerListener::IncomingSearch, const string& user, const string& s) throw();
	
	// TimerManagerListener
	virtual void on(TimerManagerListener::Second, u_int32_t) throw();
};

#endif // !defined(AFX_SPYFRAME_H__19A67830_B811_4672_BBC2_3D793E0342E8__INCLUDED_)

/**
 * @file
 * $Id$
 */