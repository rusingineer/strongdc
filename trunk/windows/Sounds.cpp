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

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "Sounds.h"
#include "../client/SettingsManager.h"
#include "WinUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PropPage::TextItem Sounds::texts[] = {
	{ IDC_PRIVATE_MESSAGE_BEEP, ResourceManager::SETTINGS_PM_BEEP },
	{ IDC_PRIVATE_MESSAGE_BEEP_OPEN, ResourceManager::SETTINGS_PM_BEEP_OPEN },
	{ IDC_CZDC_PM_SOUND, ResourceManager::SETCZDC_PRIVATE_SOUND },
	{ IDC_BROWSE, ResourceManager::BROWSE },	
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item Sounds::items[] = {
	{ IDC_PRIVATE_MESSAGE_BEEP, SettingsManager::PRIVATE_MESSAGE_BEEP, PropPage::T_BOOL },
	{ IDC_PRIVATE_MESSAGE_BEEP_OPEN, SettingsManager::PRIVATE_MESSAGE_BEEP_OPEN, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

LRESULT Sounds::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	ctrlSounds.Attach(GetDlgItem(IDC_SOUNDLIST));

	ctrlSounds.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
	ctrlSounds.InsertColumn(0, CSTRING(SETTINGS_SOUNDS), LVCFMT_LEFT, 172, 0);
	ctrlSounds.InsertColumn(1, CSTRING(FILENAME), LVCFMT_LEFT, 210, 1);

	int i;
	i = ctrlSounds.insert(0, STRING(SOUND_DOWNLOAD_BEGINS));
	ctrlSounds.SetItemText(i, 1, SETTING(BEGINFILE).c_str());

	i = ctrlSounds.insert(1, STRING(SOUND_DOWNLOAD_FINISHED));
	ctrlSounds.SetItemText(i, 1, SETTING(FINISHFILE).c_str());

	i = ctrlSounds.insert(2, STRING(SOUND_SOURCE_ADDED));
	ctrlSounds.SetItemText(i, 1, SETTING(SOURCEFILE).c_str());

	i = ctrlSounds.insert(3, STRING(SOUND_UPLOAD_FINISHED));
	ctrlSounds.SetItemText(i, 1, SETTING(UPLOADFILE).c_str());

	i = ctrlSounds.insert(4, STRING(SOUND_FAKER_FOUND));
	ctrlSounds.SetItemText(i, 1, SETTING(FAKERFILE).c_str());

	i = ctrlSounds.insert(5, STRING(SETCZDC_PRIVATE_SOUND));
	ctrlSounds.SetItemText(i, 1, SETTING(BEEPFILE).c_str());

	i = ctrlSounds.insert(6, STRING(MYNICK_IN_CHAT));
	ctrlSounds.SetItemText(i, 1, SETTING(CHATNAMEFILE).c_str());

	i = ctrlSounds.insert(7, STRING(SOUND_TTH_INVALID));
	ctrlSounds.SetItemText(i, 1, SETTING(SOUND_TTH).c_str());

	i = ctrlSounds.insert(8, STRING(SOUND_EXCEPTION));
	ctrlSounds.SetItemText(i, 1, SETTING(SOUND_EXC).c_str());

	// Do specialized reading here


	return TRUE;
}


void Sounds::write()
{
	PropPage::write((HWND)*this, items);
	
	char buf[256];
	ctrlSounds.GetItemText(0, 1, buf, 256);
	settings->set(SettingsManager::BEGINFILE, string(buf));

	ctrlSounds.GetItemText(1, 1, buf, 256);
	settings->set(SettingsManager::FINISHFILE, string(buf));

	ctrlSounds.GetItemText(2, 1, buf, 256);
	settings->set(SettingsManager::SOURCEFILE, string(buf));

	ctrlSounds.GetItemText(3, 1, buf, 256);
	settings->set(SettingsManager::UPLOADFILE, string(buf));

	ctrlSounds.GetItemText(4, 1, buf, 256);
	settings->set(SettingsManager::FAKERFILE, string(buf));

	ctrlSounds.GetItemText(5, 1, buf, 256);
	settings->set(SettingsManager::BEEPFILE, string(buf));

	ctrlSounds.GetItemText(6, 1, buf, 256);
	settings->set(SettingsManager::CHATNAMEFILE, string(buf));

	ctrlSounds.GetItemText(7, 1, buf, 256);
	settings->set(SettingsManager::SOUND_TTH, string(buf));

	ctrlSounds.GetItemText(8, 1, buf, 256);
	settings->set(SettingsManager::SOUND_EXC, string(buf));


	// Do specialized writing here
	// settings->set(XX, YY);
}

LRESULT Sounds::onBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	char buf[MAX_PATH];
	LVITEM item;
	::ZeroMemory(&item, sizeof(item));
	item.mask = LVIF_TEXT;
	item.cchTextMax = sizeof(buf);
	item.pszText = buf;
	if(ctrlSounds.GetSelectedItem(&item)) {
	string x = "";	
		if(WinUtil::browseFile(x, m_hWnd, false) == IDOK) {
			ctrlSounds.SetItemText(item.iItem, 1, x.c_str());
	}
	}
	return 0;
}

LRESULT Sounds::onClickedNone(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	char buf[MAX_PATH];
	LVITEM item;
	::ZeroMemory(&item, sizeof(item));
	item.mask = LVIF_TEXT;
	item.cchTextMax = sizeof(buf);
	item.pszText = buf;
	if(ctrlSounds.GetSelectedItem(&item)) {
		string x = "";	
		ctrlSounds.SetItemText(item.iItem, 1, x.c_str());
	}
	return 0;
}
/**
 * @file
 * $Id$
 */

