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

#include "SearchFrm.h"
#include "LineDlg.h"
#include "CZDCLib.h"

#include "../client/QueueManager.h"
#include "../client/StringTokenizer.h"
#include "../client/ClientManager.h"
#include "../client/TimerManager.h"
#include "../client/SearchManager.h"

#include "../pme-1.0.4/pme.h"

TStringList SearchFrame::lastSearches;

int SearchFrame::columnIndexes[] = { COLUMN_FILENAME, COLUMN_HITS, COLUMN_NICK, COLUMN_TYPE, COLUMN_SIZE,
	COLUMN_PATH, COLUMN_SLOTS, COLUMN_CONNECTION, COLUMN_HUB, COLUMN_EXACT_SIZE, COLUMN_UPLOAD, COLUMN_IP, COLUMN_TTH };
int SearchFrame::columnSizes[] = { 210, 80, 100, 50, 80, 100, 40, 70, 150, 80, 80, 100, 150 };

static ResourceManager::Strings columnNames[] = {ResourceManager::FILE,  ResourceManager::HIT_COUNT, ResourceManager::USER, ResourceManager::TYPE, ResourceManager::SIZE,
	ResourceManager::PATH, ResourceManager::SLOTS, ResourceManager::CONNECTION, 
	ResourceManager::HUB, ResourceManager::EXACT_SIZE, ResourceManager::AVERAGE_UPLOAD, ResourceManager::IP_BARE, ResourceManager::TTH_ROOT };

void SearchFrame::openWindow(const tstring& str /* = Util::emptyString */, LONGLONG size /* = 0 */, SearchManager::SizeModes mode /* = SearchManager::SIZE_ATLEAST */, SearchManager::TypeModes type /* = SearchManager::TYPE_ANY */) {
	SearchFrame* pChild = new SearchFrame();
	pChild->setInitial(str, size, mode, type);
	pChild->CreateEx(WinUtil::mdiClient);
}

LRESULT SearchFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	states.CreateFromImage(IDB_STATE, 16, 2, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);

	ctrlSearchBox.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL, 0);
	for(TStringIter i = lastSearches.begin(); i != lastSearches.end(); ++i) {
		ctrlSearchBox.InsertString(0, i->c_str());
	}
	searchBoxContainer.SubclassWindow(ctrlSearchBox.m_hWnd);
	ctrlSearchBox.SetExtendedUI();
	
	ctrlPurge.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_PURGE);
	ctrlPurge.SetWindowText(CTSTRING(PURGE));
	ctrlPurge.SetFont(WinUtil::systemFont);
	purgeContainer.SubclassWindow(ctrlPurge.m_hWnd);
	
	ctrlMode.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);
	modeContainer.SubclassWindow(ctrlMode.m_hWnd);

	ctrlSize.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL | ES_NUMBER, WS_EX_CLIENTEDGE);
	sizeContainer.SubclassWindow(ctrlSize.m_hWnd);
	
	ctrlSizeMode.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);
	sizeModeContainer.SubclassWindow(ctrlSizeMode.m_hWnd);

	ctrlFiletype.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_OWNERDRAWFIXED, WS_EX_CLIENTEDGE, IDC_FILETYPES);

	searchTypes.CreateFromImage(IDB_SEARCH_TYPES, 16, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	fileTypeContainer.SubclassWindow(ctrlFiletype.m_hWnd);

	if (BOOLSETTING(USE_SYSTEM_ICONS)) {
		ctrlResults.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
			WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_RESULTS);
	} else {
		ctrlResults.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
			WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, IDC_RESULTS);
	}
	resultsContainer.SubclassWindow(ctrlResults.m_hWnd);
	
	DWORD styles = LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT;
	if (BOOLSETTING(SHOW_INFOTIPS))
		styles |= LVS_EX_INFOTIP;

	styles |= 0x00010000;

	ctrlResults.SetExtendedListViewStyle(styles);
	
	if (BOOLSETTING(USE_SYSTEM_ICONS)) {
	ctrlResults.SetImageList(WinUtil::fileImages, LVSIL_SMALL);
	} else {
		images.CreateFromImage(IDB_SPEEDS, 16, 3, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
		ctrlResults.SetImageList(images, LVSIL_SMALL);
	}
	ctrlResults.SetImageList(states, LVSIL_STATE); 

	ctrlHubs.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_NOCOLUMNHEADER, WS_EX_CLIENTEDGE, IDC_HUB);
	ctrlHubs.SetExtendedListViewStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
	hubsContainer.SubclassWindow(ctrlHubs.m_hWnd);	

	ctrlFilter.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);

	ctrlFilterContainer.SubclassWindow(ctrlFilter.m_hWnd);
	ctrlFilter.SetFont(WinUtil::font);

	ctrlFilterSel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL |
		WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);

	ctrlFilterSelContainer.SubclassWindow(ctrlFilterSel.m_hWnd);
	ctrlFilterSel.SetFont(WinUtil::font);

	searchLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	searchLabel.SetFont(WinUtil::systemFont, FALSE);
	searchLabel.SetWindowText(CTSTRING(SEARCH_FOR));

	sizeLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	sizeLabel.SetFont(WinUtil::systemFont, FALSE);
	sizeLabel.SetWindowText(CTSTRING(SIZE));

	typeLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	typeLabel.SetFont(WinUtil::systemFont, FALSE);
	typeLabel.SetWindowText(CTSTRING(FILE_TYPE));

	srLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	srLabel.SetFont(WinUtil::systemFont, FALSE);
	srLabel.SetWindowText(CTSTRING(SEARCH_IN_RESULTS));

	optionLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	optionLabel.SetFont(WinUtil::systemFont, FALSE);
	optionLabel.SetWindowText(CTSTRING(SEARCH_OPTIONS));

	hubsLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	hubsLabel.SetFont(WinUtil::systemFont, FALSE);
	hubsLabel.SetWindowText(CTSTRING(HUBS));

	ctrlSlots.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_FREESLOTS);
	ctrlSlots.SetButtonStyle(BS_AUTOCHECKBOX, FALSE);
	ctrlSlots.SetFont(WinUtil::systemFont, FALSE);
	ctrlSlots.SetWindowText(CTSTRING(ONLY_FREE_SLOTS));
	slotsContainer.SubclassWindow(ctrlSlots.m_hWnd);

	ctrlTTH.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_TTHONLY);
	ctrlTTH.SetButtonStyle(BS_AUTOCHECKBOX, FALSE);
	ctrlTTH.SetFont(WinUtil::systemFont, FALSE);
	ctrlTTH.SetWindowText(CTSTRING(ONLY_TTH));
	ctrlTTH.SetCheck(BOOLSETTING(SEARCH_ONLY_TTH));
	tthContainer.SubclassWindow(ctrlTTH.m_hWnd);

	ctrlCollapsed.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_COLLAPSED);
	ctrlCollapsed.SetButtonStyle(BS_AUTOCHECKBOX, FALSE);
	ctrlCollapsed.SetFont(WinUtil::systemFont, FALSE);
	ctrlCollapsed.SetWindowText(CTSTRING(EXPANDED_RESULTS));
	collapsedContainer.SubclassWindow(ctrlCollapsed.m_hWnd);

	if(BOOLSETTING(FREE_SLOTS_DEFAULT)) {
		ctrlSlots.SetCheck(true);
		onlyFree = true;
	}

	ctrlShowUI.Create(ctrlStatus.m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlShowUI.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowUI.SetCheck(1);
	showUIContainer.SubclassWindow(ctrlShowUI.m_hWnd);

	ctrlDoSearch.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_SEARCH);
	ctrlDoSearch.SetWindowText(CTSTRING(SEARCH));
	ctrlDoSearch.SetFont(WinUtil::systemFont);
	doSearchContainer.SubclassWindow(ctrlDoSearch.m_hWnd);

	ctrlPauseSearch.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON, 0, IDC_SEARCH_PAUSE);
	ctrlPauseSearch.SetWindowText(CTSTRING(PAUSE_SEARCH));
	ctrlPauseSearch.SetFont(WinUtil::systemFont);

	ctrlSearchBox.SetFont(WinUtil::systemFont, FALSE);
	ctrlSize.SetFont(WinUtil::systemFont, FALSE);
	ctrlMode.SetFont(WinUtil::systemFont, FALSE);
	ctrlSizeMode.SetFont(WinUtil::systemFont, FALSE);
	ctrlFiletype.SetFont(WinUtil::systemFont, FALSE);

	ctrlMode.AddString(CTSTRING(NORMAL));
	ctrlMode.AddString(CTSTRING(AT_LEAST));
	ctrlMode.AddString(CTSTRING(AT_MOST));
	ctrlMode.AddString(CTSTRING(EXACT_SIZE));
	ctrlMode.SetCurSel(1);
	
	ctrlSizeMode.AddString(CTSTRING(B));
	ctrlSizeMode.AddString(CTSTRING(KB));
	ctrlSizeMode.AddString(CTSTRING(MB));
	ctrlSizeMode.AddString(CTSTRING(GB));
	if(initialSize == 0)
		ctrlSizeMode.SetCurSel(2);
	else
		ctrlSizeMode.SetCurSel(0);

	ctrlFiletype.AddString(CTSTRING(ANY));
	ctrlFiletype.AddString(CTSTRING(AUDIO));
	ctrlFiletype.AddString(CTSTRING(COMPRESSED));
	ctrlFiletype.AddString(CTSTRING(DOCUMENT));
	ctrlFiletype.AddString(CTSTRING(EXECUTABLE));
	ctrlFiletype.AddString(CTSTRING(PICTURE));
	ctrlFiletype.AddString(CTSTRING(VIDEO));
	ctrlFiletype.AddString(CTSTRING(DIRECTORY));
	ctrlFiletype.AddString(_T("TTH"));
	ctrlFiletype.SetCurSel(0);
	
	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(SEARCHFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(SEARCHFRAME_WIDTHS), COLUMN_LAST);

	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_EXACT_SIZE) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlResults.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlResults.SetColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlResults.setSortColumn(COLUMN_HITS);
	ctrlResults.setAscending(false);

	ctrlResults.SetBkColor(WinUtil::bgColor);
	ctrlResults.SetTextBkColor(WinUtil::bgColor);
	ctrlResults.SetTextColor(WinUtil::textColor);
	ctrlResults.SetFont(WinUtil::systemFont, FALSE);	// use Util::font instead to obey Appearace settings
	ctrlResults.setFlickerFree(WinUtil::bgBrush);
	
	ctrlHubs.InsertColumn(0, _T("Dummy"), LVCFMT_LEFT, LVSCW_AUTOSIZE, 0);
	ctrlHubs.SetBkColor(WinUtil::bgColor);
	ctrlHubs.SetTextBkColor(WinUtil::bgColor);
	ctrlHubs.SetTextColor(WinUtil::textColor);
	ctrlHubs.SetFont(WinUtil::systemFont, FALSE);	// use Util::font instead to obey Appearace settings
	initHubs();

	copyMenu.CreatePopupMenu();
	grantMenu.CreatePopupMenu();
	targetDirMenu.CreatePopupMenu();
	targetMenu.CreatePopupMenu();
	resultsMenu.CreatePopupMenu();

	copyMenu.AppendMenu(MF_STRING, IDC_COPY_NICK, CTSTRING(COPY_NICK));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_FILENAME, CTSTRING(FILENAME));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_PATH, CTSTRING(PATH));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_SIZE, CTSTRING(SIZE));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_TTH, CTSTRING(TTH_ROOT));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_LINK, CTSTRING(COPY_MAGNET_LINK));
	copyMenu.InsertSeparator(0, TRUE, STRING(USERINFO));
	
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CTSTRING(GRANT_EXTRA_SLOT));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_HOUR, CTSTRING(GRANT_EXTRA_SLOT_HOUR));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_DAY, CTSTRING(GRANT_EXTRA_SLOT_DAY));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_WEEK, CTSTRING(GRANT_EXTRA_SLOT_WEEK));
	grantMenu.AppendMenu(MF_SEPARATOR);
	grantMenu.AppendMenu(MF_STRING, IDC_UNGRANTSLOT, CTSTRING(REMOVE_EXTRA_SLOT));
	grantMenu.InsertSeparator(0, TRUE, STRING(GRANT_SLOTS_MENU));
	
	resultsMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD, CTSTRING(DOWNLOAD));
	resultsMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)targetMenu, CTSTRING(DOWNLOAD_TO));
	resultsMenu.AppendMenu(MF_STRING, IDC_DOWNLOADDIR, CTSTRING(DOWNLOAD_WHOLE_DIR));
	resultsMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)targetDirMenu, CTSTRING(DOWNLOAD_WHOLE_DIR_TO));
	resultsMenu.AppendMenu(MF_STRING, IDC_VIEW_AS_TEXT, CTSTRING(VIEW_AS_TEXT));
	resultsMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES));
	resultsMenu.AppendMenu(MF_STRING, IDC_MP3, CTSTRING(GET_MP3INFO));
	resultsMenu.AppendMenu(MF_SEPARATOR);
	appendUserItems(resultsMenu);
	resultsMenu.DeleteMenu(resultsMenu.GetMenuItemCount()-2, MF_BYPOSITION);
	resultsMenu.AppendMenu(MF_SEPARATOR);
	resultsMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)copyMenu, CTSTRING(COPY));
	resultsMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)grantMenu, CTSTRING(GRANT_SLOTS_MENU));
	resultsMenu.SetMenuDefaultItem(IDC_DOWNLOAD);
	resultsMenu.AppendMenu(MF_SEPARATOR);
	resultsMenu.AppendMenu(MF_STRING, IDC_BITZI_LOOKUP, CTSTRING(BITZI_LOOKUP));

	UpdateLayout();

	if(!initialString.empty()) {
		lastSearches.push_back(initialString);
		ctrlSearchBox.InsertString(0, initialString.c_str());
		ctrlSearchBox.SetCurSel(0);
		ctrlMode.SetCurSel(initialMode);
		ctrlSize.SetWindowText(Text::toT(Util::toString(initialSize)).c_str());
		ctrlFiletype.SetCurSel(initialType);
		onEnter();
	} else {
		SetWindowText(CTSTRING(SEARCH));
		::EnableWindow(GetDlgItem(IDC_SEARCH_PAUSE), FALSE);
	}

	TCHAR Buffer[500];
	LV_COLUMN lvCol;
	int indexes[32];
	ctrlResults.GetColumnOrderArray(ctrlResults.GetHeader().GetItemCount(), indexes);
	for (int i = 0; i < ctrlResults.GetHeader().GetItemCount(); ++i) {
		lvCol.mask = LVCF_TEXT;
		lvCol.pszText = Buffer;
		lvCol.cchTextMax = 255;
		ctrlResults.GetColumn(indexes[i], &lvCol);
		ctrlFilterSel.AddString(lvCol.pszText);
	}
	ctrlFilterSel.SetCurSel(0);

	bHandled = FALSE;
	return 1;
}

LRESULT SearchFrame::onMeasure(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HWND hwnd = 0;
	if(wParam == IDC_FILETYPES) return ListMeasure(hwnd, wParam, (MEASUREITEMSTRUCT *)lParam);
		else return OMenu::onMeasureItem(hwnd, uMsg, wParam, lParam, bHandled);
};

LRESULT SearchFrame::onDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HWND hwnd = 0;
	if(wParam == IDC_FILETYPES) return ListDraw(hwnd, wParam, (DRAWITEMSTRUCT*)lParam);
		else return OMenu::onDrawItem(hwnd, uMsg, wParam, lParam, bHandled);
};


BOOL SearchFrame::ListMeasure(HWND hwnd, UINT uCtrlId, MEASUREITEMSTRUCT *mis) {
	mis->itemHeight = 16;
	return TRUE;
}


BOOL SearchFrame::ListDraw(HWND hwnd, UINT uCtrlId, DRAWITEMSTRUCT *dis) {
	TCHAR szText[MAX_PATH+1];
	int idx;
	
	switch(dis->itemAction) {
		case ODA_FOCUS:
			if(!(dis->itemState & 0x0200))
				DrawFocusRect(dis->hDC, &dis->rcItem);
			break;

		case ODA_SELECT:
		case ODA_DRAWENTIRE:
			ctrlFiletype.GetLBText(dis->itemID, szText);
			if(dis->itemState & ODS_SELECTED) {
				SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
				SetBkColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHT));
			} else {			
				SetTextColor(dis->hDC, WinUtil::textColor);
				SetBkColor(dis->hDC, WinUtil::bgColor);
			}

			idx = dis->itemData;
			ExtTextOut(dis->hDC, dis->rcItem.left+22, dis->rcItem.top+1, ETO_OPAQUE, &dis->rcItem, szText, lstrlen(szText), 0);
			if(dis->itemState & ODS_FOCUS) {
				if(!(dis->itemState &  0x0200 ))
					DrawFocusRect(dis->hDC, &dis->rcItem);
			}

			ImageList_Draw(searchTypes, dis->itemID, dis->hDC, 
				dis->rcItem.left + 2, 
				dis->rcItem.top, 
				ILD_TRANSPARENT);

			break;
	}
	return TRUE;
}


void SearchFrame::onEnter() {
	StringList clients;
	
	// Start the countdown timer...
	// Can this be done in a better way?
	TimerManager::getInstance()->addListener(this);

	int n = ctrlHubs.GetItemCount();
	for(int i = 0; i < n; i++) {
		if(ctrlHubs.GetCheckState(i)) {
			clients.push_back(Text::fromT(ctrlHubs.getItemData(i)->ipPort));
		}
	}

	if(!clients.size())
		return;

	tstring s(ctrlSearch.GetWindowTextLength() + 1, _T('\0'));
	ctrlSearch.GetWindowText(&s[0], s.size());
	s.resize(s.size()-1);

	tstring size(ctrlSize.GetWindowTextLength() + 1, _T('\0'));
	ctrlSize.GetWindowText(&size[0], size.size());
	size.resize(size.size()-1);
		
	double lsize = Util::toDouble(Text::fromT(size));
	switch(ctrlSizeMode.GetCurSel()) {
		case 1:
			lsize*=1024.0; break;
		case 2:
			lsize*=1024.0*1024.0; break;
		case 3:
			lsize*=1024.0*1024.0*1024.0; break;
	}

	int64_t llsize = (int64_t)lsize;

	{
		Lock l(cs);
		search = StringTokenizer<tstring>(s, ' ').getTokens();
	}

	//strip out terms beginning with -
	tstring fullSearch = s;
	s.clear();
	for (TStringList::const_iterator si = search.begin(); si != search.end(); ++si)
		if ((*si)[0] != _T('-') || si->size() == 1) s += *si + _T(' ');	//Shouldn't get 0-length tokens, so safely assume at least a first char.
	s = s.substr(0, max(s.size(), static_cast<tstring::size_type>(1)) - 1);

	SearchManager::SizeModes mode((SearchManager::SizeModes)ctrlMode.GetCurSel());
	if(llsize == 0)
		mode = SearchManager::SIZE_DONTCARE;

	int ftype = ctrlFiletype.GetCurSel();
	exactSize1 = (mode == SearchManager::SIZE_EXACT);
	exactSize2 = llsize;		

	if(BOOLSETTING(CLEAR_SEARCH)){
		ctrlSearch.SetWindowText(_T(""));
	}

	// Add new searches to the last-search dropdown list
	if(find(lastSearches.begin(), lastSearches.end(), s) == lastSearches.end()) 
	{
		int i = SETTING(SEARCH_HISTORY)-1;
		if(ctrlSearchBox.GetCount() > i) 
			ctrlSearchBox.DeleteString(i);
		ctrlSearchBox.InsertString(0, s.c_str());

		while(lastSearches.size() > (int64_t)i) {
			lastSearches.erase(lastSearches.begin());
		}
		lastSearches.push_back(s);
	}
		
	for(SearchInfo::Iter i = PausedResults.begin(); i != PausedResults.end(); ++i) {
		delete *i;
	}
	PausedResults.clear();
	bPaused = false;
	::EnableWindow(GetDlgItem(IDC_SEARCH_PAUSE), TRUE);
	ctrlPauseSearch.SetWindowText(CTSTRING(PAUSE_SEARCH));
			
	SearchManager::getInstance()->search(clients, Text::fromT(s), llsize, 
		(SearchManager::TypeModes)ftype, mode, (int*)this, fullSearch);
	searches++;
}

void SearchFrame::on(SearchManagerListener::SR, SearchResult* aResult) throw() {
	// Check that this is really a relevant search result...
	{
		Lock l(cs);

		if(search.empty()) {
			return;
		}

		if(isHash) {
			if(aResult->getTTH() == NULL)
				return;
			if(Util::stricmp(Text::toT(aResult->getTTH()->toBase32()), search[0]) != 0)
				return;
		} else {
			// match all here
			for(TStringIter j = search.begin(); j != search.end(); ++j) {
				if((*j->begin() != _T('-') && Util::findSubString(aResult->getFile(), Text::fromT(*j)) == -1) ||
					(*j->begin() == _T('-') && j->size() != 1 && Util::findSubString(aResult->getFile(), Text::fromT(j->substr(1))) != -1)
					) 
				{
					droppedResults++;
					PostMessage(WM_SPEAKER, FILTER_RESULT, NULL);
					return;
				}
			}
		}
	}

	// Reject results without free slots if selected
	// but always show directories
	if( (onlyFree && aResult->getFreeSlots() < 1) ||
	((onlyTTH && aResult->getTTH() == NULL) && (aResult->getType() != SearchResult::TYPE_DIRECTORY)) ||
	(exactSize1 && (aResult->getSize() != exactSize2))
	)
	{
		droppedResults++;
		PostMessage(WM_SPEAKER, FILTER_RESULT, NULL);
		return;
	}


	SearchInfo* i = new SearchInfo(aResult);
	PostMessage(WM_SPEAKER, ADD_RESULT, (LPARAM)i);	
}

void SearchFrame::on(SearchManagerListener::Searching, SearchQueueItem* aSearch) throw() {
	if((searches >= 0) && (aSearch->getWindow() == (int*)this)) {
		searches--;
		dcassert(searches >= 0);

		for(int i = 0, j = mainItems.size(); i < j; ++i) {
			SearchInfo* si = mainItems[i];
			int q = 0;
			if(si->subItems.size() > 0) {
				while(q<si->subItems.size()) {
					SearchInfo* j = si->subItems[q];
					delete j;
					q++;
				}
			}
			delete si;
		}

		mainItems.clear();
		ctrlResults.DeleteAllItems();

		{
			Lock l(cs);
			search = StringTokenizer<tstring>(aSearch->getSearch(), ' ').getTokens();
		}
		isHash = (aSearch->getTypeMode() == SearchManager::TYPE_TTH);

		// Turn off the countdown timer if no more manual searches left
		if(searches == 0)
			TimerManager::getInstance()->removeListener(this);

		// Update the status bar
		PostMessage(WM_SPEAKER, SEARCH_START, (LPARAM)new tstring(Text::toT(aSearch->getTarget())));

		droppedResults = 0;
	}
}

void SearchFrame::on(TimerManagerListener::Second, u_int32_t aTick) throw() {
	if(searches > 0) {
		int32_t waitFor = (((SearchManager::getInstance()->getLastSearch() + (SETTING(MINIMUM_SEARCH_INTERVAL)*1000)) - aTick)/1000) + SETTING(MINIMUM_SEARCH_INTERVAL) * SearchManager::getInstance()->getSearchQueueNumber((int*)this);
		TCHAR buf[64];
		_stprintf(buf, CTSTRING(WAITING_FOR), waitFor);
		PostMessage(WM_SPEAKER, QUEUE_STATS, (LPARAM)new tstring(buf));
	}
}

void SearchFrame::SearchInfo::view() {
	try {
		if(sr->getType() == SearchResult::TYPE_FILE) {
			QueueManager::getInstance()->add(sr->getFile(), sr->getSize(), sr->getUser(), 
				Util::getTempPath() + Text::fromT(fileName), sr->getTTH(), 
				(QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_TEXT | (sr->getUtf8() ? QueueItem::FLAG_SOURCE_UTF8 : 0)), QueueItem::HIGHEST);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::GetMP3Info() {
	try {
		if(sr->getType() == SearchResult::TYPE_FILE) {
			QueueManager::getInstance()->add(sr->getFile(), 2100, sr->getUser(), 
				Util::getTempPath() + Text::fromT(fileName), NULL,
				(QueueItem::FLAG_MP3_INFO| (sr->getUtf8() ? QueueItem::FLAG_SOURCE_UTF8 : 0)), QueueItem::HIGHEST);
		}
	} catch(const Exception&) {
	}
}
	
void SearchFrame::SearchInfo::Download::operator()(SearchInfo* si) {
	try {
		if(si->sr->getType() == SearchResult::TYPE_FILE) {
			
			QueueManager::getInstance()->add(si->sr->getFile(), si->sr->getSize(), si->sr->getUser(), 
				Text::fromT(tgt + si->fileName), si->sr->getTTH(), QueueItem::FLAG_RESUME | (BOOLSETTING(MULTI_CHUNK) ? QueueItem::FLAG_MULTI_SOURCE : 0) | (si->sr->getUtf8() ? QueueItem::FLAG_SOURCE_UTF8 : 0),
				(GetKeyState(VK_SHIFT) & 0x8000) > 0 ? QueueItem::HIGHEST : QueueItem::DEFAULT);
			
			if(si->subItems.size()>0) {
				int q = 0;
				while(q<si->subItems.size()) {
					SearchInfo* j = si->subItems[q];
					QueueManager::getInstance()->add(j->sr->getFile(), j->sr->getSize(), j->sr->getUser(), 
						Text::fromT(tgt + si->fileName), j->sr->getTTH(), QueueItem::FLAG_RESUME | (BOOLSETTING(MULTI_CHUNK) ? QueueItem::FLAG_MULTI_SOURCE : 0) | (j->sr->getUtf8() ? QueueItem::FLAG_SOURCE_UTF8 : 0),
						(GetKeyState(VK_SHIFT) & 0x8000) > 0 ? QueueItem::HIGHEST : QueueItem::DEFAULT);
					q++;
				}
			}
		} else {
			QueueManager::getInstance()->addDirectory(si->sr->getFile(), si->sr->getUser(), Text::fromT(tgt),
			(GetKeyState(VK_SHIFT) & 0x8000) > 0 ? QueueItem::HIGHEST : QueueItem::DEFAULT);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::DownloadWhole::operator()(SearchInfo* si) {
	try {
		if(si->sr->getType() == SearchResult::TYPE_FILE) {
			QueueManager::getInstance()->addDirectory(Text::fromT(si->path), si->sr->getUser(), Text::fromT(tgt),
			(GetKeyState(VK_SHIFT) & 0x8000) > 0 ? QueueItem::HIGHEST : QueueItem::DEFAULT);
		} else {
			QueueManager::getInstance()->addDirectory(si->sr->getFile(), si->sr->getUser(), Text::fromT(tgt),
			(GetKeyState(VK_SHIFT) & 0x8000) > 0 ? QueueItem::HIGHEST : QueueItem::DEFAULT);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::DownloadTarget::operator()(SearchInfo* si) {
	try {
		if(si->sr->getType() == SearchResult::TYPE_FILE) {
			QueueManager::getInstance()->add(si->sr->getFile(), si->sr->getSize(), si->sr->getUser(), 
				Text::fromT(tgt), si->sr->getTTH(), QueueItem::FLAG_RESUME | (BOOLSETTING(MULTI_CHUNK) ? QueueItem::FLAG_MULTI_SOURCE : 0) | (si->sr->getUtf8() ? QueueItem::FLAG_SOURCE_UTF8 : 0),
			(GetKeyState(VK_SHIFT) & 0x8000) > 0 ? QueueItem::HIGHEST : QueueItem::DEFAULT);
		} else {
			QueueManager::getInstance()->addDirectory(si->sr->getFile(), si->sr->getUser(), Text::fromT(tgt),
			(GetKeyState(VK_SHIFT) & 0x8000) > 0 ? QueueItem::HIGHEST : QueueItem::DEFAULT);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::getList() {
	try {
		WinUtil::addInitalDir(sr->getUser(), Text::fromT(getPath()));
		QueueManager::getInstance()->addList(sr->getUser(), QueueItem::FLAG_CLIENT_VIEW);
	} catch(const Exception&) {
		// Ignore for now...
	}
}

void SearchFrame::SearchInfo::CheckSize::operator()(SearchInfo* si) {
	if(!si->getTTH().empty()) {
		if(firstTTH) {
			tth = si->getTTH();
			hasTTH = true;
			firstTTH = false;
		} else if(hasTTH) {
			if(tth != si->getTTH()) {
				hasTTH = false;
			}
		} 
	} else {
		firstTTH = false;
		hasTTH = false;
	}

	if(si->sr->getType() == SearchResult::TYPE_FILE) {
		if(ext.empty()) {
			ext = Util::getFileExt(si->fileName);
			size = si->sr->getSize();
		} else if(size != -1) {
			if((si->sr->getSize() != size) || (Util::stricmp(ext, Util::getFileExt(si->fileName)) != 0)) {
				size = -1;
		}
	}
	} else {
		size = -1;
	}
	if(oneHub && hub.empty()) {
		hub = Text::toT(si->sr->getUser()->getClientAddressPort());
	} else if(hub != Text::toT(si->sr->getUser()->getClientAddressPort())) {
		oneHub = false;
		hub.clear();
	}
	if(op)
		op = si->sr->getUser()->isClientOp();
}

LRESULT SearchFrame::onDownloadTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlResults.GetSelectedCount() == 1) {
		int i = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
		dcassert(i != -1);
		SearchInfo* si = ctrlResults.getItemData(i);
		SearchResult* sr = si->sr;
	
		if(sr->getType() == SearchResult::TYPE_FILE) {
			tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY)) + si->getFileName();
			if(WinUtil::browseFile(target, m_hWnd)) {
				WinUtil::addLastDir(Util::getFilePath(target));
				ctrlResults.forEachSelectedT(SearchInfo::DownloadTarget(target));
			}
		} else {
			tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
			if(WinUtil::browseDirectory(target, m_hWnd)) {
				WinUtil::addLastDir(target);
				ctrlResults.forEachSelectedT(SearchInfo::Download(target));
			}
		}
	} else {
		tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
		if(WinUtil::browseDirectory(target, m_hWnd)) {
			WinUtil::addLastDir(target);
			ctrlResults.forEachSelectedT(SearchInfo::Download(target));
		}
	}
	return 0;
}

LRESULT SearchFrame::onDownloadWholeTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
	if(WinUtil::browseDirectory(target, m_hWnd)) {
		WinUtil::addLastDir(target);
		ctrlResults.forEachSelectedT(SearchInfo::DownloadWhole(target));
	}
	return 0;
}

LRESULT SearchFrame::onDownloadTarget(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	dcassert(wID >= IDC_DOWNLOAD_TARGET);
	size_t newId = (size_t)wID - IDC_DOWNLOAD_TARGET;

	if(newId < WinUtil::lastDirs.size()) {
		ctrlResults.forEachSelectedT(SearchInfo::Download(WinUtil::lastDirs[newId]));
	} else {
		dcassert((newId - WinUtil::lastDirs.size()) < targets.size());
		ctrlResults.forEachSelectedT(SearchInfo::DownloadTarget(Text::toT(targets[newId - WinUtil::lastDirs.size()])));
	}
	return 0;
}

LRESULT SearchFrame::onDownloadWholeTarget(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	dcassert((wID-IDC_DOWNLOAD_WHOLE_TARGET) < (int)WinUtil::lastDirs.size());
	ctrlResults.forEachSelectedT(SearchInfo::DownloadWhole(WinUtil::lastDirs[wID-IDC_DOWNLOAD_WHOLE_TARGET]));
	return 0;
}

LRESULT SearchFrame::onDownloadFavoriteDirs(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	dcassert(wID >= IDC_DOWNLOAD_FAVORITE_DIRS);
	size_t newId = (size_t)wID - IDC_DOWNLOAD_FAVORITE_DIRS;

	StringPairList spl = HubManager::getInstance()->getFavoriteDirs();
	if(newId < spl.size()) {
		ctrlResults.forEachSelectedT(SearchInfo::Download(Text::toT(spl[newId].first)));
	} else {
		dcassert((newId - spl.size()) < targets.size());
		ctrlResults.forEachSelectedT(SearchInfo::DownloadTarget(Text::toT(targets[newId - spl.size()])));
	}
	return 0;
}

LRESULT SearchFrame::onDownloadWholeFavoriteDirs(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	StringPairList spl = HubManager::getInstance()->getFavoriteDirs();
	dcassert((wID-IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS) < (int)spl.size());
	ctrlResults.forEachSelectedT(SearchInfo::DownloadWhole(Text::toT(spl[wID-IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS].first)));
	return 0;
}

LRESULT SearchFrame::onDoubleClickResults(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMITEMACTIVATE item = (LPNMITEMACTIVATE)pnmh;
	
	if (item->iItem != -1) {
		CRect rect;
		ctrlResults.GetItemRect(item->iItem, rect, LVIR_ICON);

		// if double click on state icon, ignore...
		if (item->ptAction.x < rect.left)
			return 0;

		ctrlResults.forEachSelectedT(SearchInfo::Download(Text::toT(SETTING(DOWNLOAD_DIRECTORY))));
	}
	return 0;
}

LRESULT SearchFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	if(!closed) {
		SearchManager::getInstance()->removeListener(this);
		TimerManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
 		ClientManager* clientMgr = ClientManager::getInstance();
 		clientMgr->removeListener(this);

		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {

		for(int i = 0, j = mainItems.size(); i < j; ++i) {
			SearchInfo* si =  mainItems[i];
			int q = 0;
			if(si->subItems.size() > 0) {
				while(q<si->subItems.size()) {
					SearchInfo* j = si->subItems[q];
					delete j;
					q++;
				}
			}
			delete si;
		}

 		mainItems.clear();
		ctrlResults.DeleteAllItems();
		for(int i = 0; i < ctrlHubs.GetItemCount(); i++) {
			delete ctrlHubs.getItemData(i);
		}
		ctrlHubs.DeleteAllItems();

		WinUtil::saveHeaderOrder(ctrlResults, SettingsManager::SEARCHFRAME_ORDER,
			SettingsManager::SEARCHFRAME_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);

		bHandled = FALSE;
	return 0;
	}
}

void SearchFrame::UpdateLayout(BOOL bResizeBars)
{
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
	
	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[4];
		ctrlStatus.GetClientRect(sr);
		int tmp = (sr.Width()) > 316 ? 216 : ((sr.Width() > 116) ? sr.Width()-100 : 16);
		
		w[0] = 15;
		w[1] = sr.right - tmp;
		w[2] = w[1] + (tmp-16)/2;
		w[3] = w[1] + (tmp-16);
		
		ctrlStatus.SetParts(3, w);

		// Layout showUI button in statusbar part #0
		ctrlStatus.GetRect(0, sr);
		ctrlShowUI.MoveWindow(sr);
	}

	if(showUI)
	{
		int const width = 220, spacing = 50, labelH = 16, comboH = 140, lMargin = 2, rMargin = 4;
		CRect rc = rect;

		rc.left += width;
		ctrlResults.MoveWindow(rc);

		// "Search for"
		rc.right = width - rMargin;
		rc.left = lMargin;
		rc.top += 25;
		rc.bottom = rc.top + comboH + 21;
		ctrlSearchBox.MoveWindow(rc);

		searchLabel.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);

		// "Purge"
		rc.right = rc.left + spacing;
		rc.left = lMargin;
		rc.top += 25;
		rc.bottom = rc.top + 21;
		ctrlPurge.MoveWindow(rc);

		// "Size"
		int w2 = width - rMargin - lMargin;
		rc.top += spacing;
		rc.bottom += spacing;
		rc.right = w2/3;
		ctrlMode.MoveWindow(rc);

		sizeLabel.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);

		rc.left = rc.right + lMargin;
		rc.right += w2/3;
		ctrlSize.MoveWindow(rc);

		rc.left = rc.right + lMargin;
		rc.right = width - rMargin;
		ctrlSizeMode.MoveWindow(rc);

		// "File type"
		rc.left = lMargin;
		rc.right = width - rMargin;
		rc.top += spacing;
		rc.bottom = rc.top + comboH + 21;
		ctrlFiletype.MoveWindow(rc);
		//rc.bottom -= comboH;

		typeLabel.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);

		// "Search filter"
		rc.left = lMargin;
		rc.right = (width - rMargin) / 2 - 3;		
		rc.top += spacing;
		rc.bottom = rc.top + 21;
		
		ctrlFilter.MoveWindow(rc);

		rc.left = (width - rMargin) / 2 + 3;
		rc.right = width - rMargin;
		rc.bottom = rc.top + comboH + 21;
		ctrlFilterSel.MoveWindow(rc);
		rc.bottom -= comboH;

		srLabel.MoveWindow(lMargin * 2, rc.top - labelH, width - rMargin, labelH-1);

		// "Search options"
		rc.left = lMargin+4;
		rc.right = width - rMargin;
		rc.top += spacing;
		rc.bottom += spacing;

		optionLabel.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);
		ctrlSlots.MoveWindow(rc);

		rc.top += 21;
		rc.bottom += 21;
		ctrlCollapsed.MoveWindow(rc);



		rc.left = lMargin+4;
		rc.right = width - rMargin;
		rc.top += labelH+4;
		rc.bottom += labelH+4;
		ctrlTTH.MoveWindow(rc);

		// "Hubs"
		rc.left = lMargin;
		rc.right = width - rMargin;
		rc.top += spacing;
		rc.bottom = rc.top + comboH;
		if (rc.bottom + labelH + 21 > rect.bottom) {
			rc.bottom = rect.bottom - labelH - 21;
			if (rc.bottom < rc.top + (labelH*3)/2)
				rc.bottom = rc.top + (labelH*3)/2;
		}

		ctrlHubs.MoveWindow(rc);

		hubsLabel.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);

		// "Pause Search"
		rc.right = width - rMargin;
		rc.left = rc.right - 110;
		rc.top = rc.bottom + labelH;
		rc.bottom = rc.top + 21;
		ctrlPauseSearch.MoveWindow(rc);

		// "Search"
		rc.left = lMargin;
		rc.right = rc.left + 100;
		ctrlDoSearch.MoveWindow(rc);
	}
	else
	{
		CRect rc = rect;
		ctrlResults.MoveWindow(rc);

		rc.SetRect(0,0,0,0);
		ctrlSearchBox.MoveWindow(rc);
		ctrlMode.MoveWindow(rc);
		ctrlSize.MoveWindow(rc);
		ctrlSizeMode.MoveWindow(rc);
		ctrlFiletype.MoveWindow(rc);
		ctrlPauseSearch.MoveWindow(rc);
	}

	POINT pt;
	pt.x = 10; 
	pt.y = 10;
	HWND hWnd = ctrlSearchBox.ChildWindowFromPoint(pt);
	if(hWnd != NULL && !ctrlSearch.IsWindow() && hWnd != ctrlSearchBox.m_hWnd) {
		ctrlSearch.Attach(hWnd); 
		searchContainer.SubclassWindow(ctrlSearch.m_hWnd);
	}	
}

void SearchFrame::runUserCommand(UserCommand& uc) {
	if(!WinUtil::getUCParams(m_hWnd, uc, ucParams))
		return;
	set<User::Ptr> nicks;

	int sel = -1;
	while((sel = ctrlResults.GetNextItem(sel, LVNI_SELECTED)) != -1) {
		SearchResult* sr = ctrlResults.getItemData(sel)->sr;
		if(uc.getType() == UserCommand::TYPE_RAW_ONCE) {
			if(nicks.find(sr->getUser()) != nicks.end())
			continue;
			nicks.insert(sr->getUser());
		}
		if(!sr->getUser()->isOnline())
			return;
			
		ucParams["mynick"] = sr->getUser()->getClientNick();
		ucParams["mycid"] = sr->getUser()->getClientCID().toBase32();
		ucParams["file"] = sr->getFile();
		ucParams["filesize"] = Util::toString(sr->getSize());
		ucParams["filesizeshort"] = Util::formatBytes(sr->getSize());
		if(sr->getTTH() != NULL) {
			ucParams["tth"] = sr->getTTH()->toBase32();
		}

		StringMap tmp = ucParams;
		sr->getUser()->getParams(tmp);
		sr->getUser()->clientEscapeParams(tmp);
		sr->getUser()->sendUserCmd(Util::formatParams(uc.getCommand(), tmp));
	}
	return;
};

LRESULT SearchFrame::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	HWND hWnd = (HWND)lParam;
	HDC hDC = (HDC)wParam;

	if(hWnd == searchLabel.m_hWnd || hWnd == sizeLabel.m_hWnd || hWnd == optionLabel.m_hWnd || hWnd == typeLabel.m_hWnd
		|| hWnd == hubsLabel.m_hWnd || hWnd == ctrlSlots.m_hWnd || hWnd == ctrlTTH.m_hWnd || hWnd == ctrlCollapsed.m_hWnd || hWnd == srLabel.m_hWnd) {
		::SetBkColor(hDC, ::GetSysColor(COLOR_3DFACE));
		::SetTextColor(hDC, ::GetSysColor(COLOR_BTNTEXT));
		return (LRESULT)::GetSysColorBrush(COLOR_3DFACE);
	} else {
		::SetBkColor(hDC, WinUtil::bgColor);
		::SetTextColor(hDC, WinUtil::textColor);
		return (LRESULT)WinUtil::bgBrush;
	}
};

LRESULT SearchFrame::onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	switch(wParam) {
	case VK_TAB:
		if(uMsg == WM_KEYDOWN) {
			onTab((GetKeyState(VK_SHIFT) & 0x8000) > 0);
		}
		break;
	case VK_RETURN:
		if( (GetKeyState(VK_SHIFT) & 0x8000) || 
			(GetKeyState(VK_CONTROL) & 0x8000) || 
			(GetKeyState(VK_MENU) & 0x8000) ) {
			bHandled = FALSE;
		} else {
			if(uMsg == WM_KEYDOWN) {
				onEnter();
			}
		}
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}

void SearchFrame::onTab(bool shift) {
	HWND wnds[] = {
		ctrlSearch.m_hWnd, ctrlPurge.m_hWnd, ctrlMode.m_hWnd, ctrlSize.m_hWnd, ctrlSizeMode.m_hWnd, 
		ctrlFiletype.m_hWnd, ctrlSlots.m_hWnd, ctrlTTH.m_hWnd, ctrlCollapsed.m_hWnd, ctrlDoSearch.m_hWnd, ctrlSearch.m_hWnd, 
		ctrlResults.m_hWnd
	};
	
	HWND focus = GetFocus();
	if(focus == ctrlSearchBox.m_hWnd)
		focus = ctrlSearch.m_hWnd;
	
	static const int size = sizeof(wnds) / sizeof(wnds[0]);
	int i;
	for(i = 0; i < size; i++) {
		if(wnds[i] == focus)
			break;
	}

	::SetFocus(wnds[(i + (shift ? -1 : 1)) % size]);
}

LRESULT SearchFrame::onSearchByTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlResults.GetSelectedCount() == 1) {
		int i = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
		SearchResult* sr = ctrlResults.getItemData(i)->sr;

		if(sr->getTTH() != NULL) {
			WinUtil::searchHash(sr->getTTH());
		}
	} 

	return 0;
}

LRESULT SearchFrame::onBitziLookup(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlResults.GetSelectedCount() == 1) {
		int i = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
		SearchResult* sr = ctrlResults.getItemData(i)->sr;
		WinUtil::bitziLink(sr->getTTH());
	}
	return 0;
}

LRESULT SearchFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
 	switch(wParam) {
	case ADD_RESULT:
		{
			SearchInfo* si = (SearchInfo*)lParam;
			if(bPaused == false) {
				SearchResult* sr = si->sr;
				// Check previous search results for dupes
				for(int i = 0, j = mainItems.size(); i < j; ++i) {
					SearchInfo* si2 = mainItems[i];
					SearchResult* sr2 = si2->sr;


					if(!si->getTTH().empty()) {
						if((!si2->getTTH().empty()) && (si2->getTTH() == si->getTTH()) && (sr2->getSize() == sr->getSize())){
							if(sr->getUser()->getNick() == sr2->getUser()->getNick()){
								delete si;
								return 0;
							}

							for(SearchInfo::Iter k = si2->subItems.begin(); k != si2->subItems.end(); k++){
								if(sr->getUser()->getNick() == (*k)->getUser()->getNick()){
									delete si;
									return 0;
								}
							}

							si2->subItems.push_back(si);
							si->main = si2;
							si->mainitem = false;

							int pos = ctrlResults.findItem(si2);

							if(pos != -1) {
								if(si2->subItems.size() == 1){
									if(expandSR){
										ctrlResults.SetItemState(pos, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
										si2->collapsed = false;
										insertSubItem(si, pos+1);
									} else {
										ctrlResults.SetItemState(pos, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
									}
								}else if(!si2->collapsed){
									insertSubItem(si, pos + 1);
								}
							}
							si2->setHits(Text::toT(Util::toString((u_int32_t)si2->subItems.size() + 1) + " "  + CSTRING(USERS)));
							if(pos != -1)
								ctrlResults.updateItem(pos);
							return 0;
                	    }

						continue;
					} else if((sr->getUser()->getNick() == sr2->getUser()->getNick()) && (sr->getFile() == sr2->getFile())) {
						delete si;
						return 0;
					}
				}

				mainItems.push_back(si);
				si->mainitem = true;
				insertItem(0, si);
				setDirty();
				ctrlStatus.SetText(2, Text::toT(Util::toString(ctrlResults.GetItemCount()) + " " + STRING(FILES)).c_str());
			} else {
				PausedResults.push_back(si);
				ctrlStatus.SetText(2, Text::toT(Util::toString(ctrlResults.GetItemCount()) + "/" + Util::toString(ctrlResults.GetItemCount() + PausedResults.size()) + " " + STRING(FILES)).c_str());
			}
		}
		break;
	case FILTER_RESULT:
		ctrlStatus.SetText(3, Text::toT(Util::toString(droppedResults) + ' ' + STRING(FILTERED)).c_str());
		break;
 	case HUB_ADDED:
 			onHubAdded((HubInfo*)(lParam));
		break;
 	case HUB_CHANGED:
 			onHubChanged((HubInfo*)(lParam));
		break;
 	case HUB_REMOVED:
 			onHubRemoved((HubInfo*)(lParam));
 		break;
	case QUEUE_STATS:
		{
			tstring* t = (tstring*)(lParam);
			ctrlStatus.SetText(1, (*t).c_str());
			ctrlStatus.SetText(2, _T(""));
			ctrlStatus.SetText(3, _T(""));
			SetWindowText((*t).c_str());
			delete t;
		}
		break;
	case SEARCH_START:
		{
			tstring* t = (tstring*)(lParam);
			ctrlStatus.SetText(1, (TSTRING(SEARCHING_FOR) + (*t) + _T("...")).c_str());
			ctrlStatus.SetText(2, _T(""));
			ctrlStatus.SetText(3, _T(""));
	
			SetWindowText((TSTRING(SEARCH) + _T(" - ") + (*t)).c_str());
		}
		break;
	}

	return 0;
}

LRESULT SearchFrame::onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	RECT rc;                    // client area of window 
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 
	
	// Get the bounding rectangle of the client area. 
	ctrlResults.GetClientRect(&rc);
	ctrlResults.ScreenToClient(&pt); 

	if (PtInRect(&rc, pt) && ctrlResults.GetSelectedCount() > 0) {
		ctrlResults.ClientToScreen(&pt);

		while(targetMenu.GetMenuItemCount() > 0) {
			targetMenu.DeleteMenu(0, MF_BYPOSITION);
		}
		while(targetDirMenu.GetMenuItemCount() > 0) {
			targetDirMenu.DeleteMenu(0, MF_BYPOSITION);
		}

		int n = 0;

		targetMenu.InsertSeparatorFirst(STRING(DOWNLOAD_TO));
		//Append favorite download dirs
		StringPairList spl = HubManager::getInstance()->getFavoriteDirs();
		if (spl.size() > 0) {
			for(StringPairIter i = spl.begin(); i != spl.end(); i++) {
				targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_FAVORITE_DIRS + n, Text::toT(i->second).c_str());
				n++;
			}
			targetMenu.AppendMenu(MF_SEPARATOR);
		}

		n = 0;
		targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOADTO, CTSTRING(BROWSE));
		if(WinUtil::lastDirs.size() > 0) {
			targetMenu.InsertSeparatorLast(STRING(PREVIOUS_FOLDERS));
			for(TStringIter i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i) {
				targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_TARGET + n, i->c_str());
				n++;
			}
		}

		SearchInfo::CheckSize cs = ctrlResults.forEachSelectedT(SearchInfo::CheckSize());

		if(cs.size != -1 || cs.hasTTH) {
			targets.clear();
			if(cs.hasTTH) {
				QueueManager::getInstance()->getTargetsByRoot(targets, TTHValue(Text::fromT(cs.tth)));
			} else if(BOOLSETTING(USE_EXTENSION_DOWNTO)) { 
				QueueManager::getInstance()->getTargetsBySize(targets, cs.size, Text::fromT(cs.ext));
				} else {
					QueueManager::getInstance()->getTargetsBySize(targets, cs.size, Util::emptyString);
				}
			if(targets.size() > 0) {
				targetMenu.InsertSeparatorLast(STRING(ADD_AS_SOURCE));
					for(StringIter i = targets.begin(); i != targets.end(); ++i) {
					targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_TARGET + n, Text::toT(*i).c_str());
						n++;
					}
				}
			}

		n = 0;
		targetDirMenu.InsertSeparatorFirst(STRING(DOWNLOAD_WHOLE_DIR_TO));
		//Append favorite download dirs
		if (spl.size() > 0) {
			for(StringPairIter i = spl.begin(); i != spl.end(); ++i) {
				targetDirMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS + n, Text::toT(i->second).c_str());
				n++;
			}
			targetDirMenu.AppendMenu(MF_SEPARATOR);
		}

		n = 0;
		targetDirMenu.AppendMenu(MF_STRING, IDC_DOWNLOADDIRTO, CTSTRING(BROWSE));
			if(WinUtil::lastDirs.size() > 0) {
			targetDirMenu.AppendMenu(MF_SEPARATOR);
			for(TStringIter i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i) {
				targetDirMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_WHOLE_TARGET + n, i->c_str());
						n++;
					}
				}		

		int i = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
		SearchResult* sr = ctrlResults.getItemData(i)->sr;
		if(sr) {
			if (ctrlResults.GetSelectedCount() == 1 && sr->getTTH() != NULL) {
				resultsMenu.EnableMenuItem(IDC_SEARCH_ALTERNATES, MF_ENABLED);
				resultsMenu.EnableMenuItem(IDC_BITZI_LOOKUP, MF_ENABLED);
				resultsMenu.EnableMenuItem(IDC_COPY_TTH, MF_BYCOMMAND | MF_ENABLED);
				resultsMenu.EnableMenuItem(IDC_COPY_LINK, MF_BYCOMMAND | MF_ENABLED);
			} else {
				resultsMenu.EnableMenuItem(IDC_SEARCH_ALTERNATES, MF_GRAYED);
				resultsMenu.EnableMenuItem(IDC_BITZI_LOOKUP, MF_GRAYED);
				resultsMenu.EnableMenuItem(IDC_COPY_TTH, MF_BYCOMMAND | MF_GRAYED);
				resultsMenu.EnableMenuItem(IDC_COPY_LINK, MF_BYCOMMAND | MF_GRAYED);
			}
	
			if(ctrlResults.GetSelectedCount() == 1 && ((Util::getFileExt(sr->getFileName()) == ".mp3") || (Util::getFileExt(sr->getFileName()) == ".MP3"))) {
				resultsMenu.EnableMenuItem(IDC_MP3, MF_BYCOMMAND | MF_ENABLED);
			} else {
				resultsMenu.EnableMenuItem(IDC_MP3, MF_BYCOMMAND | MF_GRAYED);
			}
				
		}

		prepareMenu(resultsMenu, UserCommand::CONTEXT_SEARCH, cs.hub, cs.op);
		if(!(resultsMenu.GetMenuState(resultsMenu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {	
			resultsMenu.AppendMenu(MF_SEPARATOR);
		}
		resultsMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
		resultsMenu.InsertSeparatorFirst(sr->getFileName());
		resultsMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		resultsMenu.RemoveFirstItem();
		resultsMenu.DeleteMenu(resultsMenu.GetMenuItemCount()-1, MF_BYPOSITION);
		resultsMenu.DeleteMenu(resultsMenu.GetMenuItemCount()-1, MF_BYPOSITION);
		cleanMenu(resultsMenu);
		return TRUE; 
	}
	return FALSE; 
}

void SearchFrame::initHubs() {
	ctrlHubs.insertItem(new HubInfo(Util::emptyStringT, TSTRING(ONLY_WHERE_OP), false), 0);
	ctrlHubs.SetCheckState(0, false);

	ClientManager* clientMgr = ClientManager::getInstance();
	clientMgr->lock();
	clientMgr->addListener(this);

	Client::List& clients = clientMgr->getClients();

	Client::List::iterator it;
	Client::List::iterator endIt = clients.end();
	for(it = clients.begin(); it != endIt; ++it) {
		Client* client = *it;
		if (!client->isConnected())
			continue;

		onHubAdded(new HubInfo(Text::toT(client->getIpPort()), Text::toT(client->getName()), client->getOp()));
	}

	clientMgr->unlock();
	ctrlHubs.SetColumnWidth(0, LVSCW_AUTOSIZE);

}

void SearchFrame::onHubAdded(HubInfo* info) {
	int nItem = ctrlHubs.insertItem(info, 0);
	ctrlHubs.SetCheckState(nItem, (ctrlHubs.GetCheckState(0) ? info->op : true));
	ctrlHubs.SetColumnWidth(0, LVSCW_AUTOSIZE);
}

void SearchFrame::onHubChanged(HubInfo* info) {
	int nItem = 0;
	int n = ctrlHubs.GetItemCount();
	for(; nItem < n; nItem++) {
		if(ctrlHubs.getItemData(nItem)->ipPort == info->ipPort)
			break;
	}
	if (nItem == n)
		return;

	delete ctrlHubs.getItemData(nItem);
	ctrlHubs.SetItemData(nItem, (DWORD_PTR)info);
	ctrlHubs.updateItem(nItem);

	if (ctrlHubs.GetCheckState(0))
		ctrlHubs.SetCheckState(nItem, info->op);

	ctrlHubs.SetColumnWidth(0, LVSCW_AUTOSIZE);
}

void SearchFrame::onHubRemoved(HubInfo* info) {
	int nItem = 0;
	int n = ctrlHubs.GetItemCount();
	for(; nItem < n; nItem++) {
		if(ctrlHubs.getItemData(nItem)->ipPort == info->ipPort)
			break;
	}
	if (nItem == n)
		return;

	delete ctrlHubs.getItemData(nItem);
	ctrlHubs.DeleteItem(nItem);
	ctrlHubs.SetColumnWidth(0, LVSCW_AUTOSIZE);
}

LRESULT SearchFrame::onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlResults.forEachSelected(&SearchInfo::getList);
	return 0;
}

LRESULT SearchFrame::onItemChangedHub(int /* idCtrl */, LPNMHDR pnmh, BOOL& /* bHandled */) {
	NMLISTVIEW* lv = (NMLISTVIEW*)pnmh;
	if(lv->iItem == 0 && (lv->uNewState ^ lv->uOldState) & LVIS_STATEIMAGEMASK) {
		if (((lv->uNewState & LVIS_STATEIMAGEMASK) >> 12) - 1) {
			for(int iItem = 0; (iItem = ctrlHubs.GetNextItem(iItem, LVNI_ALL)) != -1; ) {
				HubInfo* client = ctrlHubs.getItemData(iItem);
				if (!client->op)
					ctrlHubs.SetCheckState(iItem, false);
			}
		}
	}

	return 0;
}

LRESULT SearchFrame::onPurge(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) 
{
	while(ctrlSearchBox.GetCount() > 0){
			ctrlSearchBox.DeleteString(0);
	}
	return 0;
}

LRESULT SearchFrame::onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int pos = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
	dcassert(pos != -1);
	string sCopy;
	if ( pos >= 0 ) {
		SearchResult* sr = ctrlResults.getItemData(pos)->sr;
		switch (wID) {
			case IDC_COPY_NICK:
				sCopy = sr->getUser()->getNick();
				break;
			case IDC_COPY_FILENAME:
				sCopy = Util::getFileName(sr->getFile());
				break;
			case IDC_COPY_PATH:
				sCopy = Util::getFilePath(sr->getFile());
				break;
			case IDC_COPY_SIZE:
				sCopy = Util::formatBytes(sr->getSize());
				break;
			case IDC_COPY_LINK:
				if(sr->getType() == SearchResult::TYPE_FILE) {
					WinUtil::copyMagnet(sr->getTTH(), Text::toT(sr->getFileName()), sr->getSize());
				}
				break;
			case IDC_COPY_TTH:
				if(sr->getType() == SearchResult::TYPE_FILE && sr->getTTH())
					sCopy = sr->getTTH()->toBase32();
				break;
			default:
				dcdebug("SEARCHFRAME DON'T GO HERE\n");
				return 0;
		}
		if (!sCopy.empty())
			WinUtil::setClipboard(Text::toT(sCopy));
	}
	return S_OK;
}

LRESULT SearchFrame::onLButton(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	bHandled = false;
	LPNMITEMACTIVATE item = (LPNMITEMACTIVATE)pnmh;
	
	if (item->iItem != -1) {
		CRect rect;
		ctrlResults.GetItemRect(item->iItem, rect, LVIR_ICON);

		if (item->ptAction.x < rect.left)
		{
			SearchInfo* i = (SearchInfo*)ctrlResults.getItemData(item->iItem);
			if(i->subItems.size() > 0)
				if(i->collapsed) Expand(i,item->iItem); else Collapse(i,item->iItem);
		}
	}
	return 0;
} 

void SearchFrame::Collapse(SearchInfo* i, int a) {
	size_t q = 0;
	while(q<i->subItems.size()) {
		SearchInfo* j = i->subItems[q];
		ctrlResults.deleteItem(j);
		q++;
	}

	i->collapsed = true;
	ctrlResults.SetItemState(a, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
}

void SearchFrame::Expand(SearchInfo* i, int a) {
	size_t q = 0;
	while(q<i->subItems.size()) {

		insertSubItem(i->subItems[q], a + 1);

		q++;
	}

	i->collapsed = false;
	ctrlResults.SetItemState(a, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
	//ctrlResults.resort();
}

void SearchFrame::insertSubItem(SearchInfo* j, int idx)
{
	LV_ITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE | LVIF_INDENT;
	lvi.iItem = idx;
	lvi.iSubItem = 0;
	lvi.iIndent = 1;
	lvi.pszText = LPSTR_TEXTCALLBACK;
	lvi.iImage = WinUtil::getIconIndex(Text::toT(j->sr->getFile()));
	lvi.lParam = (LPARAM)j;
	lvi.state = 0;
	lvi.stateMask = 0;
	ctrlResults.InsertItem(&lvi);
}

LRESULT SearchFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	CRect rc;
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;
	SearchInfo* si = (SearchInfo*)cd->nmcd.lItemlParam;

	if(si) {
		switch(cd->nmcd.dwDrawStage) {
		case CDDS_PREPAINT:
			return CDRF_NOTIFYITEMDRAW;

		case CDDS_ITEMPREPAINT: {
			cd->clrText = WinUtil::textColor;	
			if(si->sr != NULL) {
				targets.clear();
				if(si->sr->getTTH()) {
					QueueManager::getInstance()->getTargetsByRoot(targets, TTHValue(si->sr->getTTH()->toBase32()));
					if(si->sr->getType() == SearchResult::TYPE_FILE && targets.size() > 0) {		
						cd->clrText = SETTING(SEARCH_ALTERNATE_COLOUR);	
					}
				}
			}
			return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
		}
		case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
			if (cd->iSubItem == COLUMN_IP) {
				if(si->sr->getIP() != "" && BOOLSETTING(GET_USER_COUNTRY)) {
					if(ctrlResults.GetItemState((int)cd->nmcd.dwItemSpec, LVIS_SELECTED) & LVIS_SELECTED) {
						if(ctrlResults.m_hWnd == ::GetFocus()) {
							barva = GetSysColor(COLOR_HIGHLIGHT);
							SetBkColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHT));
							SetTextColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
						} else {
							barva = GetBkColor(cd->nmcd.hdc);
							SetBkColor(cd->nmcd.hdc, barva);
						}				
					} else {
						barva = WinUtil::bgColor;
						SetBkColor(cd->nmcd.hdc, WinUtil::bgColor);
						SetTextColor(cd->nmcd.hdc, cd->clrText);
					}

					ctrlResults.GetSubItemRect((int)cd->nmcd.dwItemSpec, COLUMN_IP, LVIR_BOUNDS, rc);
					CRect rc2 = rc;
					rc2.left += 2;
	
					HGDIOBJ oldpen = ::SelectObject(cd->nmcd.hdc, CreatePen(PS_SOLID,0, barva));
					HGDIOBJ oldbr = ::SelectObject(cd->nmcd.hdc, CreateSolidBrush(barva));
					Rectangle(cd->nmcd.hdc,rc.left, rc.top, rc.right, rc.bottom);
	
					DeleteObject(::SelectObject(cd->nmcd.hdc, oldpen));
					DeleteObject(::SelectObject(cd->nmcd.hdc, oldbr));

					TCHAR buf[256];
					ctrlResults.GetItemText((int)cd->nmcd.dwItemSpec, COLUMN_IP, buf, 255);
					buf[255] = 0;
					if(_tcslen(buf) > 0) {
						LONG top = rc2.top + (rc2.Height() - 15)/2;
						if((top - rc2.top) < 2)
							top = rc2.top + 1;

						POINT p = { rc2.left, top };
						WinUtil::flagImages.Draw(cd->nmcd.hdc, si->getflagImage(), p, LVSIL_SMALL);
						top = rc2.top + (rc2.Height() - WinUtil::getTextHeight(cd->nmcd.hdc) - 1)/2;

						::ExtTextOut(cd->nmcd.hdc, rc2.left + 30, top + 1, ETO_CLIPPED, rc2, buf, _tcslen(buf), NULL);
						return CDRF_SKIPDEFAULT;
					}
				}
			}		
		}
		default:
			return CDRF_DODEFAULT;
		}
	}
	return CDRF_DODEFAULT;
}

void SearchFrame::insertItem(int pos, SearchInfo* item) {
	PME reg(filter,"i");
	bool match = true;
	int sel = ctrlFilterSel.GetCurSel();

	if(!reg.IsValid() || filter.empty()) {
		match = true;
	} else {
		match = reg.match(Text::fromT(item->getText(columnIndexes[sel])));
	}

	if(match) {
		int image = 0;
		if (BOOLSETTING(USE_SYSTEM_ICONS)) {
			image = item->sr->getType() == SearchResult::TYPE_FILE ? WinUtil::getIconIndex(Text::toT(item->sr->getFile())) : WinUtil::getDirIconIndex();
		} else {
			const string& tmp = item->sr->getUser()->getConnection();
			if( (tmp == "28.8Kbps") ||
				(tmp == "33.6Kbps") ||
				(tmp == "56Kbps") ||
				(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_MODEM]) ||
				(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_SATELLITE]) ||
				(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_WIRELESS]) ||
				(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_ISDN]) ) {
				image = 1;
			} else if( (tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_CABLE]) ||
				(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_DSL]) ) {
				image = 2;
			} else if( (tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_T1]) ||
				(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_T3]) ) {
				image = 3;
			}
			if(item->sr->getType() == SearchResult::TYPE_FILE)
				image+=4;
		}

		int k = -1;
		if(pos == 0) {
			k = ctrlResults.insertItem(item, image);
		} else {
			k = ctrlResults.insertItem(pos, item, image);
		}

		if(item->subItems.size() > 0) {
			if(item->collapsed) {
				ctrlResults.SetItemState(k, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);	
			} else {
				ctrlResults.SetItemState(k, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);	
			}
		} else {
			ctrlResults.SetItemState(k, INDEXTOSTATEIMAGEMASK(0), LVIS_STATEIMAGEMASK);	
		}
	}
	ctrlResults.resort();
}

LRESULT SearchFrame::onFilterChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	if(wParam == VK_RETURN) {
		TCHAR *buf = new TCHAR[ctrlFilter.GetWindowTextLength()+1];
		ctrlFilter.GetWindowText(buf, ctrlFilter.GetWindowTextLength()+1);
		filter = Text::fromT(buf);
		free(buf);
	
		updateSearchList();
		ctrlStatus.SetText(2, Text::toT(Util::toString(ctrlResults.GetItemCount()) + ' ' + STRING(ITEMS)).c_str());
	}

	bHandled = false;

	return 0;
}

void SearchFrame::updateSearchList() {
	Lock l(cs);

	for(int i = 0, j = mainItems.size(); i < j; ++i) {
	    SearchInfo* si = mainItems[i];
		si->collapsed = true;
	}

	while(ctrlResults.GetItemCount() > 0) {
		ctrlResults.DeleteItem(0);
	}

	for(int i = 0, j = mainItems.size(); i < j; ++i) {
	    SearchInfo* si = mainItems[i];
		si->collapsed = true;
		insertItem(0, si);
	}


}

LRESULT SearchFrame::onSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	TCHAR *buf = new TCHAR[ctrlFilter.GetWindowTextLength()+1];
	ctrlFilter.GetWindowText(buf, ctrlFilter.GetWindowTextLength()+1);
	filter = Text::fromT(buf);
	free(buf);
	
	updateSearchList();
	
	bHandled = false;

	return 0;
}

void SearchFrame::on(SettingsManagerListener::Save, SimpleXML* /*xml*/) throw() {
	bool refresh = false;
	if(ctrlResults.GetBkColor() != WinUtil::bgColor) {
		ctrlResults.SetBkColor(WinUtil::bgColor);
		ctrlResults.SetTextBkColor(WinUtil::bgColor);
		ctrlResults.setFlickerFree(WinUtil::bgBrush);
		ctrlHubs.SetBkColor(WinUtil::bgColor);
		ctrlHubs.SetTextBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlResults.GetTextColor() != WinUtil::textColor) {
		ctrlResults.SetTextColor(WinUtil::textColor);
		ctrlHubs.SetTextColor(WinUtil::textColor);
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
