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

#include "Segment.h"
#include "../client/SettingsManager.h"
#include "WinUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


PropPage::TextItem Segment::texts[] = {
	{ IDC_AUTOSEGMENT, ResourceManager::SETTINGS_AUTO_SEARCH },
	{ IDC_SEGMENT2, ResourceManager::SEGMENT2_TEXT },
	{ IDC_SEGMENT3, ResourceManager::SEGMENT3_TEXT },
	{ IDC_SEGMENT4, ResourceManager::SEGMENT4_TEXT },
	{ IDC_SEGMENT6, ResourceManager::SEGMENT6_TEXT },
	{ IDC_SEGMENT8, ResourceManager::SEGMENT8_TEXT },
	{ IDC_DONTSEGMENT, ResourceManager::DONT_ADD_SEGMENT_TEXT },
	{ IDC_SEGMENTWARN, ResourceManager::WARN_SEGMENT_TEXT },

	{ IDC_RADIO1, ResourceManager::TEXT_FILESIZE },
	{ IDC_RADIO2, ResourceManager::TEXT_CONNECTION },
	{ IDC_RADIO3, ResourceManager::TEXT_MANUAL },
	{ IDC_EXTENSION, ResourceManager::TEXT_EXTENSION },
	{ IDC_MINIMUM, ResourceManager::TEXT_MINIMUM },
	{ IDC_KB, ResourceManager::KB },
	
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};
PropPage::Item Segment::items[] = {
	{ IDC_AUTOSEGMENT, SettingsManager::AUTO_SEARCH, PropPage::T_BOOL },
	{ IDC_SEGMENT2_MIN, SettingsManager::SET_MIN2, PropPage::T_INT },
	{ IDC_SEGMENT2_MAX, SettingsManager::SET_MAX2, PropPage::T_INT },
	{ IDC_SEGMENT3_MIN, SettingsManager::SET_MIN3, PropPage::T_INT },
	{ IDC_SEGMENT3_MAX, SettingsManager::SET_MAX3, PropPage::T_INT },
	{ IDC_SEGMENT4_MIN, SettingsManager::SET_MIN4, PropPage::T_INT },
	{ IDC_SEGMENT4_MAX, SettingsManager::SET_MAX4, PropPage::T_INT },
	{ IDC_SEGMENT6_MIN, SettingsManager::SET_MIN6, PropPage::T_INT },
	{ IDC_SEGMENT6_MAX, SettingsManager::SET_MAX6, PropPage::T_INT },
	{ IDC_SEGMENT8_MIN, SettingsManager::SET_MIN8, PropPage::T_INT },
//	{ IDC_SEGMENT2_MIN, SettingsManager::SET_MIN2, PropPage::T_INT },

	{ IDC_SEG_NUMBER, SettingsManager::NUMBER_OF_SEGMENTS, PropPage::T_INT },
	{ IDC_BLOCK_COMBO, SettingsManager::MIN_BLOCK_SIZE, PropPage::T_STR },

	{ IDC_EDIT1, SettingsManager::DONT_EXTENSIONS, PropPage::T_STR },

	{ 0, 0, PropPage::T_END }
};

LRESULT Segment::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);

	int const segType = settings->get(SettingsManager::SEGMENTS_TYPE);
	if(segType == SettingsManager::SEGMENT_ON_SIZE)
		CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO1);
	else if(segType == SettingsManager::SEGMENT_ON_CONNECTION)
		CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO2);
	else if(segType == SettingsManager::SEGMENT_MANUAL)
		CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO3);

	PropPage::read((HWND)*this, items);

	CUpDownCtrl spin;
	spin.Attach(GetDlgItem(IDC_SEG_NUMBER_SPIN));
	spin.SetRange32(1, 10);
	spin.Detach();

	ctrlBlockSize.Attach(GetDlgItem(IDC_BLOCK_COMBO));

	for(int i = 0; i < SettingsManager::SIZE_LAST; i++) {
		ctrlBlockSize.AddString(SettingsManager::blockSizes[i].c_str());
	}

	ctrlBlockSize.SetCurSel(ctrlBlockSize.FindString(0, SETTING(MIN_BLOCK_SIZE).c_str()));

	// Do specialized reading here
	
	return TRUE;
}

void Segment::write()
{
	PropPage::write((HWND)*this, items);
	
	int ct = -1;
	if(IsDlgButtonChecked(IDC_RADIO1))
		ct = SettingsManager::SEGMENT_ON_SIZE;
	else if(IsDlgButtonChecked(IDC_RADIO2))
		ct = SettingsManager::SEGMENT_ON_CONNECTION;
	else if(IsDlgButtonChecked(IDC_RADIO3))
		ct = SettingsManager::SEGMENT_MANUAL;

	if(SETTING(SEGMENTS_TYPE) != ct) settings->set(SettingsManager::SEGMENTS_TYPE, ct);

	// Do specialized writing here
	// settings->set(XX, YY);
}


/**
 * @file
 * $Id$
 */

