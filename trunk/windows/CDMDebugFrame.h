
#if !defined __CDMDEBUGFRAME_H
#define __CDMDEBUGFRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "WinUtil.h"

#include "../client/DebugManager.h"

class CDMDebugFrame : private DebugManagerListener, public MDITabChildWindowImpl<CDMDebugFrame>, public StaticFrame<CDMDebugFrame, ResourceManager::MENU_CDMDEBUG_MESSAGES>
{
public:
	DECLARE_FRAME_WND_CLASS_EX("CDMDebugFrame", IDR_MAINFRAME, 0, COLOR_3DFACE);

	CDMDebugFrame() { 
		DebugManager::getInstance()->addListener(this);
	}
	~CDMDebugFrame() { 
		DebugManager::getInstance()->removeListener(this);
	}
	
	virtual void OnFinalMessage(HWND /*hWnd*/) {
		delete this;
	}

	typedef MDITabChildWindowImpl<CDMDebugFrame> baseClass;
	BEGIN_MSG_MAP(CDMDebugFrame)
		MESSAGE_HANDLER(WM_SETFOCUS, OnFocus)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	void UpdateLayout(BOOL bResizeBars = TRUE);
	
	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		HWND hWnd = (HWND)lParam;
		HDC hDC = (HDC)wParam;
		if(hWnd == ctrlPad.m_hWnd) {
			::SetBkColor(hDC, WinUtil::bgColor);
			::SetTextColor(hDC, WinUtil::textColor);
			return (LRESULT)WinUtil::bgBrush;
		}
		bHandled = FALSE;
		return FALSE;
	};
	
	
	LRESULT OnFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlPad.SetFocus();
		return 0;
	}
	static void addLine(const string& s);
	static void setPause(bool bPause);
	
private:
	static bool pause;
	CEdit ctrlPad;
	void onAction(DebugManagerListener::Types type, const string& aLine) throw() {
		addLine(aLine);
	}
};

#endif // __CDMDEBUGFRAME_H