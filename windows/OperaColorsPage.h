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

#ifndef OperaColorsPage_H
#define OperaColorsPage_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropPage.h"
#include "PropPageTextStyles.h"

class OperaColorsPage : public CPropertyPage<IDD_OPERACOLORS>, public PropPage
{
public:
	OperaColorsPage(SettingsManager *s) : PropPage(s) {
		title = strdup((STRING(SETTINGS_CZDC) + '\\' + STRING(SETTINGS_TEXT_STYLES) + '\\' + STRING(SETTINGS_OPERACOLORS)).c_str());
		SetTitle(title);
	};

	virtual ~OperaColorsPage() { delete[] title;};

	BEGIN_MSG_MAP(OperaColorsPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)

		COMMAND_HANDLER(IDC_PROGRESS_OVERRIDE, BN_CLICKED, onClickedProgressOverride)
		COMMAND_HANDLER(IDC_PROGRESS_OVERRIDE2, BN_CLICKED, onClickedProgressOverride)
		COMMAND_HANDLER(IDC_PROGRESS_BUMPED, BN_CLICKED, onClickedProgressOverride)
		COMMAND_HANDLER(IDC_SETTINGS_DOWNLOAD_BAR_COLOR, BN_CLICKED, onClickedProgress)
		COMMAND_HANDLER(IDC_SETTINGS_UPLOAD_BAR_COLOR, BN_CLICKED, onClickedProgress)
		COMMAND_HANDLER(IDC_PROGRESS_TEXT_COLOR_DOWN, BN_CLICKED, onClickedProgressTextDown)
		COMMAND_HANDLER(IDC_PROGRESS_TEXT_COLOR_UP, BN_CLICKED, onClickedProgressTextUp)
		MESSAGE_HANDLER(WM_DRAWITEM, onDrawItem)

		COMMAND_HANDLER(IDC_SETTINGS_ODC_MENUBAR_LEFT, BN_CLICKED, onMenubarClicked)
		COMMAND_HANDLER(IDC_SETTINGS_ODC_MENUBAR_RIGHT, BN_CLICKED, onMenubarClicked)
		COMMAND_HANDLER(IDC_SETTINGS_ODC_MENUBAR_USETWO, BN_CLICKED, onMenubarClicked)
		COMMAND_HANDLER(IDC_SETTINGS_ODC_MENUBAR_BUMPED, BN_CLICKED, onMenubarClicked)
		COMMAND_HANDLER(IDC_IMAGEBROWSE, BN_CLICKED, onImageBrowse)

		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onDrawItem(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onMenubarClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
		LRESULT onClickedProgressOverride(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */) {
		updateProgress();
		return S_OK;
	}
	LRESULT onClickedProgress(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onClickedProgressTextDown(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onClickedProgressTextUp(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onImageBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);

	void updateProgress() {
		bool state = (IsDlgButtonChecked(IDC_PROGRESS_OVERRIDE) != 0);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROGRESS_OVERRIDE2), state);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_SETTINGS_DOWNLOAD_BAR_COLOR), state);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_SETTINGS_UPLOAD_BAR_COLOR), state);

		state = ((IsDlgButtonChecked(IDC_PROGRESS_OVERRIDE) != 0) && (IsDlgButtonChecked(IDC_PROGRESS_OVERRIDE2) != 0));
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROGRESS_TEXT_COLOR_DOWN), state);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROGRESS_TEXT_COLOR_UP), state);
		ctrlProgressDownDrawer.Invalidate();
		ctrlProgressUpDrawer.Invalidate();
	}

	void updateScreen() {
		PostMessage(WM_INITDIALOG,0,0);
	}

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	virtual void write();

private:
	friend UINT_PTR CALLBACK MenuBarCommDlgProc(HWND, UINT, WPARAM, LPARAM);
	friend LRESULT PropPageTextStyles::onImport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	bool bDoProgress;
	bool bDoLeft;
	
	static Item items[];
	static TextItem texts[];

	typedef CButton CCheckBox;

	COLORREF crProgressDown;
	COLORREF crProgressUp;
	COLORREF crProgressTextDown;
	COLORREF crProgressTextUp;
	CCheckBox ctrlProgressOverride1;
	CCheckBox ctrlProgressOverride2;
	CButton ctrlProgressDownDrawer;
	CButton ctrlProgressUpDrawer;

		void checkBox(int id, bool b) {
		CheckDlgButton(id, b ? BST_CHECKED : BST_UNCHECKED);
	}
	bool getCheckbox(int id) {
		return (BST_CHECKED == IsDlgButtonChecked(id));
	}

	void BrowseForPic(int DLGITEM);

	COLORREF crMenubarLeft;
	COLORREF crMenubarRight;
	CButton ctrlLeftColor;
	CButton ctrlRightColor;
	CCheckBox ctrlTwoColors;
	CCheckBox ctrlBumped;
	CStatic ctrlMenubarDrawer;
	char* title;
};

#endif //OperaColorsPage_H

/**
 * @file
 * $Id$
 */

