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

#ifndef __WINUTIL_H
#define __WINUTIL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/Util.h"
#include "../client/SettingsManager.h"
#include "../client/User.h"
#include "../client/ResourceManager.h"

#include "resource.h"
#include "CZDCLib.h"


// Some utilities for handling HLS colors, taken from Jean-Michel LE FOL's codeproject
// article on WTL OfficeXP Menus
typedef DWORD HLSCOLOR;
#define HLS(h,l,s) ((HLSCOLOR)(((BYTE)(h)|((WORD)((BYTE)(l))<<8))|(((DWORD)(BYTE)(s))<<16)))
#define HLS_H(hls) ((BYTE)(hls))
#define HLS_L(hls) ((BYTE)(((WORD)(hls)) >> 8))
#define HLS_S(hls) ((BYTE)((hls)>>16))

HLSCOLOR RGB2HLS (COLORREF rgb);
COLORREF HLS2RGB (HLSCOLOR hls);

COLORREF HLS_TRANSFORM (COLORREF rgb, int percent_L, int percent_S);

class UserInfoBase {
public:
	UserInfoBase(const User::Ptr& u) : user(u) { };
	
	void getList();
	void checkList();
	void matchQueue();
	void pm();
	void grant();
	void grantSlotHour();
	void grantSlotDay();
	void grantSlotWeek();
	void ungrantSlot();
	void addFav();
	void removeAll();

	User::Ptr& getUser() { return user; }
	User::Ptr user;
};

template<class T>
class UserInfoBaseHandler {
public:
	UserInfoBaseHandler()
	{
		sSelectedUser = "";
	}

	BEGIN_MSG_MAP(UserInfoBaseHandler)
		COMMAND_ID_HANDLER(IDC_GETLIST, onGetList)
		COMMAND_ID_HANDLER(IDC_CHECKLIST, onCheckList)
		COMMAND_ID_HANDLER(IDC_MATCH_QUEUE, onMatchQueue)
		COMMAND_ID_HANDLER(IDC_PRIVATEMESSAGE, onPrivateMessage)
		COMMAND_ID_HANDLER(IDC_ADD_TO_FAVORITES, onAddToFavorites)
		COMMAND_ID_HANDLER(IDC_GRANTSLOT, onGrantSlot)
		COMMAND_ID_HANDLER(IDC_REMOVEALL, onRemoveAll)
		COMMAND_ID_HANDLER(IDC_GRANTSLOT_HOUR, onGrantSlotHour)
		COMMAND_ID_HANDLER(IDC_GRANTSLOT_DAY, onGrantSlotDay)
		COMMAND_ID_HANDLER(IDC_GRANTSLOT_WEEK, onGrantSlotWeek)
		COMMAND_ID_HANDLER(IDC_UNGRANTSLOT, onUnGrantSlot)
	END_MSG_MAP()

	LRESULT onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if ( sSelectedUser != "" ) {
			int nAtPos = ((T*)this)->getUserList().findItem( sSelectedUser );
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::matchQueue);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::matchQueue);
		}
		return 0;
	}
	LRESULT onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if ( sSelectedUser != "" ) {
			int nAtPos = ((T*)this)->getUserList().findItem( sSelectedUser );
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::getList);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::getList);
		}
		return 0;
	}

	LRESULT onCheckList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if ( sSelectedUser != "" ) {
			int nAtPos = ((T*)this)->getUserList().findItem( sSelectedUser );
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::checkList);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::checkList);
		}
		return 0;
	}

	LRESULT onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if ( sSelectedUser != "" ) {
			int nAtPos = ((T*)this)->getUserList().findItem( sSelectedUser );
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::addFav);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::addFav);
		}
		return 0;
	}
	LRESULT onPrivateMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if ( sSelectedUser != "" ) {
			int nAtPos = ((T*)this)->getUserList().findItem( sSelectedUser );
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::pm);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::pm);
		}		
		return 0;
	}
	LRESULT onGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) { 
		if ( sSelectedUser != "" ) {
			int nAtPos = ((T*)this)->getUserList().findItem( sSelectedUser );
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::grant);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::grant);
		}
		return 0;
	}
	LRESULT onRemoveAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) { 
		if ( sSelectedUser != "" ) {
			int nAtPos = ((T*)this)->getUserList().findItem( sSelectedUser );
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::removeAll);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::removeAll);
		}
		return 0;
	}
	LRESULT onGrantSlotHour(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){ 
		if ( sSelectedUser != "" ) {
			int nAtPos = ((T*)this)->getUserList().findItem( sSelectedUser );
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::grantSlotHour);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::grantSlotHour);
		}
		return 0;
	}
	LRESULT onGrantSlotDay(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){ 
	if ( sSelectedUser != "" ) {
			int nAtPos = ((T*)this)->getUserList().findItem( sSelectedUser );
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::grantSlotDay);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::grantSlotDay);
		}
		return 0;
	}
	LRESULT onGrantSlotWeek(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){ 
		if ( sSelectedUser != "" ) {
			int nAtPos = ((T*)this)->getUserList().findItem( sSelectedUser );
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::grantSlotWeek);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::grantSlotWeek);
		}
		return 0;
	}

	LRESULT onUnGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){ 
	if ( sSelectedUser != "" ) {
			int nAtPos = ((T*)this)->getUserList().findItem( sSelectedUser );
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::ungrantSlot );
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::ungrantSlot );
		}
		return 0;
	}

	void appendUserItems(CMenu& menu) {
		sSelectedUser = "";

		menu.AppendMenu(MF_STRING, IDC_GETLIST, CSTRING(GET_FILE_LIST));
		menu.AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CSTRING(MATCH_QUEUE));
		menu.AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CSTRING(SEND_PRIVATE_MESSAGE));
		menu.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CSTRING(ADD_TO_FAVORITES));
		menu.AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::grantMenu, CSTRING(GRANT_SLOTS_MENU));
		menu.AppendMenu(MF_STRING, IDC_REMOVEALL, CSTRING(REMOVE_FROM_ALL));
	}

protected:
	string sSelectedUser;
};

class FlatTabCtrl;
class UserCommand;

template<class T, int title, int ID = -1>
class StaticFrame {
public:
	~StaticFrame() { frame = NULL; };

	static T* frame;
	static void openWindow() {
		if(frame == NULL) {
			frame = new T();
			frame->CreateEx(WinUtil::mdiClient, frame->rcDefault, ResourceManager::getInstance()->getString(ResourceManager::Strings(title)).c_str());
			CZDCLib::setButtonPressed(ID, true);
		} else {
			HANDLE_MDI_CLICK(ID, T);
		}
	}
};

template<class T, int title, int ID>
T* StaticFrame<T, title, ID>::frame = NULL;

class WinUtil {
public:
	static CImageList fileImages;
	static int fileImageCount;
	static CImageList userImages;

	typedef HASH_MAP<string, int> ImageMap;
	typedef ImageMap::iterator ImageIter;

	struct TextItem {
		WORD itemID;
		ResourceManager::Strings translatedString;
	};

	static ImageMap fileIndexes;
	static HBRUSH bgBrush;
	static COLORREF textColor;
	static COLORREF bgColor;
	static HFONT font;
	static int fontHeight;
	static HFONT boldFont;
	static HFONT systemFont;
	static HFONT monoFont;
	static HFONT tinyFont;
	static HFONT smallBoldFont;
	static CMenu mainMenu;
	static CMenu grantMenu;
	static int dirIconIndex;
	static StringList lastDirs;
	static HWND mainWnd;
	static HWND mdiClient;
	static FlatTabCtrl* tabCtrl;
	static string commands;
	static HHOOK hook;
	static bool isPM;
	static bool isAppActive;
	static bool isMinimized;
	static bool trayIcon;

	static void init(HWND hWnd);
	static void uninit();

	static void decodeFont(const string& setting, LOGFONT &dest);

	/**
	 * Check if this is a common /-command.
	 * @param cmd The whole text string, will be updated to contain only the command.
	 * @param param Set to any parameters.
	 * @param message Message that should be sent to the chat.
	 * @param status Message that should be shown in the status line.
	 * @return True if the command was processed, false otherwise.
	 */
	static bool checkCommand(string& cmd, string& param, string& message, string& status);

	static int getTextWidth(const string& str, HWND hWnd) {
		HDC dc = ::GetDC(hWnd);
		int sz = getTextWidth(str, dc);
		::ReleaseDC(mainWnd, dc);
		return sz;
	}
	static int getTextWidth(const string& str, HDC dc) {
		SIZE sz = { 0, 0 };
		::GetTextExtentPoint32(dc, str.c_str(), str.length(), &sz);
		return sz.cx;		
	}

	static int getTextHeight(HWND wnd, HFONT fnt) {
		HDC dc = ::GetDC(wnd);
		int h = getTextHeight(dc, fnt);
		::ReleaseDC(wnd, dc);
		return h;
	}

	static int getTextHeight(HDC dc, HFONT fnt) {
		HGDIOBJ old = ::SelectObject(dc, fnt);
		int h = getTextHeight(dc);
		::SelectObject(dc, old);
		return h;
	}

	static int getTextHeight(HDC dc) {
		TEXTMETRIC tm;
		::GetTextMetrics(dc, &tm);
		return tm.tmHeight;
	}

	static void setClipboard(const string& str);

	static void addLastDir(const string& dir) {
		if(find(lastDirs.begin(), lastDirs.end(), dir) != lastDirs.end()) {
			return;
		}
		if(lastDirs.size() == 10) {
			lastDirs.erase(lastDirs.begin());
		}
		lastDirs.push_back(dir);
	}
	
	static string encodeFont(LOGFONT const& font)
	{
		string res(font.lfFaceName);
		res += ',';
		res += Util::toString(font.lfHeight);
		res += ',';
		res += Util::toString(font.lfWeight);
		res += ',';
		res += Util::toString(font.lfItalic);
		return res;
	}
	
	static bool browseFile(string& target, HWND owner = NULL, bool save = true, const string& initialDir = Util::emptyString, const char* types = NULL, const char* defExt = NULL);
	static bool browseDirectory(string& target, HWND owner = NULL);

	static void openLink(const string& url);
	static void openFile(const string& file) {
		::ShellExecute(NULL, NULL, file.c_str(), NULL, NULL, SW_SHOWNORMAL);
	}

	static int getIconIndex(const string& aFileName);

	static int getDirIconIndex() {
		return dirIconIndex;
	}
	
	static bool getUCParams(HWND parent, const UserCommand& cmd, StringMap& sm) throw();

	static void splitTokens(int* array, const string& tokens, int maxItems = -1) throw();
	static void saveHeaderOrder(CListViewCtrl& ctrl, SettingsManager::StrSetting order, 
		SettingsManager::StrSetting widths, int n, int* indexes, int* sizes) throw();

	static void ClearPreviewMenu(CMenu &previewMenu);
	static int SetupPreviewMenu(CMenu &previewMenu, string extension);
	static void RunPreviewCommand(int index, string target);
	static void translate(HWND page, TextItem* textItems) 
	{
		if (textItems != NULL) {
			for(int i = 0; textItems[i].itemID != 0; i++) {
				::SetDlgItemText(page, textItems[i].itemID,
					ResourceManager::getInstance()->getString(textItems[i].translatedString).c_str());
			}
		}
	}

	struct tbIDImage {
		int id, image;
		bool check;
		ResourceManager::Strings tooltip;
	};
	static tbIDImage ToolbarButtons[];

private:
	static int CALLBACK browseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /*lp*/, LPARAM pData);
};

#endif // __WINUTIL_H

/**
 * @file
 * $Id$
 */
