
#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"
#include "UserListColours.h"
#include "../client/SettingsManager.h"
#include "WinUtil.h"

PropPage::TextItem UserListColours::texts[] = {
	{ IDC_CHANGE_COLOR, ResourceManager::SETTINGS_CHANGE },
	{ IDC_USERLIST, ResourceManager::USERLIST_ICONS },
	{ IDC_IMAGEBROWSE2, ResourceManager::BROWSE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item UserListColours::items[] = {
	{ IDC_USERLIST_IMAGE, SettingsManager::USERLIST_IMAGE, PropPage::T_STR },
	{ 0, 0, PropPage::T_END }
};

LRESULT UserListColours::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	normalColour = SETTING(NORMAL_COLOUR);
	favoriteColour = SETTING(FAVORITE_COLOR);
	reservedSlotColour = SETTING(RESERVED_SLOT_COLOR);
	ignoredColour = SETTING(IGNORED_COLOR);
	clientCheckedColour = SETTING(CLIENT_CHECKED_COLOUR);
	fileListCheckedColour = SETTING(FILELIST_CHECKED_COLOUR);
	fullCheckedColour = SETTING(FULL_CHECKED_COLOUR);
	badClientColour = SETTING(BAD_CLIENT_COLOUR);
	badFilelistColour = SETTING(BAD_FILELIST_COLOUR);

	n_lsbList.Attach( GetDlgItem(IDC_USERLIST_COLORS) );
	n_Preview.Attach( GetDlgItem(IDC_PREVIEW) );
	n_Preview.SetBackgroundColor(WinUtil::bgColor);
	n_lsbList.AddString(CTSTRING(SETTINGS_COLOR_NORMAL));
	n_lsbList.AddString(CTSTRING(SETTINGS_COLOR_FAVORITE));	
	n_lsbList.AddString(CTSTRING(SETTINGS_COLOR_RESERVED));
	n_lsbList.AddString(CTSTRING(SETTINGS_COLOR_IGNORED));
	n_lsbList.AddString(CTSTRING(SETTINGS_COLOR_CLIENT_CHECKED));
	n_lsbList.AddString(CTSTRING(SETTINGS_COLOR_FILELIST_CHECKED));
	n_lsbList.AddString(CTSTRING(SETTINGS_COLOR_FULL_CHECKED));
	n_lsbList.AddString(CTSTRING(SETTINGS_COLOR_BAD_CLIENT));
	n_lsbList.AddString(CTSTRING(SETTINGS_COLOR_BAD_FILELIST));	
	n_lsbList.SetCurSel( 0 );

	refreshPreview();

	return TRUE;
}
LRESULT UserListColours::onChangeColour(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/) {
	int index = n_lsbList.GetCurSel();
	if(index == -1) return 0;
	int colour = 0;
	switch(index) {
		case 0: colour = normalColour; break;
		case 1: colour = favoriteColour; break;
		case 2: colour = reservedSlotColour; break;
		case 3: colour = ignoredColour; break;
		case 4: colour = clientCheckedColour; break;
		case 5: colour = fileListCheckedColour; break;
		case 6: colour = fullCheckedColour; break;
		case 7: colour = badClientColour; break;
		case 8: colour = badFilelistColour; break;
		default: break;
	}
	CColorDialog d( colour, 0, hWndCtl );
	if (d.DoModal() == IDOK) {
		switch(index) {
			case 0: normalColour = d.GetColor(); break;
			case 1: favoriteColour = d.GetColor(); break;
			case 2: reservedSlotColour = d.GetColor(); break;
			case 3: ignoredColour = d.GetColor(); break;
			case 4: clientCheckedColour = d.GetColor(); break;
			case 5: fileListCheckedColour = d.GetColor(); break;
			case 6: fullCheckedColour = d.GetColor(); break;
			case 7: badClientColour = d.GetColor(); break;
			case 8: badFilelistColour = d.GetColor(); break;
			default: break;
		}
		refreshPreview();
	}
	return TRUE;
}

void UserListColours::refreshPreview() {
	CHARFORMAT2 cf;
	n_Preview.SetWindowText(_T(""));
	cf.dwMask = CFM_COLOR;
	cf.dwEffects = 0;
	
	cf.crTextColor = normalColour;
	n_Preview.SetSelectionCharFormat(cf);
	n_Preview.AppendText(TSTRING(SETTINGS_COLOR_NORMAL).c_str());

	cf.crTextColor = favoriteColour;
	n_Preview.SetSelectionCharFormat(cf);
	n_Preview.AppendText((_T("\r\n") + TSTRING(SETTINGS_COLOR_FAVORITE)).c_str());
	
	cf.crTextColor = reservedSlotColour;
	n_Preview.SetSelectionCharFormat(cf);
	n_Preview.AppendText((_T("\r\n") + TSTRING(SETTINGS_COLOR_RESERVED)).c_str());
	
	cf.crTextColor = ignoredColour;
	n_Preview.SetSelectionCharFormat(cf);
	n_Preview.AppendText((_T("\r\n") + TSTRING(SETTINGS_COLOR_IGNORED)).c_str());
	
	cf.crTextColor = clientCheckedColour;
	n_Preview.SetSelectionCharFormat(cf);
	n_Preview.AppendText((_T("\r\n") + TSTRING(SETTINGS_COLOR_CLIENT_CHECKED)).c_str());

	cf.crTextColor = fileListCheckedColour;
	n_Preview.SetSelectionCharFormat(cf);
	n_Preview.AppendText((_T("\r\n") + TSTRING(SETTINGS_COLOR_FILELIST_CHECKED)).c_str());

	cf.crTextColor = fullCheckedColour;
	n_Preview.SetSelectionCharFormat(cf);
	n_Preview.AppendText((_T("\r\n") + TSTRING(SETTINGS_COLOR_FULL_CHECKED)).c_str());

	cf.crTextColor = badClientColour;
	n_Preview.SetSelectionCharFormat(cf);
	n_Preview.AppendText((_T("\r\n") + TSTRING(SETTINGS_COLOR_BAD_CLIENT)).c_str());

	cf.crTextColor = badFilelistColour;
	n_Preview.SetSelectionCharFormat(cf);
	n_Preview.AppendText((_T("\r\n") + TSTRING(SETTINGS_COLOR_BAD_FILELIST)).c_str());

	n_Preview.InvalidateRect( NULL );
}

void UserListColours::write() {
	PropPage::write((HWND)*this, items);
	SettingsManager::getInstance()->set(SettingsManager::NORMAL_COLOUR, normalColour);
	SettingsManager::getInstance()->set(SettingsManager::FAVORITE_COLOR, favoriteColour);
	SettingsManager::getInstance()->set(SettingsManager::RESERVED_SLOT_COLOR, reservedSlotColour);
	SettingsManager::getInstance()->set(SettingsManager::IGNORED_COLOR, ignoredColour);
	SettingsManager::getInstance()->set(SettingsManager::CLIENT_CHECKED_COLOUR, clientCheckedColour);
	SettingsManager::getInstance()->set(SettingsManager::FILELIST_CHECKED_COLOUR, fileListCheckedColour);
	SettingsManager::getInstance()->set(SettingsManager::FULL_CHECKED_COLOUR, fullCheckedColour);
	SettingsManager::getInstance()->set(SettingsManager::BAD_CLIENT_COLOUR, badClientColour);
	SettingsManager::getInstance()->set(SettingsManager::BAD_FILELIST_COLOUR, badFilelistColour);

	WinUtil::reLoadImages(); // User Icon Begin / End

}

void UserListColours::BrowseForPic(int DLGITEM) {
	TCHAR buf[MAX_PATH];

	GetDlgItemText(DLGITEM, buf, MAX_PATH);
	tstring x = buf;

	if(WinUtil::browseFile(x, m_hWnd, false) == IDOK) {
		SetDlgItemText(DLGITEM, x.c_str());
	}
}

LRESULT UserListColours::onImageBrowse(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	BrowseForPic(IDC_USERLIST_IMAGE);
	return 0;
}
/**
 * @file
 * $Id$
 */