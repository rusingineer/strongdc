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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PropPage::TextItem FakeDetect::texts[] = {
	{ IDC_IGNORE_JUNK_FILES, ResourceManager::TEXT_IGNORE_JUNK },
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
	{ IDC_IGNORE_JUNK_FILES, SettingsManager::IGNORE_JUNK_FILES, PropPage::T_BOOL }, 
	{ 0, 0, PropPage::T_END }
};

FakeDetect::ListItem FakeDetect::listItems[] = {
	{ SettingsManager::CHECK_NEW_USERS, ResourceManager::CHECK_ON_CONNECT },
		{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT FakeDetect::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	//PropPage::read((HWND)*this, items);
	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_FAKE_BOOLEANS));

	// Do specialized reading here
	
	return TRUE;
}

void FakeDetect::write()
{
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_FAKE_BOOLEANS));
	
	// Do specialized writing here
	// settings->set(XX, YY);
}


/**
 * @file
 * $Id$
 */

