
#if !defined __CDMDEBUGFRAME_H
#define __CDMDEBUGFRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define COMMAND_MESSAGE_MAP 14
#define DETECTION_MESSAGE_MAP 15
#define HUB_COMMAND_MESSAGE_MAP 16
#define DEBUG_FILTER_MESSAGE_MAP 17
#define DEBUG_FILTER_TEXT_MESSAGE_MAP 18
#define CLEAR_MESSAGE_MAP 19

#include "FlatTabCtrl.h"
#include "WinUtil.h"

#include "../client/DebugManager.h"

class CDMDebugFrame : private DebugManagerListener, public MDITabChildWindowImpl<CDMDebugFrame, RGB(0, 0, 0), IDR_CDM>, public StaticFrame<CDMDebugFrame, ResourceManager::MENU_CDMDEBUG_MESSAGES>
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("CDMDebugFrame"), IDR_CDM, 0, COLOR_3DFACE);

	CDMDebugFrame() : closed(false), showCommands(true), showHubCommands(false), showDetection(false), bFilterIp(false),
		detectionContainer(WC_BUTTON, this, DETECTION_MESSAGE_MAP),
		HubCommandContainer(WC_BUTTON, this, HUB_COMMAND_MESSAGE_MAP),
		commandContainer(WC_BUTTON, this, COMMAND_MESSAGE_MAP),
		cFilterContainer(WC_BUTTON, this, DEBUG_FILTER_MESSAGE_MAP),
		eFilterContainer(WC_EDIT, this, DEBUG_FILTER_TEXT_MESSAGE_MAP),
		clearContainer(WC_BUTTON, this, CLEAR_MESSAGE_MAP),
		statusContainer(STATUSCLASSNAME, this, CLEAR_MESSAGE_MAP)
 { 
		DebugManager::getInstance()->addListener(this);
	}
	~CDMDebugFrame() {		
	}
	
	virtual void OnFinalMessage(HWND /*hWnd*/) {
		delete this;
	}

	typedef MDITabChildWindowImpl<CDMDebugFrame, RGB(0, 0, 0), IDR_CDM> baseClass;
	BEGIN_MSG_MAP(CDMDebugFrame)
		MESSAGE_HANDLER(WM_SETFOCUS, OnFocus)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(DETECTION_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onSetCheckDetection)
	ALT_MSG_MAP(COMMAND_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onSetCheckCommand)
	ALT_MSG_MAP(HUB_COMMAND_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onSetCheckHubCommand)
	ALT_MSG_MAP(DEBUG_FILTER_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onSetCheckFilter)
	ALT_MSG_MAP(DEBUG_FILTER_TEXT_MESSAGE_MAP)
		COMMAND_HANDLER(IDC_DEBUG_FILTER_TEXT, EN_CHANGE, onChange)
 	ALT_MSG_MAP(CLEAR_MESSAGE_MAP)
		COMMAND_ID_HANDLER(IDC_CLEAR, onClear)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClear(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
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
	LRESULT onSetCheckDetection(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		showDetection = wParam == BST_CHECKED;
		bHandled = FALSE;
		return 0;
	}
	LRESULT onSetCheckCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		showCommands = wParam == BST_CHECKED;
		bHandled = FALSE;
		return 0;
	}
	LRESULT onSetCheckHubCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		showHubCommands = wParam == BST_CHECKED;
		bHandled = FALSE;
		return 0;
	}
	LRESULT onSetCheckFilter(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		bFilterIp = wParam == BST_CHECKED;
		UpdateLayout();
		bHandled = FALSE;
		return 0;
	}
	LRESULT onChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		AutoArray<TCHAR> buf(ctrlFilterText.GetWindowTextLength() + 1);
		ctrlFilterText.GetWindowText(buf, ctrlFilterText.GetWindowTextLength() + 1);
		sFilterIp = buf;
		UpdateLayout();
		return 0;
	}

	void addLine(const string& aLine);
	
private:
	CEdit ctrlPad, ctrlFilterText;
	CStatusBarCtrl ctrlStatus;
	CButton ctrlClear, ctrlCommands, ctrlHubCommands, ctrlDetection, ctrlFilterIp;
	CContainedWindow clearContainer, statusContainer, detectionContainer, commandContainer, HubCommandContainer, cFilterContainer, eFilterContainer;

	bool showCommands, showHubCommands, showDetection, bFilterIp;
	tstring sFilterIp;

	CriticalSection cs;
	bool closed;
	
	void on(DebugManagerListener::DebugDetection, const string& aLine) throw() {
		if(!showDetection)
			return;
		addLine(aLine);
	}
	void on(DebugManagerListener::DebugCommand, const string& aLine, int typeDir, const string& ip) throw() {
			switch(typeDir) {
				case DebugManager::HUB_IN:
					if(!showHubCommands)
						return;
					if(!bFilterIp || Text::toT(ip) == sFilterIp) {
						addLine("Hub:\t[" + ip + "]\t <<  \t" + aLine);
					}
					break;
				case DebugManager::HUB_OUT:
					if(!showHubCommands)
						return;
					if(!bFilterIp || Text::toT(ip) == sFilterIp) {
						addLine("Hub:\t[" + ip + "]\t   >>\t" + aLine);
					}
					break;
				case DebugManager::CLIENT_IN:
					if(!showCommands)
						return;
					if(!bFilterIp || Text::toT(ip) == sFilterIp) {
						addLine("Client:\t[" + ip + "]\t <<  \t" + aLine);
					}
					break;
				case DebugManager::CLIENT_OUT:
					if(!showCommands)
						return;
					if(!bFilterIp || Text::toT(ip) == sFilterIp) {
						addLine("Client:\t[" + ip + "]\t   >>\t" + aLine);
					}
					break;
				default: dcassert(0);
			}
	}
};

#endif // __CDMDEBUGFRAME_H