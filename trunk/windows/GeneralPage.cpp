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

#include "GeneralPage.h"
#include "../client/SettingsManager.h"
#include "../client/Socket.h"
#include "WinUtil.h"

PropPage::TextItem GeneralPage::texts[] = {
	{ IDC_SETTINGS_PERSONAL_INFORMATION, ResourceManager::SETTINGS_PERSONAL_INFORMATION },
	{ IDC_SETTINGS_NICK, ResourceManager::NICK },
	{ IDC_SETTINGS_EMAIL, ResourceManager::EMAIL },
	{ IDC_SETTINGS_DESCRIPTION, ResourceManager::DESCRIPTION },
	{ IDC_SETTINGS_UPLOAD_SPEED, ResourceManager::SETTINGS_UPLOAD_SPEED },
	{ IDC_SETTINGS_MEBIBYES, ResourceManager::MBITSPS },
	{ IDC_SHOW_SPEED_CHECK, ResourceManager::SHOW_SPEED },
	{ IDC_DU, ResourceManager::DU },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item GeneralPage::items[] = {
	{ IDC_NICK,			SettingsManager::NICK,			PropPage::T_STR }, 
	{ IDC_EMAIL,		SettingsManager::EMAIL,			PropPage::T_STR }, 
	{ IDC_DESCRIPTION,	SettingsManager::DESCRIPTION,	PropPage::T_STR }, 
	{ IDC_DOWN_COMBO,	SettingsManager::DOWN_SPEED,	PropPage::T_STR },  
	{ IDC_UP_COMBO,		SettingsManager::UP_SPEED,		PropPage::T_STR },  
	{ IDC_SHOW_SPEED_CHECK, SettingsManager::SHOW_DESCRIPTION_SPEED, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

void GeneralPage::write()
{
	PropPage::write((HWND)(*this), items);
	settings->set(SettingsManager::UPLOAD_SPEED, SettingsManager::connectionSpeeds[ctrlConnection.GetCurSel()]);
}

LRESULT GeneralPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	ctrlConnection.Attach(GetDlgItem(IDC_CONNECTION));
	ConnTypes.CreateFromImage(IDB_USERS, 16, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
    ctrlConnection.SetImageList(ConnTypes);	

	ctrlDownloadSpeed.Attach(GetDlgItem(IDC_DOWN_COMBO));
	ctrlUploadSpeed.Attach(GetDlgItem(IDC_UP_COMBO));

	for(int i = 0; i < SettingsManager::SP_LAST; i++) {
		ctrlDownloadSpeed.AddString(Text::toT(SettingsManager::speeds[i]).c_str());
		ctrlUploadSpeed.AddString(Text::toT(SettingsManager::speeds[i]).c_str());
	}

	ctrlDownloadSpeed.SetCurSel(ctrlDownloadSpeed.FindString(0, Text::toT(SETTING(DOWN_SPEED)).c_str()));
	ctrlUploadSpeed.SetCurSel(ctrlUploadSpeed.FindString(0, Text::toT(SETTING(UP_SPEED)).c_str()));

	int q = -1;
	for(size_t i = 0; i < SettingsManager::connectionSpeeds.size(); i++) {
		COMBOBOXEXITEM cbitem = {CBEIF_TEXT|CBEIF_IMAGE|CBEIF_SELECTEDIMAGE};
		tstring conn = Text::toT(SettingsManager::connectionSpeeds[i]); // oprava connections
		cbitem.pszText = const_cast<TCHAR*>(conn.c_str());
		cbitem.iItem = i; 


	/*	switch(i) {
			case 0: q = 1; break;
			case 1: q = 2; break;
			case 2: q = 3; break;
			case 3: q = 4; break;
			case 4: q = 6; break;
			case 5: q = 5; break;
			case 6: q = 7; break;
			case 7: q = 7; break;
		}*/
		
		cbitem.iImage = q;

		cbitem.iSelectedImage = q;
		ctrlConnection.InsertItem(&cbitem);

	}

	PropPage::read((HWND)(*this), items);

	int m = 0;
	for (m = 0; m<8; m++) {
		if(SettingsManager::connectionSpeeds[m] == SETTING(UPLOAD_SPEED)) break;
	}
	ctrlConnection.SetCurSel(m);

	nick.Attach(GetDlgItem(IDC_NICK));
	nick.LimitText(35);
	CEdit desc;
	desc.Attach(GetDlgItem(IDC_DESCRIPTION));
	desc.LimitText(50);
	desc.Detach();
	desc.Attach(GetDlgItem(IDC_SETTINGS_EMAIL));
	desc.LimitText(64);
	desc.Detach();
	desc.Detach();
	return TRUE;
}

LRESULT GeneralPage::onTextChanged(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
{
	TCHAR buf[SETTINGS_BUF_LEN];

	GetDlgItemText(wID, buf, SETTINGS_BUF_LEN);
	tstring old = buf;

	// Strip '$', '|', '<', '>' and ' ' from text
	TCHAR *b = buf, *f = buf, c;
	while( (c = *b++) != 0 )
	{
		if(c != '$' && c != '|' && (wID == IDC_DESCRIPTION || c != ' ') && ( (wID != IDC_NICK && wID != IDC_DESCRIPTION && wID != IDC_SETTINGS_EMAIL) || (c != '<' && c != '>') ) )
			*f++ = c;
	}

	*f = '\0';

	if(old != buf)
	{
		// Something changed; update window text without changing cursor pos
		CEdit tmp;
		tmp.Attach(hWndCtl);
		int start, end;
		tmp.GetSel(start, end);
		tmp.SetWindowText(buf);
		if(start > 0) start--;
		if(end > 0) end--;
		tmp.SetSel(start, end);
		tmp.Detach();
	}

	return TRUE;
}


/**
 * @file
 * $Id$
 */
