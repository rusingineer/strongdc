#ifndef FAVORITEDIRSPAGE_H
#define FAVORITEDIRSPAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"
#include "WinUtil.h"

class FavoriteDirsPage : public CPropertyPage<IDD_FAVORITE_DIRSPAGE>, public PropPage
{
public:
	FavoriteDirsPage(SettingsManager *s) : PropPage(s) { 
		title = _tcsdup((TSTRING(SETTINGS_GENERAL) + _T('\\') + TSTRING(SETTINGS_DOWNLOADS) + _T('\\') + TSTRING(SETTINGS_FAVORITE_DIRS_PAGE)).c_str());
		SetTitle(title);
		//m_psp.dwFlags |= PSP_HASHELP;
	};
	~FavoriteDirsPage() {
		ctrlDirectories.Detach();
		free(title);
	};

	BEGIN_MSG_MAP(FavoriteDirsPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		MESSAGE_HANDLER(WM_DROPFILES, onDropFiles)
		NOTIFY_HANDLER(IDC_FAVORITE_DIRECTORIES, LVN_ITEMCHANGED, onItemchangedDirectories)
		COMMAND_ID_HANDLER(IDC_ADD, onClickedAdd)
		COMMAND_ID_HANDLER(IDC_REMOVE, onClickedRemove)
		COMMAND_ID_HANDLER(IDC_RENAME, onClickedRename)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onDropFiles(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onItemchangedDirectories(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onClickedAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedRename(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	virtual void write();
	
protected:
	static TextItem texts[];
	ExListViewCtrl ctrlDirectories;
	TCHAR* title;

	void addDirectory(const tstring& aPath);
};

#endif //FAVORITEDIRSPAGE_H
