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
#include "TimerManager.h"


class DebugManagerListener {
public:
template<int I>	struct X { enum { TYPE = I };  };

	typedef X<0> DebugMessage;

	virtual void on(DebugMessage, const string&) throw() { }
};

class DebugManager : public Singleton<DebugManager>, public Speaker<DebugManagerListener>
{
	CriticalSection cs;
	friend class Singleton<DebugManager>;
	DebugManager();
public:
	void SendDebugMessage(const string& s);
	~DebugManager();
};

#endif // !defined(AFX_SCRIPTMANAGER_H__730F7CF5_5C7D_4A2A_827B_53267D0EF4C5__INCLUDED_)

/**
 * @file ScriptManager.h
 * $Id$
 */
