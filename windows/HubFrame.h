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

#if !defined(HUB_FRAME_H)
#define HUB_FRAME_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"
#include "ChatCtrl.h"
#include "MainFrm.h"
#include "EmoticonsDlg.h"

#include "../client/Client.h"
#include "../client/User.h"
#include "../client/ClientManager.h"
#include "../client/TimerManager.h"
#include "../client/FastAlloc.h"
#include "../client/DirectoryListing.h"

#include "WinUtil.h"
#include "UCHandler.h"
#include "UserInfo.h"

#define EDIT_MESSAGE_MAP 10		// This could be any number, really...
#define FILTER_MESSAGE_MAP 8
struct CompareItems;

class HubFrame : public MDITabChildWindowImpl<HubFrame, RGB(255, 0, 0), IDR_HUB, IDR_HUB_OFF>, private ClientListener, 
	public CSplitterImpl<HubFrame>, private TimerManagerListener, public UCHandler<HubFrame>, 
	public UserInfoBaseHandler<HubFrame>, private SettingsManagerListener
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("HubFrame"), IDR_HUB, 0, COLOR_3DFACE);

	typedef CSplitterImpl<HubFrame> splitBase;
	CHorSplitterWindow splitChat;
	typedef MDITabChildWindowImpl<HubFrame, RGB(255, 0, 0), IDR_HUB, IDR_HUB_OFF> baseClass;
	typedef UCHandler<HubFrame> ucBase;
	typedef UserInfoBaseHandler<HubFrame> uibBase;
	
	BEGIN_MSG_MAP(HubFrame)
		NOTIFY_HANDLER(IDC_USERS, LVN_GETDISPINFO, ctrlUsers.onGetDispInfo)
		NOTIFY_HANDLER(IDC_USERS, LVN_COLUMNCLICK, ctrlUsers.onColumnClick)
		NOTIFY_HANDLER(IDC_USERS, LVN_GETINFOTIP, ctrlUsers.onInfoTip)
		NOTIFY_HANDLER(IDC_USERS, NM_DBLCLK, onDoubleClickUsers)	
		NOTIFY_HANDLER(IDC_USERS, LVN_KEYDOWN, onKeyDownUsers)
		NOTIFY_HANDLER(IDC_USERS, NM_RETURN, onEnterUsers)
		NOTIFY_HANDLER(IDC_CLIENT, EN_LINK, onClientEnLink)
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFO, onGetToolTip)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(FTM_CONTEXTMENU, onTabContextMenu)
		MESSAGE_HANDLER(WM_MOUSEMOVE, onStyleChange)
		MESSAGE_HANDLER(WM_CAPTURECHANGED, onStyleChanged)
		MESSAGE_HANDLER(WM_WINDOWPOSCHANGING, onSizeMove)
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
		COMMAND_ID_HANDLER(IDC_BAN_IP, onBanIP)
		COMMAND_ID_HANDLER(IDC_UNBAN_IP, onUnBanIP)
		COMMAND_ID_HANDLER(IDC_COPY_URL, onCopyURL)
		COMMAND_ID_HANDLER(IDC_OPEN_HUB_LOG, onOpenHubLog)
		COMMAND_ID_HANDLER(IDC_OPEN_USER_LOG, onOpenUserLog)
		COMMAND_ID_HANDLER(IDC_WHOIS_IP, onWhoisIP)
		COMMAND_ID_HANDLER(IDC_EMOT, onEmoticons)
		COMMAND_RANGE_HANDLER(IDC_EMOMENU, IDC_EMOMENU + menuItems, onEmoPackChange);
		NOTIFY_HANDLER(IDC_USERS, NM_CUSTOMDRAW, onCustomDraw)
		COMMAND_ID_HANDLER(IDC_COPY_IP, onCopyUserInfo)
		COMMAND_ID_HANDLER(IDC_COPY_NICK_IP, onCopyUserInfo)
		COMMAND_ID_HANDLER(IDC_COPY_ALL, onCopyUserInfo)
		
		COMMAND_ID_HANDLER(IDC_COPY_HUBNAME, onCopyHubInfo)
		COMMAND_ID_HANDLER(IDC_COPY_HUBADDRESS, onCopyHubInfo)

		COMMAND_ID_HANDLER(IDC_IGNORE, onIgnore)
		COMMAND_ID_HANDLER(IDC_UNIGNORE, onUnignore)
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
		//MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
	ALT_MSG_MAP(FILTER_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, onCtlColor)
		MESSAGE_HANDLER(WM_KEYUP, onFilterChar)
		COMMAND_CODE_HANDLER(CBN_SELCHANGE, onSelChange)
	END_MSG_MAP()

	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onCopyUserInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyHubInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
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
	LRESULT onRButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClientEnLink(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
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
	LRESULT onStyleChange(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onStyleChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSizeMove(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled); 
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onFilterChar(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onEmoPackChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onEmoticons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled) {
		if (hWndCtl != ctrlEmoticons.m_hWnd) {
			bHandled = false;
			return 0;
		}

		EmoticonsDlg dlg;
		ctrlEmoticons.GetWindowRect(dlg.pos);
		dlg.DoModal(m_hWnd);
		if (!dlg.result.empty()) {
			TCHAR* message = new TCHAR[ctrlMessage.GetWindowTextLength()+1];
			ctrlMessage.GetWindowText(message, ctrlMessage.GetWindowTextLength()+1);
			tstring s(message, ctrlMessage.GetWindowTextLength());
			delete[] message;
			ctrlMessage.SetWindowText(Text::toT(Text::fromT(s)+dlg.result).c_str());
			ctrlMessage.SetFocus();
			ctrlMessage.SetSel( ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength() );
		}
		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE);
	void addLine(const tstring& aLine);
	void addLine(const tstring& aLine, CHARFORMAT2& cf, bool bUseEmo = true);
	void addLine(const Identity& i, const tstring& aLine, CHARFORMAT2& cf, bool bUseEmo = true);
	void addClientLine(const tstring& aLine, bool inChat = true);
	void addClientLine(const tstring& aLine, CHARFORMAT2& cf, bool inChat = true );
	void onEnter();
	void onTab();
	void runUserCommand(::UserCommand& uc);
		
	static void openWindow(const tstring& server
		, const tstring& rawOne = Util::emptyStringT
		, const tstring& rawTwo = Util::emptyStringT
		, const tstring& rawThree = Util::emptyStringT
		, const tstring& rawFour = Util::emptyStringT
		, const tstring& rawFive = Util::emptyStringT
		, int windowposx = 0, int windowposy = 0, int windowsizex = 0, int windowsizey = 0, int windowtype = 0, int chatusersplit = 0, bool userliststate = true,
		        string sColumsOrder = Util::emptyString, string sColumsWidth = Util::emptyString, string sColumsVisible = Util::emptyString);
	static void closeDisconnected();

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

	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlMessage.SetFocus();
		//ctrlClient.GoToEnd();
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

	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		HDC hDC = (HDC)wParam;
				::SetBkColor(hDC, WinUtil::bgColor);
				::SetTextColor(hDC, WinUtil::textColor);
			return (LRESULT)WinUtil::bgBrush;
	}

	LRESULT onRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(client->isConnected()) {
			clearUserList();
			client->refreshUserList();
		}
		return 0;
	}

	LRESULT OnFileReconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		client->disconnect(false);
		clearUserList();
		client->connect();
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
			if(sSelectedUser != _T("")) {
				ignoreList.insert(sSelectedUser);
			} else {
				while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
					ignoreList.insert(Text::toT((((UserInfo*)ctrlUsers.getItemData(i))->user)->getFirstNick()));
				}
			}
		}
		return 0;
	}

	LRESULT onUnignore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/) {
		int i=-1;
		if(client->isConnected()) {
			if(sSelectedUser != _T("")) {
				ignoreList.erase(sSelectedUser);
			} else {
				while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
					ignoreList.erase(Text::toT((((UserInfo*)ctrlUsers.getItemData(i))->user)->getFirstNick()));
				}
			}
		}
		return 0;
	}

	TypedListViewCtrl<UserInfo, IDC_USERS>& getUserList() { return ctrlUsers; };
private:
	enum {
		COLUMN_FIRST,
		COLUMN_NICK = COLUMN_FIRST, 
		COLUMN_SHARED, 
		COLUMN_EXACT_SHARED, 
		COLUMN_DESCRIPTION, 
		COLUMN_TAG,
		COLUMN_CONNECTION, 
		COLUMN_EMAIL, 
		COLUMN_CLIENTID, 
		COLUMN_VERSION, 
		COLUMN_MODE, 
		COLUMN_HUBS, 
		COLUMN_SLOTS,
		COLUMN_UPLOAD_SPEED, 
		COLUMN_IP, 
		COLUMN_PK, 
		COLUMN_LOCK, 
		COLUMN_SUPPORTS,
		COLUMN_LAST
	};

	friend class PrivateFrame;
	
	enum Speakers { UPDATE_USER, UPDATE_USERS, REMOVE_USER, ADD_CHAT_LINE,
		ADD_STATUS_LINE, ADD_SILENT_STATUS_LINE, SET_WINDOW_TITLE, GET_PASSWORD, 
		PRIVATE_MESSAGE, STATS, CONNECTED, DISCONNECTED, CHEATING_USER,
		GET_SHUTDOWN, SET_SHUTDOWN, KICK_MSG
	};

	class MessageInfo {
	public:
		MessageInfo(Identity from_, const User::Ptr& to_, const User::Ptr& replyTo_, const string& m) : from(from_), to(to_), replyTo(replyTo_), msg(Text::toT(m)) { };
		Identity from;
		User::Ptr to;
		User::Ptr replyTo;
		tstring msg;
	};

	HubFrame(const tstring& aServer
		, const tstring& aRawOne
		, const tstring& aRawTwo
		, const tstring& aRawThree
		, const tstring& aRawFour
		, const tstring& aRawFive
		, int chatusersplit, bool userliststate) : 
		waitingForPW(false), extraSort(false), server(aServer), closed(false), 
		showUsers(BOOLSETTING(GET_USER_INFO)), updateUsers(false), resort(false), curCommandPosition(0), currentNeedlePos(-1),
		timeStamps(BOOLSETTING(TIME_STAMPS)), hubchatusersplit(chatusersplit), menuItems(0),
		ctrlMessageContainer(WC_EDIT, this, EDIT_MESSAGE_MAP), 
		showUsersContainer(WC_BUTTON, this, EDIT_MESSAGE_MAP),
		clientContainer(WC_EDIT, this, EDIT_MESSAGE_MAP),
		ctrlFilterContainer(WC_EDIT, this, FILTER_MESSAGE_MAP),
		ctrlFilterSelContainer(WC_COMBOBOX, this, FILTER_MESSAGE_MAP) {
		client = ClientManager::getInstance()->getClient(Text::fromT(aServer));

		client->setRawOne(Text::fromT(aRawOne));
		client->setRawTwo(Text::fromT(aRawTwo));
		client->setRawThree(Text::fromT(aRawThree));
		client->setRawFour(Text::fromT(aRawFour));
		client->setRawFive(Text::fromT(aRawFive));
		//client->addListener(this);
		if(FavoriteManager::getInstance()->getFavoriteHubEntry(Text::fromT(server)) != NULL) {
			showUsers = userliststate;
		} else {
			showUsers = BOOLSETTING(GET_USER_INFO);
		}
	}

	virtual ~HubFrame() {
		dcassert(frames.find(server) != frames.end());
		dcassert(frames[server] == this);
		frames.erase(server);

		ClientManager::getInstance()->putClient(client);
	}

	typedef HASH_MAP<tstring, HubFrame*> FrameMap;
	typedef FrameMap::const_iterator FrameIter;
	static FrameMap frames;

	tstring redirect;
	bool timeStamps;
	bool showJoins;
	bool favShowJoins;
	tstring complete;
	int menuItems;

	bool waitingForPW;
	bool extraSort;

	TStringList prevCommands;
	tstring currentCommand;
	TStringList::size_type curCommandPosition;

	typedef hash_set<tstring> TStringHash;
	TStringHash ignoreList;

	tstring currentNeedle;		// search in chat window
	int currentNeedlePos;		// search in chat window
	void findText(tstring const& needle) throw();
	tstring findTextPopup();

	CFindReplaceDialog findDlg;

	const tstring& getNick(const User::Ptr& u);
	
	Client* client;
	tstring server;
	CContainedWindow ctrlMessageContainer;
	CContainedWindow clientContainer;
	CContainedWindow showUsersContainer;
	CContainedWindow ctrlFilterContainer;
	CContainedWindow ctrlFilterSelContainer;

	OMenu copyMenu;
	OMenu copyHubMenu;
	OMenu grantMenu;
	OMenu userMenu;
	OMenu tabMenu;
	OMenu emoMenu;
	
	CButton ctrlShowUsers;
	CButton ctrlEmoticons;
	ChatCtrl ctrlClient;
	CEdit ctrlMessage;
	CEdit ctrlFilter;
	CComboBox ctrlFilterSel;
	typedef TypedListViewCtrl<UserInfo, IDC_USERS> CtrlUsers;
	CtrlUsers ctrlUsers;
	CStatusBarCtrl ctrlStatus;
	
	HBITMAP hEmoticonBmp;
	bool closed;

	TStringMap tabParams;
	bool tabMenuShown;

	typedef vector<pair<UpdateInfo, Speakers> > UpdateList;
	typedef UpdateList::const_iterator UpdateIter;
	typedef HASH_MAP<User::Ptr, UserInfo*, User::HashFunction> UserMap;
	typedef UserMap::const_iterator UserMapIter;

	UserMap userMap;
	UpdateList updateList;
	CriticalSection updateCS;
	bool updateUsers;
	bool resort;

	enum { MAX_CLIENT_LINES = 5 };
	TStringList lastLinesList;
	tstring lastLines;
	CToolTipCtrl ctrlLastLines;
	
	static int columnIndexes[COLUMN_LAST];
	static int columnSizes[COLUMN_LAST];
	
	int findUser(const User::Ptr& aUser);

	bool updateUser(const UpdateInfo& u);
	void removeUser(const User::Ptr& aUser);

	void addAsFavorite();
	void removeFavoriteHub();

	void clearUserList();

	int hubchatusersplit;
	tstring filter;
	void updateUserList();
	void filterUser(UserInfo* ui);

	bool PreparePopupMenu(CWindow *pCtrl, const tstring& sNick, OMenu *pMenu);
	bool showUsers;
	string sColumsOrder;
    string sColumsWidth;
    string sColumsVisible;

	void updateStatusBar() {
		if(m_hWnd)
			PostMessage(WM_SPEAKER, STATS);
	}

	// TimerManagerListener
	virtual void on(TimerManagerListener::Second, DWORD /*aTick*/) throw();
	virtual void on(SettingsManagerListener::Save, SimpleXML* /*xml*/) throw();

	// ClientListener
	virtual void on(Connecting, Client*) throw();
	virtual void on(Connected, Client*) throw();
	virtual void on(BadPassword, Client*) throw();
	virtual void on(UserUpdated, Client*, const OnlineUser&) throw();
	virtual void on(UsersUpdated, Client*, const OnlineUser::List&) throw();
	virtual void on(UserRemoved, Client*, const OnlineUser&) throw();
	virtual void on(Redirect, Client*, const string&) throw();
	virtual void on(Failed, Client*, const string&) throw();
	virtual void on(GetPassword, Client*) throw();
	virtual void on(HubUpdated, Client*) throw();
	virtual void on(Message, Client*, const OnlineUser&, const char*) throw();
	virtual void on(PrivateMessage, Client*, const OnlineUser&, const OnlineUser&, const OnlineUser&, const string&) throw();
	virtual void on(NickTaken, Client*) throw();
	virtual void on(SearchFlood, Client*, const string&) throw();
	virtual void on(CheatMessage, Client*, const string&) throw();	

	void speak(Speakers s) { PostMessage(WM_SPEAKER, (WPARAM)s); };
	void speak(Speakers s, const string& msg) { PostMessage(WM_SPEAKER, (WPARAM)s, (LPARAM)new tstring(Text::toT(msg))); };
	void speak(Speakers s, const OnlineUser& u) { 
		Lock l(updateCS);
		updateList.push_back(make_pair(UpdateInfo(u), s));
		updateUsers = true;
	};
	void speak(Speakers s, const OnlineUser& from, const User::Ptr& to, const User::Ptr& replyTo, const string& line) { PostMessage(WM_SPEAKER, (WPARAM)s, (LPARAM)new MessageInfo(&from ? from.getIdentity() : Identity(NULL, Util::emptyString), to, replyTo, line)); };

};

#endif // !defined(HUB_FRAME_H)

/**
 * @file
 * $Id$
 */
