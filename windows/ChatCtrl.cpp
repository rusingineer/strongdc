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
#include "ChatCtrl.h"
#include "atlstr.h"
#include "../client/FavoriteManager.h"
#include "AGEmotionSetup.h"

extern CAGEmotionSetup* g_pEmotionsSetup;

#define MAX_EMOTICONS 32

static const TCHAR* Links[] = { _T("http://"), _T("https://"), _T("www."), _T("ftp://"), 
	_T("magnet:?"), _T("dchub://"), _T("irc://"), _T("ed2k://"), _T("mms://"), _T("file://"),
	_T("adc://"), _T("adcs://") };
tstring ChatCtrl::sSelectedLine = Util::emptyStringT;
tstring ChatCtrl::sSelectedIP = Util::emptyStringT;
tstring ChatCtrl::sTempSelectedUser = Util::emptyStringT;

ChatCtrl::ChatCtrl() {
	m_boAutoScroll = true;
	m_pUsers = NULL;
	g_BufTemp = (TCHAR *) calloc(512, sizeof(TCHAR)); 
	g_BufTemplen = 511;
	beforeAppendText = (TCHAR *) calloc(512, sizeof(TCHAR)); 
	afterAppendText = (TCHAR *) calloc(512, sizeof(TCHAR)); 
	AppendTextlen = 511;
}

ChatCtrl::~ChatCtrl() {
	m_pUsers = NULL;
	free(g_BufTemp);
	free(beforeAppendText);
	free(afterAppendText);
}

void ChatCtrl::SetUsers(TypedListViewCtrl<UserInfo, IDC_USERS> *pUsers) {
	m_pUsers = pUsers;
}

void ChatCtrl::AdjustTextSize() {
	if(GetWindowTextLength() > 25000) {
		// We want to limit the buffer to 25000 characters...after that, w95 becomes sad...
		SetRedraw(FALSE);
		SetSel(0, LineIndex(LineFromChar(2000)));
		ReplaceSel(_T(""));
		SetRedraw(TRUE);
	}
}

void ChatCtrl::AppendText(const Identity& i, const tstring& sMyNick, const tstring& sTime, const LPCTSTR sMsg, CHARFORMAT2& cf, bool bUseEmo/* = true*/) {
	SetRedraw(FALSE);
	long lSelBeginSaved, lSelEndSaved;
	GetSel(lSelBeginSaved, lSelEndSaved);
	POINT cr;
	GetScrollPos(&cr);

	long lSelBegin = 0, lSelEnd = 0;

	PARAFORMAT2 pf;
	memset(&pf, 0, sizeof(PARAFORMAT2));
	pf.dwMask = PFM_STARTINDENT; 
	pf.dxStartIndent = 0;

	// Insert TimeStamp and format with default style
	if(!sTime.empty()) {
		lSelEnd = lSelBegin = GetTextLengthEx(GTL_NUMCHARS);
		SetSel(lSelEnd, lSelEnd);
		ReplaceSel(sTime.c_str(), false);
		lSelEnd = GetTextLengthEx(GTL_NUMCHARS);
		SetSel(lSelBegin, lSelEnd - 1);
		SetSelectionCharFormat(WinUtil::m_TextStyleTimestamp);
		SetParaFormat(pf);
	}

	tstring msg = sMsg;
	CAtlString sText;
	tstring sAuthor = Text::toT(i.getNick());
	bool bMyMess = i.getUser() == ClientManager::getInstance()->getMe();
	if(!sAuthor.empty()) {
		unsigned int iLen = 0, iAuthorLen = _tcslen(sAuthor.c_str())+1;
		if(sMsg[0] == _T('*')) iLen = 1;
   		sText = sMsg+iAuthorLen+iLen;
		msg = msg.substr(0, iAuthorLen+iLen);
		lSelEnd = lSelBegin = GetTextLengthEx(GTL_NUMCHARS);
		SetSel(lSelEnd, lSelEnd);
		ReplaceSel(msg.c_str(), false);
		if(bMyMess) {
			SetSel(lSelBegin, lSelBegin+iLen+1);
			SetSelectionCharFormat(WinUtil::m_ChatTextMyOwn);
			SetSel(lSelBegin+iLen+1, lSelBegin+iLen+iAuthorLen);
			SetSelectionCharFormat(WinUtil::m_TextStyleMyNick);
		} else {
			bool isFavorite = FavoriteManager::getInstance()->isFavoriteUser(i.getUser());

			if(BOOLSETTING(BOLD_AUTHOR_MESS) || i.isOp() || isFavorite) {
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
				TCHAR *temp;
                if((temp = _tcschr((wchar_t *)sMsg+1, _T('>'))) != NULL) {
                    temp[0] = NULL;
                    int iAuthorLen = _tcslen(sMsg+1)+1;
                    temp[0] = '>';
                    sText = sMsg+iAuthorLen;
		            msg = msg.substr(0, iAuthorLen);
		            lSelEnd = lSelBegin = GetTextLengthEx(GTL_NUMCHARS);
		            SetSel(lSelEnd, lSelEnd);
            		ReplaceSel(msg.c_str(), false);
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
				TCHAR *temp;
                if(sMsg[1] == _T(' ') && (temp = _tcschr((wchar_t *)sMsg+2, _T(' '))) != NULL) {
                    temp[0] = NULL;
                    int iAuthorLen = _tcslen(sMsg+2)+1;
                    temp[0] = ' ';
                    sText = sMsg+iAuthorLen+1;
		            msg = msg.substr(0, iAuthorLen+1);
		            lSelEnd = lSelBegin = GetTextLengthEx(GTL_NUMCHARS);
		            SetSel(lSelEnd, lSelEnd);
            		ReplaceSel(msg.c_str(), false);
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

	long lMask = GetEventMask();
	SetEventMask(lMask | ENM_LINK);

	// Cachry machry, aby tam byly vzdy stejne oddelovace radku, 
	//   nejlip se osvedcil pouze "\n", jinak ujizdi obarveni nicku
	sText.Replace(_T("\r\n"), _T("\n"));
	sText.Replace(_T("\n\r"), _T("\n"));
	sText.Replace(_T("\r"), _T("\n"));

	sText += "\n";
	int len = sText.GetLength();
	if(len > AppendTextlen) {
		beforeAppendText = (TCHAR *) realloc(beforeAppendText, (len+1) * sizeof(TCHAR));
		afterAppendText = (TCHAR *) realloc(afterAppendText, (len+1) * sizeof(TCHAR));
		AppendTextlen = len;
	}

	// cachry machry a maj s toho vylizt smajlove
	if(g_pEmotionsSetup->getUseEmoticons() && BOOLSETTING(USE_EMOTICONS) && bUseEmo) {
		CAGEmotion::List& Emoticons = g_pEmotionsSetup->EmotionsList;
		int smiles = 0; int nIdxFound = -1;
		while(true) {
			TCHAR Delimiter[1024] = { NULL };
			TCHAR *rpl = NULL;
			CAGEmotion::Ptr pFoundedEmotion = NULL;
			len = sText.GetLength();
			for(CAGEmotion::Iter pEmotion = Emoticons.begin(); pEmotion != Emoticons.end(); ++pEmotion) {
				nIdxFound = -1;
				TCHAR *txt = Util::strstr(sText, Text::toT((*pEmotion)->GetEmotionText()).c_str(), &nIdxFound);
				if((txt < rpl && txt) || !rpl && txt) {
					if(len > nIdxFound) {
						rpl = txt;
						pFoundedEmotion = (*pEmotion);
						_tcscpy(Delimiter, Text::toT((*pEmotion)->GetEmotionText()).c_str());
						len = nIdxFound;
					}
				}
			}

			if(rpl && (smiles < MAX_EMOTICONS)) {
				TCHAR *cmp = (TCHAR*)_tcsstr(sText, Delimiter);
				if(cmp) {
					_tcsncpy(beforeAppendText, sText, cmp - sText);
					beforeAppendText[cmp - sText] = 0;
					TCHAR *out = cmp + _tcslen(Delimiter); 
					_tcscpy(afterAppendText, out);
				}
				AppendTextOnly(sMyNick, beforeAppendText, cf, bMyMess, sAuthor);
				COLORREF clrBkColor = WinUtil::m_ChatTextGeneral.crBackColor;
				if(bMyMess) clrBkColor = WinUtil::m_ChatTextMyOwn.crBackColor;
				HBITMAP hbmNext = pFoundedEmotion->GetEmotionBmp(clrBkColor);
				lSelEnd = GetTextLengthEx(GTL_NUMCHARS);
				SetSel(lSelEnd, lSelEnd);
				CImageDataObject::InsertBitmap(GetIRichEditOle(), hbmNext);
				sText = afterAppendText;
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
	SetScrollPos(&cr);
	EndRedrawAppendTextOnly();
}

void ChatCtrl::AppendTextOnly(const tstring& sMyNick, const LPCTSTR sText, CHARFORMAT2& cf, bool bMyMess, const tstring& sAuthor) {
	long lSelBegin = 0, lSelEnd = 0;

	PARAFORMAT2 pf;
	memset(&pf, 0, sizeof(PARAFORMAT2));
	pf.dwMask = PFM_STARTINDENT; 
	pf.dxStartIndent = 0;

	// Insert text at the end
	lSelEnd = lSelBegin = GetTextLengthEx(GTL_NUMCHARS);
	SetSel(lSelEnd, lSelEnd);
	ReplaceSel(sText, false);

	// Set text format
	long lMyNickStart = -1, lMyNickEnd = -1;
	CAtlString sMsgLower = sText;
	sMsgLower.MakeLower();
	CAtlString sNick = sMyNick.c_str();
	if(bMyMess) {
		// Moje vlastni zprava
		lSelEnd = GetTextLengthEx(GTL_NUMCHARS);
		SetSel(lSelBegin, lSelEnd);
		SetSelectionCharFormat(WinUtil::m_ChatTextMyOwn);
	} else {
		lSelEnd = GetTextLengthEx(GTL_NUMCHARS);
		SetSel(lSelBegin, lSelEnd);
		SetSelectionCharFormat(cf);
	}
	
	// Zvyrazneni vsech URL a nastaveni "klikatelnosti"
	lSelEnd = GetTextLengthEx(GTL_NUMCHARS);
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
	lSelEnd = GetTextLengthEx(GTL_NUMCHARS);
	lSearchFrom = 0;
	sNick.MakeLower();

	while(true) {
		lMyNickStart = sMsgLower.Find(sNick, lSearchFrom);
		if(lMyNickStart < 0) 
			break;

		lMyNickEnd = lMyNickStart + sNick.GetLength();
		SetSel(lSelBegin + lMyNickStart, lSelBegin + lMyNickEnd);
		SetSelectionCharFormat(WinUtil::m_TextStyleMyNick);
		lSearchFrom = lMyNickEnd;

		if(	!SETTING(CHATNAMEFILE).empty() && !BOOLSETTING(SOUNDS_DISABLED) &&
			!sAuthor.empty() && (Util::stricmp(sAuthor, sMyNick) != 0)) {
				PlaySound(Text::toT(SETTING(CHATNAMEFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);	 	
        }
	}

	// Zvyrazneni vsech vyskytu nicku Favorite useru
	lSelEnd = GetTextLengthEx(GTL_NUMCHARS);
	FavoriteManager::FavoriteMap ul = FavoriteManager::getInstance()->getFavoriteUsers();
	for(FavoriteManager::FavoriteMap::const_iterator i = ul.begin(); i != ul.end(); ++i) {
		FavoriteUser pUser = i->second;

		lSearchFrom = 0;
		sNick = pUser.getNick().c_str();
		sNick.MakeLower();

		while(true) {
			lMyNickStart = sMsgLower.Find(sNick, lSearchFrom);
			if(lMyNickStart < 0) 
				break;

			lMyNickEnd = lMyNickStart + sNick.GetLength();
			SetSel(lSelBegin + lMyNickStart, lSelBegin + lMyNickEnd);
			SetSelectionCharFormat(WinUtil::m_TextStyleFavUsers);
			lSearchFrom = lMyNickEnd;
		}
	}
}

void ChatCtrl::EndRedrawAppendTextOnly() {
	// Force window to redraw
	SetRedraw(TRUE);
	InvalidateRect(NULL);
	GoToEnd();
}

IRichEditOle* ChatCtrl::GetIRichEditOle() const {
	IRichEditOle *pRichItem = NULL;
	::SendMessage(m_hWnd, EM_GETOLEINTERFACE, 0, (LPARAM)&pRichItem);
	return pRichItem;
}

LRESULT ChatCtrl::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if(m_boAutoScroll) {
        SetRedraw(FALSE);
		ScrollCaret();
		SetSel(0, 0);
		this->CRichEditCtrl::SendMessage(EM_SCROLLCARET);
		SetSel(-1, -1);
		this->CRichEditCtrl::SendMessage(EM_SCROLLCARET);
		SetRedraw(TRUE);
	}
    InvalidateRect(NULL);
	return 1;
}

bool ChatCtrl::HitNick(POINT p, CAtlString *sNick, int *piBegin, int *piEnd) {
	if(!m_pUsers) 
		return FALSE;

	int iCharPos = CharFromPos(p), line = LineFromChar(iCharPos), len = LineLength(iCharPos) + 1;
	long lSelBegin = 0, lSelEnd = 0;
	if(len < 3)
		return 0;

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

	if(len > g_BufTemplen) {
		g_BufTemp = (TCHAR *) realloc(g_BufTemp, (len+1) * sizeof(TCHAR));
		g_BufTemplen = len;
	}
	GetTextRange(lSelBegin, lSelEnd, g_BufTemp);
	g_BufTemp[len] = 0;
	CAtlString sText = g_BufTemp;

	int iLeft = 0, iRight = 0, iCRLF = sText.GetLength(), iPos = sText.Find('<');
	if(iPos >= 0) {
		iLeft = iPos + 1;
		iPos = sText.Find('>', iLeft);
		if(iPos < 0) 
			return false;
		iRight = iPos - 1;
	iCRLF = iRight - iLeft + 1;
	} else {
		iLeft = 0;
	}

	CAtlString sN = sText.Mid(iLeft, iCRLF);
	if(sN.GetLength() == 0)
		return false;

	if(m_pUsers->findItem((tstring)sN) >= 0) {
		*sNick = sN;
		if(piBegin && piEnd) {
			*piBegin = lSelBegin + iLeft;
			*piEnd = lSelBegin + iLeft + iCRLF;
		}
		return true;
	}
    
	// Jeste pokus odmazat eventualni koncovou ':' nebo '>' 
	// Nebo pro obecnost posledni znak 
	// A taky prvni znak 
	// A pak prvni i posledni :-)
	if(iCRLF > 1) {
		sN = sText.Mid(iLeft, iCRLF - 1);
		if(m_pUsers->findItem((tstring)sN) >= 0) {
			*sNick = sN;
       		if(piBegin && piEnd) {
       			*piBegin = lSelBegin + iLeft;
       			*piEnd = lSelBegin + iLeft + iCRLF - 1;
        		}
			return true;
		}

		sN = sText.Mid(iLeft + 1, iCRLF - 1);
		if(m_pUsers->findItem((tstring)sN) >= 0) {
        	*sNick = sN;
       		if(piBegin && piEnd) {
				*piBegin = lSelBegin + iLeft + 1;
				*piEnd = lSelBegin + iLeft + iCRLF;
        		}
			return true;
		}

		sN = sText.Mid(iLeft + 1, iCRLF - 2);
		if(m_pUsers->findItem((tstring)sN) >= 0) {
			*sNick = sN;
       			if(piBegin && piEnd) {
       				*piBegin = lSelBegin + iLeft + 1;
   					*piEnd = lSelBegin + iLeft + iCRLF - 1;
        		}
			return true;
		}
	}	
	return false;
}

bool ChatCtrl::HitIP(POINT p, CAtlString *sIP, int *piBegin, int *piEnd) {
	int iCharPos = CharFromPos(p), len = LineLength(iCharPos) + 1;
	if(len < 3)
		return false;

	DWORD lPosBegin = FindWordBreak(WB_LEFT, iCharPos);
	DWORD lPosEnd = FindWordBreak(WB_RIGHTBREAK, iCharPos);
	len = lPosEnd - lPosBegin;
	if(len > g_BufTemplen) {
		g_BufTemp = (TCHAR *) realloc(g_BufTemp, (len+1) * sizeof(TCHAR));
		g_BufTemplen = len;
	}
	GetTextRange(lPosBegin, lPosEnd, g_BufTemp);
	g_BufTemp[len] = 0;
	for(int i = 0; i < len; i++) {
		if(!((g_BufTemp[i] == 0) || (g_BufTemp[i] == '.') || ((g_BufTemp[i] >= '0') && (g_BufTemp[i] <= '9')))) {
			return false;
		}
	}
	CAtlString sText = g_BufTemp;

	sText.ReleaseBuffer();
	sText.TrimLeft();
	sText.TrimRight();
	sText = sText + '.';
	int iFindBegin = 0, iPos = -1, iEnd2 = 0;
	bool boOK = true;
	for(int i = 0; i < 4; i++) {
		iPos = sText.Find('.', iFindBegin);
		if(iPos < 0) {
			boOK = false;
			break;
		}
		iEnd2 = atoi(Text::fromT((tstring)sText.Mid(iFindBegin)).c_str());
		if((iEnd2 < 0) || (iEnd2 > 255)) {
			boOK = false;
			break;
		}
		iFindBegin = iPos + 1;
	}

	if(boOK) {
		*sIP = sText.Mid(0, iPos);
		if(piBegin && piEnd) {
			*piBegin = lPosBegin;
			*piEnd = lPosEnd;
		}
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

tstring ChatCtrl::LineFromPos(POINT p) {
	int iCharPos = CharFromPos(p), line = LineFromChar(iCharPos), len = LineLength(iCharPos) + 1;
	if(len < 3) {
		return _T("");
	}
	if(len > g_BufTemplen) {
		g_BufTemp = (TCHAR *) realloc(g_BufTemp, (len+1) * sizeof(TCHAR));
		g_BufTemplen = len;
	}
	GetLine(line, g_BufTemp, len);
	tstring x(g_BufTemp, len-1);
	return x;
}

void ChatCtrl::GoToEnd() {
/** TODO make auto-scroll dependent on scrollbar position
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
	GetScrollInfo(SB_VERT, &si);

	if (si.nPos >= si.nMax - si.nPage- 14)
*/
	if(m_boAutoScroll)
		PostMessage(EM_SCROLL, SB_BOTTOM, 0);
}

bool ChatCtrl::GetAutoScroll() {
	return m_boAutoScroll;
}

void ChatCtrl::SetAutoScroll(bool boAutoScroll) {
	m_boAutoScroll = boAutoScroll;
	 if(boAutoScroll)
		GoToEnd();
}

LRESULT ChatCtrl::OnRButtonDown(POINT pt) {
	long lSelBegin = 0, lSelEnd = 0; CAtlString sSel;

	sSelectedLine = LineFromPos(pt);
	sTempSelectedUser = _T("");
	sSelectedIP = _T("");

	// Po kliku dovnitr oznaceneho textu si zkusime poznamenat pripadnej nick ci ip...
	// jinak by nam to neuznalo napriklad druhej klik na uz oznaceny nick =)
	GetSel(lSelBegin, lSelEnd);
	int iCharPos = CharFromPos(pt), iBegin = 0, iEnd = 0;
	if((lSelEnd > lSelBegin) && (iCharPos >= lSelBegin) && (iCharPos <= lSelEnd)) {
		if(HitIP(pt, &sSel, &iBegin, &iEnd)) {
			sSelectedIP = sSel;
		} else if(HitNick(pt, &sSel, &iBegin, &iEnd)) {
			sTempSelectedUser = sSel;
		}
		return 1;
	}

	// Po kliku do IP oznacit IP
	if(HitIP(pt, &sSel, &iBegin, &iEnd)) {
		sSelectedIP = sSel;
		SetSel(iBegin, iEnd);
		InvalidateRect(NULL);
	// Po kliku na Nick oznacit Nick
	} else if(HitNick(pt, &sSel, &iBegin, &iEnd)) {
		sTempSelectedUser = sSel;
		SetSel(iBegin, iEnd);
		InvalidateRect(NULL);
	}
	return 1;
}
