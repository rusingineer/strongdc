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

#include "OperaColorsPage.h"
#include "../client/SettingsManager.h"
#include "WinUtil.h"
#include "PropertiesDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PropPage::TextItem OperaColorsPage::texts[] = {
	{ IDC_PROGRESS_OVERRIDE, ResourceManager::SETTINGS_ZDC_PROGRESS_OVERRIDE },
	{ IDC_PROGRESS_OVERRIDE2, ResourceManager::SETTINGS_ZDC_PROGRESS_OVERRIDE2 },
	{ IDC_SETTINGS_DOWNLOAD_BAR_COLOR, ResourceManager::DOWNLOAD },
	{ IDC_PROGRESS_TEXT_COLOR_DOWN, ResourceManager::DOWNLOAD },
	{ IDC_SETTINGS_UPLOAD_BAR_COLOR, ResourceManager::SETCZDC_UPLOAD },
	{ IDC_PROGRESS_TEXT_COLOR_UP, ResourceManager::SETCZDC_UPLOAD },
	{ IDC_PROGRESS_BUMPED, ResourceManager::BUMPED },
	{ IDC_SETTINGS_ODC_MENUBAR, ResourceManager::MENUBAR },
	{ IDC_SETTINGS_ODC_MENUBAR_LEFT, ResourceManager::LEFT_COLOR },
	{ IDC_SETTINGS_ODC_MENUBAR_RIGHT, ResourceManager::RIGHT_COLOR },
	{ IDC_SETTINGS_ODC_MENUBAR_USETWO, ResourceManager::TWO_COLORS },
	{ IDC_SETTINGS_ODC_MENUBAR_BUMPED, ResourceManager::BUMPED },
	{ IDC_CZDC_PROGRESS_COLOR, ResourceManager::SETCZDC_PROGRESSBAR_COLORS },
	{ IDC_CZDC_PROGRESS_TEXT, ResourceManager::SETCZDC_PROGRESSBAR_TEXT },
	{ IDC_IMAGEBROWSE, ResourceManager::BROWSE },
	{ IDC_USERLIST, ResourceManager::USERLIST_ICONS },
	{ IDC_SETTINGS_SEGMENTBAR, ResourceManager::SEGMENTBAR },
	{ IDC_PROGRESS_SEGMENT_SHOW, ResourceManager::SHOW_SEGMENT_PART },
	{ IDC_SETTINGS_SEGMENT_BAR_COLOR, ResourceManager::SEGMENT_PART_COLOR },
	{ IDC_BACK_IMAGE, ResourceManager::BACKGROUND_IMAGE },
	{ IDC_IMAGEBROWSE2, ResourceManager::BROWSE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

#define CTLID_VALUE_RED 0x2C2
#define CTLID_VALUE_BLUE 0x2C3
#define CTLID_VALUE_GREEN 0x2C4

OperaColorsPage* current_page;
LPCCHOOKPROC color_proc;

PropPage::Item OperaColorsPage::items[] = {
	{ IDC_PROGRESS_OVERRIDE, SettingsManager::PROGRESS_OVERRIDE_COLORS, PropPage::T_BOOL },
	{ IDC_PROGRESS_OVERRIDE2, SettingsManager::PROGRESS_OVERRIDE_COLORS2, PropPage::T_BOOL },
	{ IDC_PROGRESS_SEGMENT_SHOW, SettingsManager::SHOW_SEGMENT_COLOR, PropPage::T_BOOL },
	{ IDC_PROGRESS_BUMPED, SettingsManager::PROGRESS_BUMPED, PropPage::T_BOOL },
	{ IDC_USERLIST_IMAGE, SettingsManager::USERLIST_IMAGE, PropPage::T_STR },
	{ IDC_BACKGROUND_IMAGE, SettingsManager::BACKGROUND_IMAGE, PropPage::T_STR },
	{ 0, 0, PropPage::T_END }
};

UINT_PTR CALLBACK MenuBarCommDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (uMsg == WM_COMMAND) {
		if (HIWORD(wParam) == EN_CHANGE && 
			(LOWORD(wParam) == CTLID_VALUE_RED || LOWORD(wParam) == CTLID_VALUE_GREEN || LOWORD(wParam) == CTLID_VALUE_BLUE)) {
			char buf[16];
			ZeroMemory(buf, 16);
			::GetDlgItemText(hWnd, CTLID_VALUE_RED, buf, 15);
			int color_r = Util::toInt(buf);
			ZeroMemory(buf, 16);
			::GetDlgItemText(hWnd, CTLID_VALUE_BLUE, buf, 15);
			int color_g = Util::toInt(buf);
			ZeroMemory(buf, 16);
			::GetDlgItemText(hWnd, CTLID_VALUE_GREEN, buf, 15);
			int color_b = Util::toInt(buf);

			color_r = (color_r < 0) ? 0 : ((color_r > 255) ? 255 : color_r);
			color_g = (color_g < 0) ? 0 : ((color_g > 255) ? 255 : color_g);
			color_b = (color_b < 0) ? 0 : ((color_b > 255) ? 255 : color_b);

			if (current_page->bDoProgress) {
				if (current_page->bDoLeft) {
					current_page->crProgressDown = RGB(color_r, color_g, color_b);
					current_page->ctrlProgressDownDrawer.Invalidate();
					current_page->ctrlProgressSegmentDrawer.Invalidate();
				} else if (current_page->bDoSegment) {
					current_page->crProgressSegment = RGB(color_r, color_g, color_b);
					current_page->ctrlProgressSegmentDrawer.Invalidate();
				} else {
					current_page->crProgressUp = RGB(color_r, color_g, color_b);
					current_page->ctrlProgressUpDrawer.Invalidate();
				}
			} else {
				if (current_page->bDoLeft)
					current_page->crMenubarLeft = RGB(color_r, color_g, color_b);
				else
					current_page->crMenubarRight = RGB(color_r, color_g, color_b);
				current_page->ctrlMenubarDrawer.Invalidate();
			}
		}
	}
	
	return (*color_proc)(hWnd, uMsg, wParam, lParam);

}
LRESULT OperaColorsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	crProgressDown = SETTING(DOWNLOAD_BAR_COLOR);
	crProgressUp = SETTING(UPLOAD_BAR_COLOR);
	crProgressTextDown = SETTING(PROGRESS_TEXT_COLOR_DOWN);
	crProgressTextUp = SETTING(PROGRESS_TEXT_COLOR_UP);
	crProgressSegment = SETTING(SEGMENT_BAR_COLOR);

/*	ctrlProgressOverride1.Attach(GetDlgItem(IDC_PROGRESS_OVERRIDE));
	ctrlProgressOverride2.Attach(GetDlgItem(IDC_PROGRESS_OVERRIDE2));*/
	ctrlProgressDownDrawer.Attach(GetDlgItem(IDC_PROGRESS_COLOR_DOWN_SHOW));
	ctrlProgressUpDrawer.Attach(GetDlgItem(IDC_PROGRESS_COLOR_UP_SHOW));
	ctrlProgressSegmentDrawer.Attach(GetDlgItem(IDC_PROGRESS_COLOR_SEGMENT_SHOW));

	updateProgress();
	checkBox(IDC_PROGRESS_BUMPED, BOOLSETTING(PROGRESS_BUMPED));

	crMenubarLeft = SETTING(MENUBAR_LEFT_COLOR);
	crMenubarRight = SETTING(MENUBAR_RIGHT_COLOR);
	ctrlLeftColor.Attach(GetDlgItem(IDC_SETTINGS_ODC_MENUBAR_LEFT));
	ctrlRightColor.Attach(GetDlgItem(IDC_SETTINGS_ODC_MENUBAR_RIGHT));
	ctrlTwoColors.Attach(GetDlgItem(IDC_SETTINGS_ODC_MENUBAR_USETWO));
	ctrlBumped.Attach(GetDlgItem(IDC_SETTINGS_ODC_MENUBAR_BUMPED));
	ctrlMenubarDrawer.Attach(GetDlgItem(IDC_SETTINGS_ODC_MENUBAR_COLOR));

	checkBox(IDC_SETTINGS_ODC_MENUBAR_BUMPED, BOOLSETTING(MENUBAR_BUMPED));
	checkBox(IDC_SETTINGS_ODC_MENUBAR_USETWO, BOOLSETTING(MENUBAR_TWO_COLORS));
	BOOL b;
	onMenubarClicked(0, IDC_SETTINGS_ODC_MENUBAR_USETWO, 0, b);
	// Do specialized reading here
	
	return TRUE;
}

void OperaColorsPage::write()
{
	PropPage::write((HWND)*this, items);
	
	// Do specialized writing here
	// settings->set(XX, YY);
//	if(!PropertiesDlg::needUpdate) {
	SettingsManager::getInstance()->set(SettingsManager::MENUBAR_LEFT_COLOR, (int)crMenubarLeft);
	SettingsManager::getInstance()->set(SettingsManager::MENUBAR_RIGHT_COLOR, (int)crMenubarRight);
	SettingsManager::getInstance()->set(SettingsManager::MENUBAR_BUMPED, getCheckbox(IDC_SETTINGS_ODC_MENUBAR_BUMPED));
	SettingsManager::getInstance()->set(SettingsManager::MENUBAR_TWO_COLORS, getCheckbox(IDC_SETTINGS_ODC_MENUBAR_USETWO));

	SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_BAR_COLOR, (int)crProgressDown);
	SettingsManager::getInstance()->set(SettingsManager::UPLOAD_BAR_COLOR, (int)crProgressUp);
	SettingsManager::getInstance()->set(SettingsManager::SEGMENT_BAR_COLOR, (int)crProgressSegment);
	SettingsManager::getInstance()->set(SettingsManager::PROGRESS_TEXT_COLOR_DOWN, (int)crProgressTextDown);
	SettingsManager::getInstance()->set(SettingsManager::PROGRESS_TEXT_COLOR_UP, (int)crProgressTextUp);
//	}
//	PropertiesDlg::needUpdate = false;
}

LRESULT OperaColorsPage::onDrawItem(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if(PropertiesDlg::needUpdate)
	{
		SendMessage(WM_DESTROY,0,0);
		SendMessage(WM_INITDIALOG,0,0);
		PropertiesDlg::needUpdate = false;
	}
	bHandled = FALSE;
//	if (wParam == IDC_SETTINGS_ODC_MENUBAR_COLOR) {
		DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
		if (dis->CtlType == ODT_STATIC) {
			bHandled = TRUE;
			CDC dc;
			dc.Attach(dis->hDC);
			CRect rc(dis->rcItem);
			if (dis->CtlID == IDC_SETTINGS_ODC_MENUBAR_COLOR) {
				if (getCheckbox(IDC_SETTINGS_ODC_MENUBAR_USETWO))
					OperaColors::FloodFill(dc, rc.left, rc.top, rc.right, rc.bottom, crMenubarLeft, crMenubarRight, getCheckbox(IDC_SETTINGS_ODC_MENUBAR_BUMPED));
				else
					dc.FillSolidRect(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, crMenubarLeft);
				dc.SetTextColor(OperaColors::TextFromBackground(crMenubarLeft));
			} else if (dis->CtlID == IDC_PROGRESS_COLOR_DOWN_SHOW || dis->CtlID == IDC_PROGRESS_COLOR_UP_SHOW) {
				COLORREF clr = getCheckbox(IDC_PROGRESS_OVERRIDE) ? ((dis->CtlID == IDC_PROGRESS_COLOR_DOWN_SHOW) ? crProgressDown : crProgressUp) : GetSysColor(COLOR_HIGHLIGHT);
				COLORREF a, b;
				OperaColors::EnlightenFlood(clr, a, b);
				OperaColors::FloodFill(dc, rc.left, rc.top, rc.right, rc.bottom, a, b, getCheckbox(IDC_PROGRESS_BUMPED));
				int textcolor = getCheckbox(IDC_PROGRESS_OVERRIDE2) ? ((dis->CtlID == IDC_PROGRESS_COLOR_DOWN_SHOW) ? crProgressTextDown : crProgressTextUp) : OperaColors::TextFromBackground(clr);
				dc.SetTextColor(textcolor);
			} else if (dis->CtlID == IDC_PROGRESS_COLOR_SEGMENT_SHOW) {
				COLORREF clr = getCheckbox(IDC_PROGRESS_OVERRIDE) ? crProgressDown : GetSysColor(COLOR_HIGHLIGHT);
				COLORREF a, b;
				OperaColors::EnlightenFlood(clr, a, b);

				HBRUSH hBrDefBg = CreateSolidBrush(OperaColors::blendColors(WinUtil::bgColor, clr, 0.85));
				HGDIOBJ oldBg = ::SelectObject(dc, hBrDefBg);
				::Rectangle(dc, rc.left, rc.top, rc.right, rc.bottom);

				DeleteObject(::SelectObject(dc, oldBg));

				clr = getCheckbox(IDC_PROGRESS_OVERRIDE) ? crProgressSegment : GetSysColor(COLOR_HIGHLIGHT);				
				OperaColors::EnlightenFlood(clr, a, b);
				if(getCheckbox(IDC_PROGRESS_SEGMENT_SHOW)) OperaColors::FloodFill(dc, rc.left, rc.top, rc.right / 2, rc.bottom, a, b, getCheckbox(IDC_PROGRESS_BUMPED));

				clr = getCheckbox(IDC_PROGRESS_OVERRIDE) ? crProgressDown : GetSysColor(COLOR_HIGHLIGHT);
				OperaColors::EnlightenFlood(clr, a, b);
				OperaColors::FloodFill(dc, rc.right / 2 + 1, rc.top, rc.right, rc.bottom, a, b, getCheckbox(IDC_PROGRESS_BUMPED));				
				int textcolor = getCheckbox(IDC_PROGRESS_OVERRIDE2) ? ((dis->CtlID == IDC_PROGRESS_COLOR_DOWN_SHOW) ? crProgressTextDown : crProgressTextUp) : OperaColors::TextFromBackground(clr);
				dc.SetTextColor(textcolor);
			}

			dc.DrawText("Sample text", sizeof("Sample text")-1, rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

			dc.Detach();
		}
//	}
	return S_OK;
}

LRESULT OperaColorsPage::onMenubarClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
	if (wID == IDC_SETTINGS_ODC_MENUBAR_LEFT) {
		CColorDialog d(crMenubarLeft, CC_FULLOPEN, *this);
		color_proc = d.m_cc.lpfnHook;
		d.m_cc.lpfnHook = MenuBarCommDlgProc;
		current_page = this;//d.m_cc.lCustData = (LPARAM)this;
		bDoProgress = false;
		bDoLeft = true;
		COLORREF backup = crMenubarLeft;
		if (d.DoModal() == IDOK)
			crMenubarLeft = d.GetColor();
		else
			crMenubarLeft = backup;
	} else if (wID == IDC_SETTINGS_ODC_MENUBAR_RIGHT) {
		CColorDialog d(crMenubarRight, CC_FULLOPEN, *this);
		color_proc = d.m_cc.lpfnHook;
		d.m_cc.lpfnHook = MenuBarCommDlgProc;
		current_page = this;//d.m_cc.lCustData = (LPARAM)this;
		bDoProgress = false;
		bDoLeft = false;
		COLORREF backup = crMenubarRight;
		if (d.DoModal() == IDOK)
			crMenubarRight = d.GetColor();
		else
			crMenubarRight = backup;
	} else if (wID == IDC_SETTINGS_ODC_MENUBAR_USETWO) {
		bool b = getCheckbox(IDC_SETTINGS_ODC_MENUBAR_USETWO);
		ctrlRightColor.EnableWindow(b);
		ctrlBumped.EnableWindow(b);
	} else if (wID == IDC_SETTINGS_ODC_MENUBAR_BUMPED) {
	}
	ctrlMenubarDrawer.Invalidate();
	return S_OK;
}
LRESULT OperaColorsPage::onClickedProgress(WORD /* wNotifyCode */, WORD wID, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	if (wID == IDC_SETTINGS_DOWNLOAD_BAR_COLOR) {
		CColorDialog d(crProgressDown, CC_FULLOPEN, *this);
		color_proc = d.m_cc.lpfnHook;
		d.m_cc.lpfnHook = MenuBarCommDlgProc;
		current_page = this;//d.m_cc.lCustData = (LPARAM)this;
		bDoProgress = true;
		bDoLeft = true;
		bDoSegment = false;
		COLORREF backup = crProgressDown;
		if (d.DoModal() == IDOK)
			crProgressDown = d.GetColor();
		else
			crProgressDown = backup;
		ctrlProgressDownDrawer.Invalidate();
		ctrlProgressSegmentDrawer.Invalidate();
	} else if (wID == IDC_SETTINGS_UPLOAD_BAR_COLOR) {
		CColorDialog d(crProgressUp, CC_FULLOPEN, *this);
		color_proc = d.m_cc.lpfnHook;
		d.m_cc.lpfnHook = MenuBarCommDlgProc;
		current_page = this;//d.m_cc.lCustData = (LPARAM)this;
		bDoProgress = true;
		bDoLeft = false;
		bDoSegment = false;
		COLORREF backup = crProgressUp;
		if (d.DoModal() == IDOK)
			crProgressUp = d.GetColor();
		else
			crProgressUp = backup;
		ctrlProgressUpDrawer.Invalidate();
	} else if (wID == IDC_SETTINGS_SEGMENT_BAR_COLOR) {
		CColorDialog d(crProgressSegment, CC_FULLOPEN, *this);
		color_proc = d.m_cc.lpfnHook;
		d.m_cc.lpfnHook = MenuBarCommDlgProc;
		current_page = this;//d.m_cc.lCustData = (LPARAM)this;
		bDoProgress = true;
		bDoLeft = false;
		bDoSegment = true;
		COLORREF backup = crProgressSegment;
		if (d.DoModal() == IDOK)
			crProgressSegment = d.GetColor();
		else
			crProgressSegment = backup;
		ctrlProgressSegmentDrawer.Invalidate();
	}
	return TRUE;
}

LRESULT OperaColorsPage::onClickedProgressTextDown(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	CColorDialog d(crProgressTextDown, 0, *this);
	if (d.DoModal() == IDOK) {
		crProgressTextDown = d.GetColor();
	}
		ctrlProgressDownDrawer.Invalidate();
	return TRUE;
}

LRESULT OperaColorsPage::onClickedProgressTextUp(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	CColorDialog d(crProgressTextUp, 0, *this);
	if (d.DoModal() == IDOK) {
		crProgressTextUp = d.GetColor();
	}
		ctrlProgressUpDrawer.Invalidate();
	return TRUE;
}

void OperaColorsPage::BrowseForPic(int DLGITEM) {
	char buf[MAX_PATH];

	GetDlgItemText(DLGITEM, buf, MAX_PATH);
	string x = buf;

	if(WinUtil::browseFile(x, m_hWnd, false) == IDOK) {
		SetDlgItemText(DLGITEM, x.c_str());
	}
}

LRESULT OperaColorsPage::onImageBrowse(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(wID == IDC_IMAGEBROWSE)
		BrowseForPic(IDC_USERLIST_IMAGE);
	else
		BrowseForPic(IDC_BACKGROUND_IMAGE);
	return 0;
}

LRESULT OperaColorsPage::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	if (ctrlProgressDownDrawer.m_hWnd != NULL)	
		ctrlProgressDownDrawer.Detach();
	if (ctrlProgressUpDrawer.m_hWnd != NULL)
		ctrlProgressUpDrawer.Detach();
	if (ctrlProgressSegmentDrawer.m_hWnd != NULL)
		ctrlProgressSegmentDrawer.Detach();
	if (ctrlLeftColor.m_hWnd != NULL)
		ctrlLeftColor.Detach();
	if (ctrlRightColor.m_hWnd != NULL)
		ctrlRightColor.Detach();
	if (ctrlTwoColors.m_hWnd != NULL)
		ctrlTwoColors.Detach();
	if (ctrlBumped.m_hWnd != NULL)
		ctrlBumped.Detach();
	if (ctrlMenubarDrawer.m_hWnd != NULL)
		ctrlMenubarDrawer.Detach();
	return 1;
}
/**
 * @file
 * $Id$
 */

