#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "../client/SettingsManager.h"
#include "../client/Util.h"

#include "WinUtil.h"
#include "PopupManager.h"
#include "MainFrm.h"


PopupManager* Singleton< PopupManager >::instance = NULL;

void PopupManager::Show(const string &aMsg, const string &aTitle, int Icon) {
	if(!activated)
		return;

	if(SETTING(POPUP_TYPE) == 0) {

		NOTIFYICONDATA m_nid;
		m_nid.cbSize = sizeof(NOTIFYICONDATA);
		m_nid.hWnd = MainFrame::getMainFrame()->m_hWnd;
		m_nid.uID = 0;
		m_nid.uFlags = NIF_INFO;
		m_nid.uTimeout = 5000;
		m_nid.dwInfoFlags = Icon;
		_tcscpy(m_nid.szInfo, Text::toT(aMsg).c_str());
		_tcscpy(m_nid.szInfoTitle, Text::toT(aTitle).c_str());
		Shell_NotifyIcon(NIM_MODIFY, &m_nid);
		return;

	}

/*	if (!Util::getAway() && BOOLSETTING(POPUP_AWAY)) {
		return;
	}

	if(!minimized && BOOLSETTING(POPUP_MINIMIZED)) 
		return;*/
	
	
	CRect rcDesktop;
	
	//get desktop rect so we know where to place the popup
	::SystemParametersInfo(SPI_GETWORKAREA,0,&rcDesktop,0);
	
	int screenHeight = rcDesktop.bottom;
	int screenWidth = rcDesktop.right;

	Lock l(cs);

	//if we have popups all the way up to the top of the screen do not create a new one
	if( (offset + height) > screenHeight)
		return;
	
	//get the handle of the window that has focus
	HWND gotFocus = ::SetFocus(WinUtil::mainWnd);
	
	//compute the window position
	CRect rc(screenWidth - width , screenHeight - height - offset, screenWidth, screenHeight - offset);
	
	//Create a new popup
	PopupWnd *p = new PopupWnd(aMsg, aTitle, rc);
			
	if(LOBYTE(LOWORD(GetVersion())) >= 5) {
		p->SetWindowLong(GWL_EXSTYLE, p->GetWindowLong(GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT);
		typedef bool (CALLBACK* LPFUNC)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
		LPFUNC _d_SetLayeredWindowAttributes = (LPFUNC)GetProcAddress(LoadLibrary(_T("user32")), "SetLayeredWindowAttributes");
		_d_SetLayeredWindowAttributes(p->m_hWnd, 0, 200, LWA_ALPHA);
	}

	//move the window to the top of the z-order and display it
	p->SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	p->ShowWindow(SW_SHOWNA);
	
		
	//restore focus to window
	::SetFocus(gotFocus);

	
	
	//increase offset so we know where to place the next popup
	offset = offset + height;

	popups.push_back(p);
}

void PopupManager::on(TimerManagerListener::Second /*type*/, u_int32_t tick) {
//	if(!BOOLSETTING(REMOVE_POPUPS))
//		return;

	Lock l(cs);

	//we got nothing to do here
	if(popups.empty()) {
		return;
	}

	//check all popups and see if we need to remove anyone
	PopupList::iterator i = popups.begin();
	for(; i != popups.end(); ++i) {

		if((*i)->visible + /*SETTING(POPUP_TIMEOUT)*/5 * 1000 < tick) {
			//okay remove the first popup
			Remove();

			//if list is empty there is nothing more to do
			if(popups.empty())
				return;
				
			//start over from the beginning
			i = popups.begin();
		}
	}
}

void PopupManager::Remove(int pos) {
	Lock l(cs);

	CRect rcDesktop;

	//get desktop rect so we know where to place the popup
	::SystemParametersInfo(SPI_GETWORKAREA,0,&rcDesktop,0);
	if(pos == 0)
		pos = rcDesktop.bottom;

	//find the correct window
	int end = (rcDesktop.bottom - pos) / height;
	PopupList::iterator i = popups.begin();
	for(int j = 0; j < end; ++j, ++i);
	
	//remove the window from the list
	PopupWnd *p = (*i);
	popups.erase(i);
	
	//close the window and delete it
	if(p == NULL){
		return;
	}
	
	p->SendMessage(WM_CLOSE, 0, 0);
	delete p;
	p = NULL;
	
	    
	//set offset one window position lower
	dcassert(offset > 0);
	offset = offset - height;

	//nothing to do
	if(popups.empty())
		return;

	CRect rc;

	//move down all windows
	for(i = popups.begin(); i != popups.end(); ++i) {
		(*i)->GetWindowRect(rc);
		if(rc.bottom <= pos){
			rc.top += height;
			rc.bottom += height;
			(*i)->MoveWindow(rc);
		}
	}
}
