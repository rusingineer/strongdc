#include "stdafx.h"
#include "Resource.h"
#include "../client/DCPlusPlus.h"
#include "ChatCtrl.h"
#include "atlstr.h"
#include "../client/HubManager.h"

#ifndef AGEMOTIONSETUP_H__
#include "AGEmotionSetup.h"
#endif

extern CAGEmotionSetup* g_pEmotionsSetup;

ChatCtrl::ChatCtrl() {
	memset(&m_TextStyleGeneral, 0, sizeof(CHARFORMAT2));
	memset(&m_TextStyleTimestamp, 0, sizeof(CHARFORMAT2));
	memset(&m_TextStyleMyNick, 0, sizeof(CHARFORMAT2));
	memset(&m_TextStyleBold, 0, sizeof(CHARFORMAT2));
	memset(&m_TextStyleFavUsers, 0, sizeof(CHARFORMAT2));
	memset(&m_TextStyleOPUsers, 0, sizeof(CHARFORMAT2));
	ReadSettings();
	m_boAutoScroll = true;
	m_pUsers = NULL;
	g_BufTemp = (char *) calloc(1024, sizeof(char)); 
	g_BufTemplen = 1023;
	beforeAppendText = (char *) calloc(1024, sizeof(char)); 
	afterAppendText = (char *) calloc(1024, sizeof(char)); 
	AppendTextlen = 1023;
}

ChatCtrl::~ChatCtrl() {
	m_pUsers = NULL;
	delete[] g_BufTemp;
	delete[] beforeAppendText;
	delete[] afterAppendText;
}

void ChatCtrl::SetUsers( TypedListViewCtrlCleanup<UserInfo, IDC_USERS> *pUsers ) {
	m_pUsers = pUsers;
}

void ChatCtrl::ReadSettings() {
	memset(&m_TextStyleGeneral, 0, sizeof(CHARFORMAT2));
	m_TextStyleGeneral.cbSize = sizeof(m_TextStyleGeneral);
	m_TextStyleGeneral.dwMask = CFM_COLOR | CFM_BOLD | CFM_ITALIC | CFM_BACKCOLOR;
	m_TextStyleGeneral.dwReserved = 0;
	m_TextStyleGeneral.crBackColor = SETTING(TEXT_GENERAL_BACK_COLOR);
	m_TextStyleGeneral.crTextColor = SETTING(TEXT_GENERAL_FORE_COLOR);
	m_TextStyleGeneral.dwEffects = 0;
	if ( SETTING(TEXT_GENERAL_BOLD) )
		m_TextStyleGeneral.dwEffects |= CFE_BOLD;
	if ( SETTING(TEXT_GENERAL_ITALIC) )
		m_TextStyleGeneral.dwEffects |= CFE_ITALIC;

	memset(&m_TextStyleTimestamp, 0, sizeof(CHARFORMAT2));
	m_TextStyleTimestamp.cbSize = sizeof(m_TextStyleTimestamp);
	m_TextStyleTimestamp.dwMask = CFM_COLOR | CFM_BOLD | CFM_ITALIC | CFM_BACKCOLOR;
	m_TextStyleTimestamp.dwReserved = 0;
	m_TextStyleTimestamp.crBackColor = SETTING(TEXT_TIMESTAMP_BACK_COLOR);
	m_TextStyleTimestamp.crTextColor = SETTING(TEXT_TIMESTAMP_FORE_COLOR);
	m_TextStyleTimestamp.dwEffects = 0;
	if ( SETTING(TEXT_TIMESTAMP_BOLD) )
		m_TextStyleTimestamp.dwEffects |= CFE_BOLD;
	if ( SETTING(TEXT_TIMESTAMP_ITALIC) )
		m_TextStyleTimestamp.dwEffects |= CFE_ITALIC;

	memset(&m_TextStyleMyNick, 0, sizeof(CHARFORMAT2));
	m_TextStyleMyNick.cbSize = sizeof(m_TextStyleMyNick);
	m_TextStyleMyNick.dwMask = CFM_COLOR | CFM_BOLD | CFM_ITALIC | CFM_BACKCOLOR;
	m_TextStyleMyNick.dwReserved = 0;
	m_TextStyleMyNick.crBackColor = SETTING(TEXT_MYNICK_BACK_COLOR);
	m_TextStyleMyNick.crTextColor = SETTING(TEXT_MYNICK_FORE_COLOR);
	m_TextStyleMyNick.dwEffects = 0;
	if ( SETTING(TEXT_MYNICK_BOLD) )
		m_TextStyleMyNick.dwEffects |= CFE_BOLD;
	if ( SETTING(TEXT_MYNICK_ITALIC) )
		m_TextStyleMyNick.dwEffects |= CFE_ITALIC;

	memset(&m_ChatTextMyOwn, 0, sizeof(CHARFORMAT2));
	m_ChatTextMyOwn.cbSize = sizeof(m_ChatTextMyOwn);
	m_ChatTextMyOwn.dwMask = CFM_COLOR | CFM_BOLD | CFM_ITALIC | CFM_BACKCOLOR;
	m_ChatTextMyOwn.dwReserved = 0;
	m_ChatTextMyOwn.crBackColor     = SETTING(TEXT_MYOWN_BACK_COLOR);
	m_ChatTextMyOwn.crTextColor     = SETTING(TEXT_MYOWN_FORE_COLOR);
	m_ChatTextMyOwn.dwEffects = 0;
	if ( SETTING(TEXT_MYOWN_BOLD) )
		m_ChatTextMyOwn.dwEffects       |= CFE_BOLD;
	if ( SETTING(TEXT_MYOWN_ITALIC) )
		m_ChatTextMyOwn.dwEffects       |= CFE_ITALIC;

	memset(&m_TextStyleBold, 0, sizeof(CHARFORMAT2));
	m_TextStyleBold.cbSize = sizeof(m_TextStyleBold);
	m_TextStyleBold.dwMask = CFM_BOLD;
	m_TextStyleBold.dwReserved = 0;
	m_TextStyleBold.crBackColor = 0;
	m_TextStyleBold.crTextColor = 0;
	m_TextStyleBold.dwEffects = CFE_BOLD;

	memset(&m_TextStyleFavUsers, 0, sizeof(CHARFORMAT2));
	m_TextStyleFavUsers.cbSize = sizeof(m_TextStyleFavUsers);
	m_TextStyleFavUsers.dwMask = CFM_COLOR | CFM_BOLD | CFM_ITALIC | CFM_BACKCOLOR;
	m_TextStyleFavUsers.dwReserved = 0;
	m_TextStyleFavUsers.crBackColor = SETTING(TEXT_FAV_BACK_COLOR);
	m_TextStyleFavUsers.crTextColor = SETTING(TEXT_FAV_FORE_COLOR);
	m_TextStyleFavUsers.dwEffects = 0;
	if ( SETTING(TEXT_FAV_BOLD) )
		m_TextStyleFavUsers.dwEffects |= CFE_BOLD;
	if ( SETTING(TEXT_FAV_ITALIC) )
		m_TextStyleFavUsers.dwEffects |= CFE_ITALIC;

	memset(&m_TextStyleOPUsers, 0, sizeof(CHARFORMAT2));
	m_TextStyleOPUsers.cbSize = sizeof(m_TextStyleOPUsers);
	m_TextStyleOPUsers.dwMask = CFM_COLOR | CFM_BOLD | CFM_ITALIC | CFM_BACKCOLOR;
	m_TextStyleOPUsers.dwReserved = 0;
	m_TextStyleOPUsers.crBackColor = SETTING(TEXT_OP_BACK_COLOR);
	m_TextStyleOPUsers.crTextColor = SETTING(TEXT_OP_FORE_COLOR);
	m_TextStyleOPUsers.dwEffects = 0;
	if ( SETTING(TEXT_OP_BOLD) )
		m_TextStyleOPUsers.dwEffects |= CFE_BOLD;
	if ( SETTING(TEXT_OP_ITALIC) )
		m_TextStyleOPUsers.dwEffects |= CFE_ITALIC;
}

void ChatCtrl::AdjustTextSize( LPCTSTR lpstrTextToAdd ) {
	sText = lpstrTextToAdd;

	if( ( GetTextLengthEx( GTL_PRECISE ) + sText.GetLength() ) > 25000 ) {
		GetSel( lSelBeginSaved, lSelEndSaved );
		line = LineFromChar( 2000 );
		iFindBegin = LineIndex( line );
		SetSel( 0, iFindBegin );
		ReplaceSel("");
	}
	SendMessage( EM_SETEVENTMASK, 0, (LPARAM)ENM_MOUSEEVENTS );
}

char *stristr(const char *str1, const char *str2, int *pnIdxFound = NULL) {
	char *s1, *s2;
	char *cp = (char *) str1;
	if ( !*str2 )
		return (char *) str1;
	int nIdx = 0;
	while (*cp)
	{
		s1 = cp;
		s2 = (char *) str2;
		while ( *s1 && *s2 && ( !(*s1-*s2) ||
			!(*s1-tolower(*s2)) || !(*s1-toupper(*s2)) ) )
				s1++, s2++;
		if (!*s2) {
			if (pnIdxFound != NULL)
				*pnIdxFound = nIdx;
			return cp;
		}
		cp++;
		nIdx++;
	}
	if (pnIdxFound != NULL)
		*pnIdxFound = -1;
	return NULL;
}

void strstp(const char *in, char *before, const char *txt, char *after) {
	char *cmp = stristr(in, txt);
	if (!cmp) return;
	strncpy(before, in, cmp - in);
	before[cmp - in] = 0;
	char *out = cmp + strlen(txt); 
	strcpy(after, out);
}


int strrpl(char *lpszBuf, const char *lpszOld, const char *lpszNew) {
	int nSourceLen;
	if (!lpszOld) nSourceLen = 0;
	else nSourceLen = strlen(lpszOld);
	if (nSourceLen == 0)
		return 0;
	int nReplacementLen = strlen(lpszNew);

	int nCount = 0;
	char *lpszStart = lpszBuf;
	char *lpszEnd = lpszBuf + strlen(lpszBuf);
	char *lpszTarget;
	while (lpszStart < lpszEnd) {
		while ((lpszTarget = strstr(lpszStart, lpszOld)) != NULL) {
			nCount++;
			lpszStart = lpszTarget + nSourceLen;
		}
		lpszStart += strlen(lpszStart) + 1;
	}

	if (nCount > 0) {
		int nOldLength = strlen(lpszBuf); 

		lpszStart = lpszBuf;
		lpszEnd = lpszBuf + strlen(lpszBuf); 

		while (lpszStart < lpszEnd)
		{
			while ( (lpszTarget = strstr(lpszStart, lpszOld)) != NULL)
			{
				int nBalance = nOldLength - int(lpszTarget - lpszBuf + nSourceLen);
				memmove(lpszTarget + nReplacementLen, lpszTarget + nSourceLen,
					nBalance * sizeof(char));
				memcpy(lpszTarget, lpszNew, nReplacementLen * sizeof(char));
				lpszStart = lpszTarget + nReplacementLen;
				lpszStart[nBalance] = '\0';
				nOldLength += (nReplacementLen - nSourceLen);
			}
 			lpszStart += strlen(lpszStart) + 1;
		}
	}
	return nCount;
}

void ChatCtrl::AppendText( LPCTSTR sMyNick, LPCTSTR sTime, LPCTSTR sMsg, CHARFORMAT2& cf, LPCTSTR sAuthor ) {
	myMess = false;
	msg = sMsg;
	sText = strchr(sMsg+_tcslen(sAuthor), ' ');
	msg = msg.substr(0, (msg.length()-sText.GetLength()));

	SetRedraw( FALSE );

	lMask = GetEventMask();
	SetEventMask( lMask | ENM_LINK );
	lMask = GetEventMask();

	iEnd1 = sText.GetLength();
	iBegin = 0;
	iEnd2 = 0;

	// Cachry machry, aby tam byly vzdy stejne oddelovace radku, 
	//   nejlip se osvedcil pouze "\n", jinak ujizdi obarveni nicku
	sText.Replace( "\r\n", "\n" );
	sText.Replace( "\n\r", "\n" );
	sText.Replace( "\r", "\n" );

	iCRLF = sText.Find( "\n", 0 );
	iCRLF_Len = 0;
	if ( iCRLF >= 0 ) {
		iCRLF_Len = 1;
		iEnd1 = iCRLF;
		iBegin = iCRLF + 1;
		iEnd2 = sText.GetLength();
	}
	if ( iCRLF < 0 ) {
		iCRLF = sText.GetLength();
		iCRLF_Len = 1;
	}

	sText += "\n";
	len = sText.GetLength();
	if(len > AppendTextlen) {
		beforeAppendText = (char *) realloc(beforeAppendText, len+1);
		afterAppendText = (char *) realloc(afterAppendText, len+1);
		AppendTextlen = len;
	}

	AppendTextOnly(sMyNick, sTime, msg.c_str(), cf, sAuthor, false); // maybe ugly but no emoticon in author nick ;-)
	// cachry machry a maj s toho vylizt smajlove
	if(g_pEmotionsSetup->getUseEmoticons() && BOOLSETTING(USE_EMOTICONS)) {
		bMyMessage = (sMyNick == sAuthor );

		bRedrawControl = false;
		CAGEmotion::List& Emoticons = g_pEmotionsSetup->EmotionsList;
		smiles = 0;
		maxsmiles = SETTING(MAX_EMOTICONS);
		for (;;)
		{
			char Delimiter[1024] = { NULL };
			char *rpl = NULL;
			CAGEmotion::Ptr pFoundedEmotion = NULL;
			len = sText.GetLength();
			for(CAGEmotion::Iter pEmotion = Emoticons.begin(); pEmotion != Emoticons.end(); ++pEmotion) {
				nIdxFound = -1;
				char *txt = stristr(sText, (*pEmotion)->GetEmotionText().c_str(), &nIdxFound);
				if ((txt < rpl && txt) || !rpl && txt)
				{
					if (len > nIdxFound) {
						rpl = txt;
						pFoundedEmotion = (*pEmotion);
						strcpy(Delimiter, (*pEmotion)->GetEmotionText().c_str());

						len = nIdxFound;
					}
				}
			}

			if(rpl && (smiles < maxsmiles)) {
				bRedrawControl = true;
				strstp(sText, beforeAppendText, Delimiter, afterAppendText);
				AppendTextOnly(sMyNick, "", beforeAppendText, cf, sAuthor, !bRedrawControl);

				COLORREF clrBkColor = m_TextStyleGeneral.crBackColor;
				if (bMyMessage)
					clrBkColor = m_ChatTextMyOwn.crBackColor;
				HBITMAP hbmNext = pFoundedEmotion->GetEmotionBmp(clrBkColor);
				AppendBitmap(hbmNext);
				sText = afterAppendText;
				smiles++;
			} else {
				if (strlen(sText) > 0) {
					AppendTextOnly(sMyNick, "", sText, cf, sAuthor, !bRedrawControl);
				}
				break;
			}
		}
		if (bRedrawControl) {
			EndRedrawAppendTextOnly();
		}
	} else {
		AppendTextOnly(sMyNick, "", sText, cf, sAuthor);
	}
}

void ChatCtrl::AppendTextOnly( LPCTSTR sMyNick, LPCTSTR sTime, LPCTSTR sText, CHARFORMAT2& cf, LPCTSTR sAuthor , bool bRedrawControlAtEnd) {
	boOK = false;
	boAtTheEnd = false;

	GetSel( lSelBeginSaved, lSelEndSaved );
	POINT cr;
	SendMessage(EM_GETSCROLLPOS, 0, (LPARAM)&cr);
	lTextLength = GetTextLengthEx( GTL_PRECISE );

	boAtTheEnd = ( lSelEndSaved >= ( lTextLength - 2 ) );

	PARAFORMAT2 pf;
	memset(&pf, 0, sizeof(PARAFORMAT2));
	pf.dwMask = PFM_STARTINDENT; 
	pf.dxStartIndent = 0;

	// Insert TimeStamp and format with default style
	if ((sTime != 0)&& (_tcslen(sTime) > 0)) {
		lSelEnd = lSelBegin = GetTextLengthEx( GTL_PRECISE );
		SetSel( lSelEnd, lSelEnd );
		ReplaceSel( sTime, false );
		lSelEnd = GetTextLengthEx( GTL_PRECISE );
		SetSel( lSelBegin, lSelEnd - 1 );
		boOK = SetSelectionCharFormat( m_TextStyleTimestamp );
		boOK = SetParaFormat( pf );
	}

	// Insert text at the end
	lSelEnd = lSelBegin = GetTextLengthEx( GTL_PRECISE );
	SetSel( lSelEnd, lSelEnd );
	ReplaceSel( sText, false );

	// Set text format
	lMyNickStart = -1;
	lMyNickEnd = -1;
	sMsgLower = sText;
	sMsgLower.MakeLower();
	sNick = sMyNick;
	if(sNick == sAuthor || myMess == true ) {
		// Moje vlastni zprava
		lSelEnd = GetTextLengthEx( GTL_PRECISE );
		SetSel( lSelBegin, lSelEnd );
		boOK = SetSelectionCharFormat( m_ChatTextMyOwn );
		myMess = true;
	} else {
		lSelEnd = GetTextLengthEx( GTL_PRECISE );
		SetSel( lSelBegin, lSelEnd );
		boOK = SetSelectionCharFormat( cf );
		myMess = false;
	}

	// Zvyrazneni nicku autora zpravy ci OPicaka ;o)
	if(!myMess) { // don't waste cpu time, it's my mess and not need to set another author style :-P
		if (_tcslen(sAuthor) > 0) {
			int i = -1;
			if(m_pUsers != NULL) // fix for pm windows without userlist :-(
				i = m_pUsers->findItem(sAuthor);
			isOP = false;
			if(i != -1) {
				UserInfo* ui = m_pUsers->getItemData(i);
				isOP = ui->getOp();
			}
			if(BOOLSETTING(BOLD_AUTHOR_MESS) || isOP) {
  				sAuthorNick = sAuthor;
  			sAuthorNick.MakeLower();
				lAuthorBegin = sMsgLower.Find( sAuthorNick, 0 );
    		if ( lAuthorBegin > 0 ) {
					lAuthorEnd = lAuthorBegin + sAuthorNick.GetLength();
				SetSel( lSelBegin + lAuthorBegin, lSelBegin + lAuthorEnd );
					if(isOP)
						boOK = SetSelectionCharFormat( m_TextStyleOPUsers );
					else
				boOK = SetSelectionCharFormat( m_TextStyleBold );
			}
		}
	}
	}
	
	// Zvyrazneni vsech vyskytu vlastniho nicku
	lSelEnd = GetTextLengthEx( GTL_PRECISE );
			lSearchFrom = 0;
	sNick.MakeLower();

	while ( true ) {
		lMyNickStart = sMsgLower.Find( sNick, lSearchFrom );
		if ( lMyNickStart < 0 ) 
			break;

		lMyNickEnd = lMyNickStart + sNick.GetLength();
		SetSel( lSelBegin + lMyNickStart, lSelBegin + lMyNickEnd );
		boOK = SetSelectionCharFormat( m_TextStyleMyNick );
		lSearchFrom = lMyNickEnd;
		CAtlString autor = sAuthor;
		autor.MakeLower();
		if( ( sNick != autor ) && (autor != "")) {	
	        if ((!SETTING(CHATNAMEFILE).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
		        PlaySound(SETTING(CHATNAMEFILE).c_str(), NULL, SND_FILENAME | SND_ASYNC);	 	
        }
	}

	// Zvyrazneni vsech vyskytu nicku Favorite useru
	lSelEnd = GetTextLengthEx( GTL_PRECISE );
	User::List ul = HubManager::getInstance()->getFavoriteUsers();
	for(User::Iter i = ul.begin(); i != ul.end(); ++i) {
		User::Ptr pUser = *i;
		string sU = "";

		lSearchFrom = 0;
		sNick = pUser->getNick().c_str();
		sNick.MakeLower();

		while ( true ) {
			lMyNickStart = sMsgLower.Find( sNick, lSearchFrom );
			if ( lMyNickStart < 0 ) 
				break;

			lMyNickEnd = lMyNickStart + sNick.GetLength();
			SetSel( lSelBegin + lMyNickStart, lSelBegin + lMyNickEnd );
			boOK = SetSelectionCharFormat( m_TextStyleFavUsers );
			lSearchFrom = lMyNickEnd;
		}
	}

	// Uprava pozadi pro text s bitem CFE_LINK
		lSelEnd = GetTextLengthEx( GTL_PRECISE );
		CHARFORMAT2 cfSel;
		memset(&cfSel, 0, sizeof(CHARFORMAT2));
		cfSel.cbSize = sizeof( cfSel );

	for ( lPos = lSelBegin; lPos < lSelEnd; lPos++ ) {
			SetSel( lPos, lPos + 1 );
			GetSelectionCharFormat( cfSel );
			if ( cfSel.dwEffects & CFE_LINK ) {
				cfSel.crBackColor = SETTING(TEXT_URL_BACK_COLOR);
				cfSel.crTextColor = SETTING(TEXT_URL_FORE_COLOR);
				if ( SETTING(TEXT_URL_BOLD) )
					cfSel.dwEffects |= CFE_BOLD;
				if ( SETTING(TEXT_URL_ITALIC) )
					cfSel.dwEffects |= CFE_ITALIC;
				boOK = SetSelectionCharFormat( cfSel );
			}
		}

		SetSel( lSelBeginSaved, lSelEndSaved );
		SendMessage(EM_SETSCROLLPOS, 0, (LPARAM)&cr);

	if (bRedrawControlAtEnd){
		EndRedrawAppendTextOnly();
		}
}

void ChatCtrl::EndRedrawAppendTextOnly() {
	// Force window to redraw
	SetRedraw( TRUE );
	InvalidateRect( NULL );
	GoToEnd();
}

void ChatCtrl::AppendBitmap(HBITMAP hbm) {
	POINT cr;
	SendMessage(EM_GETSCROLLPOS, 0, (LPARAM)&cr);
	GetSel( lSelBeginSaved, lSelEndSaved );
	
	// Insert text at the end
	lTextLength = GetTextLengthEx( GTL_PRECISE );
	SetSel( lSelEnd, lSelEnd );
	CImageDataObject::InsertBitmap(GetIRichEditOle(), hbm);

	SetSel( lSelBeginSaved, lSelEndSaved );
	SendMessage(EM_SETSCROLLPOS, 0, (LPARAM)&cr);
}

IRichEditOle* ChatCtrl::GetIRichEditOle() const {
	IRichEditOle *pRichItem = NULL;
	::SendMessage(m_hWnd, EM_GETOLEINTERFACE, 0, (LPARAM)&pRichItem);
	return pRichItem;
}

LRESULT ChatCtrl::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if ( m_boAutoScroll ) {
		InvalidateRect( NULL );
		ScrollCaret();
		this->CRichEditCtrl::SendMessage(EM_SCROLL, SB_BOTTOM, 0);
	}
	return 1;
}

bool ChatCtrl::HitNick( POINT p, CAtlString *sNick, int *piBegin, int *piEnd ) {
		if ( !m_pUsers ) 
			return FALSE;

	iCharPos = CharFromPos( p );
	line = LineFromChar( iCharPos );
	len = LineLength( iCharPos ) + 1;
		if ( len < 3 )
			return 0;

		// Metoda FindWordBreak nestaci, protoze v nicku mohou byt znaky povazovane za konec slova
	iFindBegin = LineIndex( line );
	iEnd1 = LineIndex( line ) + LineLength( iCharPos );

	for ( lSelBegin = iCharPos; lSelBegin >= iFindBegin; lSelBegin-- ) {
		if ( FindWordBreak( WB_ISDELIMITER, lSelBegin ) )
				break;
		}
	lSelBegin++;
	for ( lSelEnd = iCharPos; lSelEnd < iEnd1; lSelEnd++ ) {
		if ( FindWordBreak( WB_ISDELIMITER, lSelEnd ) )
				break;
		}

	len = lSelEnd - lSelBegin;
		if ( len <= 0 )
			return false;

	if(len > g_BufTemplen) {
		g_BufTemp = (char *) realloc(g_BufTemp, len+1);
		g_BufTemplen = len;
	}
	iRet = GetTextRange( lSelBegin, lSelEnd, g_BufTemp);
		UNREFERENCED_PARAMETER(iRet);
		g_BufTemp[len] = 0;
	sText = g_BufTemp;

	iLeft = 0, iRight = 0;
	iCRLF = sText.GetLength();

	iPos = sText.Find( '<' );
		if ( iPos >= 0 ) {
			iLeft = iPos + 1;
			iPos = sText.Find( '>', iLeft );
			if ( iPos < 0 ) 
				return false;
			iRight = iPos - 1;
		iCRLF = iRight - iLeft + 1;
		} else {
			iLeft = 0;
		}

	sN = sText.Mid( iLeft, iCRLF );
		if ( sN.GetLength() == 0 )
			return false;

		if ( m_pUsers->findItem( (string) sN) >= 0 ) {
			*sNick = sN;
			if ( piBegin && piEnd ) {
			*piBegin = lSelBegin + iLeft;
			*piEnd = lSelBegin + iLeft + iCRLF;
			}
			return true;
		}
    
		// Jeste pokus odmazat eventualni koncovou ':' nebo '>' 
		// Nebo pro obecnost posledni znak 
		// A taky prvni znak 
		// A pak prvni i posledni :-)
	if ( iCRLF > 1 ) {
		sN = sText.Mid( iLeft, iCRLF - 1 );
			if ( m_pUsers->findItem( (string) sN) >= 0 ) {
				*sNick = sN;
        			if ( piBegin && piEnd ) {
       				*piBegin = lSelBegin + iLeft;
       				*piEnd = lSelBegin + iLeft + iCRLF - 1;
        			}
				return true;
			}

		sN = sText.Mid( iLeft + 1, iCRLF - 1 );
			if ( m_pUsers->findItem( (string) sN) >= 0 ) {
        			*sNick = sN;
        			if ( piBegin && piEnd ) {
					*piBegin = lSelBegin + iLeft + 1;
					*piEnd = lSelBegin + iLeft + iCRLF;
        			}
				return true;
			}

		sN = sText.Mid( iLeft + 1, iCRLF - 2 );
			if ( m_pUsers->findItem( (string) sN) >= 0 ) {
				*sNick = sN;
        			if ( piBegin && piEnd ) {
       				*piBegin = lSelBegin + iLeft + 1;
   					*piEnd = lSelBegin + iLeft + iCRLF - 1;
        			}
				return true;
			}
		}	
	return false;
}

bool ChatCtrl::HitIP( POINT p, CAtlString *sIP, int *piBegin, int *piEnd ) {
	iCharPos = CharFromPos( p );
	len = LineLength( iCharPos ) + 1;
		if ( len < 3 )
			return false;

		DWORD lPosBegin = FindWordBreak( WB_LEFT, iCharPos );
		DWORD lPosEnd = FindWordBreak( WB_RIGHTBREAK, iCharPos );
		len = lPosEnd - lPosBegin;
	if(len > g_BufTemplen) {
		g_BufTemp = (char *) realloc(g_BufTemp, len+1);
		g_BufTemplen = len;
	}
	iRet = GetTextRange( lPosBegin, lPosEnd, g_BufTemp );
		UNREFERENCED_PARAMETER(iRet);
		g_BufTemp[len] = 0;
		for ( int i = 0; i < len; i++ ) {
			if ( !( ( g_BufTemp[i] == 0 ) ||( g_BufTemp[i] == '.' ) || ( ( g_BufTemp[i] >= '0' ) && ( g_BufTemp[i] <= '9' ) ) ) ){
				return false;
			}
		}
	sText = g_BufTemp;

		sText.ReleaseBuffer();
		sText.TrimLeft();
		sText.TrimRight();
		sText = sText + '.';
	iFindBegin = 0;
	iPos = -1;
	boOK = true;
	iEnd2 = 0;
		for ( int i = 0; i < 4; i++ ) {
		iPos = sText.Find( '.', iFindBegin );
			if ( iPos < 0 ) {
				boOK = false;
				break;
			}
		iEnd2 = atoi( sText.Mid( iFindBegin ) );
		if ( ( iEnd2 < 0 ) || ( iEnd2 > 255 ) ) {
        				boOK = false;
        				break;
				}
		iFindBegin = iPos + 1;
		}

		if ( boOK ) {
			*sIP = sText.Mid( 0, iPos );
			if ( piBegin && piEnd ) {
				*piBegin = lPosBegin;
				*piEnd = lPosEnd;
			}
		}
		return boOK;
}

bool ChatCtrl::HitURL(POINT p) {
	GetSel( lSelBegin, lSelEnd );
	boOK = false;

		CHARFORMAT2 cfSel;
		cfSel.cbSize = sizeof( cfSel );
    
		GetSelectionCharFormat( cfSel );
		if ( cfSel.dwEffects & CFE_LINK ) {
		boOK = true;
		}
	return boOK;
}

string ChatCtrl::LineFromPos( POINT p ) {
	iCharPos = CharFromPos( p );
	line = LineFromChar( iCharPos );
	len = LineLength( iCharPos ) + 1;
	if ( len < 3 ) {
		return _T("");
	}
	if(len > g_BufTemplen) {
		g_BufTemp = (char *) realloc(g_BufTemp, len+1);
		g_BufTemplen = len;
	}
	GetLine( line, g_BufTemp, len );
	string x(g_BufTemp, len-1);
	return x;
}

void ChatCtrl::GoToEnd() {
	if(m_boAutoScroll) {
		this->CRichEditCtrl::SendMessage(EM_SCROLL, SB_BOTTOM, 0);
	}
}

bool ChatCtrl::GetAutoScroll() {
	return m_boAutoScroll;
}

void ChatCtrl::SetAutoScroll( bool boAutoScroll ) {
	m_boAutoScroll = boAutoScroll;
	 if(boAutoScroll)
		GoToEnd();
}

LRESULT ChatCtrl::OnRButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	// Po kliku dovnitr oznaceneho textu nedelat nic
	GetSel( lSelBegin, lSelEnd );
	iCharPos = CharFromPos( pt );
	if ( ( lSelEnd > lSelBegin ) && ( iCharPos >= lSelBegin ) && ( iCharPos <= lSelEnd ) ) {
		return 1;
	}

	// Po kliku do IP oznacit IP
	iBegin = 0, iEnd1 = 0;
	if ( HitIP( pt, &sSel, &iBegin, &iEnd1 ) ) {
		SetSel( iBegin, iEnd1 );
		InvalidateRect( NULL );
	} else if ( HitNick( pt, &sSel, &iBegin, &iEnd1 ) ) {
		SetSel( iBegin, iEnd1 );
		InvalidateRect( NULL );
	}
	return 1;
}

