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

#include "HubFrame.h"
#include "LineDlg.h"
#include "SearchFrm.h"
#include "PrivateFrame.h"
#include "CZDCLib.h"

#include "../client/QueueManager.h"
#include "../client/ShareManager.h"
#include "../client/Util.h"
#include "../client/StringTokenizer.h"
#include "../client/HubManager.h"
#include "../client/LogManager.h"
#include "../client/SettingsManager.h"
#include "../client/ConnectionManager.h" 

HubFrame::FrameMap HubFrame::frames;

string sSelectedLine = "";
string sSelectedIP = "";
string sSelectedURL = "";
long lURLBegin = 0;
long lURLEnd = 0;

LRESULT HubFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	ctrlClient.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY, WS_EX_CLIENTEDGE, IDC_CLIENT);

	ctrlClient.LimitText(0);
	ctrlClient.SetFont(WinUtil::font);
	clientContainer.SubclassWindow(ctrlClient.m_hWnd);
	
	CHARFORMAT2 cf;
	memset(&cf, 0, sizeof(CHARFORMAT2));
	cf.cbSize = sizeof(cf);
	cf.dwReserved = 0;
	cf.dwMask = CFM_BACKCOLOR | CFM_COLOR | CFM_BOLD | CFM_ITALIC;
	cf.dwEffects = 0;
	cf.crBackColor = SETTING(BACKGROUND_COLOR);
	cf.crTextColor = SETTING(TEXT_COLOR);

	m_ChatTextGeneral = cf;
	m_ChatTextGeneral.crBackColor     = SETTING(TEXT_GENERAL_BACK_COLOR);
	m_ChatTextGeneral.crTextColor     = SETTING(TEXT_GENERAL_FORE_COLOR);
	if ( SETTING(TEXT_GENERAL_BOLD) )
		m_ChatTextGeneral.dwEffects       |= CFE_BOLD;
	if ( SETTING(TEXT_GENERAL_ITALIC) )
		m_ChatTextGeneral.dwEffects       |= CFE_ITALIC;

	m_ChatTextPrivate = cf;
	m_ChatTextPrivate.crBackColor   = SETTING(TEXT_PRIVATE_BACK_COLOR);
	m_ChatTextPrivate.crTextColor   = SETTING(TEXT_PRIVATE_FORE_COLOR);
	if ( SETTING(TEXT_PRIVATE_BOLD) )
		m_ChatTextPrivate.dwEffects     |= CFE_BOLD;
	if ( SETTING(TEXT_PRIVATE_ITALIC) )
		m_ChatTextPrivate.dwEffects     |= CFE_ITALIC;

	m_ChatTextSystem = cf;
	m_ChatTextSystem.crBackColor    = SETTING(TEXT_SYSTEM_BACK_COLOR);
	m_ChatTextSystem.crTextColor    = SETTING(TEXT_SYSTEM_FORE_COLOR);
	if ( SETTING(TEXT_SYSTEM_BOLD) )
		m_ChatTextSystem.dwEffects      |= CFE_BOLD;
	if ( SETTING(TEXT_SYSTEM_ITALIC) )
		m_ChatTextSystem.dwEffects      |= CFE_ITALIC;

	m_ChatTextServer = cf;
	m_ChatTextServer.crBackColor = SETTING(TEXT_SERVER_BACK_COLOR);
	m_ChatTextServer.crTextColor = SETTING(TEXT_SERVER_FORE_COLOR);
	if ( SETTING(TEXT_SERVER_BOLD) )
		m_ChatTextServer.dwEffects      |= CFE_BOLD;
	if ( SETTING(TEXT_SERVER_ITALIC) )
		m_ChatTextServer.dwEffects      |= CFE_ITALIC;

	ctrlClient.SetBackgroundColor( SETTING(BACKGROUND_COLOR) ); 
	ctrlClient.SetAutoURLDetect( true );
	ctrlClient.SetEventMask( ctrlClient.GetEventMask() | ENM_LINK );
	ctrlClient.SetUsers( &ctrlUsers );
	
	ctrlMessage.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | ES_NOHIDESEL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE, WS_EX_CLIENTEDGE);
	
	ctrlMessageContainer.SubclassWindow(ctrlMessage.m_hWnd);
	ctrlMessage.SetFont(WinUtil::font);

	ctrlFilter.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);
	ctrlFilterContainer.SubclassWindow(ctrlFilter.m_hWnd);

	ctrlFilterBy.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |/* WS_HSCROLL | WS_VSCROLL |*/ CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);
	ctrlFilterByContainer.SubclassWindow(ctrlFilterBy.m_hWnd);

	ctrlUsers.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_USERS);
	
	DWORD styles = LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT;
	if (BOOLSETTING(SHOW_INFOTIPS))
		styles |= LVS_EX_INFOTIP;
	ctrlUsers.SetExtendedListViewStyle(styles);

	ctrlFilter.SetFont(WinUtil::font);
	ctrlFilterBy.SetFont(WinUtil::font);
	ctrlFilterBy.AddString(CSTRING(NICK_MENU));
	ctrlFilterBy.AddString(CSTRING(DESCRIPTION_MENU));
	ctrlFilterBy.AddString(CSTRING(TAG_MENU));
	ctrlFilterBy.AddString(CSTRING(CLIENT_MENU));
	ctrlFilterBy.AddString(CSTRING(CONNECTION_MENU));
	ctrlFilterBy.AddString(CSTRING(EMAIL_MENU));
	ctrlFilterBy.AddString(CSTRING(ANY_MENU));
	ctrlFilterBy.SetCurSel(0);
	iFilterBySel = ctrlFilterBy.GetCurSel(); // Make sure we are were we are...
	splitChat.Create( m_hWnd );
	splitChat.SetSplitterPanes(ctrlClient.m_hWnd, ctrlMessage.m_hWnd, true);
	splitChat.SetSplitterExtendedStyle(0);
	splitChat.SetSplitterPos( 40 );

	SetSplitterPanes(ctrlClient.m_hWnd, ctrlUsers.m_hWnd, false);
//	SetSplitterPanes(splitChat.m_hWnd, ctrlUsers.m_hWnd, false);
	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
	if(hubchatusersplit){ m_nProportionalPos = hubchatusersplit;
	} else { m_nProportionalPos = 7500; }
	ctrlShowUsers.Create(ctrlStatus.m_hWnd, rcDefault, "+/-", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlShowUsers.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowUsers.SetFont(WinUtil::systemFont);
	ctrlShowUsers.SetCheck(ShowUserList ? BST_CHECKED : BST_UNCHECKED);
	showUsersContainer.SubclassWindow(ctrlShowUsers.m_hWnd);

	m_UserListColumns.ReadFromSetup();
	m_UserListColumns.SetToList(ctrlUsers);

	
	ctrlUsers.SetBkColor(WinUtil::bgColor);
	ctrlUsers.SetTextBkColor(WinUtil::bgColor);
	ctrlUsers.SetTextColor(WinUtil::textColor);
	
	ctrlUsers.setSortColumn(UserInfo::COLUMN_NICK);
				
	ctrlUsers.SetImageList(WinUtil::userImages, LVSIL_SMALL);

	CToolInfo ti(TTF_SUBCLASS, ctrlStatus.m_hWnd);
	
	ctrlLastLines.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, WS_EX_TOPMOST);
	ctrlLastLines.AddTool(&ti);

	tabMenu.CreatePopupMenu();
	if(BOOLSETTING(LOG_MAIN_CHAT)) {
		tabMenu.AppendMenu(MF_STRING, IDC_OPEN_HUB_LOG, CSTRING(OPEN_HUB_LOG));
		tabMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	}
	tabMenu.AppendMenu(MF_STRING, IDC_ADD_AS_FAVORITE, CSTRING(ADD_TO_FAVORITES));
	tabMenu.AppendMenu(MF_STRING, ID_FILE_RECONNECT, CSTRING(MENU_RECONNECT));
	tabMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);	
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CSTRING(CLOSE));

	if (CZDCLib::isXp()) {
		pmicon.hIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE( IDR_TRAY_PM_XP ), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	} else {
		pmicon.hIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE( IDR_TRAY_PM ), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	}

	showJoins = BOOLSETTING(SHOW_JOINS);
	favShowJoins = BOOLSETTING(FAV_SHOW_JOINS);

	bHandled = FALSE;
	client->connect();
	return 1;
}

void HubFrame::openWindow(const string& aServer, const string& aNick /* = Util::emptyString */, const string& aPassword /* = Util::emptyString */, const string& aDescription /* = Util::emptyString */, 
		int windowposx, int windowposy, int windowsizex, int windowsizey, int windowtype, int chatusersplit, bool stealth, bool userliststate
							// CDM EXTENSION BEGINS FAVS
							, const string& rawOne /*= Util::emptyString*/
							, const string& rawTwo /*= Util::emptyString*/
							, const string& rawThree /*= Util::emptyString*/
							, const string& rawFour /*= Util::emptyString*/
							, const string& rawFive /*= Util::emptyString*/
						//	, bool userIp /*= false*/
							// CDM EXTENSION ENDS		
		) {
	FrameIter i = frames.find(aServer);
	if(i == frames.end()) {
		HubFrame* frm = new HubFrame(aServer, aNick, aPassword, aDescription, 
			windowposx, windowposy, windowsizex, windowsizey, windowtype, chatusersplit, stealth, userliststate
			// CDM EXTENSION BEGINS FAVS
			, rawOne
			, rawTwo 
			, rawThree 
			, rawFour 
			, rawFive 
//			, userIp 
			// CDM EXTENSION ENDS			
			);
		frames[aServer] = frm;

		int nCmdShow = SW_SHOWDEFAULT;
		CRect rc = frm->rcDefault;

		rc.left = windowposx;
		rc.top = windowposy;
		rc.right = rc.left + windowsizex;
		rc.bottom = rc.top + windowsizey;
		if( (rc.left < 0 ) || (rc.top < 0) || (rc.right - rc.left < 10) || ((rc.bottom - rc.top) < 10) ) {
			rc = frm->rcDefault;
		}
		frm->CreateEx(WinUtil::mdiClient, rc);
		if(windowtype)
			frm->ShowWindow(((nCmdShow == SW_SHOWDEFAULT) || (nCmdShow == SW_SHOWNORMAL)) ? windowtype : nCmdShow);
	} else {
		i->second->MDIActivate(i->second->m_hWnd);
	}
}

void HubFrame::onEnter() {
	char* message;
	
	if(ctrlMessage.GetWindowTextLength() > 0) {
		message = new char[ctrlMessage.GetWindowTextLength()+1];
		ctrlMessage.GetWindowText(message, ctrlMessage.GetWindowTextLength()+1);
		string s(message, ctrlMessage.GetWindowTextLength());
		delete[] message;

		// save command in history, reset current buffer pointer to the newest command
		curCommandPosition = prevCommands.size();		//this places it one position beyond a legal subscript
		if (!curCommandPosition || curCommandPosition > 0 && prevCommands[curCommandPosition - 1] != s) {
			++curCommandPosition;
			prevCommands.push_back(s);
		}
		currentCommand = "";

		// Special command
		if(s[0] == '/') {
			string m = s;
			string param;
			string message;
			string status;
			if(WinUtil::checkCommand(s, param, message, status)) {
				if(!message.empty()) {
					client->hubMessage(message);
				}
				if(!status.empty()) {
					addClientLine(status, m_ChatTextSystem);
				}
			} else if(Util::stricmp(s.c_str(), "join")==0) {
				if(!param.empty()) {
					redirect = param;
					BOOL whatever = FALSE;
					onFollow(0, 0, 0, whatever);
				} else {
					addClientLine(STRING(SPECIFY_SERVER), m_ChatTextSystem);
				}
			} else if((Util::stricmp(s.c_str(), "clear") == 0) || (Util::stricmp(s.c_str(), "c") == 0)) {
				ctrlClient.SetWindowText("");
			} else if(Util::stricmp(s.c_str(), "ts") == 0) {
				timeStamps = !timeStamps;
				if(timeStamps) {
					addClientLine(STRING(TIMESTAMPS_ENABLED), m_ChatTextSystem);
				} else {
					addClientLine(STRING(TIMESTAMPS_DISABLED), m_ChatTextSystem);
				}
			} else if( (Util::stricmp(s.c_str(), "password") == 0) && waitingForPW ) {
				client->setPassword(param);
				client->password(param);
				waitingForPW = false;
			} else if( Util::stricmp(s.c_str(), "showjoins") == 0 ) {
				showJoins = !showJoins;
				if(showJoins) {
					addClientLine(STRING(JOIN_SHOWING_ON), m_ChatTextSystem);
				} else {
					addClientLine(STRING(JOIN_SHOWING_OFF), m_ChatTextSystem);
				}
			} else if( Util::stricmp(s.c_str(), "favshowjoins") == 0 ) {
				favShowJoins = !favShowJoins;
				if(favShowJoins) {
					addClientLine(STRING(FAV_JOIN_SHOWING_ON), m_ChatTextSystem);
				} else {
					addClientLine(STRING(FAV_JOIN_SHOWING_OFF), m_ChatTextSystem);
				}
			}  else if(Util::stricmp(s.c_str(), "close") == 0) {
				PostMessage(WM_CLOSE);
			} else if(Util::stricmp(s.c_str(), "userlist") == 0) {
				ctrlShowUsers.SetCheck(ShowUserList ? BST_UNCHECKED : BST_CHECKED);
			} else if(Util::stricmp(s.c_str(), "connection") == 0) {
				addClientLine((STRING(IP) + client->getLocalIp() + ", " + STRING(PORT) + Util::toString(SETTING(IN_PORT))), m_ChatTextSystem);
			} else if((Util::stricmp(s.c_str(), "favorite") == 0) || (Util::stricmp(s.c_str(), "fav") == 0)) {
				addAsFavorite();
			} else if(Util::stricmp(s.c_str(), "getlist") == 0){
				if( !param.empty() ){
					int k = ctrlUsers.findItem(param);
					if(k != -1) {
						ctrlUsers.getItemData(k)->getList();
					}
				}
			} else if(Util::stricmp(s.c_str(), "f") == 0) {
				if(param.empty())
					param=findTextPopup();
				findText(param);
			}  else if(Util::stricmp(s.c_str(), "extraslots")==0) {
				int j = Util::toInt(param);
				if(j > 0) {
					SettingsManager::getInstance()->set(SettingsManager::EXTRA_SLOTS, j);
					addClientLine(STRING(EXTRA_SLOTS_SET), m_ChatTextSystem );
				} else {
					addClientLine(STRING(INVALID_NUMBER_OF_SLOTS), m_ChatTextSystem );
				}
			} else if(Util::stricmp(s.c_str(), "smallfilesize")==0) {
				int j = Util::toInt(param);
				if(j >= 64) {
					SettingsManager::getInstance()->set(SettingsManager::SMALL_FILE_SIZE, j);
					addClientLine(STRING(SMALL_FILE_SIZE_SET), m_ChatTextSystem );
				} else {
					addClientLine(STRING(INVALID_SIZE), m_ChatTextSystem );
				}
			} else if(stricmp(s.c_str(), "savequeue") == 0) {
				QueueManager::getInstance()->saveQueue();
				addClientLine("Queue saved.", m_ChatTextSystem );
			} else if(Util::stricmp(s.c_str(), "whois") == 0) {
				WinUtil::openLink("http://www.ripe.net/perl/whois?form_type=simple&full_query_string=&searchtext=" + param);
			} else if(Util::stricmp(s.c_str(), "showblockedipports") == 0) {
				StringList sl = ConnectionManager::getInstance()->getBlockedIpPorts();
				string ips = "***";
				for(StringIter i = sl.begin(); i != sl.end(); ++i)
					ips += ' ' + *i;
				addLine(ips);
			} else if(Util::stricmp(s.c_str(), "showshared") == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_SHARED, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), "showexactshared") == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_EXACT_SHARED, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), "showdescription") == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_DESCRIPTION, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), "showtag") == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_TAG, ctrlUsers);

			} else if(Util::stricmp(s.c_str(), "showconnection") == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_CONNECTION, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), "showemail") == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_EMAIL, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), "showclient") == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_CLIENTID, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), "showversion") == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_VERSION, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), "showmode") == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_MODE, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), "showhubs") == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_HUBS, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), "showslots") == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_SLOTS, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), "showupload") == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_UPLOAD_SPEED, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), "showip") == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_IP, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), "showisp") == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_ISP, ctrlUsers);
			} else if(stricmp(s.c_str(), "ignorelist")==0) {
				string ignorelist = "Ignored users:";
				for (StringHash::iterator i = ignoreList.begin(); i != ignoreList.end(); ++i)
					ignorelist += " " + *i;
				addClientLine(ignorelist, m_ChatTextSystem);
			} else if(Util::stricmp(s.c_str(), "help") == 0) {
addLine(WinUtil::commands + "------------------------------------------------------------------------------------------------------------------------------------------------------------\n/join <hub-ip> \t\t\t\t(pripojit k hubu...) \t\t\t\t\t\t\n/close \t\t\t\t\t(odpoji z hubu) \t\t\t\t\t\t\t\n/favorite \t\t\t\t(prida hub do favorites) \t\t\t\t\t\t\n/fav \t\t\t\t\t(prida hub do favorites) \t\t\t\t\t\t\n------------------------------------------------------------------------------------------------------------------------------------------------------------\n/pm <user> [message] \t\t\t(odesle uzivateli privatni zpravu) \t\t\t\t\t\n/getlist <user> \t\t\t\t(stahne od uzivatele file list) \t\t\t\t\t\n------------------------------------------------------------------------------------------------------------------------------------------------------------\n/showshared \t\t\t\t(zapne/vypne sloupec Shared v userlistu) \t\t\t\n/showexactshared \t\t\t(zapne/vypne sloupec Exact Shared v userlistu) \t\t\t\n/showdescription \t\t\t(zapne/vypne sloupec Description v userlistu) \t\t\t\n/showtag \t\t\t\t(zapne/vypne sloupec Tag v userlistu) \t\t\t\t\n/showconnection \t\t\t(zapne/vypne sloupec Connection v userlistu) \t\t\t\n/showemail \t\t\t\t(zapne/vypne sloupec E-mail v userlistu) \t\t\t\t\n/showclient \t\t\t\t(zapne/vypne sloupec Client v userlistu) \t\t\t\t\n/showversion \t\t\t\t(zapne/vypne sloupec Version v userlistu) \t\t\t\n/showmode \t\t\t\t(zapne/vypne sloupec Mode v userlistu) \t\t\t\t\n/showhubs \t\t\t\t(zapne/vypne sloupec Hubs v userlistu) \t\t\t\t\n/showslots \t\t\t\t(zapne/vypne sloupec Slots v userlistu) \t\t\t\t\n/showupload \t\t\t\t(zapne/vypne sloupec Upload v userlistu) \t\t\t\n/showisp \t\t\t\t(zapne/vypne sloupec ISP v userlistu) \t\t\t\t\n", m_ChatTextSystem);
			} else if(Util::stricmp(s.c_str(), "pm") == 0) {
				string::size_type j = param.find(' ');
				if(j != string::npos) {
					string nick = param.substr(0, j);
					int k = ctrlUsers.findItem(nick);
					if(k != -1) {
						UserInfo* ui = ctrlUsers.getItemData(k);
						if(param.size() > j + 1)
							PrivateFrame::openWindow(ui->user, param.substr(j+1));
						else
							PrivateFrame::openWindow(ui->user);
					}
				} else if(!param.empty()) {
					int k = ctrlUsers.findItem(param);
					if(k != -1) {
						UserInfo* ui = ctrlUsers.getItemData(k);
						PrivateFrame::openWindow(ui->user);
					}
				}
			} else if(Util::stricmp(s.c_str(), "me") == 0) {
				client->hubMessage(m);
			} else {
				if (BOOLSETTING(SEND_UNKNOWN_COMMANDS)) {
					client->hubMessage(m);
				} else {
					addClientLine(STRING(UNKNOWN_COMMAND) + s);
				}
			}
		} else {
			client->hubMessage(s);
		}
		ctrlMessage.SetWindowText("");
	} else {
		MessageBeep(MB_ICONEXCLAMATION);
	}
}

struct CompareItems {
	CompareItems(int aCol) : col(aCol) { }
	bool operator()(const UserInfo& a, const UserInfo& b) const {
		return UserInfo::compareItems(&a, &b, col) == -1;
	}
	const int col;
};

int HubFrame::findUser(const User::Ptr& aUser) {
	UserMapIter i = userMap.find(aUser);
	if(i == userMap.end())
		return -1;

	if(ctrlUsers.getSortColumn() == UserInfo::COLUMN_NICK) {
		// Sort order of the other columns changes too late when the user's updated
		UserInfo* ui = i->second;
		dcassert(ctrlUsers.getItemData(ctrlUsers.getSortPos(ui)) == ui);
		return ctrlUsers.getSortPos(ui);
	}
	return ctrlUsers.findItem(aUser->getNick());
}

void HubFrame::addAsFavorite() {
	FavoriteHubEntry aEntry;
	char buf[256];
	this->GetWindowText(buf, 255);
	aEntry.setServer(server);
	aEntry.setName(buf);
	aEntry.setDescription(buf);
	aEntry.setConnect(TRUE);
	aEntry.setNick(client->getNick());
	aEntry.setConnect(false);
	HubManager::getInstance()->addFavorite(aEntry);
	addClientLine(STRING(FAVORITE_HUB_ADDED), m_ChatTextSystem );
}

LRESULT HubFrame::onCopyUserInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if(client->isConnected()) {
        string sCopy;
		UserInfo* ui;

		if (sSelectedUser != "") {
			int i = ctrlUsers.findItem( sSelectedUser );
			if ( i >= 0 ) {
				ui = (UserInfo*)ctrlUsers.getItemData(i);

				switch (wID) {
					case IDC_COPY_NICK:
						sCopy += ui->user->getNick();
						break;
					case IDC_COPY_EXACT_SHARE:
						sCopy += Util::formatNumber(ui->user->getBytesShared());
						break;
					case IDC_COPY_DESCRIPTION:
						sCopy += ui->user->getDescription();
						break;
					case IDC_COPY_TAG:
						sCopy += ui->user->getTag();
						break;
					case IDC_COPY_EMAIL_ADDRESS:
						sCopy += ui->user->getEmail();
						break;
					case IDC_COPY_IP:
						sCopy += ui->user->getIp();
						break;
					case IDC_COPY_ISP:
						sCopy += ui->user->getHost();
						break;
					case IDC_COPY_NICK_IP:
						sCopy += "Info User:\r\n"
							"\tNick: " + ui->user->getNick() + "\r\n" + 
							"\tIP: " + ui->user->getIp() + "\r\n" ; 
						break;
					case IDC_COPY_ALL:
						sCopy += "Info User:\r\n"
							"\tNick: " + ui->user->getNick() + "\r\n" + 
							"\tShare: " + Util::formatBytes(ui->user->getBytesShared()) + "\r\n" + 
							"\tDescription: " + ui->user->getDescription() + "\r\n" +
							"\tTag: " + ui->user->getTag() + "\r\n" +
							"\tConnection: " + ui->user->getConnection() + "\r\n" + 
							"\te-Mail: " + ui->user->getEmail() + "\r\n" +
							"\tClient: " + ui->user->getClientType() + "\r\n" + 
							"\tVersion: " + ui->user->getVersion() + "\r\n" +
							"\tMode: " + ui->user->getMode() + "\r\n" +
							"\tHubs: " + ui->user->getHubs() + "\r\n" +
							"\tSlots: " + ui->user->getSlots() + "\r\n" +
							"\tUpLimit: " + ui->user->getUpload() + "\r\n" +
							"\tIP: " + ui->user->getIp() + "\r\n"+
							"\tISP: " + ui->user->getHost() + "\r\n"+
							"\tPk String: " + ui->user->getPk() + "\r\n"+
							"\tLock: " + ui->user->getLock() + "\r\n"+
							"\tSupports: " + ui->user->getSupports();
						break;		
					default:
						dcdebug("DON'T GO HERE\n");
						return 0;
				}
			}
		}
		if (!sCopy.empty())
			WinUtil::setClipboard(sCopy);
    }
	return 0;
}

LRESULT HubFrame::onDoubleClickUsers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;
	if(client->isConnected() && item->iItem != -1) {
		ctrlUsers.getItemData(item->iItem)->getList();
	}
	return 0;
}

LRESULT HubFrame::onEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	HWND focus = GetFocus();
	if ( focus == ctrlClient.m_hWnd ) {
		ctrlClient.Copy();
	}
	return 0;
}

LRESULT HubFrame::onEditSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	HWND focus = GetFocus();
	if ( focus == ctrlClient.m_hWnd ) {
		ctrlClient.SetSelAll();
	}
	return 0;
}

LRESULT HubFrame::onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	HWND focus = GetFocus();
	if ( focus == ctrlClient.m_hWnd ) {
		ctrlClient.SetWindowText("");
	}
	return 0;
}

bool HubFrame::updateUser(const User::Ptr& u, bool searchinlist /* = true */) {
	int i = -1;
	if(searchinlist)
		i = findUser(u);

	bool bHideUser = false;
	if (!stFilter.getTokens().empty()) {
		string filter_s;
		int64_t filter_i = -1;
		switch (iFilterBySel) {
			case 0: // Nick
				filter_s = u->getNick();
				break;
			case 1: // Description
				filter_s = u->getDescription();
				break;
			case 2: // Tag
				filter_s = u->getTag();
				break;
			case 3: // Client
				filter_s = u->getClientType();
				break;
			case 4: // Connection
				filter_s = u->getConnection();
				break;
			case 5: // E-Mail
				filter_s = u->getEmail();
				break;
			case 6: // Any
				filter_s = u->getNick() + ' ' + u->getTag() + ' ' + u->getClientType() + ' ' + u->getDescription() + ' ' + u->getConnection() + ' ' + u->getEmail();
				break;
			default: // We should NEVER be here...
				dcassert(FALSE);
				filter_s = "";
				break;
		}
		if (filter_i == -1)
			bHideUser = !stFilter.getTokens().empty() && !CZDCLib::findListChild(stFilter.getTokens(), filter_s);
		else
			bHideUser = !stFilter.getTokens().empty() && !CZDCLib::findListChild(stFilter.getTokens(), filter_i);
	}
	if (bHideUser) {
		if (i != -1) {
			//delete (UserInfo*) ctrlUsers.getItemData(i);
			UserInfo* ui = ctrlUsers.getItemData(i);
			ctrlUsers.deleteItem(i);
			dcassert(userMap[u] == ui);
			userMap.erase(u);
//			delete ui;
		}
		return false;
	} else {
	if(i == -1) {
		UserInfo* ui = new UserInfo(u, &m_UserListColumns);
		userMap.insert(make_pair(u, ui));
		ctrlUsers.insertItem(ui, getImage(u));
		return true;
	} else {
		UserInfo* ui = ctrlUsers.getItemData(i);
		bool resort = (ui->getOp() != u->isSet(User::OP));
		ctrlUsers.getItemData(i)->update();
		ctrlUsers.updateItem(i);
		ctrlUsers.SetItem(i, 0, LVIF_IMAGE, NULL, getImage(u), 0, 0, NULL);
		if(resort)
			ctrlUsers.resort();		
		return false;
	}
}
}

LRESULT HubFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(wParam == UPDATE_USERS) {
		ctrlUsers.SetRedraw(FALSE);
		bool resort = false;
		{
			Lock l(updateCS);
			for(UpdateIter i = updateList.begin(); i != updateList.end(); ++i) {
				User::Ptr& u = i->first;
				switch(i->second) {
				case UPDATE_USER:
					if(updateUser(u)) {
						if(showJoins) {
							if (!favShowJoins | u->isFavoriteUser()) {
								addLine("*** " + STRING(JOINS) + u->getNick(), m_ChatTextSystem);
							}
						}
					} else {
						resort = true;
		}
					break;
				case UPDATE_USERS:
					if(!updateUser(u))
						resort = true;
					
					break;
				case REMOVE_USER:
					int j = findUser(u);
					if( j != -1 ) {
						if(showJoins) {
							if (!favShowJoins | u->isFavoriteUser())
								addLine("*** " + STRING(PARTS) + u->getNick(), m_ChatTextSystem);
						}

						ctrlUsers.SetItemState(j, 0, LVIS_SELECTED);
						dcassert(userMap[u] ==(UserInfo*)ctrlUsers.getItemData(j));
						userMap.erase(u);
						ctrlUsers.deleteItem(j);
					}
					break;
				}
		}
			updateList.clear();
		}
		if(ctrlUsers.getSortColumn() != UserInfo::COLUMN_NICK)
			ctrlUsers.resort();
		ctrlUsers.SetRedraw(TRUE);
	} else if(wParam == CHEATING_USER) {
		User* u = (User*)wParam;

		CHARFORMAT2 cf;
		memset(&cf, 0, sizeof(CHARFORMAT2));
		cf.cbSize = sizeof(cf);
		cf.dwReserved = 0;
		cf.dwMask = CFM_BACKCOLOR | CFM_COLOR | CFM_BOLD;
		cf.dwEffects = 0;
		cf.crBackColor = SETTING(BACKGROUND_COLOR);
		cf.crTextColor = SETTING(ERROR_COLOR);

		addLine("*** "+STRING(USER)+" "+u->getNick()+": "+u->getCheatingString(),cf);
		delete u;
	} else if(wParam == DISCONNECTED) {
		clearUserList();
		setTabColor(RGB(255, 0, 0));
		setIconState();
	} else if(wParam == CONNECTED) {
		addClientLine(STRING(CONNECTED), m_ChatTextServer);
		setTabColor(RGB(0, 255, 0));
		unsetIconState();
	} else if(wParam == ADD_CHAT_LINE) {
		string* x = (string*)lParam;
		int nickPos = x->find('<') + 1;
		string nick = x->substr(nickPos, x->find('>') - nickPos);
		int i = -1;
		if(nick != "")
			i = ctrlUsers.findItem(nick);
		if ( i >= 0 ) {
			UserInfo* ui = (UserInfo*)ctrlUsers.getItemData(i);
			if (!ignoreList.count(nick) || (ui->user->isSet(User::OP) && !client->getOp()))
				addLine(*x, m_ChatTextGeneral);
		} else {
			if (!ignoreList.count(nick))
				addLine(*x, m_ChatTextGeneral);
		}
		delete x;
	} else if(wParam == ADD_STATUS_LINE) {
		string* x = (string*)lParam;
		addClientLine(*x, m_ChatTextServer );
		delete x;
	} else if(wParam == ADD_SILENT_STATUS_LINE) {
		string* x = (string*)lParam;
		addClientLine(*x, false);
		delete x;
	} else if(wParam == SET_WINDOW_TITLE) {
		string* x = (string*)lParam;
		SetWindowText(x->c_str());
		delete x;
	} else if(wParam == STATS) {
		ctrlStatus.SetText(1, (Util::toString(client->getUserCount()) + " " + STRING(HUB_USERS)).c_str());
				ctrlStatus.SetText(2, Util::formatBytes(client->getAvailable()).c_str());
	} else if(wParam == GET_PASSWORD) {
		if(client->getPassword().size() > 0) {
			client->password(client->getPassword());
			addClientLine(STRING(STORED_PASSWORD_SENT), m_ChatTextSystem);
		} else {
			ctrlMessage.SetWindowText("/password ");
			ctrlMessage.SetFocus();
			ctrlMessage.SetSel(10, 10);
			waitingForPW = true;
		}
	} else if(wParam == PRIVATE_MESSAGE) {
		PMInfo* i = (PMInfo*)lParam;
		if(!ignoreList.count(i->user->getNick()) || (i->user->isSet(User::OP) && !client->getOp())) {
			if(i->user->isOnline()) {
			if(BOOLSETTING(POPUP_PMS) || PrivateFrame::isOpen(i->user)) {
					PrivateFrame::gotMessage(i->user, i->msg);
				} else {
					addLine(STRING(PRIVATE_MESSAGE_FROM) + i->user->getNick() + ": " + i->msg, m_ChatTextPrivate);
				}
		HWND hMainWnd = GetTopLevelWindow();
		if (BOOLSETTING(MINIMIZE_TRAY) && (!WinUtil::isAppActive || WinUtil::isMinimized) && !WinUtil::isPM) {
			NOTIFYICONDATA nid;
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = hMainWnd;
			nid.uID = 0;
			nid.uFlags = NIF_ICON;
			nid.hIcon = pmicon.hIcon;
			::Shell_NotifyIcon(NIM_MODIFY, &nid);
			WinUtil::isPM = true;
		}
			} else {
				if(BOOLSETTING(IGNORE_OFFLINE)) {
					addClientLine(STRING(IGNORED_MESSAGE) + i->msg, m_ChatTextPrivate, false);
				} else if(BOOLSETTING(POPUP_OFFLINE)) {
					PrivateFrame::gotMessage(i->user, i->msg);
				} else {
					addLine(STRING(PRIVATE_MESSAGE_FROM) + i->user->getNick() + ": " + i->msg, m_ChatTextPrivate);
				}
			}
		}
		delete i;
	}

	return 0;
};

void HubFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
	
	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[4];
		ctrlStatus.GetClientRect(sr);

		int tmp = (sr.Width()) > 332 ? 232 : ((sr.Width() > 132) ? sr.Width()-100 : 32);
		
		w[0] = sr.right - tmp;
		w[1] = w[0] + (tmp-32)/2;
		w[2] = w[0] + (tmp-32);
		w[3] = w[2] + 16;
		
		ctrlStatus.SetParts(4, w);

		ctrlLastLines.SetMaxTipWidth(w[0]);
		ctrlLastLines.SetWindowPos(HWND_TOPMOST, sr.left, sr.top, sr.Width(), sr.Height(), SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

		// Strange, can't get the correct width of the last field...
		ctrlStatus.GetRect(2, sr);
		sr.left = sr.right + 2;
		sr.right = sr.left + 16;
		ctrlShowUsers.MoveWindow(sr);
	}
	int h = WinUtil::fontHeight + 4;

	CRect rc = rect;
	rc.bottom -= h + 10;
	if(!ShowUserList) {
		if(GetSinglePaneMode() == SPLIT_PANE_NONE)
			SetSinglePaneMode(SPLIT_PANE_LEFT);
	} else {
		if(GetSinglePaneMode() != SPLIT_PANE_NONE)
			SetSinglePaneMode(SPLIT_PANE_NONE);
	}
	SetSplitterRect(rc);

/*	if ( rc.Height() > ((2 * h) + 5) ) {
		splitChat.SetSplitterPos( rc.Height() - ((2 * h) + 5) );
	} else {
		splitChat.SetSplitterPos( -1 );
	}*/

	rc = rect;
	rc.bottom -= 2;
	rc.top = rc.bottom - h - 5;
	rc.left +=2;
	rc.right -=(2 + (ShowUserList ? 200 : 0));  //oDC: si aggiunge il menu
	ctrlMessage.MoveWindow(rc);
	if (ShowUserList) {
		rc.left = rc.right + 4;
		rc.right = rc.left + 116;
		ctrlFilter.MoveWindow(rc);

		rc.left = rc.right + 4;
		rc.right = rc.left + 76;
		rc.top = rc.top + 0;
		rc.bottom = rc.bottom + 120;
		ctrlFilterBy.MoveWindow(rc);
	}
}

LRESULT HubFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if(!closed) {
		RecentHubEntry* r = HubManager::getInstance()->getRecentHubEntry(server);
		if(r) {
			char buf[256];
			this->GetWindowText(buf, 255);
			r->setName(buf);
			r->setUsers(Util::toString(client->getUserCount()));
			r->setShared(Util::toString(client->getAvailable()));
			HubManager::getInstance()->updateRecent(r);
		}
		TimerManager::getInstance()->removeListener(this);
		client->removeListener(this);
		client->disconnect();
	
		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		SettingsManager::getInstance()->set(SettingsManager::GET_USER_INFO, ShowUserList);

		userMap.clear();
		ctrlUsers.DeleteAll();
	
		m_UserListColumns.WriteToSetup(ctrlUsers);

		FavoriteHubEntry* hub = HubManager::getInstance()->getFavoriteHubEntry(server);
		if(hub) {
			WINDOWPLACEMENT wp;
			wp.length = sizeof(wp);
			GetWindowPlacement(&wp);

			CRect rc;
			GetWindowRect(rc);
			CRect rcmdiClient;
			::GetWindowRect(WinUtil::mdiClient, &rcmdiClient);
			if(wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWNORMAL) {
				hub->setWindowPosX(rc.left - (rcmdiClient.left + 2));
				hub->setWindowPosY(rc.top - (rcmdiClient.top + 2));
				hub->setWindowSizeX(rc.Width());
				hub->setWindowSizeY(rc.Height());
			}
			if(wp.showCmd == SW_SHOWNORMAL || wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_MAXIMIZE)
				hub->setWindowType((int)wp.showCmd);
			hub->setChatUserSplit(m_nProportionalPos);
			hub->setUserListState(ShowUserList);
			HubManager::getInstance()->save();
		}
		DestroyIcon(pmicon.hIcon);				
		MDIDestroy(m_hWnd);
	return 0;
	}
}

void HubFrame::findText(string const& needle) throw() {
	int max = ctrlClient.GetWindowTextLength();
	// a new search? reset cursor to bottom
	if(needle != currentNeedle || currentNeedlePos == -1) {
		currentNeedle = needle;
		currentNeedlePos = max;
	}
	// set current selection
	FINDTEXT ft;
	ft.chrg.cpMin = currentNeedlePos;
	ft.chrg.cpMax = 0; // REVERSED!! GAH!! FUCKING RETARDS! *blowing off steam*
	ft.lpstrText = needle.c_str();
	// empty search? stop
	if(needle.empty())
		return;
	// find upwards
	currentNeedlePos = (int)ctrlClient.SendMessage(EM_FINDTEXT, 0, (LPARAM)&ft);
	// not found? try again on full range
	if(currentNeedlePos == -1 && ft.chrg.cpMin != max) { // no need to search full range twice
		currentNeedlePos = max;
		ft.chrg.cpMin = currentNeedlePos;
		currentNeedlePos = (int)ctrlClient.SendMessage(EM_FINDTEXT, 0, (LPARAM)&ft);
	}
	// found? set selection
	if(currentNeedlePos != -1) {
		ft.chrg.cpMin = currentNeedlePos;
		ft.chrg.cpMax = currentNeedlePos + needle.length();
		ctrlClient.SetFocus();
		ctrlClient.SendMessage(EM_EXSETSEL, 0, (LPARAM)&ft);
	} else {
		addClientLine(CSTRING(STRING_NOT_FOUND)+needle);
		currentNeedle="";
	}
}


string HubFrame::findTextPopup() {
	LineDlg *finddlg;
	string param="";
		finddlg=new LineDlg;
		finddlg->title=CSTRING(SEARCH);
		finddlg->description=CSTRING(SPECIFY_SEARCH_STRING);
		if(finddlg->DoModal()== IDOK) {
		param=finddlg->line;
	}
	delete[] finddlg;
	return param;
}

LRESULT HubFrame::onLButton(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HWND focus = GetFocus();
	bHandled = false;
	if(focus == ctrlClient.m_hWnd) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		string x;

		int i = ctrlClient.CharFromPos(pt);
		int line = ctrlClient.LineFromChar(i);
		int c = LOWORD(i) - ctrlClient.LineIndex(line);
		int len = ctrlClient.LineLength(i) + 1;
		if(len < 3) {
			return 0;
		}

		char* buf = new char[len];
		ctrlClient.GetLine(line, buf, len);
		x = string(buf, len-1);
		delete buf;

		string::size_type start = x.find_last_of(" <\t\r\n", c);
		if(start == string::npos)
			start = 0;
		else
			start++;
					
		if(Util::strnicmp(x.c_str() + start, "dchub://", 8) == 0) {

			bHandled = true;

			string server, file;
			short port = 411;
			Util::decodeUrl((x.c_str() + start), server, port, file);
			HubFrame::openWindow(server + ":" + Util::toString(port));
		}else if(Util::strnicmp(x.c_str() + start, "magnet:?", 8) == 0) {
			string::size_type i = 0;
			i = start + 8;
			string tth;
			if( (start = x.find("xt=urn:tree:tiger:", i)) != string::npos) {
				tth = x.substr(start + 18, 39);
				if(Encoder::isBase32(tth.c_str()))
					SearchFrame::openWindow(tth, 0, SearchManager::SIZE_DONTCARE, SearchManager::TYPE_HASH);
			} else if( (start = x.find("xt=urn:bitprint:", i)) != string::npos) {
				i = start + 16;
				if( (start = x.find(".", i)) != string::npos) {
					tth = x.substr(start + 1, 39);
					if(Encoder::isBase32(tth.c_str()))
						SearchFrame::openWindow(tth, 0, SearchManager::SIZE_DONTCARE, SearchManager::TYPE_HASH);
				}
			}
		} else {
			string::size_type end = x.find_first_of(" >\t", start+1);

			if(end == string::npos) // get EOL as well
				end = x.length();
			else if(end == start + 1)
				return 0;

			// Nickname click, let's see if we can find one like it in the name list...
			int pos = ctrlUsers.findItem(x.substr(start, end - start));
			if(pos != -1) {
				bHandled = true;
				if (wParam & MK_CONTROL) { // MK_CONTROL = 0x0008
					PrivateFrame::openWindow((ctrlUsers.getItemData(pos))->user);
				} else if (wParam & MK_SHIFT) {
					try {
						QueueManager::getInstance()->addList((ctrlUsers.getItemData(pos))->user, QueueItem::FLAG_CLIENT_VIEW);
					} catch(const Exception& e) {
						addClientLine(e.getError(), m_ChatTextSystem);
					}
				} else {
					int items = ctrlUsers.GetItemCount();
					ctrlUsers.SetRedraw(FALSE);
					for(int i = 0; i < items; ++i) {
						ctrlUsers.SetItemState(i, (i == pos) ? LVIS_SELECTED | LVIS_FOCUSED : 0, LVIS_SELECTED | LVIS_FOCUSED);
					}
					ctrlUsers.SetRedraw(TRUE);
					ctrlUsers.EnsureVisible(pos, FALSE);
				}
			}
		}
	}
	return 0;
}

void HubFrame::addLine(const string& aLine) {
	addLine( aLine, m_ChatTextGeneral );
}

void HubFrame::addLine(const string& aLine, CHARFORMAT2& cf) {
	ctrlClient.AdjustTextSize();

	string sTmp = aLine;
	string sAuthor = "";
	if (aLine.find("<") == 0) {
		string::size_type i = aLine.find(">");
		if (i != string::npos) {
       		sAuthor = aLine.substr(1, i-1);
			if ( strnicmp(" /me ", aLine.substr(i+1, 5).c_str(), 5) == 0 ) {
				sTmp = "* " + sAuthor + aLine.substr(i+5);
			}
		}
	}
	sMyNick = client->getNick().c_str();
	if(BOOLSETTING(LOG_MAIN_CHAT)) {
		StringMap params;
		params["message"] = sTmp;
		LOG(client->getAddressPort(), Util::formatParams(SETTING(LOG_FORMAT_MAIN_CHAT), params));
	}
	if(timeStamps) {
		ctrlClient.AppendText(sMyNick, ("[" + Util::getShortTimeString() + "] ").c_str(), sTmp.c_str(), cf, sAuthor.c_str() );
	} else {
		ctrlClient.AppendText(sMyNick, "", sTmp.c_str(), cf, sAuthor.c_str() );
	}
	setDirty();
}

LRESULT HubFrame::onTabContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 
	tabMenuShown = true;
	CMenu hSysMenu;
	tabMenu.InsertSeparatorFirst((client->getName() != "") ? (client->getName().size() > 50 ? client->getName().substr(0, 50) : client->getName()) : client->getAddressPort());	
	prepareMenu(tabMenu, ::UserCommand::CONTEXT_HUB, client->getAddressPort(), client->getOp());
	hSysMenu.Attach((wParam == NULL) ? (HMENU)tabMenu : (HMENU)wParam);
	if (wParam != NULL) {
		hSysMenu.InsertMenu(hSysMenu.GetMenuItemCount() - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)(HMENU)tabMenu, /*CSTRING(USER_COMMANDS)*/ "User Commands");
		hSysMenu.InsertMenu(hSysMenu.GetMenuItemCount() - 1, MF_BYPOSITION | MF_SEPARATOR, 0, (LPCTSTR)NULL);
	}
	hSysMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	if (wParam != NULL) {
		hSysMenu.RemoveMenu(hSysMenu.GetMenuItemCount() - 2, MF_BYPOSITION);
		hSysMenu.RemoveMenu(hSysMenu.GetMenuItemCount() - 2, MF_BYPOSITION);
	}
	cleanMenu(tabMenu);	
	tabMenu.RemoveFirstItem();
	hSysMenu.Detach();
	return TRUE;
}

LRESULT HubFrame::onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
		RECT rc;                    // client area of window 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click
		POINT ptCl;
		tabMenuShown = false;
		OMenu Mnu;
		string sU = "";
	
		sSelectedUser = "";
		sSelectedLine = "";
		sSelectedIP = "";

		ctrlUsers.GetClientRect(&rc);
		ptCl = pt;
		ctrlUsers.ScreenToClient(&ptCl);
	
		if (PtInRect(&rc, ptCl) && ShowUserList) { 
			if ( ctrlUsers.GetSelectedCount() == 1 ) {
				int i = -1;
				i = ctrlUsers.GetNextItem(i, LVNI_SELECTED);
				if ( i >= 0 ) {
				sSelectedUser = sU = ((UserInfo*)ctrlUsers.getItemData(i))->user->getNick();
				}
			}

			if ( PreparePopupMenu( &ctrlUsers, false, sU, &Mnu ) ) {
				prepareMenu(Mnu, ::UserCommand::CONTEXT_CHAT, client->getAddressPort(), client->getOp());
				if(!(Mnu.GetMenuState(Mnu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {	
					Mnu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
				}
				Mnu.AppendMenu(MF_STRING, IDC_REFRESH, CSTRING(REFRESH_USER_LIST));
				Mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
				cleanMenu(Mnu);
				if(copyMenu != NULL) copyMenu.DestroyMenu();
				if(grantMenu != NULL) grantMenu.DestroyMenu();
				if(Mnu != NULL) Mnu.DestroyMenu();
				return TRUE; 
			}
		}

		// Get the bounding rectangle of the client area. 
		ctrlClient.GetClientRect(&rc);
		ptCl = pt;
		ctrlClient.ScreenToClient(&ptCl); 
	
		if (PtInRect(&rc, ptCl)) { 
			CAtlString sUser;

			sSelectedLine = ctrlClient.LineFromPos( ptCl );

			// Klik na nick ? A existuje zjisteny nick ?

			bool boHitNick = ctrlClient.HitNick( ptCl, &sUser );
			if ( boHitNick ) 
			sSelectedUser = sUser;

			CAtlString sIP;
			bool boHitIP = ctrlClient.HitIP( ptCl, &sIP );
			if ( boHitIP )
				sSelectedIP = sIP;

			bool boHitURL = ctrlClient.HitURL(ptCl);
			if ( !boHitURL )
				sSelectedURL = "";

			sU = boHitNick ? sSelectedUser : "";
			if ( PreparePopupMenu( &ctrlClient, !boHitNick, sU, &Mnu ) ) {
				if (sU == "") {
					Mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
					if(copyMenu != NULL) copyMenu.DestroyMenu();
					if(grantMenu != NULL) grantMenu.DestroyMenu();
					if(Mnu != NULL) Mnu.DestroyMenu();
					return TRUE;
				} else {
					prepareMenu(Mnu, ::UserCommand::CONTEXT_CHAT, client->getAddressPort(), client->getOp());
					if(!(Mnu.GetMenuState(Mnu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {	
						Mnu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
					}
					Mnu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CSTRING(CLEAR));
					Mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
					cleanMenu(Mnu);
					if(copyMenu != NULL) copyMenu.DestroyMenu();
					if(grantMenu != NULL) grantMenu.DestroyMenu();
					if(Mnu != NULL) Mnu.DestroyMenu();
					return TRUE;
				}
			}
		}
		return FALSE; 
}

void HubFrame::runUserCommand(::UserCommand& uc) {
	if(!WinUtil::getUCParams(m_hWnd, uc, ucParams))
		return;

	ucParams["mynick"] = client->getNick();

	if(tabMenuShown) {
		client->send(Util::formatParams(uc.getCommand(), ucParams));
	} else {
		int sel;
		UserInfo* u = NULL;
		if (sSelectedUser != "") {
			sel = ctrlUsers.findItem( sSelectedUser );
			if ( sel >= 0 ) { 
				u = (UserInfo*)ctrlUsers.getItemData(sel);
				if(u->user->isOnline()) {
					u->user->getParams(ucParams);
					client->send(Util::formatParams(uc.getCommand(), ucParams));
				}
			}
		} else {
			sel = -1;
			while((sel = ctrlUsers.GetNextItem(sel, LVNI_SELECTED)) != -1) {
				u = (UserInfo*) ctrlUsers.getItemData(sel);
				if(u->user->isOnline()) {
					u->user->getParams(ucParams);
					client->send(Util::formatParams(uc.getCommand(), ucParams));
				}
			}
		}
	}
	return;
};

void HubFrame::onTab() {
	if(	BOOLSETTING(TAB_COMPLETION) && 
		(GetFocus() == ctrlMessage.m_hWnd) && 
		!(GetAsyncKeyState(VK_SHIFT) & 0x8000) ) 
	{
		int n = ctrlMessage.GetWindowTextLength();
		AutoArray<char> buf(n+1);
		ctrlMessage.GetWindowText(buf, n+1);
		string text(buf, n);
		string::size_type textStart = text.find_last_of(" \n\t");

		if(complete.empty()) {
			if(textStart != string::npos) {
				complete = text.substr(textStart + 1);
			} else {
				complete = text;
			}
			if(complete.empty()) {
				// Still empty, no text entered...
				return;
			}
			int y = ctrlUsers.GetItemCount();

			for(int x = 0; x < y; ++x)
				ctrlUsers.SetItemState(x, 0, LVNI_FOCUSED | LVNI_SELECTED);
		}

		if(textStart == string::npos)
			textStart = 0;
		else
			textStart++;

		int start = ctrlUsers.GetNextItem(-1, LVNI_FOCUSED) + 1;
		int i = start;
		int j = ctrlUsers.GetItemCount();

		bool firstPass = i < j;
		if(!firstPass)
			i = 0;
		while(firstPass || (!firstPass && i < start)) {
			UserInfo* ui = ctrlUsers.getItemData(i);
			const string& nick = ui->user->getNick();
			bool found = (Util::strnicmp(nick, complete, complete.length()) == 0);
			string::size_type x = string::npos;
			if(!found) {
				// Check if there's one or more [ISP] tags to ignore...
				string::size_type y = 0;
				while(nick[y] == '[') {
					x = safestring::SafeFind(nick, ']', y);
					if(x != string::npos) {
						if(Util::strnicmp(nick.c_str() + x + 1, complete.c_str(), complete.length()) == 0) {
							found = true;
							break;
						}
					} else {
						break;
					}
					y = x + 1; // assuming that nick[y] == '\0' is legal
				}
			}
			if(found) {
				if((start - 1) != -1) {
					ctrlUsers.SetItemState(start - 1, 0, LVNI_SELECTED | LVNI_FOCUSED);
				}
				ctrlUsers.SetItemState(i, LVNI_FOCUSED | LVNI_SELECTED, LVNI_FOCUSED | LVNI_SELECTED);
				ctrlUsers.EnsureVisible(i, FALSE);
				ctrlMessage.SetSel(textStart, ctrlMessage.GetWindowTextLength(), TRUE);
				ctrlMessage.ReplaceSel(ui->user->getNick().c_str());
				return;
			}
			i++;
			if(i == j) {
				firstPass = false;
				i = 0;
			}
		}
	} else {
		HWND focus = GetFocus();

		if(focus == ctrlClient.m_hWnd) {
			ctrlMessage.SetFocus();
		} else if(focus == ctrlMessage.m_hWnd) {
			ctrlUsers.SetFocus();
		} else if(focus == ctrlUsers.m_hWnd) {
			ctrlFilter.SetFocus();
		} else if(focus == ctrlFilter.m_hWnd) {
			ctrlFilterBy.SetFocus();
		} else if(focus == ctrlFilterBy.m_hWnd) {
			ctrlClient.SetFocus();
		} 
	}
}

LRESULT HubFrame::onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
//	if (GetFocus() != ctrlMessage.m_hWnd) { // don't handle these keys unless the user is entering a message
//		bHandled = FALSE;
//		return 0;
//	}
	if(!complete.empty() && wParam != VK_TAB && uMsg == WM_KEYDOWN)
		complete.clear();

	if (uMsg != WM_KEYDOWN) {
	switch(wParam) {
			case VK_RETURN:
				if( (GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000) ) {
					bHandled = FALSE;
				}
				break;
		case VK_TAB:
				bHandled = TRUE;
  				break;
  			default:
  				bHandled = FALSE;
				break;
			}
		return 0;
			}

	if(wParam == VK_TAB) {
		onTab();
		return 0;
	} else if ( wParam == VK_ESCAPE ){
		// Clear find text and give the focus back to the message box
		ctrlMessage.SetFocus();
		ctrlClient.SetSel(-1, -1);
		ctrlClient.SendMessage(EM_SCROLL, SB_BOTTOM, 0);
		ctrlClient.InvalidateRect(NULL);
		currentNeedle="";
	} else if( wParam == VK_F3 && ( GetKeyState(VK_LSHIFT)&(0x80) )  ) {
		currentNeedle=findTextPopup();
		findText(currentNeedle);
		return 0;
	}else if(wParam == VK_F3) {
		string whattofind;
		whattofind=currentNeedle;
		if (whattofind.empty())
			whattofind=findTextPopup();
		findText(whattofind);
		return 0;
	}

	// don't handle these keys unless the user is entering a message
	if (GetFocus() != ctrlMessage.m_hWnd) {
		bHandled = FALSE;
		return 0;
	}

	switch(wParam) {
		case VK_RETURN:
			if( (GetKeyState(VK_CONTROL) & 0x8000) || 
				(GetKeyState(VK_MENU) & 0x8000) ) {
					bHandled = FALSE;
				} else {
						onEnter();
					}
			break;
		case VK_UP:
			if ((GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
				//scroll up in chat command history
				//currently beyond the last command?
				if (curCommandPosition > 0) {
					//check whether current command needs to be saved
					if (curCommandPosition == prevCommands.size()) {
						auto_ptr<char> messageContents(new char[ctrlMessage.GetWindowTextLength()+2]);
						ctrlMessage.GetWindowText(messageContents.get(), ctrlMessage.GetWindowTextLength()+1);
						currentCommand = string(messageContents.get());
					}

					//replace current chat buffer with current command
					ctrlMessage.SetWindowText(prevCommands[--curCommandPosition].c_str());
				}
			} else {
				bHandled = FALSE;
			}

			break;
		case VK_DOWN:
			if ((GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
				//scroll down in chat command history

				//currently beyond the last command?
				if (curCommandPosition + 1 < prevCommands.size()) {
					//replace current chat buffer with current command
					ctrlMessage.SetWindowText(prevCommands[++curCommandPosition].c_str());
				} else if (curCommandPosition + 1 == prevCommands.size()) {
					//revert to last saved, unfinished command

					ctrlMessage.SetWindowText(currentCommand.c_str());
					++curCommandPosition;
				}
			} else {
				bHandled = FALSE;
			}

			break;
		case VK_HOME:
			if (!prevCommands.empty() && (GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
				curCommandPosition = 0;
				
				auto_ptr<char> messageContents(new char[ctrlMessage.GetWindowTextLength()+2]);
				ctrlMessage.GetWindowText(messageContents.get(), ctrlMessage.GetWindowTextLength()+1);
				currentCommand = string(messageContents.get());

				ctrlMessage.SetWindowText(prevCommands[curCommandPosition].c_str());
			} else {
				bHandled = FALSE;
			}

			break;
		case VK_END:
			if ((GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
				curCommandPosition = prevCommands.size();

				ctrlMessage.SetWindowText(currentCommand.c_str());
			} else {
				bHandled = FALSE;
				}
				break;
		default:
			bHandled = FALSE;
	}
	return 0;
}

LRESULT HubFrame::onFilterChar(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	bHandled = (wParam == -1 && lParam == -1) ? TRUE : FALSE;
	iFilterBySel = ctrlFilterBy.GetCurSel();
	if (iFilterBySel == -1)
		return S_OK;
	// Get the text from the edit box
	int len = ctrlFilter.GetWindowTextLength() + 1;
	char *c = new char[len];
	ctrlFilter.GetWindowText(c, len);
	string s(c, len-1);
	delete[] c;
	// Tokenize the text
	stFilter = StringTokenizer(s, ' ');
	// Update E-v-e-r-y user in the list
	updateEntireUserList();
	return S_OK;
}
LRESULT HubFrame::onFilterCharDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	if (wParam != VK_TAB) {
		bHandled = FALSE;
		return S_OK;
	}
	bHandled = TRUE;
	onTab();
	return S_OK;
}
LRESULT HubFrame::onFilterByChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	return onFilterChar(0, 0, 0, bHandled);
}
LRESULT HubFrame::onFilterClipboard(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	bHandled = FALSE;
	::PostMessage(ctrlFilter.m_hWnd, WM_KEYUP, (WPARAM)-1, -1);
	return S_OK;
}

LRESULT HubFrame::onShowUsers(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	bHandled = FALSE;
	if((wParam == BST_CHECKED)) {
		ShowUserList = true;
		updateEntireUserList();
	} else {
		ShowUserList = false;
		clearUserList();
	}

	UpdateLayout(FALSE);
	return 0;
}

LRESULT HubFrame::onFollow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	if(!redirect.empty()) {
		string s, f;
		short p = 411;
		Util::decodeUrl(redirect, s, p, f);
		if(ClientManager::getInstance()->isConnected(s, p)) {
			addClientLine(STRING(REDIRECT_ALREADY_CONNECTED), m_ChatTextServer);
			return 0;
		}
		
		dcassert(frames.find(server) != frames.end());
		dcassert(frames[server] == this);
		frames.erase(server);
		server = redirect;
		frames[server] = this;

		// Is the redirect hub a favorite? Then honor settings for it.
		FavoriteHubEntry* hub = HubManager::getInstance()->getFavoriteHubEntry(server);
		if(hub) {
			client->setNick(hub->getNick(true));
			client->setDescription(hub->getUserDescription());
			client->setPassword(hub->getPassword());
			client->setStealth(hub->getStealth());
		}
		// else keep current settings

		RecentHubEntry r;
		r.setName("*");
		r.setDescription("***");
		r.setUsers("*");
		r.setShared("*");
		r.setServer(redirect);
		HubManager::getInstance()->addRecent(r);

		client->addListener(this);
		//client->connect(redirect);
	}
	return 0;
}

LRESULT HubFrame::onEnterUsers(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
	int item = ctrlUsers.GetNextItem(-1, LVNI_FOCUSED);
	if(client->isConnected() && (item != -1)) {
		try {
			QueueManager::getInstance()->addList((ctrlUsers.getItemData(item))->user, QueueItem::FLAG_CLIENT_VIEW);
		} catch(const Exception& e) {
			addClientLine(e.getError());
		}
	}
	return 0;
}

LRESULT HubFrame::onGetToolTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMTTDISPINFO* nm = (NMTTDISPINFO*)pnmh;
	lastLines.clear();
	for(StringIter i = lastLinesList.begin(); i != lastLinesList.end(); ++i) {
		lastLines += *i;
		lastLines += "\r\n";
	}
	if(lastLines.size() > 2) {
		lastLines.erase(lastLines.size() - 2);
	}
	nm->lpszText = const_cast<char*>(lastLines.c_str());
	return 0;
}

void HubFrame::addClientLine(const string& aLine, bool inChat /* = true */) {
	string line = "[" + Util::getShortTimeString() + "] " + aLine;

	ctrlStatus.SetText(0, line.c_str());
	while(lastLinesList.size() + 1 > MAX_CLIENT_LINES)
		lastLinesList.erase(lastLinesList.begin());
	lastLinesList.push_back(line);

	setDirty();
	
	if(BOOLSETTING(STATUS_IN_CHAT) && inChat) {
		addLine("*** " + aLine, m_ChatTextSystem);
	}
	if(BOOLSETTING(LOG_STATUS_MESSAGES) && inChat /*&& BOOLSETTING(FILTER_MESSAGES)*/) {
		LOGDT(client->getAddressPort() + "_Status", aLine);
	}
}

void HubFrame::closeDisconnected() {
	for(FrameIter i=frames.begin(); i!= frames.end(); ++i) {
		if (!(i->second->client->isConnected())) {
			i->second->PostMessage(WM_CLOSE);
		}
	}
};

void HubFrame::on(TimerManagerListener::Second, DWORD /*aTick*/) throw() {
		updateStatusBar();
		if(updateUsers) {
			updateUsers = false;
			PostMessage(WM_SPEAKER, UPDATE_USERS);
		}
}

void HubFrame::on(Connecting, Client*) throw() { 
			if(BOOLSETTING(SEARCH_PASSIVE) && (SETTING(CONNECTION_TYPE) == SettingsManager::CONNECTION_ACTIVE)) {
				addLine(STRING(ANTI_PASSIVE_SEARCH), m_ChatTextSystem);
			}
			speak(ADD_STATUS_LINE, STRING(CONNECTING_TO) + client->getAddressPort() + "...");
			speak(SET_WINDOW_TITLE, client->getAddressPort());
}
void HubFrame::on(Connected, Client*) throw() { 
	speak(CONNECTED);
}
void HubFrame::on(BadPassword, Client*) throw() { 
	client->setPassword(Util::emptyString);
}
void HubFrame::on(UserUpdated, Client*, const User::Ptr& user) throw() { 
	if(getUserInfo() && !user->isSet(User::HIDDEN)) 
		speak(UPDATE_USER, user);
}
void HubFrame::on(UsersUpdated, Client*, const User::List& aList) throw() {
	Lock l(updateCS);
	updateList.reserve(aList.size());
	for(User::List::const_iterator i = aList.begin(); i != aList.end(); ++i) {
		if(!(*i)->isSet(User::HIDDEN))
			updateList.push_back(make_pair(*i, UPDATE_USERS));
	}
	if(!updateList.empty()) {
		PostMessage(WM_SPEAKER, UPDATE_USERS);
	}
}

void HubFrame::on(UserRemoved, Client*, const User::Ptr& user) throw() {
	if(getUserInfo()) 
		speak(REMOVE_USER, user);
}

void HubFrame::on(Redirect, Client*, const string& line) throw() { 
	string s, f;
	short p = 411;
	Util::decodeUrl(line, s, p, f);
	if(ClientManager::getInstance()->isConnected(s, p)) {
		speak(ADD_STATUS_LINE, STRING(REDIRECT_ALREADY_CONNECTED));
		return;
	}

	redirect = line;
	if(BOOLSETTING(AUTO_FOLLOW)) {
		PostMessage(WM_COMMAND, IDC_FOLLOW, 0);
	} else {
		speak(ADD_STATUS_LINE, STRING(PRESS_FOLLOW) + line);
	}
}
void HubFrame::on(Failed, Client*, const string& line) throw() { 
	speak(ADD_STATUS_LINE, line); 
	speak(DISCONNECTED); 
}
void HubFrame::on(GetPassword, Client*) throw() { 
	speak(GET_PASSWORD);
}
void HubFrame::on(HubUpdated, Client*) throw() { 
	speak(SET_WINDOW_TITLE, Util::validateMessage(client->getName(), true, false) + " (" + client->getAddressPort() + ")");
}
void HubFrame::on(Message, Client*, const string& line) throw() { 
	if(SETTING(FILTER_MESSAGES)) {
		if((line.find("Hub-Security") != string::npos) && (line.find("was kicked by") != string::npos)) {
			// Do nothing...
		} else if((line.find("is kicking") != string::npos) && (line.find("because:") != string::npos)) {
			speak(ADD_SILENT_STATUS_LINE, line);
		} else {
			speak(ADD_CHAT_LINE, line);
		}
	} else {
		speak(ADD_CHAT_LINE, line);
	}
}
void HubFrame::on(PrivateMessage, Client*, const User::Ptr& user, const string& line) throw() { 
	speak(PRIVATE_MESSAGE, user, line);
}
void HubFrame::on(NickTaken, Client*) throw() { 
	speak(ADD_STATUS_LINE, STRING(NICK_TAKEN));
	speak(DISCONNECTED);
}
void HubFrame::on(SearchFlood, Client*, const string& line) throw() {
	speak(ADD_STATUS_LINE, STRING(SEARCH_SPAM_FROM) + line);
}
void HubFrame::on(CheatMessage, Client*, const string& line) throw() {
				CHARFORMAT2 cf;
				memset(&cf, 0, sizeof(CHARFORMAT2));
				cf.cbSize = sizeof(cf);
				cf.dwReserved = 0;
				cf.dwMask = CFM_BACKCOLOR | CFM_COLOR | CFM_BOLD;
				cf.dwEffects = 0;
				cf.crBackColor = SETTING(BACKGROUND_COLOR);
				cf.crTextColor = SETTING(ERROR_COLOR);
				HubFrame::addLine("*** "+STRING(USER)+" "+line, cf);
}

void HubFrame::updateEntireUserList() {
	Lock l(updateCS);
	User::NickMap& um = client->lockUserList();
	updateList.reserve(um.size());
	for(User::NickMap::const_iterator i = um.begin(); i != um.end(); ++i) {
		updateList.push_back(make_pair(i->second, UPDATE_USERS));
	}
	client->unlockUserList();
	if(!updateList.empty()) {
		PostMessage(WM_SPEAKER, UPDATE_USERS);
	}
}


void HubFrame::addClientLine(const string& aLine, CHARFORMAT2& cf, bool inChat /* = true */) {
	string line = "[" + Util::getShortTimeString() + "] " + aLine;

	ctrlStatus.SetText(0, line.c_str());
	while(lastLinesList.size() + 1 > MAX_CLIENT_LINES)
		lastLinesList.erase(lastLinesList.begin());
	lastLinesList.push_back(line);
	
	setDirty();
	
	if(BOOLSETTING(STATUS_IN_CHAT) && inChat) {
		addLine("*** " + aLine, cf);
	}
}

LRESULT HubFrame::onMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	RECT rc;
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click
	// Get the bounding rectangle of the client area. 
	ctrlClient.GetClientRect(&rc);
	ctrlClient.ScreenToClient(&pt); 
	if (PtInRect(&rc, pt)) { 
		sSelectedURL = "";
  		return TRUE;
	}
	return 1;
}


BOOL HubFrame::checkCheating(User::Ptr &user, DirectoryListing* dl) {

	int64_t statedSize = dl->getUser()->getBytesShared();
	int64_t realSize = dl->getTotalSize();
	int64_t junkSize = 0;
	
	if(!BOOLSETTING(IGNORE_JUNK_FILES)) {
		junkSize = dl->getJunkSize();
		realSize -= junkSize;
	}
			
			user->setChecked(true);
			double multiplier = ((100+(double)SETTING(PERCENT_FAKE_SHARE_TOLERATED))/100); 
			int64_t sizeTolerated = (int64_t)(realSize*multiplier);
			string detectString = "";
			string inflationString = "";
			user->setJunkBytesShared(junkSize);
			user->setRealBytesShared(realSize);
			bool isFakeSharing = false;

			if((junkSize > 0) && (!BOOLSETTING(IGNORE_JUNK_FILES)))
			{
				isFakeSharing = true;
			}

			if(statedSize > sizeTolerated)
			{
				isFakeSharing = true;
			}

			if(Util::toString(statedSize).find("000000") != -1)
			{
				detectString = Util::formatBytes(statedSize)+" - the share size had too many zeroes in it";
				isFakeSharing = true;
			}

			if(isFakeSharing)
			{
				user->setFakeSharing(true);
				detectString += STRING(CHECK_MISMATCHED_SHARE_SIZE);
				if(realSize == 0)
				{
					detectString += STRING(CHECK_0BYTE_SHARE);
				}
				else
				{
					double qwe = (double)((double)statedSize / (double)realSize);
					string str = Util::toString(qwe);
					char buf[128];
					sprintf(buf, CSTRING(CHECK_INFLATED), str);
					inflationString = buf;
					detectString += inflationString;
				}
				detectString += STRING(CHECK_SHOW_REAL_SHARE);

				if((junkSize > 0) && (!BOOLSETTING(IGNORE_JUNK_FILES)))
				{
					detectString = STRING(CHECK_JUNK_FILES);
				}

				detectString = user->insertUserData(detectString);
				user->setCheatingString(Util::validateMessage(detectString, false));

				if (!SETTING(FAKERFILE).empty())
					PlaySound(SETTING(FAKERFILE).c_str(), NULL, SND_FILENAME | SND_ASYNC);								

			}     

			this->updateUser(user);
			if(isFakeSharing) return true; else	return false;

}

LRESULT HubFrame::onRButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	RECT rc;
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	// Get the bounding rectangle of the client area. 
	ctrlClient.GetClientRect(&rc);
	if (PtInRect(&rc, pt)) { 
		// Po kliku dovnitr oznaceneho textu nedelat nic
		long lBegin, lEnd;
		ctrlClient.GetSel( lBegin, lEnd );
		int iCharPos = ctrlClient.CharFromPos( pt );
		if ( ( lEnd > lBegin ) && ( iCharPos >= lBegin ) && ( iCharPos <= lEnd ) ) {
			return 1;
		}

		// Po kliku do IP oznacit IP
		CAtlString sSel;
		int iBegin = 0, iEnd = 0;
		if ( ctrlClient.HitIP( pt, &sSel, &iBegin, &iEnd ) ) {
			ctrlClient.SetSel( iBegin, iEnd );
			ctrlClient.InvalidateRect( NULL );
		} else if ( ctrlClient.HitNick( pt, &sSel, &iBegin, &iEnd ) ) {
			ctrlClient.SetSel( iBegin, iEnd );
			ctrlClient.InvalidateRect( NULL );
		}

		// Po kliku do nicku uvnitr <> oznacit nick

		sSelectedURL = "";
	}
	return 1;
}

LRESULT HubFrame::onRButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	RECT rc;
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click

	// Get the bounding rectangle of the client area. 
	ctrlClient.GetClientRect(&rc);
	ctrlClient.ScreenToClient(&pt); 
	if (PtInRect(&rc, pt)) { 
		sSelectedURL = "";
	}
	return 1;
}

bool HubFrame::PreparePopupMenu( CWindow *pCtrl, bool boCopyOnly, string& sNick, OMenu *pMenu ) {
	if (copyMenu.m_hMenu != NULL) {
		copyMenu.DestroyMenu();
		copyMenu.m_hMenu = NULL;
	}
	if (grantMenu.m_hMenu != NULL) {
		grantMenu.DestroyMenu();
		grantMenu.m_hMenu = NULL;
	}
	if (pMenu->m_hMenu != NULL) {
		pMenu->DestroyMenu();
		pMenu->m_hMenu = NULL;
	}

	copyMenu.CreatePopupMenu();
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_NICK, CSTRING(COPY_NICK));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_EXACT_SHARE, CSTRING(COPY_EXACT_SHARE));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_DESCRIPTION, CSTRING(COPY_DESCRIPTION));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_TAG, CSTRING(COPY_TAG));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_EMAIL_ADDRESS, CSTRING(COPY_EMAIL_ADDRESS));
	if ( client->getOp() ) { copyMenu.AppendMenu(MF_STRING, IDC_COPY_IP, CSTRING(COPY_IP)); }
	if ( client->getOp() ) { copyMenu.AppendMenu(MF_STRING, IDC_COPY_NICK_IP, CSTRING(COPY_NICK_IP)); }
	if ( client->getOp() ) { copyMenu.AppendMenu(MF_STRING, IDC_COPY_ISP, CSTRING(COPY_ISP)); }
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_ALL, CSTRING(COPY_ALL));
	copyMenu.InsertSeparator(0, TRUE, STRING(COPY));

	grantMenu.CreatePopupMenu();
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CSTRING(GRANT_EXTRA_SLOT));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_HOUR, CSTRING(GRANT_EXTRA_SLOT_HOUR));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_DAY, CSTRING(GRANT_EXTRA_SLOT_DAY));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_WEEK, CSTRING(GRANT_EXTRA_SLOT_WEEK));
	grantMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	grantMenu.AppendMenu(MF_STRING, IDC_UNGRANTSLOT, CSTRING(REMOVE_EXTRA_SLOT));
	grantMenu.InsertSeparator(0, TRUE, STRING(GRANT_SLOTS_MENU));

	pMenu->CreatePopupMenu();
		
	if ( boCopyOnly ) {
		if ( sSelectedIP != "" ) {
			pMenu->InsertSeparator(0, TRUE, sSelectedIP);
			pMenu->AppendMenu(MF_STRING, IDC_WHOIS_IP, ( (CSTRING(WHO_IS)) + sSelectedIP ).c_str() );
			if ( client->getOp() ) {
				pMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
				pMenu->AppendMenu(MF_STRING, IDC_BAN_IP, ( "!ban " + sSelectedIP ).c_str() );
				pMenu->SetMenuDefaultItem( IDC_BAN_IP );
				pMenu->AppendMenu(MF_STRING, IDC_UNBAN_IP, ( "!unban " + sSelectedIP ).c_str() );
				pMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
			}
		} else pMenu->InsertSeparator(0, TRUE, "Text");
		pMenu->AppendMenu(MF_STRING, ID_EDIT_COPY, CSTRING(COPY));
		pMenu->AppendMenu(MF_STRING, IDC_COPY_ACTUAL_LINE,  CSTRING(COPY_LINE));
		if ( sSelectedURL != "" ) 
  			pMenu->AppendMenu(MF_STRING, IDC_COPY_URL, CSTRING(COPY_URL));
		pMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
		pMenu->AppendMenu(MF_STRING, ID_EDIT_SELECT_ALL, CSTRING(SELECT_ALL));
		pMenu->AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CSTRING(CLEAR));
		pMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
		pMenu->AppendMenu(MF_STRING, IDC_AUTOSCROLL_CHAT, CSTRING(ASCROLL_CHAT));

		if ( ctrlClient.GetAutoScroll() )
			pMenu->CheckMenuItem( IDC_AUTOSCROLL_CHAT, MF_BYCOMMAND | MF_CHECKED );
	} else {
		if ( sNick != "" ) {
			// Jediny nick
			string sTmp = STRING(USER) + " " + sNick;
			pMenu->InsertSeparator(0, TRUE, sTmp);
			if ( pCtrl == ( (CWindow*) &ctrlClient ) ) {
				if(!BOOLSETTING(LOG_PRIVATE_CHAT)) {
					//pMenu->AppendMenu(MF_STRING | MF_DISABLED, 0, sTmp.c_str());
				} else {
					pMenu->AppendMenu(MF_STRING, IDC_OPEN_USER_LOG, sTmp.c_str());
					pMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
				}				
			}
			if ( pCtrl != ( (CWindow*) &ctrlClient ) ) {
				pMenu->AppendMenu(MF_STRING, IDC_OPEN_USER_LOG, CSTRING(OPEN_USER_LOG));
				pMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
			}
			pMenu->AppendMenu(MF_STRING, IDC_PUBLIC_MESSAGE, CSTRING(SEND_PUBLIC_MESSAGE));
			if ( sNick != sMyNick ) {
				pMenu->AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CSTRING(SEND_PRIVATE_MESSAGE));
			}
			if ( pCtrl == ( (CWindow*) &ctrlClient ) ) {
				pMenu->AppendMenu(MF_STRING, IDC_SELECT_USER, CSTRING(SELECT_USER_LIST));
			}
			pMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
			pMenu->AppendMenu(MF_POPUP, (UINT)(HMENU)copyMenu, CSTRING(COPY));
			pMenu->AppendMenu(MF_POPUP, (UINT)(HMENU)grantMenu, CSTRING(GRANT_SLOTS_MENU));
			if ( sNick != sMyNick ) {
				pMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
				pMenu->AppendMenu(MF_STRING, IDC_IGNORE, CSTRING(IGNORE_USER));
				pMenu->AppendMenu(MF_STRING, IDC_UNIGNORE, CSTRING(UNIGNORE_USER));
			}
		} else {
			// Muze byt vice nicku
			if ( pCtrl == ( (CWindow*) &ctrlUsers ) ) {
				// Pocet oznacenych
				int iCount = ctrlUsers.GetSelectedCount();
				string sTmp = Util::toString(iCount)+ " " + CSTRING(HUB_USERS);
				pMenu->InsertSeparator(0, TRUE, sTmp);
			}
			if (ctrlUsers.GetSelectedCount() <= 25) {
				pMenu->AppendMenu(MF_STRING, IDC_PUBLIC_MESSAGE, CSTRING(SEND_PUBLIC_MESSAGE));
				pMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
			}
		}		
		pMenu->AppendMenu(MF_STRING, IDC_GETLIST, CSTRING(GET_FILE_LIST));
		pMenu->AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CSTRING(MATCH_QUEUE));
		pMenu->AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CSTRING(ADD_TO_FAVORITES));
		pMenu->SetMenuDefaultItem( IDC_GETLIST );
//		pMenu->AppendMenu(MF_SEPARATOR);
//		pMenu->AppendMenu(MF_STRING, IDC_GET_USER_RESPONSES, CSTRING(GET_USER_RESPONSES));
//		pMenu->AppendMenu(MF_STRING, IDC_REPORT, CSTRING(REPORT));
//		pMenu->AppendMenu(MF_STRING, IDC_CHECKLIST, "Check File List");
	}
	return true;
}

LRESULT HubFrame::onCopyActualLine(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if ( ( GetFocus() == ctrlClient.m_hWnd ) && ( sSelectedLine != "" ) ) {
		WinUtil::setClipboard( sSelectedLine );
	}
	return 0;
}

LRESULT HubFrame::onSelectUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (sSelectedUser == "") {
		// No nick selected
		return 0;
	}

	int pos = ctrlUsers.findItem(sSelectedUser);
	if ( pos == -1 ) {
		// User not found is list
		return 0;
	}

	int items = ctrlUsers.GetItemCount();
	ctrlUsers.SetRedraw(FALSE);
	for(int i = 0; i < items; ++i) {
		ctrlUsers.SetItemState(i, (i == pos) ? LVIS_SELECTED | LVIS_FOCUSED : 0, LVIS_SELECTED | LVIS_FOCUSED);
	}
	ctrlUsers.SetRedraw(TRUE);
	ctrlUsers.EnsureVisible(pos, FALSE);
	return 0;
}

LRESULT HubFrame::onPublicMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	CAtlString sUsers = "";
	CAtlString sText = "";

	if ( !client->isConnected() )
		return 0;

	if (sSelectedUser != "") {
		sUsers = sSelectedUser.c_str();
	} else {
		while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
			if ( sUsers.GetLength() > 0 )
  				sUsers += ", ";
			sUsers += ((UserInfo*)ctrlUsers.getItemData(i))->user->getNick().c_str();
		}
	}

	int iSelBegin, iSelEnd;
	ctrlMessage.GetSel( iSelBegin, iSelEnd );
	ctrlMessage.GetWindowText( sText );

	if ( ( iSelBegin == 0 ) && ( iSelEnd == 0 ) ) {
		if ( sText.GetLength() == 0 ) {
			sUsers += ": ";
			ctrlMessage.SetWindowText( sUsers );
			ctrlMessage.SetFocus();
			ctrlMessage.SetSel( ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength() );
		} else {
			sUsers += ": ";
			ctrlMessage.ReplaceSel( sUsers );
			ctrlMessage.SetFocus();
		}
	} else {
		sUsers += " ";
		ctrlMessage.ReplaceSel( sUsers );
		ctrlMessage.SetFocus();
	}
	return 0;
}

LRESULT HubFrame::onAutoScrollChat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlClient.SetAutoScroll( !ctrlClient.GetAutoScroll() );
	return 0;
}

LRESULT HubFrame::onBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if ( sSelectedIP != "" ) {
		string s = "!ban " + sSelectedIP;
		client->hubMessage(s);
	}
	return 0;
}

LRESULT HubFrame::onUnBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if ( sSelectedIP != "" ) {
		string s = "!unban " + sSelectedIP;
		client->hubMessage(s);
	}
	return 0;
}

LRESULT HubFrame::onCopyURL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if ( sSelectedURL != "" ) {
		WinUtil::setClipboard( sSelectedURL );
	}
	return 0;
}

LRESULT HubFrame::onClientEnLink(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
	ENLINK* pEL = (ENLINK*)pnmh;

	if ( pEL->msg == WM_LBUTTONUP ) {
		long lBegin = pEL->chrg.cpMin, lEnd = pEL->chrg.cpMax;
		char sURLTemp[2 * MAX_PATH];
		int iRet = ctrlClient.GetTextRange( lBegin, lEnd, sURLTemp );
		UNREFERENCED_PARAMETER(iRet);
		string sURL = sURLTemp;
		WinUtil::openLink(sURL);
	} else if ( pEL->msg == WM_RBUTTONUP ) {
		sSelectedURL = "";
		long lBegin = pEL->chrg.cpMin, lEnd = pEL->chrg.cpMax;
		char sURLTemp[2 * MAX_PATH];
		int iRet = ctrlClient.GetTextRange( lBegin, lEnd, sURLTemp );
		UNREFERENCED_PARAMETER(iRet);
		sSelectedURL = sURLTemp;

		ctrlClient.SetSel( lBegin, lEnd );
		ctrlClient.InvalidateRect( NULL );
		return 0;
	}
	return 0;
}

LRESULT HubFrame::onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {	
	string file = Util::emptyString;
	string xNick;
	if (sSelectedUser != "") {
		xNick = sSelectedUser.c_str();
	} else {
		int i = -1;
		while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
			xNick = ((UserInfo*)ctrlUsers.getItemData(i))->user->getNick().c_str();
		}
	}
	if (xNick != "") {
		file = Util::validateFileName(SETTING(LOG_DIRECTORY) + xNick + ".log");
	}
	if(File::existsFile(file)) {
		ShellExecute(NULL, NULL, file.c_str(), NULL, NULL, SW_SHOWNORMAL);
	} else {
		MessageBox(CSTRING(NO_LOG_FOR_USER),CSTRING(NO_LOG_FOR_USER), MB_OK );	  
	}	

	return 0;
}

LRESULT HubFrame::onOpenHubLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	string filename  = Util::validateFileName(SETTING(LOG_DIRECTORY) + client->getAddressPort() + ".log");
	if(File::existsFile(filename)){
		ShellExecute(NULL, NULL, filename.c_str(), NULL, NULL, SW_SHOWNORMAL);

	} else {
		MessageBox(CSTRING(NO_LOG_FOR_HUB),CSTRING(NO_LOG_FOR_HUB), MB_OK );	  
	}
	return 0;
}

LRESULT HubFrame::onWhoisIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if ( sSelectedIP != "" ) {
 		WinUtil::openLink("http://www.ripe.net/perl/whois?form_type=simple&full_query_string=&searchtext="+sSelectedIP);
 	}
	return 0;
}
// CDM EXTENSION BEGINS
void HubFrame::getUserResponses() {
	QueueManager* queueManager = QueueManager::getInstance();
	if(client->isConnected())
	{
		int i = -1;
		User::Ptr user = NULL;
		while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
			user = ((UserInfo*)ctrlUsers.getItemData(i))->user;
			if( (user->getNick() != client->getNick()) )
			{
				try {
					queueManager->addTestSUR(user);
				} catch(const Exception&) {
					//continue;
				}
			}
		}
	}
}
// CDM EXTENSION ENDS
LRESULT HubFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {

	CRect rc;
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		{
			//UserInfo* ii = (UserInfo*)cd->nmcd.lItemlParam;
			UserInfo* ii = ctrlUsers.getStoredItemAt(cd->nmcd.lItemlParam); 
			if(ii->user->getFakeSharing()) {
				cd->clrText = RGB(204,0,0);
				return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
			} else if(/*(BOOLSETTING(SHOW_SHARE_CHECKED_USERS)) &&*/ (ii->user->getChecked())) {
				cd->clrText = RGB(0, 160, 0);
				return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
			}
		}
		return CDRF_NOTIFYSUBITEMDRAW;

	default:
		return CDRF_DODEFAULT;
	}
	
}

/**
 * @file
 * $Id$
 */