/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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

#ifndef Segment_H
#define Segment_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropPage.h"


class Segment : public CPropertyPage<IDD_SEGMENT>, public PropPage
{
public:
	Segment(SettingsManager *s) : PropPage(s) { 
		title = strdup((STRING(SETTINGS_CZDC) + '\\' + STRING(SETTINGS_SEGMENT)).c_str());
		SetTitle(title);
	};

	virtual ~Segment() { delete[] title; };

	BEGIN_MSG_MAP(Segment)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	
	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	virtual void write();

protected:
	static Item items[];
	static TextItem texts[];
	char* title;
	CComboBox ctrlBlockSize;
};

#endif //Segment_H

/**
 * @file
 * $Id$
 */

