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

#include "AppearancePage.h"
#include "../client/SettingsManager.h"
#include "../client/StringTokenizer.h"

#include "WinUtil.h"

PropPage::TextItem AppearancePage::texts[] = {
	{ IDC_SETTINGS_APPEARANCE_OPTIONS, ResourceManager::SETTINGS_OPTIONS },
	{ IDC_SETTINGS_DEFAULT_AWAY_MSG, ResourceManager::SETTINGS_DEFAULT_AWAY_MSG },
	{ IDC_SETTINGS_TIME_STAMPS_FORMAT, ResourceManager::SETTINGS_TIME_STAMPS_FORMAT },
	{ IDC_SETTINGS_LANGUAGE_FILE, ResourceManager::SETTINGS_LANGUAGE_FILE },
	{ IDC_BROWSE, ResourceManager::BROWSE_ACCEL },
	{ IDC_SETCZDC_PM_LINES, ResourceManager::SETCZDC_PM_LINES },
	{ IDC_SETTINGS_REQUIRES_RESTART, ResourceManager::SETTINGS_REQUIRES_RESTART },
	{ IDC_SETTINGS_GET_USER_COUNTRY, ResourceManager::SETTINGS_GET_USER_COUNTRY }, 
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item AppearancePage::items[] = {
	{ IDC_DEFAULT_AWAY_MESSAGE, SettingsManager::DEFAULT_AWAY_MESSAGE, PropPage::T_STR },
	{ IDC_TIME_STAMPS_FORMAT, SettingsManager::TIME_STAMPS_FORMAT, PropPage::T_STR },
	{ IDC_LANGUAGE, SettingsManager::LANGUAGE_FILE, PropPage::T_STR },
	{ IDC_PM_LINES, SettingsManager::PM_LOG_LINES, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

PropPage::ListItem AppearancePage::listItems[] = {
	{ SettingsManager::SHOW_PROGRESS_BARS, ResourceManager::SETTINGS_SHOW_PROGRESS_BARS },
	{ SettingsManager::SHOW_INFOTIPS , ResourceManager::SETTINGS_SHOW_INFO_TIPS },
	{ SettingsManager::MINIMIZE_TRAY, ResourceManager::SETTINGS_MINIMIZE_TRAY },
	{ SettingsManager::TIME_STAMPS, ResourceManager::SETTINGS_TIME_STAMPS },
	{ SettingsManager::STATUS_IN_CHAT, ResourceManager::SETTINGS_STATUS_IN_CHAT },
	{ SettingsManager::SHOW_JOINS, ResourceManager::SETTINGS_SHOW_JOINS },
	{ SettingsManager::FAV_SHOW_JOINS, ResourceManager::SETTINGS_FAV_SHOW_JOINS },
	{ SettingsManager::FILTER_MESSAGES, ResourceManager::SETTINGS_FILTER_MESSAGES },
	{ SettingsManager::FINISHED_DIRTY, ResourceManager::SETTINGS_FINISHED_DIRTY },
	{ SettingsManager::QUEUE_DIRTY, ResourceManager::SETTINGS_QUEUE_DIRTY },
	{ SettingsManager::TAB_DIRTY, ResourceManager::SETTINGS_TAB_DIRTY },
	{ SettingsManager::EXPAND_QUEUE , ResourceManager::SETTINGS_EXPAND_QUEUE },
	{ SettingsManager::USE_SYSTEM_ICONS, ResourceManager::SETTINGS_USE_SYSTEM_ICONS },
	{ SettingsManager::USE_OEM_MONOFONT, ResourceManager::SETTINGS_USE_OEM_MONOFONT },
	{ SettingsManager::SHOW_SHARED , ResourceManager::SETTINGS_SHOW_SHARED },
	{ SettingsManager::SHOW_EXACT_SHARED , ResourceManager::SETTINGS_SHOW_EXACT_SHARED },
	{ SettingsManager::SHOW_DESCRIPTION , ResourceManager::SETTINGS_SHOW_DESCRIPTION },
	{ SettingsManager::SHOW_TAG , ResourceManager::SETTINGS_SHOW_TAG },
	{ SettingsManager::SHOW_CONNECTION , ResourceManager::SETTINGS_SHOW_CONNECTION },
	{ SettingsManager::SHOW_EMAIL , ResourceManager::SETTINGS_SHOW_EMAIL },
	{ SettingsManager::SHOW_CLIENT , ResourceManager::SETTINGS_CLIENT },
	{ SettingsManager::SHOW_VERSION , ResourceManager::SETTINGS_SHOW_VERSION },
	{ SettingsManager::SHOW_MODE , ResourceManager::SETTINGS_SHOW_MODE },
	{ SettingsManager::SHOW_HUBS , ResourceManager::SETTINGS_SHOW_HUBS },
	{ SettingsManager::SHOW_SLOTS , ResourceManager::SETTINGS_SHOW_SLOTS },
	{ SettingsManager::SHOW_UPLOAD , ResourceManager::SETTINGS_SHOW_UPLOAD },
	{ SettingsManager::SHOW_IP , ResourceManager::SETTINGS_SHOW_IP },
	{ SettingsManager::SHOW_ISP , ResourceManager::SHOW_ISP },
	{ SettingsManager::SHOW_PK , ResourceManager::SHOW_PK },
	{ SettingsManager::SHOW_LOCK , ResourceManager::SHOW_LOCK },
	{ SettingsManager::SHOW_SUPPORTS , ResourceManager::SHOW_SUPPORTS },
	{ SettingsManager::GET_USER_COUNTRY, ResourceManager::SETTINGS_GET_USER_COUNTRY },
	{ SettingsManager::SHOW_PM_LOG , ResourceManager::SETCZDC_PM_LOG },	
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

AppearancePage::~AppearancePage(){ delete[] title; }

LRESULT AppearancePage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);

	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_APPEARANCE_BOOLEANS));

	CUpDownCtrl spin;
	spin.Attach(GetDlgItem(IDC_PM_LINESSPIN));
	spin.SetRange32(1, 999);
	spin.Detach();

	// Do specialized reading here
	return TRUE;
}

void AppearancePage::write() {
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_APPEARANCE_BOOLEANS));
}

LRESULT AppearancePage::onBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	char buf[MAX_PATH];
	static const char types[] = "Language Files\0*.xml\0All Files\0*.*\0";

	GetDlgItemText(IDC_LANGUAGE, buf, MAX_PATH);
	string x = buf;

	if(WinUtil::browseFile(x, m_hWnd, false, Util::getAppPath(), types) == IDOK) {
		SetDlgItemText(IDC_LANGUAGE, x.c_str());
	}
	return 0;
}

LRESULT AppearancePage::onClickedHelp(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	MessageBox(CSTRING(TIMESTAMP_HELP), CSTRING(TIMESTAMP_HELP_DESC), MB_OK | MB_ICONINFORMATION);
	return S_OK;
}

/**
 * @file
 * $Id$
 */
