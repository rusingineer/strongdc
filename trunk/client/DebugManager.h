/*
 * Copyright (C) 2003 BlackClaw, BlackClaw@parsoma.net
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

#if !defined __DEBUGMANAGER_H
#define __DEBUGMANAGER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DCPlusPlus.h"
#include "Singleton.h"
//#include "User.h"
//#include "DownloadManager.h"
//#include "Socket.h"
//#include "ClientManager.h"
#include "TimerManager.h"
//#include "SearchManager.h"


class DebugManagerListener {
public:
	typedef DebugManagerListener* Ptr;
	typedef vector<Ptr> List;
	typedef List::iterator Iter;
	
	enum Types {
		DEBUG_MESSAGE,
	};

	virtual void onAction(Types, const string&) throw() = 0;
};

class DebugManager : public Singleton<DebugManager>, public Speaker<DebugManagerListener>
		//, private DownloadManagerListener, private ClientManagerListener//, private ClientListener, private TimerManagerListener
{
	CriticalSection cs;
	//Socket s;

	//lua_State* L;
	friend class Singleton<DebugManager>;
	DebugManager();
public:
	/*bool onHubFrameEnter(const string& aLine);
	bool onClientMessage(Client* aClient, const string& aLine);
	bool onUserConnectionMessageIn(UserConnection* aConn, const string& aLine);
	bool onUserConnectionMessageOut(UserConnection* aConn, const string& aLine);
	string formatChatMessage(Client* aClient, string aLine);
	void EvaluateFile(const string& fn);
	void EvaluateChunk(const string& chunk);*/
	void SendDebugMessage(const string& s);
	~DebugManager();
private:
	//friend struct LuaManager;

	/*bool MakeCall(lua_State* L, int args, int ret) throw();
	bool GetLuaBool(lua_State* L);

	void onAction(DownloadManagerListener::Types type, Download* u) throw();
	void onAction(ClientManagerListener::Types, Client*) throw();
	void onAction(ClientListener::Types, Client*) throw();
	void onAction(ClientListener::Types, Client*, const string&) throw();
	void onAction(TimerManagerListener::Types type, u_int32_t ticks);*/
};

#endif // !defined(AFX_SCRIPTMANAGER_H__730F7CF5_5C7D_4A2A_827B_53267D0EF4C5__INCLUDED_)

/**
 * @file ScriptManager.h
 * $Id$
 */
