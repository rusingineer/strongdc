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

#if !defined(AFX_CHILDFRM_H__A7078724_FD85_4F39_8463_5A08A5F45E33__INCLUDED_)
#define AFX_CHILDFRM_H__A7078724_FD85_4F39_8463_5A08A5F45E33__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"
#include "ChatCtrl.h"
#include "MainFrm.h"

#include "../client/Client.h"
#include "../client/User.h"
#include "../client/ClientManager.h"
#include "../client/TimerManager.h"
#include "../client/FastAlloc.h"
#include "../client/DirectoryListing.h"

#include "WinUtil.h"
#include "UCHandler.h"

#ifndef USERINFO_H
	#include "UserInfo.h"
#endif

#define EDIT_MESSAGE_MAP 10		// This could be any number, really...
#define EDIT_FILTER_MAP 11		//oDC: This could be any number, really...

struct CompareItems;

class HubFrame : public MDITabChildWindowImpl<HubFrame, RGB(255, 0, 0), IDR_HUB, IDR_HUB_OFF>, private ClientListener, 
	public CSplitterImpl<HubFrame>, private TimerManagerListener, public UCHandler<HubFrame>, 
	public UserInfoBaseHandler<HubFrame>
{
public:
	DECLARE_FRAME_WND_CLASS_EX("HubFrame", IDR_HUB, 0, COLOR_3DFACE);

	typedef CSplitterImpl<HubFrame> splitBase;
	CHorSplitterWindow splitChat;
	typedef MDITabChildWindowImpl<HubFrame, RGB(255, 0, 0), IDR_HUB, IDR_HUB_OFF> baseClass;
	typedef UCHandler<HubFrame> ucBase;
	typedef UserInfoBaseHandler<HubFrame> uibBase;
	
	BEGIN_MSG_MAP(HubFrame)
		NOTIFY_HANDLER(IDC_USERS, LVN_GETDISPINFO, ctrlUsers.onGetDispInfo)
		NOTIFY_HANDLER(IDC_USERS, LVN_COLUMNCLICK, ctrlUsers.onColumnClick)
		NOTIFY_HANDLER(IDC_USERS, NM_DBLCLK, onDoubleClickUsers)	
		NOTIFY_HANDLER(IDC_USERS, LVN_KEYDOWN, onKeyDownUsers)
		NOTIFY_HANDLER(IDC_USERS, NM_RETURN, onEnterUsers)
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFO, onGetToolTip)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)

		MESSAGE_HANDLER(FTM_CONTEXTMENU, onTabContextMenu)
		COMMAND_ID_HANDLER(ID_FILE_RECONNECT, OnFileReconnect)
		COMMAND_ID_HANDLER(IDC_REFRESH, onRefresh)
		COMMAND_ID_HANDLER(IDC_FOLLOW, onFollow)
		COMMAND_ID_HANDLER(IDC_SEND_MESSAGE, onSendMessage)
		COMMAND_ID_HANDLER(IDC_ADD_AS_FAVORITE, onAddAsFavorite)
		COMMAND_ID_HANDLER(IDC_COPY_NICK, onCopyUserInfo)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, onEditCopy)
		COMMAND_ID_HANDLER(ID_EDIT_SELECT_ALL, onEditSelectAll)
		COMMAND_ID_HANDLER(ID_EDIT_CLEAR_ALL, onEditClearAll)
		COMMAND_ID_HANDLER(IDC_COPY_ACTUAL_LINE, onCopyActualLine)
		COMMAND_ID_HANDLER(IDC_SELECT_USER, onSelectUser)
		COMMAND_ID_HANDLER(IDC_PUBLIC_MESSAGE, onPublicMessage)
		COMMAND_ID_HANDLER(IDC_COPY_EXACT_SHARE, onCopyUserInfo)
		COMMAND_ID_HANDLER(IDC_COPY_TAG, onCopyUserInfo)
		COMMAND_ID_HANDLER(IDC_COPY_DESCRIPTION, onCopyUserInfo)
		COMMAND_ID_HANDLER(IDC_COPY_EMAIL_ADDRESS, onCopyUserInfo)
		COMMAND_ID_HANDLER(IDC_AUTOSCROLL_CHAT, onAutoScrollChat)

		COMMAND_ID_HANDLER(IDC_GET_USER_RESPONSES, onGetUserResponses)
		COMMAND_ID_HANDLER(IDC_REPORT, onReport)

		COMMAND_ID_HANDLER(IDC_BAN_IP, onBanIP)
		COMMAND_ID_HANDLER(IDC_UNBAN_IP, onUnBanIP)
		COMMAND_ID_HANDLER(IDC_COPY_URL, onCopyURL)
		COMMAND_ID_HANDLER(IDC_OPEN_HUB_LOG, onOpenHubLog)
		COMMAND_ID_HANDLER(IDC_OPEN_USER_LOG, onOpenUserLog)
		COMMAND_ID_HANDLER(IDC_WHOIS_IP, onWhoisIP)
		NOTIFY_HANDLER(IDC_USERS, NM_CUSTOMDRAW, onCustomDraw)
		COMMAND_ID_HANDLER(IDC_COPY_IP, onCopyUserInfo)
		COMMAND_ID_HANDLER(IDC_COPY_NICK_IP, onCopyUserInfo)
		COMMAND_ID_HANDLER(IDC_COPY_ISP, onCopyUserInfo)
		COMMAND_ID_HANDLER(IDC_COPY_ALL, onCopyUserInfo)
		COMMAND_ID_HANDLER(IDC_IGNORE, onIgnore)
		COMMAND_ID_HANDLER(IDC_UNIGNORE, onUnignore)
		NOTIFY_HANDLER(IDC_CLIENT, EN_LINK, onClientEnLink)
		CHAIN_COMMANDS(ucBase)
		CHAIN_COMMANDS(uibBase)
		CHAIN_MSG_MAP(baseClass)
		CHAIN_MSG_MAP(splitBase)
	ALT_MSG_MAP(EDIT_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CHAR, onChar)
		MESSAGE_HANDLER(WM_KEYDOWN, onChar)
		MESSAGE_HANDLER(WM_KEYUP, onChar)
		MESSAGE_HANDLER(BM_SETCHECK, onShowUsers)
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK, onLButton)
	ALT_MSG_MAP(EDIT_FILTER_MAP)
		MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, onCtlColor)
		MESSAGE_HANDLER(WM_KEYUP, onFilterChar)
		MESSAGE_HANDLER(WM_KEYDOWN, onFilterCharDown)
		COMMAND_CODE_HANDLER(CBN_SELCHANGE, onFilterByChange)
		MESSAGE_HANDLER(WM_CLEAR, onFilterClipboard)
		MESSAGE_HANDLER(WM_COPY, onFilterClipboard)
		MESSAGE_HANDLER(WM_CUT, onFilterClipboard)
		MESSAGE_HANDLER(WM_PASTE, onFilterClipboard)
	END_MSG_MAP()

	virtual void OnFinalMessage(HWND /*hWnd*/) {
		dcassert(frames.find(server) != frames.end());
		dcassert(frames[server] == this);
		frames.erase(server);
		delete this;
	}

	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onCopyUserInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onDoubleClickUsers(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onShowUsers(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onFollow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onLButton(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onEnterUsers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onGetToolTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onRButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onRButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClientEnLink(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onClientRButtonDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onEditSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyActualLine(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSelectUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPublicMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAutoScrollChat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onUnBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onWhoIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBanned(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyURL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenHubLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onWhoisIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onFilterChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onFilterCharDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onFilterByChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled);
	LRESULT onFilterClipboard(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);


	void UpdateLayout(BOOL bResizeBars = TRUE);
	void addLine(const string& aLine);
	void addClientLine(const string& aLine, bool inChat = true);
	void addLine(const string& aLine, CHARFORMAT2& cf);
	void addClientLine(const string& aLine, CHARFORMAT2& cf, bool inChat = true );
	void onEnter();
	void onTab();
	void runUserCommand(UserCommand& uc);

	void updateEntireUserList();
		
	static void openWindow(const string& server, const string& nick = Util::emptyString, const string& password = Util::emptyString, const string& description = Util::emptyString, 
		int windowposx = 0, int windowposy = 0, int windowsizex = 0, int windowsizey = 0, int windowtype = 0, int chatusersplit = 0, bool stealth = false
		// CDM EXTENSION BEGINS FAVS
		, const string& rawOne = Util::emptyString
		, const string& rawTwo = Util::emptyString
		, const string& rawThree = Util::emptyString
		, const string& rawFour = Util::emptyString
		, const string& rawFive = Util::emptyString
//		, bool userIp = false
		// CDM EXTENSION ENDS
		);		

	static void closeDisconnected();

	// CDM EXTENSION BEGINS

	BOOL checkCheating(User::Ptr &user, DirectoryListing* dl);

	static HubFrame* getHub(Client* aClient) {
		HubFrame* hubFrame = NULL;
		for(FrameIter i = frames.begin() ; i != frames.end() ; i++)
		{
			hubFrame = (i->second);
			if(hubFrame->client == aClient)
			{
				return hubFrame;
			}
		}
		return NULL;
	}

	LRESULT onGetUserResponses(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		getUserResponses();
		return 0;
	}
	LRESULT onReport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int i=-1;
		if(client->isConnected()) {
			while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
				doReport(((UserInfo*)ctrlUsers.GetItemData(i))->user);
			}
		}
		return 0;
	}
	void doReport(User::Ptr& u)
	{
		string param = u->getNick();
		/*if (toOpChat) {
			client->send("$To: " + client->getClientOpChat() + " From: " + client->getNick() + " $<" + client->getNick() + "> " + 
			"\r\n *** Info on " + param + " ***" + "\r\n" + u->getReport() + "|");
		} else {*/
			addLine("*** Info on " + param + " ***" + "\r\n" + u->getReport() + "\r\n" );
		//}
	}
	void getUserResponses();
	// CDM EXTENSION ENDS

	
	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlMessage.SetFocus();
		return 0;
	}

	LRESULT onSendMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		onEnter();
		return 0;
	}
	
	LRESULT onAddAsFavorite(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		addAsFavorite();
		return 0;
	}

	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}

	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	//	HWND hWnd = (HWND)lParam;
		HDC hDC = (HDC)wParam;
	//	if(hWnd == ctrlClient.m_hWnd || hWnd == ctrlMessage.m_hWnd || hWnd == ctrlFilter.m_hWnd || hWnd == ctrlFilterBy.m_hWnd) {
				::SetBkColor(hDC, WinUtil::bgColor);
				::SetTextColor(hDC, WinUtil::textColor);
			return (LRESULT)WinUtil::bgBrush;
	//	} else {
	//		return 0;
	//	}
	}

	LRESULT onRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(client->isConnected()) {
			clearUserList();
			client->refreshUserList();
		}
		return 0;
	}
	
	LRESULT OnFileReconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		clearUserList();
		client->addListener(this);
		client->connect(server);
		return 0;
	}

	LRESULT onKeyDownUsers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVKEYDOWN* l = (NMLVKEYDOWN*)pnmh;
		if(l->wVKey == VK_TAB) {
			onTab();
		}
		return 0;
	}

	LRESULT onIgnore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/) {
		int i=-1;
		if(client->isConnected()) {
			if (sSelectedUser != "") {
				ignoreList.insert(sSelectedUser);
			} else {
				while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
					ignoreList.insert((((UserInfo*)ctrlUsers.GetItemData(i))->user)->getNick());
				}
			}
		}
		return 0;
	}

	LRESULT onUnignore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/) {
		int i=-1;
		if(client->isConnected()) {
			if (sSelectedUser != "") {
				ignoreList.erase(sSelectedUser);
			} else {
				while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
					ignoreList.erase((((UserInfo*)ctrlUsers.GetItemData(i))->user)->getNick());
				}
			}
		}
		return 0;
	}

	NOTIFYICONDATA pmicon;

public:
	TypedListViewCtrl<UserInfo, IDC_USERS>& getUserList() { return ctrlUsers; };
private:

//oDC
	friend class PrivateFrame;  //Per cmd PM
	StringTokenizer stFilter;
	int iFilterBySel;
//oDC FINE
	enum Speakers { UPDATE_USER, UPDATE_USERS, REMOVE_USER, ADD_CHAT_LINE,
		ADD_STATUS_LINE, ADD_SILENT_STATUS_LINE, SET_WINDOW_TITLE, GET_PASSWORD, 
		PRIVATE_MESSAGE, STATS, CONNECTED, DISCONNECTED, CHEATING_USER,
		GET_SHUTDOWN, SET_SHUTDOWN
	};

	enum {
		IMAGE_OP = 0, IMAGE_MODEM, IMAGE_ISDN, IMAGE_SATELLITE, IMAGE_WIRELESS, IMAGE_DSL, IMAGE_CABLE, IMAGE_LAN, IMAGE_SERVER, IMAGE_FIREBALL, IMAGE_UNKNOWN
	};
	
	class PMInfo {
	public:
		PMInfo(const User::Ptr& u, const string& m) : user(u), msg(m) { };
		User::Ptr user;
		string msg;
	};

	class CheatInfo {
	public:
		CheatInfo(const string& m) : msg(m) { };
		string msg;
	};

	HubFrame(const string& aServer, const string& aNick, const string& aPassword, const string& aDescription, 
		int windowposx, int windowposy, int windowsizex, int windowsizey, int windowtype, int chatusersplit, bool stealth
		// CDM EXTENSION BEGINS FAVS
		, const string& aRawOne
		, const string& aRawTwo
		, const string& aRawThree
		, const string& aRawFour
		, const string& aRawFive
//		, bool aUserIp
		// CDM EXTENSION ENDS		
		) : 
	waitingForPW(false), extraSort(false), server(aServer), closed(false), 
		updateUsers(false), curCommandPosition(0),
		ctrlMessageContainer("edit", this, EDIT_MESSAGE_MAP), 
		ctrlFilterContainer("edit", this, EDIT_FILTER_MAP),  //oDC
		ctrlFilterByContainer("COMBOBOX", this, EDIT_FILTER_MAP),  //oDC
		showUsersContainer("BUTTON", this, EDIT_MESSAGE_MAP),
		clientContainer("edit", this, EDIT_MESSAGE_MAP)
		, stFilter("")
	{
		client = ClientManager::getInstance()->getClient();
		client->setMe(ClientManager::getInstance()->getUser(SETTING(NICK), client, false)); 
		client->setUserInfo(BOOLSETTING(GET_USER_INFO));
		client->setNick(aNick.empty() ? SETTING(NICK) : aNick);
		if (!aDescription.empty())
			client->setDescription(aDescription);
		client->setPassword(aPassword);
		// CDM EXTENSION BEGINS FAVS
		client->setRawOne(aRawOne);
		client->setRawTwo(aRawTwo);
		client->setRawThree(aRawThree);
		client->setRawFour(aRawFour);
		client->setRawFive(aRawFive);
//		client->setSendUserIp(aUserIp);
		// CDM EXTENSION ENDS
		client->addListener(this);
		TimerManager::getInstance()->addListener(this);
		timeStamps = BOOLSETTING(TIME_STAMPS);
		hubchatusersplit = chatusersplit;
		client->setStealth(stealth);
	}

	~HubFrame() {
		ClientManager::getInstance()->putClient(client);
	}

	typedef HASH_MAP<string, HubFrame*> FrameMap;
	typedef FrameMap::iterator FrameIter;
	static FrameMap frames;

	string redirect;
	bool timeStamps;
	bool showJoins;
	string complete;
	
	bool waitingForPW;
	bool extraSort;

	StringList prevCommands;
	string currentCommand;
	StringList::size_type curCommandPosition;

	typedef hash_set<string> StringHash;
	StringHash ignoreList;

	Client* client;
	string server;
	CContainedWindow ctrlMessageContainer;
	CContainedWindow ctrlFilterContainer;  //oDC
	CContainedWindow ctrlFilterByContainer;  //oDC

	CContainedWindow clientContainer;
	CContainedWindow showUsersContainer;

	OMenu copyMenu;
	OMenu grantMenu;
	OMenu userMenu;
	OMenu tabMenu;
	
	CButton ctrlShowUsers;
	ChatCtrl ctrlClient;
	CEdit ctrlMessage;
	typedef TypedListViewCtrl<UserInfo, IDC_USERS> CtrlUsers;
	CtrlUsers ctrlUsers;
	CEdit ctrlFilter;  //oDC
	CComboBox ctrlFilterBy;  //oDC

	CStatusBarCtrl ctrlStatus;

	bool closed;

	StringMap ucParams;
	StringMap tabParams;
	bool tabMenuShown;
	
	typedef vector<pair<User::Ptr, Speakers> > UpdateList;
	typedef UpdateList::iterator UpdateIter;
	typedef HASH_MAP<User::Ptr, UserInfo*, User::HashFunction> UserMap;
	typedef UserMap::iterator UserMapIter;

	UserMap userMap;
	UpdateList updateList;
	CriticalSection updateCS;
	bool updateUsers;

	enum { MAX_CLIENT_LINES = 5 };
	StringList lastLinesList;
	string lastLines;
	CToolTipCtrl ctrlLastLines;
	
	UserListColumns m_UserListColumns;
	
	bool updateUser(const User::Ptr& u);
	
	CHARFORMAT2 m_ChatTextGeneral;
	CHARFORMAT2 m_ChatTextPrivate;
	CHARFORMAT2 m_ChatTextServer;
	CHARFORMAT2 m_ChatTextSystem;
	
	LPCTSTR sMyNick;
	int hubchatusersplit;

	bool PreparePopupMenu( CWindow *pCtrl, bool boCopyOnly, string& sNick, OMenu *pMenu );

	int findUser(const User::Ptr& aUser);
	void addAsFavorite();

	void clearUserList() {
		{
			Lock l(updateCS);
			updateList.clear();
		}

		userMap.clear();
		int j = ctrlUsers.GetItemCount();
		for(int i = 0; i < j; i++) {
			delete (UserInfo*) ctrlUsers.GetItemData(i);
		}
		ctrlUsers.DeleteAllItems();
	}

	int getImage(const User::Ptr& u) {
		int image = IMAGE_UNKNOWN;
		const string& tmp = u->getConnection();
		int status = u->getStatus();
		if (u->isSet(User::OP)) {
			image = IMAGE_OP;
		} else if( (status == 4) || (status == 5)  || (status == 6) || (status == 7) ) {
			image = IMAGE_SERVER;
		} else if( (status == 8) || (status == 9)  || (status == 10) || (status == 11) ) {
			image = IMAGE_FIREBALL;
		} else if( (tmp == "28.8Kbps") ||
			(tmp == "33.6Kbps") ||
			(tmp == "56Kbps") ||
			(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_MODEM]) ) {
			image = IMAGE_MODEM;
		} else if(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_ISDN]) {
			image = IMAGE_ISDN;
		} else if(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_SATELLITE]) {
			image = IMAGE_SATELLITE;
		} else if( (tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_WIRELESS]) ||
			(tmp == "Microwave") ) {
			image = IMAGE_WIRELESS;
		} else if(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_DSL]) {
			image = IMAGE_DSL;
		} else if(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_CABLE]) {
			image = IMAGE_CABLE;
		} else if( (tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_T1]) ||
			(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_T3]) ) {
			image = IMAGE_LAN;
		}
		if( (status == 2) || (status == 3) || (status == 6) || (status == 7) || (status == 10) || (status == 11) ) {
			image+=11;
		}
		if(u->isSet(User::DCPLUSPLUS)) {
			image+=22;
		}
		if(u->isSet(User::PASSIVE) || (u->getMode() == "P") || (u->getMode() == "5")) {
			image+=44;
		}
		return image;	
	}

	void updateStatusBar() {
		if(m_hWnd)
			PostMessage(WM_SPEAKER, STATS);
	}

	// TimerManagerListener
	virtual void onAction(TimerManagerListener::Types type, DWORD /*aTick*/) throw();

	// ClientListener
	virtual void onAction(ClientListener::Types type, Client* client) throw();
	virtual void onAction(ClientListener::Types type, Client* /*client*/, const string& line) throw();
	virtual void onAction(ClientListener::Types type, Client* /*client*/, const User::Ptr& user) throw();
	virtual void onAction(ClientListener::Types type, Client* /*client*/, const User::List& aList) throw();
	virtual void onAction(ClientListener::Types type, Client* /*client*/, const User::Ptr& user, const string&  line) throw();

	void speak(Speakers s) { PostMessage(WM_SPEAKER, (WPARAM)s); };
	void speak(Speakers s, const string& msg) { PostMessage(WM_SPEAKER, (WPARAM)s, (LPARAM)new string(msg)); };
	void speak(Speakers s, const User::Ptr& u) { 
		Lock l(updateCS);
		updateList.push_back(make_pair(u, s));
		updateUsers = true;
	};
	void speak(Speakers s, const User::Ptr& u, const string& line) { PostMessage(WM_SPEAKER, (WPARAM)s, (LPARAM)new PMInfo(u, line)); };

};
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHILDFRM_H__A7078724_FD85_4F39_8463_5A08A5F45E33__INCLUDED_)

/**
 * @file
 * $Id$
 */

