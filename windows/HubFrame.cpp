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
#include "../client/AdcCommand.h"
#include "../client/SettingsManager.h"
#include "../client/ConnectionManager.h" 

#include "../pme-1.0.4/pme.h"

HubFrame::FrameMap HubFrame::frames;


tstring sSelectedLine = _T("");
tstring sSelectedIP = _T("");
tstring sSelectedURL = _T("");
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
	
	ctrlClient.SetAutoURLDetect(false);
	ctrlClient.SetEventMask( ctrlClient.GetEventMask() | ENM_LINK );
	ctrlClient.SetUsers( &ctrlUsers );
	
	ctrlMessage.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | ES_NOHIDESEL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE, WS_EX_CLIENTEDGE);
	
	ctrlMessageContainer.SubclassWindow(ctrlMessage.m_hWnd);
	ctrlMessage.SetFont(WinUtil::font);
	ctrlMessage.SetLimitText(9999);

	ctrlFilter.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);

	ctrlFilterContainer.SubclassWindow(ctrlFilter.m_hWnd);
	ctrlFilter.SetFont(WinUtil::font);

	ctrlFilterSel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL |
		WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);

	ctrlFilterSelContainer.SubclassWindow(ctrlFilterSel.m_hWnd);
	ctrlFilterSel.SetFont(WinUtil::font);

	ctrlUsers.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_USERS);
	
	DWORD styles = LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | 0x00010000;;
	if (BOOLSETTING(SHOW_INFOTIPS))
		styles |= LVS_EX_INFOTIP;

	ctrlUsers.SetExtendedListViewStyle(styles);

	splitChat.Create( m_hWnd );
	splitChat.SetSplitterPanes(ctrlClient.m_hWnd, ctrlMessage.m_hWnd, true);
	splitChat.SetSplitterExtendedStyle(0);
	splitChat.SetSplitterPos( 40 );

	SetSplitterPanes(ctrlClient.m_hWnd, ctrlUsers.m_hWnd, false);
	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
	if(hubchatusersplit){ m_nProportionalPos = hubchatusersplit;
	} else { m_nProportionalPos = 7500; }
	ctrlShowUsers.Create(ctrlStatus.m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlShowUsers.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowUsers.SetFont(WinUtil::systemFont);
	ctrlShowUsers.SetCheck(ShowUserList ? BST_CHECKED : BST_UNCHECKED);
	showUsersContainer.SubclassWindow(ctrlShowUsers.m_hWnd);

	m_UserListColumns.ReadFromSetup();
	m_UserListColumns.SetToList(ctrlUsers);
	
	ctrlUsers.SetBkColor(WinUtil::bgColor);
	ctrlUsers.SetTextBkColor(WinUtil::bgColor);
	ctrlUsers.SetTextColor(WinUtil::textColor);
	ctrlUsers.setFlickerFree(WinUtil::bgBrush);
	ctrlClient.SetBackgroundColor(WinUtil::bgColor); 
	
	ctrlUsers.setSortColumn(UserInfo::COLUMN_NICK);
				
	ctrlUsers.SetImageList(WinUtil::userImages, LVSIL_SMALL);

	CToolInfo ti(TTF_SUBCLASS, ctrlStatus.m_hWnd);
	
	ctrlLastLines.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON, WS_EX_TOPMOST);
	ctrlLastLines.AddTool(&ti);

	copyHubMenu.CreatePopupMenu();
	copyHubMenu.AppendMenu(MF_STRING, IDC_COPY_HUBNAME, CTSTRING(HUB_NAME));
	copyHubMenu.AppendMenu(MF_STRING, IDC_COPY_HUBADDRESS, CTSTRING(HUB_ADDRESS));

	tabMenu.CreatePopupMenu();
	if(BOOLSETTING(LOG_MAIN_CHAT)) {
		tabMenu.AppendMenu(MF_STRING, IDC_OPEN_HUB_LOG, CTSTRING(OPEN_HUB_LOG));
		tabMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	}
	tabMenu.AppendMenu(MF_STRING, IDC_ADD_AS_FAVORITE, CTSTRING(ADD_TO_FAVORITES));
	tabMenu.AppendMenu(MF_STRING, ID_FILE_RECONNECT, CTSTRING(MENU_RECONNECT));
	tabMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)copyHubMenu, CTSTRING(COPY));
	tabMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);	
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));

	if (CZDCLib::isXp()) {
		pmicon.hIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE( IDR_TRAY_PM_XP ), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	} else {
		pmicon.hIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE( IDR_TRAY_PM ), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	}

	showJoins = BOOLSETTING(SHOW_JOINS);
	favShowJoins = BOOLSETTING(FAV_SHOW_JOINS);

	TCHAR Buffer[256];
	LV_COLUMN lvCol;
	int indexes[32];
	ctrlUsers.GetColumnOrderArray(ctrlUsers.GetHeader().GetItemCount(), indexes);
	for (int i = 0; i < ctrlUsers.GetHeader().GetItemCount(); ++i) {
		lvCol.mask = LVCF_TEXT;
		lvCol.pszText = Buffer;
		lvCol.cchTextMax = 255;
		ctrlUsers.GetColumn(indexes[i], &lvCol);
		ctrlFilterSel.AddString(lvCol.pszText);
	}
	ctrlFilterSel.SetCurSel(0);

	bHandled = FALSE;
	client->connect();

    TimerManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	return 1;
}

void HubFrame::openWindow(const tstring& aServer
							, const tstring& rawOne /*= Util::emptyString*/
							, const tstring& rawTwo /*= Util::emptyString*/
							, const tstring& rawThree /*= Util::emptyString*/
							, const tstring& rawFour /*= Util::emptyString*/
							, const tstring& rawFive /*= Util::emptyString*/
		, int windowposx, int windowposy, int windowsizex, int windowsizey, int windowtype, int chatusersplit, bool stealth, bool userliststate) {
	FrameIter i = frames.find(aServer);
	if(i == frames.end()) {
		HubFrame* frm = new HubFrame(aServer
			, rawOne
			, rawTwo 
			, rawThree 
			, rawFour 
			, rawFive 
			, windowposx, windowposy, windowsizex, windowsizey, windowtype, chatusersplit, stealth, userliststate);
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
		if(::IsIconic(i->second->m_hWnd))
			::ShowWindow(i->second->m_hWnd, SW_RESTORE);
		i->second->MDIActivate(i->second->m_hWnd);
	}
}

void HubFrame::onEnter() {	
	if(ctrlMessage.GetWindowTextLength() > 0) {
		AutoArray<TCHAR> msg(ctrlMessage.GetWindowTextLength()+1);
		ctrlMessage.GetWindowText(msg, ctrlMessage.GetWindowTextLength()+1);
		tstring s(msg, ctrlMessage.GetWindowTextLength());

		// save command in history, reset current buffer pointer to the newest command
		curCommandPosition = prevCommands.size();		//this places it one position beyond a legal subscript
		if (!curCommandPosition || curCommandPosition > 0 && prevCommands[curCommandPosition - 1] != s) {
			++curCommandPosition;
			prevCommands.push_back(s);
		}
		currentCommand = _T("");

		// Special command
		if(s[0] == _T('/')) {
			tstring m = s;
			tstring param;
			tstring message;
			tstring status;

			if(WinUtil::checkCommand(s, param, message, status)) {
				if(!message.empty()) {
					client->hubMessage(Text::fromT(message));
				}
				if(!status.empty()) {
					addClientLine(status, WinUtil::m_ChatTextSystem);
				}
			} else if(Util::stricmp(s.c_str(), _T("join"))==0) {
				if(!param.empty()) {
					redirect = param;
					BOOL whatever = FALSE;
					onFollow(0, 0, 0, whatever);
				} else {
					addClientLine(TSTRING(SPECIFY_SERVER), WinUtil::m_ChatTextSystem);
				}
			} else if((Util::stricmp(s.c_str(), _T("clear")) == 0) || (Util::stricmp(s.c_str(), _T("cls")) == 0)) {
				ctrlClient.SetWindowText(_T(""));
			} else if(Util::stricmp(s.c_str(), _T("ts")) == 0) {
				timeStamps = !timeStamps;
				if(timeStamps) {
					addClientLine(TSTRING(TIMESTAMPS_ENABLED), WinUtil::m_ChatTextSystem);
				} else {
					addClientLine(TSTRING(TIMESTAMPS_DISABLED), WinUtil::m_ChatTextSystem);
				}
			} else if( (Util::stricmp(s.c_str(), _T("password")) == 0) && waitingForPW ) {
				client->setPassword(Text::fromT(param));
				client->password(Text::fromT(param));
				waitingForPW = false;
			} else if( Util::stricmp(s.c_str(), _T("showjoins")) == 0 ) {
				showJoins = !showJoins;
				if(showJoins) {
					addClientLine(TSTRING(JOIN_SHOWING_ON), WinUtil::m_ChatTextSystem);
				} else {
					addClientLine(TSTRING(JOIN_SHOWING_OFF), WinUtil::m_ChatTextSystem);
				}
			} else if( Util::stricmp(s.c_str(), _T("favshowjoins")) == 0 ) {
				favShowJoins = !favShowJoins;
				if(favShowJoins) {
					addClientLine(TSTRING(FAV_JOIN_SHOWING_ON), WinUtil::m_ChatTextSystem);
				} else {
					addClientLine(TSTRING(FAV_JOIN_SHOWING_OFF), WinUtil::m_ChatTextSystem);
				}
			} else if(Util::stricmp(s.c_str(), _T("close")) == 0) {
				PostMessage(WM_CLOSE);
			} else if(Util::stricmp(s.c_str(), _T("userlist")) == 0) {
				ctrlShowUsers.SetCheck(ShowUserList ? BST_UNCHECKED : BST_CHECKED);
			} else if(Util::stricmp(s.c_str(), _T("connection")) == 0) {
				addClientLine(Text::toT((STRING(IP) + client->getLocalIp() + ", " + STRING(PORT) + Util::toString(SETTING(IN_PORT)))), WinUtil::m_ChatTextSystem);
			} else if((Util::stricmp(s.c_str(), _T("favorite")) == 0) || (Util::stricmp(s.c_str(), _T("fav")) == 0)) {
				addAsFavorite();
			} else if(Util::stricmp(s.c_str(), _T("getlist")) == 0){
				if( !param.empty() ){
					int k = ctrlUsers.findItem(param);
					if(k != -1) {
						ctrlUsers.getItemData(k)->getList();
					}
				}
			} else if(Util::stricmp(s.c_str(), _T("f")) == 0) {
				if(param.empty())
					param = findTextPopup();
				findText(param);
			} else if(Util::stricmp(s.c_str(), _T("extraslots"))==0) {
				int j = Util::toInt(Text::fromT(param));
				if(j > 0) {
					SettingsManager::getInstance()->set(SettingsManager::EXTRA_SLOTS, j);
					addClientLine(TSTRING(EXTRA_SLOTS_SET), WinUtil::m_ChatTextSystem );
				} else {
					addClientLine(TSTRING(INVALID_NUMBER_OF_SLOTS), WinUtil::m_ChatTextSystem );
				}
			} else if(Util::stricmp(s.c_str(), _T("smallfilesize"))==0) {
				int j = Util::toInt(Text::fromT(param));
				if(j >= 64) {
					SettingsManager::getInstance()->set(SettingsManager::SMALL_FILE_SIZE, j);
					addClientLine(TSTRING(SMALL_FILE_SIZE_SET), WinUtil::m_ChatTextSystem );
				} else {
					addClientLine(TSTRING(INVALID_SIZE), WinUtil::m_ChatTextSystem );
				}
			} else if(Util::stricmp(s.c_str(), _T("savequeue")) == 0) {
				QueueManager::getInstance()->saveQueue();
				addClientLine(_T("Queue saved."), WinUtil::m_ChatTextSystem );
			} else if(Util::stricmp(s.c_str(), _T("whois")) == 0) {
				WinUtil::openLink(_T("http://www.ripe.net/perl/whois?form_type=simple&full_query_string=&searchtext=") + Text::toT(Util::encodeURI(Text::fromT(param))));
			} else if(Util::stricmp(s.c_str(), _T("showblockedipports")) == 0) {
				StringList sl = ConnectionManager::getInstance()->getBlockedIpPorts();
				string ips = "***";
				for(StringIter i = sl.begin(); i != sl.end(); ++i)
					ips += ' ' + *i;
				addLine(Text::toT(ips));
			} else if(Util::stricmp(s.c_str(), _T("showshared")) == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_SHARED, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), _T("showexactshared")) == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_EXACT_SHARED, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), _T("showdescription")) == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_DESCRIPTION, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), _T("showtag")) == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_TAG, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), _T("showconnection")) == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_CONNECTION, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), _T("showemail")) == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_EMAIL, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), _T("showclient")) == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_CLIENTID, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), _T("showversion")) == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_VERSION, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), _T("showmode")) == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_MODE, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), _T("showhubs")) == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_HUBS, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), _T("showslots")) == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_SLOTS, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), _T("showupload")) == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_UPLOAD_SPEED, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), _T("showip")) == 0) {
				m_UserListColumns.SwitchColumnVisibility(UserInfo::COLUMN_IP, ctrlUsers);
			} else if(Util::stricmp(s.c_str(), _T("ignorelist"))==0) {
				tstring ignorelist = _T("Ignored users:");
				for(TStringHash::iterator i = ignoreList.begin(); i != ignoreList.end(); ++i)
					ignorelist += _T(" ") + *i;
				addLine(ignorelist, WinUtil::m_ChatTextSystem);
			} else if(Util::stricmp(s.c_str(), _T("help")) == 0) {
				addLine(_T("*** ") + Text::toT(WinUtil::commands) + _T(", /smallfilesize #, /extraslots #, /savequeue, /join <hub-ip>, /clear, /ts, /showjoins, /favshowjoins, /close, /userlist, /connection, /favorite, /pm <user> [message], /getlist <user>, /winamp, /showblockedipports, /whois [IP], /showshared, /showexactshared, /showdescription, /showtag, /showconnection, /showemail, /showclient, /showversion, /showmode, /showhubs, /showslots, /showupload, /ignorelist"), WinUtil::m_ChatTextSystem);
			} else if(Util::stricmp(s.c_str(), _T("pm")) == 0) {
				string::size_type j = param.find(_T(' '));
				if(j != string::npos) {
					tstring nick = param.substr(0, j);
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
			} else if(Util::stricmp(s.c_str(), _T("me")) == 0) {
				client->hubMessage(Text::fromT(m));
			} else {
				if (BOOLSETTING(SEND_UNKNOWN_COMMANDS)) {
					client->hubMessage(Text::fromT(m));
				} else {
					addClientLine(TSTRING(UNKNOWN_COMMAND) + s);
				}
			}
		} else {
			client->hubMessage(Text::fromT(s));
		}
		ctrlMessage.SetWindowText(_T(""));
	} else {
		MessageBeep(MB_ICONEXCLAMATION);
	}
}

struct CompareItems {
	CompareItems(int aCol) : col(aCol) { }
	bool operator()(const UserInfo& a, const UserInfo& b) const {
		return UserInfo::compareItems(&a, &b, col) < 0;
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

		int a = ctrlUsers.getSortPos(ui);
		if(ctrlUsers.getItemData(a) != ui) {
			return ctrlUsers.findItem(Text::toT(aUser->getNick()));
		}

		return a;
	}
	return ctrlUsers.findItem(Text::toT(aUser->getNick()));
}

void HubFrame::addAsFavorite() {
	FavoriteHubEntry aEntry;
	TCHAR buf[256];
	this->GetWindowText(buf, 255);
	aEntry.setServer(Text::fromT(server));
	aEntry.setName(Text::fromT(buf));
	aEntry.setDescription(Text::fromT(buf));
	aEntry.setConnect(TRUE);
	aEntry.setNick(client->getNick());
	aEntry.setPassword(client->getPassword());
	aEntry.setConnect(false);
	HubManager::getInstance()->addFavorite(aEntry);
	addClientLine(TSTRING(FAVORITE_HUB_ADDED), WinUtil::m_ChatTextSystem );
}

LRESULT HubFrame::onCopyHubInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if(client->isConnected()) {
        string sCopy;

		switch (wID) {
			case IDC_COPY_HUBNAME:
				sCopy += client->getName();
				break;
			case IDC_COPY_HUBADDRESS:
				sCopy += client->getAddressPort();
				break;
		}

		if (!sCopy.empty())
			WinUtil::setClipboard(Text::toT(sCopy));
    }
	return 0;
}

LRESULT HubFrame::onCopyUserInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if(client->isConnected()) {
        string sCopy;
		UserInfo* ui;

		if(sSelectedUser != _T("")) {
			int i = ctrlUsers.findItem(sSelectedUser);
			if ( i >= 0 ) {
				ui = (UserInfo*)ctrlUsers.getItemData(i);

				switch (wID) {
					case IDC_COPY_NICK:
						sCopy += ui->user->getNick();
						break;
					case IDC_COPY_EXACT_SHARE:
						sCopy += Util::formatExactSize(ui->user->getBytesShared());
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
							"\tUpLimit: " + ui->user->getUpload() + "\r\n";
						if(ui->user->isClientOp()) {
							sCopy += "\tIP: " + ui->user->getIp() + "\r\n"+
								"\tISP: " + ui->user->getHost() + "\r\n";
						}
						sCopy += "\tPk String: " + ui->user->getPk() + "\r\n"+
							"\tLock: " + ui->user->getLock() + "\r\n"+
							"\tSupports: " + ui->user->getSupports();
						break;		
					default:
						dcdebug("HUBFRAME DON'T GO HERE\n");
						return 0;
				}
			}
		}
		if (!sCopy.empty())
			WinUtil::setClipboard(Text::toT(sCopy));
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
		ctrlClient.SetWindowText(_T(""));
	}
	return 0;
}

bool HubFrame::updateUser(const User::Ptr& u, bool searchinlist /* = true */) {
	int i = -1;
	if(searchinlist)
		i = findUser(u);
	if(i == -1) {
		UserInfo* ui = new UserInfo(u, &m_UserListColumns);
		userMap.insert(make_pair(u, ui));

		bool add = false;
		if(filter.empty()){
			add = true;
		} else {
			PME reg(Text::fromT(filter),"i");
			if(!reg.IsValid()) { 
				add = true;
			} else if(reg.match(Text::fromT(ui->getText(ctrlFilterSel.GetCurSel())))) {
				add = true;
			}
		}
	
		if( add ){
			ctrlUsers.insertItem(ui, WinUtil::getImage(u));
		}		
		return true;
	} else {
		UserInfo* ui = ctrlUsers.getItemData(i);
		bool resort = (ui->getOp() != u->isSet(User::OP));
		ctrlUsers.getItemData(i)->update();
		ctrlUsers.updateItem(i);
		ctrlUsers.SetItem(i, 0, LVIF_IMAGE, NULL, WinUtil::getImage(u), 0, 0, NULL);
		if(resort)
			ctrlUsers.resort();
		return false;
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
						if (u->isFavoriteUser() && (!SETTING(SOUND_FAVUSER).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
							PlaySound(Text::toT(SETTING(SOUND_FAVUSER)).c_str(), NULL, SND_FILENAME | SND_ASYNC);

						if(u->isFavoriteUser() && BOOLSETTING(POPUP_FAVORITE_CONNECTED)) {
							MainFrame::getMainFrame()->ShowBalloonTip(Text::toT(u->getFullNick()).c_str(), CTSTRING(FAVUSER_ONLINE));
						}

						if (showJoins || (favShowJoins && u->isFavoriteUser())) {
						 	addLine(_T("*** ") + TSTRING(JOINS) + Text::toT(u->getNick()), WinUtil::m_ChatTextSystem);
						}	

						if(client->getOp() && !u->isSet(User::OP)) {
							if(Util::toString(u->getBytesShared()).find("000000") != -1) {
								string detectString = Util::formatExactSize(u->getBytesShared())+" - the share size had too many zeroes in it";
								u->setBadFilelist(true);
								u->setCheat(Util::validateMessage(detectString, false), false);
								u->sendRawCommand(SETTING(FAKESHARE_RAW));
								this->updateUser(u);
							}
						
							if(BOOLSETTING(CHECK_NEW_USERS)) {
								if(u->getMode() == "A" || !u->isSet(User::PASSIVE) || client->getMode() == SettingsManager::CONNECTION_ACTIVE) {
									try {
										QueueManager::getInstance()->addTestSUR(u, true);
									} catch(const Exception&) {
									}
								}
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
					int j = ctrlUsers.findItem(Text::toT(u->getNick()));
					if( j != -1 ) {
						UserInfo* ui = ctrlUsers.getItemData(j);
						ctrlUsers.SetItemState(j, 0, LVIS_SELECTED);
						ctrlUsers.DeleteItem(j);
						delete ui;
					} else {
						UserMapIter i = userMap.find(u);
						if(i != userMap.end())
							delete i->second;
					}
					if (showJoins || (favShowJoins && u->isFavoriteUser())) {
						addLine(Text::toT("*** " + STRING(PARTS) + u->getNick()), WinUtil::m_ChatTextSystem);
					}
					userMap.erase(u);
					break;
				}
			}
			updateList.clear();
		}
		if(ctrlUsers.getSortColumn() != UserInfo::COLUMN_NICK)
			ctrlUsers.resort();
		ctrlUsers.SetRedraw(TRUE);
	} else if(wParam == CHEATING_USER) {
		tstring* x = (tstring*)lParam;

		CHARFORMAT2 cf;
		memset(&cf, 0, sizeof(CHARFORMAT2));
		cf.cbSize = sizeof(cf);
		cf.dwReserved = 0;
		cf.dwMask = CFM_BACKCOLOR | CFM_COLOR | CFM_BOLD;
		cf.dwEffects = 0;
		cf.crBackColor = SETTING(BACKGROUND_COLOR);
		cf.crTextColor = SETTING(ERROR_COLOR);

		if(BOOLSETTING(POPUP_CHEATING_USER) && (*x).length() < 256) {
			MainFrame::getMainFrame()->ShowBalloonTip((*x).c_str(), CTSTRING(CHEATING_USER));
		}

		addLine(_T("*** ") + *x,cf);
		delete x;
	} else if(wParam == DISCONNECTED) {
		clearUserList();
		setTabColor(RGB(255, 0, 0));
		setIconState();
		if ((!SETTING(SOUND_HUBDISCON).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
			PlaySound(Text::toT(SETTING(SOUND_HUBDISCON)).c_str(), NULL, SND_FILENAME | SND_ASYNC);

		if(BOOLSETTING(POPUP_HUB_DISCONNECTED)) {
			MainFrame::getMainFrame()->ShowBalloonTip(Text::toT(client->getAddress()).c_str(), CTSTRING(DISCONNECTED));
		}

	} else if(wParam == CONNECTED) {
		addClientLine(TSTRING(CONNECTED), WinUtil::m_ChatTextServer);
		setTabColor(RGB(0, 255, 0));
		unsetIconState();

		if(BOOLSETTING(POPUP_HUB_CONNECTED)) {
			MainFrame::getMainFrame()->ShowBalloonTip(Text::toT(client->getAddress()).c_str(), CTSTRING(CONNECTED));
		}

		if ((!SETTING(SOUND_HUBCON).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
			PlaySound(Text::toT(SETTING(SOUND_HUBCON)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
	} else if(wParam == ADD_CHAT_LINE) {
		tstring* x = (tstring*)lParam;
		int nickPos = x->find('<') + 1;
		tstring nick = x->substr(nickPos, x->find('>') - nickPos);
		int i = -1;
		if(nick != _T(""))
			i = ctrlUsers.findItem(nick);
		if ( i >= 0 ) {
			UserInfo* ui = (UserInfo*)ctrlUsers.getItemData(i);
			if (!ignoreList.count(nick) || (ui->user->isSet(User::OP) && !client->getOp()))
				addLine(*x, WinUtil::m_ChatTextGeneral);
		} else {
			if (!ignoreList.count(nick))
				addLine(*x, WinUtil::m_ChatTextGeneral);
		}
		delete x;
	} else if(wParam == ADD_STATUS_LINE) {
		tstring* x = (tstring*)lParam;
		addClientLine(*x, WinUtil::m_ChatTextServer );
		delete x;
	} else if(wParam == ADD_SILENT_STATUS_LINE) {
		tstring* x = (tstring*)lParam;
		addClientLine(*x, false);
		delete x;
	} else if(wParam == SET_WINDOW_TITLE) {
		tstring* x = (tstring*)lParam;
		SetWindowText(x->c_str());
		delete x;
	} else if(wParam == STATS) {
		ctrlStatus.SetText(1, Text::toT(Util::toString(client->getUserCount()) + " " + STRING(HUB_USERS)).c_str());
		ctrlStatus.SetText(2, Text::toT(Util::formatBytes(client->getAvailable())).c_str());
		if(client->getUserCount() > 0)
			ctrlStatus.SetText(3, Text::toT((Util::formatBytes(client->getAvailable() / client->getUserCount()) + "/" + STRING(USER))).c_str());
		else
			ctrlStatus.SetText(3, _T(""));
	} else if(wParam == GET_PASSWORD) {
		if(client->getPassword().size() > 0) {
			client->password(client->getPassword());
			addClientLine(TSTRING(STORED_PASSWORD_SENT), WinUtil::m_ChatTextSystem);
		} else {
			ctrlMessage.SetWindowText(_T("/password "));
			ctrlMessage.SetFocus();
			ctrlMessage.SetSel(10, 10);
			waitingForPW = true;
		}
	} else if(wParam == PRIVATE_MESSAGE) {
		PMInfo* i = (PMInfo*)lParam;
		if(!ignoreList.count(Text::toT(i->user->getNick())) || (i->user->isSet(User::OP) && !client->getOp())) {
			if(i->user->isOnline()) {
			if(BOOLSETTING(POPUP_PMS) || PrivateFrame::isOpen(i->user)) {
					PrivateFrame::gotMessage(i->user, i->msg);
				} else {
					addLine(TSTRING(PRIVATE_MESSAGE_FROM) + Text::toT(i->user->getNick()) + _T(": ") + i->msg, WinUtil::m_ChatTextPrivate);
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
					addClientLine(TSTRING(IGNORED_MESSAGE) + i->msg, WinUtil::m_ChatTextPrivate, false);
				} else if(BOOLSETTING(POPUP_OFFLINE)) {
					PrivateFrame::gotMessage(i->user, i->msg);
				} else {
					addLine(TSTRING(PRIVATE_MESSAGE_FROM) + Text::toT(i->user->getNick()) + _T(": ") + i->msg, WinUtil::m_ChatTextPrivate);
				}
			}
		}
		delete i;
	} else if(wParam == KICK_MSG) {
		tstring* x = (tstring*)lParam;
		addLine(*x, WinUtil::m_ChatTextServer);
		delete x;
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
		int w[5];
		ctrlStatus.GetClientRect(sr);

		int tmp = (sr.Width()) > 332 ? 232 : ((sr.Width() > 132) ? sr.Width()-100 : 32);
		
		w[0] = sr.right - tmp - 50;
		w[1] = w[0] + (tmp-50)/2;
		w[2] = w[0] + (tmp-80);
		w[3] = w[2] + 96;
		w[4] = w[3] + 16;
		
		ctrlStatus.SetParts(5, w);

		ctrlLastLines.SetMaxTipWidth(w[0]);
		ctrlLastLines.SetWindowPos(HWND_TOPMOST, sr.left, sr.top, sr.Width(), sr.Height(), SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

		// Strange, can't get the correct width of the last field...
		ctrlStatus.GetRect(3, sr);
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

	rc = rect;
	rc.bottom -= 2;
	rc.top = rc.bottom - h - 5;
	rc.left +=2;
	rc.right -= ShowUserList ? 202 : 2;
	ctrlMessage.MoveWindow(rc);

	if(ShowUserList){
		rc.left = rc.right + 4;
		rc.right = rc.left + 116;
		ctrlFilter.MoveWindow(rc);

		rc.left = rc.right + 4;
		rc.right = rc.left + 76;
		rc.top = rc.top + 0;
		rc.bottom = rc.bottom + 120;
		ctrlFilterSel.MoveWindow(rc);
	}
	ctrlClient.GoToEnd();
}

LRESULT HubFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		RecentHubEntry* r = HubManager::getInstance()->getRecentHubEntry(Text::fromT(server));
		if(r) {
			TCHAR buf[256];
			this->GetWindowText(buf, 255);
			r->setName(Text::fromT(buf));
			r->setUsers(Util::toString(client->getUserCount()));
			r->setShared(Util::toString(client->getAvailable()));
			HubManager::getInstance()->updateRecent(r);
		}
		TimerManager::getInstance()->removeListener(this);
			SettingsManager::getInstance()->removeListener(this);
		client->removeListener(this);
		client->disconnect();
	
		closed = true;
		clearUserList();
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		SettingsManager::getInstance()->set(SettingsManager::GET_USER_INFO, ShowUserList);
		HubManager::getInstance()->removeUserCommand(Text::fromT(server));

		int i = 0;
		int j = ctrlUsers.GetItemCount();
		while(i < j) {
			delete ctrlUsers.getItemData(i);
			i++;
		}
	
		m_UserListColumns.WriteToSetup(ctrlUsers);

		FavoriteHubEntry* hub = HubManager::getInstance()->getFavoriteHubEntry(Text::fromT(server));
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
		bHandled = FALSE;
	return 0;
	}
}

void HubFrame::findText(tstring const& needle) throw() {
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
		addClientLine(CTSTRING(STRING_NOT_FOUND) + needle);
		currentNeedle = Util::emptyStringT;
	}
}

tstring HubFrame::findTextPopup() {
	LineDlg *finddlg;
	tstring param = Util::emptyStringT;
		finddlg = new LineDlg;
		finddlg->title = CTSTRING(SEARCH);
		finddlg->description = CTSTRING(SPECIFY_SEARCH_STRING);
		if(finddlg->DoModal() == IDOK) {
		param = finddlg->line;
	}
	delete[] finddlg;
	return param;
}

LRESULT HubFrame::onLButton(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HWND focus = GetFocus();
	bHandled = false;
	if(focus == ctrlClient.m_hWnd) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		tstring x;

		int i = ctrlClient.CharFromPos(pt);
		int line = ctrlClient.LineFromChar(i);
		int c = LOWORD(i) - ctrlClient.LineIndex(line);
		int len = ctrlClient.LineLength(i) + 1;
		if(len < 3) {
			return 0;
		}

		TCHAR* buf = new TCHAR[len];
		ctrlClient.GetLine(line, buf, len);
		x = tstring(buf, len-1);
		delete buf;

		string::size_type start = x.find_last_of(_T(" <\t\r\n"), c);
		if(start == string::npos)
			start = 0;
		else
			start++;
					

		string::size_type end = x.find_first_of(_T(" >\t"), start+1);

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
					addClientLine(Text::toT(e.getError()), WinUtil::m_ChatTextSystem);
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
	return 0;
}

void HubFrame::addLine(const tstring& aLine) {
	addLine(aLine, WinUtil::m_ChatTextGeneral );
}

void HubFrame::addLine(const tstring& aLine, CHARFORMAT2& cf) {
	ctrlClient.AdjustTextSize();

	tstring sTmp = aLine;
	tstring sAuthor = _T("");
	if (aLine.find(_T("<")) == 0) {
		string::size_type i = aLine.find(_T(">"));
		if (i != string::npos) {
       		sAuthor = aLine.substr(1, i-1);
			if (_tcsncmp(_T(" /me "), aLine.substr(i+1, 5).c_str(), 5) == 0 ) {
				sTmp = _T("* ") + sAuthor + aLine.substr(i+5);
			}
		}
	}
	sMyNick = client->getNick().c_str();
	if(BOOLSETTING(LOG_MAIN_CHAT)) {
		StringMap params;
		params["message"] = Text::fromT(sTmp);
		LOG("MainChat\\" + client->getAddressPort(), Util::formatParams(SETTING(LOG_FORMAT_MAIN_CHAT), params));
	}
	if(timeStamps) {
		ctrlClient.AppendText(Text::toT(sMyNick).c_str(), Text::toT("[" + Util::getShortTimeString() + "] ").c_str(), sTmp.c_str(), cf, sAuthor.c_str() );
	} else {
		ctrlClient.AppendText(Text::toT(sMyNick).c_str(), _T(""), sTmp.c_str(), cf, sAuthor.c_str() );
	}
	if (BOOLSETTING(TAB_DIRTY)) {
		setDirty();
	}
}

LRESULT HubFrame::onTabContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 
	tabMenuShown = true;
	CMenu hSysMenu;
	tabMenu.InsertSeparatorFirst((client->getName() != "") ? (client->getName().size() > 50 ? client->getName().substr(0, 50) : client->getName()) : client->getAddressPort());	
	copyHubMenu.InsertSeparatorFirst(STRING(COPY));
	
	if(!client->isConnected())
		tabMenu.EnableMenuItem((UINT)(HMENU)copyHubMenu, MF_GRAYED);
	else
		tabMenu.EnableMenuItem((UINT)(HMENU)copyHubMenu, MF_ENABLED);

	prepareMenu(tabMenu, ::UserCommand::CONTEXT_HUB, Text::toT(client->getAddressPort()), client->getOp());
	hSysMenu.Attach((wParam == NULL) ? (HMENU)tabMenu : (HMENU)wParam);
	if (wParam != NULL) {
		hSysMenu.InsertMenu(hSysMenu.GetMenuItemCount() - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)(HMENU)tabMenu, /*CTSTRING(USER_COMMANDS)*/ _T("User Commands"));
		hSysMenu.InsertMenu(hSysMenu.GetMenuItemCount() - 1, MF_BYPOSITION | MF_SEPARATOR, 0, (LPCTSTR)NULL);
	}
	hSysMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	if (wParam != NULL) {
		hSysMenu.RemoveMenu(hSysMenu.GetMenuItemCount() - 2, MF_BYPOSITION);
		hSysMenu.RemoveMenu(hSysMenu.GetMenuItemCount() - 2, MF_BYPOSITION);
	}
	cleanMenu(tabMenu);	
	tabMenu.RemoveFirstItem();
	copyHubMenu.RemoveFirstItem();
	hSysMenu.Detach();
	return TRUE;
}

LRESULT HubFrame::onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
		RECT rc;                    // client area of window 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click
		POINT ptCl;
		tabMenuShown = false;
	OMenu Mnu;
	tstring sU = _T("");
	
	sSelectedUser = _T("");
	sSelectedLine = _T("");
	sSelectedIP = _T("");

		ctrlUsers.GetClientRect(&rc);
		ptCl = pt;
		ctrlUsers.ScreenToClient(&ptCl);
	
		if (PtInRect(&rc, ptCl) && ShowUserList) { 
			if ( ctrlUsers.GetSelectedCount() == 1 ) {
				int i = -1;
				i = ctrlUsers.GetNextItem(i, LVNI_SELECTED);
				if ( i >= 0 ) {
				sSelectedUser = sU = Text::toT(((UserInfo*)ctrlUsers.getItemData(i))->user->getNick());
				}
			}

			if ( PreparePopupMenu( &ctrlUsers, false, sU, &Mnu ) ) {
				prepareMenu(Mnu, ::UserCommand::CONTEXT_CHAT, Text::toT(client->getAddressPort()), client->getOp());
				if(!(Mnu.GetMenuState(Mnu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {	
					Mnu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
				}
				Mnu.AppendMenu(MF_STRING, IDC_REFRESH, CTSTRING(REFRESH_USER_LIST));
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

		sSelectedLine = Text::toT(ctrlClient.LineFromPos(ptCl));

			// Klik na nick ? A existuje zjisteny nick ?

			bool boHitNick = ctrlClient.HitNick( ptCl, &sUser );
			if ( boHitNick ) 
			sSelectedUser = sUser;

			CAtlString sIP;
			bool boHitIP = ctrlClient.HitIP( ptCl, &sIP );
			if ( boHitIP )
				sSelectedIP = sIP;

			bool boHitURL = ctrlClient.HitURL(ptCl);
		if (!boHitURL)
			sSelectedURL = _T("");

		sU = boHitNick ? sSelectedUser : _T("");
		if(PreparePopupMenu( &ctrlClient, !boHitNick, sU, &Mnu )) {
			if(sU == _T("")) {
					Mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
					if(copyMenu != NULL) copyMenu.DestroyMenu();
					if(grantMenu != NULL) grantMenu.DestroyMenu();
					if(Mnu != NULL) Mnu.DestroyMenu();
					return TRUE;
				} else {
				prepareMenu(Mnu, ::UserCommand::CONTEXT_CHAT, Text::toT(client->getAddressPort()), client->getOp());
					if(!(Mnu.GetMenuState(Mnu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {	
						Mnu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
					}
					Mnu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR));
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
	ucParams["mycid"] = client->getMe()->getCID().toBase32();

	if(tabMenuShown) {
		client->escapeParams(ucParams);

		client->sendUserCmd(Util::formatParams(uc.getCommand(), ucParams));
	} else {
		int sel;
		UserInfo* u = NULL;
		if (sSelectedUser != _T("")) {
			sel = ctrlUsers.findItem(sSelectedUser);
			if ( sel >= 0 ) { 
				u = (UserInfo*)ctrlUsers.getItemData(sel);
				if(u->user->isOnline()) {
					StringMap tmp = ucParams;
					u->user->getParams(tmp);
					client->escapeParams(tmp); 

					client->sendUserCmd(Util::formatParams(uc.getCommand(), tmp));
				}
			}
		} else {
			sel = -1;
			while((sel = ctrlUsers.GetNextItem(sel, LVNI_SELECTED)) != -1) {
				u = (UserInfo*)ctrlUsers.getItemData(sel);
				if(u->user->isOnline()) {
					StringMap tmp = ucParams;
					u->user->getParams(tmp);
					client->escapeParams(tmp);

					client->sendUserCmd(Util::formatParams(uc.getCommand(), tmp));
				}
			}
		}
	}
	return;
};

void HubFrame::onTab() {
	if(BOOLSETTING(TAB_COMPLETION) && 
		(GetFocus() == ctrlMessage.m_hWnd) && 
		!(GetAsyncKeyState(VK_SHIFT) & 0x8000) ) 
	{
		int n = ctrlMessage.GetWindowTextLength();
		AutoArray<TCHAR> buf(n+1);
		ctrlMessage.GetWindowText(buf, n+1);
		tstring text(buf, n);
		string::size_type textStart = text.find_last_of(_T(" \n\t"));

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
			const tstring& nick = ui->columns[UserInfo::COLUMN_NICK];
			bool found = (Util::strnicmp(nick, complete, complete.length()) == 0);
			tstring::size_type x = 0;
			if(!found) {
				// Check if there's one or more [ISP] tags to ignore...
				tstring::size_type y = 0;
				while(nick[y] == _T('[')) {
					x = nick.find(_T(']'), y);
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
				ctrlMessage.ReplaceSel(nick.c_str());
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
			ctrlClient.SetFocus();
		} 
	}
}

LRESULT HubFrame::onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
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
	} else if (wParam == VK_ESCAPE) {
		// Clear find text and give the focus back to the message box
		ctrlMessage.SetFocus();
		ctrlClient.SetSel(-1, -1);
		ctrlClient.SendMessage(EM_SCROLL, SB_BOTTOM, 0);
		ctrlClient.InvalidateRect(NULL);
		currentNeedle = _T("");
	} else if((wParam == VK_F3 && GetKeyState(VK_SHIFT) & 0x8000) ||
		(wParam == 'F' && GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000)) {
		findText(findTextPopup());
		return 0;
	} else if(wParam == VK_F3) {
		findText(currentNeedle.empty() ? findTextPopup() : currentNeedle);
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
			if ( (GetKeyState(VK_MENU) & 0x8000) ||	( ((GetKeyState(VK_CONTROL) & 0x8000) == 0) ^ (BOOLSETTING( SETTINGS_USE_CTRL_FOR_LINE_HISTORY ) == true) ) ) {
				//scroll up in chat command history
				//currently beyond the last command?
				if (curCommandPosition > 0) {
					//check whether current command needs to be saved
					if (curCommandPosition == prevCommands.size()) {
						auto_ptr<TCHAR> messageContents(new TCHAR[ctrlMessage.GetWindowTextLength()+2]);
						ctrlMessage.GetWindowText(messageContents.get(), ctrlMessage.GetWindowTextLength()+1);
						currentCommand = tstring(messageContents.get());
					}

					//replace current chat buffer with current command
					ctrlMessage.SetWindowText(prevCommands[--curCommandPosition].c_str());
				}
			} else {
				bHandled = FALSE;
			}

			break;
		case VK_DOWN:
			if ( (GetKeyState(VK_MENU) & 0x8000) ||	( ((GetKeyState(VK_CONTROL) & 0x8000) == 0) ^ (BOOLSETTING( SETTINGS_USE_CTRL_FOR_LINE_HISTORY ) == true) ) ) {
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
				
				auto_ptr<TCHAR> messageContents(new TCHAR[ctrlMessage.GetWindowTextLength()+2]);
				ctrlMessage.GetWindowText(messageContents.get(), ctrlMessage.GetWindowTextLength()+1);
				currentCommand = tstring(messageContents.get());

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

LRESULT HubFrame::onShowUsers(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	bHandled = FALSE;
	if(wParam == BST_CHECKED) {
		ShowUserList = true;
		User::NickMap& lst = client->lockUserList();
		ctrlUsers.SetRedraw(FALSE);
		for(User::NickIter i = lst.begin(); i != lst.end(); ++i) {
			updateUser(i->second);
		}
		client->unlockUserList();
		ctrlUsers.SetRedraw(TRUE);
		ctrlUsers.resort();
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
		Util::decodeUrl(Text::fromT(redirect), s, p, f);
		if(ClientManager::getInstance()->isConnected(s, p)) {
			addClientLine(TSTRING(REDIRECT_ALREADY_CONNECTED), WinUtil::m_ChatTextServer);
			return 0;
		}
		
		dcassert(frames.find(server) != frames.end());
		dcassert(frames[server] == this);
		frames.erase(server);
		server = redirect;
		frames[server] = this;

		// the client is dead, long live the client!
		client->removeListener(this);
		ClientManager::getInstance()->putClient(client);
		clearUserList();
		client = ClientManager::getInstance()->getClient(Text::fromT(server));

		RecentHubEntry r;
		r.setName("*");
		r.setDescription("***");
		r.setUsers("*");
		r.setShared("*");
		r.setServer(Text::fromT(redirect));
		HubManager::getInstance()->addRecent(r);

		client->addListener(this);
		client->connect();
	}
	return 0;
}

LRESULT HubFrame::onEnterUsers(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
	int item = ctrlUsers.GetNextItem(-1, LVNI_FOCUSED);
	if(client->isConnected() && (item != -1)) {
		try {
			QueueManager::getInstance()->addList((ctrlUsers.getItemData(item))->user, QueueItem::FLAG_CLIENT_VIEW);
		} catch(const Exception& e) {
			addClientLine(Text::toT(e.getError()));
		}
	}
	return 0;
}

LRESULT HubFrame::onGetToolTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMTTDISPINFO* nm = (NMTTDISPINFO*)pnmh;
	lastLines.clear();
	for(TStringIter i = lastLinesList.begin(); i != lastLinesList.end(); ++i) {
		lastLines += *i;
		lastLines += _T("\r\n");
	}
	if(lastLines.size() > 2) {
		lastLines.erase(lastLines.size() - 2);
	}
	nm->lpszText = const_cast<TCHAR*>(lastLines.c_str());
	return 0;
}

void HubFrame::addClientLine(const tstring& aLine, bool inChat /* = true */) {
	tstring line = _T("[") + Text::toT(Util::getShortTimeString()) + _T("] ") + aLine;

	ctrlStatus.SetText(0, line.c_str());
	while(lastLinesList.size() + 1 > MAX_CLIENT_LINES)
		lastLinesList.erase(lastLinesList.begin());
	lastLinesList.push_back(line);

	if (BOOLSETTING(TAB_DIRTY)) {
		setDirty();
	}
	
	if(BOOLSETTING(STATUS_IN_CHAT) && inChat) {
		addLine(_T("*** ") + aLine, WinUtil::m_ChatTextSystem);
	}
	if(BOOLSETTING(LOG_STATUS_MESSAGES)) {
		LOGDT("MainChat\\" + client->getAddressPort() + "_Status", Text::fromT(aLine));
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
		addLine(TSTRING(ANTI_PASSIVE_SEARCH), WinUtil::m_ChatTextSystem);
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
	if(ShowUserList && !user->isSet(User::HIDDEN)) 
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
	if(ShowUserList) 
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

	redirect = Text::toT(line);
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
			speak(ADD_SILENT_STATUS_LINE, Util::toDOS(line));
		} else {
			speak(ADD_CHAT_LINE, Util::toDOS(line));
		}
	} else if((strstr(line.c_str(), "is kicking") != NULL) && (strstr(line.c_str(), "because:") != NULL) || 
		(strstr(line.c_str(), "Hub-Security") != NULL) && (strstr(line.c_str(), "was kicked by") != NULL)) {
		speak(KICK_MSG, Util::toDOS(line));		
	} else {
		speak(ADD_CHAT_LINE, Util::toDOS(line));
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
	speak(CHEATING_USER, STRING(USER)+" "+line);
}

void HubFrame::addClientLine(const tstring& aLine, CHARFORMAT2& cf, bool inChat /* = true */) {
	tstring line = _T("[") + Text::toT(Util::getShortTimeString()) + _T("] ") + aLine;

	ctrlStatus.SetText(0, line.c_str());
	while(lastLinesList.size() + 1 > MAX_CLIENT_LINES)
		lastLinesList.erase(lastLinesList.begin());
	lastLinesList.push_back(line);
	
	setDirty();
	
	if(BOOLSETTING(STATUS_IN_CHAT) && inChat) {
		addLine(_T("*** ") + aLine, cf);
	}
}

LRESULT HubFrame::onMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	RECT rc;
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click
	// Get the bounding rectangle of the client area. 
	ctrlClient.GetClientRect(&rc);
	ctrlClient.ScreenToClient(&pt); 
	if (PtInRect(&rc, pt)) { 
		sSelectedURL = _T("");
  		return TRUE;
	}
	return 1;
}

BOOL HubFrame::checkCheating(User::Ptr &user, DirectoryListing* dl) {
	if(user && dl && user->getClient()) {
		if(user->getClient()->getOp()) {
			int64_t statedSize = user->getBytesShared();
			int64_t realSize = dl->getTotalSize();
	
			double multiplier = ((100+(double)SETTING(PERCENT_FAKE_SHARE_TOLERATED))/100); 
			int64_t sizeTolerated = (int64_t)(realSize*multiplier);
			string detectString = "";
			string inflationString = "";
			user->setRealBytesShared(realSize);
			bool isFakeSharing = false;
			
			PME reg("^0.403");
			if(reg.match(user->getVersion()) && dl->detectRMDC403B7()) {
				user->setCheat("rmDC++ 0.403B[7] with DC++ emulation" , true);
				user->setClientType("rmDC++ 0.403B[7]");
				user->setBadClient(true);
				user->setBadFilelist(true);
				return true;
			}

			if((user->getVersion() == "0.403") && dl->detectRMDC403D1()) {
				user->setCheat("rmDC++ 0.403D[1] with DC++ emulation" , true);
				user->setClientType("rmDC++ 0.403D[1]");
				user->setBadClient(true);
				return true;
			}

			PME reg1("^<StrgDC\\+\\+ V:1.00 RC7");
			if(reg1.match(user->getTag()) && dl->detectRMDC403D1()) {
				user->setCheat("rmDC++ 0.403D[1] with StrongDC++ emulation" , true);
				user->setClientType("rmDC++ 0.403D[1]");
				user->setBadClient(true);
				user->setBadFilelist(true);
				return true;
			}

			if(statedSize > sizeTolerated) {
				isFakeSharing = true;
			}

			if(isFakeSharing) {
				user->setBadFilelist(true);
				detectString += STRING(CHECK_MISMATCHED_SHARE_SIZE);
				if(realSize == 0) {
					detectString += STRING(CHECK_0BYTE_SHARE);
				} else {
					double qwe = (double)((double)statedSize / (double)realSize);
					string str = Util::toString(qwe);
					char buf[128];
					_snprintf(buf, 127, CSTRING(CHECK_INFLATED), str);
					buf[127] = 0;
					inflationString = buf;
					detectString += inflationString;
				}
				detectString += STRING(CHECK_SHOW_REAL_SHARE);

				detectString = user->insertUserData(detectString);
				user->setCheat(Util::validateMessage(detectString, false), false);
				user->sendRawCommand(SETTING(FAKESHARE_RAW));
			}     
			user->setFilelistComplete(true);
			user->updated();
			if(isFakeSharing) return true;
		}
	}
	return false;
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

		sSelectedURL = _T("");
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
		sSelectedURL = _T("");
	}
	return 1;
}

bool HubFrame::PreparePopupMenu( CWindow *pCtrl, bool boCopyOnly, tstring& sNick, OMenu *pMenu ) {
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
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_NICK, CTSTRING(COPY_NICK));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_EXACT_SHARE, CTSTRING(COPY_EXACT_SHARE));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_DESCRIPTION, CTSTRING(COPY_DESCRIPTION));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_TAG, CTSTRING(COPY_TAG));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_EMAIL_ADDRESS, CTSTRING(COPY_EMAIL_ADDRESS));
	if ( client->getOp() ) { copyMenu.AppendMenu(MF_STRING, IDC_COPY_IP, CTSTRING(COPY_IP)); }
	if ( client->getOp() ) { copyMenu.AppendMenu(MF_STRING, IDC_COPY_NICK_IP, CTSTRING(COPY_NICK_IP)); }
	if ( client->getOp() ) { copyMenu.AppendMenu(MF_STRING, IDC_COPY_ISP, CTSTRING(COPY_ISP)); }
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_ALL, CTSTRING(COPY_ALL));
	copyMenu.InsertSeparator(0, TRUE, STRING(COPY));

	grantMenu.CreatePopupMenu();
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CTSTRING(GRANT_EXTRA_SLOT));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_HOUR, CTSTRING(GRANT_EXTRA_SLOT_HOUR));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_DAY, CTSTRING(GRANT_EXTRA_SLOT_DAY));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_WEEK, CTSTRING(GRANT_EXTRA_SLOT_WEEK));
	grantMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	grantMenu.AppendMenu(MF_STRING, IDC_UNGRANTSLOT, CTSTRING(REMOVE_EXTRA_SLOT));
	grantMenu.InsertSeparator(0, TRUE, STRING(GRANT_SLOTS_MENU));

	pMenu->CreatePopupMenu();
		
	if ( boCopyOnly ) {
		if(sSelectedIP != _T("")) {
			pMenu->InsertSeparator(0, TRUE, Text::fromT(sSelectedIP));
			pMenu->AppendMenu(MF_STRING, IDC_WHOIS_IP, (CTSTRING(WHO_IS) + sSelectedIP).c_str() );
			if ( client->getOp() ) {
				pMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
				pMenu->AppendMenu(MF_STRING, IDC_BAN_IP, (_T("!banip ") + sSelectedIP).c_str());
				pMenu->SetMenuDefaultItem(IDC_BAN_IP);
				pMenu->AppendMenu(MF_STRING, IDC_UNBAN_IP, (_T("!unban ") + sSelectedIP).c_str());
				pMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
			}
		} else pMenu->InsertSeparator(0, TRUE, "Text");
		pMenu->AppendMenu(MF_STRING, ID_EDIT_COPY, CTSTRING(COPY));
		pMenu->AppendMenu(MF_STRING, IDC_COPY_ACTUAL_LINE,  CTSTRING(COPY_LINE));
		if(sSelectedURL != _T("")) 
  			pMenu->AppendMenu(MF_STRING, IDC_COPY_URL, CTSTRING(COPY_URL));
		pMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
		pMenu->AppendMenu(MF_STRING, ID_EDIT_SELECT_ALL, CTSTRING(SELECT_ALL));
		pMenu->AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR));
		pMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
		pMenu->AppendMenu(MF_STRING, IDC_AUTOSCROLL_CHAT, CTSTRING(ASCROLL_CHAT));

		if ( ctrlClient.GetAutoScroll() )
			pMenu->CheckMenuItem(IDC_AUTOSCROLL_CHAT, MF_BYCOMMAND | MF_CHECKED);
	} else {
		if(sNick != _T("")) {
			// Jediny nick
			tstring sTmp = TSTRING(USER) + _T(" ") + sNick;
			pMenu->InsertSeparator(0, TRUE, Text::fromT(sTmp));
			if ( pCtrl == ((CWindow*) &ctrlClient )) {
				if(!BOOLSETTING(LOG_PRIVATE_CHAT)) {
				} else {
					pMenu->AppendMenu(MF_STRING, IDC_OPEN_USER_LOG, sTmp.c_str());
					pMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
				}				
			}
			if ( pCtrl != ( (CWindow*) &ctrlClient ) ) {
				pMenu->AppendMenu(MF_STRING, IDC_OPEN_USER_LOG, CTSTRING(OPEN_USER_LOG));
				pMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
			}
			pMenu->AppendMenu(MF_STRING, IDC_PUBLIC_MESSAGE, CTSTRING(SEND_PUBLIC_MESSAGE));
			if(sNick != Text::toT(sMyNick)) {
				pMenu->AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CTSTRING(SEND_PRIVATE_MESSAGE));
			}
			if(pCtrl == ((CWindow*) &ctrlClient)) {
				pMenu->AppendMenu(MF_STRING, IDC_SELECT_USER, CTSTRING(SELECT_USER_LIST));
			}
			pMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
			pMenu->AppendMenu(MF_POPUP, (UINT)(HMENU)copyMenu, CTSTRING(COPY));
			pMenu->AppendMenu(MF_POPUP, (UINT)(HMENU)grantMenu, CTSTRING(GRANT_SLOTS_MENU));
			if(sNick != Text::toT(sMyNick)) {
				int i = ctrlUsers.findItem(sNick);
				if ( i >= 0 ) {
					UserInfo* ui = (UserInfo*)ctrlUsers.getItemData(i);
					if (client->getOp() || !ui->user->isSet(User::OP)) {
				pMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
				pMenu->AppendMenu(MF_STRING, IDC_IGNORE, CTSTRING(IGNORE_USER));
				pMenu->AppendMenu(MF_STRING, IDC_UNIGNORE, CTSTRING(UNIGNORE_USER));
			}
				}
			}
			pMenu->SetMenuDefaultItem( IDC_PUBLIC_MESSAGE );
		} else {
			// Muze byt vice nicku
			if ( pCtrl == ( (CWindow*) &ctrlUsers ) ) {
				// Pocet oznacenych
				int iCount = ctrlUsers.GetSelectedCount();
				TCHAR sTmp[256];
				_stprintf(sTmp, _T("%i %s"), iCount, CTSTRING(HUB_USERS));
				pMenu->InsertSeparator(0, TRUE, Text::fromT(sTmp));
			}
			if (ctrlUsers.GetSelectedCount() <= 25) {
				pMenu->AppendMenu(MF_STRING, IDC_PUBLIC_MESSAGE, CTSTRING(SEND_PUBLIC_MESSAGE));
				pMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
			}
		}		
		pMenu->AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
		pMenu->AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CTSTRING(MATCH_QUEUE));
		pMenu->AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
		pMenu->SetMenuDefaultItem( IDC_GETLIST );
	}
	return true;
}

LRESULT HubFrame::onCopyActualLine(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if((GetFocus() == ctrlClient.m_hWnd) && (sSelectedLine != _T(""))) {
		WinUtil::setClipboard(sSelectedLine);
	}
	return 0;
}

LRESULT HubFrame::onSelectUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(sSelectedUser == _T("")) {
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

	if(sSelectedUser != _T("")) {
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
	if(sSelectedIP != _T("")) {
		tstring s = _T("!banip ") + sSelectedIP;
		client->hubMessage(Text::fromT(s));
	}
	return 0;
}

LRESULT HubFrame::onUnBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(sSelectedIP != _T("")) {
		tstring s = _T("!unban ") + sSelectedIP;
		client->hubMessage(Text::fromT(s));
	}
	return 0;
}

LRESULT HubFrame::onCopyURL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(sSelectedURL != _T("")) {
		WinUtil::setClipboard(sSelectedURL);
	}
	return 0;
}

LRESULT HubFrame::onClientEnLink(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	ENLINK* pEL = (ENLINK*)pnmh;

	if ( pEL->msg == WM_LBUTTONUP ) {
		long lBegin = pEL->chrg.cpMin, lEnd = pEL->chrg.cpMax;
		TCHAR sURLTemp[INTERNET_MAX_URL_LENGTH];
		int iRet = ctrlClient.GetTextRange(lBegin, lEnd, sURLTemp);
		UNREFERENCED_PARAMETER(iRet);
		tstring sURL = sURLTemp;
		WinUtil::openLink(sURL);
	} else if(pEL->msg == WM_RBUTTONUP) {
		sSelectedURL = _T("");
		long lBegin = pEL->chrg.cpMin, lEnd = pEL->chrg.cpMax;
		TCHAR sURLTemp[INTERNET_MAX_URL_LENGTH];
		int iRet = ctrlClient.GetTextRange(lBegin, lEnd, sURLTemp);
		UNREFERENCED_PARAMETER(iRet);
		sSelectedURL = sURLTemp;

		ctrlClient.SetSel(lBegin, lEnd);
		ctrlClient.InvalidateRect(NULL);
		return 0;
	}
	return 0;
}

LRESULT HubFrame::onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {	
	tstring file = Util::emptyStringT;
	tstring xNick;
	if(sSelectedUser != _T("")) {
		xNick = sSelectedUser;
	} else {
		int i = -1;
		while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
			xNick = Text::toT(((UserInfo*)ctrlUsers.getItemData(i))->user->getNick());
		}
	}
	if(xNick != _T("")) {
		file = Text::toT(Util::validateFileName(Text::fromT(Text::toT(SETTING(LOG_DIRECTORY)) + _T("PM\\") + xNick + _T(".log"))));
	}
	if(Util::fileExists(Text::fromT(file))) {
		ShellExecute(NULL, NULL, file.c_str(), NULL, NULL, SW_SHOWNORMAL);
	} else {
		MessageBox(CTSTRING(NO_LOG_FOR_USER),CTSTRING(NO_LOG_FOR_USER), MB_OK );	  
	}	

	return 0;
}

LRESULT HubFrame::onOpenHubLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring filename = Text::toT(Util::validateFileName(Text::fromT(Text::toT(SETTING(LOG_DIRECTORY)) + Text::toT(client->getAddressPort()) + _T(".log"))));
	if(Util::fileExists(Text::fromT(filename))){
		ShellExecute(NULL, NULL, filename.c_str(), NULL, NULL, SW_SHOWNORMAL);

	} else {
		MessageBox(CTSTRING(NO_LOG_FOR_HUB),CTSTRING(NO_LOG_FOR_HUB), MB_OK );	  
	}
	return 0;
}

LRESULT HubFrame::onWhoisIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(sSelectedIP != _T("")) {
 		WinUtil::openLink(_T("http://www.ripe.net/perl/whois?form_type=simple&full_query_string=&searchtext=") + sSelectedIP);
 	}
	return 0;
}

LRESULT HubFrame::onStyleChange(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	bHandled = FALSE;
	if((wParam & MK_LBUTTON) && ::GetCapture() == m_hWnd) {
		UpdateLayout(FALSE);
	}
	return 0;
}

LRESULT HubFrame::onStyleChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	bHandled = FALSE;
	UpdateLayout(FALSE);
	return 0;
}

LRESULT HubFrame::onFilterChar(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	TCHAR *buf = new TCHAR[ctrlFilter.GetWindowTextLength()+1];
	ctrlFilter.GetWindowText(buf, ctrlFilter.GetWindowTextLength()+1);
	filter = buf;
	free(buf);
	
	updateUserList();

	bHandled = false;

	return 0;
}

LRESULT HubFrame::onSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	TCHAR *buf = new TCHAR[ctrlFilter.GetWindowTextLength()+1];
	ctrlFilter.GetWindowText(buf, ctrlFilter.GetWindowTextLength()+1);
	filter = buf;
	free(buf);
	
	updateUserList();
	
	bHandled = false;

	return 0;
}

void HubFrame::updateUserList() {
	Lock l(updateCS);

	ctrlUsers.SetRedraw(FALSE);
	ctrlUsers.DeleteAllItems();

	if(filter.empty()) {
		UserMap::iterator i = userMap.begin();
		for(; i != userMap.end(); ++i){
			if(i->second != NULL)
				ctrlUsers.insertItem(i->second, WinUtil::getImage(i->second->user));	
		}
		ctrlUsers.SetRedraw(TRUE);
		return;
	}
	
	int sel = ctrlFilterSel.GetCurSel();

	UserMap::iterator i = userMap.begin();
	for(; i != userMap.end(); ++i){
		if( i->second != NULL ) {
			PME reg(Text::fromT(filter),"i");
			if(!reg.IsValid()) {
				ctrlUsers.insertItem(i->second, WinUtil::getImage(i->second->user));
			} else if(reg.match(Text::fromT(i->second->getText(sel)))) {
				ctrlUsers.insertItem(i->second, WinUtil::getImage(i->second->user));
			}
		}
	}

	ctrlUsers.SetRedraw(TRUE);
}

LRESULT HubFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		{
			UserInfo* ii = (UserInfo*)cd->nmcd.lItemlParam;
			try
			{
				if (ii->user->isFavoriteUser()) {
					cd->clrText = SETTING(FAVORITE_COLOR);
				}
				if (UploadManager::getInstance()->hasReservedSlot(ii->user)) {
					cd->clrText = SETTING(RESERVED_SLOT_COLOR);
				}
				if (ignoreList.count(Text::toT(ii->user->getNick()))) {
					cd->clrText = SETTING(IGNORED_COLOR);
				}
				if (client->getOp()) {				
					if (ii->user->getBadClient()) {
						cd->clrText = SETTING(BAD_CLIENT_COLOUR);
					} else if(ii->user->getBadFilelist()) {
						cd->clrText = SETTING(BAD_FILELIST_COLOUR);
					} else if(BOOLSETTING(SHOW_SHARE_CHECKED_USERS)) {
						bool cClient = ii->user->getTestSURComplete();
						bool cFilelist = ii->user->getFilelistComplete();
						if(cClient && cFilelist) {
							cd->clrText = SETTING(FULL_CHECKED_COLOUR);
						} else if(cClient) {
							cd->clrText = SETTING(CLIENT_CHECKED_COLOUR);
						} else if(cFilelist) {
							cd->clrText = SETTING(FILELIST_CHECKED_COLOUR);
						}
					}
				}
				return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
			}
			catch(const Exception&)
			{	
			}
			catch(...)
			{
			}
		}
		return CDRF_NOTIFYSUBITEMDRAW;

	default:
		return CDRF_DODEFAULT;
	}
}

void HubFrame::on(SettingsManagerListener::Save, SimpleXML* /*xml*/) throw() {
	bool refresh = false;
	if(ctrlUsers.GetBkColor() != WinUtil::bgColor) {
		ctrlClient.SetBackgroundColor(WinUtil::bgColor);
		ctrlUsers.SetBkColor(WinUtil::bgColor);
		ctrlUsers.SetTextBkColor(WinUtil::bgColor);
		ctrlUsers.setFlickerFree(WinUtil::bgBrush);
		refresh = true;
	}
	if(ctrlUsers.GetTextColor() != WinUtil::textColor) {
		ctrlUsers.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

LRESULT HubFrame::onSizeMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	bHandled = FALSE;
	ctrlClient.GoToEnd();
	return 0;
}

/**
 * @file
 * $Id$
 */