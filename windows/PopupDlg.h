#ifndef POPUPWND_H
#define POPUPWND_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"
#include "WinUtil.h"

class PopupWnd : public CWindowImpl<PopupWnd, CWindow>
{
public:
	DECLARE_WND_CLASS(_T("Popup"));

	BEGIN_MSG_MAP(PopupWnd)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, onLButtonDown)
	END_MSG_MAP()

	PopupWnd(const string& aMsg, const string& aTitle, CRect rc): visible(GET_TICK()) {
			msg = aMsg;
			title = aTitle;

		Create(NULL, rc, NULL, WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW );

		WinUtil::decodeFont(Text::toT(SETTING(TEXT_FONT))/*SETTING(POPUP_FONT)*/, logFont);
		font = ::CreateFontIndirect(&logFont);

	}

	~PopupWnd(){
		DeleteObject(font);
	}

	LRESULT onLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/){
		CRect rc;
		GetWindowRect(rc);
		::PostMessage(WinUtil::mainWnd, WM_SPEAKER, WM_CLOSE, (LPARAM)rc.bottom);
		return 0;
	}

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled){
		::SetClassLongPtr(m_hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)::GetSysColorBrush(COLOR_INFOTEXT));
		CRect rc;
		GetClientRect(rc);

		rc.top += 1;
		rc.left += 1;
		rc.right -= 1;
		rc.bottom /= 3;

		label.Create(m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
			SS_CENTER | SS_NOPREFIX);

		rc.top += rc.bottom - 1;
		rc.bottom *= 3;

		label1.Create(m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
			SS_CENTER | SS_NOPREFIX);

		label.SetFont(WinUtil::boldFont);
		label.SetWindowText(Text::toT(title).c_str());
		label1.SetFont(WinUtil::font);
		label1.SetWindowText(Text::toT(msg).c_str());


		bHandled = false;
		return 1;
	}

	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled){
		label.DestroyWindow();
		label.Detach();
		DestroyWindow();

		bHandled = false;
		return 1;
	}

	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		HDC hDC = (HDC)wParam;
				::SetBkColor(hDC, ::GetSysColor(COLOR_INFOBK));
				::SetTextColor(hDC, ::GetSysColor(COLOR_INFOTEXT));
		return (LRESULT)::GetSysColorBrush(COLOR_INFOBK);
	}

	u_int32_t visible;

private:
	string  msg, title;
	CStatic label, label1;
	LOGFONT logFont;
	HFONT   font;
};


#endif