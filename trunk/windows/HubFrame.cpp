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

#include "HubFrame.h"
#include "LineDlg.h"
#include "SearchFrm.h"
#include "PrivateFrame.h"
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
HubFrame::IgnoreMap HubFrame::ignoreList;

int HubFrame::columnSizes[] = { 100, 75, 75, 75, 100, 75, 100, 100, 50, 40, 40, 40 };
int HubFrame::columnIndexes[] = { OnlineUser::COLUMN_NICK, OnlineUser::COLUMN_SHARED, OnlineUser::COLUMN_EXACT_SHARED,
	OnlineUser::COLUMN_DESCRIPTION, OnlineUser::COLUMN_TAG,	OnlineUser::COLUMN_CONNECTION, OnlineUser::COLUMN_IP, OnlineUser::COLUMN_EMAIL,
	OnlineUser::COLUMN_VERSION, OnlineUser::COLUMN_MODE, OnlineUser::COLUMN_HUBS, OnlineUser::COLUMN_SLOTS };
static ResourceManager::Strings columnNames[] = { ResourceManager::NICK, ResourceManager::SHARED, ResourceManager::EXACT_SHARED, 
	ResourceManager::DESCRIPTION, ResourceManager::TAG, ResourceManager::CONNECTION, ResourceManager::IP_BARE, ResourceManager::EMAIL,
	ResourceManager::VERSION, ResourceManager::MODE, ResourceManager::HUBS, ResourceManager::SLOTS };

tstring sSelectedURL = Util::emptyStringT;
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
	ctrlClient.SetEventMask(ctrlClient.GetEventMask() | ENM_LINK);
	ctrlClient.setClient(client);
	
	ctrlMessage.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL |
		ES_NOHIDESEL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE, WS_EX_CLIENTEDGE);
	
	ctrlMessageContainer.SubclassWindow(ctrlMessage.m_hWnd);
	ctrlMessage.SetFont(WinUtil::font);
	ctrlMessage.SetLimitText(9999);

	hEmoticonBmp = (HBITMAP) ::LoadImage(_Module.get_m_hInst(), MAKEINTRESOURCE(IDB_EMOTICON), IMAGE_BITMAP, 0, 0, LR_SHARED);
	ctrlEmoticons.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_FLAT | BS_BITMAP | BS_CENTER, 0, IDC_EMOT);
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
	ctrlUsers.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | 0x00010000 | LVS_EX_INFOTIP);

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

	const FavoriteHubEntry *fhe = FavoriteManager::getInstance()->getFavoriteHubEntry(Text::fromT(server));
	if(fhe) {
		WinUtil::splitTokens(columnIndexes, fhe->getHeaderOrder(), OnlineUser::COLUMN_LAST);
		WinUtil::splitTokens(columnSizes, fhe->getHeaderWidths(), OnlineUser::COLUMN_LAST);
	} else {
		WinUtil::splitTokens(columnIndexes, SETTING(HUBFRAME_ORDER), OnlineUser::COLUMN_LAST);
		WinUtil::splitTokens(columnSizes, SETTING(HUBFRAME_WIDTHS), OnlineUser::COLUMN_LAST);                           
	}
    	
	for(uint8_t j=0; j<OnlineUser::COLUMN_LAST; j++) {
		int fmt = (j == OnlineUser::COLUMN_SHARED || j == OnlineUser::COLUMN_EXACT_SHARED || j == OnlineUser::COLUMN_SLOTS) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlUsers.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlUsers.setColumnOrderArray(OnlineUser::COLUMN_LAST, columnIndexes);

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
	
	ctrlUsers.setSortColumn(OnlineUser::COLUMN_NICK);
				
	ctrlUsers.SetImageList(WinUtil::userImages, LVSIL_SMALL);

	CToolInfo ti(TTF_SUBCLASS, ctrlStatus.m_hWnd);
	
	ctrlLastLines.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON, WS_EX_TOPMOST);
	ctrlLastLines.SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	ctrlLastLines.AddTool(&ti);
	ctrlLastLines.SetDelayTime(TTDT_AUTOPOP, 15000);

	showJoins = BOOLSETTING(SHOW_JOINS);
	favShowJoins = BOOLSETTING(FAV_SHOW_JOINS);

	for(int j=0; j<OnlineUser::COLUMN_LAST; j++) {
		ctrlFilterSel.AddString(CTSTRING_I(columnNames[j]));
	}
	ctrlFilterSel.AddString(CTSTRING(ANY));
	ctrlFilterSel.SetCurSel(0);

	bHandled = FALSE;
	client->connect();

	FavoriteManager::getInstance()->addListener(this);
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
		, int chatusersplit, bool userliststate, string sColumsOrder, string sColumsWidth, string sColumsVisible) {
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

//		int nCmdShow = SW_SHOWDEFAULT;
		frm->CreateEx(WinUtil::mdiClient, frm->rcDefault);
//		if(windowtype)
//			frm->ShowWindow(((nCmdShow == SW_SHOWDEFAULT) || (nCmdShow == SW_SHOWNORMAL)) ? windowtype : nCmdShow);
	} else {
		if(::IsIconic(i->second->m_hWnd))
			::ShowWindow(i->second->m_hWnd, SW_RESTORE);
		i->second->MDIActivate(i->second->m_hWnd);
	}
}

LRESULT HubFrame::OnForwardMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	LPMSG pMsg = (LPMSG)lParam;
	if((pMsg->message >= WM_MOUSEFIRST) && (pMsg->message <= WM_MOUSELAST))
		ctrlLastLines.RelayEvent(pMsg);
	return 0;
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
			tstring cmd = s;
			tstring param;
			tstring message;
			tstring status;
			if(WinUtil::checkCommand(cmd, param, message, status)) {
				if(!message.empty()) {
					client->hubMessage(Text::fromT(message));
				}
				if(!status.empty()) {
					addClientLine(status, WinUtil::m_ChatTextSystem);
				}
			} else if(Util::stricmp(cmd.c_str(), _T("join"))==0) {
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
			} else if((Util::stricmp(cmd.c_str(), _T("clear")) == 0) || (Util::stricmp(cmd.c_str(), _T("cls")) == 0)) {
				ctrlClient.SetWindowText(_T(""));
			} else if(Util::stricmp(cmd.c_str(), _T("ts")) == 0) {
				timeStamps = !timeStamps;
				if(timeStamps) {
					addClientLine(TSTRING(TIMESTAMPS_ENABLED), WinUtil::m_ChatTextSystem);
				} else {
					addClientLine(TSTRING(TIMESTAMPS_DISABLED), WinUtil::m_ChatTextSystem);
				}
			} else if( (Util::stricmp(cmd.c_str(), _T("password")) == 0) && waitingForPW ) {
				client->setPassword(Text::fromT(param));
				client->password(Text::fromT(param));
				waitingForPW = false;
			} else if( Util::stricmp(cmd.c_str(), _T("showjoins")) == 0 ) {
				showJoins = !showJoins;
				if(showJoins) {
					addClientLine(TSTRING(JOIN_SHOWING_ON), WinUtil::m_ChatTextSystem);
				} else {
					addClientLine(TSTRING(JOIN_SHOWING_OFF), WinUtil::m_ChatTextSystem);
				}
			} else if( Util::stricmp(cmd.c_str(), _T("favshowjoins")) == 0 ) {
				favShowJoins = !favShowJoins;
				if(favShowJoins) {
					addClientLine(TSTRING(FAV_JOIN_SHOWING_ON), WinUtil::m_ChatTextSystem);
				} else {
					addClientLine(TSTRING(FAV_JOIN_SHOWING_OFF), WinUtil::m_ChatTextSystem);
				}
			} else if(Util::stricmp(cmd.c_str(), _T("close")) == 0) {
				PostMessage(WM_CLOSE);
			} else if(Util::stricmp(cmd.c_str(), _T("userlist")) == 0) {
				ctrlShowUsers.SetCheck(showUsers ? BST_UNCHECKED : BST_CHECKED);
			} else if(Util::stricmp(cmd.c_str(), _T("connection")) == 0) {
				addClientLine(Text::toT((STRING(IP) + client->getLocalIp() + ", " + 
					STRING(PORT) + 
					Util::toString(ConnectionManager::getInstance()->getPort()) + "/" + 
					Util::toString(SearchManager::getInstance()->getPort()) + "/" +
					Util::toString(ConnectionManager::getInstance()->getSecurePort())))
					, WinUtil::m_ChatTextSystem);
			} else if((Util::stricmp(cmd.c_str(), _T("favorite")) == 0) || (Util::stricmp(cmd.c_str(), _T("fav")) == 0)) {
				addAsFavorite();
			} else if((Util::stricmp(cmd.c_str(), _T("removefavorite")) == 0) || (Util::stricmp(cmd.c_str(), _T("removefav")) == 0)) {
				removeFavoriteHub();
			} else if(Util::stricmp(cmd.c_str(), _T("getlist")) == 0){
				if( !param.empty() ){
					OnlineUser* ui = client->findUser(Text::fromT(param));
					if(ui) {
						ui->getList();
					}
				}
			} else if(Util::stricmp(cmd.c_str(), _T("log")) == 0) {
				StringMap params;
				params["hubNI"] = client->getHubName();
				params["hubURL"] = client->getHubUrl();
				params["myNI"] = client->getMyNick(); 
				if(param.empty()) {
					WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_MAIN_CHAT), params, false))));
				} else if(Util::stricmp(param.c_str(), _T("status")) == 0) {
					WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_STATUS), params, false))));
				}
			} else if(Util::stricmp(cmd.c_str(), _T("f")) == 0) {
				if(param.empty())
					param = findTextPopup();
				findText(param);
			} else if(Util::stricmp(cmd.c_str(), _T("extraslots"))==0) {
				int j = Util::toInt(Text::fromT(param));
				if(j > 0) {
					SettingsManager::getInstance()->set(SettingsManager::EXTRA_SLOTS, j);
					addClientLine(TSTRING(EXTRA_SLOTS_SET), WinUtil::m_ChatTextSystem );
				} else {
					addClientLine(TSTRING(INVALID_NUMBER_OF_SLOTS), WinUtil::m_ChatTextSystem );
				}
			} else if(Util::stricmp(cmd.c_str(), _T("smallfilesize"))==0) {
				int j = Util::toInt(Text::fromT(param));
				if(j >= 64) {
					SettingsManager::getInstance()->set(SettingsManager::SET_MINISLOT_SIZE, j);
					addClientLine(TSTRING(SMALL_FILE_SIZE_SET), WinUtil::m_ChatTextSystem );
				} else {
					addClientLine(TSTRING(INVALID_SIZE), WinUtil::m_ChatTextSystem );
				}
			} else if(Util::stricmp(cmd.c_str(), _T("savequeue")) == 0) {
				QueueManager::getInstance()->saveQueue();
				addClientLine(_T("Queue saved."), WinUtil::m_ChatTextSystem );
			} else if(Util::stricmp(cmd.c_str(), _T("whois")) == 0) {
				WinUtil::openLink(_T("http://www.ripe.net/perl/whois?form_type=simple&full_query_string=&searchtext=") + Text::toT(Util::encodeURI(Text::fromT(param))));
			} else if(Util::stricmp(cmd.c_str(), _T("ignorelist"))==0) {
				tstring ignorelist = _T("Ignored users:");
				for(IgnoreMap::const_iterator i = ignoreList.begin(); i != ignoreList.end(); ++i)
					ignorelist += _T(" ") + Text::toT((*i)->getFirstNick());
				addLine(ignorelist, WinUtil::m_ChatTextSystem);
			} else if(Util::stricmp(cmd.c_str(), _T("log")) == 0) {
				StringMap params;
				params["hubNI"] = client->getHubName();
				params["hubURL"] = client->getHubUrl();
				params["myNI"] = client->getMyNick(); 
				if(param.empty()) {
					WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_MAIN_CHAT), params, false))));
				} else if(Util::stricmp(param.c_str(), _T("status")) == 0) {
					WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_STATUS), params, false))));
				}
			} else if(Util::stricmp(cmd.c_str(), _T("help")) == 0) {
				addLine(_T("*** ") + WinUtil::commands + _T(", /smallfilesize #, /extraslots #, /savequeue, /join <hub-ip>, /clear, /ts, /showjoins, /favshowjoins, /close, /userlist, /connection, /favorite, /pm <user> [message], /getlist <user>, /winamp, /whois [IP], /ignorelist, /removefavorite"), WinUtil::m_ChatTextSystem);
			} else if(Util::stricmp(cmd.c_str(), _T("pm")) == 0) {
				string::size_type j = param.find(_T(' '));
				if(j != string::npos) {
					tstring nick = param.substr(0, j);
					const OnlineUser* ui = client->findUser(Text::fromT(nick));

					if(ui) {
						if(param.size() > j + 1)
							PrivateFrame::openWindow(ui->getUser(), param.substr(j+1));
						else
							PrivateFrame::openWindow(ui->getUser());
					}
				} else if(!param.empty()) {
					const OnlineUser* ui = client->findUser(Text::fromT(param));
					if(ui) {
						PrivateFrame::openWindow(ui->getUser());
					}
				}
			} else if(Util::stricmp(cmd.c_str(), _T("me")) == 0) {
				client->hubMessage(Text::fromT(s));
			} else if(Util::stricmp(cmd.c_str(), _T("switch")) == 0) {
				m_bVertical = !m_bVertical;
				if(m_bVertical) {
					SetSplitterPanes(ctrlClient.m_hWnd, ctrlUsers.m_hWnd, false);
				} else {
					SetSplitterPanes(ctrlUsers.m_hWnd, ctrlClient.m_hWnd, false);
				}
				UpdateLayout();
			} else if(Util::stricmp(cmd.c_str(), _T("stats")) == 0) {
			    if(client->isOp())
					client->hubMessage(WinUtil::generateStats());
			    else
					addLine(Text::toT(WinUtil::generateStats()));
			} else {
				if (BOOLSETTING(SEND_UNKNOWN_COMMANDS)) {
					client->hubMessage(Text::fromT(s));
				} else {
					addClientLine(TSTRING(UNKNOWN_COMMAND) + cmd);
				}
			}
			ctrlMessage.SetWindowText(_T(""));
		} else if(waitingForPW) {
			addClientLine(TSTRING(DONT_REMOVE_SLASH_PASSWORD));
			ctrlMessage.SetWindowText(_T("/password "));
			ctrlMessage.SetFocus();
			ctrlMessage.SetSel(10, 10);
		} else {
			if(BOOLSETTING(CZCHARS_DISABLE))
				s = WinUtil::disableCzChars(s);

			client->hubMessage(Text::fromT(s));
			ctrlMessage.SetWindowText(_T(""));
		}
	} else {
		MessageBeep(MB_ICONEXCLAMATION);
	}
}

struct CompareItems {
	CompareItems(uint8_t aCol) : col(aCol) { }
	bool operator()(const OnlineUser& a, const OnlineUser& b) const {
		return OnlineUser::compareItems(&a, &b, col) < 0;
	}
	const uint8_t col;
};

void HubFrame::addAsFavorite() {
	const FavoriteHubEntry* existingHub = FavoriteManager::getInstance()->getFavoriteHubEntry(client->getHubUrl());
	if(!existingHub) {
		FavoriteHubEntry aEntry;
		TCHAR buf[256];
		this->GetWindowText(buf, 255);
		aEntry.setServer(Text::fromT(server));
		aEntry.setName(Text::fromT(buf));
		aEntry.setDescription(Text::fromT(buf));
		aEntry.setConnect(false);
		if(!client->getPassword().empty()) {
			aEntry.setNick(client->getMyNick());
			aEntry.setPassword(client->getPassword());
		}
		aEntry.setConnect(false);
		FavoriteManager::getInstance()->addFavorite(aEntry);
		addClientLine(TSTRING(FAVORITE_HUB_ADDED), WinUtil::m_ChatTextSystem );
	} else {
		addClientLine(TSTRING(FAVORITE_HUB_ALREADY_EXISTS), WinUtil::m_ChatTextSystem);
	}
}

void HubFrame::removeFavoriteHub() {
	const FavoriteHubEntry* removeHub = FavoriteManager::getInstance()->getFavoriteHubEntry(client->getHubUrl());
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
	tstring sCopy;
	if(!ChatCtrl::sSelectedUser.empty()) {
		const OnlineUser* ui = client->findUser(Text::fromT(ChatCtrl::sSelectedUser));
		if(ui) {
			switch (wID) {
				case IDC_COPY_NICK:
					sCopy += Text::toT(ui->getNick());
					break;
				case IDC_COPY_EXACT_SHARE:
					sCopy += Util::formatExactSize(ui->getIdentity().getBytesShared());
					break;
				case IDC_COPY_DESCRIPTION:
					sCopy += Text::toT(ui->getIdentity().getDescription());
					break;
				case IDC_COPY_TAG:
					sCopy += Text::toT(ui->getIdentity().getTag());
					break;
				case IDC_COPY_EMAIL_ADDRESS:
					sCopy += Text::toT(ui->getIdentity().getEmail());
					break;
				case IDC_COPY_IP:
					sCopy += Text::toT(ui->getIdentity().getIp());
					break;
				case IDC_COPY_NICK_IP:
					sCopy += _T("Info User:\r\n")
						_T("\tNick: ") + Text::toT(ui->getIdentity().getNick()) + _T("\r\n") + 
						_T("\tIP: ") +  Text::toT(ui->getIdentity().getIp()) + _T("\r\n"); 
					break;
				case IDC_COPY_ALL:
					sCopy += _T("Info User:\r\n")
						_T("\tNick: ") + Text::toT(ui->getIdentity().getNick()) + _T("\r\n") + 
						_T("\tShare: ") + Util::formatBytesW(ui->getIdentity().getBytesShared()) + _T("\r\n") + 
						_T("\tDescription: ") + Text::toT(ui->getIdentity().getDescription()) + _T("\r\n") +
						_T("\tTag: ") + Text::toT(ui->getIdentity().getTag()) + _T("\r\n") +
						_T("\tConnection: ") + Text::toT(ui->getIdentity().getConnection()) + _T("\r\n") + 
						_T("\tE-Mail: ") + Text::toT(ui->getIdentity().getEmail()) + _T("\r\n") +
						_T("\tClient: ") + Text::toT(ui->getIdentity().get("CT")) + _T("\r\n") + 
						_T("\tVersion: ") + Text::toT(ui->getIdentity().get("VE")) + _T("\r\n") +
						_T("\tMode: ") + ui->getText(OnlineUser::COLUMN_MODE) + _T("\r\n") +
						_T("\tHubs: ") + ui->getText(OnlineUser::COLUMN_HUBS) + _T("\r\n") +
						_T("\tSlots: ") + ui->getText(OnlineUser::COLUMN_SLOTS) + _T("\r\n") +
						_T("\tIP: ") + Text::toT(ui->getIdentity().getIp()) + _T("\r\n") +
						_T("\tLock: " )+ Text::toT(ui->getIdentity().get("LO")) + _T("\r\n")+
						_T("\tSupports: ") + Text::toT(ui->getIdentity().get("SU"));
					break;		
				default:
					dcdebug("HUBFRAME DON'T GO HERE\n");
					return 0;
			}
		}
	}
	if (!sCopy.empty())
		WinUtil::setClipboard(sCopy);

	return 0;
}

LRESULT HubFrame::onDoubleClickUsers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;
	if(item->iItem != -1) {
	    switch(SETTING(USERLIST_DBLCLICK)) {
		    case 0:
				ctrlUsers.getItemData(item->iItem)->getList();
		        break;
		    case 1: {
				tstring sUser = Text::toT(ctrlUsers.getItemData(item->iItem)->getNick());
	            int iSelBegin, iSelEnd;
	            ctrlMessage.GetSel(iSelBegin, iSelEnd);

	            if((iSelBegin == 0) && (iSelEnd == 0)) {
					sUser += _T(": ");
					if(ctrlMessage.GetWindowTextLength() == 0) {
			            ctrlMessage.SetWindowText(sUser.c_str());
			            ctrlMessage.SetFocus();
			            ctrlMessage.SetSel(ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength());
                    } else {
			            ctrlMessage.ReplaceSel(sUser.c_str());
			            ctrlMessage.SetFocus();
                    }
				} else {
					sUser += _T(" ");
                    ctrlMessage.ReplaceSel(sUser.c_str());
                    ctrlMessage.SetFocus();
	            }
				break;
		    }    
		    case 2:
				if(ctrlUsers.getItemData(item->iItem)->getUser() != ClientManager::getInstance()->getMe())
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
		ctrlClient.SetWindowText(Util::emptyStringT.c_str());
	}
	return 0;
}

bool HubFrame::updateUser(const UserTask& u) {
	if(!showUsers) return false;
	
	if(!u.onlineUser->isInList) {
		u.onlineUser->update(-1);

		if(!u.onlineUser->isHidden()) {
			u.onlineUser->inc();
			ctrlUsers.insertItem(u.onlineUser, UserInfoBase::getImage(u.onlineUser->getIdentity()));
		}

		if(!filter.empty())
			updateUserList(u.onlineUser);
		return true;
	} else {
		int pos = ctrlUsers.findItem(u.onlineUser);

		if(pos != -1) {
			TCHAR buf[255];
			ListView_GetItemText(ctrlUsers, pos, ctrlUsers.getSortColumn(), buf, 255);
			
			resort = u.onlineUser->update(ctrlUsers.getSortColumn(), buf) || resort;
			if(u.onlineUser->isHidden()) {
				ctrlUsers.DeleteItem(pos);
				u.onlineUser->dec();				
			} else {
				ctrlUsers.updateItem(pos);
				ctrlUsers.SetItem(pos, 0, LVIF_IMAGE, NULL, UserInfoBase::getImage(u.onlineUser->getIdentity()), 0, 0, NULL);
			}
		}

		u.onlineUser->getIdentity().set("WO", u.onlineUser->getIdentity().isOp() ? "1" : Util::emptyString);
		updateUserList(u.onlineUser);
		return false;
	}
}

void HubFrame::removeUser(OnlineUser* aUser) {
	if(!showUsers) return;
	
	if(!aUser->isHidden()) {
		int i = ctrlUsers.findItem(aUser);
		if(i != -1) {
			ctrlUsers.DeleteItem(i);
			aUser->dec();
		}
	}
}

LRESULT HubFrame::onSpeaker(UINT /*uMsg*/, WPARAM /* wParam */, LPARAM /* lParam */, BOOL& /*bHandled*/) {
	TaskQueue::List t;
	tasks.get(t);

	if(t.size() > 2) {
		ctrlUsers.SetRedraw(FALSE);
	}

	for(TaskQueue::Iter i = t.begin(); i != t.end(); ++i) {
		if(i->first == UPDATE_USER) {
			updateUser(*static_cast<UserTask*>(i->second));
		} else if(i->first == UPDATE_USER_JOIN) {
			UserTask& u = *static_cast<UserTask*>(i->second);
			if(updateUser(u)) {
				bool isFavorite = FavoriteManager::getInstance()->isFavoriteUser(u.onlineUser->getUser());
				if (isFavorite && (!SETTING(SOUND_FAVUSER).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
					PlaySound(Text::toT(SETTING(SOUND_FAVUSER)).c_str(), NULL, SND_FILENAME | SND_ASYNC);

				if(isFavorite && BOOLSETTING(POPUP_FAVORITE_CONNECTED)) {
					MainFrame::getMainFrame()->ShowBalloonTip(Text::toT(u.onlineUser->getIdentity().getNick() + " - " + client->getHubName()).c_str(), CTSTRING(FAVUSER_ONLINE));
				}

				if (showJoins || (favShowJoins && isFavorite)) {
				 	addLine(_T("*** ") + TSTRING(JOINS) + Text::toT(u.onlineUser->getIdentity().getNick()), WinUtil::m_ChatTextSystem);
				}	

				if(client->isOp() && !u.onlineUser->getIdentity().isBot() && !u.onlineUser->getIdentity().isHub()) {
					int64_t bytesSharedInt64 = u.onlineUser->getIdentity().getBytesShared();
					if(bytesSharedInt64 > 0) {
						string bytesShared = Util::toString(bytesSharedInt64);
						bool samenumbers = false;
						const char* sSameNumbers[] = { "000000", "111111", "222222", "333333", "444444", "555555", "666666", "777777", "888888", "999999" };
						for(int i = 0; i < 10; ++i) {
							if(strstr(bytesShared.c_str(), sSameNumbers[i]) != 0) {
								samenumbers = true;
								break;
							}
						}
						if(samenumbers) {
							tstring detectString = Util::formatExactSize(u.onlineUser->getIdentity().getBytesShared()) + _T(" - the share size had too many same numbers in it");
							u.onlineUser->getIdentity().set("BF", "1");
							u.onlineUser->getIdentity().setCheat(u.onlineUser->getClient(), Text::fromT(detectString), false);
							
							TCHAR buf[512];
							snwprintf(buf, sizeof(buf), _T("*** %s %s - %s"), TSTRING(USER).c_str(), Text::toT(u.onlineUser->getIdentity().getNick()).c_str(), detectString.c_str());

							if(BOOLSETTING(POPUP_CHEATING_USER)) {
								MainFrame::getMainFrame()->ShowBalloonTip(buf, CTSTRING(CHEATING_USER));
							}

							if(BOOLSETTING(DISPLAY_CHEATS_IN_MAIN_CHAT)) {
								CHARFORMAT2 cf;
								memzero(&cf, sizeof(CHARFORMAT2));
								cf.cbSize = sizeof(cf);
								cf.dwMask = CFM_BACKCOLOR | CFM_COLOR | CFM_BOLD;
								cf.crBackColor = SETTING(BACKGROUND_COLOR);
								cf.crTextColor = SETTING(ERROR_COLOR);							
								
								addLine((tstring)buf, cf);
							}

							ClientManager::getInstance()->sendRawCommand(u.onlineUser->getUser(), *client, SETTING(FAKESHARE_RAW));
						}
					}
						
					if(BOOLSETTING(CHECK_NEW_USERS)) {
						if(u.onlineUser->getIdentity().isTcpActive() || client->isActive()) {
							try {
								QueueManager::getInstance()->addTestSUR(u.onlineUser->getUser(), true);
							} catch(const Exception&) {
							}
						}
					}
				}
			}
		} else if(i->first == REMOVE_USER) {
			const UserTask& u = *static_cast<UserTask*>(i->second);
			removeUser(u.onlineUser);

			if (showJoins || (favShowJoins && FavoriteManager::getInstance()->isFavoriteUser(u.onlineUser->getUser()))) {
				addLine(Text::toT("*** " + STRING(PARTS) + u.onlineUser->getIdentity().getNick()), WinUtil::m_ChatTextSystem);
			}
		} else if(i->first == CONNECTED) {
			addClientLine(TSTRING(CONNECTED), WinUtil::m_ChatTextServer);
			setTabColor(RGB(0, 255, 0));
			unsetIconState();

			if(BOOLSETTING(POPUP_HUB_CONNECTED)) {
				MainFrame::getMainFrame()->ShowBalloonTip(Text::toT(client->getAddress()).c_str(), CTSTRING(CONNECTED));
			}

			if ((!SETTING(SOUND_HUBCON).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
				PlaySound(Text::toT(SETTING(SOUND_HUBCON)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
		} else if(i->first == DISCONNECTED) {
			clearUserList();
			setTabColor(RGB(255, 0, 0));
			setIconState();
			if ((!SETTING(SOUND_HUBDISCON).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
				PlaySound(Text::toT(SETTING(SOUND_HUBDISCON)).c_str(), NULL, SND_FILENAME | SND_ASYNC);

			if(BOOLSETTING(POPUP_HUB_DISCONNECTED)) {
				MainFrame::getMainFrame()->ShowBalloonTip(Text::toT(client->getAddress()).c_str(), CTSTRING(DISCONNECTED));
			}
		} else if(i->first == ADD_CHAT_LINE) {
    		const MessageTask& msg = *static_cast<MessageTask*>(i->second);
        	if(!msg.from.getUser() || (ignoreList.find(msg.from.getUser()) == ignoreList.end()) ||
	          (msg.from.isOp() && !client->isOp())) {
				  addLine(msg.from, Text::toT(msg.str), WinUtil::m_ChatTextGeneral);
        	}
		} else if(i->first == ADD_STATUS_LINE) {
			addClientLine(Text::toT(static_cast<StringTask*>(i->second)->str), WinUtil::m_ChatTextServer );
		} else if(i->first == ADD_SILENT_STATUS_LINE) {
			addClientLine(Text::toT(static_cast<StringTask*>(i->second)->str), false);
		} else if(i->first == SET_WINDOW_TITLE) {
			SetWindowText(Text::toT(static_cast<StringTask*>(i->second)->str).c_str());
			SetMDIFrameMenu();
		} else if(i->first == STATS) {
			size_t AllUsers = client->getUserCount();
			size_t ShownUsers = ctrlUsers.GetItemCount();
			if(AllUsers != ShownUsers) {
				ctrlStatus.SetText(1, (Util::toStringW(ShownUsers) + _T("/") + Util::toStringW(AllUsers) + _T(" ") + TSTRING(HUB_USERS)).c_str());
			} else {
				ctrlStatus.SetText(1, (Util::toStringW(AllUsers) + _T(" ") + TSTRING(HUB_USERS)).c_str());
			}
			int64_t available = client->getAvailable();
			ctrlStatus.SetText(2, Util::formatBytesW(available).c_str());
			if(AllUsers > 0)
				ctrlStatus.SetText(3, (Util::formatBytesW(available / AllUsers) + _T("/") + TSTRING(USER)).c_str());
			else
				ctrlStatus.SetText(3, _T(""));
		} else if(i->first == GET_PASSWORD) {
			if(client->getPassword().size() > 0) {
				client->password(client->getPassword());
				addClientLine(TSTRING(STORED_PASSWORD_SENT), WinUtil::m_ChatTextSystem);
			} else {
				if(!BOOLSETTING(PROMPT_PASSWORD)) {
					ctrlMessage.SetWindowText(_T("/password "));
					ctrlMessage.SetFocus();
					ctrlMessage.SetSel(10, 10);
					waitingForPW = true;
				} else {
					LineDlg linePwd;
					linePwd.title = CTSTRING(ENTER_PASSWORD);
					linePwd.description = CTSTRING(ENTER_PASSWORD);
					linePwd.password = true;
					if(linePwd.DoModal(m_hWnd) == IDOK) {
						client->setPassword(Text::fromT(linePwd.line));
						client->password(Text::fromT(linePwd.line));
						waitingForPW = false;
					} else {
						client->disconnect(true);
					}
				}
			}
		} else if(i->first == PRIVATE_MESSAGE) {
			const MessageTask& pm = *static_cast<MessageTask*>(i->second);
			tstring nick = Text::toT(pm.from.getNick());
			if(!pm.from.getUser() || (ignoreList.find(pm.from.getUser()) == ignoreList.end()) ||
			  (pm.from.isOp() && !client->isOp())) {
				bool myPM = pm.replyTo == ClientManager::getInstance()->getMe();
				const UserPtr& user = myPM ? pm.to : pm.replyTo;
				if(pm.hub) {
					if(BOOLSETTING(IGNORE_HUB_PMS)) {
						addClientLine(TSTRING(IGNORED_MESSAGE) + Text::toT(pm.str), false);
					} else if(BOOLSETTING(POPUP_HUB_PMS) || PrivateFrame::isOpen(user)) {
						PrivateFrame::gotMessage(pm.from, pm.to, pm.replyTo, Text::toT(pm.str));
					} else {
						addLine(TSTRING(PRIVATE_MESSAGE_FROM) + nick + _T(": ") + Text::toT(pm.str), WinUtil::m_ChatTextPrivate);
					}
				} else if(pm.bot) {
					if(BOOLSETTING(IGNORE_BOT_PMS)) {
						addClientLine(TSTRING(IGNORED_MESSAGE) + Text::toT(pm.str), WinUtil::m_ChatTextPrivate, false);
					} else if(BOOLSETTING(POPUP_BOT_PMS) || PrivateFrame::isOpen(user)) {
						PrivateFrame::gotMessage(pm.from, pm.to, pm.replyTo, Text::toT(pm.str));
					} else {
						addLine(TSTRING(PRIVATE_MESSAGE_FROM) + nick + _T(": ") + Text::toT(pm.str), WinUtil::m_ChatTextPrivate);
					}
				} else {
					if(BOOLSETTING(POPUP_PMS) || PrivateFrame::isOpen(user)) {
						PrivateFrame::gotMessage(pm.from, pm.to, pm.replyTo, Text::toT(pm.str));
					} else {
						addLine(TSTRING(PRIVATE_MESSAGE_FROM) + nick + _T(": ") + Text::toT(pm.str), WinUtil::m_ChatTextPrivate);
					}
					if(BOOLSETTING(MINIMIZE_TRAY)) {
						HWND hMainWnd = MainFrame::getMainFrame()->m_hWnd;//GetTopLevelWindow();
						::PostMessage(hMainWnd, WM_SPEAKER, MainFrame::SET_PM_TRAY_ICON, NULL);
					}										
				}
			}
		} else if(i->first == KICK_MSG) {
    	    const MessageTask& km = *static_cast<MessageTask*>(i->second);
        	if(SETTING(FILTER_MESSAGES)) {
            	addClientLine(Text::toT(km.str), false);
	        } else {
    			addLine(km.from, Text::toT(km.str), WinUtil::m_ChatTextServer, false);
        	}
		} else if(i->first == CHEATING_USER) {
			CHARFORMAT2 cf;
			memzero(&cf, sizeof(CHARFORMAT2));
			cf.cbSize = sizeof(cf);
			cf.dwMask = CFM_BACKCOLOR | CFM_COLOR | CFM_BOLD;
			cf.crBackColor = SETTING(BACKGROUND_COLOR);
			cf.crTextColor = SETTING(ERROR_COLOR);

			tstring msg = Text::toT(static_cast<StringTask*>(i->second)->str);
			if(BOOLSETTING(POPUP_CHEATING_USER) && msg.length() < 256) {
				MainFrame::getMainFrame()->ShowBalloonTip(msg.c_str(), CTSTRING(CHEATING_USER));
			}

			addLine(Text::toT(static_cast<StringTask*>(i->second)->str), cf);
		}

		delete i->second;
	}
	
	if(resort && showUsers) {
		ctrlUsers.resort();
		resort = false;
	}

	if(t.size() > 2) {
		ctrlUsers.SetRedraw(TRUE);
	}
	
	return 0;
}

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
		
		w[0] = sr.right - tmp - 55;
		w[1] = w[0] + (tmp-30)/2;
		w[2] = w[0] + (tmp-65);
		w[3] = w[2] + 100;
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

		SettingsManager::getInstance()->removeListener(this);
		TimerManager::getInstance()->removeListener(this);
		FavoriteManager::getInstance()->removeListener(this);
		client->removeListener(this);
		client->disconnect(true);

		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		SettingsManager::getInstance()->set(SettingsManager::GET_USER_INFO, showUsers);
		FavoriteManager::getInstance()->removeUserCommand(Text::fromT(server));

		clearUserList();
		clearTaskList();
				
		string tmp, tmp2, tmp3;
		ctrlUsers.saveHeaderOrder(tmp, tmp2, tmp3);

		FavoriteHubEntry *fhe = FavoriteManager::getInstance()->getFavoriteHubEntry(Text::fromT(server));
		if(fhe != NULL) {
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
	for(CtrlUsers::iterator i = ctrlUsers.begin(); i != ctrlUsers.end(); i++) {
		(*i).dec();
	}
	ctrlUsers.DeleteAllItems();
}

void HubFrame::clearTaskList() {
	tasks.clear();
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
	LineDlg *finddlg = new LineDlg;
	finddlg->title = CTSTRING(SEARCH);
	finddlg->description = CTSTRING(SPECIFY_SEARCH_STRING);

	tstring param = Util::emptyStringT;
	if(finddlg->DoModal() == IDOK) {
		param = finddlg->line;
	}
	delete finddlg;
	return param;
}

LRESULT HubFrame::onLButton(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HWND focus = GetFocus();
	bHandled = false;
	if(focus == ctrlClient.m_hWnd) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		int i = ctrlClient.CharFromPos(pt);
		int line = ctrlClient.LineFromChar(i);
		int c = LOWORD(i) - ctrlClient.LineIndex(line);
		int len = ctrlClient.LineLength(i) + 1;
		if(len < 3) {
			return 0;
		}

		TCHAR* buf = new TCHAR[len];
		ctrlClient.GetLine(line, buf, len);
		tstring x = tstring(buf, len-1);
		delete[] buf;

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
			tstring nick = x.substr(start, end - start);
			OnlineUser* ui = client->findUser(Text::fromT(nick));
			if(ui) {
				bHandled = true;
				if (wParam & MK_CONTROL) { // MK_CONTROL = 0x0008
					PrivateFrame::openWindow(ui->getUser());
				} else if (wParam & MK_SHIFT) {
					try {
						QueueManager::getInstance()->addList(ui->getUser(), QueueItem::FLAG_CLIENT_VIEW);
					} catch(const Exception& e) {
						addClientLine(Text::toT(e.getError()), WinUtil::m_ChatTextSystem);
					}
				} else {
				switch(SETTING(CHAT_DBLCLICK)) {
					case 0: {
						int items = ctrlUsers.GetItemCount();
						int pos = -1;
						ctrlUsers.SetRedraw(FALSE);
						for(int i = 0; i < items; ++i) {
							if(ctrlUsers.getItemData(i) == ui)
								pos = i;
							ctrlUsers.SetItemState(i, (i == pos) ? LVIS_SELECTED | LVIS_FOCUSED : 0, LVIS_SELECTED | LVIS_FOCUSED);
						}
						ctrlUsers.SetRedraw(TRUE);
						ctrlUsers.EnsureVisible(pos, FALSE);
					    break;
					}    
					case 1: {
					     tstring sUser = ui->getText(OnlineUser::COLUMN_NICK);
					     int iSelBegin, iSelEnd;
					     ctrlMessage.GetSel(iSelBegin, iSelEnd);

					     if((iSelBegin == 0) && (iSelEnd == 0)) {
							sUser += _T(": ");
							if(ctrlMessage.GetWindowTextLength() == 0) {   
			                    ctrlMessage.SetWindowText(sUser.c_str());
			                    ctrlMessage.SetFocus();
			                    ctrlMessage.SetSel(ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength());
							} else {
			                    ctrlMessage.ReplaceSel(sUser.c_str());
								ctrlMessage.SetFocus();
					        }
					     } else {
					          sUser += _T(" ");
					          ctrlMessage.ReplaceSel(sUser.c_str());
					          ctrlMessage.SetFocus();
					     }
					     break;
					}
					case 2:
						if(ui->getUser() != ClientManager::getInstance()->getMe())
					          ui->pm();
					     break;
					case 3:
					     ui->getList();
					     break;
					case 4:
					     ui->matchQueue();
					     break;
					case 5:
					     ui->grant();
					     break;
					case 6:
					     ui->addFav();
					     break;
				}
			}
		}
	}
	return 0;
}

void HubFrame::addLine(const tstring& aLine) {
	addLine(Identity(NULL, 0), aLine, WinUtil::m_ChatTextGeneral );
}

void HubFrame::addLine(const tstring& aLine, CHARFORMAT2& cf, bool bUseEmo/* = true*/) {
    addLine(Identity(NULL, 0), aLine, cf, bUseEmo);
}

void HubFrame::addLine(const Identity& i, const tstring& aLine, CHARFORMAT2& cf, bool bUseEmo/* = true*/) {
	ctrlClient.AdjustTextSize();

	if(BOOLSETTING(LOG_MAIN_CHAT)) {
		StringMap params;
		params["message"] = Text::fromT(aLine);
		client->getHubIdentity().getParams(params, "hub", false);
		params["hubURL"] = client->getHubUrl();
		client->getMyIdentity().getParams(params, "my", true);
		LOG(LogManager::CHAT, params);
	}

	if(timeStamps) {
		ctrlClient.AppendText(i, Text::toT(client->getCurrentNick()), Text::toT("[" + Util::getShortTimeString() + "] "), aLine.c_str(), cf, bUseEmo);
	} else {
		ctrlClient.AppendText(i, Text::toT(client->getCurrentNick()), _T(""), aLine.c_str(), cf, bUseEmo);
	}
	if (BOOLSETTING(BOLD_HUB)) {
		setDirty();
	}
}

LRESULT HubFrame::onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 
	tabMenuShown = true;
	OMenu tabMenu, copyHubMenu;

	tabMenu.CreatePopupMenu();
	tabMenu.InsertSeparatorFirst(Text::toT((client->getHubName() != "") ? (client->getHubName().size() > 50 ? client->getHubName().substr(0, 50) : client->getHubName()) : client->getHubUrl()));	
	if(BOOLSETTING(LOG_MAIN_CHAT)) {
		tabMenu.AppendMenu(MF_STRING, IDC_OPEN_HUB_LOG, CTSTRING(OPEN_HUB_LOG));
		tabMenu.AppendMenu(MF_SEPARATOR);
	}
	tabMenu.AppendMenu(MF_STRING, IDC_ADD_AS_FAVORITE, CTSTRING(ADD_TO_FAVORITES));
	tabMenu.AppendMenu(MF_STRING, ID_FILE_RECONNECT, CTSTRING(MENU_RECONNECT));
	tabMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)copyHubMenu, CTSTRING(COPY));
	prepareMenu(tabMenu, ::UserCommand::CONTEXT_HUB, client->getHubUrl());
	tabMenu.AppendMenu(MF_SEPARATOR);
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));
	
	copyHubMenu.CreatePopupMenu();
	copyHubMenu.InsertSeparatorFirst(TSTRING(COPY));
	copyHubMenu.AppendMenu(MF_STRING, IDC_COPY_HUBNAME, CTSTRING(HUB_NAME));
	copyHubMenu.AppendMenu(MF_STRING, IDC_COPY_HUBADDRESS, CTSTRING(HUB_ADDRESS));

	if(!client->isConnected())
		tabMenu.EnableMenuItem((UINT)(HMENU)copyHubMenu, MF_GRAYED);
	else
		tabMenu.EnableMenuItem((UINT)(HMENU)copyHubMenu, MF_ENABLED);
	
	tabMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	return TRUE;
}

LRESULT HubFrame::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	HDC hDC = (HDC)wParam;
	::SetBkColor(hDC, WinUtil::bgColor);
	::SetTextColor(hDC, WinUtil::textColor);
	return (LRESULT)WinUtil::bgBrush;
}
	
LRESULT HubFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	CRect rc;            // client area of window 
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click
	tabMenuShown = false;
	OMenu Mnu;
	Mnu.CreatePopupMenu();

	ChatCtrl::sSelectedUser = Util::emptyStringT;

	ctrlUsers.GetHeader().GetWindowRect(&rc);
		
	if(PtInRect(&rc, pt) && showUsers) {
		ctrlUsers.showMenu(pt);
		return TRUE;
	}
		
	if(reinterpret_cast<HWND>(wParam) == ctrlUsers && showUsers && (ctrlUsers.GetSelectedCount() > 0)) {
		if ( ctrlUsers.GetSelectedCount() == 1 ) {
			if(pt.x == -1 && pt.y == -1) {
				WinUtil::getContextMenuPos(ctrlUsers, pt);
			}
			int i = -1;
			i = ctrlUsers.GetNextItem(i, LVNI_SELECTED);
			if ( i >= 0 ) {
				ChatCtrl::sSelectedUser = Text::toT(((OnlineUser*)ctrlUsers.getItemData(i))->getNick());
			}
		}

		if(PreparePopupMenu(&ctrlUsers, ChatCtrl::sSelectedUser, Mnu)) {
			prepareMenu(Mnu, ::UserCommand::CONTEXT_CHAT, client->getHubUrl());
			Mnu.AppendMenu(MF_SEPARATOR);
			Mnu.AppendMenu(MF_STRING, IDC_REFRESH, CTSTRING(REFRESH_USER_LIST));
			Mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
			return TRUE; 
		}
	}

	if(reinterpret_cast<HWND>(wParam) == ctrlEmoticons) {
		if(emoMenu.m_hMenu) {
			emoMenu.DestroyMenu();
			emoMenu.m_hMenu = NULL;
		}
		emoMenu.CreatePopupMenu();
		menuItems = 0;
		emoMenu.InsertSeparatorFirst(_T("Emoticons Pack"));
		emoMenu.AppendMenu(MF_STRING, IDC_EMOMENU, _T("Disabled"));
		if (SETTING(EMOTICONS_FILE)=="Disabled") emoMenu.CheckMenuItem( IDC_EMOMENU, MF_BYCOMMAND | MF_CHECKED );
		// nacteme seznam emoticon packu (vsechny *.xml v adresari EmoPacks)
		WIN32_FIND_DATA data;
		HANDLE hFind;
		hFind = FindFirstFile(Text::toT(Util::getDataPath()+"EmoPacks\\*.xml").c_str(), &data);
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

		emoMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
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

		bool boHitURL = ctrlClient.HitURL();
		if (!boHitURL)
			sSelectedURL = _T("");

		if(PreparePopupMenu(&ctrlClient, ChatCtrl::sSelectedUser, Mnu )) {
			if(ChatCtrl::sSelectedUser.empty()) {
				Mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
				return TRUE;
			} else {
				prepareMenu(Mnu, ::UserCommand::CONTEXT_CHAT, client->getHubUrl());
				Mnu.AppendMenu(MF_SEPARATOR);
				Mnu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR));
				Mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
				return TRUE;
			}
		}
	}
	return FALSE; 
}

void HubFrame::runUserCommand(::UserCommand& uc) {
	if(!WinUtil::getUCParams(m_hWnd, uc, ucLineParams))
		return;

	StringMap ucParams = ucLineParams;

	client->getMyIdentity().getParams(ucParams, "my", true);
	client->getHubIdentity().getParams(ucParams, "hub", false);

	if(tabMenuShown) {
		client->escapeParams(ucParams);
		client->sendUserCmd(Util::formatParams(uc.getCommand(), ucParams, false));
	} else {
		int sel;
		if (!ChatCtrl::sSelectedUser.empty()) {
			sel = ctrlUsers.findItem(ChatCtrl::sSelectedUser);
			if ( sel >= 0 ) { 
				const OnlineUser* u = ctrlUsers.getItemData(sel);
				if(u->getUser()->isOnline()) {
					StringMap tmp = ucParams;		
					u->getIdentity().getParams(tmp, "user", true);
					client->escapeParams(tmp); 
					client->sendUserCmd(Util::formatParams(uc.getCommand(), tmp, false));
				}
			}
		} else {
			sel = -1;
			while((sel = ctrlUsers.GetNextItem(sel, LVNI_SELECTED)) != -1) {
				const OnlineUser* u = ctrlUsers.getItemData(sel);
				if(u->getUser()->isOnline()) {
					StringMap tmp = ucParams;
					u->getIdentity().getParams(tmp, "user", true);
					client->escapeParams(tmp);
					client->sendUserCmd(Util::formatParams(uc.getCommand(), tmp, false));
				}
			}
		}
	}
}

void HubFrame::onTab() {
	if(ctrlMessage.GetWindowTextLength() == 0) {
		handleTab(WinUtil::isShift());
		return;
	}
		
	HWND focus = GetFocus();
	if( (focus == ctrlMessage.m_hWnd) && !WinUtil::isShift() ) 
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
			const OnlineUser* ui = ctrlUsers.getItemData(i);
			const tstring& nick = ui->getText(OnlineUser::COLUMN_NICK);
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
}

LRESULT HubFrame::onFileReconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	client->reconnect();
	return 0;
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
			if ( (GetKeyState(VK_MENU) & 0x8000) ||	( ((GetKeyState(VK_CONTROL) & 0x8000) == 0) ^ (BOOLSETTING(USE_CTRL_FOR_LINE_HISTORY) == true) ) ) {
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
			if ( (GetKeyState(VK_MENU) & 0x8000) ||	( ((GetKeyState(VK_CONTROL) & 0x8000) == 0) ^ (BOOLSETTING(USE_CTRL_FOR_LINE_HISTORY) == true) ) ) {
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
		client->refreshUserList(true);
	} else {
		showUsers = false;
		clearUserList();
	}

	SettingsManager::getInstance()->set(SettingsManager::GET_USER_INFO, showUsers);

	UpdateLayout(FALSE);
	return 0;
}

LRESULT HubFrame::onFollow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!redirect.empty()) {
		if(ClientManager::getInstance()->isConnected(Text::fromT(redirect))) {
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
		clearUserList();
		ClientManager::getInstance()->putClient(client);
		clearTaskList();
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
	if(item != -1) {
		try {
			QueueManager::getInstance()->addList((ctrlUsers.getItemData(item))->getUser(), QueueItem::FLAG_CLIENT_VIEW);
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

void HubFrame::resortUsers() {
	for(FrameIter i = frames.begin(); i != frames.end(); ++i)
		i->second->resortForFavsFirst(true);
}

void HubFrame::closeDisconnected() {
	for(FrameIter i=frames.begin(); i!= frames.end(); ++i) {
		if (!(i->second->client->isConnected())) {
			i->second->PostMessage(WM_CLOSE);
		}
	}
}

void HubFrame::on(FavoriteManagerListener::UserAdded, const FavoriteUser& /*aUser*/) throw() {
	resortForFavsFirst();
}
void HubFrame::on(FavoriteManagerListener::UserRemoved, const FavoriteUser& /*aUser*/) throw() {
	resortForFavsFirst();
}

void HubFrame::resortForFavsFirst(bool justDoIt /* = false */) {
	if(justDoIt || BOOLSETTING(SORT_FAVUSERS_FIRST)) {
		resort = true;
		PostMessage(WM_SPEAKER);
	}
}

void HubFrame::on(Second, uint64_t /*aTick*/) throw() {
	if(updateUsers) {
		updateStatusBar();
		updateUsers = false;
		PostMessage(WM_SPEAKER);
	}
}

void HubFrame::on(Connecting, const Client*) throw() { 
	if(BOOLSETTING(SEARCH_PASSIVE) && ClientManager::getInstance()->isActive(client->getHubUrl())) {
		addLine(TSTRING(ANTI_PASSIVE_SEARCH), WinUtil::m_ChatTextSystem);
	}
	speak(ADD_STATUS_LINE, STRING(CONNECTING_TO) + client->getHubUrl() + "...");
	speak(SET_WINDOW_TITLE, client->getHubUrl());
}
void HubFrame::on(Connected, const Client*) throw() { 
	speak(CONNECTED);
}
void HubFrame::on(UserUpdated, const Client*, const OnlineUser& user) throw() {
	speak(UPDATE_USER_JOIN, user);
}
void HubFrame::on(UsersUpdated, const Client*, const OnlineUser::List& aList) throw() {
	for(OnlineUser::List::const_iterator i = aList.begin(); i != aList.end(); ++i) {
		tasks.add(UPDATE_USER, new UserTask(*(*i)));
	}
	updateUsers = true;
}

void HubFrame::on(ClientListener::UserRemoved, const Client*, const OnlineUser& user) throw() {
	speak(REMOVE_USER, user);
}

void HubFrame::on(Redirect, const Client*, const string& line) throw() { 
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
void HubFrame::on(Failed, const Client*, const string& line) throw() { 
	speak(ADD_STATUS_LINE, line); 
	speak(DISCONNECTED); 
}
void HubFrame::on(GetPassword, const Client*) throw() { 
	speak(GET_PASSWORD);
}
void HubFrame::on(HubUpdated, const Client*) throw() { 
	string hubName = client->getHubName();
	if(!client->getHubDescription().empty()) {
		hubName += " - " + client->getHubDescription();
	}
	hubName += " (" + client->getHubUrl() + ")";
#ifdef _DEBUG
	string version = client->getHubIdentity().get("VE");
	if(!version.empty()) {
		hubName += " - " + version;
	}
#endif
	speak(SET_WINDOW_TITLE, hubName);
}
void HubFrame::on(Message, const Client*, const OnlineUser& from, const string& msg) throw() {
	if(SETTING(FILTER_MESSAGES) && from.getIdentity().isOp()) {
		if((msg.find("is kicking") != string::npos) && (msg.find("because:") != string::npos)) {
			speak(KICK_MSG, Identity(NULL, 0), Util::formatMessage(from.getIdentity().getNick(), msg));
			return;
		}	
	}
	speak(ADD_CHAT_LINE, from.getIdentity(), Util::formatMessage(from.getIdentity().getNick(), msg));
}	

void HubFrame::on(StatusMessage, const Client*, const string& line) {
	if(SETTING(FILTER_MESSAGES)) {
		if((line.find("Hub-Security") != string::npos) && (line.find("was kicked by") != string::npos)) {
			// Do nothing...
		} else if((line.find("is kicking") != string::npos) && (line.find("because:") != string::npos)) {
			speak(KICK_MSG, Identity(NULL, 0), Text::toDOS(line));
		} else {
			speak(ADD_CHAT_LINE, Identity(NULL, 0), Text::toDOS(line));
		}
	} else {
		speak(ADD_CHAT_LINE, Identity(NULL, 0), Text::toDOS(line));
	}
}

void HubFrame::on(PrivateMessage, const Client*, const OnlineUser& from, const OnlineUser& to, const OnlineUser& replyTo, const string& line) throw() { 
	speak(PRIVATE_MESSAGE, from, to, replyTo, Util::formatMessage(from.getIdentity().getNick(), line));
}
void HubFrame::on(NickTaken, const Client*) throw() {
	speak(ADD_STATUS_LINE, STRING(NICK_TAKEN));
}
void HubFrame::on(SearchFlood, const Client*, const string& line) throw() {
	speak(ADD_STATUS_LINE, STRING(SEARCH_SPAM_FROM) + line);
}
void HubFrame::on(CheatMessage, const Client*, const string& line) throw() {
	speak(CHEATING_USER, line);
}
void HubFrame::on(HubTopic, const Client*, const string& line) throw() {
	speak(ADD_STATUS_LINE, STRING(HUB_TOPIC) + "\t" + line);
}

LRESULT HubFrame::onFilterChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	if(uMsg == WM_CHAR && wParam == VK_TAB) {
		handleTab(WinUtil::isShift());
		return 0;
	}
	
	if(!BOOLSETTING(FILTER_ENTER) || (wParam == VK_RETURN)) {
		TCHAR *buf = new TCHAR[ctrlFilter.GetWindowTextLength()+1];
		ctrlFilter.GetWindowText(buf, ctrlFilter.GetWindowTextLength()+1);
		filter = buf;
		delete[] buf;
	
		updateUserList();
		updateUsers = true;
	}

	bHandled = FALSE;

	return 0;
}

LRESULT HubFrame::onSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	TCHAR *buf = new TCHAR[ctrlFilter.GetWindowTextLength()+1];
	ctrlFilter.GetWindowText(buf, ctrlFilter.GetWindowTextLength()+1);
	filter = buf;
	delete[] buf;
	
	updateUserList();
	updateUsers = true;

	bHandled = FALSE;

	return 0;
}

bool HubFrame::parseFilter(FilterModes& mode, int64_t& size) {
	tstring::size_type start = (tstring::size_type)tstring::npos;
	tstring::size_type end = (tstring::size_type)tstring::npos;
	int64_t multiplier = 1;
	
	if(filter.empty()) {
		return false;
	}
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

void HubFrame::updateUserList(OnlineUser* ui) {
	int64_t size = -1;
	FilterModes mode = NONE;
	
	//single update?
	//avoid refreshing the whole list and just update the current item
	//instead
	if(ui != NULL) {
		if(ui->isHidden()) {
			return;
		}
		if(filter.empty()) {
			if(ctrlUsers.findItem(ui) == -1) {
				ui->inc();
				ctrlUsers.insertItem(ui, UserInfoBase::getImage(ui->getIdentity()));
			}
		} else {
			int sel = ctrlFilterSel.GetCurSel();
			bool doSizeCompare = sel == OnlineUser::COLUMN_SHARED && parseFilter(mode, size);

			if(matchFilter(*ui, sel, doSizeCompare, mode, size)) {
				if(ctrlUsers.findItem(ui) == -1) {
					ui->inc();
					ctrlUsers.insertItem(ui, UserInfoBase::getImage(ui->getIdentity()));
				}
			} else {
				int i = ctrlUsers.findItem(ui);
				if(i != -1) {
					ctrlUsers.DeleteItem(i);
					ui->dec();
				}
			}
		}
	} else {
		ctrlUsers.SetRedraw(FALSE);
		clearUserList();

		OnlineUser::List l;
		client->getUserList(l);

		if(filter.empty()) {
			for(OnlineUser::Iter i = l.begin(); i != l.end(); ++i){
				OnlineUser* ui = *i;
				if(!ui->isHidden()) {
					ui->inc();
					ctrlUsers.insertItem(ui, UserInfoBase::getImage(ui->getIdentity()));
				}
					
				ui->dec();	
			}
		} else {
			int sel = ctrlFilterSel.GetCurSel();
			bool doSizeCompare = sel == OnlineUser::COLUMN_SHARED && parseFilter(mode, size);

			for(OnlineUser::Iter i = l.begin(); i != l.end(); ++i) {
				OnlineUser* ui = *i;
				if(!ui->isHidden() && matchFilter(*ui, sel, doSizeCompare, mode, size)) {
					ui->inc();
					ctrlUsers.insertItem(ui, UserInfoBase::getImage(ui->getIdentity()));
				}
				ui->dec();
			}
		}
		ctrlUsers.SetRedraw(TRUE);
	}

}

void HubFrame::handleTab(bool reverse) {
	HWND focus = GetFocus();

	if(reverse) {
		if(focus == ctrlFilterSel.m_hWnd) {
			ctrlFilter.SetFocus();
		} else if(focus == ctrlFilter.m_hWnd) {
			ctrlMessage.SetFocus();
		} else if(focus == ctrlMessage.m_hWnd) {
			ctrlUsers.SetFocus();
		} else if(focus == ctrlUsers.m_hWnd) {
			ctrlClient.SetFocus();
		} else if(focus == ctrlClient.m_hWnd) {
			ctrlFilterSel.SetFocus();
		}
	} else {
		if(focus == ctrlClient.m_hWnd) {
			ctrlUsers.SetFocus();
		} else if(focus == ctrlUsers.m_hWnd) {
			ctrlMessage.SetFocus();
		} else if(focus == ctrlMessage.m_hWnd) {
			ctrlFilter.SetFocus();
		} else if(focus == ctrlFilter.m_hWnd) {
			ctrlFilterSel.SetFocus();
		} else if(focus == ctrlFilterSel.m_hWnd) {
			ctrlClient.SetFocus();
		}
	}
}

bool HubFrame::matchFilter(const OnlineUser& ui, int sel, bool doSizeCompare, FilterModes mode, int64_t size) {
	if(filter.empty())
		return true;

	bool insert = false;
	if(doSizeCompare) {
		switch(mode) {
			case EQUAL: insert = (size == ui.getIdentity().getBytesShared()); break;
			case GREATER_EQUAL: insert = (size <=  ui.getIdentity().getBytesShared()); break;
			case LESS_EQUAL: insert = (size >=  ui.getIdentity().getBytesShared()); break;
			case GREATER: insert = (size < ui.getIdentity().getBytesShared()); break;
			case LESS: insert = (size > ui.getIdentity().getBytesShared()); break;
			case NOT_EQUAL: insert = (size != ui.getIdentity().getBytesShared()); break;
		}
	} else {
		PME reg(filter,_T("i"));
		if(!reg.IsValid()) { 
			insert = true;
		} else if(sel >= OnlineUser::COLUMN_LAST) {
			for(uint8_t i = OnlineUser::COLUMN_FIRST; i < OnlineUser::COLUMN_LAST; ++i) {
				if(reg.match(ui.getText(i))) {
					insert = true;
					break;
				}
			}
		} else {
			if(reg.match(ui.getText(static_cast<uint8_t>(sel))))
				insert = true;
		}
	}

	return insert;
}

void HubFrame::addClientLine(const tstring& aLine, CHARFORMAT2& cf, bool inChat /* = true */) {
	tstring line = _T("[") + Text::toT(Util::getShortTimeString()) + _T("] ") + aLine;

	ctrlStatus.SetText(0, line.c_str());
	while(lastLinesList.size() + 1 > MAX_CLIENT_LINES)
		lastLinesList.erase(lastLinesList.begin());
	lastLinesList.push_back(line);
	
	if (BOOLSETTING(BOLD_HUB)) {
		setDirty();
	}
	
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

bool HubFrame::PreparePopupMenu(CWindow *pCtrl, const tstring& sNick, OMenu& pMenu ) {
	if (copyMenu.m_hMenu != NULL) {
		copyMenu.DestroyMenu();
		copyMenu.m_hMenu = NULL;
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
	copyMenu.InsertSeparator(0, TRUE, TSTRING(COPY));

    bool bIsChat = (pCtrl == ((CWindow*)&ctrlClient));
	if(bIsChat && sNick.empty()) {
		if(!ChatCtrl::sSelectedIP.empty()) {
			pMenu.InsertSeparator(0, TRUE, ChatCtrl::sSelectedIP);
			pMenu.AppendMenu(MF_STRING, IDC_WHOIS_IP, (CTSTRING(WHO_IS) + ChatCtrl::sSelectedIP).c_str() );
			if ( client->isOp() ) {
				pMenu.AppendMenu(MF_SEPARATOR);
				pMenu.AppendMenu(MF_STRING, IDC_BAN_IP, (_T("!banip ") + ChatCtrl::sSelectedIP).c_str());
				pMenu.SetMenuDefaultItem(IDC_BAN_IP);
				pMenu.AppendMenu(MF_STRING, IDC_UNBAN_IP, (_T("!unban ") + ChatCtrl::sSelectedIP).c_str());
				pMenu.AppendMenu(MF_SEPARATOR);
			}
		} else pMenu.InsertSeparator(0, TRUE, _T("Text"));
		pMenu.AppendMenu(MF_STRING, ID_EDIT_COPY, CTSTRING(COPY));
		pMenu.AppendMenu(MF_STRING, IDC_COPY_ACTUAL_LINE,  CTSTRING(COPY_LINE));
		if(!sSelectedURL.empty()) 
  			pMenu.AppendMenu(MF_STRING, IDC_COPY_URL, CTSTRING(COPY_URL));
		pMenu.AppendMenu(MF_SEPARATOR);
		pMenu.AppendMenu(MF_STRING, ID_EDIT_SELECT_ALL, CTSTRING(SELECT_ALL));
		pMenu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR));
		pMenu.AppendMenu(MF_SEPARATOR);
		pMenu.AppendMenu(MF_STRING, IDC_AUTOSCROLL_CHAT, CTSTRING(ASCROLL_CHAT));

		if ( ctrlClient.GetAutoScroll() )
			pMenu.CheckMenuItem(IDC_AUTOSCROLL_CHAT, MF_BYCOMMAND | MF_CHECKED);
	} else {
		if(!sNick.empty()) {
		    bool bIsMe = (sNick == Text::toT(client->getMyNick()));
			// Jediny nick
			pMenu.InsertSeparator(0, TRUE, sNick);

			if(bIsChat) {
				if(!BOOLSETTING(LOG_PRIVATE_CHAT)) {
					pMenu.AppendMenu(MF_STRING | MF_DISABLED, (UINT_PTR)0,  CTSTRING(OPEN_USER_LOG));
				} else {
					pMenu.AppendMenu(MF_STRING, IDC_OPEN_USER_LOG,  CTSTRING(OPEN_USER_LOG));
				}				
				pMenu.AppendMenu(MF_SEPARATOR);
			}
			if(bIsChat == false && bIsMe == false && BOOLSETTING(LOG_PRIVATE_CHAT) == true) {
				pMenu.AppendMenu(MF_STRING, IDC_OPEN_USER_LOG, CTSTRING(OPEN_USER_LOG));
				pMenu.AppendMenu(MF_SEPARATOR);
			}
			if(bIsMe == false) {
				pMenu.AppendMenu(MF_STRING, IDC_PUBLIC_MESSAGE, CTSTRING(SEND_PUBLIC_MESSAGE));
				pMenu.AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CTSTRING(SEND_PRIVATE_MESSAGE));
			}
			if(bIsChat) {
				pMenu.AppendMenu(MF_STRING, IDC_SELECT_USER, CTSTRING(SELECT_USER_LIST));
			}
			if(bIsMe == false) {
				pMenu.AppendMenu(MF_SEPARATOR);
			}	   
			pMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)copyMenu, CTSTRING(COPY));
			if(bIsMe == false) {
				pMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::grantMenu, CTSTRING(GRANT_SLOTS_MENU));
				int i = ctrlUsers.findItem(sNick);
				if ( i >= 0 ) {
					const OnlineUser* ui = ctrlUsers.getItemData(i);
					if (client->isOp() || !ui->getIdentity().isOp()) {
						pMenu.AppendMenu(MF_SEPARATOR);
						if(ignoreList.find(ui->getUser()) == ignoreList.end()) {
							pMenu.AppendMenu(MF_STRING, IDC_IGNORE, CTSTRING(IGNORE_USER));
						} else {    
							pMenu.AppendMenu(MF_STRING, IDC_UNIGNORE, CTSTRING(UNIGNORE_USER));
						}
					}
				}
				pMenu.AppendMenu(MF_SEPARATOR);
				pMenu.AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
				pMenu.AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CTSTRING(MATCH_QUEUE));
				pMenu.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
			}
		} else {
			// Muze byt vice nicku
			if(bIsChat == false) {
				// Pocet oznacenych
				int iCount = ctrlUsers.GetSelectedCount();
				pMenu.InsertSeparator(0, TRUE, Util::toStringW(iCount) + _T(" ") + TSTRING(HUB_USERS));
			}
			if (ctrlUsers.GetSelectedCount() <= 25) {
				pMenu.AppendMenu(MF_STRING, IDC_PUBLIC_MESSAGE, CTSTRING(SEND_PUBLIC_MESSAGE));
				pMenu.AppendMenu(MF_SEPARATOR);
			}
			pMenu.AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
			pMenu.AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CTSTRING(MATCH_QUEUE));
			pMenu.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
		}
		if(bIsChat) {
			switch(SETTING(CHAT_DBLCLICK)) {
		          case 0:
						pMenu.SetMenuDefaultItem(IDC_SELECT_USER);
						break;
		          case 1:
						pMenu.SetMenuDefaultItem(IDC_PUBLIC_MESSAGE);
						break;
		          case 2:
						pMenu.SetMenuDefaultItem(IDC_PRIVATEMESSAGE);
						break;
		          case 3:
						pMenu.SetMenuDefaultItem(IDC_GETLIST);
						break;
		          case 4:
						pMenu.SetMenuDefaultItem(IDC_MATCH_QUEUE);
						break;
		          case 6:
						pMenu.SetMenuDefaultItem(IDC_ADD_TO_FAVORITES);
						break;
			}    
		} else {    
			switch(SETTING(USERLIST_DBLCLICK)) {
		          case 0:
						pMenu.SetMenuDefaultItem( IDC_GETLIST );
						break;
		          case 1:
						pMenu.SetMenuDefaultItem(IDC_PUBLIC_MESSAGE);
						break;
		          case 2:
						pMenu.SetMenuDefaultItem(IDC_PRIVATEMESSAGE);
						break;
		          case 3:
						pMenu.SetMenuDefaultItem(IDC_MATCH_QUEUE);
						break;
		          case 5:
						pMenu.SetMenuDefaultItem(IDC_ADD_TO_FAVORITES);
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
	if(ChatCtrl::sSelectedUser.empty()) {
		// No nick selected
		return 0;
	}

	int pos = ctrlUsers.findItem(ChatCtrl::sSelectedUser);
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
	tstring sUsers = Util::emptyStringT;

	if(!client->isConnected())
		return 0;

	if(!ChatCtrl::sSelectedUser.empty()) {
		sUsers = ChatCtrl::sSelectedUser.c_str();
	} else {
		while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
			if (!sUsers.empty())
  				sUsers += _T(", ");
			sUsers += Text::toT(((OnlineUser*)ctrlUsers.getItemData(i))->getNick());
		}
	}

	int iSelBegin, iSelEnd;
	ctrlMessage.GetSel( iSelBegin, iSelEnd );

	if ( ( iSelBegin == 0 ) && ( iSelEnd == 0 ) ) {
		sUsers += _T(": ");
		if (ctrlMessage.GetWindowTextLength() == 0) {	
			ctrlMessage.SetWindowText(sUsers.c_str());
			ctrlMessage.SetFocus();
			ctrlMessage.SetSel( ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength() );
		} else {
			ctrlMessage.ReplaceSel( sUsers.c_str() );
			ctrlMessage.SetFocus();
		}
	} else {
		sUsers += _T(" ");
		ctrlMessage.ReplaceSel( sUsers.c_str() );
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
		TCHAR* sURLTemp = new TCHAR[(lEnd - lBegin)+1];
		if(sURLTemp) {
			ctrlClient.GetTextRange(lBegin, lEnd, sURLTemp);
			tstring sURL = sURLTemp;
			WinUtil::openLink(sURL);
			delete[] sURLTemp;
		}
	} else if(pEL->msg == WM_RBUTTONUP) {
		sSelectedURL = _T("");
		long lBegin = pEL->chrg.cpMin, lEnd = pEL->chrg.cpMax;
		TCHAR* sURLTemp = new TCHAR[(lEnd - lBegin)+1];
		if(sURLTemp) {
			ctrlClient.GetTextRange(lBegin, lEnd, sURLTemp);
			sSelectedURL = sURLTemp;
			delete[] sURLTemp;
		}

		ctrlClient.SetSel(lBegin, lEnd);
		ctrlClient.InvalidateRect(NULL);
		return 0;
	}
	return 0;
}

LRESULT HubFrame::onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {	
	StringMap params;
	OnlineUser* ui = NULL;
	if(!ChatCtrl::sSelectedUser.empty()) {
		int k = ctrlUsers.findItem(ChatCtrl::sSelectedUser);
		if(k != -1) ui = ctrlUsers.getItemData(k);
	} else {
		int i = -1;
		if((i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
			ui = ctrlUsers.getItemData(i);
		}
	}

	if(ui == NULL) return 0;

	params["userNI"] = ui->getNick();
	params["hubNI"] = client->getHubName();
	params["myNI"] = client->getMyNick();
	params["userCID"] = ui->getUser()->getCID().toBase32();
	params["hubURL"] = client->getHubUrl();
	tstring file = Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_PRIVATE_CHAT), params, false)));
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
	tstring filename = Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_MAIN_CHAT), params, false)));
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

void HubFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
	ctrlUsers.SetImageList(WinUtil::userImages, LVSIL_SMALL);
	//ctrlUsers.Invalidate();
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
			OnlineUser* ui = (OnlineUser*)cd->nmcd.lItemlParam;
			if (FavoriteManager::getInstance()->isFavoriteUser(ui->getUser())) {
				cd->clrText = SETTING(FAVORITE_COLOR);
			} else if (UploadManager::getInstance()->hasReservedSlot(ui->getUser())) {
				cd->clrText = SETTING(RESERVED_SLOT_COLOR);
			} else if (ignoreList.find(ui->getUser()) != ignoreList.end()) {
				cd->clrText = SETTING(IGNORED_COLOR);
			} else if(ui->getUser()->isSet(User::FIREBALL)) {
				cd->clrText = SETTING(FIREBALL_COLOR);
			} else if(ui->getUser()->isSet(User::SERVER)) {
				cd->clrText = SETTING(SERVER_COLOR);
			} else if(ui->getIdentity().isOp()) {
				cd->clrText = SETTING(OP_COLOR);
			} else if(!ui->getIdentity().isTcpActive()) {
				cd->clrText = SETTING(PASIVE_COLOR);
			} else {
				cd->clrText = SETTING(NORMAL_COLOUR);
			}

			if (client->isOp()) {				
				if (!ui->getIdentity().get("BC").empty()) {
					cd->clrText = SETTING(BAD_CLIENT_COLOUR);
				} else if(!ui->getIdentity().get("BF").empty()) {
					cd->clrText = SETTING(BAD_FILELIST_COLOUR);
				} else if(BOOLSETTING(SHOW_SHARE_CHECKED_USERS)) {
					bool cClient = !ui->getIdentity().get("TC").empty();
					bool cFilelist = !ui->getIdentity().get("FC").empty();
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
	if (buf != Text::toT(SETTING(EMOTICONS_FILE))) {
		SettingsManager::getInstance()->set(SettingsManager::EMOTICONS_FILE, Text::fromT(buf));
		g_pEmotionsSetup->Unload();
		g_pEmotionsSetup->Load();
	}
	return 0;
}


LRESULT HubFrame::onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!ChatCtrl::sSelectedUser.empty()) {
		OnlineUser* ui = client->findUser(Text::fromT(ChatCtrl::sSelectedUser));
		if(ui) {
			ui->matchQueue();
		}
	} else {
		ctrlUsers.forEachSelected(&UserInfoBase::matchQueue);
	}
	return 0;
}
LRESULT HubFrame::onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!ChatCtrl::sSelectedUser.empty()) {
		OnlineUser* ui = client->findUser(Text::fromT(ChatCtrl::sSelectedUser));
		if(ui) {
			ui->getList();
		}
	} else {
		ctrlUsers.forEachSelected(&UserInfoBase::getList);
	}
	return 0;
}
LRESULT HubFrame::onBrowseList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!ChatCtrl::sSelectedUser.empty()) {
		OnlineUser* ui = client->findUser(Text::fromT(ChatCtrl::sSelectedUser));
		if(ui) {
			ui->browseList();
		}
	} else {
		ctrlUsers.forEachSelected(&UserInfoBase::browseList);
	}
	return 0;
}
LRESULT HubFrame::onReport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!ChatCtrl::sSelectedUser.empty()) {
		OnlineUser* ui = client->findUser(Text::fromT(ChatCtrl::sSelectedUser));
		if(ui) {
			ui->doReport();
		}
	} else {
		ctrlUsers.forEachSelected(&UserInfoBase::doReport);
	}
	return 0;
}

LRESULT HubFrame::onGetUserResponses(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!ChatCtrl::sSelectedUser.empty()) {
		OnlineUser* ui = client->findUser(Text::fromT(ChatCtrl::sSelectedUser));
		if(ui) {
			ui->getUserResponses();
		}
	} else {
		ctrlUsers.forEachSelected(&UserInfoBase::getUserResponses);
	}
	return 0;
}

LRESULT HubFrame::onCheckList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!ChatCtrl::sSelectedUser.empty()) {
		OnlineUser* ui = client->findUser(Text::fromT(ChatCtrl::sSelectedUser));
		if(ui) {
			ui->checkList();
		}
	} else {
		ctrlUsers.forEachSelected(&UserInfoBase::checkList);
	}
	return 0;
}

LRESULT HubFrame::onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!ChatCtrl::sSelectedUser.empty()) {
		OnlineUser* ui = client->findUser(Text::fromT(ChatCtrl::sSelectedUser));
		if(ui) {
			ui->addFav();
		}
	} else {
		ctrlUsers.forEachSelected(&UserInfoBase::addFav);
	}
	return 0;
}
LRESULT HubFrame::onPrivateMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!ChatCtrl::sSelectedUser.empty()) {
		OnlineUser* ui = client->findUser(Text::fromT(ChatCtrl::sSelectedUser));
		if(ui) {
			ui->pm();
		}
	} else {
		ctrlUsers.forEachSelected(&UserInfoBase::pm);
	}		
	return 0;
}
LRESULT HubFrame::onGrantSlot(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) { 
	if(!ChatCtrl::sSelectedUser.empty()) {
		OnlineUser* ui = client->findUser(Text::fromT(ChatCtrl::sSelectedUser));
		if(ui) {
			switch(wID) {
				case IDC_GRANTSLOT:		ui->grant(); break;
				case IDC_GRANTSLOT_DAY:	ui->grantDay(); break;
				case IDC_GRANTSLOT_HOUR:	ui->grantHour(); break;
				case IDC_GRANTSLOT_WEEK:	ui->grantWeek(); break;
				case IDC_UNGRANTSLOT:	ui->ungrant(); break;
			}
		}
	} else {
		switch(wID) {
			case IDC_GRANTSLOT:		ctrlUsers.forEachSelected(&UserInfoBase::grant); break;
			case IDC_GRANTSLOT_DAY:	ctrlUsers.forEachSelected(&UserInfoBase::grantDay); break;
			case IDC_GRANTSLOT_HOUR:	ctrlUsers.forEachSelected(&UserInfoBase::grantHour); break;
			case IDC_GRANTSLOT_WEEK:	ctrlUsers.forEachSelected(&UserInfoBase::grantWeek); break;
			case IDC_UNGRANTSLOT:	ctrlUsers.forEachSelected(&UserInfoBase::ungrant); break;
		}
	}
	return 0;
}
LRESULT HubFrame::onRemoveAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) { 
	if(!ChatCtrl::sSelectedUser.empty()) {
		OnlineUser* ui = client->findUser(Text::fromT(ChatCtrl::sSelectedUser));
		if(ui) {
			ui->removeAll();
		}
	} else {
		ctrlUsers.forEachSelected(&UserInfoBase::removeAll);
	}
	return 0;
}

/**
 * @file
 * $Id$
 */