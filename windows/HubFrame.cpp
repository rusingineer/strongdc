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
#include "AGEmotionSetup.h"

#include "../client/QueueManager.h"
#include "../client/ShareManager.h"
#include "../client/Util.h"
#include "../client/StringTokenizer.h"
#include "../client/FavoriteManager.h"
#include "../client/LogManager.h"
#include "../client/AdcCommand.h"
#include "../client/SettingsManager.h"
#include "../client/ConnectionManager.h" 
#include "../client/NmdcHub.h"

#include "../client/pme.h"

HubFrame::FrameMap HubFrame::frames;

int HubFrame::columnSizes[] = { 100, 75, 75, 75, 100, 75, 40, 100, 40, 40, 40, 40, 40, 100, 100, 100, 175 };
int HubFrame::columnIndexes[] = { COLUMN_NICK, COLUMN_SHARED, COLUMN_EXACT_SHARED, COLUMN_DESCRIPTION, COLUMN_TAG,
	COLUMN_CONNECTION, COLUMN_EMAIL, COLUMN_CLIENTID, COLUMN_VERSION, COLUMN_MODE, COLUMN_HUBS, COLUMN_SLOTS,
	COLUMN_UPLOAD_SPEED, COLUMN_IP, COLUMN_PK, COLUMN_LOCK, COLUMN_SUPPORTS };
static ResourceManager::Strings columnNames[] = { ResourceManager::NICK, ResourceManager::SHARED, ResourceManager::EXACT_SHARED, 
ResourceManager::DESCRIPTION, ResourceManager::TAG, ResourceManager::CONNECTION, ResourceManager::EMAIL, 
ResourceManager::CLIENTID, ResourceManager::VERSION, ResourceManager::MODE, ResourceManager::HUBS, ResourceManager::SLOTS,
ResourceManager::AVERAGE_UPLOAD, ResourceManager::IP_BARE, ResourceManager::PK, ResourceManager::LOCK, ResourceManager::SUPPORTS };

tstring sSelectedURL = Util::emptyStringT;
long lURLBegin = 0;
long lURLEnd = 0;
extern CAGEmotionSetup* g_pEmotionsSetup;

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

	ctrlEmoticons.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_FLAT | BS_BITMAP | BS_CENTER, 0, IDC_EMOT);
	hEmoticonBmp = (HBITMAP) ::LoadImage(_Module.get_m_hInst(), MAKEINTRESOURCE(IDB_EMOTICON), IMAGE_BITMAP, 0, 0, LR_SHARED);

	ctrlEmoticons.SetBitmap(hEmoticonBmp);

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
	ctrlUsers.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | 0x00010000 | (BOOLSETTING(SHOW_INFOTIPS) ? LVS_EX_INFOTIP : 0));

	splitChat.Create( m_hWnd );
	splitChat.SetSplitterPanes(ctrlClient.m_hWnd, ctrlMessage.m_hWnd, true);
	splitChat.SetSplitterExtendedStyle(0);
	splitChat.SetSplitterPos( 40 );

	m_bVertical = BOOLSETTING(USE_VERTICAL_VIEW);
	if(m_bVertical) {
		SetSplitterPanes(ctrlClient.m_hWnd, ctrlUsers.m_hWnd, false);
		m_nProportionalPos = 7500;
	} else {
		SetSplitterPanes(ctrlUsers.m_hWnd, ctrlClient.m_hWnd, false);
		m_nProportionalPos = 2500;
	}

	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);

	if(hubchatusersplit)
		m_nProportionalPos = hubchatusersplit;

	ctrlShowUsers.Create(ctrlStatus.m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlShowUsers.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowUsers.SetFont(WinUtil::systemFont);
	ctrlShowUsers.SetCheck(showUsers ? BST_CHECKED : BST_UNCHECKED);
	showUsersContainer.SubclassWindow(ctrlShowUsers.m_hWnd);

	FavoriteHubEntry *fhe = FavoriteManager::getInstance()->getFavoriteHubEntry(Text::fromT(server));

	if(fhe) {
		WinUtil::splitTokens(columnIndexes, fhe->getHeaderOrder(), COLUMN_LAST);
		WinUtil::splitTokens(columnSizes, fhe->getHeaderWidths(), COLUMN_LAST);
	} else {
		WinUtil::splitTokens(columnIndexes, SETTING(HUBFRAME_ORDER), COLUMN_LAST);
		WinUtil::splitTokens(columnSizes, SETTING(HUBFRAME_WIDTHS), COLUMN_LAST);                           
	}
    	
	for(int j=0; j<UserInfo::COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SHARED || j == COLUMN_EXACT_SHARED || j == COLUMN_SLOTS) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlUsers.insertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlUsers.setColumnOrderArray(COLUMN_LAST, columnIndexes);

	if(fhe) {
		ctrlUsers.setVisible(fhe->getHeaderVisible());
    } else {
	    ctrlUsers.setVisible(SETTING(HUBFRAME_VISIBLE));
    }
	
	ctrlUsers.SetBkColor(WinUtil::bgColor);
	ctrlUsers.SetTextBkColor(WinUtil::bgColor);
	ctrlUsers.SetTextColor(WinUtil::textColor);
	ctrlUsers.setFlickerFree(WinUtil::bgBrush);
	ctrlClient.SetBackgroundColor(WinUtil::bgColor); 
	
	ctrlUsers.setSortColumn(UserInfo::COLUMN_NICK);
				
	ctrlUsers.SetImageList(WinUtil::userImages, LVSIL_SMALL);

	CToolInfo ti(TTF_SUBCLASS, ctrlStatus.m_hWnd);
	
	ctrlLastLines.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON, WS_EX_TOPMOST);
	ctrlLastLines.SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	ctrlLastLines.AddTool(&ti);
	ctrlLastLines.SetDelayTime(TTDT_AUTOPOP, 30000);

	copyHubMenu.CreatePopupMenu();
	copyHubMenu.AppendMenu(MF_STRING, IDC_COPY_HUBNAME, CTSTRING(HUB_NAME));
	copyHubMenu.AppendMenu(MF_STRING, IDC_COPY_HUBADDRESS, CTSTRING(HUB_ADDRESS));

	tabMenu.CreatePopupMenu();
	if(BOOLSETTING(LOG_MAIN_CHAT)) {
		tabMenu.AppendMenu(MF_STRING, IDC_OPEN_HUB_LOG, CTSTRING(OPEN_HUB_LOG));
		tabMenu.AppendMenu(MF_SEPARATOR);
	}
	tabMenu.AppendMenu(MF_STRING, IDC_ADD_AS_FAVORITE, CTSTRING(ADD_TO_FAVORITES));
	tabMenu.AppendMenu(MF_STRING, ID_FILE_RECONNECT, CTSTRING(MENU_RECONNECT));
	tabMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)copyHubMenu, CTSTRING(COPY));
	tabMenu.AppendMenu(MF_SEPARATOR);	
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));

	showJoins = BOOLSETTING(SHOW_JOINS);
	favShowJoins = BOOLSETTING(FAV_SHOW_JOINS);

	for(int j=0; j<COLUMN_LAST; ++j) {
		ctrlFilterSel.AddString(CTSTRING_I(columnNames[j]));
	}
	ctrlFilterSel.SetCurSel(0);

	bHandled = FALSE;

	client->addListener(this);
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
		, int windowposx, int windowposy, int windowsizex, int windowsizey, int windowtype, int chatusersplit, bool userliststate,
       string sColumsOrder, string sColumsWidth, string sColumsVisible) {
	FrameIter i = frames.find(aServer);
	if(i == frames.end()) {
		HubFrame* frm = new HubFrame(aServer
			, rawOne
			, rawTwo 
			, rawThree 
			, rawFour 
			, rawFive 
			, chatusersplit, userliststate);
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
					if(BOOLSETTING(JOIN_OPEN_NEW_WINDOW)) {
						HubFrame::openWindow(param);
					} else {
						BOOL whatever = FALSE;
						onFollow(0, 0, 0, whatever);
					}
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
				ctrlShowUsers.SetCheck(showUsers ? BST_UNCHECKED : BST_CHECKED);
			} else if(Util::stricmp(s.c_str(), _T("connection")) == 0) {
				addClientLine(Text::toT((STRING(IP) + client->getLocalIp() + ", " + STRING(PORT) + Util::toString(ConnectionManager::getInstance()->getPort()) + "/" + Util::toString(SearchManager::getInstance()->getPort()))), WinUtil::m_ChatTextSystem);
			} else if((Util::stricmp(s.c_str(), _T("favorite")) == 0) || (Util::stricmp(s.c_str(), _T("fav")) == 0)) {
				addAsFavorite();
			} else if((Util::stricmp(s.c_str(), _T("removefavorite")) == 0) || (Util::stricmp(s.c_str(), _T("removefav")) == 0)) {
				removeFavoriteHub();
			} else if(Util::stricmp(s.c_str(), _T("getlist")) == 0){
				if( !param.empty() ){
					int k = ctrlUsers.findItem(param);
					if(k != -1) {
						ctrlUsers.getItemData(k)->getList();
					}
				}
			} else if(Util::stricmp(s.c_str(), _T("log")) == 0) {
				StringMap params;
				params["hubNI"] = client->getHubName();
				params["hubURL"] = client->getHubUrl();
				params["myNI"] = client->getMyNick(); 
				if(param.empty()) {
					WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_MAIN_CHAT), params))));
				} else if(Util::stricmp(param.c_str(), _T("status")) == 0) {
					WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_STATUS), params))));
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
			} else if(Util::stricmp(s.c_str(), _T("ignorelist"))==0) {
				tstring ignorelist = _T("Ignored users:");
				for(TStringHash::const_iterator i = ignoreList.begin(); i != ignoreList.end(); ++i)
					ignorelist += _T(" ") + *i;
				addLine(ignorelist, WinUtil::m_ChatTextSystem);
			} else if(Util::stricmp(s.c_str(), _T("log")) == 0) {
				StringMap params;
				params["hubNI"] = client->getHubName();
				params["hubURL"] = client->getHubUrl();
				params["myNI"] = client->getMyNick(); 
				if(param.empty()) {
					WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_MAIN_CHAT), params))));
				} else if(Util::stricmp(param.c_str(), _T("status")) == 0) {
					WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_STATUS), params))));
				}
			} else if(Util::stricmp(s.c_str(), _T("help")) == 0) {
				addLine(_T("*** ") + WinUtil::commands + _T(", /smallfilesize #, /extraslots #, /savequeue, /join <hub-ip>, /clear, /ts, /showjoins, /favshowjoins, /close, /userlist, /connection, /favorite, /pm <user> [message], /getlist <user>, /winamp, /showblockedipports, /whois [IP], /ignorelist, /removefavorite"), WinUtil::m_ChatTextSystem);
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
			} else if(Util::stricmp(s.c_str(), _T("switch")) == 0) {
				m_bVertical = !m_bVertical;
				if(m_bVertical) {
					SetSplitterPanes(ctrlClient.m_hWnd, ctrlUsers.m_hWnd, false);
//					horPos = m_nProportionalPos;
//					m_nProportionalPos = verPos;
				} else {
					SetSplitterPanes(ctrlUsers.m_hWnd, ctrlClient.m_hWnd, false);
//					verPos = m_nProportionalPos;
//					m_nProportionalPos = horPos;
				}
				UpdateLayout();
			} else if(Util::stricmp(s.c_str(), _T("stats")) == 0) {
			    if(client->isOp())
					client->hubMessage(WinUtil::generateStats());
			    else
					addLine(Text::toT(WinUtil::generateStats()));
			} else {
				if (BOOLSETTING(SEND_UNKNOWN_COMMANDS)) {
					client->hubMessage(Text::fromT(m));
				} else {
					addClientLine(TSTRING(UNKNOWN_COMMAND) + s);
				}
			}
			ctrlMessage.SetWindowText(_T(""));
		} else if(waitingForPW) {
			addClientLine(TSTRING(DONT_REMOVE_SLASH_PASSWORD));
			ctrlMessage.SetWindowText(_T("/password "));
			ctrlMessage.SetFocus();
			ctrlMessage.SetSel(10, 10);
		} else {
			client->hubMessage(Text::fromT(s));
			ctrlMessage.SetWindowText(_T(""));
		}
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

	UserInfo* ui = i->second;
		
	if(ctrlUsers.getSortColumn() == UserInfo::COLUMN_NICK) {
		// Sort order of the other columns changes too late when the user's updated
		int a = ctrlUsers.getSortPos(ui);
		if(ctrlUsers.getItemData(a) == ui) {
			return a;
		}
	}
	return ctrlUsers.findItem(ui);
}

const tstring& HubFrame::getNick(const User::Ptr& aUser) {
	UserMapIter i = userMap.find(aUser);
	if(i == userMap.end())
		return Util::emptyStringT;

	UserInfo* ui = i->second;
	return ui->columns[COLUMN_NICK];
}

void HubFrame::addAsFavorite() {
	FavoriteHubEntry* existingHub = FavoriteManager::getInstance()->getFavoriteHubEntry(client->getHubUrl());
	if(!existingHub) {
		FavoriteHubEntry aEntry;
		TCHAR buf[256];
		this->GetWindowText(buf, 255);
		aEntry.setServer(Text::fromT(server));
		aEntry.setName(Text::fromT(buf));
		aEntry.setDescription(Text::fromT(buf));
		aEntry.setConnect(false);
		aEntry.setNick(client->getMyNick());
		aEntry.setPassword(client->getPassword());
		aEntry.setConnect(false);
		FavoriteManager::getInstance()->addFavorite(aEntry);
		addClientLine(TSTRING(FAVORITE_HUB_ADDED), WinUtil::m_ChatTextSystem );
	} else {
		addClientLine(TSTRING(FAVORITE_HUB_ALREADY_EXISTS), WinUtil::m_ChatTextSystem);
	}
}

void HubFrame::removeFavoriteHub() {
	FavoriteHubEntry* removeHub = FavoriteManager::getInstance()->getFavoriteHubEntry(client->getHubUrl());
	if(removeHub) {
		FavoriteManager::getInstance()->removeFavorite(removeHub);
		addClientLine(TSTRING(FAVORITE_HUB_REMOVED), WinUtil::m_ChatTextSystem);
	} else {
		addClientLine(TSTRING(FAVORITE_HUB_DOES_NOT_EXIST), WinUtil::m_ChatTextSystem);
	}
}

LRESULT HubFrame::onCopyHubInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if(client->isConnected()) {
        string sCopy;

		switch (wID) {
			case IDC_COPY_HUBNAME:
				sCopy += client->getHubName();
				break;
			case IDC_COPY_HUBADDRESS:
				sCopy += client->getHubUrl();
				break;
		}

		if (!sCopy.empty())
			WinUtil::setClipboard(Text::toT(sCopy));
    }
	return 0;
}

LRESULT HubFrame::onCopyUserInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if(client->isConnected()) {
        tstring sCopy;
		UserInfo* ui;

		if(sSelectedUser != _T("")) {
			int i = ctrlUsers.findItem(sSelectedUser);
			if ( i >= 0 ) {
				ui = (UserInfo*)ctrlUsers.getItemData(i);

				switch (wID) {
					case IDC_COPY_NICK:
						sCopy += ui->columns[COLUMN_NICK];
						break;
					case IDC_COPY_EXACT_SHARE:
						sCopy += Text::toT(Util::formatExactSize(ui->getBytes()));
						break;
					case IDC_COPY_DESCRIPTION:
						sCopy += ui->columns[COLUMN_DESCRIPTION];
						break;
					case IDC_COPY_TAG:
						sCopy += ui->columns[COLUMN_TAG];
						break;
					case IDC_COPY_EMAIL_ADDRESS:
						sCopy += ui->columns[COLUMN_EMAIL];
						break;
					case IDC_COPY_IP:
						sCopy +=  ui->columns[COLUMN_IP];
						break;
					case IDC_COPY_NICK_IP:
						sCopy += _T("Info User:\r\n")
							_T("\tNick: ") + ui->columns[COLUMN_NICK] + _T("\r\n") + 
							_T("\tIP: ") +  ui->columns[COLUMN_IP] + _T("\r\n"); 
						break;
					case IDC_COPY_ALL:
						sCopy += _T("Info User:\r\n")
							_T("\tNick: ") + ui->columns[COLUMN_NICK] + _T("\r\n") + 
							_T("\tShare: ") + Text::toT(Util::formatBytes(ui->getBytes())) + _T("\r\n") + 
							_T("\tDescription: ") + ui->columns[COLUMN_DESCRIPTION] + _T("\r\n") +
							_T("\tTag: ") + ui->columns[COLUMN_TAG] + _T("\r\n") +
							_T("\tConnection: ") + ui->columns[COLUMN_CONNECTION] + _T("\r\n") + 
							_T("\tE-Mail: ") + ui->columns[COLUMN_EMAIL] + _T("\r\n") +
							_T("\tClient: ") + ui->columns[COLUMN_CLIENTID] + _T("\r\n") + 
							_T("\tVersion: ") + ui->columns[COLUMN_VERSION] + _T("\r\n") +
							_T("\tMode: ") + ui->columns[COLUMN_MODE] + _T("\r\n") +
							_T("\tHubs: ") + ui->columns[COLUMN_HUBS] + _T("\r\n") +
							_T("\tSlots: ") + ui->columns[COLUMN_SLOTS] + _T("\r\n") +
							_T("\tUpLimit: ") + ui->columns[COLUMN_UPLOAD_SPEED] + _T("\r\n") +
							_T("\tIP: ") + ui->columns[COLUMN_IP] + _T("\r\n") +
							_T("\tPk String: ") + ui->columns[COLUMN_PK] + _T("\r\n") +
							_T("\tLock: " )+ ui->columns[COLUMN_LOCK] + _T("\r\n")+
							_T("\tSupports: ") + ui->columns[COLUMN_SUPPORTS];
						break;		
					default:
						dcdebug("HUBFRAME DON'T GO HERE\n");
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
	    switch(SETTING(USERLIST_DBLCLICK)) {
		    case 0:
				ctrlUsers.getItemData(item->iItem)->getList();
		        break;
		    case 1: {
				CAtlString sUser = Text::toT(ctrlUsers.getItemData(item->iItem)->getIdentity().getNick()).c_str();
	            CAtlString sText = "";
	            int iSelBegin, iSelEnd;
	            ctrlMessage.GetSel(iSelBegin, iSelEnd);
	            ctrlMessage.GetWindowText(sText);

	            if((iSelBegin == 0) && (iSelEnd == 0)) {
					if(sText.GetLength() == 0) {
						sUser += ": ";
			            ctrlMessage.SetWindowText(sUser);
			            ctrlMessage.SetFocus();
			            ctrlMessage.SetSel(ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength());
                    } else {
			            sUser += ": ";
			            ctrlMessage.ReplaceSel(sUser);
			            ctrlMessage.SetFocus();
                    }
				} else {
					sUser += " ";
                    ctrlMessage.ReplaceSel(sUser);
                    ctrlMessage.SetFocus();
	            }
				break;
		    }    
		    case 2:
				if(ctrlUsers.getItemData(item->iItem)->user != ClientManager::getInstance()->getMe())
		         	 ctrlUsers.getItemData(item->iItem)->pm();
		        break;
		    case 3:
		        ctrlUsers.getItemData(item->iItem)->matchQueue();
		        break;
		    case 4:
		        ctrlUsers.getItemData(item->iItem)->grant();
		        break;
		    case 5:
		        ctrlUsers.getItemData(item->iItem)->addFav();
		        break;
		}	
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

bool HubFrame::updateUser(const UpdateInfo& u) {
	UserMapIter i = userMap.find(u.user);
	if(i == userMap.end()) {
		UserInfo* ui = new UserInfo(u);
		userMap.insert(make_pair(u.user, ui));
		if(!ui->getIdentity().isHidden() && showUsers)
			filterUser(ui);

		client->availableBytes += ui->getBytes();
		return true;
	} else {
		UserInfo* ui = i->second;
		if(!ui->getIdentity().isHidden() && u.identity.isHidden() && showUsers) {
			ctrlUsers.deleteItem(ui);
		}
		
		client->availableBytes -= ui->getBytes();
		resort = ui->update(u.identity, ctrlUsers.getSortColumn()) || resort;
		client->availableBytes += ui->getBytes();

		if(showUsers) {
			int pos = ctrlUsers.findItem(ui);
			if(pos != -1) {
				ctrlUsers.updateItem(pos);
				ctrlUsers.SetItem(pos, 0, LVIF_IMAGE, NULL, WinUtil::getImage(u.identity), 0, 0, NULL);
			}
		}

		return false;
	}
}

void HubFrame::removeUser(const User::Ptr& aUser) {
	UserMap::iterator i = userMap.find(aUser);
	if(i == userMap.end()) {
		// Should never happen?
		dcassert(i != userMap.end());
		return;
	}

	UserInfo* ui = i->second;
	if(!ui->getIdentity().isHidden() && showUsers)
		ctrlUsers.deleteItem(ui);

	client->availableBytes -= ui->getBytes();
	userMap.erase(i);
	delete ui;
}

static const char* sSameNumbers[] = { "000000", "111111", "222222", "333333", "444444", "555555", "666666", "777777", "888888", "999999" };

LRESULT HubFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(wParam == UPDATE_USERS) {
		ctrlUsers.SetRedraw(FALSE);
		{
			Lock l(updateCS);
			for(UpdateIter i = updateList.begin(); i != updateList.end(); ++i) {
				UpdateInfo& u = const_cast<UpdateInfo&>(i->first);
				switch(i->second) {
				case UPDATE_USER:
					if(updateUser(u)) {
						bool isFavorite = FavoriteManager::getInstance()->isFavoriteUser(u.user);
						if (isFavorite && (!SETTING(SOUND_FAVUSER).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
							PlaySound(Text::toT(SETTING(SOUND_FAVUSER)).c_str(), NULL, SND_FILENAME | SND_ASYNC);

						if(isFavorite && BOOLSETTING(POPUP_FAVORITE_CONNECTED)) {
							MainFrame::getMainFrame()->ShowBalloonTip(Text::toT(u.identity.getNick() + " - " + client->getHubName()).c_str(), CTSTRING(FAVUSER_ONLINE));
						}

						if (showJoins || (favShowJoins && isFavorite)) {
						 	addLine(_T("*** ") + TSTRING(JOINS) + Text::toT(u.identity.getNick()), WinUtil::m_ChatTextSystem);
						}	

						if(client->isOp() && !u.identity.isOp()) {
							int64_t bytesSharedInt64 = u.identity.getBytesShared();
							if(bytesSharedInt64 > 0) {
								string bytesShared = Util::toString(bytesSharedInt64);
								bool samenumbers = false;
								for(int i = 0; i < 10; ++i) {
									if(strstr(bytesShared.c_str(), sSameNumbers[i]) != 0) {
										samenumbers = true;
										break;
									}
								}
								if(samenumbers) {
									string detectString = Util::formatExactSize(u.identity.getBytesShared())+" - the share size had too many same numbers in it";
									u.identity.setBadFilelist("1");
									u.identity.setCheat(*client, Util::validateMessage(detectString, false), false);
									
									CHARFORMAT2 cf;
									memset(&cf, 0, sizeof(CHARFORMAT2));
									cf.cbSize = sizeof(cf);
									cf.dwReserved = 0;
									cf.dwMask = CFM_BACKCOLOR | CFM_COLOR | CFM_BOLD;
									cf.dwEffects = 0;
									cf.crBackColor = SETTING(BACKGROUND_COLOR);
									cf.crTextColor = SETTING(ERROR_COLOR);

									char buf[512];
									sprintf(buf, "*** %s %s - %s", STRING(USER).c_str(), u.identity.getNick().c_str(), detectString.c_str());

									if(BOOLSETTING(POPUP_CHEATING_USER)) {
										MainFrame::getMainFrame()->ShowBalloonTip(Text::toT(buf).c_str(), CTSTRING(CHEATING_USER));
									}

									addLine(Text::toT(buf), cf);

									u.identity.sendRawCommand(*client, SETTING(FAKESHARE_RAW));
								}
							}
						
							if(BOOLSETTING(CHECK_NEW_USERS)) {
								if(u.identity.isTcpActive() || ClientManager::getInstance()->isActive(client->getHubUrl())) {
									try {
										QueueManager::getInstance()->addTestSUR(u.user, true);
									} catch(const Exception&) {
									}
								}
							}
						}
					}
					break;
				case UPDATE_USERS:
					updateUser(u);
					break;
				case REMOVE_USER:
					removeUser(u.user);
					if (showJoins || (favShowJoins && FavoriteManager::getInstance()->isFavoriteUser(u.user))) {
						addLine(Text::toT("*** " + STRING(PARTS) + u.identity.getNick()), WinUtil::m_ChatTextSystem);
					}

					break;
				}
			}
			updateList.clear();
		}

		if(resort && showUsers) {
			ctrlUsers.resort();
			resort = false;
		}

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

		addLine(*x,cf);
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
        MessageInfo* i = (MessageInfo*)lParam;
        if(!ignoreList.count(Text::toT(i->from.getNick())) ||
          (i->from.isOp() && !client->isOp())) {
            addLine(i->from, i->msg, WinUtil::m_ChatTextGeneral);
        }

		delete i;
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
		SetMDIFrameMenu();
		delete x;
	} else if(wParam == STATS) {
		size_t AllUsers = client->getUserCount();
		size_t ShownUsers = ctrlUsers.GetItemCount();
		if(filter.empty() == false && AllUsers != ShownUsers) {
			ctrlStatus.SetText(1, Text::toT(Util::toString(ShownUsers) + "/" + Util::toString(AllUsers) + " " + STRING(HUB_USERS)).c_str());
		} else {
			ctrlStatus.SetText(1, Text::toT(Util::toString(AllUsers) + " " + STRING(HUB_USERS)).c_str());
		}
		int64_t available = client->getAvailable();
		ctrlStatus.SetText(2, Text::toT(Util::formatBytes(available)).c_str());
		if(AllUsers > 0)
			ctrlStatus.SetText(3, Text::toT((Util::formatBytes(available / AllUsers) + "/" + STRING(USER))).c_str());
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
		MessageInfo* i = (MessageInfo*)lParam;
		tstring nick = Text::toT(i->from.getNick());
		if(!ignoreList.count(nick) || (i->from.isOp() && !client->isOp())) {
			if(i->replyTo->isOnline()) {
				if(BOOLSETTING(POPUP_PMS) || PrivateFrame::isOpen(i->replyTo)) {
						PrivateFrame::gotMessage(i->from, i->to, i->replyTo, i->msg);
					} else {
						addLine(TSTRING(PRIVATE_MESSAGE_FROM) + nick + _T(": ") + i->msg, WinUtil::m_ChatTextPrivate);
					}
				if(BOOLSETTING(MINIMIZE_TRAY) && !i->from.getConnection().empty()) {
					HWND hMainWnd = MainFrame::getMainFrame()->m_hWnd;//GetTopLevelWindow();
					::PostMessage(hMainWnd, WM_SPEAKER, MainFrame::SET_PM_TRAY_ICON, NULL);
				}
			} else {
				if(BOOLSETTING(IGNORE_OFFLINE)) {
					addClientLine(TSTRING(IGNORED_MESSAGE) + i->msg, WinUtil::m_ChatTextPrivate, false);
				} else if(BOOLSETTING(POPUP_OFFLINE)) {
					PrivateFrame::gotMessage(i->from, i->to, i->replyTo, i->msg);
				} else {
					addLine(TSTRING(PRIVATE_MESSAGE_FROM) + nick + _T(": ") + i->msg, WinUtil::m_ChatTextPrivate);
				}
			}
		}
		delete i;
	} else if(wParam == KICK_MSG) {
        MessageInfo* i = (MessageInfo*)lParam;
        if(SETTING(FILTER_MESSAGES)) {
            addClientLine(i->msg, false);
        } else {
    		addLine(i->from, i->msg, WinUtil::m_ChatTextServer, false);
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

		// Strange, can't get the correct width of the last field...
		ctrlStatus.GetRect(3, sr);
		sr.left = sr.right + 2;
		sr.right = sr.left + 16;
		ctrlShowUsers.MoveWindow(sr);
	}
	int h = WinUtil::fontHeight + 4;

	CRect rc = rect;
	rc.bottom -= h + 10;
	if(!showUsers) {
		if(GetSinglePaneMode() == SPLIT_PANE_NONE)
			SetSinglePaneMode(m_bVertical ? SPLIT_PANE_LEFT : SPLIT_PANE_RIGHT);
	} else {
		if(GetSinglePaneMode() != SPLIT_PANE_NONE)
			SetSinglePaneMode(SPLIT_PANE_NONE);
	}
	SetSplitterRect(rc);

	rc = rect;
	rc.bottom -= 2;
	rc.top = rc.bottom - h - 5;
	rc.left +=2;
	rc.right -= (showUsers ? 202 : 2) + 24;
	ctrlMessage.MoveWindow(rc);

	rc.left = rc.right + 2;
	rc.right += 24;

	ctrlEmoticons.MoveWindow(rc);
	if(showUsers){
		rc.left = rc.right + 2;
		rc.right = rc.left + 116;
		ctrlFilter.MoveWindow(rc);

		rc.left = rc.right + 4;
		rc.right = rc.left + 76;
		rc.top = rc.top + 0;
		rc.bottom = rc.bottom + 120;
		ctrlFilterSel.MoveWindow(rc);
	} else {
		rc.left = 0;
		rc.right = 0;
		ctrlFilter.MoveWindow(rc);
		ctrlFilterSel.MoveWindow(rc);
	}
}

LRESULT HubFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		RecentHubEntry* r = FavoriteManager::getInstance()->getRecentHubEntry(Text::fromT(server));
		if(r) {
			TCHAR buf[256];
			this->GetWindowText(buf, 255);
			r->setName(Text::fromT(buf));
			r->setUsers(Util::toString(client->getUserCount()));
			r->setShared(Util::toString(client->getAvailable()));
			FavoriteManager::getInstance()->updateRecent(r);
		}
		DeleteObject(hEmoticonBmp);

		TimerManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		client->removeListener(this);
		client->disconnect(true);

		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		SettingsManager::getInstance()->set(SettingsManager::GET_USER_INFO, showUsers);
		FavoriteManager::getInstance()->removeUserCommand(Text::fromT(server));

		clearUserList();
		string tmp, tmp2, tmp3;
		ctrlUsers.saveHeaderOrder(tmp, tmp2, tmp3);

		FavoriteHubEntry *fhe = FavoriteManager::getInstance()->getFavoriteHubEntry(Text::fromT(server));
		if(fhe != NULL){
			WINDOWPLACEMENT wp;
			wp.length = sizeof(wp);
			GetWindowPlacement(&wp);

			CRect rc;
			GetWindowRect(rc);
			CRect rcmdiClient;
			::GetWindowRect(WinUtil::mdiClient, &rcmdiClient);
			if(wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWNORMAL) {
				fhe->setWindowPosX(rc.left - (rcmdiClient.left + 2));
				fhe->setWindowPosY(rc.top - (rcmdiClient.top + 2));
				fhe->setWindowSizeX(rc.Width());
				fhe->setWindowSizeY(rc.Height());
			}
			if(wp.showCmd == SW_SHOWNORMAL || wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_MAXIMIZE)
				fhe->setWindowType((int)wp.showCmd);
			fhe->setChatUserSplit(m_nProportionalPos);
			fhe->setUserListState(showUsers);
			fhe->setHeaderOrder(tmp);
			fhe->setHeaderWidths(tmp2);
			fhe->setHeaderVisible(tmp3);
			
			FavoriteManager::getInstance()->save();
		} else {
			SettingsManager::getInstance()->set(SettingsManager::HUBFRAME_ORDER, tmp);
			SettingsManager::getInstance()->set(SettingsManager::HUBFRAME_WIDTHS, tmp2);
			SettingsManager::getInstance()->set(SettingsManager::HUBFRAME_VISIBLE, tmp3);
		}

		bHandled = FALSE;
		return 0;
	}
}

void HubFrame::clearUserList() {
	{
		Lock l(updateCS);
		updateList.clear();
	}

	for(UserMapIter i = userMap.begin(); i != userMap.end(); ++i) {
		delete i->second;
	}
	ctrlUsers.DeleteAllItems();
	userMap.clear();
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
					PrivateFrame::openWindow(ctrlUsers.getItemData(pos)->user);
				} else if (wParam & MK_SHIFT) {
					try {
						QueueManager::getInstance()->addList((ctrlUsers.getItemData(pos))->user, QueueItem::FLAG_CLIENT_VIEW);
					} catch(const Exception& e) {
					addClientLine(Text::toT(e.getError()), WinUtil::m_ChatTextSystem);
					}
				} else {
				switch(SETTING(CHAT_DBLCLICK)) {
					case 0: {
						int items = ctrlUsers.GetItemCount();
						ctrlUsers.SetRedraw(FALSE);
						for(int i = 0; i < items; ++i) {
							ctrlUsers.SetItemState(i, (i == pos) ? LVIS_SELECTED | LVIS_FOCUSED : 0, LVIS_SELECTED | LVIS_FOCUSED);
						}
						ctrlUsers.SetRedraw(TRUE);
						ctrlUsers.EnsureVisible(pos, FALSE);
					     break;
					}    
					case 1: {
					     CAtlString sUser = Text::toT(ctrlUsers.getItemData(pos)->getIdentity().getNick()).c_str();
					     CAtlString sText = "";
					     int iSelBegin, iSelEnd;
					     ctrlMessage.GetSel(iSelBegin, iSelEnd);
					     ctrlMessage.GetWindowText(sText);

					     if((iSelBegin == 0) && (iSelEnd == 0)) {
					          if(sText.GetLength() == 0) {
                                   sUser += ": ";
			                       ctrlMessage.SetWindowText(sUser);
			                       ctrlMessage.SetFocus();
			                       ctrlMessage.SetSel(ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength());
					          } else {
			                       sUser += ": ";
			                       ctrlMessage.ReplaceSel(sUser);
			                       ctrlMessage.SetFocus();
					          }
					     } else {
					          sUser += " ";
					          ctrlMessage.ReplaceSel(sUser);
					          ctrlMessage.SetFocus();
					     }
					     break;
					}
					case 2:
						if(ctrlUsers.getItemData(pos)->user != ClientManager::getInstance()->getMe())
					          ctrlUsers.getItemData(pos)->pm();
					     break;
					case 3:
					     ctrlUsers.getItemData(pos)->getList();
					     break;
					case 4:
					     ctrlUsers.getItemData(pos)->matchQueue();
					     break;
					case 5:
					     ctrlUsers.getItemData(pos)->grant();
					     break;
					case 6:
					     ctrlUsers.getItemData(pos)->addFav();
					     break;
				}
			}
		}
	}
	return 0;
}

void HubFrame::addLine(const tstring& aLine) {
	addLine(aLine, WinUtil::m_ChatTextGeneral );
}

void HubFrame::addLine(const tstring& aLine, CHARFORMAT2& cf, bool bUseEmo/* = true*/) {
	Identity i = Identity(NULL, Util::emptyString);
    addLine(i, aLine, cf, bUseEmo);
}

void HubFrame::addLine(Identity& i, const tstring& aLine, CHARFORMAT2& cf, bool bUseEmo/* = true*/) {
	ctrlClient.AdjustTextSize();

	if(BOOLSETTING(LOG_MAIN_CHAT)) {
		StringMap params;
		params["message"] = Text::fromT(aLine);
		client->getHubIdentity().getParams(params, "hub", false);
		params["hubURL"] = client->getHubUrl();
		client->getMyIdentity().getParams(params, "my", true);
		LOG(LogManager::CHAT, params);
	}

	bool bMyMess = i.getNick() == client->getMyNick();
	if(timeStamps) {
		ctrlClient.AppendText(i, Text::toT(client->getMyNick()).c_str(), bMyMess, Text::toT("[" + Util::getShortTimeString() + "] ").c_str(), aLine.c_str(), cf, bUseEmo);
	} else {
		ctrlClient.AppendText(i, Text::toT(client->getMyNick()).c_str(), bMyMess, _T(""), aLine.c_str(), cf, bUseEmo);
	}
	if (BOOLSETTING(BOLD_HUB)) {
		setDirty();
	}
}

LRESULT HubFrame::onTabContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 
	tabMenuShown = true;
	CMenu hSysMenu;
	tabMenu.InsertSeparatorFirst((client->getHubName() != "") ? (client->getHubName().size() > 50 ? client->getHubName().substr(0, 50) : client->getHubName()) : client->getHubUrl());	
	copyHubMenu.InsertSeparatorFirst(STRING(COPY));
	
	if(!client->isConnected())
		tabMenu.EnableMenuItem((UINT)(HMENU)copyHubMenu, MF_GRAYED);
	else
		tabMenu.EnableMenuItem((UINT)(HMENU)copyHubMenu, MF_ENABLED);

	prepareMenu(tabMenu, ::UserCommand::CONTEXT_HUB, client->getHubUrl());
	hSysMenu.Attach((wParam == NULL) ? (HMENU)tabMenu : (HMENU)wParam);
	if (wParam != NULL) {
		hSysMenu.InsertMenu(hSysMenu.GetMenuItemCount() - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)(HMENU)tabMenu, /*CTSTRING(USER_COMMANDS)*/ _T("User Commands"));
		hSysMenu.InsertMenu(hSysMenu.GetMenuItemCount() - 1, MF_BYPOSITION | MF_SEPARATOR);
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

LRESULT HubFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	CRect rc;            // client area of window 
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click
	tabMenuShown = false;
	OMenu Mnu;
	sSelectedUser = _T("");

	ctrlUsers.GetHeader().GetWindowRect(&rc);
		
	if(PtInRect(&rc, pt) && showUsers) {
		ctrlUsers.showMenu(pt);
		return TRUE;
	}
		
	if(reinterpret_cast<HWND>(wParam) == ctrlUsers && showUsers) { 
		if ( ctrlUsers.GetSelectedCount() == 1 ) {
			if(pt.x == -1 && pt.y == -1) {
				WinUtil::getContextMenuPos(ctrlUsers, pt);
	
	
	
			}
			int i = -1;
			i = ctrlUsers.GetNextItem(i, LVNI_SELECTED);
			if ( i >= 0 ) {
				sSelectedUser = Text::toT(((UserInfo*)ctrlUsers.getItemData(i))->getIdentity().getNick());
			}
		}

		if(PreparePopupMenu(&ctrlUsers, sSelectedUser, &Mnu)) {
			prepareMenu(Mnu, ::UserCommand::CONTEXT_CHAT, client->getHubUrl());
			
			if(!(client->getSupportFlags() & NmdcHub::SUPPORTS_QUICKLIST)) {
				if(!(Mnu.GetMenuState(Mnu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {	
					Mnu.AppendMenu(MF_SEPARATOR);
				}
				Mnu.AppendMenu(MF_STRING, IDC_REFRESH, CTSTRING(REFRESH_USER_LIST));
			}

			if(ctrlUsers.GetSelectedCount() > 0)
				Mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
			cleanMenu(Mnu);
			if(copyMenu != NULL) copyMenu.DestroyMenu();
			if(grantMenu != NULL) grantMenu.DestroyMenu();
			if(Mnu != NULL) Mnu.DestroyMenu();
			return TRUE; 
		}
	}

	if(reinterpret_cast<HWND>(wParam) == ctrlEmoticons) { 
		if(emoMenu != NULL) emoMenu.DestroyMenu();
		emoMenu.CreatePopupMenu();
		menuItems = 0;
		emoMenu.AppendMenu(MF_STRING, IDC_EMOMENU, _T("Disabled"));
		if (SETTING(EMOTICONS_FILE)=="Disabled") emoMenu.CheckMenuItem( IDC_EMOMENU, MF_BYCOMMAND | MF_CHECKED );
		// nacteme seznam emoticon packu (vsechny *.xml v adresari EmoPacks)
		WIN32_FIND_DATA data;
		HANDLE hFind;
		hFind = FindFirstFile(Text::toT(Util::getAppPath()+"EmoPacks\\*.xml").c_str(), &data);
		if(hFind != INVALID_HANDLE_VALUE) {
			do {
				tstring name = data.cFileName;
				tstring::size_type i = name.rfind('.');
				name = name.substr(0, i);

				menuItems++;
				emoMenu.AppendMenu(MF_STRING, IDC_EMOMENU + menuItems, name.c_str());
				if(name == Text::toT(SETTING(EMOTICONS_FILE))) emoMenu.CheckMenuItem( IDC_EMOMENU + menuItems, MF_BYCOMMAND | MF_CHECKED );
			} while(FindNextFile(hFind, &data));
			FindClose(hFind);
		}
		emoMenu.InsertSeparatorFirst("Emoticons Pack");
		if(menuItems>0) emoMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		emoMenu.RemoveFirstItem();
		return TRUE;
	}

	if(reinterpret_cast<HWND>(wParam) == ctrlClient) { 
		if(pt.x == -1 && pt.y == -1) {
			CRect erc;
			ctrlClient.GetRect(&erc);
			pt.x = erc.Width() / 2;
			pt.y = erc.Height() / 2;
			ctrlClient.ClientToScreen(&pt);
		}
		POINT ptCl = pt;
		ctrlClient.ScreenToClient(&ptCl); 
		ctrlClient.OnRButtonDown(ptCl);

		sSelectedUser = ChatCtrl::sTempSelectedUser;
		bool boHitURL = ctrlClient.HitURL();
		if (!boHitURL)
			sSelectedURL = _T("");

		if(PreparePopupMenu(&ctrlClient, sSelectedUser, &Mnu )) {
			if(sSelectedUser == _T("")) {
				Mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
				if(copyMenu != NULL) copyMenu.DestroyMenu();
				if(grantMenu != NULL) grantMenu.DestroyMenu();
				if(Mnu != NULL) Mnu.DestroyMenu();
				return TRUE;
			} else {
				prepareMenu(Mnu, ::UserCommand::CONTEXT_CHAT, client->getHubUrl());
				if(!(Mnu.GetMenuState(Mnu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {	
					Mnu.AppendMenu(MF_SEPARATOR);
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
	StringMap ucParams;
	if(!WinUtil::getUCParams(m_hWnd, uc, ucParams))
		return;

	client->getMyIdentity().getParams(ucParams, "my", true);
	client->getHubIdentity().getParams(ucParams, "hub", false);

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
					
					u->getIdentity().getParams(tmp, "user", true);
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

					u->getIdentity().getParams(tmp, "user", true);
					client->escapeParams(tmp);
					client->sendUserCmd(Util::formatParams(uc.getCommand(), tmp));
				}
			}
		}
	}
	return;
};

void HubFrame::onTab() {
	HWND focus = GetFocus();
	if( (focus == ctrlMessage.m_hWnd) && 
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
				ctrlUsers.SetFocus();
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
			const tstring& nick = ui->columns[COLUMN_NICK];
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
	}

	if(focus == ctrlClient.m_hWnd) {
		ctrlMessage.SetFocus();
	} else if(focus == ctrlUsers.m_hWnd) {
		ctrlClient.SetFocus();
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
		if ((uMsg == WM_CHAR) && (GetFocus() == ctrlMessage.m_hWnd) && (wParam != VK_RETURN) && (wParam != VK_TAB) && (wParam != VK_BACK)) {
			if ((!SETTING(SOUND_TYPING_NOTIFY).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
				PlaySound(Text::toT(SETTING(SOUND_TYPING_NOTIFY)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
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
				// move cursor to end of line
				ctrlMessage.SetSel(ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength());
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
				// move cursor to end of line
				ctrlMessage.SetSel(ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength());
			} else {
				bHandled = FALSE;
			}

			break;
		case VK_PRIOR: // page up
			ctrlClient.SendMessage(WM_VSCROLL, SB_PAGEUP);

			break;
		case VK_NEXT: // page down
			ctrlClient.SendMessage(WM_VSCROLL, SB_PAGEDOWN);

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
		showUsers = true;
		ctrlUsers.SetRedraw(FALSE);
		ctrlUsers.DeleteAllItems();
		
		for(UserMapIter i = userMap.begin(); i != userMap.end(); ++i) {
			filterUser(i->second);
		}

		ctrlUsers.SetRedraw(TRUE);
		ctrlUsers.resort();
	} else {
		showUsers = false;
	}

	SettingsManager::getInstance()->set(SettingsManager::GET_USER_INFO, showUsers);

	UpdateLayout(FALSE);
	return 0;
}

LRESULT HubFrame::onFollow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!redirect.empty()) {
		if(ClientManager::getInstance()->isConnected(Text::fromT(redirect))) {
			addClientLine(TSTRING(REDIRECT_ALREADY_CONNECTED));
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
		FavoriteManager::getInstance()->addRecent(r);

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
    TCHAR* sLine = (TCHAR*)line.c_str();

    if(_tcslen(sLine) > 512) {
        sLine[512] = NULL;
    }

	ctrlStatus.SetText(0, sLine);
	while(lastLinesList.size() + 1 > MAX_CLIENT_LINES)
		lastLinesList.erase(lastLinesList.begin());
	lastLinesList.push_back(sLine);

	if (BOOLSETTING(BOLD_HUB)) {
		setDirty();
	}
	
	if(BOOLSETTING(STATUS_IN_CHAT) && inChat) {
		addLine(_T("*** ") + aLine, WinUtil::m_ChatTextSystem);
	}
	if(BOOLSETTING(LOG_STATUS_MESSAGES)) {
		StringMap params;
		client->getHubIdentity().getParams(params, "hub", false);
		params["hubURL"] = client->getHubUrl();
		client->getMyIdentity().getParams(params, "my", true);
		params["message"] = Text::fromT(aLine);
		LOG(LogManager::STATUS, params);
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
	if(updateUsers) {
		updateStatusBar();
		updateUsers = false;
		PostMessage(WM_SPEAKER, UPDATE_USERS);

	}
}

void HubFrame::on(Connecting, Client* c) throw() { 
	if(BOOLSETTING(SEARCH_PASSIVE) && ClientManager::getInstance()->isActive(c->getHubUrl())) {
		addLine(TSTRING(ANTI_PASSIVE_SEARCH), WinUtil::m_ChatTextSystem);
	}
	speak(ADD_STATUS_LINE, STRING(CONNECTING_TO) + client->getHubUrl() + "...");
	speak(SET_WINDOW_TITLE, client->getHubUrl());
}
void HubFrame::on(Connected, Client*) throw() { 
	speak(CONNECTED);
}
void HubFrame::on(BadPassword, Client*) throw() { 
	client->setPassword(Util::emptyString);
}
void HubFrame::on(UserUpdated, Client*, const OnlineUser& user) throw() { 
	speak(UPDATE_USER, user);
}
void HubFrame::on(UsersUpdated, Client*, const OnlineUser::List& aList) throw() {
	Lock l(updateCS);
	updateList.reserve(aList.size());
	for(OnlineUser::List::const_iterator i = aList.begin(); i != aList.end(); ++i) {
		if(!(*i)->getIdentity().isHidden())
			updateList.push_back(make_pair(UpdateInfo(*(*i)), UPDATE_USERS));
	}
	if(!updateList.empty()) {
		PostMessage(WM_SPEAKER, UPDATE_USERS);
	}
}

void HubFrame::on(UserRemoved, Client*, const OnlineUser& user) throw() {
	speak(REMOVE_USER, user);
}

void HubFrame::on(Redirect, Client*, const string& line) throw() { 
	if(ClientManager::getInstance()->isConnected(line)) {
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
	speak(SET_WINDOW_TITLE, Util::validateMessage(client->getHubName() + " " + client->getHubDescription(), true, false) + " (" + client->getHubUrl() + ")");
}
void HubFrame::on(Message, Client*, const OnlineUser& from, const char* line) throw() {
    char *temp, *chatline = (char *)line;
	if(!(&from) || from.getIdentity().isOp()) {
		if((temp = strchr(chatline, '\n')) != NULL) {
            temp[0] = NULL;
 	     	if((strstr(chatline, "is kicking") != NULL) && (strstr(chatline, "because:") != NULL) || 
			  (strstr(chatline, "was kicked by") != NULL)) {
    			temp[0] = '\n';
				speak(KICK_MSG, from, NULL, NULL, Util::toDOS(line));		
			    return;
  			}
 	       temp[0] = '\n';
		} else if((strstr(chatline, "is kicking") != NULL) && (strstr(chatline, "because:") != NULL) || 
			(strstr(chatline, "was kicked by") != NULL)) {
			speak(KICK_MSG, from, NULL, NULL, Util::toDOS(line));
 	       return;	
		}
    }
	
	speak(ADD_CHAT_LINE, from, NULL, NULL, Util::toDOS(line));
}

void HubFrame::on(PrivateMessage, Client*, const OnlineUser& from, const OnlineUser& to, const OnlineUser& replyTo, const string& line) throw() { 
	speak(PRIVATE_MESSAGE, from, to, replyTo, Util::toDOS("<" + from.getIdentity().getNick() + "> " + line));
}
void HubFrame::on(NickTaken, Client*) throw() { 
	speak(ADD_STATUS_LINE, STRING(NICK_TAKEN));
	speak(DISCONNECTED);
}
void HubFrame::on(SearchFlood, Client*, const string& line) throw() {
	speak(ADD_STATUS_LINE, STRING(SEARCH_SPAM_FROM) + line);
}
void HubFrame::on(CheatMessage, Client*, const string& line) throw() {
	speak(CHEATING_USER, line);
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

LRESULT HubFrame::onMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
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

LRESULT HubFrame::onRButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
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

bool HubFrame::PreparePopupMenu(CWindow *pCtrl, tstring& sNick, OMenu *pMenu ) {
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
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_IP, CTSTRING(COPY_IP));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_NICK_IP, CTSTRING(COPY_NICK_IP));

	copyMenu.AppendMenu(MF_STRING, IDC_COPY_ALL, CTSTRING(COPY_ALL));
	copyMenu.InsertSeparator(0, TRUE, STRING(COPY));

	grantMenu.CreatePopupMenu();
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CTSTRING(GRANT_EXTRA_SLOT));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_HOUR, CTSTRING(GRANT_EXTRA_SLOT_HOUR));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_DAY, CTSTRING(GRANT_EXTRA_SLOT_DAY));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_WEEK, CTSTRING(GRANT_EXTRA_SLOT_WEEK));
	grantMenu.AppendMenu(MF_SEPARATOR);
	grantMenu.AppendMenu(MF_STRING, IDC_UNGRANTSLOT, CTSTRING(REMOVE_EXTRA_SLOT));
	grantMenu.InsertSeparator(0, TRUE, STRING(GRANT_SLOTS_MENU));

	pMenu->CreatePopupMenu();
		
    bool bIsChat = (pCtrl == ((CWindow*)&ctrlClient));
	if(sNick == _T("") && bIsChat == true) {
		if(ChatCtrl::sSelectedIP != _T("")) {
			pMenu->InsertSeparator(0, TRUE, Text::fromT(ChatCtrl::sSelectedIP));
			pMenu->AppendMenu(MF_STRING, IDC_WHOIS_IP, (CTSTRING(WHO_IS) + ChatCtrl::sSelectedIP).c_str() );
			if ( client->isOp() ) {
				pMenu->AppendMenu(MF_SEPARATOR);
				pMenu->AppendMenu(MF_STRING, IDC_BAN_IP, (_T("!banip ") + ChatCtrl::sSelectedIP).c_str());
				pMenu->SetMenuDefaultItem(IDC_BAN_IP);
				pMenu->AppendMenu(MF_STRING, IDC_UNBAN_IP, (_T("!unban ") + ChatCtrl::sSelectedIP).c_str());
				pMenu->AppendMenu(MF_SEPARATOR);
			}
		} else pMenu->InsertSeparator(0, TRUE, "Text");
		pMenu->AppendMenu(MF_STRING, ID_EDIT_COPY, CTSTRING(COPY));
		pMenu->AppendMenu(MF_STRING, IDC_COPY_ACTUAL_LINE,  CTSTRING(COPY_LINE));
		if(sSelectedURL != _T("")) 
  			pMenu->AppendMenu(MF_STRING, IDC_COPY_URL, CTSTRING(COPY_URL));
		pMenu->AppendMenu(MF_SEPARATOR);
		pMenu->AppendMenu(MF_STRING, ID_EDIT_SELECT_ALL, CTSTRING(SELECT_ALL));
		pMenu->AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR));
		pMenu->AppendMenu(MF_SEPARATOR);
		pMenu->AppendMenu(MF_STRING, IDC_AUTOSCROLL_CHAT, CTSTRING(ASCROLL_CHAT));

		if ( ctrlClient.GetAutoScroll() )
			pMenu->CheckMenuItem(IDC_AUTOSCROLL_CHAT, MF_BYCOMMAND | MF_CHECKED);
	} else {
		if(sNick != _T("")) {
		    bool bIsMe = (sNick == Text::toT(client->getMyNick()));
			// Jediny nick
			tstring sTmp = TSTRING(USER) + _T(" ") + sNick;
			pMenu->InsertSeparator(0, TRUE, Text::fromT(sTmp));

			if(bIsChat) {
				if(!BOOLSETTING(LOG_PRIVATE_CHAT)) {
					pMenu->AppendMenu(MF_STRING | MF_DISABLED, (UINT_PTR)0,  CTSTRING(OPEN_USER_LOG));
				} else {
					pMenu->AppendMenu(MF_STRING, IDC_OPEN_USER_LOG,  CTSTRING(OPEN_USER_LOG));
				}				
				pMenu->AppendMenu(MF_SEPARATOR);
			}
			if(bIsChat == false && bIsMe == false && BOOLSETTING(LOG_PRIVATE_CHAT) == true) {
				pMenu->AppendMenu(MF_STRING, IDC_OPEN_USER_LOG, CTSTRING(OPEN_USER_LOG));
				pMenu->AppendMenu(MF_SEPARATOR);
			}
			if(bIsMe == false) {
				pMenu->AppendMenu(MF_STRING, IDC_PUBLIC_MESSAGE, CTSTRING(SEND_PUBLIC_MESSAGE));
				pMenu->AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CTSTRING(SEND_PRIVATE_MESSAGE));
			}
			if(bIsChat) {
				pMenu->AppendMenu(MF_STRING, IDC_SELECT_USER, CTSTRING(SELECT_USER_LIST));
			}
			if(bIsMe == false) {
				pMenu->AppendMenu(MF_SEPARATOR);
			}	   
			pMenu->AppendMenu(MF_POPUP, (UINT)(HMENU)copyMenu, CTSTRING(COPY));
			if(bIsMe == false) {
				pMenu->AppendMenu(MF_POPUP, (UINT)(HMENU)grantMenu, CTSTRING(GRANT_SLOTS_MENU));
				int i = ctrlUsers.findItem(sNick);
				if ( i >= 0 ) {
					UserInfo* ui = (UserInfo*)ctrlUsers.getItemData(i);
					if (client->isOp() || !ui->getIdentity().isOp()) {
						pMenu->AppendMenu(MF_SEPARATOR);
						if(!ignoreList.count(sNick)) {
							pMenu->AppendMenu(MF_STRING, IDC_IGNORE, CTSTRING(IGNORE_USER));
						} else {    
							pMenu->AppendMenu(MF_STRING, IDC_UNIGNORE, CTSTRING(UNIGNORE_USER));
						}
					}
				}
				pMenu->AppendMenu(MF_SEPARATOR);
				pMenu->AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
				pMenu->AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CTSTRING(MATCH_QUEUE));
				pMenu->AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
			}
		} else {
			// Muze byt vice nicku
			if(bIsChat == false) {
				// Pocet oznacenych
				int iCount = ctrlUsers.GetSelectedCount();
				TCHAR sTmp[256];
				_stprintf(sTmp, _T("%i %s"), iCount, CTSTRING(HUB_USERS));
				pMenu->InsertSeparator(0, TRUE, Text::fromT(sTmp));
			}
			if (ctrlUsers.GetSelectedCount() <= 25) {
				pMenu->AppendMenu(MF_STRING, IDC_PUBLIC_MESSAGE, CTSTRING(SEND_PUBLIC_MESSAGE));
				pMenu->AppendMenu(MF_SEPARATOR);
			}
			pMenu->AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
			pMenu->AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CTSTRING(MATCH_QUEUE));
			pMenu->AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
		}
		if(bIsChat) {
			switch(SETTING(CHAT_DBLCLICK)) {
		          case 0:
						pMenu->SetMenuDefaultItem(IDC_SELECT_USER);
						break;
		          case 1:
						pMenu->SetMenuDefaultItem(IDC_PUBLIC_MESSAGE);
						break;
		          case 2:
						pMenu->SetMenuDefaultItem(IDC_PRIVATEMESSAGE);
						break;
		          case 3:
						pMenu->SetMenuDefaultItem(IDC_GETLIST);
						break;
		          case 4:
						pMenu->SetMenuDefaultItem(IDC_MATCH_QUEUE);
						break;
		          case 6:
						pMenu->SetMenuDefaultItem(IDC_ADD_TO_FAVORITES);
						break;
			}    
		} else {    
			switch(SETTING(USERLIST_DBLCLICK)) {
		          case 0:
						pMenu->SetMenuDefaultItem( IDC_GETLIST );
						break;
		          case 1:
						pMenu->SetMenuDefaultItem(IDC_PUBLIC_MESSAGE);
						break;
		          case 2:
						pMenu->SetMenuDefaultItem(IDC_PRIVATEMESSAGE);
						break;
		          case 3:
						pMenu->SetMenuDefaultItem(IDC_MATCH_QUEUE);
						break;
		          case 5:
						pMenu->SetMenuDefaultItem(IDC_ADD_TO_FAVORITES);
						break;
			}
		}   		
	}
	return true;
}

LRESULT HubFrame::onCopyActualLine(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if((GetFocus() == ctrlClient.m_hWnd) && (ChatCtrl::sSelectedLine != _T(""))) {
		WinUtil::setClipboard(ChatCtrl::sSelectedLine);
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
			sUsers += Text::toT(((UserInfo*)ctrlUsers.getItemData(i))->getIdentity().getNick()).c_str();
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
	if(ChatCtrl::sSelectedIP != _T("")) {
		tstring s = _T("!banip ") + ChatCtrl::sSelectedIP;

		client->hubMessage(Text::fromT(s));
	}
	return 0;
}

LRESULT HubFrame::onUnBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ChatCtrl::sSelectedIP != _T("")) {
		tstring s = _T("!unban ") + ChatCtrl::sSelectedIP;

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
	StringMap params;
	UserInfo* ui = NULL;
	if(sSelectedUser != _T("")) {
		int k = ctrlUsers.findItem(sSelectedUser);
		if(k != -1) ui = ctrlUsers.getItemData(k);
	} else {
	int i = -1;
		if((i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
			ui = (UserInfo*)ctrlUsers.getItemData(i);
		}
	}
	if(ui == NULL) return 0;
	params["userNI"] = ui->getIdentity().getNick();
	params["hubNI"] = client->getHubName();
	params["myNI"] = client->getMyNick();
	params["userCID"] = ui->user->getCID().toBase32();
	params["hubURL"] = client->getHubUrl();
	tstring file = Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_PRIVATE_CHAT), params)));
	if(Util::fileExists(Text::fromT(file))) {
		ShellExecute(NULL, NULL, file.c_str(), NULL, NULL, SW_SHOWNORMAL);
	} else {
		MessageBox(CTSTRING(NO_LOG_FOR_USER),CTSTRING(NO_LOG_FOR_USER), MB_OK );	  
	}
	return 0;
}

LRESULT HubFrame::onOpenHubLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	StringMap params;
	params["hubNI"] = client->getHubName();
	params["hubURL"] = client->getHubUrl();
	params["myNI"] = client->getMyNick(); 
	tstring filename = Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_MAIN_CHAT), params)));
	if(Util::fileExists(Text::fromT(filename))){
		ShellExecute(NULL, NULL, filename.c_str(), NULL, NULL, SW_SHOWNORMAL);
	} else {
		MessageBox(CTSTRING(NO_LOG_FOR_HUB),CTSTRING(NO_LOG_FOR_HUB), MB_OK );	  
	}
	return 0;
}

LRESULT HubFrame::onWhoisIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ChatCtrl::sSelectedIP != _T("")) {
 		WinUtil::openLink(_T("http://www.ripe.net/perl/whois?form_type=simple&full_query_string=&searchtext=") + ChatCtrl::sSelectedIP);
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
	ctrlUsers.SetRedraw(FALSE);
	
	{
		Lock l(updateCS);

		ctrlUsers.DeleteAllItems();

		for(UserMapIter i = userMap.begin(); i != userMap.end(); ++i) {
			filterUser(i->second);
		}

		updateUsers = true;
	}
	ctrlUsers.SetRedraw(TRUE);
}

void HubFrame::filterUser(UserInfo* ui) {
	if(!ui->getIdentity().isHidden()) {
		bool adduser = false;
		if(filter.empty()) {
			adduser = true;
		} else {
			PME reg(Text::fromT(filter),"i");
			if(!reg.IsValid()) { 
				adduser = true;
			} else if(reg.match(Text::fromT(ui->getText(ctrlFilterSel.GetCurSel())))) {
					adduser = true;
			}
		}
		if(adduser == true){
			ctrlUsers.insertItem(ui, WinUtil::getImage(ui->getIdentity()));
		}
	}
}

void HubFrame::on(SettingsManagerListener::Save, SimpleXML* /*xml*/) throw() {
	ctrlUsers.SetImageList(WinUtil::userImages, LVSIL_SMALL);
	ctrlUsers.Invalidate();
	if(ctrlUsers.GetBkColor() != WinUtil::bgColor) {
		ctrlClient.SetBackgroundColor(WinUtil::bgColor);
		ctrlUsers.SetBkColor(WinUtil::bgColor);
		ctrlUsers.SetTextBkColor(WinUtil::bgColor);
		ctrlUsers.setFlickerFree(WinUtil::bgBrush);
	}
	if(ctrlUsers.GetTextColor() != WinUtil::textColor) {
		ctrlUsers.SetTextColor(WinUtil::textColor);
	}
	RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

LRESULT HubFrame::onSizeMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	bHandled = FALSE;
	ctrlClient.GoToEnd();
	return 0;
}

LRESULT HubFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT: {
			UserInfo* ui = (UserInfo*)cd->nmcd.lItemlParam;
			if (FavoriteManager::getInstance()->isFavoriteUser(ui->user)) {
				cd->clrText = SETTING(FAVORITE_COLOR);
			} else if (UploadManager::getInstance()->hasReservedSlot(ui->user)) {
				cd->clrText = SETTING(RESERVED_SLOT_COLOR);
			} else if (ignoreList.count(Text::toT(ui->user->getFirstNick()))) {
				cd->clrText = SETTING(IGNORED_COLOR);
			} else if(ui->user->isSet(User::FIREBALL)) {
				cd->clrText = SETTING(FIREBALL_COLOR);
			} else if(ui->user->isSet(User::SERVER)) {
				cd->clrText = SETTING(SERVER_COLOR);
			} else if(ui->getIdentity().isOp()) {
				cd->clrText = SETTING(OP_COLOR);
			} else if(ui->getIdentity().isTcpActive()) {
				cd->clrText = SETTING(ACTIVE_COLOR);
			} else if(!ui->getIdentity().isTcpActive()) {
				cd->clrText = SETTING(PASIVE_COLOR);
			} else {
				cd->clrText = SETTING(NORMAL_COLOUR);
			}

			if (client->isOp()) {				
				if (!ui->getIdentity().getBadClient().empty()) {
					cd->clrText = SETTING(BAD_CLIENT_COLOUR);
				} else if(!ui->getIdentity().getBadFilelist().empty()) {
					cd->clrText = SETTING(BAD_FILELIST_COLOUR);
				} else if(BOOLSETTING(SHOW_SHARE_CHECKED_USERS)) {
					bool cClient = !ui->getIdentity().getTestSURComplete().empty();
					bool cFilelist = !ui->getIdentity().getFilelistComplete().empty();
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

	default:
		return CDRF_DODEFAULT;
	}
}

LRESULT HubFrame::onEmoPackChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TCHAR buf[256];
	emoMenu.GetMenuString(wID, buf, 256, MF_BYCOMMAND);
	if (buf!=Text::toT(SETTING(EMOTICONS_FILE))) {
		SettingsManager::getInstance()->set(SettingsManager::EMOTICONS_FILE, Text::fromT(buf));
		delete g_pEmotionsSetup;
		g_pEmotionsSetup = NULL;
		g_pEmotionsSetup = new CAGEmotionSetup;
		if ((g_pEmotionsSetup == NULL)||
			(!g_pEmotionsSetup->Create())){
			dcassert(FALSE);
			return -1;
		}
	}
	return 0;
}

/**
 * @file
 * $Id$
 */