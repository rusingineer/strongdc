/*
 * Copyright (C) 2001-2005 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(UPLOAD_PAGE_H)
#define UPLOAD_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"
#include "WinUtil.h"
#include "FolderTree.h"
#include "../client/SettingsManager.h"

class UploadPage : public CPropertyPage<IDD_UPLOADPAGE>, public PropPage
{
public:
	UploadPage(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_GENERAL) + _T('\\') + TSTRING(SETTINGS_UPLOADS)).c_str());
		SetTitle(title);
	};
	virtual ~UploadPage() {
		ctrlDirectories.Detach();
		ctrlTotal.Detach();
		free(title);
	};

	BEGIN_MSG_MAP_EX(UploadPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		MESSAGE_HANDLER(WM_DROPFILES, onDropFiles)
		NOTIFY_HANDLER(IDC_DIRECTORIES, LVN_ITEMCHANGED, onItemchangedDirectories)
		COMMAND_ID_HANDLER(IDC_ADD, onClickedAdd)
		COMMAND_ID_HANDLER(IDC_REMOVE, onClickedRemove)
		COMMAND_ID_HANDLER(IDC_RENAME, onClickedRename)
		COMMAND_ID_HANDLER(IDC_SHAREHIDDEN, onClickedShareHidden)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onDropFiles(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onItemchangedDirectories(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onClickedAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onClickedRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT UploadPage::onClickedRename(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onClickedShareHidden(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	virtual void write();
	
protected:
	static Item items[];
	static TextItem texts[];
	ExListViewCtrl ctrlDirectories;
	CStatic ctrlTotal;
	TCHAR* title;

	void addDirectory(const tstring& aPath);
	FolderTree ft;
};

#endif // !defined(UPLOAD_PAGE_H)

/**
 * @file
 * $Id$
 */
