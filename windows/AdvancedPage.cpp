/* 
 * Copyright (C) 2001-2004 Jacek Sieka, j_s at telia com
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

#include "AdvancedPage.h"
#include "CommandDlg.h"

#include "../client/SettingsManager.h"
#include "../client/HubManager.h"
#include "WinUtil.h"

PropPage::TextItem AdvancedPage::texts[] = {
	{ IDC_SETTINGS_ADVANCED, ResourceManager::SETTINGS_ADVANCED_SETTINGS },
	{ IDC_SETTINGS_ROLLBACK, ResourceManager::SETTINGS_ROLLBACK },
	{ IDC_SETTINGS_B, ResourceManager::B },
	{ IDC_SETTINGS_WRITE_BUFFER, ResourceManager::SETTINGS_WRITE_BUFFER },
	{ IDC_SETTINGS_KB, ResourceManager::KB },
	{ IDC_SETTINGS_MAX_TAB_ROWS, ResourceManager::SETTINGS_MAX_TAB_ROWS },
	{ IDC_SETTINGS_MAX_HASH_SPEED, ResourceManager::SETTINGS_MAX_HASH_SPEED },
	{ IDC_SETTINGS_MBS, ResourceManager::MBPS },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item AdvancedPage::items[] = {
	{ IDC_ROLLBACK, SettingsManager::ROLLBACK, PropPage::T_INT }, 
	{ IDC_BUFFERSIZE, SettingsManager::BUFFER_SIZE, PropPage::T_INT },
	{ IDC_MAX_TAB_ROWS, SettingsManager::MAX_TAB_ROWS, PropPage::T_INT },
	{ IDC_MAX_HASH_SPEED, SettingsManager::MAX_HASH_SPEED, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

AdvancedPage::ListItem AdvancedPage::listItems[] = {
	{ SettingsManager::AUTO_AWAY, ResourceManager::SETTINGS_AUTO_AWAY },
	{ SettingsManager::AUTO_FOLLOW, ResourceManager::SETTINGS_AUTO_FOLLOW },
	{ SettingsManager::CLEAR_SEARCH, ResourceManager::SETTINGS_CLEAR_SEARCH },
	{ SettingsManager::GET_UPDATE_INFO, ResourceManager::SETCZDC_GET_DC_UPDATE },
	{ SettingsManager::MINIMIZE_ON_STARTUP, ResourceManager::SETTINGS_MINIMIZE_ON_STARTUP },
	{ SettingsManager::FREE_SLOTS_DEFAULT, ResourceManager::SETTINGS_FREE_SLOT_DEFAULT },
	{ SettingsManager::USE_EXTENSION_DOWNTO, ResourceManager::SETTINGS_USE_EXTENSION_DOWNTO },
	{ SettingsManager::LIST_DUPES, ResourceManager::SETTINGS_LIST_DUPES },
	{ SettingsManager::REMOVE_FORBIDDEN, ResourceManager::SETCZDC_REMOVE_FORBIDDEN },
	{ SettingsManager::URL_HANDLER, ResourceManager::SETTINGS_URL_HANDLER },
	{ SettingsManager::IGNORE_OFFLINE, ResourceManager::SETTINGS_IGNORE_OFFLINE },
	{ SettingsManager::MAGNET_REGISTER, ResourceManager::SETCZDC_MAGNET_URI_HANDLER },
	{ SettingsManager::SMALL_SEND_BUFFER, ResourceManager::SETTINGS_SMALL_SEND_BUFFER },
	{ SettingsManager::CONFIRM_DELETE, ResourceManager::SETTINGS_CONFIRM_DELETE },
	{ SettingsManager::KEEP_LISTS, ResourceManager::SETTINGS_KEEP_LISTS },
	{ SettingsManager::AUTO_KICK, ResourceManager::SETTINGS_AUTO_KICK },
	{ SettingsManager::SFV_CHECK, ResourceManager::SETTINGS_SFV_CHECK },
	{ SettingsManager::AUTO_UPDATE_LIST, ResourceManager::SETTINGS_AUTO_UPDATE_LIST },
	{ SettingsManager::ANTI_FRAG, ResourceManager::SETTINGS_ANTI_FRAG },
	{ SettingsManager::NO_AWAYMSG_TO_BOTS, ResourceManager::SETTINGS_NO_AWAYMSG_TO_BOTS },
	{ SettingsManager::SKIP_ZERO_BYTE, ResourceManager::SETTINGS_SKIP_ZERO_BYTE },
	{ SettingsManager::ADLS_BREAK_ON_FIRST, ResourceManager::SETTINGS_ADLS_BREAK_ON_FIRST },
	{ SettingsManager::TAB_COMPLETION, ResourceManager::SETTINGS_TAB_COMPLETION },
	{ SettingsManager::COMPRESS_TRANSFERS, ResourceManager::SETTINGS_COMPRESS_TRANSFERS },
	{ SettingsManager::HUB_USER_COMMANDS, ResourceManager::SETTINGS_HUB_USER_COMMANDS },
	{ SettingsManager::AUTO_SEARCH_AUTO_MATCH, ResourceManager::SETTINGS_AUTO_SEARCH_AUTO_MATCH },
	{ SettingsManager::LOG_FILELIST_TRANSFERS, ResourceManager::SETTINGS_LOG_FILELIST_TRANSFERS },
	{ SettingsManager::SEARCH_PASSIVE, ResourceManager::SETCZDC_PASSIVE_SEARCH },
	{ SettingsManager::CONFIRM_EXIT, ResourceManager::SETTINGS_CONFIRM_EXIT },
	{ SettingsManager::SEND_UNKNOWN_COMMANDS, ResourceManager::SETTINGS_SEND_UNKNOWN_COMMANDS },
	{ SettingsManager::ADD_FINISHED_INSTANTLY, ResourceManager::ADD_FINISHED_INSTANTLY },
	{ SettingsManager::SETTINGS_USE_UPNP, ResourceManager::SETTINGS_USE_UPNP },
	{ SettingsManager::DONT_DL_ALREADY_SHARED, ResourceManager::DONT_DL_ALREADY_SHARED },
	{ SettingsManager::SETTINGS_USE_CTRL_FOR_LINE_HISTORY, ResourceManager::SETTINGS_USE_CTRL_FOR_LINE_HISTORY },
	{ SettingsManager::SEARCH_ONLY_TTH, ResourceManager::SETTINGS_ONLY_TTH },
	{ SettingsManager::ADVANCED_RESUME, ResourceManager::SETTINGS_ADVANCED_RESUME },
	{ SettingsManager::TOGGLE_ACTIVE_WINDOW, ResourceManager::SETTINGS_TOGGLE_ACTIVE_WINDOW },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT AdvancedPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_ADVANCED_BOOLEANS));

	// Do specialized reading here
	return TRUE;
}

void AdvancedPage::write() {
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_ADVANCED_BOOLEANS));
}

/**
 * @file
 * $Id$
 */
