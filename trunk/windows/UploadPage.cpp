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

#include "UploadPage.h"
#include "WinUtil.h"

#include "../client/Util.h"
#include "../client/ShareManager.h"
#include "../client/SettingsManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PropPage::TextItem UploadPage::texts[] = {
	{ IDC_SETTINGS_SHARED_DIRECTORIES, ResourceManager::SETTINGS_SHARED_DIRECTORIES },
	{ IDC_SETTINGS_SHARE_SIZE, ResourceManager::SETTINGS_SHARE_SIZE }, 
	{ IDC_SHAREHIDDEN, ResourceManager::SETTINGS_SHARE_HIDDEN },
	{ IDC_REMOVE, ResourceManager::REMOVE },
	{ IDC_ADD, ResourceManager::SETTINGS_ADD_FOLDER },
	{ IDC_SETTINGS_UPLOADS_MIN_SPEED, ResourceManager::SETTINGS_UPLOADS_MIN_SPEED },
	{ IDC_SETTINGS_KBPS, ResourceManager::KBPS }, 
	{ IDC_SETTINGS_UPLOADS_SLOTS, ResourceManager::SETTINGS_UPLOADS_SLOTS },
	{ IDC_CZDC_SMALL_SLOTS, ResourceManager::SETCZDC_SMALL_UP_SLOTS },
	{ IDC_CZDC_SMALL_SIZE, ResourceManager::SETCZDC_SMALL_FILES },
	{ IDC_CZDC_NOTE_SMALL, ResourceManager::SETCZDC_NOTE_SMALL_UP },
	{ IDC_STATICb, ResourceManager::EXTRA_HUB_SLOTS },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item UploadPage::items[] = {
	{ IDC_SLOTS, SettingsManager::SLOTS, PropPage::T_INT }, 
	{ IDC_SHAREHIDDEN, SettingsManager::SHARE_HIDDEN, PropPage::T_BOOL },
	{ IDC_MIN_UPLOAD_SPEED, SettingsManager::MIN_UPLOAD_SPEED, PropPage::T_INT },
	{ IDC_EXTRA_SLOTS, SettingsManager::EXTRA_SLOTS, PropPage::T_INT },
	{ IDC_SMALL_FILE_SIZE, SettingsManager::SMALL_FILE_SIZE, PropPage::T_INT },
	{ IDC_EXTRA_SLOTS2, SettingsManager::HUB_SLOTS, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

LRESULT UploadPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);

/* POSSUM_MOD_BEGIN */
	GetDlgItem(IDC_TREE1).ShowWindow((BOOLSETTING(USE_OLD_SHARING_UI)) ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_DIRECTORIES).ShowWindow((BOOLSETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_ADD).ShowWindow((BOOLSETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_REMOVE).ShowWindow((BOOLSETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);
/* POSSUM_MOD_END */

	ctrlDirectories.Attach(GetDlgItem(IDC_DIRECTORIES));
		ctrlDirectories.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
		
	ctrlTotal.Attach(GetDlgItem(IDC_TOTAL));

	PropPage::read((HWND)*this, items);

/* POSSUM_MOD_BEGIN */
	if(BOOLSETTING(USE_OLD_SHARING_UI))
	{
	// Prepare shared dir list
	ctrlDirectories.InsertColumn(0, CSTRING(DIRECTORY), LVCFMT_LEFT, 277, 0);
	ctrlDirectories.InsertColumn(1, CSTRING(SIZE), LVCFMT_RIGHT, 90, 1);
	StringList directories = ShareManager::getInstance()->getDirectories();
	for(StringIter j = directories.begin(); j != directories.end(); j++)
	{
		int i = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), *j);
		ctrlDirectories.SetItemText(i, 1, Util::formatBytes(ShareManager::getInstance()->getShareSize(*j)).c_str());
	}
	}
/* POSSUM_MOD_END */
	ctrlTotal.SetWindowText(Util::formatBytes(ShareManager::getInstance()->getShareSize()).c_str());

	CUpDownCtrl updown;
	updown.Attach(GetDlgItem(IDC_SLOTSPIN));
	updown.SetRange(1, 100);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_MIN_UPLOAD_SPIN));
	updown.SetRange32(0, 30000);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_EXTRA_SLOTS_SPIN));
	updown.SetRange(3, 100);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_SMALL_FILE_SIZE_SPIN));
	updown.SetRange32(64, 30000);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_EXTRASPIN));
	updown.SetRange(0,10);

/* POSSUM_MOD_BEGIN */
	ft.SubclassWindow(GetDlgItem(IDC_TREE1));
	ft.SetStaticCtrl(&ctrlTotal);
	if(!BOOLSETTING(USE_OLD_SHARING_UI))
		ft.PopulateTree();
/* POSSUM_MOD_END */

	return TRUE;
}

LRESULT UploadPage::onDropFiles(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/){
	HDROP drop = (HDROP)wParam;
	TCHAR buf[MAX_PATH];
	UINT nrFiles;
	
	nrFiles = DragQueryFile(drop, (UINT)-1, NULL, 0);
	
	for(UINT i = 0; i < nrFiles; ++i){
		if(DragQueryFile(drop, i, buf, MAX_PATH)){
			if(PathIsDirectory(buf))
				addDirectory(buf);
		}
	}

	DragFinish(drop);

	return 0;
}


void UploadPage::write()
{
	PropPage::write((HWND)*this, items);

	if(SETTING(SLOTS) < 1)
		settings->set(SettingsManager::SLOTS, 1);

	// Do specialized writing here
	if(SETTING(EXTRA_SLOTS) < 3)
		settings->set(SettingsManager::EXTRA_SLOTS, 3);

	if(SETTING(SMALL_FILE_SIZE) < 64)
		settings->set(SettingsManager::SMALL_FILE_SIZE, 64);

	if( SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL) > 0) {
		if( SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL) < ((2 * SETTING(SLOTS)) + 3) ) {
			settings->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT_NORMAL, ((2 * SETTING(SLOTS)) + 3) );
		}
		if ( (SETTING(MAX_DOWNLOAD_SPEED_LIMIT_NORMAL) > ( SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL) * 6)) || ( SETTING(MAX_DOWNLOAD_SPEED_LIMIT_NORMAL) == 0) ) {
			settings->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT_NORMAL, (SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL)*6) );
		}
	}

	if(SETTING(HUB_SLOTS) < 0)
	settings->set(SettingsManager::HUB_SLOTS, 0);

	if( SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME) > 0) {
		if( SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME) < ((2 * SETTING(SLOTS)) + 3) ) {
			settings->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT_TIME, ((2 * SETTING(SLOTS)) + 3) );
		}
		if ( (SETTING(MAX_DOWNLOAD_SPEED_LIMIT_TIME) > ( SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME) * 6)) || ( SETTING(MAX_DOWNLOAD_SPEED_LIMIT_TIME) == 0) ) {
			settings->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT_TIME, (SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME)*6) );
		}
	}

	/* POSSUM_MOD_BEGIN */
	if(!BOOLSETTING(USE_OLD_SHARING_UI) && ft.IsDirty())
	{
		ShareManager::getInstance()->setDirty();
		ShareManager::getInstance()->refresh(true);
	}
	/* POSSUM_MOD_END */
}

LRESULT UploadPage::onItemchangedDirectories(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	NM_LISTVIEW* lv = (NM_LISTVIEW*) pnmh;
	::EnableWindow(GetDlgItem(IDC_REMOVE), (lv->uNewState & LVIS_FOCUSED));
	return 0;		
}

LRESULT UploadPage::onClickedAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	string target;
	if(WinUtil::browseDirectory(target, (HWND) *this)) {
		addDirectory(target);
	}
	
	return 0;
}

LRESULT UploadPage::onClickedRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	char buf[MAX_PATH];
	LVITEM item;
	::ZeroMemory(&item, sizeof(item));
	item.mask = LVIF_TEXT;
	item.cchTextMax = sizeof(buf);
	item.pszText = buf;
	if(ctrlDirectories.GetSelectedItem(&item)) {
		ShareManager::getInstance()->removeDirectory(buf);
		ctrlTotal.SetWindowText(Util::formatBytes(ShareManager::getInstance()->getShareSize()).c_str());
		ctrlDirectories.DeleteItem(item.iItem);
	}
	
	return 0;
}

LRESULT UploadPage::onClickedShareHidden(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// Save the checkbox state so that ShareManager knows to include/disclude hidden files
	Item i = items[1]; // The checkbox. Explicit index used - bad!
	if(::IsDlgButtonChecked((HWND)* this, i.itemID) == BST_CHECKED){
		settings->set((SettingsManager::IntSetting)i.setting, true);
	} else {
		settings->set((SettingsManager::IntSetting)i.setting, false);
	}

	// Refresh the share. This is a blocking refresh. Might cause problems?
	// Hopefully people won't click the checkbox enough for it to be an issue. :-)
	ShareManager::getInstance()->setDirty();
	ShareManager::getInstance()->refresh(true, false, true);

/* POSSUM_MOD_BEGIN */
	if(BOOLSETTING(USE_OLD_SHARING_UI))
	{
	// Clear the GUI list, for insertion of updated shares
	ctrlDirectories.DeleteAllItems();
	StringList directories = ShareManager::getInstance()->getDirectories();
	for(StringIter j = directories.begin(); j != directories.end(); j++)
	{
		int i = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), *j);
		ctrlDirectories.SetItemText(i, 1, Util::formatBytes(ShareManager::getInstance()->getShareSize(*j)).c_str());
	}
	}
/* POSSUM_MOD_END */
	// Display the new total share size
	ctrlTotal.SetWindowText(Util::formatBytes(ShareManager::getInstance()->getShareSize()).c_str());
	return 0;
}

void UploadPage::addDirectory(string path){
	try {
		ShareManager::getInstance()->addDirectory(path);
		int i = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), path);
		ctrlDirectories.SetItemText(i, 1, Util::formatBytes(ShareManager::getInstance()->getShareSize(path)).c_str());
		ctrlTotal.SetWindowText(Util::formatBytes(ShareManager::getInstance()->getShareSize()).c_str());
	} catch(const ShareException& e) {
		MessageBox(e.getError().c_str(), APPNAME " " VERSIONSTRING, MB_ICONSTOP | MB_OK);
	}
}

