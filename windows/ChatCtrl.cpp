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
#include "../client/HubManager.h"
#include "AGEmotionSetup.h"

extern CAGEmotionSetup* g_pEmotionsSetup;

static const TCHAR* Links[] = { _T("http://"), _T("https://"), _T("www."), _T("ftp://"), 
_T("magnet:?"), _T("dchub://"), _T("irc://"), _T("ed2k://"), _T("file://") };
tstring ChatCtrl::sSelectedLine = Util::emptyStringT;
tstring ChatCtrl::sSelectedIP = Util::emptyStringT;
tstring ChatCtrl::sTempSelectedUser = Util::emptyStringT;

ChatCtrl::ChatCtrl() {
	m_boAutoScroll = true;
	m_pUsers = NULL;
	g_BufTemp = (TCHAR *) calloc(1024, sizeof(TCHAR)); 
	g_BufTemplen = 1023;
	beforeAppendText = (TCHAR *) calloc(1024, sizeof(TCHAR)); 
	afterAppendText = (TCHAR *) calloc(1024, sizeof(TCHAR)); 
	AppendTextlen = 1023;
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

void ChatCtrl::AdjustTextSize(LPCTSTR lpstrTextToAdd) {
	CAtlString sText = lpstrTextToAdd;

	if((GetTextLengthEx(GTL_PRECISE) + sText.GetLength()) > 25000) {
		GetSel(lSelBeginSaved, lSelEndSaved);
		int line = LineFromChar(2000), iFindBegin = LineIndex(line);
		SetSel(0, iFindBegin);
		ReplaceSel(_T(""));
	}
	SendMessage(EM_SETEVENTMASK, 0, (LPARAM)ENM_MOUSEEVENTS);
}

void ChatCtrl::AppendText(LPCTSTR sMyNick, LPCTSTR sTime, LPCTSTR sMsg, CHARFORMAT2& cf, LPCTSTR sAuthor) {
	myMess = false;
	tstring msg = sMsg;
	CAtlString sText = _tcschr(sMsg+_tcslen(sAuthor), ' ');
	msg = msg.substr(0, (msg.length()-sText.GetLength()));

	SetRedraw(FALSE);

	long lMask = GetEventMask();
	SetEventMask(lMask | ENM_LINK);

	int iEnd1 = sText.GetLength(), iBegin = 0, iEnd2 = 0;

	// Cachry machry, aby tam byly vzdy stejne oddelovace radku, 
	//   nejlip se osvedcil pouze "\n", jinak ujizdi obarveni nicku
	sText.Replace(_T("\r\n"), _T("\n"));
	sText.Replace(_T("\n\r"), _T("\n"));
	sText.Replace(_T("\r"), _T("\n"));

	int iCRLF = sText.Find(_T("\n"), 0), iCRLF_Len = 0;
	if(iCRLF >= 0) {
		iCRLF_Len = 1;
		iEnd1 = iCRLF;
		iBegin = iCRLF + 1;
		iEnd2 = sText.GetLength();
	}
	if(iCRLF < 0) {
		iCRLF = sText.GetLength();
		iCRLF_Len = 1;
	}

	sText += "\n";
	int len = sText.GetLength();
	if(len > AppendTextlen) {
		beforeAppendText = (TCHAR *) realloc(beforeAppendText, (len+1) * sizeof(TCHAR));
		afterAppendText = (TCHAR *) realloc(afterAppendText, (len+1) * sizeof(TCHAR));
		AppendTextlen = len;
	}

	AppendTextOnly(sMyNick, sTime, msg.c_str(), cf, sAuthor, false); // maybe ugly but no emoticon in author nick ;-)
	// cachry machry a maj s toho vylizt smajlove
	if(g_pEmotionsSetup->getUseEmoticons() && BOOLSETTING(USE_EMOTICONS)) {
		bool bMyMessage = (sMyNick == sAuthor);

		bool bRedrawControl = false;
		CAGEmotion::List& Emoticons = g_pEmotionsSetup->EmotionsList;
		int smiles = 0; int nIdxFound = -1, maxsmiles = SETTING(MAX_EMOTICONS);
		for(;;) {
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

			if(rpl && (smiles < maxsmiles)) {
				bRedrawControl = true;
				TCHAR *cmp = _tcsstr(sText, Delimiter);
				if(cmp) {
					_tcsncpy(beforeAppendText, sText, cmp - sText);
					beforeAppendText[cmp - sText] = 0;
					TCHAR *out = cmp + _tcslen(Delimiter); 
					_tcscpy(afterAppendText, out);
				}
				AppendTextOnly(sMyNick, _T(""), beforeAppendText, cf, sAuthor, !bRedrawControl);
				COLORREF clrBkColor = WinUtil::m_ChatTextGeneral.crBackColor;
				if(bMyMessage)
					clrBkColor = WinUtil::m_ChatTextMyOwn.crBackColor;
				HBITMAP hbmNext = pFoundedEmotion->GetEmotionBmp(clrBkColor);
				AppendBitmap(hbmNext);
				sText = afterAppendText;
				smiles++;
			} else {
				if(_tcslen(sText) > 0) {
					AppendTextOnly(sMyNick, _T(""), sText, cf, sAuthor, !bRedrawControl);
				}
				break;
			}
		}
		if(bRedrawControl) {
			EndRedrawAppendTextOnly();
		}
	} else {
		AppendTextOnly(sMyNick, _T(""), sText, cf, sAuthor);
	}
}

void ChatCtrl::AppendTextOnly(LPCTSTR sMyNick, LPCTSTR sTime, LPCTSTR sText, CHARFORMAT2& cf, LPCTSTR sAuthor , bool bRedrawControlAtEnd) {
	bool boOK = false;
	long lSelBegin = 0, lSelEnd = 0;

	GetSel(lSelBeginSaved, lSelEndSaved);
	POINT cr;
	SendMessage(EM_GETSCROLLPOS, 0, (LPARAM)&cr);

	PARAFORMAT2 pf;
	memset(&pf, 0, sizeof(PARAFORMAT2));
	pf.dwMask = PFM_STARTINDENT; 
	pf.dxStartIndent = 0;

	// Insert TimeStamp and format with default style
	if((sTime != 0)&& (_tcslen(sTime) > 0)) {
		lSelEnd = lSelBegin = GetTextLengthEx(GTL_PRECISE);
		SetSel(lSelEnd, lSelEnd);
		ReplaceSel(sTime, false);
		lSelEnd = GetTextLengthEx(GTL_PRECISE);
		SetSel(lSelBegin, lSelEnd - 1);
		boOK = SetSelectionCharFormat(WinUtil::m_TextStyleTimestamp);
		boOK = SetParaFormat(pf);
	}

	// Insert text at the end
	lSelEnd = lSelBegin = GetTextLengthEx(GTL_PRECISE);
	SetSel(lSelEnd, lSelEnd);
	ReplaceSel(sText, false);

	// Set text format
	long lMyNickStart = -1, lMyNickEnd = -1;
	CAtlString sMsgLower = sText;
	sMsgLower.MakeLower();
	CAtlString sNick = sMyNick;
	if(sNick == sAuthor || myMess == true) {
		// Moje vlastni zprava
		lSelEnd = GetTextLengthEx(GTL_PRECISE);
		SetSel(lSelBegin, lSelEnd);
		boOK = SetSelectionCharFormat(WinUtil::m_ChatTextMyOwn);
		myMess = true;
	} else {
		lSelEnd = GetTextLengthEx(GTL_PRECISE);
		SetSel(lSelBegin, lSelEnd);
		boOK = SetSelectionCharFormat(cf);
		myMess = false;
	}

	// Zvyrazneni nicku autora zpravy ci OPicaka ;o)
	if(!myMess) { // don't waste cpu time, it's my mess and not need to set another author style :-P
		if(_tcslen(sAuthor) > 0) {
			int i = -1;
			if(m_pUsers != NULL) // fix for pm windows without userlist :-(
				i = m_pUsers->findItem(sAuthor);
			bool isOP = false;
			if(i != -1) {
				UserInfo* ui = m_pUsers->getItemData(i);
				isOP = ui->getOp();
			}
			if(BOOLSETTING(BOLD_AUTHOR_MESS) || isOP) {
  				CAtlString sAuthorNick = sAuthor;
  			sAuthorNick.MakeLower();
				long lAuthorBegin = sMsgLower.Find(sAuthorNick, 0);
    			if(lAuthorBegin > 0) {
					long lAuthorEnd = lAuthorBegin + sAuthorNick.GetLength();
					SetSel(lSelBegin + lAuthorBegin, lSelBegin + lAuthorEnd);
					if(isOP)
						boOK = SetSelectionCharFormat(WinUtil::m_TextStyleOPs);
					else
						boOK = SetSelectionCharFormat(WinUtil::m_TextStyleBold);
			}
		}
	}
	}
	
	// Zvyrazneni vsech URL a nastaveni "klikatelnosti"
	lSelEnd = GetTextLengthEx(GTL_PRECISE);
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
			boOK = SetSelectionCharFormat(WinUtil::m_TextStyleURL);
			linkStart = sMsgLower.Find(Links[i], linkEnd);
		}
	}

	// Zvyrazneni vsech vyskytu vlastniho nicku
	lSelEnd = GetTextLengthEx(GTL_PRECISE);
	lSearchFrom = 0;
	sNick.MakeLower();

	while(true) {
		lMyNickStart = sMsgLower.Find( sNick, lSearchFrom );
		if ( lMyNickStart < 0 ) 
			break;

		lMyNickEnd = lMyNickStart + sNick.GetLength();
		SetSel(lSelBegin + lMyNickStart, lSelBegin + lMyNickEnd);
		boOK = SetSelectionCharFormat(WinUtil::m_TextStyleMyNick);
		lSearchFrom = lMyNickEnd;
		CAtlString autor = sAuthor;
		autor.MakeLower();
		if( ( sNick != autor ) && (autor != "")) {
	        if ((!SETTING(CHATNAMEFILE).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
		        PlaySound(Text::toT(SETTING(CHATNAMEFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);	 	
        }
	}

	// Zvyrazneni vsech vyskytu nicku Favorite useru
	lSelEnd = GetTextLengthEx(GTL_PRECISE);
	User::List ul = HubManager::getInstance()->getFavoriteUsers();
	for(User::Iter i = ul.begin(); i != ul.end(); ++i) {
		User::Ptr pUser = *i;
		string sU = "";

		lSearchFrom = 0;
		sNick = pUser->getNick().c_str();
		sNick.MakeLower();

		while(true) {
			lMyNickStart = sMsgLower.Find(sNick, lSearchFrom);
			if(lMyNickStart < 0) 
				break;

			lMyNickEnd = lMyNickStart + sNick.GetLength();
			SetSel(lSelBegin + lMyNickStart, lSelBegin + lMyNickEnd);
			boOK = SetSelectionCharFormat(WinUtil::m_TextStyleFavUsers);
			lSearchFrom = lMyNickEnd;
		}
	}

	SetSel(lSelBeginSaved, lSelEndSaved);
	SendMessage(EM_SETSCROLLPOS, 0, (LPARAM)&cr);

	if(bRedrawControlAtEnd){
		EndRedrawAppendTextOnly();
		}
}

void ChatCtrl::EndRedrawAppendTextOnly() {
	// Force window to redraw
	SetRedraw(TRUE);
	InvalidateRect(NULL);
	GoToEnd();
}

void ChatCtrl::AppendBitmap(HBITMAP hbm) {
	POINT cr;
	SendMessage(EM_GETSCROLLPOS, 0, (LPARAM)&cr);
	GetSel(lSelBeginSaved, lSelEndSaved);
	
	// Insert text at the end
	long lSelEnd = GetTextLengthEx(GTL_PRECISE);
	SetSel(lSelEnd, lSelEnd);
	CImageDataObject::InsertBitmap(GetIRichEditOle(), hbm);

	SetSel(lSelBeginSaved, lSelEndSaved);
	SendMessage(EM_SETSCROLLPOS, 0, (LPARAM)&cr);
}

IRichEditOle* ChatCtrl::GetIRichEditOle() const {
	IRichEditOle *pRichItem = NULL;
	::SendMessage(m_hWnd, EM_GETOLEINTERFACE, 0, (LPARAM)&pRichItem);
	return pRichItem;
}

LRESULT ChatCtrl::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if(m_boAutoScroll) {
		InvalidateRect(NULL);
		ScrollCaret();
		this->CRichEditCtrl::SendMessage(EM_SCROLL, SB_BOTTOM, 0);
	}
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

bool ChatCtrl::HitURL(POINT p) {
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

string ChatCtrl::LineFromPos(POINT p) {
	int iCharPos = CharFromPos(p), line = LineFromChar(iCharPos), len = LineLength(iCharPos) + 1;
	if(len < 3) {
		return "";
	}
	if(len > g_BufTemplen) {
		g_BufTemp = (TCHAR *) realloc(g_BufTemp, (len+1) * sizeof(TCHAR));
		g_BufTemplen = len;
	}
	GetLine(line, g_BufTemp, len);
	tstring x(g_BufTemp, len-1);
	return Text::fromT(x);
}

void ChatCtrl::GoToEnd() {
	if(m_boAutoScroll) {
		this->CRichEditCtrl::SendMessage(EM_SCROLL, SB_BOTTOM, 0);
	}
}

bool ChatCtrl::GetAutoScroll() {
	return m_boAutoScroll;
}

void ChatCtrl::SetAutoScroll(bool boAutoScroll) {
	m_boAutoScroll = boAutoScroll;
	 if(boAutoScroll)
		GoToEnd();
}

LRESULT ChatCtrl::OnRButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	long lSelBegin = 0, lSelEnd = 0;

	// Po kliku dovnitr oznaceneho textu nedelat nic
	sSelectedLine = Text::toT(LineFromPos(pt));
	sTempSelectedUser = _T("");
	sSelectedIP = _T("");

	GetSel(lSelBegin, lSelEnd);
	int iCharPos = CharFromPos(pt), iBegin = 0, iEnd1 = 0;
	if((lSelEnd > lSelBegin) && (iCharPos >= lSelBegin) && (iCharPos <= lSelEnd)) {
		return 1;
	}
	// Po kliku do IP oznacit IP
	CAtlString sSel;
	if(HitIP(pt, &sSel, &iBegin, &iEnd1)) {
		sSelectedIP = sSel;
		SetSel(iBegin, iEnd1);
		InvalidateRect(NULL);
	} else if(HitNick(pt, &sSel, &iBegin, &iEnd1)) {
		sTempSelectedUser = sSel;
		SetSel(iBegin, iEnd1);
		InvalidateRect(NULL);
	}
	return 1;
}
