#ifndef POPUPMANAGER_H
#define POPUPMANAGER_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "../client/Singleton.h"
#include "../client/TimerManager.h"
#include "../client/CriticalSection.h"

#include "PopupDlg.h"
#include "WinUtil.h"

#define DOWNLOAD_COMPLETE 6

class PopupManager : public Singleton< PopupManager >, private TimerManagerListener
{
public:
	PopupManager() : height(90), width(200), offset(0), activated(true), minimized(false) {
		TimerManager::getInstance()->addListener(this);

	}
	~PopupManager() {
		TimerManager::getInstance()->removeListener(this);
	}
	
	//call this with a preformatted message
	void Show(const string &aMsg, const string &aTitle, int Icon);

	//remove first popup in list and move everyone else
	void Remove(int pos = 0);

	void Mute(bool mute) {
		activated = !mute;
	}

	void Minimized(bool mini){
		minimized = mini;
	}

private:
		
	typedef deque< PopupWnd* > PopupList;
	PopupList popups;
	
	//size of the popup window
	u_int16_t height;
	u_int16_t width;

	//if we have multiple windows displayed, 
	//keep track of where the new one will be displayed
	u_int16_t offset;
	
	//used for thread safety
	CriticalSection cs;

	//turn on/off popups completely
	bool activated;

	//keep track of window state
	bool minimized;
  	
	// TimerManagerListener
	virtual void on(TimerManagerListener::Second, u_int32_t tick) throw();

};

#endif