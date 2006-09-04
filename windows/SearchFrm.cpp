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

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "SearchFrm.h"
#include "LineDlg.h"

#include "../client/QueueManager.h"
#include "../client/StringTokenizer.h"
#include "../client/ClientManager.h"
#include "../client/TimerManager.h"
#include "../client/SearchManager.h"

#include "../client/pme.h"

TStringList SearchFrame::lastSearches;

int SearchFrame::columnIndexes[] = { COLUMN_FILENAME, COLUMN_HITS, COLUMN_NICK, COLUMN_TYPE, COLUMN_SIZE,
	COLUMN_PATH, COLUMN_SLOTS, COLUMN_CONNECTION, COLUMN_HUB, COLUMN_EXACT_SIZE, COLUMN_UPLOAD, COLUMN_IP, COLUMN_TTH };
int SearchFrame::columnSizes[] = { 210, 80, 100, 50, 80, 100, 40, 70, 150, 80, 80, 100, 150 };

static ResourceManager::Strings columnNames[] = {ResourceManager::FILE,  ResourceManager::HIT_COUNT, ResourceManager::USER, ResourceManager::TYPE, ResourceManager::SIZE,
	ResourceManager::PATH, ResourceManager::SLOTS, ResourceManager::CONNECTION, 
	ResourceManager::HUB, ResourceManager::EXACT_SIZE, ResourceManager::AVERAGE_UPLOAD, ResourceManager::IP_BARE, ResourceManager::TTH_ROOT };

SearchFrame::FrameMap SearchFrame::frames;

void SearchFrame::openWindow(const tstring& str /* = Util::emptyString */, LONGLONG size /* = 0 */, SearchManager::SizeModes mode /* = SearchManager::SIZE_ATLEAST */, SearchManager::TypeModes type /* = SearchManager::TYPE_ANY */) {
	SearchFrame* pChild = new SearchFrame();
	pChild->setInitial(str, size, mode, type);
	pChild->CreateEx(WinUtil::mdiClient);

	frames.insert( FramePair(pChild->m_hWnd, pChild) );
}

void SearchFrame::closeAll() {
	for(FrameIter i = frames.begin(); i != frames.end(); ++i)
		::PostMessage(i->first, WM_CLOSE, 0, 0);
}

LRESULT SearchFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

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
	ctrlResults.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | 0x00010000 | LVS_EX_INFOTIP);
	resultsContainer.SubclassWindow(ctrlResults.m_hWnd);
	
	if (BOOLSETTING(USE_SYSTEM_ICONS)) {
		ctrlResults.SetImageList(WinUtil::fileImages, LVSIL_SMALL);
	} else {
		images.CreateFromImage(IDB_SPEEDS, 16, 3, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
		ctrlResults.SetImageList(images, LVSIL_SMALL);
	}

	ctrlHubs.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_NOCOLUMNHEADER, WS_EX_CLIENTEDGE, IDC_HUB);
	ctrlHubs.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
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
	ctrlTTH.SetCheck(onlyTTH);
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

	ctrlResults.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlResults.setAscending(false);
	ctrlResults.setVisible(SETTING(SEARCHFRAME_VISIBLE));
	ctrlResults.setSortColumn(COLUMN_HITS);

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

	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CTSTRING(GRANT_EXTRA_SLOT));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_HOUR, CTSTRING(GRANT_EXTRA_SLOT_HOUR));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_DAY, CTSTRING(GRANT_EXTRA_SLOT_DAY));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_WEEK, CTSTRING(GRANT_EXTRA_SLOT_WEEK));
	grantMenu.AppendMenu(MF_SEPARATOR);
	grantMenu.AppendMenu(MF_STRING, IDC_UNGRANTSLOT, CTSTRING(REMOVE_EXTRA_SLOT));
	
	resultsMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD, CTSTRING(DOWNLOAD));
	resultsMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)targetMenu, CTSTRING(DOWNLOAD_TO));
	resultsMenu.AppendMenu(MF_STRING, IDC_DOWNLOADDIR, CTSTRING(DOWNLOAD_WHOLE_DIR));
	resultsMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)targetDirMenu, CTSTRING(DOWNLOAD_WHOLE_DIR_TO));
	resultsMenu.AppendMenu(MF_STRING, IDC_VIEW_AS_TEXT, CTSTRING(VIEW_AS_TEXT));
	resultsMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES));
	resultsMenu.AppendMenu(MF_SEPARATOR);
	appendUserItems(resultsMenu);
	resultsMenu.DeleteMenu(resultsMenu.GetMenuItemCount()-4, MF_BYPOSITION);
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
		ctrlSize.SetWindowText(Util::toStringW(initialSize).c_str());
		ctrlFiletype.SetCurSel(initialType);
		onEnter();
	} else {
		SetWindowText(CTSTRING(SEARCH));
		::EnableWindow(GetDlgItem(IDC_SEARCH_PAUSE), FALSE);
	}

	for(int j=0; j<COLUMN_LAST; j++) {
		ctrlFilterSel.AddString(CTSTRING_I(columnNames[j]));
	}
	ctrlFilterSel.SetCurSel(0);

	SettingsManager::getInstance()->addListener(this);

	bHandled = FALSE;
	return 1;
}

LRESULT SearchFrame::onMeasure(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HWND hwnd = 0;
	if(wParam == IDC_FILETYPES) return ListMeasure(hwnd, wParam, (MEASUREITEMSTRUCT *)lParam);
		else return OMenu::onMeasureItem(hwnd, uMsg, wParam, lParam, bHandled);
}

LRESULT SearchFrame::onDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HWND hwnd = 0;
	if(wParam == IDC_FILETYPES) return ListDraw(hwnd, wParam, (DRAWITEMSTRUCT*)lParam);
		else return OMenu::onDrawItem(hwnd, uMsg, wParam, lParam, bHandled);
}


BOOL SearchFrame::ListMeasure(HWND /*hwnd*/, UINT /*uCtrlId*/, MEASUREITEMSTRUCT *mis) {
	mis->itemHeight = 16;
	return TRUE;
}


BOOL SearchFrame::ListDraw(HWND /*hwnd*/, UINT /*uCtrlId*/, DRAWITEMSTRUCT *dis) {
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
	
	// Change Default Settings If Changed
	if (onlyTTH != BOOLSETTING(SEARCH_ONLY_TTH))
		SettingsManager::getInstance()->set(SettingsManager::SEARCH_ONLY_TTH, onlyTTH);
	if (onlyFree != BOOLSETTING(FREE_SLOTS_DEFAULT))
		SettingsManager::getInstance()->set(SettingsManager::FREE_SLOTS_DEFAULT, onlyFree);

	int n = ctrlHubs.GetItemCount();
	for(int i = 0; i < n; i++) {
		if(ctrlHubs.GetCheckState(i)) {
			clients.push_back(Text::fromT(ctrlHubs.getItemData(i)->url));
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
		int i = max(SETTING(SEARCH_HISTORY)-1, 0);

		if(ctrlSearchBox.GetCount() > i) 
			ctrlSearchBox.DeleteString(i);
		ctrlSearchBox.InsertString(0, s.c_str());

		while(lastSearches.size() > (TStringList::size_type)i) {
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
			
	// Start the countdown timer...
	// Can this be done in a better way?
	TimerManager::getInstance()->addListener(this);

	SearchManager::getInstance()->search(clients, Text::fromT(s), llsize, 
		(SearchManager::TypeModes)ftype, mode, "manual", (int*)this, fullSearch);
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
				if((*j->begin() != _T('-') && Util::findSubString(aResult->getUtf8() ? aResult->getFile() : Text::acpToUtf8(aResult->getFile()), Text::fromT(*j)) == -1) ||
					(*j->begin() == _T('-') && j->size() != 1 && Util::findSubString(aResult->getUtf8() ? aResult->getFile() : Text::acpToUtf8(aResult->getFile()), Text::fromT(j->substr(1))) != -1)
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
		PostMessage(WM_SPEAKER, SEARCH_START, (LPARAM)new SearchQueueItem(*aSearch));
	}
}

void SearchFrame::on(TimerManagerListener::Second, u_int32_t aTick) throw() {
	if(searches > 0) {
		u_int32_t waitFor = (((SearchManager::getInstance()->getLastSearch() + (SETTING(MINIMUM_SEARCH_INTERVAL)*1000)) - aTick)/1000) + SETTING(MINIMUM_SEARCH_INTERVAL) * SearchManager::getInstance()->getSearchQueueNumber((int*)this);
		TCHAR buf[64];
		_stprintf(buf, CTSTRING(WAITING_FOR), waitFor);
		PostMessage(WM_SPEAKER, QUEUE_STATS, (LPARAM)new tstring(buf));
	}
}

void SearchFrame::SearchInfo::view() {
	try {
		if(sr->getType() == SearchResult::TYPE_FILE) {
			QueueManager::getInstance()->add(Util::getTempPath() + Text::fromT(columns[COLUMN_FILENAME]), 
				sr->getSize(), sr->getTTH(), sr->getUser(), sr->getFile(), 
				sr->getUtf8(), QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_TEXT);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::Download::operator()(SearchInfo* si) {
	try {
		if(si->sr->getType() == SearchResult::TYPE_FILE) {
			string target = Text::fromT(tgt + si->columns[COLUMN_FILENAME]);
			QueueManager::getInstance()->add(target, si->sr->getSize(), 
				si->sr->getTTH(), si->sr->getUser(), si->sr->getFile(), 
				si->sr->getUtf8(), (BOOLSETTING(MULTI_CHUNK) ? QueueItem::FLAG_MULTI_SOURCE : 0) | QueueItem::FLAG_RESUME);
			
			for(SearchInfo::Iter i = si->subItems.begin(); i != si->subItems.end(); i++) {
				SearchInfo* j = *i;
				try {
					QueueManager::getInstance()->add(Text::fromT(tgt + si->columns[COLUMN_FILENAME]), j->sr->getSize(), j->sr->getTTH(), j->sr->getUser(), 
						j->sr->getFile(), j->sr->getUtf8(), (BOOLSETTING(MULTI_CHUNK) ? QueueItem::FLAG_MULTI_SOURCE : 0) | QueueItem::FLAG_RESUME);
				} catch(const Exception&) {
				}
			}
			if(WinUtil::isShift())
				QueueManager::getInstance()->setPriority(target, QueueItem::HIGHEST);
		} else {
			QueueManager::getInstance()->addDirectory(si->sr->getFile(), si->sr->getUser(), Text::fromT(tgt),
			WinUtil::isShift() ? QueueItem::HIGHEST : QueueItem::DEFAULT);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::DownloadWhole::operator()(SearchInfo* si) {
	try {
		if(si->sr->getType() == SearchResult::TYPE_FILE) {
			QueueManager::getInstance()->addDirectory(Text::fromT(si->columns[COLUMN_PATH]), si->sr->getUser(), Text::fromT(tgt),
			WinUtil::isShift() ? QueueItem::HIGHEST : QueueItem::DEFAULT);
		} else {
			QueueManager::getInstance()->addDirectory(si->sr->getFile(), si->sr->getUser(), Text::fromT(tgt),
			WinUtil::isShift() ? QueueItem::HIGHEST : QueueItem::DEFAULT);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::DownloadTarget::operator()(SearchInfo* si) {
	try {
		if(si->sr->getType() == SearchResult::TYPE_FILE) {
			string target = Text::fromT(tgt);
			QueueManager::getInstance()->add(target, si->sr->getSize(), 
				si->sr->getTTH(), si->sr->getUser(), si->sr->getFile(), 
				si->sr->getUtf8(), (BOOLSETTING(MULTI_CHUNK) ? QueueItem::FLAG_MULTI_SOURCE : 0) | QueueItem::FLAG_RESUME);

			if(WinUtil::isShift())
				QueueManager::getInstance()->setPriority(target, QueueItem::HIGHEST);
		} else {
			QueueManager::getInstance()->addDirectory(si->sr->getFile(), si->sr->getUser(), Text::fromT(tgt),
			WinUtil::isShift() ? QueueItem::HIGHEST : QueueItem::DEFAULT);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::getList() {
	try {
		WinUtil::addInitalDir(sr->getUser(), Text::fromT(columns[COLUMN_PATH]));
		QueueManager::getInstance()->addList(sr->getUser(), QueueItem::FLAG_CLIENT_VIEW);
	} catch(const Exception&) {
		// Ignore for now...
	}
}

void SearchFrame::SearchInfo::browseList() {
	try {
		QueueManager::getInstance()->addPfs(sr->getUser(), Text::fromT(columns[COLUMN_PATH]));
	} catch(const Exception&) {
		// Ignore for now...
	}
}

void SearchFrame::SearchInfo::CheckSize::operator()(SearchInfo* si) {
	if(!si->columns[COLUMN_TTH].empty()) {
		if(firstTTH) {
			tth = si->columns[COLUMN_TTH];
			hasTTH = true;
			firstTTH = false;
		} else if(hasTTH) {
			if(tth != si->columns[COLUMN_TTH]) {
				hasTTH = false;
			}
		} 
	} else {
		firstTTH = false;
		hasTTH = false;
	}

	if(si->sr->getType() == SearchResult::TYPE_FILE) {
		if(ext.empty()) {
			ext = Util::getFileExt(si->columns[COLUMN_FILENAME]);
			size = si->sr->getSize();
		} else if(size != -1) {
			if((si->sr->getSize() != size) || (Util::stricmp(ext, Util::getFileExt(si->columns[COLUMN_FILENAME])) != 0)) {
				size = -1;
		}
	}
	} else {
		size = -1;
	}
 
	if(firstHubs && hubs.empty()) {
		hubs = ClientManager::getInstance()->getHubs(si->sr->getUser()->getCID());
		firstHubs = false;
	} else if(!hubs.empty()) {
		// we will merge hubs of all users to ensure we can use OP commands in all hubs
		StringList sl = ClientManager::getInstance()->getHubs(si->sr->getUser()->getCID());
		hubs.insert( hubs.end(), sl.begin(), sl.end() );
		//Util::intersect(hubs, ClientManager::getInstance()->getHubs(si->sr->getUser()->getCID()));
	}
}

LRESULT SearchFrame::onDownloadTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlResults.GetSelectedCount() == 1) {
		int i = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
		dcassert(i != -1);
		SearchInfo* si = ctrlResults.getItemData(i);
		SearchResult* sr = si->sr;
	
		if(sr->getType() == SearchResult::TYPE_FILE) {
			tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY)) + si->columns[COLUMN_FILENAME];
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

	StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
	if(newId < spl.size()) {
		ctrlResults.forEachSelectedT(SearchInfo::Download(Text::toT(spl[newId].first)));
	} else {
		dcassert((newId - spl.size()) < targets.size());
		ctrlResults.forEachSelectedT(SearchInfo::DownloadTarget(Text::toT(targets[newId - spl.size()])));
	}
	return 0;
}

LRESULT SearchFrame::onDownloadWholeFavoriteDirs(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
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
		SearchManager::getInstance()->stopSearch((int*)this);
		SettingsManager::getInstance()->removeListener(this);
		if(searches != 0) {
			searches--;
			TimerManager::getInstance()->removeListener(this);
		}
		SearchManager::getInstance()->removeListener(this);
 		ClientManager::getInstance()->removeListener(this);
		frames.erase(m_hWnd);

		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		ctrlResults.SetRedraw(FALSE);
		ctrlResults.deleteAllItems();
		ctrlResults.SetRedraw(TRUE);

		for(SearchInfo::Iter i = PausedResults.begin(); i != PausedResults.end(); ++i) {
			delete *i;
		}

		for(int i = 0; i < ctrlHubs.GetItemCount(); i++) {
			delete ctrlHubs.getItemData(i);
		}
		ctrlHubs.DeleteAllItems();

		ctrlResults.saveHeaderOrder(SettingsManager::SEARCHFRAME_ORDER, SettingsManager::SEARCHFRAME_WIDTHS, 
			SettingsManager::SEARCHFRAME_VISIBLE);

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
		
		ctrlStatus.SetParts(4, w);

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
		rc.bottom = rc.top + comboH;
		rc.right = w2/3;
		ctrlMode.MoveWindow(rc);

		sizeLabel.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);

		rc.left = rc.right + lMargin;
		rc.right += w2/3;
		rc.bottom = rc.top + 21;
		ctrlSize.MoveWindow(rc);

		rc.left = rc.right + lMargin;
		rc.right = width - rMargin;
		rc.bottom = rc.top + comboH;
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
		ctrlPurge.MoveWindow(rc);
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
	if(!WinUtil::getUCParams(m_hWnd, uc, ucLineParams))
		return;

	StringMap ucParams = ucLineParams;

	set<CID> users;

	int sel = -1;
	while((sel = ctrlResults.GetNextItem(sel, LVNI_SELECTED)) != -1) {
		SearchResult* sr = ctrlResults.getItemData(sel)->sr;

		if(!sr->getUser()->isOnline())
			continue;

		if(uc.getType() == UserCommand::TYPE_RAW_ONCE) {
			if(users.find(sr->getUser()->getCID()) != users.end())
				continue;
			users.insert(sr->getUser()->getCID());
		}

		ucParams["fileFN"] = sr->getFile();
		ucParams["fileSI"] = Util::toString(sr->getSize());
		ucParams["fileSIshort"] = Util::formatBytes(sr->getSize());
		if(sr->getTTH() != NULL) {
			ucParams["fileTR"] = sr->getTTH()->toBase32();
		} else {
			ucParams["fileTR"] = "NONE";
		}

		// compatibility with 0.674 and earlier
		ucParams["file"] = ucParams["fileFN"];
		ucParams["filesize"] = ucParams["fileSI"];
		ucParams["filesizeshort"] = ucParams["fileSIshort"];
		ucParams["tth"] = ucParams["fileTR"];

		StringMap tmp = ucParams;
		ClientManager::getInstance()->userCommand(sr->getUser(), uc, tmp, true);
	}
}

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
}

LRESULT SearchFrame::onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	switch(wParam) {
	case VK_TAB:
		if(uMsg == WM_KEYDOWN) {
			onTab(WinUtil::isShift());
		}
		break;
	case VK_RETURN:
		if( WinUtil::isShift() || WinUtil::isCtrl() || WinUtil::isAlt() ) {
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
			SearchResult* sr = si->sr;	 	
            // Check previous search results for dupes	 	
			for(vector<SearchInfo*>::const_iterator s = ctrlResults.mainItems.begin(); s != ctrlResults.mainItems.end(); ++s) {
				SearchInfo* si2 = *s;
                SearchResult* sr2 = si2->sr;
				if((sr->getUser()->getCID() == sr2->getUser()->getCID()) && (sr->getFile() == sr2->getFile())) {
					delete si;	 	
                    return 0;	 	
				}
                for(SearchInfo::Iter k = si2->subItems.begin(); k != si2->subItems.end(); k++){	 	
					if((sr->getUser()->getCID() == (*k)->getUser()->getCID()) && (sr->getFile() == (*k)->sr->getFile())) {	 	
				        delete si;	 	
		                return 0;	 	
					} 	
				}	 	
            }
			if(bPaused == false) {
				bool resort = false;
				if(resultsCount++ % 13 == 0) {
					//ctrlResults.SetRedraw(FALSE);
					resort = true;
				}

				if(!si->columns[COLUMN_TTH].empty() && useGrouping) {
					ctrlResults.insertGroupedItem(si, expandSR);
				} else {
					ctrlResults.insertItem(si, si->imageIndex());
					ctrlResults.mainItems.push_back(si);
				}

				if(!filter.empty())
					updateSearchList(si);

				if (BOOLSETTING(BOLD_SEARCH)) {
					setDirty();
				}
				ctrlStatus.SetText(2, (Util::toStringW(resultsCount) + _T(" ") + TSTRING(FILES)).c_str());

				if(resort) {
					ctrlResults.resort();
					//ctrlResults.SetRedraw(TRUE);
				}
			} else {
				PausedResults.push_back(si);
				ctrlStatus.SetText(2, (Util::toStringW(resultsCount-PausedResults.size()) + _T("/") + Util::toStringW(resultsCount) + _T(" ") + WSTRING(FILES)).c_str());
			}
		}
		break;
	case FILTER_RESULT:
		ctrlStatus.SetText(3, (Util::toStringW(droppedResults) + _T(" ") + TSTRING(FILTERED)).c_str());
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
			SearchQueueItem* aSearch = (SearchQueueItem*)(lParam);

			ctrlResults.deleteAllItems();
			resultsCount = 0;

			{
				Lock l(cs);
				search = StringTokenizer<tstring>(aSearch->getSearch(), ' ').getTokens();
			}
			isHash = (aSearch->getTypeMode() == SearchManager::TYPE_TTH);

			// Turn off the countdown timer if no more manual searches left
			if(searches == 0)
				TimerManager::getInstance()->removeListener(this);

			// Update the status bar
			ctrlStatus.SetText(1, Text::toT(STRING(SEARCHING_FOR) + aSearch->getTarget() + "...").c_str());
			ctrlStatus.SetText(2, _T(""));
			ctrlStatus.SetText(3, _T(""));
	
			SetWindowText(Text::toT(STRING(SEARCH) + " - " + aSearch->getTarget()).c_str());
			delete aSearch;

			droppedResults = 0;
		}
		break;
	}

	return 0;
}

LRESULT SearchFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlResults && ctrlResults.GetSelectedCount() > 0) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	
		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlResults, pt);
		}
		
		if(ctrlResults.GetSelectedCount() > 0) {

			while(targetMenu.GetMenuItemCount() > 0) {
				targetMenu.DeleteMenu(0, MF_BYPOSITION);
			}
			while(targetDirMenu.GetMenuItemCount() > 0) {
				targetDirMenu.DeleteMenu(0, MF_BYPOSITION);
			}

			int n = 0;

			targetMenu.InsertSeparatorFirst(TSTRING(DOWNLOAD_TO));
			//Append favorite download dirs
			StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
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
				targetMenu.InsertSeparatorLast(TSTRING(PREVIOUS_FOLDERS));
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
					targetMenu.InsertSeparatorLast(TSTRING(ADD_AS_SOURCE));
					for(StringIter i = targets.begin(); i != targets.end(); ++i) {
						targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_TARGET + n, Text::toT(*i).c_str());
						n++;
					}
				}
			}

			n = 0;
			targetDirMenu.InsertSeparatorFirst(TSTRING(DOWNLOAD_WHOLE_DIR_TO));
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
					resultsMenu.EnableMenuItem(IDC_SEARCH_ALTERNATES, MFS_ENABLED);
					resultsMenu.EnableMenuItem(IDC_BITZI_LOOKUP, MFS_ENABLED);
					resultsMenu.EnableMenuItem(IDC_COPY_TTH, MF_BYCOMMAND | MFS_ENABLED);
					resultsMenu.EnableMenuItem(IDC_COPY_LINK, MF_BYCOMMAND | MFS_ENABLED);
				} else {
					resultsMenu.EnableMenuItem(IDC_SEARCH_ALTERNATES, MFS_DISABLED);
					resultsMenu.EnableMenuItem(IDC_BITZI_LOOKUP, MFS_DISABLED);
					resultsMenu.EnableMenuItem(IDC_COPY_TTH, MF_BYCOMMAND | MFS_DISABLED);
					resultsMenu.EnableMenuItem(IDC_COPY_LINK, MF_BYCOMMAND | MFS_DISABLED);
				}		
			}

			prepareMenu(resultsMenu, UserCommand::CONTEXT_SEARCH, cs.hubs);
			if(!(resultsMenu.GetMenuState(resultsMenu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {	
				resultsMenu.AppendMenu(MF_SEPARATOR);
			}
			resultsMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
			checkAdcItems(resultsMenu);
			resultsMenu.InsertSeparatorFirst(Text::toT(sr->getFileName()));
			copyMenu.InsertSeparatorFirst(TSTRING(USERINFO));
			grantMenu.InsertSeparatorFirst(TSTRING(GRANT_SLOTS_MENU));	
			resultsMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

			resultsMenu.RemoveFirstItem();
			copyMenu.RemoveFirstItem();
			grantMenu.RemoveFirstItem();
			targetDirMenu.RemoveFirstItem();
			targetMenu.RemoveFirstItem();

			resultsMenu.DeleteMenu(resultsMenu.GetMenuItemCount()-1, MF_BYPOSITION);
			resultsMenu.DeleteMenu(resultsMenu.GetMenuItemCount()-1, MF_BYPOSITION);
			cleanMenu(resultsMenu);
			return TRUE; 
		}
	}
	bHandled = FALSE;
	return FALSE; 
}

void SearchFrame::initHubs() {
	ctrlHubs.insertItem(new HubInfo(Util::emptyStringT, TSTRING(ONLY_WHERE_OP), false), 0);
	ctrlHubs.SetCheckState(0, false);

	ClientManager* clientMgr = ClientManager::getInstance();
	clientMgr->lock();
	clientMgr->addListener(this);

	Client::List& clients = clientMgr->getClients();

	Client::List::const_iterator it;
	Client::List::const_iterator endIt = clients.end();
	for(it = clients.begin(); it != endIt; ++it) {
		Client* client = *it;
		if (!client->isConnected())
			continue;

		onHubAdded(new HubInfo(Text::toT(client->getHubUrl()), Text::toT(client->getHubName()), client->getMyIdentity().isOp()));
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
		if(ctrlHubs.getItemData(nItem)->url == info->url)
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
		if(ctrlHubs.getItemData(nItem)->url == info->url)
			break;
	}

	delete info;

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

LRESULT SearchFrame::onBrowseList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlResults.forEachSelected(&SearchInfo::browseList);
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
	ctrlSearchBox.ResetContent();
	lastSearches.clear();
	return 0;
}


void SearchFrame::SearchInfo::update() { 
	if(sr->getType() == SearchResult::TYPE_FILE) {
		if(sr->getFile().rfind(_T('\\')) == tstring::npos) {
			columns[COLUMN_FILENAME] = Text::toT(sr->getUtf8() ? sr->getFile() : Text::acpToUtf8(sr->getFile()));
		} else {
			columns[COLUMN_FILENAME] = Text::toT(Util::getFileName(sr->getUtf8() ? sr->getFile() : Text::acpToUtf8(sr->getFile())));
			columns[COLUMN_PATH] = Text::toT(Util::getFilePath(sr->getUtf8() ? sr->getFile() : Text::acpToUtf8(sr->getFile())));
		}

		columns[COLUMN_TYPE] = Text::toT(Util::getFileExt(Text::fromT(columns[COLUMN_FILENAME])));
		if(!columns[COLUMN_TYPE].empty() && columns[COLUMN_TYPE][0] == _T('.'))
			columns[COLUMN_TYPE].erase(0, 1);
		columns[COLUMN_SIZE] = Util::formatBytesW(sr->getSize());
		columns[COLUMN_EXACT_SIZE] = Util::formatExactSize(sr->getSize());
	} else {
		columns[COLUMN_FILENAME] = Text::toT(sr->getUtf8() ? sr->getFileName() : Text::acpToUtf8(sr->getFileName()));
		columns[COLUMN_PATH] = Text::toT(sr->getUtf8() ? sr->getFile() : Text::acpToUtf8(sr->getFile()));
		columns[COLUMN_TYPE] = TSTRING(DIRECTORY);
		if(sr->getSize() > 0) {
			columns[COLUMN_SIZE] = Util::formatBytesW(sr->getSize());
			columns[COLUMN_EXACT_SIZE] = Util::formatExactSize(sr->getSize());
		}
	}
	columns[COLUMN_NICK] = Text::toT(sr->getUser()->getFirstNick());
	columns[COLUMN_CONNECTION] = Text::toT(ClientManager::getInstance()->getConnection(sr->getUser()->getCID()));
	columns[COLUMN_HUB] = Text::toT(sr->getHubName());
	columns[COLUMN_SLOTS] = Text::toT(sr->getSlotString());
	columns[COLUMN_IP] = Text::toT(sr->getIP());
	flagimage = 0;
	if (!columns[COLUMN_IP].empty()) {
		// Only attempt to grab a country mapping if we actually have an IP address
		tstring tmpCountry = Text::toT(Util::getIpCountry(sr->getIP()));
		if(!tmpCountry.empty()) {
			columns[COLUMN_IP] = tmpCountry + _T(" (") + columns[COLUMN_IP] + _T(")");
			flagimage = WinUtil::getFlagImage(Text::fromT(tmpCountry).c_str());
		}
	}
	if(sr->getTTH() != NULL) {
		columns[COLUMN_TTH] = Text::toT(sr->getTTH()->toBase32());
	}	
	if (sr->getUser()->getLastDownloadSpeed() > 0) {
		columns[COLUMN_UPLOAD] = Util::formatBytesW(sr->getUser()->getLastDownloadSpeed()) + _T("/s");
	} else if(user->isSet(User::FIREBALL)) {
		columns[COLUMN_UPLOAD] = Text::toT(">=100 kB/s");
	} else {
		columns[COLUMN_UPLOAD] = Text::toT("N/A");
	}
}

LRESULT SearchFrame::onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int pos = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
	dcassert(pos != -1);
	string sCopy;
	if ( pos >= 0 ) {
		SearchResult* sr = ctrlResults.getItemData(pos)->sr;
		switch (wID) {
			case IDC_COPY_NICK:
				sCopy = sr->getUser()->getFirstNick();
				break;
			case IDC_COPY_FILENAME:
				sCopy = Util::getFileName(sr->getUtf8() ? sr->getFile() : Text::acpToUtf8(sr->getFile()));
				break;
			case IDC_COPY_PATH:
				sCopy = Util::getFilePath(sr->getUtf8() ? sr->getFile() : Text::acpToUtf8(sr->getFile()));
				break;
			case IDC_COPY_SIZE:
				sCopy = Util::formatBytes(sr->getSize());
				break;
			case IDC_COPY_LINK:
				if(sr->getType() == SearchResult::TYPE_FILE) {
					WinUtil::copyMagnet(sr->getTTH(), Text::toT(sr->getUtf8() ? sr->getFileName() : Text::acpToUtf8(sr->getFileName())), sr->getSize());
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

LRESULT SearchFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	CRect rc;
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT: {
		cd->clrText = WinUtil::textColor;	
		SearchInfo* si = (SearchInfo*)cd->nmcd.lItemlParam;
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
		if(BOOLSETTING(GET_USER_COUNTRY) && (ctrlResults.findColumn(cd->iSubItem) == COLUMN_IP)) {
			SearchInfo* si = (SearchInfo*)cd->nmcd.lItemlParam;
			ctrlResults.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);
			COLORREF color;
			if(ctrlResults.GetItemState((int)cd->nmcd.dwItemSpec, LVIS_SELECTED) & LVIS_SELECTED) {
				if(ctrlResults.m_hWnd == ::GetFocus()) {
					color = GetSysColor(COLOR_HIGHLIGHT);
					SetBkColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHT));
					SetTextColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
				} else {
					color = GetBkColor(cd->nmcd.hdc);
					SetBkColor(cd->nmcd.hdc, color);
				}				
			} else {
				color = WinUtil::bgColor;
				SetBkColor(cd->nmcd.hdc, WinUtil::bgColor);
				SetTextColor(cd->nmcd.hdc, WinUtil::textColor);
			}
			CRect rc2 = rc;
			rc2.left += 2;
			HGDIOBJ oldpen = ::SelectObject(cd->nmcd.hdc, CreatePen(PS_SOLID,0, color));
			HGDIOBJ oldbr = ::SelectObject(cd->nmcd.hdc, CreateSolidBrush(color));
			Rectangle(cd->nmcd.hdc,rc.left, rc.top, rc.right, rc.bottom);

			DeleteObject(::SelectObject(cd->nmcd.hdc, oldpen));
			DeleteObject(::SelectObject(cd->nmcd.hdc, oldbr));

			TCHAR buf[256];
			ctrlResults.GetItemText((int)cd->nmcd.dwItemSpec, cd->iSubItem, buf, 255);
			buf[255] = 0;
			if(_tcslen(buf) > 0) {
				LONG top = rc2.top + (rc2.Height() - 15)/2;
				if((top - rc2.top) < 2)
					top = rc2.top + 1;

				POINT p = { rc2.left, top };
				WinUtil::flagImages.Draw(cd->nmcd.hdc, si->getFlagImage(), p, LVSIL_SMALL);
				top = rc2.top + (rc2.Height() - WinUtil::getTextHeight(cd->nmcd.hdc) - 1)/2;
				::ExtTextOut(cd->nmcd.hdc, rc2.left + 30, top + 1, ETO_CLIPPED, rc2, buf, _tcslen(buf), NULL);
				return CDRF_SKIPDEFAULT;
			}
		}		
	}
	default:
		return CDRF_DODEFAULT;
	}
}

LRESULT SearchFrame::onFilterChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!BOOLSETTING(FILTER_ENTER) || (wParam == VK_RETURN)) {
		TCHAR *buf = new TCHAR[ctrlFilter.GetWindowTextLength()+1];
		ctrlFilter.GetWindowText(buf, ctrlFilter.GetWindowTextLength()+1);
		filter = buf;
		delete[] buf;
	
		updateSearchList();
	}

	bHandled = false;

	return 0;
}

bool SearchFrame::parseFilter(FilterModes& mode, int64_t& size) {
	tstring::size_type start = (tstring::size_type)tstring::npos;
	tstring::size_type end = (tstring::size_type)tstring::npos;
	int64_t multiplier = 1;
	
	if(filter.compare(0, 2, _T(">=")) == 0) {
		mode = GREATER_EQUAL;
		start = 2;
	} else if(filter.compare(0, 2, _T("<=")) == 0) {
		mode = LESS_EQUAL;
		start = 2;
	} else if(filter.compare(0, 2, _T("==")) == 0) {
		mode = EQUAL;
		start = 2;
	} else if(filter.compare(0, 2, _T("!=")) == 0) {
		mode = NOT_EQUAL;
		start = 2;
	} else if(filter[0] == _T('<')) {
		mode = LESS;
		start = 1;
	} else if(filter[0] == _T('>')) {
		mode = GREATER;
		start = 1;
	} else if(filter[0] == _T('=')) {
		mode = EQUAL;
		start = 1;
	}

	if(start == tstring::npos)
		return false;
	if(filter.length() <= start)
		return false;

	if((end = Util::findSubString(filter, _T("TiB"))) != tstring::npos) {
		multiplier = 1024LL * 1024LL * 1024LL * 1024LL;
	} else if((end = Util::findSubString(filter, _T("GiB"))) != tstring::npos) {
		multiplier = 1024*1024*1024;
	} else if((end = Util::findSubString(filter, _T("MiB"))) != tstring::npos) {
		multiplier = 1024*1024;
	} else if((end = Util::findSubString(filter, _T("KiB"))) != tstring::npos) {
		multiplier = 1024;
	} else if((end = Util::findSubString(filter, _T("TB"))) != tstring::npos) {
		multiplier = 1000LL * 1000LL * 1000LL * 1000LL;
	} else if((end = Util::findSubString(filter, _T("GB"))) != tstring::npos) {
		multiplier = 1000*1000*1000;
	} else if((end = Util::findSubString(filter, _T("MB"))) != tstring::npos) {
		multiplier = 1000*1000;
	} else if((end = Util::findSubString(filter, _T("kB"))) != tstring::npos) {
		multiplier = 1000;
	} else if((end = Util::findSubString(filter, _T("B"))) != tstring::npos) {
		multiplier = 1;
	}


	if(end == tstring::npos) {
		end = filter.length();
	}
	
	tstring tmpSize = filter.substr(start, end-start);
	size = static_cast<int64_t>(Util::toDouble(Text::fromT(tmpSize)) * multiplier);
	
	return true;
}

bool SearchFrame::matchFilter(SearchInfo* si, int sel, bool doSizeCompare, FilterModes mode, int64_t size) {
	bool insert = false;

	if(filter.empty()) {
		insert = true;
	} else if(doSizeCompare) {
		switch(mode) {
			case EQUAL: insert = (size == si->sr->getSize()); break;
			case GREATER_EQUAL: insert = (size <=  si->sr->getSize()); break;
			case LESS_EQUAL: insert = (size >=  si->sr->getSize()); break;
			case GREATER: insert = (size < si->sr->getSize()); break;
			case LESS: insert = (size > si->sr->getSize()); break;
			case NOT_EQUAL: insert = (size != si->sr->getSize()); break;
		}
	} else {
		PME reg(Text::fromT(filter),"i");
		if(!reg.IsValid()) {
			insert = true;
		} else {
			insert = reg.match(Text::fromT(si->getText(sel))) > 0;
		}
	}
	return insert;
}
	

void SearchFrame::updateSearchList(SearchInfo* si) {
	int64_t size = -1;
	FilterModes mode = NONE;

	int sel = ctrlFilterSel.GetCurSel();
	bool doSizeCompare = sel == COLUMN_SIZE && parseFilter(mode, size);

	if(si != NULL) {
		if(!matchFilter(si, sel, doSizeCompare, mode, size))
			ctrlResults.deleteItem(si);
	} else {
		ctrlResults.SetRedraw(FALSE);
		ctrlResults.DeleteAllItems();

		for(vector<SearchInfo*>::const_iterator i = ctrlResults.mainItems.begin(); i != ctrlResults.mainItems.end(); ++i) {
			SearchInfo* si = *i;
			si->collapsed = true;
			if(matchFilter(si, sel, doSizeCompare, mode, size)) {
				dcassert(ctrlResults.findItem(si) == -1);
				int k = ctrlResults.insertItem(si, si->imageIndex());
				if(si->subItems.size() > 0) {
					if(si->collapsed) {
						ctrlResults.SetItemState(k, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);	
					} else {
						ctrlResults.SetItemState(k, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);	
					}
				} else {
					ctrlResults.SetItemState(k, INDEXTOSTATEIMAGEMASK(0), LVIS_STATEIMAGEMASK);	
				}
			}
		}
		ctrlResults.SetRedraw(TRUE);
	}
}

LRESULT SearchFrame::onSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	TCHAR *buf = new TCHAR[ctrlFilter.GetWindowTextLength()+1];
	ctrlFilter.GetWindowText(buf, ctrlFilter.GetWindowTextLength()+1);
	filter = buf;
	delete[] buf;
	
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
