/* 
 * Copyright (C) 2003 Twink,  spm7@waikato.ac.nz
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

#ifndef AVIPPREVIEW_H
#define AVIPREVIEW_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropPage.h"
#include "ExListViewCtrl.h"
#include "resource.h"

class PreviewApplication;

class AVIPreview : public CPropertyPage<IDD_AVIPREVIEW>, public PropPage
{
public:
	AVIPreview(SettingsManager *s) : PropPage(s)  {
		title = strdup((STRING(SETTINGS_CZDC) + '\\' + STRING(SETTINGS_AVIPREVIEW)).c_str());
		SetTitle(title);
	};
	virtual ~AVIPreview() {
		ctrlCommands.Detach();
		delete[] title;};

	BEGIN_MSG_MAP(ColorPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_ADD_MENU, onAddMenu)
		COMMAND_ID_HANDLER(IDC_REMOVE_MENU, onRemoveMenu)
		COMMAND_ID_HANDLER(IDC_CHANGE_MENU, onChangeMenu)
		NOTIFY_HANDLER(IDC_MENU_ITEMS, BN_DOUBLECLICKED, onDblClick)
	END_MSG_MAP()
	
	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);


	LRESULT onAddMenu(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onChangeMenu(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemoveMenu(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onDblClick(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& bHandled) {
		return onChangeMenu(0, 0, 0, bHandled);
	}

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	virtual void write();	
protected:
	ExListViewCtrl ctrlCommands;
	static TextItem texts[];
	char* title;
	void addEntry(PreviewApplication* pa, int pos);
};


#endif