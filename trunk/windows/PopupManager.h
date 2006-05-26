/* 
* Copyright (C) 2003-2005 P�r Bj�rklund, per.bjorklund@gmail.com
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
	PopupManager() : height(90), width(200), offset(0), activated(true), id(0) {
		TimerManager::getInstance()->addListener(this);

	}
	~PopupManager() {
		TimerManager::getInstance()->removeListener(this);
	}
	
	//call this with a preformatted message
	void Show(const string &aMsg, const string &aTitle, int Icon);

	//remove first popup in list and move everyone else
	void Remove(u_int32_t pos = 0);

	//remove the popups that are scheduled to be removed
	void AutoRemove();
	
	void Mute(bool mute) {
		activated = !mute;
	}


private:
		
	typedef list< PopupWnd* > PopupList;
	typedef PopupList::iterator PopupIter;
	PopupList popups;
	
	//size of the popup window
	u_int16_t height;
	u_int16_t width;

	//if we have multiple windows displayed, 
	//keep track of where the new one will be displayed
	u_int16_t offset;
	
	//turn on/off popups completely
	bool activated;

	//id of the popup to keep track of them
	u_int32_t id;
 	
	// TimerManagerListener
	virtual void on(TimerManagerListener::Second, u_int32_t tick) throw();

};

#endif