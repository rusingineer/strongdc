#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "FavoriteDirsPage.h"
#include "WinUtil.h"
#include "LineDlg.h"

#include "../client/Util.h"
#include "../client/SettingsManager.h"
#include "../client/HubManager.h"

PropPage::TextItem FavoriteDirsPage::texts[] = {
	{ IDC_SETTINGS_FAVORITE_DIRECTORIES, ResourceManager::SETTINGS_FAVORITE_DIRS },
	{ IDC_REMOVE, ResourceManager::REMOVE },
	{ IDC_ADD, ResourceManager::SETTINGS_ADD_FOLDER },
	{ IDC_RENAME, ResourceManager::SETTINGS_RENAME_FOLDER },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT FavoriteDirsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	ctrlDirectories.Attach(GetDlgItem(IDC_FAVORITE_DIRECTORIES));
	ctrlDirectories.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
		
	// Prepare shared dir list
	ctrlDirectories.InsertColumn(0, CTSTRING(FAVORITE_DIR_NAME), LVCFMT_LEFT, 80, 0);
	ctrlDirectories.InsertColumn(1, CTSTRING(DIRECTORY), LVCFMT_LEFT, 197, 1);
	StringPairList directories = HubManager::getInstance()->getFavoriteDirs();
	for(StringPairIter j = directories.begin(); j != directories.end(); j++)
	{
		int i = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), Text::toT(j->second));
		ctrlDirectories.SetItemText(i, 1, Text::toT(j->first).c_str());
	}
	
	return TRUE;
}


void FavoriteDirsPage::write()
{
//	PropPage::write((HWND)*this, items);
}

LRESULT FavoriteDirsPage::onDropFiles(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/){
	HDROP drop = (HDROP)wParam;
	AutoArray<TCHAR> buf(MAX_PATH);
	UINT nrFiles;
	
	nrFiles = DragQueryFile(drop, (UINT)-1, NULL, 0);
	
	for(UINT i = 0; i < nrFiles; ++i){
		if(DragQueryFile(drop, i, buf, MAX_PATH)){
			if(PathIsDirectory(buf))
				addDirectory(tstring(buf));
		}
	}

	DragFinish(drop);

	return 0;
}

LRESULT FavoriteDirsPage::onItemchangedDirectories(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	NM_LISTVIEW* lv = (NM_LISTVIEW*) pnmh;
	::EnableWindow(GetDlgItem(IDC_REMOVE), (lv->uNewState & LVIS_FOCUSED));
	::EnableWindow(GetDlgItem(IDC_RENAME), (lv->uNewState & LVIS_FOCUSED));
	return 0;		
}

LRESULT FavoriteDirsPage::onClickedAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	tstring target;
	if(WinUtil::browseDirectory(target, m_hWnd)) {
		addDirectory(target);
	}
	
	return 0;
}

LRESULT FavoriteDirsPage::onClickedRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR buf[MAX_PATH];
	LVITEM item;
	::ZeroMemory(&item, sizeof(item));
	item.mask = LVIF_TEXT;
	item.cchTextMax = sizeof(buf);
	item.pszText = buf;

	int i = -1;
	while((i = ctrlDirectories.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		item.iItem = i;
		item.iSubItem = 1;
		ctrlDirectories.GetItem(&item);
		if(HubManager::getInstance()->removeFavoriteDir(Text::fromT(buf)))
			ctrlDirectories.DeleteItem(i);
	}
	
	return 0;
}

LRESULT FavoriteDirsPage::onClickedRename(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR buf[MAX_PATH];
	LVITEM item;
	::ZeroMemory(&item, sizeof(item));
	item.mask = LVIF_TEXT;
	item.cchTextMax = sizeof(buf);
	item.pszText = buf;

	int i = -1;
	while((i = ctrlDirectories.GetNextItem(i, LVNI_SELECTED)) != -1) {
		item.iItem = i;
		item.iSubItem = 0;
		ctrlDirectories.GetItem(&item);

		LineDlg virt;
		virt.title = TSTRING(FAVORITE_DIR_NAME);
		virt.description = TSTRING(FAVORITE_DIR_NAME_LONG);
		virt.line = tstring(buf);
		if(virt.DoModal(m_hWnd) == IDOK) {
			if (HubManager::getInstance()->renameFavoriteDir(Text::fromT(buf), Text::fromT(virt.line))) {
				ctrlDirectories.SetItemText(i, 0, virt.line.c_str());
			} else {
				MessageBox(CTSTRING(DIRECTORY_ADD_ERROR));
			}
		}
	}
	return 0;
}


void FavoriteDirsPage::addDirectory(const tstring& aPath){
	tstring path = aPath;
	if( path[ path.length() -1 ] != PATH_SEPARATOR )
		path += PATH_SEPARATOR;

	LineDlg virt;
	virt.title = TSTRING(FAVORITE_DIR_NAME);
	virt.description = TSTRING(FAVORITE_DIR_NAME_LONG);
	virt.line = Util::getLastDir(path);
	if(virt.DoModal(m_hWnd) == IDOK) {
		if (HubManager::getInstance()->addFavoriteDir(Text::fromT(path), Text::fromT(virt.line))) {
			int j = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), virt.line );
			ctrlDirectories.SetItemText(j, 1, path.c_str());
		} else {
			MessageBox(CTSTRING(DIRECTORY_ADD_ERROR));
		}
	}
}

/**
 * @file
 * $Id$
 */