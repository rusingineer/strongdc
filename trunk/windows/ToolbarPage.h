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


#ifndef TOOLBARPAGE_H
#define TOOLBARPAGE_H

#include "PropPage.h"

class ToolbarPage : public CPropertyPage<IDD_TOOLBARPAGE>, public PropPage
{
public:
	ToolbarPage(SettingsManager *s) : PropPage(s) {
		title = strdup((STRING(SETTINGS_CZDC) + '\\' + STRING(SETTINGS_TOOLBAR)).c_str());
		SetTitle(title);
	};
	~ToolbarPage() { delete[] title;};

	BEGIN_MSG_MAP(ToolbarPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_HANDLER(IDC_IMAGEBROWSE, BN_CLICKED, onImageBrowse)
		COMMAND_HANDLER(IDC_HOTBROWSE, BN_CLICKED, onHotBrowse)
		COMMAND_HANDLER(IDC_TOOLBAR_ADD, BN_CLICKED, onAdd)
		COMMAND_HANDLER(IDC_TOOLBAR_REMOVE, BN_CLICKED, onRemove)		
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onImageBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onHotBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	
	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	virtual void write();

protected:
	CListViewCtrl ctrlCommands, ctrlToolbar;
	void BrowseForPic(int DLGITEM);
	string filter(string s);
	void makeItem(LPLVITEM lvi, int item);

	static Item items[];
	static TextItem texts[];
	char* title;
};

#endif //TOOLBARPAGE_H
