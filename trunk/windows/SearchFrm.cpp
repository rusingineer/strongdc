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

#include "SearchFrm.h"
#include "LineDlg.h"
#include "CZDCLib.h"

#include "../client/QueueManager.h"
#include "../client/StringTokenizer.h"
#include "../client/ClientManager.h"

StringList SearchFrame::lastSearches;

int SearchFrame::columnIndexes[] = { COLUMN_FILENAME, COLUMN_NICK, COLUMN_TYPE, COLUMN_SIZE,
	COLUMN_PATH, COLUMN_SLOTS, COLUMN_CONNECTION, COLUMN_HUB, COLUMN_EXACT_SIZE, COLUMN_UPLOAD, COLUMN_IP, COLUMN_TTH };
int SearchFrame::columnSizes[] = { 200, 100, 50, 80, 100, 40, 70, 150, 80, 80, 100, 125 };

static ResourceManager::Strings columnNames[] = { ResourceManager::FILE, ResourceManager::USER, ResourceManager::TYPE, ResourceManager::SIZE, 
	ResourceManager::PATH, ResourceManager::SLOTS, ResourceManager::CONNECTION, 
	ResourceManager::HUB, ResourceManager::EXACT_SIZE, ResourceManager::AVERAGE_UPLOAD, ResourceManager::IP_BARE, ResourceManager::TTH_ROOT };

void SearchFrame::openWindow(const string& str /* = Util::emptyString */, LONGLONG size /* = 0 */, SearchManager::SizeModes mode /* = SearchManager::SIZE_ATLEAST */, SearchManager::TypeModes type /* = SearchManager::TYPE_ANY */) {
	SearchFrame* pChild = new SearchFrame();
	pChild->setInitial(str, size, mode, type);
	pChild->CreateEx(WinUtil::mdiClient);	
}

LRESULT SearchFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	ctrlSearchBox.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL, 0);
	for(StringIter i = lastSearches.begin(); i != lastSearches.end(); ++i) {
		ctrlSearchBox.InsertString(0, i->c_str());
	}
	searchBoxContainer.SubclassWindow(ctrlSearchBox.m_hWnd);
	ctrlSearchBox.SetExtendedUI();
	
	ctrlMode.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		/*WS_HSCROLL | WS_VSCROLL | */CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);
	modeContainer.SubclassWindow(ctrlMode.m_hWnd);

	ctrlSize.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL | ES_NUMBER, WS_EX_CLIENTEDGE);
	sizeContainer.SubclassWindow(ctrlSize.m_hWnd);

	ctrlSizeMode.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		/*WS_HSCROLL | WS_VSCROLL | */CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);
	sizeModeContainer.SubclassWindow(ctrlSizeMode.m_hWnd);

	ctrlFiletype.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		/*WS_HSCROLL | WS_VSCROLL | */CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_OWNERDRAWFIXED, WS_EX_CLIENTEDGE, IDC_FILETYPES);

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
	
	DWORD styles = LVS_EX_HEADERDRAGDROP;
	if (BOOLSETTING(FULL_ROW_SELECT))
		styles |= LVS_EX_FULLROWSELECT;
	if (BOOLSETTING(SHOW_INFOTIPS))
		styles |= LVS_EX_INFOTIP;
	ctrlResults.SetExtendedListViewStyle(styles);
	
	if (BOOLSETTING(USE_SYSTEM_ICONS)) {
	ctrlResults.SetImageList(WinUtil::fileImages, LVSIL_SMALL);
	} else {
		images.CreateFromImage(IDB_SPEEDS, 16, 3, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
		ctrlResults.SetImageList(images, LVSIL_SMALL);
	}

	ctrlHubs.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_NOCOLUMNHEADER, WS_EX_CLIENTEDGE, IDC_HUB);
	hubsContainer.SubclassWindow(ctrlHubs.m_hWnd);	
	ctrlHubs.SetExtendedListViewStyle(LVS_EX_CHECKBOXES | (BOOLSETTING(FULL_ROW_SELECT) ? LVS_EX_FULLROWSELECT : 0));

	searchLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	searchLabel.SetFont(WinUtil::systemFont, FALSE);
	searchLabel.SetWindowText(CSTRING(SEARCH_FOR));

	sizeLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	sizeLabel.SetFont(WinUtil::systemFont, FALSE);
	sizeLabel.SetWindowText(CSTRING(SIZE));

	typeLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	typeLabel.SetFont(WinUtil::systemFont, FALSE);
	typeLabel.SetWindowText(CSTRING(FILE_TYPE));

	optionLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	optionLabel.SetFont(WinUtil::systemFont, FALSE);
	optionLabel.SetWindowText(CSTRING(SEARCH_OPTIONS));

	hubsLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	hubsLabel.SetFont(WinUtil::systemFont, FALSE);
	hubsLabel.SetWindowText(CSTRING(HUBS));

	ctrlSlots.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_FREESLOTS);
	ctrlSlots.SetButtonStyle(BS_AUTOCHECKBOX, FALSE);
	ctrlSlots.SetFont(WinUtil::systemFont, FALSE);
	ctrlSlots.SetWindowText(CSTRING(ONLY_FREE_SLOTS));
	slotsContainer.SubclassWindow(ctrlSlots.m_hWnd);

	ctrlTTH.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_TTHONLY);
	ctrlTTH.SetButtonStyle(BS_AUTOCHECKBOX, FALSE);
	ctrlTTH.SetFont(WinUtil::systemFont, FALSE);
	ctrlTTH.SetWindowText(CSTRING(ONLY_TTH));
	tthContainer.SubclassWindow(ctrlTTH.m_hWnd);


	if(BOOLSETTING(FREE_SLOTS_DEFAULT)) {
		ctrlSlots.SetCheck(true);
		onlyFree = true;
	}
	ctrlShowUI.Create(ctrlStatus.m_hWnd, rcDefault, "+/-", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlShowUI.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowUI.SetCheck(1);
	showUIContainer.SubclassWindow(ctrlShowUI.m_hWnd);

	ctrlDoSearch.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_SEARCH);
	ctrlDoSearch.SetWindowText(CSTRING(SEARCH));
	ctrlDoSearch.SetFont(WinUtil::systemFont);
	doSearchContainer.SubclassWindow(ctrlDoSearch.m_hWnd);

	ctrlSearchBox.SetFont(WinUtil::systemFont, FALSE);
	ctrlSize.SetFont(WinUtil::systemFont, FALSE);
	ctrlMode.SetFont(WinUtil::systemFont, FALSE);
	ctrlSizeMode.SetFont(WinUtil::systemFont, FALSE);
	ctrlFiletype.SetFont(WinUtil::systemFont, FALSE);

	ctrlMode.AddString(CSTRING(NORMAL));
	ctrlMode.AddString(CSTRING(AT_LEAST));
	ctrlMode.AddString(CSTRING(AT_MOST));
	ctrlMode.AddString(CSTRING(EXACT_SIZE));
	ctrlMode.SetCurSel(1);
	
	ctrlSizeMode.AddString(CSTRING(B));
	ctrlSizeMode.AddString(CSTRING(KB));
	ctrlSizeMode.AddString(CSTRING(MB));
	ctrlSizeMode.AddString(CSTRING(GB));
	if(initialSize == 0)
		ctrlSizeMode.SetCurSel(2);
	else
		ctrlSizeMode.SetCurSel(0);

	ctrlFiletype.AddString(CSTRING(ANY));
	ctrlFiletype.AddString(CSTRING(AUDIO));
	ctrlFiletype.AddString(CSTRING(COMPRESSED));
	ctrlFiletype.AddString(CSTRING(DOCUMENT));
	ctrlFiletype.AddString(CSTRING(EXECUTABLE));
	ctrlFiletype.AddString(CSTRING(PICTURE));
	ctrlFiletype.AddString(CSTRING(VIDEO));
	ctrlFiletype.AddString(CSTRING(DIRECTORY));
	ctrlFiletype.AddString("TTH");
	ctrlFiletype.SetCurSel(0);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(SEARCHFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(SEARCHFRAME_WIDTHS), COLUMN_LAST);

	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_EXACT_SIZE) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlResults.InsertColumn(j, CSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlResults.SetColumnOrderArray(COLUMN_LAST, columnIndexes);

	ctrlResults.SetBkColor(WinUtil::bgColor);
	ctrlResults.SetTextBkColor(WinUtil::bgColor);
	ctrlResults.SetTextColor(WinUtil::textColor);
	ctrlResults.SetFont(WinUtil::systemFont, FALSE);	// use Util::font instead to obey Appearace settings
	
	ctrlHubs.InsertColumn(0, "Dummy", LVCFMT_LEFT, LVSCW_AUTOSIZE, 0);
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

	copyMenu.AppendMenu(MF_STRING, IDC_COPY_NICK, CSTRING(COPY_NICK));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_FILENAME, CSTRING(FILENAME));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_PATH, CSTRING(PATH));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_SIZE, CSTRING(SIZE));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_TTH, CSTRING(TTH_ROOT));
		
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CSTRING(GRANT_EXTRA_SLOT));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_HOUR, CSTRING(GRANT_EXTRA_SLOT_HOUR));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_DAY, CSTRING(GRANT_EXTRA_SLOT_DAY));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_WEEK, CSTRING(GRANT_EXTRA_SLOT_WEEK));
	grantMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	grantMenu.AppendMenu(MF_STRING, IDC_UNGRANTSLOT, CSTRING(REMOVE_EXTRA_SLOT));
	
	resultsMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD, CSTRING(DOWNLOAD));
	resultsMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)targetMenu, CSTRING(DOWNLOAD_TO));
	resultsMenu.AppendMenu(MF_STRING, IDC_DOWNLOADDIR, CSTRING(DOWNLOAD_WHOLE_DIR));
	resultsMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)targetDirMenu, CSTRING(DOWNLOAD_WHOLE_DIR_TO));
	resultsMenu.AppendMenu(MF_STRING, IDC_VIEW_AS_TEXT, CSTRING(VIEW_AS_TEXT));
	resultsMenu.AppendMenu(MF_STRING, IDC_MP3, CSTRING(GET_MP3INFO));
	resultsMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	appendUserItems(resultsMenu);
	resultsMenu.DeleteMenu(resultsMenu.GetMenuItemCount()-2, MF_BYPOSITION);
	resultsMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	resultsMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)copyMenu, CSTRING(COPY));
	resultsMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)grantMenu, CSTRING(GRANT_SLOTS_MENU));
	resultsMenu.SetMenuDefaultItem(IDC_DOWNLOAD);
	resultsMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	resultsMenu.AppendMenu(MF_STRING, IDC_BITZI_LOOKUP, CSTRING(BITZI_LOOKUP));
	resultsMenu.AppendMenu(MF_STRING, IDC_COPY_LINK, CSTRING(COPY_MAGNET_LINK));

	UpdateLayout();
	
	if(!initialString.empty()) {
		lastSearches.push_back(initialString);
		ctrlSearchBox.InsertString(0, initialString.c_str());
		ctrlSearchBox.SetCurSel(0);
		ctrlMode.SetCurSel(initialMode);
		ctrlSize.SetWindowText(Util::toString(initialSize).c_str());
		ctrlFiletype.SetCurSel(initialType);
		onEnter();
	} else {
		SetWindowText(CSTRING(SEARCH));
	}

	m_hMenu = WinUtil::mainMenu;

	bHandled = FALSE;
	return 1;
}

LRESULT SearchFrame::onMeasure(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	HWND hwnd = 0;
	if(wParam == IDC_FILETYPES) return ListMeasure(hwnd, wParam, (MEASUREITEMSTRUCT *)lParam);
		else return FALSE;
};

LRESULT SearchFrame::onDrawItem(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	HWND hwnd = 0;
	if(wParam == IDC_FILETYPES) return ListDraw(hwnd, wParam, (DRAWITEMSTRUCT*)lParam);
		else return FALSE;
};


BOOL SearchFrame::ListMeasure(HWND hwnd, UINT uCtrlId, MEASUREITEMSTRUCT *mis)
{
	mis->itemHeight = 16;
	return TRUE;
}


BOOL SearchFrame::ListDraw(HWND hwnd, UINT uCtrlId, DRAWITEMSTRUCT *dis)
{ 	TCHAR szText[MAX_PATH+1];
	int idx;
	
	switch(dis->itemAction)
	{
	case ODA_FOCUS:
		if(!(dis->itemState & 0x0200))
			DrawFocusRect(dis->hDC, &dis->rcItem);
		break;

	case ODA_SELECT:
	case ODA_DRAWENTIRE:

		ctrlFiletype.GetLBText(dis->itemID, szText);
		if(dis->itemState & ODS_SELECTED)
		{
			SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
			SetBkColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHT));
		}
		else
		{			
			SetTextColor(dis->hDC, WinUtil::textColor);
			SetBkColor(dis->hDC, WinUtil::bgColor);
		}

		idx = dis->itemData;

	
		ExtTextOut(dis->hDC, dis->rcItem.left+22, dis->rcItem.top+1, ETO_OPAQUE, &dis->rcItem, szText, lstrlen(szText), 0);

		if(dis->itemState & ODS_FOCUS) 
		{
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

	startSearch();
	
	if(!(ctrlSearch.GetWindowTextLength() > 0 && lastSearch + 3*1000 < TimerManager::getInstance()->getTick()))
			return;

	int n = ctrlHubs.GetItemCount();
	for(int i = 0; i < n; i++) {
		if(ctrlHubs.GetCheckState(i)) {
			clients.push_back(ctrlHubs.getItemData(i)->ipPort);
		}
	}

	if(!clients.size())
		return;

	string s(ctrlSearch.GetWindowTextLength() + 1, '\0');
	ctrlSearch.GetWindowText(&s[0], s.size());
	s.resize(s.size()-1);

	string size(ctrlSize.GetWindowTextLength() + 1, '\0');
	ctrlSize.GetWindowText(&size[0], size.size());
	size.resize(size.size()-1);

	double lsize = Util::toDouble(size);
	switch(ctrlSizeMode.GetCurSel()) {
		case 1:
			lsize*=1024.0; break;
		case 2:
			lsize*=1024.0*1024.0; break;
		case 3:
			lsize*=1024.0*1024.0*1024.0; break;
	}

	int64_t llsize = (int64_t)lsize;

	for(int i = 0; i != ctrlResults.GetItemCount(); i++) {
		delete ctrlResults.getItemData(i);
	}
	ctrlResults.DeleteAllItems();
	ctrlStatus.SetText(2, ("0 "+STRING(FILES)).c_str());
		
	{
		Lock l(cs);
		search = StringTokenizer(s, ' ').getTokens();
	}
	//strip out terms beginning with -
	s.clear();
	for (StringList::const_iterator si = search.begin(); si != search.end(); ++si)
		if ((*si)[0] != '-') s += *si + ' ';	//Shouldn't get 0-length tokens, so safely assume at least a first char.
	s = s.substr(0, max(s.size(), static_cast<string::size_type>(1)) - 1);
	
	SearchManager::SizeModes mode((SearchManager::SizeModes)ctrlMode.GetCurSel());
	if(llsize == 0)
		mode = SearchManager::SIZE_DONTCARE;

	int ftype = ctrlFiletype.GetCurSel();		

	if(BOOLSETTING(CLEAR_SEARCH)){
		ctrlSearch.SetWindowText("");
	} else {
		lastSearch = TimerManager::getInstance()->getTick();
	}

	// Add new searches to the last-search dropdown list
	if(find(lastSearches.begin(), lastSearches.end(), s) == lastSearches.end()) {
		if(ctrlSearchBox.GetCount() > 19)
			ctrlSearchBox.DeleteString(19);
		ctrlSearchBox.InsertString(0, s.c_str());

		while(lastSearches.size() > 19) {
			lastSearches.erase(lastSearches.begin());
		}
		lastSearches.push_back(s);
	}
		
	ctrlStatus.SetText(1, (STRING(SEARCHING_FOR) + s + "...").c_str());
	{
		Lock l(cs);
		search = StringTokenizer(s, ' ').getTokens();
		isHash = (ftype == SearchManager::TYPE_HASH);
	}

	SetWindowText((STRING(SEARCH) + " - " + s).c_str());

	if(ftype == SearchManager::TYPE_HASH)
		s = "TTH:" + s;

	SearchManager::getInstance()->search(clients, s, llsize, 
		(SearchManager::TypeModes)ftype, mode);

}

void SearchFrame::onSearchResult(SearchResult* aResult) {
	// Check that this is really a relevant search result...
	{
		Lock l(cs);

		if(search.empty()) {
			return;
		}

		if(isHash) {
			if(aResult->getTTH() == NULL)
				return;
			if(Util::stricmp(aResult->getTTH()->toBase32(), search[0]) != 0)
				return;
		} else {
			for(StringIter j = search.begin(); j != search.end(); ++j) {
				if((*j->begin() != '-' && Util::findSubString(aResult->getFile(), *j) == -1) ||
					(*j->begin() == '-' && Util::findSubString(aResult->getFile(), j->substr(1)) != -1)
					) {
					return;
				}
			}
		}
	}

	// Reject results without free slots if selected
	if(onlyFree && aResult->getFreeSlots() < 1)
		return;

	if(onlyTTH && aResult->getTTH() == NULL)
		return;

	if(ctrlMode.GetCurSel() == SearchManager::SIZE_EXACT)
		{			
			string size(ctrlSize.GetWindowTextLength() + 1, '\0');
			ctrlSize.GetWindowText(&size[0], size.size());
			size.resize(size.size()-1);
			if(aResult->getSize() != Util::toInt64(size))
				return;
		}

	SearchInfo* i = new SearchInfo(aResult);
	PostMessage(WM_SPEAKER, ADD_RESULT, (LPARAM)i);	
}

void SearchFrame::SearchInfo::view() {
	try {
		if(sr->getType() == SearchResult::TYPE_FILE) {
			QueueManager::getInstance()->add(sr->getFile(), sr->getSize(), sr->getUser(), 
				Util::getTempPath() + fileName, sr->getTTH(), Util::emptyString,
				(QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_TEXT));
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::GetMP3Info() {
	try {
		if(sr->getType() == SearchResult::TYPE_FILE) {
			QueueManager::getInstance()->add(sr->getFile(), 2100, sr->getUser(), 
				Util::getTempPath() + fileName, sr->getTTH(), Util::emptyString,
				(QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_MP3_INFO));
		}
	} catch(const Exception&) {
	}
}
	
void SearchFrame::SearchInfo::Download::operator()(SearchInfo* si) {
	try {
		if(si->sr->getType() == SearchResult::TYPE_FILE) {
			QueueManager::getInstance()->add(si->sr->getFile(), si->sr->getSize(), si->sr->getUser(), 
				tgt + si->fileName, si->sr->getTTH());
		} else {
			QueueManager::getInstance()->addDirectory(si->sr->getFile(), si->sr->getUser(), tgt);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::DownloadWhole::operator()(SearchInfo* si) {
		try {
		if(si->sr->getType() == SearchResult::TYPE_FILE) {
			QueueManager::getInstance()->addDirectory(si->path, si->sr->getUser(), tgt);
		} else {
			QueueManager::getInstance()->addDirectory(si->sr->getFile(), si->sr->getUser(), tgt);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::DownloadTarget::operator()(SearchInfo* si) {
		try {
		if(si->sr->getType() == SearchResult::TYPE_FILE) {
			QueueManager::getInstance()->add(si->sr->getFile(), si->sr->getSize(), si->sr->getUser(), 
			tgt, si->sr->getTTH());
		} else {
			QueueManager::getInstance()->addDirectory(si->sr->getFile(), si->sr->getUser(), tgt);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::getList() {
	try {
		QueueManager::getInstance()->addList(sr->getUser(), QueueItem::FLAG_CLIENT_VIEW, getPath());
	} catch(const Exception&) {
		// Ignore for now...
	}
}

void SearchFrame::SearchInfo::CheckSize::operator()(SearchInfo* si) {
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
		hub = si->sr->getUser()->getClientAddressPort();
	} else if(hub != si->sr->getUser()->getClientAddressPort()) {
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
			string target = SETTING(DOWNLOAD_DIRECTORY) + si->getFileName();
			if(WinUtil::browseFile(target, m_hWnd)) {
				WinUtil::addLastDir(Util::getFilePath(target));
				ctrlResults.forEachSelectedT(SearchInfo::DownloadTarget(target));
			}
			} else {
			string target = SETTING(DOWNLOAD_DIRECTORY);
			if(WinUtil::browseDirectory(target, m_hWnd)) {
				WinUtil::addLastDir(target);
				ctrlResults.forEachSelectedT(SearchInfo::Download(target));
			}
			}
	} else {
		string target = SETTING(DOWNLOAD_DIRECTORY);
		if(WinUtil::browseDirectory(target, m_hWnd)) {
			WinUtil::addLastDir(target);
			ctrlResults.forEachSelectedT(SearchInfo::Download(target));
		}
	}
	return 0;
}

LRESULT SearchFrame::onDownloadWholeTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	string target = SETTING(DOWNLOAD_DIRECTORY);
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
		ctrlResults.forEachSelectedT(SearchInfo::DownloadTarget(targets[newId - WinUtil::lastDirs.size()]));
	}
	return 0;
}

LRESULT SearchFrame::onDownloadWholeTarget(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	dcassert((wID-IDC_DOWNLOAD_WHOLE_TARGET) < (int)WinUtil::lastDirs.size());
	ctrlResults.forEachSelectedT(SearchInfo::DownloadWhole(WinUtil::lastDirs[wID-IDC_DOWNLOAD_WHOLE_TARGET]));
	return 0;
}

LRESULT SearchFrame::onDoubleClickResults(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
//	ctrlResults.forEachSelectedT(SearchInfo::Download(SETTING(DOWNLOAD_DIRECTORY)));
//PDC {
	int i = -1;
	while( (i = ctrlResults.GetNextItem(i, LVNI_SELECTED)) != -1) {
		SearchInfo* si = ctrlResults.getItemData(i);
		string t = SettingsManager::getInstance()->getDownloadDir(Util::getFileExt(si->getFileName()));		
		(SearchInfo::Download(t))(ctrlResults.getItemData(i));
	}
//PDC }
	return 0;
}

LRESULT SearchFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if(!closed) {
	SearchManager::getInstance()->removeListener(this);
 		ClientManager* clientMgr = ClientManager::getInstance();
 		clientMgr->removeListener(this);

		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		for(int i = 0; i < ctrlResults.GetItemCount(); i++) {
			delete ctrlResults.getItemData(i);
		}
		ctrlResults.DeleteAllItems();
		for(int i = 0; i < ctrlResults.GetItemCount(); i++) {
			delete ctrlHubs.getItemData(i);
		}
		ctrlHubs.DeleteAllItems();

		WinUtil::saveHeaderOrder(ctrlResults, SettingsManager::SEARCHFRAME_ORDER,
			SettingsManager::SEARCHFRAME_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);
				
		MDIDestroy(m_hWnd);
	return 0;
	}
}

LRESULT SearchFrame::onBitziLookup(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(ctrlResults.GetSelectedCount() == 1){
		SearchInfo* si = ctrlResults.getSelectedItem();
		if(si && si->sr->getType() == SearchResult::TYPE_FILE && si->sr->getTTH()) {
			string url = "http://bitzi.com/lookup/tree:tiger:" + si->sr->getTTH()->toBase32();
			WinUtil::openLink(url);
		}
	}
	
	return 0;
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

		// "Size"
		int w2 = width - rMargin - lMargin;
		rc.top += spacing;
		rc.bottom += spacing;
		rc.right = w2/3;
		ctrlMode.MoveWindow(rc);

		sizeLabel.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);

		rc.left = rc.right + lMargin;
		rc.right += w2/3;
		rc.bottom -= comboH;
		ctrlSize.MoveWindow(rc);

		rc.left = rc.right + lMargin;
		rc.right = width - rMargin;
		rc.bottom += comboH;
		ctrlSizeMode.MoveWindow(rc);
		rc.bottom -= comboH;

		// "File type"
		rc.left = lMargin;
		rc.right = width - rMargin;
		rc.top += spacing;
		rc.bottom = rc.top + comboH + 21;
		ctrlFiletype.MoveWindow(rc);
		rc.bottom -= comboH;

		typeLabel.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);

		// "Search options"
		rc.left = lMargin+4;
		rc.right = width - rMargin;
		rc.top += spacing;
		rc.bottom += spacing;
		ctrlSlots.MoveWindow(rc);

		optionLabel.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);

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

		// "Search"
		rc.right = width - rMargin;
		rc.left = rc.right - 100;
		rc.top = rc.bottom + labelH;
		rc.bottom = rc.top + 21;
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
		ucParams["mynick"] = sr->getUser()->getClientNick();
		ucParams["file"] = sr->getFile();
		ucParams["filesize"] = Util::toString(sr->getSize());
		ucParams["filesizeshort"] = Util::formatBytes(sr->getSize());

		sr->getUser()->getParams(ucParams);

			sr->getUser()->send(Util::formatParams(uc.getCommand(), ucParams));
		}
	return;
};

LRESULT SearchFrame::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	HWND hWnd = (HWND)lParam;
	HDC hDC = (HDC)wParam;
	if(hWnd == searchLabel.m_hWnd || hWnd == sizeLabel.m_hWnd || hWnd == optionLabel.m_hWnd || hWnd == typeLabel.m_hWnd
		|| hWnd == hubsLabel.m_hWnd || hWnd == ctrlSlots.m_hWnd || hWnd == ctrlTTH.m_hWnd) {
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
		ctrlSearch.m_hWnd, ctrlMode.m_hWnd, ctrlSize.m_hWnd, ctrlSizeMode.m_hWnd, 
			ctrlFiletype.m_hWnd, ctrlSlots.m_hWnd, ctrlTTH.m_hWnd, ctrlDoSearch.m_hWnd, ctrlSearch.m_hWnd, 
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

LRESULT SearchFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
 	switch(wParam) {
	case ADD_RESULT:
		{
			SearchInfo* si = (SearchInfo*)lParam;
			SearchResult* sr = si->sr;
			// Check previous search results for dupes
			if(BOOLSETTING(FILTER_SEARCH)) {
				for(int i = 0, j = ctrlResults.GetItemCount(); i < j; ++i) {
					SearchInfo* si2 = ctrlResults.getItemData(i);
					SearchResult* sr2 = si2->sr;
					if((sr->getUser()->getNick() == sr2->getUser()->getNick()) && (sr->getFile() == sr2->getFile())) {
						delete si;
						return 0;
					}
				}
			}

			int image = 0;
			if (BOOLSETTING(USE_SYSTEM_ICONS)) {
				image = sr->getType() == SearchResult::TYPE_FILE ? WinUtil::getIconIndex(sr->getFile()) : WinUtil::getDirIconIndex();
			} else {
				const string& tmp = sr->getUser()->getConnection();

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
				if(sr->getType() == SearchResult::TYPE_FILE)
					image+=4;
			}
			ctrlResults.insertItem(si, image);
			ctrlStatus.SetText(2, (Util::toString(ctrlResults.GetItemCount())+" "+STRING(FILES)).c_str());
		}
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

		targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOADTO, CSTRING(BROWSE));
		if(WinUtil::lastDirs.size() > 0) {
			targetMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
			for(StringIter i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i) {
				targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_TARGET + n, i->c_str());
				n++;
			}
		}

		SearchInfo::CheckSize cs = ctrlResults.forEachSelectedT(SearchInfo::CheckSize());

		if(cs.size != -1) {
				targets.clear();
			if(BOOLSETTING(USE_EXTENSION_DOWNTO)) { 
				QueueManager::getInstance()->getTargetsBySize(targets, cs.size, cs.ext);
			} else {
				QueueManager::getInstance()->getTargetsBySize(targets, cs.size, Util::emptyString);
			}
				if(targets.size() > 0) {
					targetMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
					for(StringIter i = targets.begin(); i != targets.end(); ++i) {
						targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_TARGET + n, i->c_str());
						n++;
					}
				}
			}

		n = 0;
			if(WinUtil::lastDirs.size() > 0) {
			targetDirMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
				for(StringIter i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i) {
				targetDirMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_WHOLE_TARGET + n, i->c_str());
						n++;
					}
				}		
		// magnet link
		resultsMenu.EnableMenuItem(IDC_BITZI_LOOKUP, MF_BYCOMMAND | MF_GRAYED);
		if(ctrlResults.GetSelectedCount() == 1){
			SearchInfo* si = ctrlResults.getSelectedItem();
			if(si && si->sr->getType() == SearchResult::TYPE_FILE && si->sr->getTTH()) {
				resultsMenu.EnableMenuItem(IDC_BITZI_LOOKUP, MF_BYCOMMAND | MF_ENABLED);
			}
		}
		prepareMenu(resultsMenu, UserCommand::CONTEXT_SEARCH, cs.hub, cs.op);
		if(!(resultsMenu.GetMenuState(resultsMenu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {	
			resultsMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
		}
		resultsMenu.AppendMenu(MF_STRING, IDC_REMOVE, CSTRING(REMOVE));
		resultsMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		resultsMenu.DeleteMenu(resultsMenu.GetMenuItemCount()-1, MF_BYPOSITION);
		resultsMenu.DeleteMenu(resultsMenu.GetMenuItemCount()-1, MF_BYPOSITION);
		cleanMenu(resultsMenu);
		return TRUE; 
	}
	return FALSE; 
}


void SearchFrame::initHubs() {
 	ctrlHubs.insertItem(new HubInfo(Util::emptyString, STRING(ONLY_WHERE_OP), false), 0);
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

		onHubAdded(new HubInfo(client->getIpPort(), client->getName(), client->getOp()));
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

LRESULT SearchFrame::onCopyNick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int pos = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
	dcassert(pos != -1);
	SearchInfo* si = ctrlResults.getItemData(pos);
	SearchResult* sr = si->sr;
	Clipboard::setText(sr->getUser()->getNick());
	return S_OK;
}

LRESULT SearchFrame::onCopyFilename(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int pos = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
	dcassert(pos != -1);
	SearchInfo* si = ctrlResults.getItemData(pos);
	SearchResult* sr = si->sr;
	Clipboard::setText(Util::getFileName(sr->getFile()));
	return S_OK;
}

LRESULT SearchFrame::onCopyPath(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int pos = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
	dcassert(pos != -1);
	SearchInfo* si = ctrlResults.getItemData(pos);
	SearchResult* sr = si->sr;
	Clipboard::setText(Util::getFilePath(sr->getFile()));
	return S_OK;
}

LRESULT SearchFrame::onCopySize(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int pos = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
	dcassert(pos != -1);
	SearchInfo* si = ctrlResults.getItemData(pos);
	SearchResult* sr = si->sr;
	Clipboard::setText(Util::formatBytes(sr->getSize()));
	return S_OK;
}

LRESULT SearchFrame::onCopyTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int pos = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
	dcassert(pos != -1);
	SearchInfo* si = ctrlResults.getItemData(pos);
	SearchResult* sr = si->sr;
	if(sr->getTTH() != NULL)
		Clipboard::setText(sr->getTTH()->toBase32());
	return S_OK;
}

LRESULT SearchFrame::onCopyMagnetLink(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlResults.GetSelectedCount() == 1){
		SearchInfo* si = ctrlResults.getSelectedItem();
		if(si && si->sr->getType() == SearchResult::TYPE_FILE && si->sr->getTTH() && si->sr->getSize()) {
			string link = "magnet:?xt=urn:tree:tiger:" + si->sr->getTTH()->toBase32() +
				"&xl=" + Util::toString(si->sr->getSize()) +
				"&dn=" +  si->sr->getFileName();
			WinUtil::setClipboard(link);
		}
	}

	return 0;
}

/**
 * @file
 * $Id$
 */
