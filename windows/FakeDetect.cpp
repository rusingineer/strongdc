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

#include "FakeDetect.h"
#include "../client/SettingsManager.h"
#include "WinUtil.h"

PropPage::TextItem FakeDetect::texts[] = {
	{ INFORMACE, ResourceManager::TEXT_FAKEINFO },
	{ AAA, ResourceManager::TEXT_JUNK },
	{ BAA, ResourceManager::TEXT_BINJUNK },
	{ CAA, ResourceManager::TEXT_VOBJUNK },
	{ DAA, ResourceManager::TEXT_FAKEPERCENT },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
}; 

PropPage::Item FakeDetect::items[] = {
	{ IDC_JUNK_FILE_SIZE, SettingsManager::JUNK_FILE_SIZE, PropPage::T_INT64 }, 
	{ IDC_JUNK_BIN_FILE_SIZE, SettingsManager::JUNK_BIN_FILE_SIZE, PropPage::T_INT64 }, 
	{ IDC_JUNK_VOB_FILE_SIZE, SettingsManager::JUNK_VOB_FILE_SIZE, PropPage::T_INT64 }, 
	{ IDC_PERCENT_FAKE_SHARE_TOLERATED, SettingsManager::PERCENT_FAKE_SHARE_TOLERATED, PropPage::T_INT }, 
	{ 0, 0, PropPage::T_END }
};

FakeDetect::ListItem FakeDetect::listItems[] = {
	{ SettingsManager::CHECK_NEW_USERS, ResourceManager::CHECK_ON_CONNECT },
	{ SettingsManager::IGNORE_JUNK_FILES, ResourceManager::TEXT_IGNORE_JUNK },
	{ SettingsManager::DISPLAY_CHEATS_IN_MAIN_CHAT, ResourceManager::SETTINGS_DISPLAY_CHEATS_IN_MAIN_CHAT },
		{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT FakeDetect::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_FAKE_BOOLEANS));
	CComboBox cRaw;

#define ADDSTRINGS \
	cRaw.AddString("No action"); \
	cRaw.AddString("Raw 1"); \
	cRaw.AddString("Raw 2"); \
	cRaw.AddString("Raw 3"); \
	cRaw.AddString("Raw 4"); \
	cRaw.AddString("Raw 5");

	cRaw.Attach(GetDlgItem(IDC_DISCONNECT_RAW));
	ADDSTRINGS
	cRaw.SetCurSel(settings->get(SettingsManager::DISCONNECT_RAW));
	cRaw.Detach();

	cRaw.Attach(GetDlgItem(IDC_TIMEOUT_RAW));
	ADDSTRINGS
	cRaw.SetCurSel(settings->get(SettingsManager::TIMEOUT_RAW));
	cRaw.Detach();

	cRaw.Attach(GetDlgItem(IDC_FAKE_RAW));
	ADDSTRINGS
	cRaw.SetCurSel(settings->get(SettingsManager::FAKESHARE_RAW));
	cRaw.Detach();

	cRaw.Attach(GetDlgItem(IDC_LISTLEN));
	ADDSTRINGS
	cRaw.SetCurSel(settings->get(SettingsManager::LISTLEN_MISMATCH));
	cRaw.Detach();

	cRaw.Attach(GetDlgItem(IDC_FILELIST_TOO_SMALL));
	ADDSTRINGS
	cRaw.SetCurSel(settings->get(SettingsManager::FILELIST_TOO_SMALL));
	cRaw.Detach();

	cRaw.Attach(GetDlgItem(IDC_FILELIST_UNAVAILABLE));
	ADDSTRINGS
	cRaw.SetCurSel(settings->get(SettingsManager::FILELIST_UNAVAILABLE));
	cRaw.Detach();


	PropPage::translate((HWND)(*this), texts);



	// Do specialized reading here
	
	return TRUE;
}

void FakeDetect::write() {
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_FAKE_BOOLEANS));
	
	// Do specialized writing here
	// settings->set(XX, YY);
	CComboBox cRaw(GetDlgItem(IDC_DISCONNECT_RAW));
	SettingsManager::getInstance()->set(SettingsManager::DISCONNECT_RAW, cRaw.GetCurSel());

	cRaw = GetDlgItem(IDC_TIMEOUT_RAW);
	SettingsManager::getInstance()->set(SettingsManager::TIMEOUT_RAW, cRaw.GetCurSel());

	cRaw = GetDlgItem(IDC_FAKE_RAW);
	SettingsManager::getInstance()->set(SettingsManager::FAKESHARE_RAW, cRaw.GetCurSel());

	cRaw = GetDlgItem(IDC_LISTLEN);
	SettingsManager::getInstance()->set(SettingsManager::LISTLEN_MISMATCH, cRaw.GetCurSel());

	cRaw = GetDlgItem(IDC_FILELIST_TOO_SMALL);
	SettingsManager::getInstance()->set(SettingsManager::FILELIST_TOO_SMALL, cRaw.GetCurSel());

	cRaw = GetDlgItem(IDC_FILELIST_UNAVAILABLE);
	SettingsManager::getInstance()->set(SettingsManager::FILELIST_UNAVAILABLE, cRaw.GetCurSel());
}

/**
 * @file
 * $Id$
 */

