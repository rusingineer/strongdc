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

#include "SDCPage.h"
#include "../client/SettingsManager.h"
#include "WinUtil.h"

PropPage::TextItem SDCPage::texts[] = {
	{ IDC_CZDC_FEAT, ResourceManager::SETCZDC_FEAT },
	{ IDC_STATIC1, ResourceManager::PORT },
	{ IDC_STATIC2, ResourceManager::USER },
	{ IDC_STATIC3, ResourceManager::PASSWORD },
	{ IDC_SETTINGS_ODC_SHUTDOWNTIMEOUT, ResourceManager::SETTINGS_ODC_SHUTDOWNTIMEOUT },
	{ IDC_MAXCOMPRESS, ResourceManager::SETTINGS_MAX_COMPRESS },
	{ IDC_MAXSOURCES, ResourceManager::SETTINGS_MAX_SOURCES },
	{ IDC_SETCZDC_MAX_EMOTICONS, ResourceManager::SETCZDC_MAX_EMOTICONS },
	{ IDC_SAVEQUEUE_TEXT, ResourceManager::SETTINGS_SAVEQUEUE },
	{ IDC_SECOND, ResourceManager::SECONDS },
	{ IDC_SECOND1, ResourceManager::SECONDS },
	{ IDC_SECOND2, ResourceManager::SECONDS },
	{ IDC_INTERVAL_TEXT, ResourceManager::MINIMUM_SEARCH_INTERVAL },
	{ IDC_MATCH_QUEUE_TEXT, ResourceManager::SETTINGS_SB_MAX_SOURCES },
	{ IDC_USERLISTDBLCLICKACTION, ResourceManager::USERLISTDBLCLICKACTION },
	{ IDC_TRANSFERLISTDBLCLICKACTION, ResourceManager::TRANSFERLISTDBLCLICKACTION },	
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item SDCPage::items[] = {
	{ IDC_EDIT1, SettingsManager::WEBSERVER_PORT, PropPage::T_INT }, 
	{ IDC_EDIT2, SettingsManager::WEBSERVER_USER, PropPage::T_STR }, 
	{ IDC_EDIT3, SettingsManager::WEBSERVER_PASS, PropPage::T_STR }, 
	{ IDC_SHUTDOWNTIMEOUT, SettingsManager::SHUTDOWN_TIMEOUT, PropPage::T_INT },
	{ IDC_MAX_COMPRESSION, SettingsManager::MAX_COMPRESSION, PropPage::T_INT },
	{ IDC_MAX_SOURCES, SettingsManager::MAX_SOURCES, PropPage::T_INT },
	{ IDC_MAX_EMOTICONS, SettingsManager::MAX_EMOTICONS, PropPage::T_INT },
	{ IDC_SAVEQUEUE, SettingsManager::AUTOSAVE_QUEUE, PropPage::T_INT },
	{ IDC_INTERVAL, SettingsManager::MINIMUM_SEARCH_INTERVAL, PropPage::T_INT },
	{ IDC_MATCH, SettingsManager::MAX_AUTO_MATCH_SOURCES, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

LRESULT SDCPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	CUpDownCtrl updown;
	updown.Attach(GetDlgItem(IDC_SHUTDOWN_SPIN));
	updown.SetRange(1, 3600);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_MAX_COMP_SPIN));
	updown.SetRange(0, 9);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_MAX_SOURCES_SPIN));
	updown.SetRange(1, 500);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_MAX_EMOTICONSSPIN));
	updown.SetRange32(1, 999);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_SAVEQUEUE_SPIN));
	updown.SetRange32(5, 999);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_INTERVAL_SPIN));
	updown.SetRange32(5, 120);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_MATCH_SPIN));
	updown.SetRange32(1, 999);
	updown.Detach();

	ctrlShutdownAction.Attach(GetDlgItem(IDC_COMBO1));
	ctrlShutdownAction.AddString(CTSTRING(POWER_OFF));
	ctrlShutdownAction.AddString(CTSTRING(LOG_OFF));
	ctrlShutdownAction.AddString(CTSTRING(REBOOT));
	ctrlShutdownAction.AddString(CTSTRING(SUSPEND));
	ctrlShutdownAction.AddString(CTSTRING(HIBERNATE));

	ctrlShutdownAction.SetCurSel(SETTING(SHUTDOWN_ACTION));
	// Do specialized reading here

	userlistaction.Attach(GetDlgItem(IDC_USERLIST_DBLCLICK));
	transferlistaction.Attach(GetDlgItem(IDC_TRANSFERLIST_DBLCLICK));
	chataction.Attach(GetDlgItem(IDC_CHAT_DBLCLICK));

    userlistaction.AddString(CTSTRING(GET_FILE_LIST));
    userlistaction.AddString(CTSTRING(SEND_PUBLIC_MESSAGE));
    userlistaction.AddString(CTSTRING(SEND_PRIVATE_MESSAGE));
    userlistaction.AddString(CTSTRING(MATCH_QUEUE));
    userlistaction.AddString(CTSTRING(GRANT_EXTRA_SLOT));
	userlistaction.AddString(CTSTRING(ADD_TO_FAVORITES));
	transferlistaction.AddString(CTSTRING(SEND_PRIVATE_MESSAGE));
	transferlistaction.AddString(CTSTRING(GET_FILE_LIST));
	transferlistaction.AddString(CTSTRING(MATCH_QUEUE));
	transferlistaction.AddString(CTSTRING(GRANT_EXTRA_SLOT));
	transferlistaction.AddString(CTSTRING(ADD_TO_FAVORITES));
	chataction.AddString(CTSTRING(SELECT_USER_LIST));
    chataction.AddString(CTSTRING(SEND_PUBLIC_MESSAGE));
    chataction.AddString(CTSTRING(SEND_PRIVATE_MESSAGE));
    chataction.AddString(CTSTRING(GET_FILE_LIST));
    chataction.AddString(CTSTRING(MATCH_QUEUE));
    chataction.AddString(CTSTRING(GRANT_EXTRA_SLOT));
	chataction.AddString(CTSTRING(ADD_TO_FAVORITES));

	userlistaction.SetCurSel(SETTING(USERLIST_DBLCLICK));
	transferlistaction.SetCurSel(SETTING(TRANSFERLIST_DBLCLICK));
	chataction.SetCurSel(SETTING(CHAT_DBLCLICK));

	userlistaction.Detach();
	transferlistaction.Detach();
	chataction.Detach();
	
	return TRUE;
}

void SDCPage::write()
{
	PropPage::write((HWND)*this, items);
	SettingsManager::getInstance()->set(SettingsManager::SHUTDOWN_ACTION, ctrlShutdownAction.GetCurSel());

	userlistaction.Attach(GetDlgItem(IDC_USERLIST_DBLCLICK));
	transferlistaction.Attach(GetDlgItem(IDC_TRANSFERLIST_DBLCLICK));
	chataction.Attach(GetDlgItem(IDC_CHAT_DBLCLICK));
	settings->set(SettingsManager::USERLIST_DBLCLICK, userlistaction.GetCurSel());
	settings->set(SettingsManager::TRANSFERLIST_DBLCLICK, transferlistaction.GetCurSel());
	settings->set(SettingsManager::CHAT_DBLCLICK, chataction.GetCurSel());
	userlistaction.Detach();
	transferlistaction.Detach(); 
	chataction.Detach(); 
	// Do specialized writing here
	// settings->set(XX, YY);
}

/**
 * @file
 * $Id$
 */

