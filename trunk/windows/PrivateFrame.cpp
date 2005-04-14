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

#include "PrivateFrame.h"
#include "SearchFrm.h"
#include "WinUtil.h"
#include "CZDCLib.h"
#include "MainFrm.h"

#include "../client/Client.h"
#include "../client/ClientManager.h"
#include "../client/Util.h"
#include "../client/LogManager.h"
#include "../client/UploadManager.h"
#include "../client/ShareManager.h"
#include "../client/FavoriteManager.h"
#include "../client/QueueManager.h"

CriticalSection PrivateFrame::cs;
PrivateFrame::FrameMap PrivateFrame::frames;
tstring pSelectedLine = Util::emptyStringT;
tstring pSelectedURL = Util::emptyStringT;

LRESULT PrivateFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	
	ctrlClient.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY, WS_EX_CLIENTEDGE, IDC_CLIENT);
	
	ctrlClient.LimitText(0);
	ctrlClient.SetFont(WinUtil::font);
	
	ctrlClient.SetBackgroundColor( SETTING(BACKGROUND_COLOR) ); 
	ctrlClient.SetAutoURLDetect(false);
	ctrlClient.SetEventMask( ctrlClient.GetEventMask() | ENM_LINK );
	ctrlMessage.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL | ES_MULTILINE | ES_AUTOVSCROLL, WS_EX_CLIENTEDGE);
	
	ctrlMessageContainer.SubclassWindow(ctrlMessage.m_hWnd);

	ctrlMessage.SetFont(WinUtil::font);
	ctrlMessage.SetLimitText(9999);

	grantMenu.CreatePopupMenu();
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CTSTRING(GRANT_EXTRA_SLOT));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_HOUR, CTSTRING(GRANT_EXTRA_SLOT_HOUR));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_DAY, CTSTRING(GRANT_EXTRA_SLOT_DAY));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_WEEK, CTSTRING(GRANT_EXTRA_SLOT_WEEK));
	grantMenu.AppendMenu(MF_SEPARATOR);
	grantMenu.AppendMenu(MF_STRING, IDC_UNGRANTSLOT, CTSTRING(REMOVE_EXTRA_SLOT));

	tabMenu.CreatePopupMenu();	
	if(BOOLSETTING(LOG_PRIVATE_CHAT)) {
		tabMenu.AppendMenu(MF_STRING, IDC_OPEN_USER_LOG,  CTSTRING(OPEN_USER_LOG));
		tabMenu.AppendMenu(MF_SEPARATOR);
	}
	tabMenu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR));
	tabMenu.AppendMenu(MF_SEPARATOR);
	tabMenu.AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
	tabMenu.AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CTSTRING(MATCH_QUEUE));
	tabMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)grantMenu, CTSTRING(GRANT_SLOTS_MENU));
	tabMenu.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));

	PostMessage(WM_SPEAKER, USER_UPDATED);
	created = true;

	ClientManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	if(BOOLSETTING(SHOW_PM_LOG)){
		readLog();
	}

	bHandled = FALSE;
	return 1;
}

void PrivateFrame::gotMessage(const User::Ptr& aUser, const tstring& aMessage) {
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

				if(BOOLSETTING(POPUP_PM)) {
					MainFrame::getMainFrame()->ShowBalloonTip(Text::toT(aUser->getFullNick()).c_str(), CTSTRING(PRIVATE_MESSAGE));
				}

				if((BOOLSETTING(PRIVATE_MESSAGE_BEEP)) && (!BOOLSETTING(SOUNDS_DISABLED))) {
					if(SETTING(BEEPFILE).empty()) {
						MessageBeep(MB_OK);
					} else {
						PlaySound(Text::toT(SETTING(BEEPFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
					}
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
				p->sendMessage(Text::toT(Util::getAwayMessage()));
			}

			if(BOOLSETTING(POPUP_NEW_PM)) {
				MainFrame::getMainFrame()->ShowBalloonTip(Text::toT(aUser->getFullNick()).c_str(), CTSTRING(PRIVATE_MESSAGE));
			}

			if((BOOLSETTING(PRIVATE_MESSAGE_BEEP) || BOOLSETTING(PRIVATE_MESSAGE_BEEP_OPEN)) && (!BOOLSETTING(SOUNDS_DISABLED))) {
				if (SETTING(BEEPFILE).empty()) {
					MessageBeep(MB_OK);
				} else {
					::PlaySound(Text::toT(SETTING(BEEPFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
				}
			}
		}
	} else {
		if(BOOLSETTING(POPUP_PM)) {
			MainFrame::getMainFrame()->ShowBalloonTip(Text::toT(aUser->getFullNick()).c_str(), CTSTRING(PRIVATE_MESSAGE));
		}

		if((BOOLSETTING(PRIVATE_MESSAGE_BEEP)) && (!BOOLSETTING(SOUNDS_DISABLED))) {
			if (SETTING(BEEPFILE).empty()) {
				MessageBeep(MB_OK);
			} else {
				::PlaySound(Text::toT(SETTING(BEEPFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
			}
		}
		i->second->addLine(aMessage);
	}
}

void PrivateFrame::openWindow(const User::Ptr& aUser, const tstring& msg) {
	PrivateFrame* p = NULL;
	Lock l(cs);
	FrameIter i = frames.find(aUser);
	if(i == frames.end()) {
		p = new PrivateFrame(aUser);
		frames[aUser] = p;
		p->CreateEx(WinUtil::mdiClient);
	} else {
		p = i->second;
		if(::IsIconic(p->m_hWnd))
			::ShowWindow(p->m_hWnd, SW_RESTORE);
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
		if ((uMsg == WM_CHAR) && (GetFocus() == ctrlMessage.m_hWnd) && (wParam != VK_RETURN) && (wParam != VK_TAB) && (wParam != VK_BACK)) {
			if ((!SETTING(SOUND_TYPING_NOTIFY).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
				PlaySound(Text::toT(SETTING(SOUND_TYPING_NOTIFY)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
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

void PrivateFrame::onEnter()
{
	bool resetText = true;

	if(ctrlMessage.GetWindowTextLength() > 0) {
		AutoArray<TCHAR> msg(ctrlMessage.GetWindowTextLength()+1);
		ctrlMessage.GetWindowText(msg, ctrlMessage.GetWindowTextLength()+1);
		tstring s(msg, ctrlMessage.GetWindowTextLength());

		if(BOOLSETTING(CZCHARS_DISABLE))
			s = Text::toT(Util::disableCzChars(Text::fromT(s)));
		// save command in history, reset current buffer pointer to the newest command
		curCommandPosition = prevCommands.size();		//this places it one position beyond a legal subscript
		if (!curCommandPosition || curCommandPosition > 0 && prevCommands[curCommandPosition - 1] != s) {
			++curCommandPosition;
			prevCommands.push_back(s);
		}
		currentCommand = Util::emptyStringT;

		// Process special commands
		if(s[0] == '/') {
			tstring m = s;
			tstring param;
			tstring message;
			tstring status;
			if(WinUtil::checkCommand(s, param, message, status)) {
				if(!message.empty()) {
					sendMessage(message);
				}
				if(!status.empty()) {
					addClientLine(status);
				}
			} else if((Util::stricmp(s.c_str(), _T("clear")) == 0) || (Util::stricmp(s.c_str(), _T("cls")) == 0)) {
				ctrlClient.SetWindowText(_T(""));
			} else if(Util::stricmp(s.c_str(), _T("grant")) == 0) {
				UploadManager::getInstance()->reserveSlot(getUser());
				addClientLine(TSTRING(SLOT_GRANTED));
			} else if(Util::stricmp(s.c_str(), _T("close")) == 0) {
				PostMessage(WM_CLOSE);
			} else if((Util::stricmp(s.c_str(), _T("favorite")) == 0) || (Util::stricmp(s.c_str(), _T("fav")) == 0)) {
				FavoriteManager::getInstance()->addFavoriteUser(getUser());
				addClientLine(TSTRING(FAVORITE_USER_ADDED));
			} else if(Util::stricmp(s.c_str(), _T("getlist")) == 0) {
				BOOL bTmp;
				onGetList(0,0,0,bTmp);
			} else if(Util::stricmp(s.c_str(), _T("log")) == 0) {
				StringMap params;
				params["user"] = user->getNick();
				params["hub"] = user->getClientName();
				params["mynick"] = user->getClientNick(); 
				params["mycid"] = user->getClientCID().toBase32(); 
				params["cid"] = user->getCID().toBase32(); 
				params["hubaddr"] = user->getClientAddressPort();
				WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_PRIVATE_CHAT), params))));
			} else if(Util::stricmp(s.c_str(), _T("stats")) == 0) {
				sendMessage(Text::toT(WinUtil::generateStats()));
			} else if(Util::stricmp(s.c_str(), _T("help")) == 0) {
				addLine(_T("*** ") + Text::toT(WinUtil::commands) + _T(", /getlist, /clear, /grant, /close, /favorite, /winamp"), WinUtil::m_ChatTextSystem);
			} else {
				if(user->isOnline()) {
					sendMessage(tstring(m));
				} else {
					ctrlStatus.SetText(0, CTSTRING(USER_WENT_OFFLINE));
					resetText = false;
				}
			}
		} else {
			if(user->isOnline()) {
				sendMessage(s);
			} else {
				ctrlStatus.SetText(0, CTSTRING(USER_WENT_OFFLINE));
				resetText = false;
			}
		}
		if(resetText)
			ctrlMessage.SetWindowText(_T(""));
	} 
}

LRESULT PrivateFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		ClientManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		Lock l(cs);
		frames.erase(user);

		bHandled = FALSE;
		return 0;
	}
}

void PrivateFrame::addLine(const tstring& aLine) {
	addLine( aLine, WinUtil::m_ChatTextGeneral );
}

void PrivateFrame::addLine(const tstring& aLine, CHARFORMAT2& cf) {
	if(!created) {
		if(BOOLSETTING(POPUNDER_PM))
			WinUtil::hiddenCreateEx(this);
		else
		CreateEx(WinUtil::mdiClient);
	}
	ctrlClient.AdjustTextSize();

	CRect r;
	ctrlClient.GetClientRect(r);

	tstring sTmp = aLine;
	tstring sAuthor = _T("");
	int iAuthorLen = 0;
	bool isMe = false;
	if(aLine[0] == _T('<')) {
		string::size_type i = aLine.find(_T(">"));
		if (i != string::npos) {
     		sAuthor = aLine.substr(1, i-1);
     		iAuthorLen = i;
			if (_tcsncmp(_T(" /me "), aLine.substr(i+1, 5).c_str(), 5) == 0 ) {
				isMe = true;
				sTmp = _T("* ") + sAuthor + aLine.substr(i+5);
			}
		}
	}

	if(BOOLSETTING(LOG_PRIVATE_CHAT)) {
		StringMap params;
		params["message"] = Text::fromT(sTmp);
		params["user"] = user->getNick();
		params["hub"] = user->getClientName();
		params["hubaddr"] = user->getClientAddressPort();
		params["mynick"] = user->getClientNick(); 
		params["mycid"] = user->getClientCID().toBase32(); 
		params["cid"] = user->getCID().toBase32(); 
		LOG(LogManager::PM, params);
	}

	if(user->isOnline()) {
		sMyNick = user->getClient()->getNick().c_str();
	} else {
		sMyNick = SETTING(NICK).c_str();
	}

	if(BOOLSETTING(TIME_STAMPS)) {
		ctrlClient.AppendText(Text::toT(sMyNick).c_str(), Text::toT("[" + Util::getShortTimeString() + "] ").c_str(), sTmp.c_str(), cf, sAuthor.c_str(), iAuthorLen, isMe);
		
	} else {
		ctrlClient.AppendText(Text::toT(sMyNick).c_str(), _T(""), sTmp.c_str(), cf, sAuthor.c_str(), iAuthorLen, isMe);
	}
	addClientLine(CTSTRING(LAST_CHANGE) +  Text::toT(Util::getTimeString()));

	if (BOOLSETTING(TAB_DIRTY)) {
		setDirty();
	}
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
	ctrlClient.SetWindowText(_T(""));
	return 0;
}

LRESULT PrivateFrame::onCopyActualLine(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (pSelectedLine != _T("")) {
		WinUtil::setClipboard(pSelectedLine);
	}
	return 0;
}

LRESULT PrivateFrame::onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 
	prepareMenu(tabMenu, UserCommand::CONTEXT_CHAT, Text::toT(user->getClientAddressPort()), user->isClientOp());
	if(!(tabMenu.GetMenuState(tabMenu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {	
		tabMenu.AppendMenu(MF_SEPARATOR);
	}
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));
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
	ucParams["mycid"] = user->getClientCID().toBase32();

	if(user->isOnline()) {
		user->getParams(ucParams);
		user->clientEscapeParams(ucParams);
		user->sendUserCmd(Util::formatParams(uc.getCommand(), ucParams));
	}
	return;
};



LRESULT PrivateFrame::onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	try {
		QueueManager::getInstance()->addList(user, QueueItem::FLAG_CLIENT_VIEW);
	} catch(const Exception& e) {
		addClientLine(Text::toT(e.getError()));
	}
	return 0;
}

LRESULT PrivateFrame::onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	try {
		QueueManager::getInstance()->addList(user, QueueItem::FLAG_MATCH_QUEUE);
	} catch(const Exception& e) {
		addClientLine(Text::toT(e.getError()));
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
	FavoriteManager::getInstance()->addFavoriteUser(user);
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
		SetWindowText(Text::toT(user->getFullNick()).c_str());
		setTabColor(RGB(0, 255,	255));
		if(isoffline) {
			if(BOOLSETTING(STATUS_IN_CHAT)) {
				addLine(_T(" *** ") + TSTRING(USER_WENT_ONLINE) + _T(" [") + Text::toT(user->getFullNick()) + _T("] ***"), WinUtil::m_ChatTextServer);
			} else {
				addClientLine(_T(" *** ") + TSTRING(USER_WENT_ONLINE) + _T(" [") + Text::toT(user->getFullNick()) + _T("] ***"));
			}
		}
		isoffline = false;
	} else {
		setIconState();
        SetWindowText(Text::toT(user->getFullNick()).c_str());
		setTabColor(RGB(255, 0, 0));
		if(BOOLSETTING(STATUS_IN_CHAT)) {
			addLine(_T(" *** ") + TSTRING(USER_WENT_OFFLINE) + _T(" [") + Text::toT(user->getFullNick()) + _T("] ***"), WinUtil::m_ChatTextServer);
		} else {
			addClientLine(_T(" *** ") + TSTRING(USER_WENT_OFFLINE) + _T(" [") + Text::toT(user->getFullNick()) + _T("] ***"));
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

	pSelectedLine = _T("");

	bool bHitURL = ctrlClient.HitURL(p);
	if (!bHitURL)
		pSelectedURL = _T("");

	int i = ctrlClient.CharFromPos(p);
	int line = ctrlClient.LineFromChar(i);
	int c = LOWORD(i) - ctrlClient.LineIndex(line);
	int len = ctrlClient.LineLength(i) + 1;
	if ( len < 3 )
		return 0;
	TCHAR* buf = new TCHAR[len];
	ctrlClient.GetLine(line, buf, len);
	tstring x = tstring(buf, len-1);
	delete buf;
	string::size_type start = x.find_last_of(_T(" <\t\r\n"), c);
	if (start == string::npos) { start = 0; }
	tstring nick = Text::toT(user->getNick());
	if (x.substr(start, (nick.length() + 2) ) == (_T("<") + nick + _T(">"))) {
		if(!user->isOnline()) {
			return S_OK;
		}
		prepareMenu(tabMenu, UserCommand::CONTEXT_CHAT, Text::toT(user->getClientAddressPort()), user->isClientOp());
		if(!(tabMenu.GetMenuState(tabMenu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {	
			tabMenu.AppendMenu(MF_SEPARATOR);
		}
		tabMenu.InsertSeparatorFirst(Text::fromT(nick));
		tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));
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
		textMenu.AppendMenu(MF_STRING, ID_EDIT_COPY, CTSTRING(COPY));
		textMenu.AppendMenu(MF_STRING, IDC_COPY_ACTUAL_LINE,  CTSTRING(COPY_LINE));
		if(pSelectedURL != _T(""))
			textMenu.AppendMenu(MF_STRING, IDC_COPY_URL, CTSTRING(COPY_URL));
		textMenu.AppendMenu(MF_SEPARATOR);
		textMenu.AppendMenu(MF_STRING, ID_EDIT_SELECT_ALL, CTSTRING(SELECT_ALL));
		textMenu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR));

		pSelectedLine = Text::toT(ctrlClient.LineFromPos(p));
		textMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, cpt.x, cpt.y, m_hWnd);
		bHandled = TRUE;
	}
	return S_OK;
}

LRESULT PrivateFrame::onClientEnLink(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
	ENLINK* pEL = (ENLINK*)pnmh;

	if ( pEL->msg == WM_LBUTTONUP ) {
		long lBegin = pEL->chrg.cpMin, lEnd = pEL->chrg.cpMax;
		TCHAR sURLTemp[INTERNET_MAX_URL_LENGTH];
		int iRet = ctrlClient.GetTextRange(lBegin, lEnd, sURLTemp);
		UNREFERENCED_PARAMETER(iRet);
		tstring sURL = sURLTemp;
		WinUtil::openLink(sURL);
	} else if ( pEL->msg == WM_RBUTTONUP ) {
		pSelectedURL = _T("");
		long lBegin = pEL->chrg.cpMin, lEnd = pEL->chrg.cpMax;
		TCHAR sURLTemp[INTERNET_MAX_URL_LENGTH];
		int iRet = ctrlClient.GetTextRange( lBegin, lEnd, sURLTemp );
		UNREFERENCED_PARAMETER(iRet);
		pSelectedURL = sURLTemp;

		ctrlClient.SetSel( lBegin, lEnd );
		ctrlClient.InvalidateRect( NULL );
		return 0;
	}
	return 0;
}

void PrivateFrame::readLog() {
	StringMap params;	
	params["user"] = user->getNick();	
	params["hub"] = user->getClientName();
	params["mynick"] = user->getClientNick();	
	params["mycid"] = user->getClientCID().toBase32();	
	params["cid"] = user->getCID().toBase32();	
	params["hubaddr"] = user->getClientAddressPort();	
	string path = Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_PRIVATE_CHAT), params));
		
	try {
		if (SETTING(SHOW_LAST_LINES_LOG) > 0) {
			File f(path, File::READ, File::OPEN);
		
			int64_t size = f.getSize();

			if(size > 32*1024) {
				f.setPos(size - 32*1024);
			}
			string buf = f.read(32*1024);
			StringList lines;

			if(Util::strnicmp(buf.c_str(), "\xef\xbb\xbf", 3) == 0)
				lines = StringTokenizer<string>(buf.substr(3), "\r\n").getTokens();
			else
				lines = StringTokenizer<string>(buf, "\r\n").getTokens();

			int linesCount = lines.size();

			int i = linesCount > (SETTING(SHOW_LAST_LINES_LOG) + 1) ? linesCount - (SETTING(SHOW_LAST_LINES_LOG)) : 0;

			for(; i < linesCount; ++i){
				if(!lines[i].empty())
					ctrlClient.AppendText(_T("- "), _T(""), (Text::toT(lines[i])).c_str(), WinUtil::m_ChatTextLog, _T(""));
			}

			f.close();
		}
	} catch(FileException & /*e*/){
	}
}

LRESULT PrivateFrame::onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {	
	StringMap params;
	params["user"] = user->getNick();
	params["hub"] = user->getClientName();
	params["mynick"] = user->getClientNick(); 
	params["mycid"] = user->getClientCID().toBase32(); 
	params["cid"] = user->getCID().toBase32(); 
	params["hubaddr"] = user->getClientAddressPort();
	string file = Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_PRIVATE_CHAT), params));
	if(Util::fileExists(file)) {
		ShellExecute(NULL, NULL, Text::toT(file).c_str(), NULL, NULL, SW_SHOWNORMAL);
	} else {
		MessageBox(CTSTRING(NO_LOG_FOR_USER), CTSTRING(NO_LOG_FOR_USER), MB_OK );	  
	}	

	return 0;
}

LRESULT PrivateFrame::onCopyURL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (pSelectedURL != _T("")) {
		WinUtil::setClipboard(pSelectedURL);
	}
	return 0;
}

void PrivateFrame::on(SettingsManagerListener::Save, SimpleXML* /*xml*/) throw() {
	ctrlClient.SetBackgroundColor(WinUtil::bgColor);
	RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

/**
 * @file
 * $Id$
 */
