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

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "DownloadDirDlg.h"
#include "WinUtil.h"

LRESULT DownloadDirDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	#define ATTACH(id, var) var.Attach(GetDlgItem(id))
	ATTACH(IDC_DOWNLOADDIR_NAME, ctrlName);
	ATTACH(IDC_DOWNLOADDIR_DIR, ctrlDir);
	ATTACH(IDC_DOWNLOADDIR_EXT, ctrlExtensions);

	ctrlName.SetWindowText(name.c_str());
	ctrlDir.SetWindowText(dir.c_str());
	ctrlExtensions.SetWindowText(extensions.c_str());

	return 0;
}

LRESULT DownloadDirDlg::OnBrowse(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/) {
	
	string target;
	if(WinUtil::browseDirectory(target, (HWND) *this)) {
		SetDlgItemText(IDC_DOWNLOADDIR_DIR, target.c_str());
	}
 
	return 0;
}