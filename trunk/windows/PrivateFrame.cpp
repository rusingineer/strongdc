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

#include "PrivateFrame.h"
#include "SearchFrm.h"
#include "CZDCLib.h"

#include "../client/Client.h"
#include "../client/ClientManager.h"
#include "../client/Util.h"
#include "../client/LogManager.h"
#include "../client/UploadManager.h"
#include "../client/ShareManager.h"
#include "../client/HubManager.h"
#include "../client/QueueManager.h"

CriticalSection PrivateFrame::cs;
PrivateFrame::FrameMap PrivateFrame::frames;
string pSelectedLine = "";
string pSelectedURL = "";

LRESULT PrivateFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	
	ctrlClient.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY, WS_EX_CLIENTEDGE, IDC_CLIENT);
	
	ctrlClient.LimitText(0);
	ctrlClient.SetFont(WinUtil::font);
	
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

	m_ChatTextLog = m_ChatTextGeneral;
	m_ChatTextLog.crTextColor = CZDCLib::blendColors(SETTING(TEXT_GENERAL_BACK_COLOR), SETTING(TEXT_GENERAL_FORE_COLOR), 0.4);

	ctrlClient.SetBackgroundColor( SETTING(BACKGROUND_COLOR) ); 
	ctrlClient.SetAutoURLDetect( true );
	ctrlClient.SetEventMask( ctrlClient.GetEventMask() | ENM_LINK );
	ctrlMessage.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL | ES_MULTILINE | ES_AUTOVSCROLL, WS_EX_CLIENTEDGE);
	
	ctrlMessageContainer.SubclassWindow(ctrlMessage.m_hWnd);

	ctrlMessage.SetFont(WinUtil::font);

	grantMenu.CreatePopupMenu();
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CSTRING(GRANT_EXTRA_SLOT));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_HOUR, CSTRING(GRANT_EXTRA_SLOT_HOUR));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_DAY, CSTRING(GRANT_EXTRA_SLOT_DAY));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_WEEK, CSTRING(GRANT_EXTRA_SLOT_WEEK));
	grantMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	grantMenu.AppendMenu(MF_STRING, IDC_UNGRANTSLOT, CSTRING(REMOVE_EXTRA_SLOT));

	textMenu.CreatePopupMenu();
	textMenu.AppendMenu(MF_STRING, ID_EDIT_COPY, CSTRING(COPY));
	textMenu.AppendMenu(MF_STRING, IDC_COPY_ACTUAL_LINE,  CSTRING(COPY_LINE));
	textMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	textMenu.AppendMenu(MF_STRING, ID_EDIT_SELECT_ALL, CSTRING(SELECT_ALL));
	textMenu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CSTRING(CLEAR));

	tabMenu.CreatePopupMenu();	
	if(BOOLSETTING(LOG_PRIVATE_CHAT)) {
		tabMenu.AppendMenu(MF_STRING, IDC_OPEN_USER_LOG,  CSTRING(OPEN_USER_LOG));
		tabMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	}
	tabMenu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CSTRING(CLEAR));
	tabMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	tabMenu.AppendMenu(MF_STRING, IDC_GETLIST, CSTRING(GET_FILE_LIST));
	tabMenu.AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CSTRING(MATCH_QUEUE));
	tabMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)grantMenu, CSTRING(GRANT_SLOTS_MENU));
	tabMenu.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CSTRING(ADD_TO_FAVORITES));

	PostMessage(WM_SPEAKER, USER_UPDATED);
	created = true;

	ClientManager::getInstance()->addListener(this);

	m_hMenu = WinUtil::mainMenu;
	
	if(BOOLSETTING(SHOW_PM_LOG)){
		if(user->isOnline()) {
			sMyNick = user->getClient()->getNick().c_str();
		} else {
			sMyNick = SETTING(NICK).c_str();
		}
		string lastsession = LOGTAIL(user->getNick(), SETTING(PM_LOG_LINES));
		lastsession = lastsession.substr(0,lastsession.size()-2);
		if(lastsession.length() > 0)
			ctrlClient.AppendText(sMyNick, "", lastsession.c_str(), m_ChatTextLog, "");
	}

	bHandled = FALSE;
	return 1;
}

void PrivateFrame::gotMessage(const User::Ptr& aUser, const string& aMessage) {
	PrivateFrame* p = NULL;
	Lock l(cs);
	FrameIter i = frames.find(aUser);
	if(i == frames.end()) {
		bool found = false;
		for(i = frames.begin(); i != frames.end(); ++i) {
			if( (!i->first->isOnline()) && 
				(i->first->getNick() == aUser->getNick()) &&
				(i->first->getLastHubAddress() == aUser->getLastHubAddress()) ) {
				
				found = true;
				p = i->second;
				frames.erase(i);
				frames[aUser] = p;
				p->setUser(aUser);
				p->addLine(aMessage);
				if((BOOLSETTING(PRIVATE_MESSAGE_BEEP)) && (!BOOLSETTING(SOUNDS_DISABLED))) {
					if (SETTING(BEEPFILE).empty())
						MessageBeep(MB_OK);
					else
						PlaySound(SETTING(BEEPFILE).c_str(), NULL, SND_FILENAME | SND_ASYNC);
				}
				break;
			}
		}
		if(!found) {
			p = new PrivateFrame(aUser);
			frames[aUser] = p;
			p->addLine(aMessage);
			if(Util::getAway()) {
				// if no_awaymsg_to_bots is set, and aUser has an empty connection type (i.e. probably is a bot), then don't send
				if(!(BOOLSETTING(NO_AWAYMSG_TO_BOTS) && aUser->getConnection().empty()))
				p->sendMessage(Util::getAwayMessage());
			}

			if((BOOLSETTING(PRIVATE_MESSAGE_BEEP) || BOOLSETTING(PRIVATE_MESSAGE_BEEP_OPEN)) && (!BOOLSETTING(SOUNDS_DISABLED))) {
				if (SETTING(BEEPFILE).empty())
					MessageBeep(MB_OK);
				else
					::PlaySound(SETTING(BEEPFILE).c_str(), NULL, SND_FILENAME | SND_ASYNC);
			}
		}
	} else {
		if((BOOLSETTING(PRIVATE_MESSAGE_BEEP)) && (!BOOLSETTING(SOUNDS_DISABLED))) {
			if (SETTING(BEEPFILE).empty())
				MessageBeep(MB_OK);
			else
				::PlaySound(SETTING(BEEPFILE).c_str(), NULL, SND_FILENAME | SND_ASYNC);
		}
		i->second->addLine(aMessage);
	}
}

void PrivateFrame::openWindow(const User::Ptr& aUser, const string& msg) {
	PrivateFrame* p = NULL;
	Lock l(cs);
	FrameIter i = frames.find(aUser);
	if(i == frames.end()) {
		p = new PrivateFrame(aUser);
		frames[aUser] = p;
		p->CreateEx(WinUtil::mdiClient);
	} else {
		p = i->second;
		p->MDIActivate(p->m_hWnd);
	}
	if(!msg.empty())
		p->sendMessage(msg);
}

LRESULT PrivateFrame::onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
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
	switch(wParam) {
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

void PrivateFrame::onEnter()
{
	char* message;
	bool resetText = true;

	if(ctrlMessage.GetWindowTextLength() > 0) {
		message = new char[ctrlMessage.GetWindowTextLength()+1];
		ctrlMessage.GetWindowText(message, ctrlMessage.GetWindowTextLength()+1);
		string s(message, ctrlMessage.GetWindowTextLength());
		delete message;

		// save command in history, reset current buffer pointer to the newest command
		curCommandPosition = prevCommands.size();		//this places it one position beyond a legal subscript
		if (!curCommandPosition || curCommandPosition > 0 && prevCommands[curCommandPosition - 1] != s) {
			++curCommandPosition;
			prevCommands.push_back(s);
		}
		currentCommand = "";

		// Process special commands
		if(s[0] == '/') {
			string m = s;
			string param;
			string message;
			string status;
			if(WinUtil::checkCommand(s, param, message, status)) {
				if(!message.empty()) {
					sendMessage(message);
				}
				if(!status.empty()) {
					addClientLine(status);
				}
			} else if(Util::stricmp(s.c_str(), "clear") == 0) {
				ctrlClient.SetWindowText("");
			} else if(Util::stricmp(s.c_str(), "grant") == 0) {
				UploadManager::getInstance()->reserveSlot(getUser());
				addClientLine(STRING(SLOT_GRANTED));
			} else if(Util::stricmp(s.c_str(), "close") == 0) {
				PostMessage(WM_CLOSE);
			} else if((Util::stricmp(s.c_str(), "favorite") == 0) || (Util::stricmp(s.c_str(), "fav") == 0)) {
				HubManager::getInstance()->addFavoriteUser(getUser());
				addClientLine(STRING(FAVORITE_USER_ADDED));
			} else if(Util::stricmp(s.c_str(), "getlist") == 0) {
				BOOL bTmp;
				onGetList(0,0,0,bTmp);
			} else if(Util::stricmp(s.c_str(), "help") == 0) {
				addLine("*** " + WinUtil::commands + ", /getlist, /clear, /grant, /close, /favorite, /winamp", m_ChatTextSystem);
			} else {
				if(user->isOnline()) {
					sendMessage(m);
				} else {
					ctrlStatus.SetText(0, CSTRING(USER_WENT_OFFLINE));
					resetText = false;
				}
			}
		} else {
			if(user->isOnline()) {
				sendMessage(s);
			} else {
				ctrlStatus.SetText(0, CSTRING(USER_WENT_OFFLINE));
				resetText = false;
			}
		}
		if(resetText)
			ctrlMessage.SetWindowText("");
	} 
}

LRESULT PrivateFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if(!closed) {
	ClientManager::getInstance()->removeListener(this);

		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
	Lock l(cs);
	frames.erase(user);

		m_hMenu = NULL;
		MDIDestroy(m_hWnd);
		return 0;
	}
}

void PrivateFrame::addLine(const string& aLine) {
	addLine( aLine, m_ChatTextGeneral );
}

void PrivateFrame::addLine(const string& aLine, CHARFORMAT2& cf) {
	if(!created) {
		CreateEx(WinUtil::mdiClient);
	}
	ctrlClient.AdjustTextSize();

	CRect r;
	ctrlClient.GetClientRect(r);

	string sTmp = aLine;
	string sAuthor = "";
	if (aLine.find("<") == 0) {
		string::size_type i = aLine.find(">");
		if (i != string::npos) {
     		sAuthor = aLine.substr(1, i-1);
			if ( strncmp(" /me ", aLine.substr(i+1, 5).c_str(), 5) == 0 ) {
				sTmp = "* " + sAuthor + aLine.substr(i+5);
			}
		}
	}

	if(BOOLSETTING(LOG_PRIVATE_CHAT)) {
		StringMap params;
		params["message"] = sTmp;
		LOG(user->getNick(), Util::formatParams(SETTING(LOG_FORMAT_PRIVATE_CHAT), params));
	}

	if(user->isOnline()) {
		sMyNick = user->getClient()->getNick().c_str();
	} else {
		sMyNick = SETTING(NICK).c_str();
	}

	if(BOOLSETTING(TIME_STAMPS)) {
		ctrlClient.AppendText(sMyNick, ("[" + Util::getShortTimeString() + "] ").c_str(), sTmp.c_str(), cf, sAuthor.c_str() );
		
	} else {
		ctrlClient.AppendText(sMyNick, "", sTmp.c_str(), cf, sAuthor.c_str() );
	}
	addClientLine(CSTRING(LAST_CHANGE) + Util::getTimeString());

	setDirty();
}

LRESULT PrivateFrame::onEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlClient.Copy();
	return 0;
}

LRESULT PrivateFrame::onEditSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlClient.SetSelAll();
	return 0;
}

LRESULT PrivateFrame::onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlClient.SetWindowText( "" );
	return 0;
}

LRESULT PrivateFrame::onCopyActualLine(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if ( pSelectedLine != "" ) {
		WinUtil::setClipboard(pSelectedLine);
	}
	return 0;
}

LRESULT PrivateFrame::onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 
	prepareMenu(tabMenu, UserCommand::CONTEXT_CHAT, user->getClientAddressPort(), user->isClientOp());
	if(!(tabMenu.GetMenuState(tabMenu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {	
		tabMenu.AppendMenu(MF_SEPARATOR);
	}
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CSTRING(CLOSE));
	tabMenu.InsertSeparatorFirst(user->getNick());	
	tabMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	tabMenu.RemoveFirstItem();
	tabMenu.DeleteMenu(tabMenu.GetMenuItemCount()-1, MF_BYPOSITION);
	tabMenu.DeleteMenu(tabMenu.GetMenuItemCount()-1, MF_BYPOSITION);
	cleanMenu(tabMenu);
	return TRUE;
}

void PrivateFrame::runUserCommand(UserCommand& uc) {
	if(!WinUtil::getUCParams(m_hWnd, uc, ucParams))
		return;

	ucParams["mynick"] = user->getClientNick();

	if(user->isOnline()) {
		user->getParams(ucParams);
		user->send(Util::formatParams(uc.getCommand(), ucParams));
	}
	return;
};



LRESULT PrivateFrame::onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	try {
		QueueManager::getInstance()->addList(user, QueueItem::FLAG_CLIENT_VIEW);
	} catch(const Exception& e) {
		addClientLine(e.getError());
	}
	return 0;
}

LRESULT PrivateFrame::onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	try {
		QueueManager::getInstance()->addList(user, QueueItem::FLAG_MATCH_QUEUE);
	} catch(const Exception& e) {
		addClientLine(e.getError());
	}
	return 0;
}

LRESULT PrivateFrame::onGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UploadManager::getInstance()->reserveSlot(user);
	return 0;
}

LRESULT PrivateFrame::onGrantSlotHour(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UploadManager::getInstance()->reserveSlotHour(user);
	return 0;
};

LRESULT PrivateFrame::onGrantSlotDay(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UploadManager::getInstance()->reserveSlotDay(user);
	return 0;
};

LRESULT PrivateFrame::onGrantSlotWeek(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UploadManager::getInstance()->reserveSlotWeek(user);
	return 0;
};

LRESULT PrivateFrame::onUnGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UploadManager::getInstance()->unreserveSlot(user);
	return 0;
};

LRESULT PrivateFrame::onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	HubManager::getInstance()->addFavoriteUser(user);
	return 0;
}

void PrivateFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
	
	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[1];
		ctrlStatus.GetClientRect(sr);
		
		w[0] = sr.right - 16;

		ctrlStatus.SetParts(1, w);
	}
	
	int h = WinUtil::fontHeight + 4;

	CRect rc = rect;
	rc.bottom -= (2*h) + 10;
	ctrlClient.MoveWindow(rc);
	
	rc = rect;
	rc.bottom -= 2;
	rc.top = rc.bottom - (2*h) - 5;
	rc.left +=2;
	rc.right -=2;
	ctrlMessage.MoveWindow(rc);
	
}

void PrivateFrame::updateTitle() {
	if(user->isOnline()) {
		unsetIconState();
		SetWindowText(user->getFullNick().c_str());
		setTabColor(RGB(0, 255,	255));
		if(isoffline) {
			if(BOOLSETTING(STATUS_IN_CHAT)) {
				addLine(" *** " + STRING(USER_WENT_ONLINE) + " [" + user->getFullNick() + "] ***", m_ChatTextServer);
			} else {
				addClientLine(" *** " + STRING(USER_WENT_ONLINE) + " [" + user->getFullNick() + "] ***");
			}
		}
		isoffline = false;
	} else {
		setIconState();
        if(user->getClientName() == STRING(OFFLINE)) {
            SetWindowText(user->getFullNick().c_str());
        } else {
            SetWindowText((user->getFullNick() + " [" + STRING(OFFLINE) + "]").c_str());
		}
		setTabColor(RGB(255, 0, 0));
		if(BOOLSETTING(STATUS_IN_CHAT)) {
			addLine(" *** " + STRING(USER_WENT_OFFLINE) + " [" + user->getFullNick() + "] ***", m_ChatTextServer);
		} else {
			addClientLine(" *** " + STRING(USER_WENT_OFFLINE) + " [" + user->getFullNick() + "] ***");
		}
		isoffline = true;
	}
	setDirty();
}

LRESULT PrivateFrame::onContextMenu(UINT uMsg, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	bHandled = FALSE;

	POINT p;
	p.x = GET_X_LPARAM(lParam);
	p.y = GET_Y_LPARAM(lParam);
	::ScreenToClient(ctrlClient.m_hWnd, &p);

	POINT cpt;
	GetCursorPos(&cpt);

	pSelectedLine = "";

	bool bHitURL = ctrlClient.HitURL(p);
	if (!bHitURL)
		pSelectedURL = "";

	int i = ctrlClient.CharFromPos(p);
	int line = ctrlClient.LineFromChar(i);
	int c = LOWORD(i) - ctrlClient.LineIndex(line);
	int len = ctrlClient.LineLength(i) + 1;
	if ( len < 3 )
		return 0;
	char* buf = new char[len];
	ctrlClient.GetLine(line, buf, len);
	string x = string(buf, len-1);
	delete buf;
	string::size_type start = x.find_last_of(" <\t\r\n", c);
	if (start == string::npos) { start = 0; }
	string nick = user->getNick();
	if (x.substr(start, (nick.length() + 2) ) == ("<" + nick + ">")) {
		if(!user->isOnline()) {
			return S_OK;
		}
		tabMenu.InsertSeparatorFirst(x);
		prepareMenu(tabMenu, UserCommand::CONTEXT_CHAT, user->getClientAddressPort(), user->isClientOp());
		if(!(tabMenu.GetMenuState(tabMenu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {	
			tabMenu.AppendMenu(MF_SEPARATOR);
		}
		tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CSTRING(CLOSE));
		tabMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, cpt.x, cpt.y, m_hWnd);
		tabMenu.RemoveFirstItem();
		tabMenu.DeleteMenu(tabMenu.GetMenuItemCount()-1, MF_BYPOSITION);
		tabMenu.DeleteMenu(tabMenu.GetMenuItemCount()-1, MF_BYPOSITION);
		cleanMenu(tabMenu);
		bHandled = TRUE;
	} else {
		if (textMenu.m_hMenu != NULL) {
			textMenu.DestroyMenu();
			textMenu.m_hMenu = NULL;
		}

		textMenu.CreatePopupMenu();
		textMenu.AppendMenu(MF_STRING, ID_EDIT_COPY, CSTRING(COPY));
		textMenu.AppendMenu(MF_STRING, IDC_COPY_ACTUAL_LINE,  CSTRING(COPY_LINE));
		if(pSelectedURL != "")
			textMenu.AppendMenu(MF_STRING, IDC_COPY_URL, CSTRING(COPY_URL));
		textMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
		textMenu.AppendMenu(MF_STRING, ID_EDIT_SELECT_ALL, CSTRING(SELECT_ALL));
		textMenu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CSTRING(CLEAR));

		pSelectedLine = ctrlClient.LineFromPos(p);
		textMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, cpt.x, cpt.y, m_hWnd);
		bHandled = TRUE;
	}
	return S_OK;
}

LRESULT PrivateFrame::onClientEnLink(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
	ENLINK* pEL = (ENLINK*)pnmh;

	if ( pEL->msg == WM_LBUTTONUP ) {
		long lBegin = pEL->chrg.cpMin, lEnd = pEL->chrg.cpMax;
		char sURLTemp[INTERNET_MAX_URL_LENGTH];
		int iRet = ctrlClient.GetTextRange( lBegin, lEnd, sURLTemp );
		UNREFERENCED_PARAMETER(iRet);
		string sURL = sURLTemp;
		WinUtil::openLink(sURL);
	} else if ( pEL->msg == WM_RBUTTONUP ) {
		pSelectedURL = "";
		long lBegin = pEL->chrg.cpMin, lEnd = pEL->chrg.cpMax;
		char sURLTemp[INTERNET_MAX_URL_LENGTH];
		int iRet = ctrlClient.GetTextRange( lBegin, lEnd, sURLTemp );
		UNREFERENCED_PARAMETER(iRet);
		pSelectedURL = sURLTemp;

		ctrlClient.SetSel( lBegin, lEnd );
		ctrlClient.InvalidateRect( NULL );
		return 0;
	}
	return 0;
}

LRESULT PrivateFrame::onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {	
	string file = Util::emptyString;
	file = Util::validateFileName(SETTING(LOG_DIRECTORY) + user->getNick() + ".log");
	if(File::existsFile(file)) {
		ShellExecute(NULL, NULL, file.c_str(), NULL, NULL, SW_SHOWNORMAL);
	} else {
		MessageBox(CSTRING(NO_LOG_FOR_USER),CSTRING(NO_LOG_FOR_USER), MB_OK );	  
	}	

	return 0;
}

LRESULT PrivateFrame::onCopyURL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (pSelectedURL != "") {
		WinUtil::setClipboard(pSelectedURL);
	}
	return 0;
}

/**
 * @file
 * $Id$
 */
