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

#if !defined(AFX_CHAT_CTRL_H__595F1372_081B_11D1_890D_00A0244AB9FD__INCLUDED_)
#define AFX_CHAT_CTRL_H__595F1372_081B_11D1_890D_00A0244AB9FD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "atlstr.h"
#include "TypedListViewCtrl.h"
#include "ImageDataObject.h"
#include "../client/Client.h"
#ifndef USERINFO_H
	#include "UserInfo.h"
#endif

#ifndef _RICHEDIT_VER
#define _RICHEDIT_VER 0x0200
#endif

class UserInfo;

class ChatCtrl: public CWindowImpl<ChatCtrl, CRichEditCtrl> {

public:
	ChatCtrl();
	virtual ~ChatCtrl();

	BEGIN_MSG_MAP(ChatCtrl)
	  // put your message handler entries here
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
	END_MSG_MAP()

	bool HitNick( POINT p, CAtlString *sNick, int *piBegin = NULL, int *piEnd = NULL );
	bool HitIP( POINT p, CAtlString *sIP, int *piBegin = NULL, int *piEnd = NULL );
	bool HitURL(POINT p);
	bool GetAutoScroll();

	string LineFromPos( POINT p );

	void ReadSettings();
	void AdjustTextSize(LPCTSTR lpstrTextToAdd = _T(""));
	void AppendText(LPCTSTR sMyNick, LPCTSTR sTime, LPCTSTR sMsg, CHARFORMAT2& cf, LPCTSTR sAuthor = _T(""));
	void AppendTextOnly(LPCTSTR sMyNick, LPCTSTR sTime, LPCTSTR sMsg, CHARFORMAT2& cf, LPCTSTR sAuthor = _T(""), bool bRedrawControlAtEnd = true);
	void EndRedrawAppendTextOnly();
	void AppendBitmap(HBITMAP hbm);

	void GoToEnd();
	void SetAutoScroll( bool boAutoScroll );
	void SetUsers( TypedListViewCtrl<UserInfo, IDC_USERS> *pUsers = NULL );
	void SetTextStyleMyNick( CHARFORMAT2 ts ) { m_TextStyleMyNick = ts; };

protected:
	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	IRichEditOle* GetIRichEditOle() const;

	CHARFORMAT2 m_TextStyleGeneral;
	CHARFORMAT2 m_TextStyleTimestamp;
	CHARFORMAT2 m_TextStyleMyNick;
	CHARFORMAT2 m_ChatTextMyOwn;
	CHARFORMAT2 m_TextStyleBold;
	CHARFORMAT2 m_TextStyleFavUsers;
	CHARFORMAT2 m_TextStyleOPs;
	CHARFORMAT2 m_TextStyleURL;
	
	TypedListViewCtrl<UserInfo, IDC_USERS> *m_pUsers;
	bool m_boAutoScroll, bMyMessage, boOK, boAtTheEnd, bRedrawControl, isOP, myMess;
	TCHAR *g_BufTemp, *beforeAppendText, *afterAppendText;
	int g_BufTemplen, AppendTextlen, line, iFindBegin, iBegin, iEnd1, iEnd2, iCRLF, iCRLF_Len, len, smiles, maxsmiles, 
		nIdxFound, iCharPos, iRet, iLeft, iRight, iPos;
	long lSelBeginSaved, lSelEndSaved, lMask, lSelBegin, lSelEnd, lTextLength, lMyNickStart, lMyNickEnd, lAuthorBegin, 
		lAuthorEnd, lSearchFrom, lPos;
	CAtlString sText, sNick, sMsgLower, sSel, sAuthorNick, sN;
	tstring msg;
};


#endif //!defined(AFX_CHAT_CTRL_H__595F1372_081B_11D1_890D_00A0244AB9FD__INCLUDED_)
