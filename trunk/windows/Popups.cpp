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

#include "Popups.h"
#include "../client/SettingsManager.h"
#include "WinUtil.h"
#include "MainFrm.h"

PropPage::TextItem Popups::texts[] = {
	{ IDC_PREVIEW, ResourceManager::SETCZDC_PREVIEW },
	{ IDC_POPUPTYPE, ResourceManager::POPUP_TYPE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item Popups::items[] = {
	{ 0, 0, PropPage::T_END }
};

Popups::ListItem Popups::listItems[] = {
	{ SettingsManager::POPUP_HUB_CONNECTED, ResourceManager::POPUP_HUB_CONNECTED },
	{ SettingsManager::POPUP_HUB_DISCONNECTED, ResourceManager::POPUP_HUB_DISCONNECTED },
	{ SettingsManager::POPUP_FAVORITE_CONNECTED, ResourceManager::POPUP_FAVORITE_CONNECTED },
	{ SettingsManager::POPUP_CHEATING_USER, ResourceManager::POPUP_CHEATING_USER },
	{ SettingsManager::POPUP_DOWNLOAD_START, ResourceManager::POPUP_DOWNLOAD_START },
	{ SettingsManager::POPUP_DOWNLOAD_FAILED, ResourceManager::POPUP_DOWNLOAD_FAILED },
	{ SettingsManager::POPUP_DOWNLOAD_FINISHED, ResourceManager::POPUP_DOWNLOAD_FINISHED },
	{ SettingsManager::POPUP_UPLOAD_FINISHED, ResourceManager::POPUP_UPLOAD_FINISHED },
	{ SettingsManager::POPUP_PM, ResourceManager::POPUP_PM },
	{ SettingsManager::POPUP_NEW_PM, ResourceManager::POPUP_NEW_PM },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT Popups::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_POPUPLIST));

	ctrlPopupType.Attach(GetDlgItem(IDC_COMBO1));
	ctrlPopupType.AddString("Balloon popup");
	ctrlPopupType.AddString("Window popup");
	ctrlPopupType.SetCurSel(SETTING(POPUP_TYPE));

	return TRUE;
}


void Popups::write()
{
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_POPUPLIST));

	SettingsManager::getInstance()->set(SettingsManager::POPUP_TYPE, ctrlPopupType.GetCurSel());
	// Do specialized writing here
	// settings->set(XX, YY);
}


LRESULT Popups::onPreview(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	MainFrame::getMainFrame()->ShowBalloonTip((
		STRING(FILE)+": sdc100rc8.rar\n"+
		STRING(USER)+": BigMuscle").c_str(), CSTRING(DOWNLOAD_FINISHED_IDLE));
	return 0;
}
/**
 * @file
 * $Id$
 */

