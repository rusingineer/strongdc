/* 
 * Copyright (C) 2002-2004 "Opera", <opera at home dot se>
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

#include "UpdateDlg.h"

#include "../client/Util.h"
#include "../client/simplexml.h"
#include "WinUtil.h"
#include "CZDCLib.h"


UpdateDlg::~UpdateDlg() {
	if (hc) {
		hc->removeListeners();
		delete hc;
	}
	if (m_hIcon)
		DeleteObject((HGDIOBJ)m_hIcon);
};


LRESULT UpdateDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ctrlCurrentVersion.Attach(GetDlgItem(IDC_UPDATE_VERSION_CURRENT));
	ctrlLatestVersion.Attach(GetDlgItem(IDC_UPDATE_VERSION_LATEST));
	ctrlChangeLog.Attach(GetDlgItem(IDC_UPDATE_HISTORY_TEXT));
	ctrlStatus.Attach(GetDlgItem(IDC_HISTORY_STATUS));
	ctrlDownload.Attach(GetDlgItem(IDC_UPDATE_DOWNLOAD));
	ctrlClose.Attach(GetDlgItem(IDCLOSE));

	::SetWindowText(GetDlgItem(IDC_UPDATE_VERSION_CURRENT_LBL), (STRING(CURRENT_VERSION) + ":").c_str());
	::SetWindowText(GetDlgItem(IDC_UPDATE_VERSION_LATEST_LBL), (STRING(LATEST_VERSION) + ":").c_str());
	PostMessage(WM_SPEAKER, UPDATE_CURRENT_VERSION, (LPARAM)new string(VERSIONSTRING CZDCVERSIONSTRING));
	PostMessage(WM_SPEAKER, UPDATE_LATEST_VERSION, (LPARAM)new string(""));
	PostMessage(WM_SPEAKER, UPDATE_CONTENT, (LPARAM)new string(""));
	ctrlDownload.SetWindowText(CSTRING(DOWNLOAD));
	ctrlDownload.EnableWindow(FALSE);
	ctrlClose.SetWindowText(CSTRING(CLOSE));
	PostMessage(WM_SPEAKER, UPDATE_STATUS, (LPARAM)new string(STRING(CONNECTING_TO_SERVER) + "..."));

	::SetWindowText(GetDlgItem(IDC_UPDATE_VERSION), CSTRING(VERSION));
	::SetWindowText(GetDlgItem(IDC_UPDATE_HISTORY), CSTRING(HISTORY));

	hc = new HttpConnection;
	hc->addListener(this);
	hc->downloadFile("http://snail.pc.cz/StrongDC/version.xml");

	SetWindowText(CSTRING(UPDATE_CHECK));

	m_hIcon = ::LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDR_UPDATE));
	SetIcon(m_hIcon, FALSE);
	SetIcon(m_hIcon, TRUE);
	
	CenterWindow(GetParent());
	return FALSE;
}

LRESULT UpdateDlg::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if (wParam == UPDATE_CURRENT_VERSION) {
		string* text = (string*)lParam;
		ctrlCurrentVersion.SetWindowText(text->c_str());
		delete text;
	} else if (wParam == UPDATE_LATEST_VERSION) {
		string* text = (string*)lParam;
		ctrlLatestVersion.SetWindowText(text->c_str());
		delete text;
	} else if (wParam == UPDATE_STATUS) {
		string* text = (string*)lParam;
		ctrlStatus.SetWindowText(text->c_str());
		delete text;
	} else if (wParam == UPDATE_CONTENT) {
		string* text = (string*)lParam;
		ctrlChangeLog.SetWindowText(text->c_str());
		delete text;
	}
	return S_OK;
}

LRESULT UpdateDlg::OnDownload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	WinUtil::openLink(downloadURL.c_str());
	return S_OK;
}


void UpdateDlg::on(HttpConnectionListener::Failed, HttpConnection* conn, const string& aLine) throw() {
	PostMessage(WM_SPEAKER, UPDATE_STATUS, (LPARAM)new string(STRING(CONNECTION_ERROR) + "!"));
}

void UpdateDlg::on(HttpConnectionListener::Complete, HttpConnection* conn, string const& /*aLine*/) throw() {
			PostMessage(WM_SPEAKER, UPDATE_STATUS, (LPARAM)new string(STRING(DATA_RETRIEVED) + "!"));
			string sText;
			try {
				string s_latestVersion;
				string latestVersion;
				double oldVersion;
				string versionURL;
				string updateURL;
				SimpleXML xml;
				xml.fromXML(xmldata);
				xml.stepIn();
				if (xml.findChild("Version")) {
					s_latestVersion = xml.getChildData();
					latestVersion = s_latestVersion;
					xml.resetCurrentChild();
				} else
					throw Exception();
				PostMessage(WM_SPEAKER, UPDATE_LATEST_VERSION, (LPARAM)new string(s_latestVersion));
				if (xml.findChild("VeryOldVersion")) {
					oldVersion = atof(xml.getChildData().c_str());					
				}
				xml.resetCurrentChild();
				/*else
					throw Exception();*/
		/*		if (xml.findChild("VersionURL")) {
					versionURL = xml.getChildData();
					xml.resetCurrentChild();
				} else
					throw Exception();
				if (versionURL != SETTING(VERSION_URL))
					SettingsManager::getInstance()->set(SettingsManager::VERSION_URL, versionURL);
				if (xml.findChild("UpdateURL")) {
					updateURL = xml.getChildData();
					xml.resetCurrentChild();
				} else
					throw Exception();
				if (updateURL != SETTING(UPDATE_URL))
					SettingsManager::getInstance()->set(SettingsManager::UPDATE_URL, updateURL);*/
				if (xml.findChild("URL")) {
					downloadURL = xml.getChildData();
					xml.resetCurrentChild();
//					if (latestVersion > oVERSIONFLOAT)
					if(latestVersion != (VERSIONSTRING CZDCVERSIONSTRING))
						ctrlDownload.EnableWindow(TRUE);
				} else
					throw Exception();

				while(xml.findChild("Message")) {
					const string& sData = xml.getChildData();
					sText += sData + "\r\n";
				}
				PostMessage(WM_SPEAKER, UPDATE_CONTENT, (LPARAM)new string(sText));
			} catch (const Exception&) {
				PostMessage(WM_SPEAKER, UPDATE_CONTENT, (LPARAM)new string("Couldn't parse xml-data"));
			}
}

void UpdateDlg::on(HttpConnectionListener::Data, HttpConnection* conn, const u_int8_t* buf, size_t len) throw() {
			if (xmldata.empty())
				PostMessage(WM_SPEAKER, UPDATE_STATUS, (LPARAM)new string(STRING(RETRIEVING_DATA) + "..."));
			xmldata += string((const char*)buf, len);
}
