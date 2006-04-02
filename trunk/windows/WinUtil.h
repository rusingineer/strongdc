/*
 * Copyright (C) 2001-2005 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(WIN_UTIL_H)
#define WIN_UTIL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/Util.h"
#include "../client/SettingsManager.h"
#include "../client/User.h"
#include "../client/MerkleTree.h"
#include "../client/ResourceManager.h"

#include "resource.h"
#include "CZDCLib.h"
#include "OMenu.h"

#include <wininet.h>

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
	UserInfoBase(const User::Ptr& u) : user(u) { }
	
	void getList();
	void browseList();
	void getUserResponses();
	void checkList();
	void doReport();
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
	UserInfoBaseHandler() {
		sSelectedUser = Util::emptyStringT;
	}

	BEGIN_MSG_MAP(UserInfoBaseHandler)
		COMMAND_ID_HANDLER(IDC_GETLIST, onGetList)
		COMMAND_ID_HANDLER(IDC_BROWSELIST, onBrowseList)
		COMMAND_ID_HANDLER(IDC_CHECKLIST, onCheckList)
		COMMAND_ID_HANDLER(IDC_GET_USER_RESPONSES, onGetUserResponses)
		COMMAND_ID_HANDLER(IDC_MATCH_QUEUE, onMatchQueue)
		COMMAND_ID_HANDLER(IDC_PRIVATEMESSAGE, onPrivateMessage)
		COMMAND_ID_HANDLER(IDC_ADD_TO_FAVORITES, onAddToFavorites)
		COMMAND_ID_HANDLER(IDC_GRANTSLOT, onGrantSlot)
		COMMAND_ID_HANDLER(IDC_REMOVEALL, onRemoveAll)
		COMMAND_ID_HANDLER(IDC_GRANTSLOT_HOUR, onGrantSlotHour)
		COMMAND_ID_HANDLER(IDC_GRANTSLOT_DAY, onGrantSlotDay)
		COMMAND_ID_HANDLER(IDC_GRANTSLOT_WEEK, onGrantSlotWeek)
		COMMAND_ID_HANDLER(IDC_UNGRANTSLOT, onUnGrantSlot)
		COMMAND_ID_HANDLER(IDC_REPORT, onReport)
	END_MSG_MAP()

	LRESULT onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(sSelectedUser != _T("")) {
			int nAtPos = ((T*)this)->getUserList().findItem(sSelectedUser);
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::matchQueue);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::matchQueue);
		}
		return 0;
	}
	LRESULT onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(sSelectedUser != _T("")) {
			int nAtPos = ((T*)this)->getUserList().findItem(sSelectedUser);
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::getList);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::getList);
		}
		return 0;
	}
	LRESULT onBrowseList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		((T*)this)->getUserList().forEachSelected(&UserInfoBase::browseList);
		return 0;
	}
	LRESULT onReport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(sSelectedUser != Util::emptyStringT) {
			int nAtPos = ((T*)this)->getUserList().findItem(sSelectedUser);
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::doReport);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::doReport);
		}
		return 0;
	}

	LRESULT onGetUserResponses(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(sSelectedUser != Util::emptyStringT) {
			int nAtPos = ((T*)this)->getUserList().findItem(sSelectedUser);
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::getUserResponses);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::getUserResponses);
		}
		return 0;
	}

	LRESULT onCheckList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(sSelectedUser != Util::emptyStringT) {
			int nAtPos = ((T*)this)->getUserList().findItem(sSelectedUser);
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::checkList);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::checkList);
		}
		return 0;
	}

	LRESULT onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(sSelectedUser != _T("")) {
			int nAtPos = ((T*)this)->getUserList().findItem(sSelectedUser);
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::addFav);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::addFav);
		}
		return 0;
	}
	LRESULT onPrivateMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(sSelectedUser != _T("")) {
			int nAtPos = ((T*)this)->getUserList().findItem(sSelectedUser);
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::pm);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::pm);
		}		
		return 0;
	}
	LRESULT onGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) { 
		if(sSelectedUser != _T("")) {
			int nAtPos = ((T*)this)->getUserList().findItem(sSelectedUser);
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::grant);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::grant);
		}
		return 0;
	}
	LRESULT onRemoveAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) { 
		if(sSelectedUser != _T("")) {
			int nAtPos = ((T*)this)->getUserList().findItem(sSelectedUser);
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::removeAll);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::removeAll);
		}
		return 0;
	}
	LRESULT onGrantSlotHour(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){ 
		if(sSelectedUser != _T("")) {
			int nAtPos = ((T*)this)->getUserList().findItem(sSelectedUser);
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::grantSlotHour);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::grantSlotHour);
		}
		return 0;
	}
	LRESULT onGrantSlotDay(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){ 
		if(sSelectedUser != _T("")) {
			int nAtPos = ((T*)this)->getUserList().findItem(sSelectedUser);
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::grantSlotDay);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::grantSlotDay);
		}
		return 0;
	}
	LRESULT onGrantSlotWeek(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){ 
		if(sSelectedUser != _T("")) {
			int nAtPos = ((T*)this)->getUserList().findItem(sSelectedUser);
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::grantSlotWeek);
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::grantSlotWeek);
		}
		return 0;
	}

	LRESULT onUnGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){ 
		if(sSelectedUser != _T("")) {
			int nAtPos = ((T*)this)->getUserList().findItem(sSelectedUser);
			if ( nAtPos >= 0 ) {
				((T*)this)->getUserList().forEachAtPos(nAtPos, &UserInfoBase::ungrantSlot );
			}
		} else {
			((T*)this)->getUserList().forEachSelected(&UserInfoBase::ungrantSlot );
		}
		return 0;
	}

	struct ADCOnly {
		ADCOnly() : adcOnly(true) { }
		void operator()(UserInfoBase* ui) { if(ui->getUser()->isSet(User::NMDC)) adcOnly = false; }

		bool adcOnly;
	};
	void checkAdcItems(CMenu& menu) {

		MENUITEMINFO mii = { 0 };
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STATE;
		if(((T*)this)->getUserList().forEachSelectedT(ADCOnly()).adcOnly) {
			menu.EnableMenuItem(IDC_BROWSELIST, MFS_ENABLED);
		} else {
			menu.EnableMenuItem(IDC_BROWSELIST, MFS_DISABLED);
		}
	}

	void appendUserItems(CMenu& menu) {
		menu.AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
		menu.AppendMenu(MF_STRING, IDC_BROWSELIST, CTSTRING(BROWSE_FILE_LIST));
		menu.AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CTSTRING(MATCH_QUEUE));
		menu.AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CTSTRING(SEND_PRIVATE_MESSAGE));
		menu.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
		menu.AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::grantMenu, CTSTRING(GRANT_SLOTS_MENU));
		menu.AppendMenu(MF_STRING, IDC_REMOVEALL, CTSTRING(REMOVE_FROM_ALL));
		menu.AppendMenu(MF_STRING, IDC_CONNECT, CTSTRING(CONNECT_FAVUSER_HUB));
	}

protected:
	tstring sSelectedUser;
};

class FlatTabCtrl;
class UserCommand;

template<class T, int title, int ID = -1>
class StaticFrame {
public:
	virtual ~StaticFrame() { frame = NULL; }

	static T* frame;
	static void openWindow() {
		if(frame == NULL) {
			frame = new T();
			frame->CreateEx(WinUtil::mdiClient, frame->rcDefault, CTSTRING_I(ResourceManager::Strings(title)));
			CZDCLib::setButtonPressed(ID, true);
		} else {
			// match the behavior of MainFrame::onSelected()
			HWND hWnd = frame->m_hWnd;
			if(isMDIChildActive(hWnd)) {
				::PostMessage(hWnd, WM_CLOSE, NULL, NULL);
			} else if(frame->MDIGetActive() != hWnd) {
				MainFrame::anyMF->MDIActivate(hWnd);
				CZDCLib::setButtonPressed(ID, true);
			} else if(BOOLSETTING(TOGGLE_ACTIVE_WINDOW)) {
				::SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
				frame->MDINext(hWnd);
				hWnd = frame->MDIGetActive();
				CZDCLib::setButtonPressed(ID, true);
			}
			if(::IsIconic(hWnd))
				::ShowWindow(hWnd, SW_RESTORE);
		}
	}
	static bool isMDIChildActive(HWND hWnd) {
		HWND wnd = MainFrame::anyMF->MDIGetActive();
		dcassert(wnd != NULL);
		return (hWnd == wnd);
	}
};

template<class T, int title, int ID>
T* StaticFrame<T, title, ID>::frame = NULL;

class WinUtil {
public:
	static CImageList fileImages;
	static int fileImageCount;
	static CImageList userImages;
	static CImageList flagImages;

	typedef HASH_MAP<string, int> ImageMap;
	typedef ImageMap::const_iterator ImageIter;

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
	static int dirMaskedIndex;
	static TStringList lastDirs;
	static HWND mainWnd;
	static HWND mdiClient;
	static FlatTabCtrl* tabCtrl;
	static tstring commands;
	static HHOOK hook;
	static tstring tth;
	static StringPairList initialDirs;	
	static tstring exceptioninfo;
	static DWORD helpCookie;	
	static bool isAppActive;
	static CHARFORMAT2 m_TextStyleTimestamp;
	static CHARFORMAT2 m_ChatTextGeneral;
	static CHARFORMAT2 m_TextStyleMyNick;
	static CHARFORMAT2 m_ChatTextMyOwn;
	static CHARFORMAT2 m_ChatTextServer;
	static CHARFORMAT2 m_ChatTextSystem;
	static CHARFORMAT2 m_TextStyleBold;
	static CHARFORMAT2 m_TextStyleFavUsers;
	static CHARFORMAT2 m_TextStyleOPs;
	static CHARFORMAT2 m_TextStyleURL;
	static CHARFORMAT2 m_ChatTextPrivate;
	static CHARFORMAT2 m_ChatTextLog;
	static bool mutesounds;

	static void reLoadImages(); // User Icon Begin / User Icon End
	static void createImageList1(CImageList &imglst, string file, int size);

	static void init(HWND hWnd);
	static void uninit();

	static void initColors();

	static void decodeFont(const tstring& setting, LOGFONT &dest);

	static void addInitalDir(const User::Ptr& user, string dir) {
		// Clear out previos initial dirs, just in case
		/// @todo clean up
		getInitialDir(user);
		while(initialDirs.size() > 30) {
			initialDirs.erase(initialDirs.begin());
		}
		initialDirs.push_back(make_pair(user->getCID().toBase32(), dir));
	}

	static string getInitialDir(const User::Ptr& user) {
		for(StringPairIter i = initialDirs.begin(); i != initialDirs.end(); ++i) {
			if(i->first == user->getCID().toBase32()) {
				string dir = i->second;
				initialDirs.erase(i);
				return dir;
			}
		}
		return Util::emptyString;
	}

	static bool getVersionInfo(OSVERSIONINFOEX& ver);

	/**
	 * Check if this is a common /-command.
	 * @param cmd The whole text string, will be updated to contain only the command.
	 * @param param Set to any parameters.
	 * @param message Message that should be sent to the chat.
	 * @param status Message that should be shown in the status line.
	 * @return True if the command was processed, false otherwise.
	 */
	static bool checkCommand(tstring& cmd, tstring& param, tstring& message, tstring& status);

	static int getTextWidth(const tstring& str, HWND hWnd) {
		HDC dc = ::GetDC(hWnd);
		int sz = getTextWidth(str, dc);
		::ReleaseDC(mainWnd, dc);
		return sz;
	}
	static int getTextWidth(const tstring& str, HDC dc) {
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

	static void setClipboard(const tstring& str);

	static void addLastDir(const tstring& dir) {
		if(find(lastDirs.begin(), lastDirs.end(), dir) != lastDirs.end()) {
			return;
		}
		if(lastDirs.size() == 10) {
			lastDirs.erase(lastDirs.begin());
		}
		lastDirs.push_back(dir);
	}
	
	static tstring encodeFont(LOGFONT const& font);
	
	static bool browseFile(tstring& target, HWND owner = NULL, bool save = true, const tstring& initialDir = Util::emptyStringW, const TCHAR* types = NULL, const TCHAR* defExt = NULL);
	static bool browseDirectory(tstring& target, HWND owner = NULL);

	// Hash related
	static void bitziLink(const TTHValue* /*aHash*/);
	static void copyMagnet(const TTHValue* /*aHash*/, const tstring& /*aFile*/, int64_t);
	static void searchHash(const TTHValue* /*aHash*/);

	// URL related
	static void registerDchubHandler();
	static void registerADChubHandler();
	static void registerMagnetHandler();
	static void unRegisterDchubHandler();
	static void unRegisterADChubHandler();
	static void unRegisterMagnetHandler();
	static void parseDchubUrl(const tstring& /*aUrl*/);
	static void parseADChubUrl(const tstring& /*aUrl*/);
	static void parseMagnetUri(const tstring& /*aUrl*/, bool aOverride = false);
	static bool parseDBLClick(const tstring& /*aString*/, string::size_type start, string::size_type end);
	static bool urlDcADCRegistered;
	static bool urlMagnetRegistered;
	static int textUnderCursor(POINT p, CEdit& ctrl, tstring& x);
	static void openLink(const tstring& url);
	static void openFile(const tstring& file) {
		::ShellExecute(NULL, NULL, file.c_str(), NULL, NULL, SW_SHOWNORMAL);
	}
	static void openFolder(const tstring& file);

	static int getIconIndex(const tstring& aFileName);

	static int getDirIconIndex() { return dirIconIndex; }
	static int getDirMaskedIndex() { return dirMaskedIndex; }
	
	static int getOsMajor();
	static int getOsMinor();
	
	//returns the position where the context menu should be
	//opened if it was invoked from the keyboard.
	//aPt is relative to the screen not the control.
	static void getContextMenuPos(CListViewCtrl& aList, POINT& aPt);
	static void getContextMenuPos(CTreeViewCtrl& aTree, POINT& aPt);
	static void getContextMenuPos(CEdit& aEdit,			POINT& aPt);
	
	static bool getUCParams(HWND parent, const UserCommand& cmd, StringMap& sm) throw();

	static tstring getNicks(const CID& cid) throw();
	static tstring getNicks(const User::Ptr& u) {
		if(u->isSet(User::NMDC)) {
			return Text::toT(u->getFirstNick());
		} else  {
			return getNicks(u->getCID());
		}
	}
	/** @return Pair of hubnames as a string and a bool representing the user's online status */
	static pair<tstring, bool> getHubNames(const CID& cid) throw();
	static pair<tstring, bool> getHubNames(const User::Ptr& u) { return getHubNames(u->getCID()); }

	static void splitTokens(int* array, const string& tokens, int maxItems = -1) throw();
	static void saveHeaderOrder(CListViewCtrl& ctrl, SettingsManager::StrSetting order, 
		SettingsManager::StrSetting widths, int n, int* indexes, int* sizes) throw();

	static bool isShift() { return (GetKeyState(VK_SHIFT) & 0x8000) > 0; }
	static bool isAlt() { return (GetKeyState(VK_MENU) & 0x8000) > 0; }
	static bool isCtrl() { return (GetKeyState(VK_CONTROL) & 0x8000) > 0; }

	template<class T> static HWND hiddenCreateEx(T& p) throw() {
		HWND active = (HWND)::SendMessage(mdiClient, WM_MDIGETACTIVE, 0, 0);
		::LockWindowUpdate(mdiClient);
		HWND ret = p.CreateEx(mdiClient);
		if(active && ::IsWindow(active))
			::SendMessage(mdiClient, WM_MDIACTIVATE, (WPARAM)active, 0);
		::LockWindowUpdate(0);
		return ret;
	}
	template<class T> static HWND hiddenCreateEx(T* p) throw() {
		return hiddenCreateEx(*p);
	}
	
	static void translate(HWND page, TextItem* textItems) 
	{
		if (textItems != NULL) {
			for(int i = 0; textItems[i].itemID != 0; i++) {
				::SetDlgItemText(page, textItems[i].itemID,
					Text::toT(ResourceManager::getInstance()->getString(textItems[i].translatedString)).c_str());
			}
		}
	}

	struct tbIDImage {
		int id, image;
		bool check;
		ResourceManager::Strings tooltip;
	};
	static tbIDImage ToolbarButtons[];

	static void ClearPreviewMenu(OMenu &previewMenu);
	static int SetupPreviewMenu(CMenu &previewMenu, string extension);
	static void RunPreviewCommand(unsigned int index, string target);
	static string formatTime(long rest);
	static int getImage(const Identity& u);
	static int getFlagImage(const char* country, bool fullname = false);
	static string generateStats();
private:
	static int CALLBACK browseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /*lp*/, LPARAM pData);
};

#endif // !defined(WIN_UTIL_H)

/**
 * @file
 * $Id$
 */
