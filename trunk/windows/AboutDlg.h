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

#if !defined(AFX_ABOUTDLG_H__D12815FA_21C0_4C20_9718_892C9F8CD196__INCLUDED_)
#define AFX_ABOUTDLG_H__D12815FA_21C0_4C20_9718_892C9F8CD196__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/HttpConnection.h"
#include "../client/SimpleXML.h"

static const TCHAR thanks[] = 
_T("D�kuji v�em, kte�� m� ve v�voji podporovali. THX: Andyman, Blackrabbit, Chmelic, Cinique, Corvik, FarCry, GargoyleMT, King Wenceslas, Liny, ProLogic, Testament a samoz�ejm� PPK, �e m� nechal opsat v�t�inu v�c� z jeho klienta :-)");

class AboutDlg : public CDialogImpl<AboutDlg>, private HttpConnectionListener
{
public:
	enum { IDD = IDD_ABOUTBOX };
	enum { WM_VERSIONDATA = WM_APP + 53 };

	AboutDlg() { };
	virtual ~AboutDlg() { };

	BEGIN_MSG_MAP(AboutDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_VERSIONDATA, onVersionData)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		SetDlgItemText(IDC_VERSION, _T("StrongDC++ v") _T(VERSIONSTRING) _T(CZDCVERSIONSTRING) _T(" (c) Copyright 2004 Big Muscle\nBased on: DC++ ") _T(DCVERSIONSTRING) _T(" (c) Copyright 2001-2004 Jacek Sieka\n\nhttp://strongdc.berlios.de"));
		CEdit ctrlThanks(GetDlgItem(IDC_THANKS));
		ctrlThanks.FmtLines(TRUE);
		ctrlThanks.AppendText(thanks, TRUE);
		ctrlThanks.Detach();
		SetDlgItemText(IDC_TTH, WinUtil::tth.c_str());
		SetDlgItemText(IDC_LATEST, CTSTRING(DOWNLOADING));
		SetDlgItemText(IDC_TOTALS, Text::toT("Upload: " + Util::formatBytes(SETTING(TOTAL_UPLOAD)) + ", Download: " + 
			Util::formatBytes(SETTING(TOTAL_DOWNLOAD))).c_str());

		if(SETTING(TOTAL_DOWNLOAD) > 0) {
			char buf[64];
			_snprintf(buf, 63, "Ratio (up/down): %.2f", ((double)SETTING(TOTAL_UPLOAD)) / ((double)SETTING(TOTAL_DOWNLOAD)));
			buf[63] = 0;
			SetDlgItemText(IDC_RATIO, Text::toT(buf).c_str());
		/*	sprintf(buf, "Uptime: %s", Util::formatTime(Util::getUptime()));
			SetDlgItemText(IDC_UPTIME, Text::toT(buf).c_str());*/
		}
		CenterWindow(GetParent());
		c.addListener(this);
		c.downloadFile("http://snail.pc.cz/StrongDC/version.xml");
		return TRUE;
	}

	LRESULT onVersionData(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		tstring* x = (tstring*) wParam;
		SetDlgItemText(IDC_LATEST, x->c_str());
		delete x;
		return 0;
	}
		
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		EndDialog(wID);
		return 0;
	}

private:
	HttpConnection c;

	AboutDlg(const AboutDlg&) { dcassert(0); };
	
	virtual void on(HttpConnectionListener::Data, HttpConnection* /*conn*/, const u_int8_t* buf, size_t len) throw() {
		downBuf.append((char*)buf, len);
	}

	virtual void on(HttpConnectionListener::Complete, HttpConnection* conn, const string&) throw() {
			if(!downBuf.empty()) {
				SimpleXML xml;
				xml.fromXML(downBuf);
				if(xml.findChild("DCUpdate")) {
					xml.stepIn();
					if(xml.findChild("Version")) {
					tstring* x = new tstring(Text::toT(xml.getChildData()));
						PostMessage(WM_VERSIONDATA, (WPARAM) x);
					}
				}
			}
		conn->removeListener(this);
	}

	virtual void on(HttpConnectionListener::Failed, HttpConnection* conn, const string& aLine) throw() {
				tstring* x = new tstring(Text::toT(aLine));
				PostMessage(WM_VERSIONDATA, (WPARAM) x);
		conn->removeListener(this);
	}

	string downBuf;
};

#endif // !defined(AFX_ABOUTDLG_H__D12815FA_21C0_4C20_9718_892C9F8CD196__INCLUDED_)

/**
 * @file
 * $Id$
 */