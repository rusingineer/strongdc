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

#include "WinUtil.h"
#include "PrivateFrame.h"
#include "SearchFrm.h"
#include "LineDlg.h"
#include "MainFrm.h"

#include "../client/Util.h"
#include "../client/StringTokenizer.h"
#include "../client/ShareManager.h"
#include "../client/ClientManager.h"
#include "../client/TimerManager.h"
#include "../client/HubManager.h"
#include "../client/ResourceManager.h"
#include "../client/QueueManager.h"
#include "../client/UploadManager.h"
#include "../client/HashManager.h"
#include "winamp.h"
#include <strmif.h>
#include <control.h>
#include <Windows.h>
#include "../client/PluginManager.h"

WinUtil::ImageMap WinUtil::fileIndexes;
int WinUtil::fileImageCount;
HBRUSH WinUtil::bgBrush = NULL;
COLORREF WinUtil::textColor = 0;
COLORREF WinUtil::bgColor = 0;
HFONT WinUtil::font = NULL;
int WinUtil::fontHeight = 0;
HFONT WinUtil::boldFont = NULL;
HFONT WinUtil::systemFont = NULL;
HFONT WinUtil::monoFont = NULL;
HFONT WinUtil::tinyFont = NULL;
HFONT WinUtil::smallBoldFont = NULL;
CMenu WinUtil::mainMenu;
CMenu WinUtil::grantMenu;
CImageList WinUtil::fileImages;
CImageList WinUtil::userImages;
int WinUtil::dirIconIndex = 0;
StringList WinUtil::lastDirs;
HWND WinUtil::mainWnd = NULL;
HWND WinUtil::mdiClient = NULL;
FlatTabCtrl* WinUtil::tabCtrl = NULL;
HHOOK WinUtil::hook = NULL;
string WinUtil::tth;
string WinUtil::exceptioninfo;
bool WinUtil::isPM = false;
bool WinUtil::isAppActive = false;
bool WinUtil::trayIcon = false;
bool WinUtil::isMinimized = false;

WinUtil::tbIDImage WinUtil::ToolbarButtons[] = {
	{ID_FILE_CONNECT, 0, true, ResourceManager::MENU_PUBLIC_HUBS},
	{ID_FILE_RECONNECT, 1, false, ResourceManager::MENU_RECONNECT},
	{IDC_FOLLOW, 2, false, ResourceManager::MENU_FOLLOW_REDIRECT},
	{IDC_FAVORITES, 3, true, ResourceManager::MENU_FAVORITE_HUBS},
	{IDC_FAVUSERS, 4, true, ResourceManager::MENU_FAVORITE_USERS},
	{IDC_RECENTS, 5, true, ResourceManager::MENU_FILE_RECENT_HUBS},
	{IDC_QUEUE, 6, true, ResourceManager::MENU_DOWNLOAD_QUEUE},
	{IDC_FINISHED, 7, true, ResourceManager::FINISHED_DOWNLOADS},
	{IDC_UPLOAD_QUEUE, 8, true, ResourceManager::UPLOAD_QUEUE},
	{IDC_FINISHED_UL, 9, true, ResourceManager::FINISHED_UPLOADS},
	{ID_FILE_SEARCH, 10, false, ResourceManager::MENU_SEARCH},
	{IDC_FILE_ADL_SEARCH, 11, true, ResourceManager::MENU_ADL_SEARCH},
	{IDC_SEARCH_SPY, 12, true, ResourceManager::MENU_SEARCH_SPY},
	{IDC_NET_STATS, 13, true, ResourceManager::NETWORK_STATISTICS},
	{IDC_OPEN_FILE_LIST, 14, false, ResourceManager::MENU_OPEN_FILE_LIST},
	{ID_FILE_SETTINGS, 15, false, ResourceManager::MENU_SETTINGS},
	{IDC_NOTEPAD, 16, true, ResourceManager::MENU_NOTEPAD},
	{IDC_AWAY, 17, true, ResourceManager::AWAY},
	{IDC_SHUTDOWN, 18, true, ResourceManager::SHUTDOWN},
	{IDC_LIMITER, 19, true, ResourceManager::SETCZDC_ENABLE_LIMITING},
	{IDC_UPDATE, 20, false, ResourceManager::UPDATE_CHECK},
	{IDC_DISABLE_SOUNDS, 21, true, ResourceManager::DISABLE_SOUNDS},
	{0, 0, false, ResourceManager::MENU_NOTEPAD}
};

HLSCOLOR RGB2HLS (COLORREF rgb) {
	unsigned char minval = min(GetRValue(rgb), min(GetGValue(rgb), GetBValue(rgb)));
	unsigned char maxval = max(GetRValue(rgb), max(GetGValue(rgb), GetBValue(rgb)));
	float mdiff  = float(maxval) - float(minval);
	float msum   = float(maxval) + float(minval);

	float luminance = msum / 510.0f;
	float saturation = 0.0f;
	float hue = 0.0f; 

	if ( maxval != minval ) { 
		float rnorm = (maxval - GetRValue(rgb)  ) / mdiff;      
		float gnorm = (maxval - GetGValue(rgb)) / mdiff;
		float bnorm = (maxval - GetBValue(rgb) ) / mdiff;   

		saturation = (luminance <= 0.5f) ? (mdiff / msum) : (mdiff / (510.0f - msum));

		if (GetRValue(rgb) == maxval) hue = 60.0f * (6.0f + bnorm - gnorm);
		if (GetGValue(rgb) == maxval) hue = 60.0f * (2.0f + rnorm - bnorm);
		if (GetBValue(rgb) == maxval) hue = 60.0f * (4.0f + gnorm - rnorm);
		if (hue > 360.0f) hue = hue - 360.0f;
	}
	return HLS ((hue*255)/360, luminance*255, saturation*255);
}

static BYTE _ToRGB (float rm1, float rm2, float rh) {
	if      (rh > 360.0f) rh -= 360.0f;
	else if (rh <   0.0f) rh += 360.0f;

	if      (rh <  60.0f) rm1 = rm1 + (rm2 - rm1) * rh / 60.0f;   
	else if (rh < 180.0f) rm1 = rm2;
	else if (rh < 240.0f) rm1 = rm1 + (rm2 - rm1) * (240.0f - rh) / 60.0f;      

	return (BYTE)(rm1 * 255);
}

COLORREF HLS2RGB (HLSCOLOR hls) {
	float hue        = ((int)HLS_H(hls)*360)/255.0f;
	float luminance  = HLS_L(hls)/255.0f;
	float saturation = HLS_S(hls)/255.0f;

	if ( saturation == 0.0f ) {
		return RGB (HLS_L(hls), HLS_L(hls), HLS_L(hls));
	}
	float rm1, rm2;

	if ( luminance <= 0.5f ) rm2 = luminance + luminance * saturation;  
	else                     rm2 = luminance + saturation - luminance * saturation;
	rm1 = 2.0f * luminance - rm2;   
	BYTE red   = _ToRGB (rm1, rm2, hue + 120.0f);   
	BYTE green = _ToRGB (rm1, rm2, hue);
	BYTE blue  = _ToRGB (rm1, rm2, hue - 120.0f);

	return RGB (red, green, blue);
}

COLORREF HLS_TRANSFORM (COLORREF rgb, int percent_L, int percent_S) {
	HLSCOLOR hls = RGB2HLS (rgb);
	BYTE h = HLS_H(hls);
	BYTE l = HLS_L(hls);
	BYTE s = HLS_S(hls);

	if ( percent_L > 0 ) {
		l = BYTE(l + ((255 - l) * percent_L) / 100);
	} else if ( percent_L < 0 )	{
		l = BYTE((l * (100+percent_L)) / 100);
	}
	if ( percent_S > 0 ) {
		s = BYTE(s + ((255 - s) * percent_S) / 100);
	} else if ( percent_S < 0 ) {
		s = BYTE((s * (100+percent_S)) / 100);
	}
	return HLS2RGB (HLS(h, l, s));
}

void UserInfoBase::matchQueue() {
	try {
		QueueManager::getInstance()->addList(user, QueueItem::FLAG_MATCH_QUEUE);
	} catch(const Exception&) {
	}
}
void UserInfoBase::getList() {
	try {
		QueueManager::getInstance()->addList(user, QueueItem::FLAG_CLIENT_VIEW);
	} catch(const Exception&) {
	}
}
void UserInfoBase::checkList() {
	try {
		QueueManager::getInstance()->addList(user, 0);
	} catch(const Exception&) {
	}
}
void UserInfoBase::addFav() {
	HubManager::getInstance()->addFavoriteUser(user);
}
void UserInfoBase::pm() {
	PrivateFrame::openWindow(user);
}
void UserInfoBase::grant() {
	UploadManager::getInstance()->reserveSlot(user);
}
void UserInfoBase::removeAll() {
	QueueManager::getInstance()->removeSources(user, QueueItem::Source::FLAG_REMOVED);
}
void UserInfoBase::grantSlotHour() {
	UploadManager::getInstance()->reserveSlotHour(user);
}
void UserInfoBase::grantSlotDay() {
	UploadManager::getInstance()->reserveSlotDay(user);
}
void UserInfoBase::grantSlotWeek() {
	UploadManager::getInstance()->reserveSlotWeek(user);
}
void UserInfoBase::ungrantSlot() {
	UploadManager::getInstance()->unreserveSlot(user);
}

bool WinUtil::getVersionInfo(OSVERSIONINFOEX& ver) {
	memset(&ver, 0, sizeof(OSVERSIONINFOEX));
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if(!GetVersionEx((OSVERSIONINFO*)&ver)) {
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if(!GetVersionEx((OSVERSIONINFO*)&ver)) {
			return false;
		}
	}
	return true;
}

static LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
	if(code == HC_ACTION) {
		if(wParam == VK_CONTROL && LOWORD(lParam) == 1) {
			if(lParam & 0x80000000) {
				WinUtil::tabCtrl->endSwitch();
			} else {
				WinUtil::tabCtrl->startSwitch();
			}
		}
	}
	return CallNextHookEx(WinUtil::hook, code, wParam, lParam);
}

void createImageList1(CImageList &imglst, string file) {
	HBITMAP hBitmap = (HBITMAP)::LoadImage(
		NULL, file.c_str(), IMAGE_BITMAP,
		0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
	
	if( !hBitmap )
		return;
    CBitmap b;
	b.Attach(hBitmap);
	
	imglst.Create(16, 16, ILC_MASK | ILC_COLOR32, 0, 0);
	imglst.Add(b, RGB(255,0,255)); 
}

void WinUtil::init(HWND hWnd) {
	mainWnd = hWnd;

	mainMenu.CreateMenu();

	CMenuHandle file;
	file.CreatePopupMenu();

	file.AppendMenu(MF_STRING, IDC_OPEN_FILE_LIST, CSTRING(MENU_OPEN_FILE_LIST));
	file.AppendMenu(MF_STRING, IDC_REFRESH_FILE_LIST, CSTRING(MENU_REFRESH_FILE_LIST));
	file.AppendMenu(MF_STRING, IDC_OPEN_DOWNLOADS, CSTRING(MENU_OPEN_DOWNLOADS_DIR));
	file.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	file.AppendMenu(MF_STRING, IDC_FOLLOW, CSTRING(MENU_FOLLOW_REDIRECT));
	file.AppendMenu(MF_STRING, ID_FILE_RECONNECT, CSTRING(MENU_RECONNECT));
	file.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	file.AppendMenu(MF_STRING, IDC_IMPORT_QUEUE, CSTRING(MENU_IMPORT_QUEUE));
	file.AppendMenu(MF_STRING, ID_FILE_SETTINGS, CSTRING(MENU_SETTINGS));
	file.AppendMenu(MF_STRING, ID_GET_TTH, CSTRING(MENU_TTH));
	file.AppendMenu(MF_STRING, IDC_UPDATE, CSTRING(UPDATE_CHECK));
	file.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	file.AppendMenu(MF_STRING, ID_APP_EXIT, CSTRING(MENU_EXIT));

	mainMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)file, CSTRING(MENU_FILE));

	CMenuHandle view;
	view.CreatePopupMenu();

	view.AppendMenu(MF_STRING, ID_FILE_CONNECT, CSTRING(MENU_PUBLIC_HUBS));
	view.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	view.AppendMenu(MF_STRING, IDC_RECENTS, CSTRING(MENU_FILE_RECENT_HUBS));
	view.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	view.AppendMenu(MF_STRING, IDC_FAVORITES, CSTRING(MENU_FAVORITE_HUBS));
	view.AppendMenu(MF_STRING, IDC_FAVUSERS, CSTRING(MENU_FAVORITE_USERS));
	view.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	view.AppendMenu(MF_STRING, ID_FILE_SEARCH, CSTRING(MENU_SEARCH));
	view.AppendMenu(MF_STRING, IDC_FILE_ADL_SEARCH, CSTRING(MENU_ADL_SEARCH));
	view.AppendMenu(MF_STRING, IDC_SEARCH_SPY, CSTRING(MENU_SEARCH_SPY));
	view.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	view.AppendMenu(MF_STRING, IDC_CDMDEBUG_WINDOW, CSTRING(MENU_CDMDEBUG_MESSAGES));
	view.AppendMenu(MF_STRING, IDC_NOTEPAD, CSTRING(MENU_NOTEPAD));
	view.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	view.AppendMenu(MF_STRING, ID_VIEW_TOOLBAR, CSTRING(MENU_TOOLBAR));
	view.AppendMenu(MF_STRING, ID_VIEW_STATUS_BAR, CSTRING(MENU_STATUS_BAR));
	view.AppendMenu(MF_STRING, ID_VIEW_TRANSFERS, CSTRING(MENU_TRANSFER_VIEW));

	mainMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)view, CSTRING(MENU_VIEW));

	CMenuHandle transfers;
	transfers.CreatePopupMenu();

	transfers.AppendMenu(MF_STRING, IDC_QUEUE, CSTRING(MENU_DOWNLOAD_QUEUE));
	transfers.AppendMenu(MF_STRING, IDC_FINISHED, CSTRING(FINISHED_DOWNLOADS));
	transfers.AppendMenu(MF_STRING, IDC_FINISHEDMP3, CSTRING(FINISHED_MP3_DOWNLOADS));
	transfers.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	transfers.AppendMenu(MF_STRING, IDC_UPLOAD_QUEUE, CSTRING(UPLOAD_QUEUE));
	transfers.AppendMenu(MF_STRING, IDC_FINISHED_UL, CSTRING(FINISHED_UPLOADS));
	transfers.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	transfers.AppendMenu(MF_STRING, IDC_NET_STATS, CSTRING(MENU_NETWORK_STATISTICS));

	mainMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)transfers, CSTRING(MENU_TRANSFER_VIEW));

	CMenuHandle window;
	window.CreatePopupMenu();

	window.AppendMenu(MF_STRING, ID_WINDOW_CASCADE, CSTRING(MENU_CASCADE));
	window.AppendMenu(MF_STRING, ID_WINDOW_TILE_HORZ, CSTRING(MENU_HORIZONTAL_TILE));
	window.AppendMenu(MF_STRING, ID_WINDOW_TILE_VERT, CSTRING(MENU_VERTICAL_TILE));
	window.AppendMenu(MF_STRING, ID_WINDOW_ARRANGE, CSTRING(MENU_ARRANGE));
	window.AppendMenu(MF_STRING, ID_WINDOW_MINIMIZE_ALL, CSTRING(MENU_MINIMIZE_ALL));
	window.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	window.AppendMenu(MF_STRING, IDC_CLOSE_DISCONNECTED, CSTRING(MENU_CLOSE_DISCONNECTED));

	mainMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)window, CSTRING(MENU_WINDOW));

	CMenuHandle help;
	help.CreatePopupMenu();

	help.AppendMenu(MF_STRING, ID_APP_ABOUT, CSTRING(MENU_ABOUT));
	help.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	help.AppendMenu(MF_STRING, IDC_HELP_HOMEPAGE, CSTRING(MENU_HOMEPAGE));
	help.AppendMenu(MF_STRING, IDC_HELP_DISCUSS, CSTRING(MENU_DISCUSS));

	mainMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)help, CSTRING(MENU_HELP));

	if(BOOLSETTING(USE_SYSTEM_ICONS)) {
		SHFILEINFO fi;
		memset(&fi, 0, sizeof(SHFILEINFO));
		fileImages.Create(16, 16, ILC_COLOR32 | ILC_MASK, 16, 16);
		::SHGetFileInfo(".", FILE_ATTRIBUTE_DIRECTORY, &fi, sizeof(fi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		fileImages.AddIcon(fi.hIcon);
		::DestroyIcon(fi.hIcon);
		dirIconIndex = fileImageCount++;	
	} else {
		fileImages.CreateFromImage(IDB_FOLDERS, 16, 3, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
		dirIconIndex = 0;
	}

	if(SETTING(USERLIST_IMAGE) == "")
		userImages.CreateFromImage(IDB_USERS, 16, 9, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	else 
		createImageList1(userImages, SETTING(USERLIST_IMAGE));

	LOGFONT lf, lf2;
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
	SettingsManager::getInstance()->setDefault(SettingsManager::TEXT_FONT, encodeFont(lf));
	decodeFont(SETTING(TEXT_FONT), lf);
	::GetObject((HFONT)GetStockObject(ANSI_FIXED_FONT), sizeof(lf2), &lf2);
	
	lf2.lfHeight = lf.lfHeight;
	lf2.lfWeight = lf.lfWeight;
	lf2.lfItalic = lf.lfItalic;

	bgBrush = CreateSolidBrush(SETTING(BACKGROUND_COLOR));
	textColor = SETTING(TEXT_COLOR);
	bgColor = SETTING(BACKGROUND_COLOR);
	font = ::CreateFontIndirect(&lf);
	fontHeight = WinUtil::getTextHeight(mainWnd, font);
	lf.lfWeight = FW_BOLD;
	boldFont = ::CreateFontIndirect(&lf);
	LONG _lfh = lf.lfHeight;
	lf.lfHeight *= 5;
	lf.lfHeight /= 6;
	smallBoldFont = ::CreateFontIndirect(&lf);
	lf.lfHeight = _lfh;
	lf.lfHeight *= 3;
	lf.lfHeight /= 4;
	tinyFont = ::CreateFontIndirect(&lf);
	systemFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	monoFont = (HFONT)::GetStockObject(BOOLSETTING(USE_OEM_MONOFONT)?OEM_FIXED_FONT:ANSI_FIXED_FONT);

	hook = SetWindowsHookEx(WH_KEYBOARD, &KeyboardProc, NULL, GetCurrentThreadId());
	
	grantMenu.CreatePopupMenu();
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CSTRING(GRANT_EXTRA_SLOT));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_HOUR, CSTRING(GRANT_EXTRA_SLOT_HOUR));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_DAY, CSTRING(GRANT_EXTRA_SLOT_DAY));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_WEEK, CSTRING(GRANT_EXTRA_SLOT_WEEK));
	grantMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	grantMenu.AppendMenu(MF_STRING, IDC_UNGRANTSLOT, CSTRING(REMOVE_EXTRA_SLOT));
}

void WinUtil::uninit() {
	fileImages.Destroy();
	userImages.Destroy();
	::DeleteObject(font);
	::DeleteObject(boldFont);
	::DeleteObject(smallBoldFont);
	::DeleteObject(tinyFont);
	::DeleteObject(bgBrush);

	mainMenu.DestroyMenu();
	grantMenu.DestroyMenu();

	UnhookWindowsHookEx(hook);
	
}

void WinUtil::decodeFont(const string& setting, LOGFONT &dest) {
	StringTokenizer st(setting, ',');
	StringList &sl = st.getTokens();
	
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(dest), &dest);
	string face;
	if(sl.size() == 4)
	{
		face = sl[0];
		dest.lfHeight = Util::toInt(sl[1]);
		dest.lfWeight = Util::toInt(sl[2]);
		dest.lfItalic = (BYTE)Util::toInt(sl[3]);
	}
	
	if(!face.empty()) {
		::ZeroMemory(dest.lfFaceName, LF_FACESIZE);
		strcpy(dest.lfFaceName, face.c_str());
	}
}

int CALLBACK WinUtil::browseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /*lp*/, LPARAM pData) {
	switch(uMsg) {
	case BFFM_INITIALIZED: 
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
		break;
	}
	return 0;
}

bool WinUtil::browseDirectory(string& target, HWND owner /* = NULL */) {
	char buf[MAX_PATH];
	BROWSEINFO bi;
	LPMALLOC ma;
	
	ZeroMemory(&bi, sizeof(bi));
	
	bi.hwndOwner = owner;
	bi.pszDisplayName = buf;
	bi.lpszTitle = CSTRING(CHOOSE_FOLDER);
	bi.ulFlags = BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.lParam = (LPARAM)target.c_str();
	bi.lpfn = &browseCallbackProc;
	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if(pidl != NULL) {
		SHGetPathFromIDList(pidl, buf);
		target = buf;
		
		if(target.size() > 0 && target[target.size()-1] != '\\')
			target+='\\';
		
		if(SHGetMalloc(&ma) != E_FAIL) {
			ma->Free(pidl);
			ma->Release();
		}
		return true;
	}
	return false;
}

bool WinUtil::browseFile(string& target, HWND owner /* = NULL */, bool save /* = true */, const string& initialDir /* = Util::emptyString */, const char* types /* = NULL */, const char* defExt /* = NULL */) {
	char buf[MAX_PATH];
	OPENFILENAME ofn;       // common dialog box structure
	target = Util::validateFileName(target);
	memcpy(buf, target.c_str(), target.length() + 1);
	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = owner;
	ofn.lpstrFile = buf;
	ofn.lpstrFilter = types;
	ofn.lpstrDefExt = defExt;
	ofn.nFilterIndex = 1;

	if(!initialDir.empty()) {
		ofn.lpstrInitialDir = initialDir.c_str();
	}
	ofn.nMaxFile = sizeof(buf);
	ofn.Flags = (save ? 0: OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST);
	
	// Display the Open dialog box. 
	if ( (save ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn) ) ==TRUE) {
		target = ofn.lpstrFile;
		return true;
	}
	return false;
}

void WinUtil::setClipboard(const string& str) {
	if(!::OpenClipboard(mainWnd)) {
		return;
	}

	EmptyClipboard();

	// Allocate a global memory object for the text. 
	HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (str.size() + 1)); 
	if (hglbCopy == NULL) { 
		CloseClipboard(); 
		return; 
	} 

	// Lock the handle and copy the text to the buffer. 
	char* lptstrCopy = (char*)GlobalLock(hglbCopy); 
	memcpy(lptstrCopy, str.c_str(), str.length() + 1);
	GlobalUnlock(hglbCopy); 

	// Place the handle on the clipboard. 
	SetClipboardData(CF_TEXT, hglbCopy); 
	CloseClipboard();
}

void WinUtil::splitTokens(int* array, const string& tokens, int maxItems /* = -1 */) throw() {
	StringTokenizer t(tokens, ',');
	StringList& l = t.getTokens();
	if(maxItems == -1)
		maxItems = l.size();
	
	int k = 0;
	for(StringList::const_iterator i = l.begin(); i != l.end() && k < maxItems; ++i, ++k) {
		array[k] = Util::toInt(*i);
	}
}

bool WinUtil::getUCParams(HWND parent, const UserCommand& uc, StringMap& sm) throw() {
	string::size_type i = 0;
	StringMap done;

	while( (i = safestring::SafeFind(uc.getCommand(),"%[line:", i)) != string::npos) {
		i += 7;
		string::size_type j = safestring::SafeFind(uc.getCommand(),']', i);
		if(j == string::npos)
			break;

		string name = uc.getCommand().substr(i, j-i);
		if(done.find(name) == done.end()) {
			LineDlg dlg;
			dlg.title = uc.getName();
			dlg.description = name;
			dlg.line = sm["line:" + name];
			if(dlg.DoModal(parent) == IDOK) {
				sm["line:" + name] = Util::validateMessage(dlg.line, false);
					done[name] = dlg.line;
			} else {
				return false;
			}
		}
		i = j + 1;
	}
	i = 0;
	while( (i = safestring::SafeFind(uc.getCommand(),"%[kickline:", i)) != string::npos) {
		i += 11;
		string::size_type j = safestring::SafeFind(uc.getCommand(),']', i);
		if(j == string::npos)
			break;

		string name = uc.getCommand().substr(i, j-i);
		if(done.find(name) == done.end()) {
			KickDlg dlg;
			dlg.title = uc.getName();
			dlg.description = name;
			if(dlg.DoModal(parent) == IDOK) {
				sm["kickline:" + name] = Util::validateMessage(dlg.line, false);
					done[name] = dlg.line;
			} else {
				return false;
			}
		}
		i = j + 1;
	}
	return true;
}

#define LINE2 "-- http://dcplusplus.sourceforge.net  <DC++ " DCVERSIONSTRING ">"
char *msgs[] = { "\r\n-- I'm a happy dc++ user. You could be happy too.\r\n" LINE2,
"\r\n-- Neo-...what? Nope...never heard of it...\r\n" LINE2,
"\r\n-- Evolution of species: Ape --> Man\r\n-- Evolution of science: \"The Earth is Flat\" --> \"The Earth is Round\"\r\n-- Evolution of sharing: NMDC --> DC++\r\n" LINE2,
"\r\n-- I share, therefore I am.\r\n" LINE2,
"\r\n-- I came, I searched, I found...\r\n" LINE2,
"\r\n-- I came, I shared, I sent...\r\n" LINE2,
"\r\n-- I can set away mode, can't you?\r\n" LINE2,
"\r\n-- I don't have to see any ads, do you?\r\n" LINE2,
"\r\n-- I don't have to see those annoying kick messages, do you?\r\n" LINE2,
"\r\n-- I can resume my files to a different filename, can you?\r\n" LINE2,
"\r\n-- I can share huge amounts of files, can you?\r\n" LINE2,
"\r\n-- My client doesn't spam the chat with useless debug messages, does yours?\r\n" LINE2,
"\r\n-- I can add multiple users to the same download and have the client connect to another automatically when one goes offline, can you?\r\n" LINE2,
"\r\n-- These addies are pretty annoying, aren't they? Get revenge by sending them yourself!\r\n" LINE2,
"\r\n-- My client supports TTH hashes, does yours?\r\n" LINE2,
"\r\n-- My client supports XML file lists, does yours?\r\n" LINE2
};

#define MSGS 16

#define LINE3 "-- http://snail.pc.cz/StrongDC  <StrongDC++ " VERSIONSTRING "" CZDCVERSIONSTRING">"
char *strgmsgs[] = { "\r\n-- To mrne je docela sikovny ale porad ho je co ucit :-)\r\n" LINE3,
"\r\n-- Je to pekny ale bude jeste hezci :-)\r\n" LINE3,
"\r\n-- Muzu omezit rychlost sveho downloadu aby mi zbyla linka pro brouzdani na webu :-D\r\n" LINE3,
"\r\n-- Nepodporuju klienty bez TTH proto jim nedam extra slot na filelist ;)\r\n" LINE3,
"\r\n-- Umim stahovat jeden soubor od vice lidi najednou, a proto dokazu vyuzit linku na maximum :-))\r\n" LINE3,
"\r\n-- Dokazu seskupovat vysledky hledani se stejnym TTH pod jednu polozku ;)\r\n" LINE3,
"\r\n-- Nedovolim michat soubory s TTH a bez TTH a predejit tak poskozeni souboru :-)\r\n" LINE3,
"\r\n-- Po stazeni souboru zkontroluju TTH, abych zjistil jestli je soubor v poradku :-D\r\n" LINE3,
};

#define STRGMSGS 8

#define LINE4 "-- http://czdcplusplus.no-ip.org  <CZDC++ " DCVERSIONSTRING "[X]>"
char *czmsgs[] = { "\r\n-- To mrne je docela sikovny ale porad ho je co ucit :-)\r\n" LINE4,
"\r\n-- Nekdo ma a nekdo nema....ja mam :-D\r\n" LINE4,
"\r\n-- Je to sice porad slusnej buglist ale proti original DC++ ci neo-modusu je to zlaty :o)\r\n" LINE4,
"\r\n-- Jsem prilis sexy pro normalni dc klienty :)\r\n" LINE4,
"\r\n-- Nepodporuju klienty bez TTH proto jim nedam extra slot na filelist ;)\r\n" LINE4,
};

#define CZMSGS 9

string WinUtil::commands = "\t\t\t\t\t HELP \t\t\t\t\t\t\t\t\n------------------------------------------------------------------------------------------------------------------------------------------------------------\n/refresh \t\t\t\t(obnoveni share) \t\t\t\t\t\t\n/savequeue \t\t\t\t(ulozi Download Queue) \t\t\t\t\t\t\n------------------------------------------------------------------------------------------------------------------------------------------------------------\n/search <string> \t\t\t(hledat neco...) \t\t\t\t\t\t\t\n/g <searchstring> \t\t\t(hledat Googlem) \t\t\t\t\t\t\n/imdb <imdbquery> \t\t\t(hledat film v IMDB databazi) \t\t\t\t\t\n/whois [IP] \t\t\t\t(hledat podrobnosti o IP) \t\t\t\t\t\n------------------------------------------------------------------------------------------------------------------------------------------------------------\n/slots # \t\t\t\t(upload sloty) \t\t\t\t\t\t\t\n/extraslots # \t\t\t\t(extra sloty pro male soubory) \t\t\t\t\t\n/smallfilesize # \t\t\t\t(maximalni velikost malych souboru) \t\t\t\t\n/ts \t\t\t\t\t(zobrazi datum a cas u zprav v mainchatu) \t\t\t\n/connection \t\t\t\t(zobrazi IP a port prez ktery jste pripojen) \t\t\t\t\n/showjoins \t\t\t\t(zapne/vypne zobrazovani a odpojovani useru v mainchatu) \t\n/showblockedipports \t\t\t(zobrazi zablokovane porty-mozna:)) \t\t\t\t\n/shutdown \t\t\t\t(vypne pocitac po urcitem timeoutu) \t\t\t\t\n------------------------------------------------------------------------------------------------------------------------------------------------------------\n/dc++ \t\t\t\t\t(zobrazi verzi DC++ v mainchatu) \t\t\t\t\t\n/czdc++ \t\t\t\t(zobrazi verzi CZDC++ v mainchatu) \t\t\t\t\n/strongdc++ \t\t\t\t(zobrazi verzi StrongDC++ v mainchatu) \t\t\t\t\n------------------------------------------------------------------------------------------------------------------------------------------------------------\n/away <msg> \t\t\t\t(zapne/vypne away mod) \t\t\t\t\t\n/winamp \t\t\t\t(Winamp spam v mainchatu) \t\t\t\t\t\n/w \t\t\t\t\t(Winamp spam v mainchatu) \t\t\t\t\t\n/clear,/c \t\t\t\t(smazat obsah mainchatu) \t\t\t\t\t\n/ignorelist \t\t\t\t(zobrazi ignorelist v mainchatu) \t\t\t\t\t\n";


bool WinUtil::checkCommand(string& cmd, string& param, string& message, string& status) {
	string::size_type i = cmd.find(' ');
	if(i != string::npos) {
		param = cmd.substr(i+1);
		cmd = cmd.substr(1, i - 1);
	} else {
		cmd = cmd.substr(1);
	}

	if(Util::stricmp(cmd.c_str(), "refresh")==0) {
		try {
			ShareManager::getInstance()->setDirty();
			ShareManager::getInstance()->refresh(true);
		} catch(const ShareException& e) {
			status = e.getError();
		}
	} else if(Util::stricmp(cmd.c_str(), "slots")==0) {
		int j = Util::toInt(param);
		if(j > 0) {
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, j);
			status = STRING(SLOTS_SET);
			ClientManager::getInstance()->infoUpdated(false);
		} else {
			status = STRING(INVALID_NUMBER_OF_SLOTS);
		}
	} else if(Util::stricmp(cmd.c_str(), "search") == 0) {
		if(!param.empty()) {
			SearchFrame::openWindow(param);
		} else {
			status = STRING(SPECIFY_SEARCH_STRING);
		}
	} else if(Util::stricmp(cmd.c_str(), "dc++") == 0) {
		message = msgs[GET_TICK() % MSGS];
	} else if(Util::stricmp(cmd.c_str(), "czdc++") == 0) {
		message = czmsgs[GET_TICK() % CZMSGS];
	} else if(Util::stricmp(cmd.c_str(), "strongdc++") == 0) {
		message = strgmsgs[GET_TICK() % STRGMSGS];
	} else if(Util::stricmp(cmd.c_str(), "away") == 0) {
		if(Util::getAway() && param.empty()) {
			Util::setAway(false);
			MainFrame::setAwayButton(false);
			status = STRING(AWAY_MODE_OFF);
		} else {
			Util::setAway(true);
			MainFrame::setAwayButton(true);
			Util::setAwayMessage(param);
			status = STRING(AWAY_MODE_ON) + Util::getAwayMessage();
		}
		ClientManager::getInstance()->infoUpdated(true);
	} else if(Util::stricmp(cmd.c_str(), "g") == 0) {
		if(param.empty()) {
			status = STRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink("http://www.google.com/search?q="+param);
		}
	} else if(Util::stricmp(cmd.c_str(), "imdb") == 0) {
		if(param.empty()) {
			status = STRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink("http://www.imdb.com/find?q="+param);
		}
	 	} else if(Util::stricmp(cmd.c_str(), "rebuild") == 0) {
		HashManager::getInstance()->rebuild();
		status = STRING(HASH_REBUILT);
	} else if(Util::stricmp(cmd.c_str(), "shutdown") == 0) {
		MainFrame::setShutDown(!(MainFrame::getShutDown()));
		if (MainFrame::getShutDown()) {
			status = STRING(SHUTDOWN_ON);
		} else {
			status = STRING(SHUTDOWN_OFF);
			}
		} else if((Util::stricmp(cmd.c_str(), "winamp") == 0) || (Util::stricmp(cmd.c_str(), "w") == 0)) {
		HWND hwndWinamp = FindWindow("Winamp v1.x", NULL);
		if (hwndWinamp) {
			StringMap params;
			int waVersion = SendMessage(hwndWinamp,WM_USER, 0, IPC_GETVERSION),
				majorVersion, minorVersion;
			majorVersion = waVersion >> 12;
			if (((waVersion & 0x00F0) >> 4) == 0) {
				minorVersion = ((waVersion & 0x0f00) >> 8) * 10 + (waVersion & 0x000f);
			} else {
				minorVersion = ((waVersion & 0x00f0) >> 4) * 10 + (waVersion & 0x000f);
			}
			params["version"] = Util::toString(majorVersion + minorVersion / 100.0);
			int state = SendMessage(hwndWinamp,WM_USER, 0, IPC_ISPLAYING);
			switch (state) {
				case 0: params["state"] = "stopped";
					break;
				case 1: params["state"] = "playing";
					break;
				case 3: params["state"] = "paused";
			};
			char titleBuffer[2048];
			int buffLength = sizeof(titleBuffer);
			GetWindowText(hwndWinamp, titleBuffer, buffLength);
			string title = titleBuffer;
			params["rawtitle"] = title;
			// fix the title if scrolling is on, so need to put the stairs to the end of it
			string titletmp=title.substr(title.find("***")+2, title.size());
			title = titletmp + title.substr(0, title.size()-titletmp.size());
			title = title.substr(title.find(".")+2, title.size());
			if (title.rfind("-") != string::npos) {
				params["title"] = title.substr(0, title.rfind("-")-1);
			}
			int curPos = SendMessage(hwndWinamp,WM_USER, 0, IPC_GETOUTPUTTIME);
			int length = SendMessage(hwndWinamp,WM_USER, 1, IPC_GETOUTPUTTIME);
			if (curPos == -1) {
				curPos = 0;
			} else {
				curPos /= 1000;
			}
			int intPercent;
			if (length > 0 ) {
				intPercent = curPos * 100 / length;
			} else {
				length = 0;
				intPercent = 0;
			}
			params["percent"] = Util::toString(intPercent) + "%";
			params["elapsed"] = Util::formatSeconds(curPos, true);
			params["length"] = Util::formatSeconds(length, true);
			int numFront = min(max(intPercent / 10, 0), 10),
				numBack = min(max(10 - 1 - numFront, 0), 10);
			string inFront = string(numFront, '-'),
				inBack = string(numBack, '-');
			params["bar"] = "[" + inFront + (state ? "|" : "-") + inBack + "]";
			int waSampleRate = SendMessage(hwndWinamp,WM_USER, 0, IPC_GETINFO),
				waBitRate = SendMessage(hwndWinamp,WM_USER, 1, IPC_GETINFO),
				waChannels = SendMessage(hwndWinamp,WM_USER, 2, IPC_GETINFO);
			params["bitrate"] = Util::toString(waBitRate) + "kbps";
			params["sample"] = Util::toString(waSampleRate) + "kHz";
			params["channels"] = (waChannels==2?"stereo":"mono"); // 3+ channels? 0 channels?
			message = Util::formatParams(SETTING(WINAMP_FORMAT), params);
		} else {
			status = "Supported version of Winamp is not running";
		}
	} else if(Util::stricmp(cmd.c_str(), "uptime") == 0) {
		char buf[128];
		sprintf(buf, "CZDC++ %s[%s] Uptime: ",VERSIONSTRING, CZDCVERSIONSTRING);
		string uptime = (string)buf;
		long n, rest, i;
		rest = Util::getUptime();
		i = 0;
		n = rest / (24*3600*7);
		rest %= (24*3600*7);
		if(n) {
			sprintf(buf, "%d weeks ", n); 
			uptime += (string)buf;
			i++;
		}
		n = rest / (24*3600);
		rest %= (24*3600);
		if(n) {
			sprintf(buf, "%d days ", n); 
			uptime += (string)buf;
			i++;
		}
		n = rest / (3600);
		rest %= (3600);
		if(n) {
			sprintf(buf, "%d hours ", n); 
			uptime += (string)buf;
			i++;
		}
		n = rest / (60);
		rest %= (60);
		if(n) {
			sprintf(buf, "%d min ", n); 
			uptime += (string)buf;
			i++;
		}
		n = rest;
		if(++i <= 3) {
			sprintf(buf, "%d sec ", n); 
			uptime += (string)buf;
		}
		uptime += "@ D: " + Util::formatBytes(Socket::getTotalDown()) + 
			" " + "U: " + Util::formatBytes(Socket::getTotalUp());
		message = uptime;
	} else if(Util::stricmp(cmd.c_str(), "tvtome") == 0) {
		if(param.empty()) {
			status = STRING(SPECIFY_SEARCH_STRING);
		} else
			WinUtil::openLink("http://www.tvtome.com/tvtome/servlet/Search?searchType=all&searchString="+param);
	} else {
		return false;
	}

	return true;
}

void WinUtil::openLink(const string& url) {
	CRegKey key;
	char regbuf[MAX_PATH];
	ULONG len = MAX_PATH;
	if(key.Open(HKEY_CLASSES_ROOT, "http\\shell\\open\\command", KEY_READ) == ERROR_SUCCESS) {
		if(key.QueryStringValue(NULL, regbuf, &len) == ERROR_SUCCESS) {
			/*
			 * Various values:
			 *  C:\PROGRA~1\MOZILL~1\FIREFOX.EXE -url "%1"
			 *  "C:\Program Files\Internet Explorer\iexplore.exe" -nohome
			 *  "C:\Apps\Opera7\opera.exe"
			 *  C:\PROGRAMY\MOZILLA\MOZILLA.EXE -url "%1"
			 */
			string cmd(regbuf); // otherwise you consistently get two trailing nulls
			
			if(!cmd.empty() && cmd.length() > 1) {
				string::size_type start,end;
				if(cmd[0] == '"') {
					start = 1;
					end = safestring::SafeFind(cmd, '"', 1);
				} else {
					start = 0;
					end = safestring::SafeFind(cmd, ' ', 1);
				}
				if(end == string::npos)
					end = cmd.length();

				string cmdLine(cmd);
				cmd = cmd.substr(start, end-start);
				size_t arg_pos;
				if((arg_pos = cmdLine.find("%1")) != string::npos) {
					cmdLine.replace(arg_pos, 2, url);
				} else {
					cmdLine.append(" \"" + url + '\"');
				}

				STARTUPINFO si = { sizeof(si), 0 };
				PROCESS_INFORMATION pi = { 0 };
				AutoArray<char> buf(cmdLine.length() + 1);
				strcpy(buf, cmdLine.c_str());
				if(::CreateProcess(cmd.c_str(), buf, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
					::CloseHandle(pi.hThread);
					::CloseHandle(pi.hProcess);
					return;
				}
			}
		}
	}

	::ShellExecute(NULL, NULL, url.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void WinUtil::saveHeaderOrder(CListViewCtrl& ctrl, SettingsManager::StrSetting order, 
							  SettingsManager::StrSetting widths, int n, 
							  int* indexes, int* sizes) throw() {
	string tmp;

	ctrl.GetColumnOrderArray(n, indexes);
	int i;
	for(i = 0; i < n; ++i) {
		tmp += Util::toString(indexes[i]);
		tmp += ',';
	}
	tmp.erase(tmp.size()-1, 1);
	SettingsManager::getInstance()->set(order, tmp);
	tmp.clear();
	int nHeaderItemsCount = ctrl.GetHeader().GetItemCount();
	for(i = 0; i < n; ++i) {
		sizes[i] = ctrl.GetColumnWidth(i);
		if (i >= nHeaderItemsCount) // Not exist column
			sizes[i] = 0;
		tmp += Util::toString(sizes[i]);
		tmp += ',';
	}
	tmp.erase(tmp.size()-1, 1);
	SettingsManager::getInstance()->set(widths, tmp);
}

int WinUtil::getIconIndex(const string& aFileName) {
	if(BOOLSETTING(USE_SYSTEM_ICONS)) {
		SHFILEINFO fi;
		string x = Util::toLower(Util::getFileExt(aFileName));
		if(!x.empty()) {
			ImageIter j = fileIndexes.find(x);
			if(j != fileIndexes.end())
				return j->second;
		}
		string fn = Util::toLower(Util::getFileName(aFileName));

		::SHGetFileInfo(fn.c_str(), FILE_ATTRIBUTE_NORMAL, &fi, sizeof(fi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		fileImages.AddIcon(fi.hIcon);
		::DestroyIcon(fi.hIcon);

		fileIndexes[x] = fileImageCount++;
		return fileImageCount - 1;
	} else {
		return 2;
	}
}

void WinUtil::ClearPreviewMenu(CMenu &previewMenu){
	while(previewMenu.GetMenuItemCount() > 0) {
		previewMenu.RemoveMenu(0, MF_BYPOSITION);
	}
}

int WinUtil::SetupPreviewMenu(CMenu &previewMenu, string extension){
	int PreviewAppsSize = 0;
	PreviewApplication::List lst = PluginManager::getInstance()->getPreviewApps();
	if(lst.size()>0){		
		PreviewAppsSize = 0;
		for(PreviewApplication::Iter i = lst.begin(); i != lst.end(); i++){
			StringList tok = StringTokenizer((*i)->getExtension(), ';').getTokens();
			bool add = false;
			if(tok.size()==0)add = true;

			
			for(StringIter si = tok.begin(); si != tok.end(); ++si) {
				if(stricmp(extension.c_str(), si->c_str())==0){
					add = true;
					break;
				}
			}
							
			if (add) previewMenu.AppendMenu(MF_STRING, IDC_PREVIEW_APP + PreviewAppsSize, ((*i)->getName()).c_str());
			PreviewAppsSize++;
		}
	}
	return PreviewAppsSize;
}

void WinUtil::RunPreviewCommand(int index, string target){
	PreviewApplication::List lst = PluginManager::getInstance()->getPreviewApps();

	if(index <= lst.size()) {
	string application = lst[index]->getApplication();
	string arguments = lst[index]->getArguments();
	StringMap ucParams;				
	
	ucParams["file"] = "\"" + target + "\"";
	ucParams["dir"] = "\"" + Util::getFilePath(target) + "\"";

	::ShellExecute(NULL, NULL, application.c_str(), Util::formatParams(arguments, ucParams).c_str(), ucParams["dir"].c_str(), SW_SHOWNORMAL);
	}
}

