/* 
 * Copyright (C) 2001-2004 Jacek Sieka, j_s at telia com
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

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "NotepadFrame.h"
#include "WinUtil.h"
#include "../client/File.h"
#include "CZDCLib.h"

LRESULT NotepadFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	ctrlPad.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL, WS_EX_CLIENTEDGE);
	
	ctrlPad.LimitText(0);
	ctrlPad.SetFont(WinUtil::font);
	string tmp;
	try {
		tmp = File(Util::getAppPath() + SETTINGS_DIR + "Notepad.txt", File::READ, File::OPEN).read();
	} catch(const FileException&) {
		// ...
	}
	
	if(tmp.empty()) {
		tmp = SETTING(NOTEPAD_TEXT);
		if(!tmp.empty()) {
			dirty = true;
			SettingsManager::getInstance()->set(SettingsManager::NOTEPAD_TEXT, Util::emptyString);
		}
	}

	ctrlPad.SetWindowText(Text::toT(tmp).c_str());
	ctrlPad.EmptyUndoBuffer();
	ctrlClientContainer.SubclassWindow(ctrlPad.m_hWnd);
	
	bHandled = FALSE;
	return 1;
}

LRESULT NotepadFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		SettingsManager::getInstance()->removeListener(this);
	if(dirty || ctrlPad.GetModify()) {
			AutoArray<TCHAR> buf(ctrlPad.GetWindowTextLength() + 1);
		ctrlPad.GetWindowText(buf, ctrlPad.GetWindowTextLength() + 1);
		try {
			string tmp(Text::fromT(tstring(buf, ctrlPad.GetWindowTextLength())));
			File(Util::getAppPath() + SETTINGS_DIR + "Notepad.txt", File::WRITE, File::CREATE | File::TRUNCATE).write(tmp);
		} catch(const FileException&) {
			// Oops...
		}
	}
	
		closed = true;
		CZDCLib::setButtonPressed(IDC_NOTEPAD, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		bHandled = FALSE;
		return 0;
	}
}

void NotepadFrame::UpdateLayout(BOOL /*bResizeBars*/ /* = TRUE */)
{
	CRect rc;

	GetClientRect(rc);
	
	rc.bottom -= 1;
	rc.top += 1;
	rc.left +=1;
	rc.right -=1;
	ctrlPad.MoveWindow(rc);
	
}

LRESULT NotepadFrame::onLButton(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	HWND focus = GetFocus();
	bHandled = false;
	if(focus == ctrlPad.m_hWnd) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		tstring x;
		string::size_type start = (string::size_type)WinUtil::textUnderCursor(pt, ctrlPad, x);
		string::size_type end = x.find(_T(" "), start);

		if(end == string::npos)
			end = x.length();
		
		bHandled = WinUtil::parseDBLClick(x, start, end);
	}
	return 0;
}

void NotepadFrame::on(SettingsManagerListener::Save, SimpleXML* /*xml*/) throw() {
	RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

/**
 * @file
 * $Id$
 */