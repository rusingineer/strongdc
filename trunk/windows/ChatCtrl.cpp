/* 
 * 
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
#include "Resource.h"
#include "../client/DCPlusPlus.h"
#include "../client/FavoriteManager.h"
#include "../client/UploadManager.h"

#include "ChatCtrl.h"
#include "AGEmotionSetup.h"
#include "PrivateFrame.h"
#include "atlstr.h"

CAGEmotionSetup* g_pEmotionsSetup = NULL;

#define MAX_EMOTICONS 48

tstring ChatCtrl::sSelectedLine = Util::emptyStringT;
tstring ChatCtrl::sSelectedIP = Util::emptyStringT;
tstring ChatCtrl::sSelectedUser = Util::emptyStringT;
tstring ChatCtrl::sSelectedURL = Util::emptyStringT;

static const TCHAR* Links[] = { _T("http://"), _T("https://"), _T("www."), _T("ftp://"), 
	_T("magnet:?"), _T("dchub://"), _T("irc://"), _T("ed2k://"), _T("mms://"), _T("file://"),
	_T("adc://"), _T("adcs://") };

ChatCtrl::ChatCtrl() : client(NULL) {
	if(g_pEmotionsSetup == NULL) {
		g_pEmotionsSetup = new CAGEmotionSetup();
	}
	
	g_pEmotionsSetup->inc();
}

ChatCtrl::~ChatCtrl() {
	if(g_pEmotionsSetup->unique()) {
		g_pEmotionsSetup->dec();
		g_pEmotionsSetup = NULL;
	} else {
		g_pEmotionsSetup->dec();
	}
}
	
void ChatCtrl::AdjustTextSize() {
	if(GetWindowTextLength() > 25000) {
		// We want to limit the buffer to 25000 characters...after that, w95 becomes sad...
		SetRedraw(FALSE);
		SetSel(0, LineIndex(LineFromChar(2000)));
		ReplaceSel(_T(""));

		// scroll to bottom
		PostMessage(EM_SCROLL, SB_BOTTOM, 0);
		SetRedraw(TRUE);
	}
}

void ChatCtrl::AppendText(const Identity& i, const tstring& sMyNick, const tstring& sTime, const TCHAR* sMsg, CHARFORMAT2& cf, bool bUseEmo/* = true*/) {
	SetRedraw(FALSE);

	SCROLLINFO si = { 0 };
	POINT pt = { 0 };

	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
	GetScrollInfo(SB_VERT, &si);
	GetScrollPos(&pt);

	long lSelBegin = 0, lSelEnd = 0;
	long lSelBeginSaved, lSelEndSaved;
	GetSel(lSelBeginSaved, lSelEndSaved);

	// Insert TimeStamp and format with default style
	if(!sTime.empty()) {
		lSelEnd = lSelBegin = GetTextLengthEx(GTL_NUMCHARS);
		SetSel(lSelEnd, lSelEnd);
		ReplaceSel(sTime.c_str(), false);
		lSelEnd = GetTextLengthEx(GTL_NUMCHARS);
		SetSel(lSelBegin, lSelEnd - 1);
		SetSelectionCharFormat(WinUtil::m_TextStyleTimestamp);

		PARAFORMAT2 pf;
		memzero(&pf, sizeof(PARAFORMAT2));
		pf.dwMask = PFM_STARTINDENT; 
		pf.dxStartIndent = 0;
		SetParaFormat(pf);
	}

	CAtlString sText;
	tstring sAuthor = Text::toT(i.getNick());
	bool bMyMess = i.getUser() == ClientManager::getInstance()->getMe();
	if(!sAuthor.empty()) {
		size_t iLen = (sMsg[0] == _T('*')) ? 1 : 0;
		size_t iAuthorLen = _tcslen(sAuthor.c_str())+1;
   		sText = sMsg+iAuthorLen+iLen;
		lSelEnd = lSelBegin = GetTextLengthEx(GTL_NUMCHARS);
		SetSel(lSelEnd, lSelEnd);
		ReplaceSel(((tstring)sMsg).substr(0, iAuthorLen+iLen).c_str(), false);
		if(bMyMess) {
			SetSel(lSelBegin, lSelBegin+iLen+1);
			SetSelectionCharFormat(WinUtil::m_ChatTextMyOwn);
			SetSel(lSelBegin+iLen+1, lSelBegin+iLen+iAuthorLen);
			SetSelectionCharFormat(WinUtil::m_TextStyleMyNick);
		} else {
			bool isFavorite = FavoriteManager::getInstance()->isFavoriteUser(i.getUser());

			if(BOOLSETTING(BOLD_AUTHOR_MESS) || isFavorite || i.isOp()) {
				SetSel(lSelBegin, lSelBegin+iLen+1);
				SetSelectionCharFormat(cf);
				SetSel(lSelBegin+iLen+1, lSelBegin+iLen+iAuthorLen);
				if(isFavorite){
					SetSelectionCharFormat(WinUtil::m_TextStyleFavUsers);
				} else if(i.isOp()) {
					SetSelectionCharFormat(WinUtil::m_TextStyleOPs);
				} else {
					SetSelectionCharFormat(WinUtil::m_TextStyleBold);
				}
			} else {
				SetSel(lSelBegin, lSelBegin+iLen+iAuthorLen);
				SetSelectionCharFormat(cf);
            }
		}
	} else {
        switch(sMsg[0]) {
            case _T('<'): {
                if((_tcschr((TCHAR*)sMsg+1, _T('>'))) != NULL) {
                    size_t iAuthorLen = _tcslen(sMsg+1)+1;
                    sText = sMsg+iAuthorLen;
		            lSelEnd = lSelBegin = GetTextLengthEx(GTL_NUMCHARS);
		            SetSel(lSelEnd, lSelEnd);
            		ReplaceSel(((tstring)sMsg).substr(0, iAuthorLen).c_str(), false);
        			if(BOOLSETTING(BOLD_AUTHOR_MESS)) {
        				SetSel(lSelBegin, lSelBegin+1);
        				SetSelectionCharFormat(cf);
                        SetSel(lSelBegin+1, lSelBegin+iAuthorLen);
        				SetSelectionCharFormat(WinUtil::m_TextStyleBold);
        			} else {
        				SetSel(lSelBegin, lSelBegin+iAuthorLen);
        				SetSelectionCharFormat(cf);
                    }
				} else {
					sText = sMsg;
				}
                break;
            }
            case _T('*'): {
                if(sMsg[1] == _T(' ') && (_tcschr((wchar_t *)sMsg+2, _T(' '))) != NULL) {
                    size_t iAuthorLen = _tcslen(sMsg+2)+1;
                    sText = sMsg+iAuthorLen+1;
		            lSelEnd = lSelBegin = GetTextLengthEx(GTL_NUMCHARS);
		            SetSel(lSelEnd, lSelEnd);
            		ReplaceSel(((tstring)sMsg).substr(0, iAuthorLen+1).c_str(), false);
        			if(BOOLSETTING(BOLD_AUTHOR_MESS)) {
        				SetSel(lSelBegin, lSelBegin+2);
        				SetSelectionCharFormat(cf);
      					SetSel(lSelBegin+2, lSelBegin+1+iAuthorLen);
        				SetSelectionCharFormat(WinUtil::m_TextStyleBold);
        			} else {
        				SetSel(lSelBegin, lSelBegin+1+iAuthorLen);
        				SetSelectionCharFormat(cf);
                    }
				} else {
					sText = sMsg;
				}
                break;
            }
            default:
                sText = sMsg;
                break;
        }
	}

	sText.Remove(_T('\r'));
	sText += "\n";

	// Insert emoticons
	if(bUseEmo && g_pEmotionsSetup->getUseEmoticons()) {
		const CAGEmotion::List& Emoticons = g_pEmotionsSetup->getEmoticonsList();
		uint8_t smiles = 0; int nIdxFound = -1;
		while(true) {
			TCHAR *rpl = NULL;
			CAGEmotion* pFoundEmotion = NULL;
			int len = sText.GetLength();
			for(CAGEmotion::Iter pEmotion = Emoticons.begin(); pEmotion != Emoticons.end(); ++pEmotion) {
				nIdxFound = -1;
				TCHAR *txt = Util::strstr(sText, (*pEmotion)->getEmotionText().c_str(), &nIdxFound);
				if((txt < rpl && txt) || !rpl && txt) {
					if(len > nIdxFound) {
						rpl = txt;
						pFoundEmotion = (*pEmotion);
						len = nIdxFound;
					}
				}
			}

			if(rpl && (smiles < MAX_EMOTICONS)) {
				AppendTextOnly(sMyNick, sText.Left(rpl - sText), cf, bMyMess, sAuthor);
				lSelEnd = GetTextLengthEx(GTL_NUMCHARS);
				SetSel(lSelEnd, lSelEnd);
				CImageDataObject::InsertBitmap(GetOleInterface(), 
					pFoundEmotion->getEmotionBmp(bMyMess ? WinUtil::m_ChatTextMyOwn.crBackColor : WinUtil::m_ChatTextGeneral.crBackColor));

				sText = rpl + pFoundEmotion->getEmotionText().size();
				smiles++;
			} else {
				if(_tcslen(sText) > 0) {
					AppendTextOnly(sMyNick, sText, cf, bMyMess, sAuthor);
				}
				break;
			}
		}
	} else {
		AppendTextOnly(sMyNick, sText, cf, bMyMess, sAuthor);
	}
	SetSel(lSelBeginSaved, lSelEndSaved);
	
	if (lSelBeginSaved == lSelEndSaved && (si.nPage == 0 || (size_t)si.nPos >= (size_t)si.nMax - si.nPage - 5)) {
		PostMessage(EM_SCROLL, SB_BOTTOM, 0);
	} else {
		SetScrollPos(&pt);
	}

	// Force window to redraw
	SetRedraw(TRUE);
	InvalidateRect(NULL);
}

void ChatCtrl::AppendTextOnly(const tstring& sMyNick, const TCHAR* sText, CHARFORMAT2& cf, bool bMyMess, const tstring& sAuthor) {
	// Insert text at the end
	long lSelEnd = GetTextLengthEx(GTL_NUMCHARS);
	long lSelBegin = lSelEnd;
	SetSel(lSelBegin, lSelEnd);
	ReplaceSel(sText, false);

	// Set text format
	CAtlString sMsgLower = sText;
	sMsgLower.MakeLower();

	lSelEnd = GetTextLengthEx(GTL_NUMCHARS);
	SetSel(lSelBegin, lSelEnd);
	SetSelectionCharFormat(bMyMess ? WinUtil::m_ChatTextMyOwn : cf);
	
	// Zvyrazneni vsech URL a nastaveni "klikatelnosti"
	long lSearchFrom = 0;
	for(size_t i = 0; i < (sizeof(Links) / sizeof(Links[0])); i++) {
		long linkStart = sMsgLower.Find(Links[i], lSearchFrom);
		while(linkStart > 0) {
			long linkEnd;
			long linkEndSpace = sMsgLower.Find(_T(" "), linkStart);
			long linkEndLine = sMsgLower.Find(_T("\n"), linkStart);
			if((linkEndSpace <= linkStart && linkEndLine > linkStart) || (linkEndSpace > linkEndLine && linkEndLine > linkStart)) {
				linkEnd = linkEndLine;
			} else if(linkEndSpace > linkStart) {
				linkEnd = linkEndSpace;
			} else {
				linkEnd = _tcslen(sMsgLower);
			}
			SetSel(lSelBegin + linkStart, lSelBegin + linkEnd);
			SetSelectionCharFormat(WinUtil::m_TextStyleURL);
			linkStart = sMsgLower.Find(Links[i], linkEnd);
		}
	}

	// Zvyrazneni vsech vyskytu vlastniho nicku
	long lMyNickStart = -1, lMyNickEnd = -1;	
	CAtlString sNick = sMyNick.c_str();
	sNick.MakeLower();

	while((lMyNickStart = sMsgLower.Find(sNick, lSearchFrom)) >= 0) {
		lMyNickEnd = lMyNickStart + sNick.GetLength();
		SetSel(lSelBegin + lMyNickStart, lSelBegin + lMyNickEnd);
		SetSelectionCharFormat(WinUtil::m_TextStyleMyNick);
		lSearchFrom = lMyNickEnd;

		if(	!SETTING(CHATNAMEFILE).empty() && !BOOLSETTING(SOUNDS_DISABLED) &&
			!sAuthor.empty() && (Util::stricmp(sAuthor.c_str(), sNick) != 0)) {
				::PlaySound(Text::toT(SETTING(CHATNAMEFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);	 	
        }
	}

	// Zvyrazneni vsech vyskytu nicku Favorite useru
	FavoriteManager::FavoriteMap ul = FavoriteManager::getInstance()->getFavoriteUsers();
	for(FavoriteManager::FavoriteMap::const_iterator i = ul.begin(); i != ul.end(); ++i) {
		const FavoriteUser& pUser = i->second;

		lSearchFrom = 0;
		sNick = Text::toT(pUser.getNick()).c_str();
		sNick.MakeLower();

		while((lMyNickStart = sMsgLower.Find(sNick, lSearchFrom)) >= 0) {
			lMyNickEnd = lMyNickStart + sNick.GetLength();
			SetSel(lSelBegin + lMyNickStart, lSelBegin + lMyNickEnd);
			SetSelectionCharFormat(WinUtil::m_TextStyleFavUsers);
			lSearchFrom = lMyNickEnd;
		}
	}
}

bool ChatCtrl::HitNick(const POINT& p, tstring& sNick, int& iBegin, int& iEnd) {
	if(client == NULL) return false;
	
	int iCharPos = CharFromPos(p), line = LineFromChar(iCharPos), len = LineLength(iCharPos) + 1;
	long lSelBegin = 0, lSelEnd = 0;
	if(len < 3)
		return false;

	// Metoda FindWordBreak nestaci, protoze v nicku mohou byt znaky povazovane za konec slova
	int iFindBegin = LineIndex(line), iEnd1 = LineIndex(line) + LineLength(iCharPos);

	for(lSelBegin = iCharPos; lSelBegin >= iFindBegin; lSelBegin--) {
		if(FindWordBreak(WB_ISDELIMITER, lSelBegin))
			break;
	}
	lSelBegin++;
	for(lSelEnd = iCharPos; lSelEnd < iEnd1; lSelEnd++) {
		if(FindWordBreak(WB_ISDELIMITER, lSelEnd))
			break;
	}

	len = lSelEnd - lSelBegin;
	if(len <= 0)
		return false;

	AutoArray<TCHAR> buf(len+1);
	GetTextRange(lSelBegin, lSelEnd, buf);
	tstring sText(buf, len);

	int iLeft = 0, iRight = 0, iCRLF = sText.size(), iPos = sText.find(_T('<'));
	if(iPos >= 0) {
		iLeft = iPos + 1;
		iPos = sText.find(_T('>'), iLeft);
		if(iPos < 0) 
			return false;

		iRight = iPos - 1;
		iCRLF = iRight - iLeft + 1;
	} else {
		iLeft = 0;
	}

	tstring sN = sText.substr(iLeft, iCRLF);
	if(sN.size() == 0)
		return false;

	if(client->findUser(Text::fromT(sN)) != NULL) {
		sNick = sN;
		iBegin = lSelBegin + iLeft;
		iEnd = lSelBegin + iLeft + iCRLF;
		return true;
	}
    
	// Jeste pokus odmazat eventualni koncovou ':' nebo '>' 
	// Nebo pro obecnost posledni znak 
	// A taky prvni znak 
	// A pak prvni i posledni :-)
	if(iCRLF > 1) {
		sN = sText.substr(iLeft, iCRLF - 1);
		if(client->findUser(Text::fromT(sN)) != NULL) {
			sNick = sN;
   			iBegin = lSelBegin + iLeft;
   			iEnd = lSelBegin + iLeft + iCRLF - 1;
			return true;
		}

		sN = sText.substr(iLeft + 1, iCRLF - 1);
		if(client->findUser(Text::fromT(sN)) != NULL) {
        	sNick = sN;
			iBegin = lSelBegin + iLeft + 1;
			iEnd = lSelBegin + iLeft + iCRLF;
			return true;
		}

		sN = sText.substr(iLeft + 1, iCRLF - 2);
		if(client->findUser(Text::fromT(sN)) != NULL) {
			sNick = sN;
   			iBegin = lSelBegin + iLeft + 1;
			iEnd = lSelBegin + iLeft + iCRLF - 1;
			return true;
		}
	}	
	return false;
}

bool ChatCtrl::HitIP(const POINT& p, tstring& sIP, int& iBegin, int& iEnd) {
	int iCharPos = CharFromPos(p), len = LineLength(iCharPos) + 1;
	if(len < 3)
		return false;

	DWORD lPosBegin = FindWordBreak(WB_LEFT, iCharPos);
	DWORD lPosEnd = FindWordBreak(WB_RIGHTBREAK, iCharPos);
	len = lPosEnd - lPosBegin;
	
	AutoArray<TCHAR> buf(len+1);
	GetTextRange(lPosBegin, lPosEnd, buf);
	for(int i = 0; i < len; i++) {
		if(!((buf[i] == 0) || (buf[i] == '.') || ((buf[i] >= '0') && (buf[i] <= '9')))) {
			return false;
		}
	}
	tstring sText(buf, len);

	sText += _T('.');
	int iFindBegin = 0, iPos = -1, iEnd2 = 0;
	bool boOK = true;
	for(int i = 0; i < 4; i++) {
		iPos = sText.find(_T('.'), iFindBegin);
		if(iPos < 0) {
			boOK = false;
			break;
		}
		iEnd2 = atoi(Text::fromT(sText.substr(iFindBegin)).c_str());
		if((iEnd2 < 0) || (iEnd2 > 255)) {
			boOK = false;
			break;
		}
		iFindBegin = iPos + 1;
	}

	if(boOK) {
		sIP = sText.substr(0, iPos);
		iBegin = lPosBegin;
		iEnd = lPosEnd;
	}
	return boOK;
}

bool ChatCtrl::HitURL() {
	long lSelBegin = 0, lSelEnd = 0;
	GetSel(lSelBegin, lSelEnd);
	bool boOK = false;

	CHARFORMAT2 cfSel;
	cfSel.cbSize = sizeof(cfSel);
    
	GetSelectionCharFormat(cfSel);
	if(cfSel.dwEffects & CFE_LINK) {
		boOK = true;
	}
	return boOK;
}

tstring ChatCtrl::LineFromPos(const POINT& p) const {
	int iCharPos = CharFromPos(p), line = LineFromChar(iCharPos), len = LineLength(iCharPos) + 1;
	if(len < 3) {
		return Util::emptyStringT;
	}

	AutoArray<TCHAR> buf(len+1);
	GetLine(line, buf, len);

	return tstring(buf, len - 1);
}

LRESULT ChatCtrl::OnRButtonDown(POINT pt) {
	long lSelBegin = 0, lSelEnd = 0; tstring sSel;

	sSelectedLine = LineFromPos(pt);
	sSelectedUser = Util::emptyStringT;
	sSelectedIP = Util::emptyStringT;

	// Po kliku dovnitr oznaceneho textu si zkusime poznamenat pripadnej nick ci ip...
	// jinak by nam to neuznalo napriklad druhej klik na uz oznaceny nick =)
	GetSel(lSelBegin, lSelEnd);
	int iCharPos = CharFromPos(pt), iBegin = 0, iEnd = 0;
	if((lSelEnd > lSelBegin) && (iCharPos >= lSelBegin) && (iCharPos <= lSelEnd)) {
		if(HitIP(pt, sSel, iBegin, iEnd)) {
			sSelectedIP = sSel;
		} else if(HitNick(pt, sSel, iBegin, iEnd)) {
			sSelectedUser = sSel;
		}
		return 1;
	}

	// Po kliku do IP oznacit IP
	if(HitIP(pt, sSel, iBegin, iEnd)) {
		sSelectedIP = sSel;
		SetSel(iBegin, iEnd);
		InvalidateRect(NULL);
	// Po kliku na Nick oznacit Nick
	} else if(HitNick(pt, sSel, iBegin, iEnd)) {
		sSelectedUser = sSel;
		SetSel(iBegin, iEnd);
		InvalidateRect(NULL);
	}
	return 1;
}

LRESULT ChatCtrl::onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click

	if(pt.x == -1 && pt.y == -1) {
		CRect erc;
		GetRect(&erc);
		pt.x = erc.Width() / 2;
		pt.y = erc.Height() / 2;
		ClientToScreen(&pt);
	}

	POINT ptCl = pt;
	ScreenToClient(&ptCl); 
	OnRButtonDown(ptCl);

	bool boHitURL = HitURL();
	if (!boHitURL)
		sSelectedURL = _T("");

	OMenu menu;
	menu.CreatePopupMenu();

	if (copyMenu.m_hMenu != NULL) {
		// delete copy menu if it exists
		copyMenu.DestroyMenu();
		copyMenu.m_hMenu = NULL;
	}

	if(sSelectedUser.empty()) {

		if(!sSelectedIP.empty()) {
			menu.InsertSeparatorFirst(sSelectedIP);
			menu.AppendMenu(MF_STRING, IDC_WHOIS_IP, (CTSTRING(WHO_IS) + sSelectedIP).c_str() );
			if (client && client->isOp()) {
				menu.AppendMenu(MF_SEPARATOR);
				menu.AppendMenu(MF_STRING, IDC_BAN_IP, (_T("!banip ") + sSelectedIP).c_str());
				menu.SetMenuDefaultItem(IDC_BAN_IP);
				menu.AppendMenu(MF_STRING, IDC_UNBAN_IP, (_T("!unban ") + sSelectedIP).c_str());
				menu.AppendMenu(MF_SEPARATOR);
			}
		} else {
			menu.InsertSeparatorFirst(_T("Text"));
		}

		menu.AppendMenu(MF_STRING, ID_EDIT_COPY, CTSTRING(COPY));
		menu.AppendMenu(MF_STRING, IDC_COPY_ACTUAL_LINE,  CTSTRING(COPY_LINE));

		if(!sSelectedURL.empty()) 
  			menu.AppendMenu(MF_STRING, IDC_COPY_URL, CTSTRING(COPY_URL));
	} else {
		bool isMe = (sSelectedUser == Text::toT(client->getMyNick()));

		// click on nick
		copyMenu.CreatePopupMenu();
		copyMenu.InsertSeparatorFirst(TSTRING(COPY));
		copyMenu.AppendMenu(MF_STRING, IDC_COPY_NICK, CTSTRING(COPY_NICK));
		copyMenu.AppendMenu(MF_STRING, IDC_COPY_EXACT_SHARE, CTSTRING(COPY_EXACT_SHARE));
		copyMenu.AppendMenu(MF_STRING, IDC_COPY_DESCRIPTION, CTSTRING(COPY_DESCRIPTION));
		copyMenu.AppendMenu(MF_STRING, IDC_COPY_TAG, CTSTRING(COPY_TAG));
		copyMenu.AppendMenu(MF_STRING, IDC_COPY_EMAIL_ADDRESS, CTSTRING(COPY_EMAIL_ADDRESS));
		copyMenu.AppendMenu(MF_STRING, IDC_COPY_IP, CTSTRING(COPY_IP));

		menu.InsertSeparatorFirst(sSelectedUser);

		if(BOOLSETTING(LOG_PRIVATE_CHAT)) {
			menu.AppendMenu(MF_STRING, IDC_OPEN_USER_LOG,  CTSTRING(OPEN_USER_LOG));
			menu.AppendMenu(MF_SEPARATOR);
		}		

		menu.AppendMenu(MF_STRING, IDC_SELECT_USER, CTSTRING(SELECT_USER_LIST));
		menu.AppendMenu(MF_SEPARATOR);
		
		if(!isMe) {
			
			const OnlineUser* ou = client->findUser(Text::fromT(sSelectedUser));
			if (client->isOp() || !ou->getIdentity().isOp()) {
				if(HubFrame::ignoreList.find(ou->getUser()) == HubFrame::ignoreList.end()) {
					menu.AppendMenu(MF_STRING, IDC_IGNORE, CTSTRING(IGNORE_USER));
				} else {    
					menu.AppendMenu(MF_STRING, IDC_UNIGNORE, CTSTRING(UNIGNORE_USER));
				}
				menu.AppendMenu(MF_SEPARATOR);
			}

			menu.AppendMenu(MF_STRING, IDC_PUBLIC_MESSAGE, CTSTRING(SEND_PUBLIC_MESSAGE));
			menu.AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CTSTRING(SEND_PRIVATE_MESSAGE));
			menu.AppendMenu(MF_SEPARATOR);
		}
		
		menu.AppendMenu(MF_POPUP, (UINT)(HMENU)copyMenu, CTSTRING(COPY));
		
		if(!isMe) {
			menu.AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::grantMenu, CTSTRING(GRANT_SLOTS_MENU));
			menu.AppendMenu(MF_SEPARATOR);
			menu.AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
			menu.AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CTSTRING(MATCH_QUEUE));
			menu.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
			
			// add user commands
			prepareMenu(menu, ::UserCommand::CONTEXT_CHAT, client->getHubUrl());
		}

		// default doubleclick action
		switch(SETTING(CHAT_DBLCLICK)) {
        case 0:
			menu.SetMenuDefaultItem(IDC_SELECT_USER);
			break;
        case 1:
			menu.SetMenuDefaultItem(IDC_PUBLIC_MESSAGE);
			break;
        case 2:
			menu.SetMenuDefaultItem(IDC_PRIVATEMESSAGE);
			break;
        case 3:
			menu.SetMenuDefaultItem(IDC_GETLIST);
			break;
        case 4:
			menu.SetMenuDefaultItem(IDC_MATCH_QUEUE);
			break;
        case 6:
			menu.SetMenuDefaultItem(IDC_ADD_TO_FAVORITES);
			break;
		} 
	}

	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, ID_EDIT_SELECT_ALL, CTSTRING(SELECT_ALL));
	menu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR));
	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

	return 0;
}

LRESULT ChatCtrl::onSize(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if(wParam != SIZE_MINIMIZED && HIWORD(lParam) > 0) {
		SCROLLINFO si = { 0 };
		POINT pt = { 0 };

		si.cbSize = sizeof(si);
		si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
		GetScrollInfo(SB_VERT, &si);
		GetScrollPos(&pt);

		if (si.nPage == 0 || (size_t)si.nPos >= (size_t)si.nMax - si.nPage - 5)
			PostMessage(EM_SCROLL, SB_BOTTOM, 0);
		else
			SetScrollPos(&pt);

		InvalidateRect(NULL);
	}

	bHandled = FALSE;
	return 0;
}

LRESULT ChatCtrl::onClientEnLink(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	ENLINK* pEL = (ENLINK*)pnmh;

	if ( pEL->msg == WM_LBUTTONUP ) {
		long lBegin = pEL->chrg.cpMin, lEnd = pEL->chrg.cpMax;
		TCHAR* sURLTemp = new TCHAR[(lEnd - lBegin)+1];
		if(sURLTemp) {
			GetTextRange(lBegin, lEnd, sURLTemp);
			tstring sURL = sURLTemp;

			WinUtil::openLink(sURL);

			delete[] sURLTemp;
		}
	} else if(pEL->msg == WM_RBUTTONUP) {
		sSelectedURL = _T("");
		long lBegin = pEL->chrg.cpMin, lEnd = pEL->chrg.cpMax;
		TCHAR* sURLTemp = new TCHAR[(lEnd - lBegin)+1];
		if(sURLTemp) {
			GetTextRange(lBegin, lEnd, sURLTemp);
			sSelectedURL = sURLTemp;
			delete[] sURLTemp;
		}

		SetSel(lBegin, lEnd);
		InvalidateRect(NULL);
	}

	return 0;
}

LRESULT ChatCtrl::onEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Copy();
	return 0;
}

LRESULT ChatCtrl::onEditSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	SetSelAll();
	return 0;
}

LRESULT ChatCtrl::onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	SetWindowText(Util::emptyStringT.c_str());
	return 0;
}

LRESULT ChatCtrl::onCopyActualLine(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!sSelectedLine.empty()) {
		WinUtil::setClipboard(sSelectedLine);
	}
	return 0;
}

LRESULT ChatCtrl::onBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!sSelectedIP.empty()) {
		tstring s = _T("!banip ") + sSelectedIP;
		client->hubMessage(Text::fromT(s));
	}
	return 0;
}

LRESULT ChatCtrl::onUnBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!sSelectedIP.empty()) {
		tstring s = _T("!unban ") + sSelectedIP;
		client->hubMessage(Text::fromT(s));
	}
	return 0;
}

LRESULT ChatCtrl::onCopyURL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!sSelectedURL.empty()) {
		WinUtil::setClipboard(sSelectedURL);
	}
	return 0;
}

LRESULT ChatCtrl::onWhoisIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!sSelectedIP.empty()) {
 		WinUtil::openLink(_T("http://www.ripe.net/perl/whois?form_type=simple&full_query_string=&searchtext=") + sSelectedIP);
 	}
	return 0;
}

LRESULT ChatCtrl::onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	OnlineUser* ou = client->findUser(Text::fromT(sSelectedUser));
	if(ou) {
	}

	return 0;
}

LRESULT ChatCtrl::onPrivateMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	OnlineUser* ou = client->findUser(Text::fromT(sSelectedUser));
	if(ou)
		PrivateFrame::openWindow(ou->getUser(), client);

	return 0;
}

LRESULT ChatCtrl::onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	OnlineUser* ou = client->findUser(Text::fromT(sSelectedUser));
	if(ou)
		ou->getList();

	return 0;
}

LRESULT ChatCtrl::onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	OnlineUser* ou = client->findUser(Text::fromT(sSelectedUser));
	if(ou)
		ou->matchQueue();

	return 0;
}

LRESULT ChatCtrl::onGrantSlot(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	const OnlineUser* ou = client->findUser(Text::fromT(sSelectedUser));
	if(ou) {
		uint64_t time = 0;
		switch(wID) {
			case IDC_GRANTSLOT:			time = 600; break;
			case IDC_GRANTSLOT_DAY:		time = 3600; break;
			case IDC_GRANTSLOT_HOUR:	time = 24*3600; break;
			case IDC_GRANTSLOT_WEEK:	time = 7*24*3600; break;
			case IDC_UNGRANTSLOT:		time = 0; break;
		}
		
		if(time > 0)
			UploadManager::getInstance()->reserveSlot(ou->getUser(), time);
		else
			UploadManager::getInstance()->unreserveSlot(ou->getUser());
	}

	return 0;
}

LRESULT ChatCtrl::onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	OnlineUser* ou = client->findUser(Text::fromT(sSelectedUser));
	if(ou)
		ou->addFav();

	return 0;
}

LRESULT ChatCtrl::onIgnore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/){
	OnlineUser* ou = client->findUser(Text::fromT(sSelectedUser));
	if(ou)
		HubFrame::ignoreList.insert(ou->getUser());

	return 0;
}

LRESULT ChatCtrl::onUnignore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/){
	OnlineUser* ou = client->findUser(Text::fromT(sSelectedUser));
	if(ou)
		HubFrame::ignoreList.erase(ou->getUser());

	return 0;
}

LRESULT ChatCtrl::onCopyUserInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring sCopy;
	
	const OnlineUser* ou = client->findUser(Text::fromT(sSelectedUser));
	if(ou) {
		switch (wID) {
			case IDC_COPY_NICK:
				sCopy += Text::toT(ou->getNick());
				break;
			case IDC_COPY_EXACT_SHARE:
				sCopy += Util::formatExactSize(ou->getIdentity().getBytesShared());
				break;
			case IDC_COPY_DESCRIPTION:
				sCopy += Text::toT(ou->getIdentity().getDescription());
				break;
			case IDC_COPY_TAG:
				sCopy += Text::toT(ou->getIdentity().getTag());
				break;
			case IDC_COPY_EMAIL_ADDRESS:
				sCopy += Text::toT(ou->getIdentity().getEmail());
				break;
			case IDC_COPY_IP:
				sCopy += Text::toT(ou->getIdentity().getIp());
				break;
		}
	}

	if (!sCopy.empty())
		WinUtil::setClipboard(sCopy);

	return 0;
}

LRESULT ChatCtrl::onReport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	const OnlineUser* ou = client->findUser(Text::fromT(sSelectedUser));
	if(ou)
		ClientManager::getInstance()->reportUser(ou->getUser());

	return 0;
}

LRESULT ChatCtrl::onGetUserResponses(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	const OnlineUser* ou = client->findUser(Text::fromT(sSelectedUser));
	if(ou) {
		try {
			QueueManager::getInstance()->addTestSUR(ou->getUser(), false);
		} catch(const Exception& e) {
			LogManager::getInstance()->message(e.getError());		
		}
	}

	return 0;
}

LRESULT ChatCtrl::onCheckList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	const OnlineUser* ou = client->findUser(Text::fromT(sSelectedUser));
	if(ou) {
		try {
			QueueManager::getInstance()->addList(ou->getUser(), QueueItem::FLAG_CHECK_FILE_LIST);
		} catch(const Exception& e) {
			LogManager::getInstance()->message(e.getError());		
		}
	}
	
	return 0;
}

void ChatCtrl::runUserCommand(UserCommand& uc) {
	StringMap ucParams;

	if(!WinUtil::getUCParams(m_hWnd, uc, ucParams))
		return;

	client->getMyIdentity().getParams(ucParams, "my", true);
	client->getHubIdentity().getParams(ucParams, "hub", false);

	const OnlineUser* ou = client->findUser(Text::fromT(sSelectedUser));
	if(ou->getUser()->isOnline()) {
		StringMap tmp = ucParams;
		ou->getIdentity().getParams(tmp, "user", true);
		client->escapeParams(tmp);
		client->sendUserCmd(Util::formatParams(uc.getCommand(), tmp, false));
	}
}
