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
#include "../client/SettingsManager.h"

#include "DownloadDirsPage.h"
#include "DownloadDirDlg.h"
#include "WinUtil.h"

PropPage::TextItem DownloadDirsPage::texts[] = {
	{ IDC_ADD_MENU, ResourceManager::ADD },
	{ IDC_CHANGE_MENU, ResourceManager::SETTINGS_CHANGE },
	{ IDC_REMOVE_MENU, ResourceManager::REMOVE },
	{ IDC_DIR_DESCRIPTION, ResourceManager::SETTINGS_DIRS_DESCRIPTION },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item DownloadDirsPage::items[] = {
	{ 0, 0, PropPage::T_END }
};

LRESULT DownloadDirsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/){
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	CRect rc;

	ctrlCommands.Attach(GetDlgItem(IDC_MENU_ITEMS));
	ctrlCommands.GetClientRect(rc);

	ctrlCommands.InsertColumn(0, CTSTRING(SETTINGS_NAME), LVCFMT_LEFT, rc.Width()/4, 0);
	ctrlCommands.InsertColumn(1, CTSTRING(DIRECTORY), LVCFMT_LEFT, rc.Width()*2 / 4, 1);	
	ctrlCommands.InsertColumn(3, CTSTRING(SETTINGS_EXTENSIONS), LVCFMT_LEFT, rc.Width() / 4, 2);

	ctrlCommands.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);

	// Do specialized reading here
	SettingsManager::DDList d = SettingsManager::getInstance()->getDownloadDirs();
	for(SettingsManager::DDList::iterator i = d.begin(); i != d.end(); ++i) {		
		addEntry(i, ctrlCommands.GetItemCount());
	}
	return TRUE;
}

void DownloadDirsPage::write(){
}

void DownloadDirsPage::addEntry(SettingsManager::DownloadDirectory *pa, int pos) {
	TStringList lst;
	lst.push_back(Text::toT(pa->name));
	lst.push_back(Text::toT(pa->dir)); 
	lst.push_back(Text::toT(pa->ext));
	ctrlCommands.insert(pos, lst, 0, 0);
}


LRESULT DownloadDirsPage::onAddMenu(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	DownloadDirDlg dlg;
	if(dlg.DoModal() == IDOK){
		SettingsManager::DownloadDirectory d = SettingsManager::getInstance()->addDownloadDir(dlg.dir, dlg.extensions, dlg.name);
		addEntry(&d, ctrlCommands.GetItemCount());
		WinUtil::addLastDir(Text::toT(dlg.dir));
	}
	return 0;
}

LRESULT DownloadDirsPage::onChangeMenu(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	if(ctrlCommands.GetSelectedCount() == 1) {
		int sel = ctrlCommands.GetSelectedIndex();
		SettingsManager::DownloadDirectory pa = SettingsManager::getInstance()->getDownloadDirs()[sel];

		DownloadDirDlg dlg;
		dlg.name = pa.name;
		dlg.dir  = pa.dir;
		dlg.extensions = pa.ext;

		if(dlg.DoModal() == IDOK) {
			pa.name = dlg.name;
			pa.dir = dlg.dir;
			pa.ext = dlg.extensions;

			SettingsManager::getInstance()->updateDownloadDir(sel, pa);

			ctrlCommands.SetItemText(sel, 0, Text::toT(dlg.name).c_str());
			ctrlCommands.SetItemText(sel, 1, Text::toT(dlg.dir).c_str());
			ctrlCommands.SetItemText(sel, 2, Text::toT(dlg.extensions).c_str());
		}
	}
	return 0;
}

LRESULT DownloadDirsPage::onRemoveMenu(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	if(ctrlCommands.GetSelectedCount() == 1) {
		int sel = ctrlCommands.GetSelectedIndex();
		SettingsManager::getInstance()->removeDownloadDir(sel);
		ctrlCommands.DeleteItem(sel);
	}
	return 0;
}