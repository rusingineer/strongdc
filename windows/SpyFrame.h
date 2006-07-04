/*
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(SPY_FRAME_H)
#define SPY_FRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/ClientManager.h"
#include "../client/TimerManager.h"
#include "CZDCLib.h"
#include "../client/SearchManager.h"

#include "FlatTabCtrl.h"
#include "ExListViewCtrl.h"

#define IGNORETTH_MESSAGE_MAP 7

class SpyFrame : public MDITabChildWindowImpl<SpyFrame, RGB(0, 0, 0), IDR_SPY>, public StaticFrame<SpyFrame, ResourceManager::SEARCH_SPY, IDC_SEARCH_SPY>,
	private ClientManagerListener, private TimerManagerListener, private SettingsManagerListener
{
public:
	SpyFrame() : total(0), cur(0), closed(false), ignoreTth(BOOLSETTING(SPY_FRAME_IGNORE_TTH_SEARCHES)), ignoreTthContainer(WC_BUTTON, this, IGNORETTH_MESSAGE_MAP) {
		ZeroMemory(perSecond, sizeof(perSecond));
		ClientManager::getInstance()->addListener(this);
		TimerManager::getInstance()->addListener(this);
		SettingsManager::getInstance()->addListener(this);
	}

	virtual ~SpyFrame() {
	}

	enum {
		COLUMN_FIRST,
		COLUMN_STRING = COLUMN_FIRST,
		COLUMN_COUNT,
		COLUMN_USERS,
		COLUMN_TIME,
		COLUMN_LAST
	};

	static int columnIndexes[COLUMN_LAST];
	static int columnSizes[COLUMN_LAST];

	DECLARE_FRAME_WND_CLASS_EX(_T("SpyFrame"), IDR_SPY, 0, COLOR_3DFACE)

	typedef MDITabChildWindowImpl<SpyFrame, RGB(0, 0, 0), IDR_SPY> baseClass;
	BEGIN_MSG_MAP(SpyFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		COMMAND_ID_HANDLER(IDC_SEARCH, onSearch)
		NOTIFY_HANDLER(IDC_RESULTS, LVN_COLUMNCLICK, onColumnClickResults)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(IGNORETTH_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onIgnoreTth)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onColumnClickResults(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	void UpdateLayout(BOOL bResizeBars = TRUE);

	LRESULT onIgnoreTth(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		ignoreTth = (wParam == BST_CHECKED);
		return 0;
	}
	
private:

	enum { AVG_TIME = 60 };
	enum {
		SEARCH,
		TICK_AVG
	};

	ExListViewCtrl ctrlSearches;
	CStatusBarCtrl ctrlStatus;
	CContainedWindow ignoreTthContainer;
	CButton ctrlIgnoreTth;
	int total;
	int perSecond[AVG_TIME];
	int cur;
	tstring searchString;

	bool closed;
	bool ignoreTth;
	
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
	virtual void on(TimerManagerListener::Second, time_t) throw();
	virtual void on(SettingsManagerListener::Save, SimpleXML* /*xml*/) throw();
};

#endif // !defined(SPY_FRAME_H)

/**
 * @file
 * $Id$
 */
