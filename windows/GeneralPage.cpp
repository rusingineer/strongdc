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
#include "../client/BufferedSocket.h"
#include <winsock2.h>
#include "WinUtil.h"

PropPage::TextItem GeneralPage::texts[] = {
	{ IDC_SETTINGS_PERSONAL_INFORMATION, ResourceManager::SETTINGS_PERSONAL_INFORMATION },
	{ IDC_SETTINGS_NICK, ResourceManager::NICK },
	{ IDC_SETTINGS_EMAIL, ResourceManager::EMAIL },
	{ IDC_SETTINGS_DESCRIPTION, ResourceManager::DESCRIPTION },
	{ IDC_SETTINGS_CONNECTION_TYPE, ResourceManager::SETTINGS_CONNECTION_TYPE },
	{ IDC_SETTINGS_CONNECTION_SETTINGS, ResourceManager::SETTINGS_CONNECTION_SETTINGS },
	{ IDC_ACTIVE, ResourceManager::ACTIVE },
	{ IDC_PASSIVE, ResourceManager::SETTINGS_PASSIVE },
	{ IDC_SOCKS5, ResourceManager::SETTINGS_SOCKS5 }, 
	{ IDC_SETTINGS_IP, ResourceManager::SETTINGS_IP },
	{ IDC_SETTINGS_PORT, ResourceManager::SETTINGS_PORT },
	{ IDC_SETTINGS_SOCKS5_IP, ResourceManager::SETTINGS_SOCKS5_IP },
	{ IDC_SETTINGS_SOCKS5_PORT, ResourceManager::SETTINGS_SOCKS5_PORT },
	{ IDC_SETTINGS_SOCKS5_USERNAME, ResourceManager::SETTINGS_SOCKS5_USERNAME },
	{ IDC_SETTINGS_SOCKS5_PASSWORD, ResourceManager::PASSWORD },
	{ IDC_SOCKS_RESOLVE, ResourceManager::SETTINGS_SOCKS5_RESOLVE },
	{ IDC_GETIP, ResourceManager::GET_IP },
	{ IDC_UPDATEIP, ResourceManager::UPDATE_IP },
	{ IDC_SHOW_SPEED_CHECK, ResourceManager::SHOW_SPEED },
	{ IDC_DU, ResourceManager::DU },	
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item GeneralPage::items[] = {
	{ IDC_NICK,			SettingsManager::NICK,			PropPage::T_STR }, 
	{ IDC_EMAIL,		SettingsManager::EMAIL,			PropPage::T_STR }, 
	{ IDC_DESCRIPTION,	SettingsManager::DESCRIPTION,	PropPage::T_STR }, 
//	{ IDC_CONNECTION,	SettingsManager::CONNECTION,	PropPage::T_STR }, 
	{ IDC_DOWN_COMBO,	SettingsManager::DOWN_SPEED,	PropPage::T_STR },  
	{ IDC_UP_COMBO,		SettingsManager::UP_SPEED,		PropPage::T_STR },  
	{ IDC_SHOW_SPEED_CHECK, SettingsManager::SHOW_DESCRIPTION_SPEED, PropPage::T_BOOL },
	{ IDC_SERVER,		SettingsManager::SERVER,		PropPage::T_STR }, 
	{ IDC_PORT,			SettingsManager::IN_PORT,		PropPage::T_INT }, 
	{ IDC_SOCKS_SERVER, SettingsManager::SOCKS_SERVER,	PropPage::T_STR },
	{ IDC_SOCKS_PORT,	SettingsManager::SOCKS_PORT,	PropPage::T_INT },
	{ IDC_SOCKS_USER,	SettingsManager::SOCKS_USER,	PropPage::T_STR },
	{ IDC_SOCKS_PASSWORD, SettingsManager::SOCKS_PASSWORD, PropPage::T_STR },
	{ IDC_SOCKS_RESOLVE, SettingsManager::SOCKS_RESOLVE, PropPage::T_BOOL },
	{ IDC_UPDATEIP, SettingsManager::IPUPDATE, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

void GeneralPage::write()
{
	TCHAR tmp[1024];
	GetDlgItemText(IDC_SOCKS_SERVER, tmp, 1024);
	tstring x = tmp;
	tstring::size_type i;

	while((i = x.find(' ')) != string::npos)
		x.erase(i, 1);
	SetDlgItemText(IDC_SOCKS_SERVER, x.c_str());

	GetDlgItemText(IDC_SERVER, tmp, 1024);
	x = tmp;

	while((i = x.find(' ')) != string::npos)
		x.erase(i, 1);

	SetDlgItemText(IDC_SERVER, x.c_str());
	
	PropPage::write((HWND)(*this), items);

	// Set connection active/passive
	int ct = -1;
	if(IsDlgButtonChecked(IDC_ACTIVE))
		ct = SettingsManager::CONNECTION_ACTIVE;
	else if(IsDlgButtonChecked(IDC_PASSIVE))
		ct = SettingsManager::CONNECTION_PASSIVE;
	else if(IsDlgButtonChecked(IDC_SOCKS5))
		ct = SettingsManager::CONNECTION_SOCKS5;

	if(SETTING(CONNECTION_TYPE) != ct) {
		settings->set(SettingsManager::CONNECTION_TYPE, ct);
		Socket::socksUpdated();
	}
		
	settings->set(SettingsManager::CONNECTION, SettingsManager::connectionSpeeds[ctrlConnection.GetCurSel()]);
}

LRESULT GeneralPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	ctrlConnection.Attach(GetDlgItem(IDC_CONNECTION));
	//CRect pos(0, 0, 0, 128);
	//ctrlConnection.Create(m_hWnd, pos, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
	//	WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST);
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

	int q = 1;
//	for(int i = 0; i < SettingsManager::SPEED_LAST; i++) {
		//COMBOBOXEXITEM cbitem = {CBEIF_TEXT|CBEIF_IMAGE|CBEIF_SELECTEDIMAGE};
		//cbitem.pszText = const_cast<TCHAR*>(Text::toT(SettingsManager::connectionSpeeds[i]).c_str());
		//cbitem.iItem = i;
	for(int i = 0; i < SettingsManager::SPEED_LAST; i++) {
		COMBOBOXEXITEM cbitem = {CBEIF_TEXT|CBEIF_IMAGE|CBEIF_SELECTEDIMAGE};
		tstring conn = Text::toT(SettingsManager::connectionSpeeds[i]); // oprava connections
		cbitem.pszText = const_cast<TCHAR*>(conn.c_str());
		cbitem.iItem = i; 


		switch(i) {
			case 0: q = 1; break;
			case 1: q = 2; break;
			case 2: q = 3; break;
			case 3: q = 4; break;
			case 4: q = 6; break;
			case 5: q = 5; break;
			case 6: q = 7; break;
			case 7: q = 7; break;
		}
		
		cbitem.iImage = q;

		cbitem.iSelectedImage = q;
		ctrlConnection.InsertItem(&cbitem);

	}

	int const connType = settings->get(SettingsManager::CONNECTION_TYPE);
	if(connType == SettingsManager::CONNECTION_ACTIVE)
		CheckRadioButton(IDC_ACTIVE, IDC_SOCKS5, IDC_ACTIVE);
	else if(connType == SettingsManager::CONNECTION_PASSIVE)
		CheckRadioButton(IDC_ACTIVE, IDC_SOCKS5, IDC_PASSIVE);
	else if(connType == SettingsManager::CONNECTION_SOCKS5)
		CheckRadioButton(IDC_ACTIVE, IDC_SOCKS5, IDC_SOCKS5);

	PropPage::read((HWND)(*this), items);

	fixControls();

	int m = 0;
	for (m = 0; m<8; m++) {
		if(SettingsManager::connectionSpeeds[m] == SETTING(CONNECTION)) break;
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
	desc.Attach(GetDlgItem(IDC_SOCKS_SERVER));
	desc.LimitText(250);
	desc.Detach();
	desc.Attach(GetDlgItem(IDC_SOCKS_PORT));
	desc.LimitText(5);
	desc.Detach();
	desc.Attach(GetDlgItem(IDC_SOCKS_USER));
	desc.LimitText(250);
	desc.Detach();
	desc.Attach(GetDlgItem(IDC_SOCKS_PASSWORD));
	desc.LimitText(250);
	desc.Detach();
	return TRUE;
}

void GeneralPage::fixControls() {
	BOOL checked = IsDlgButtonChecked(IDC_ACTIVE);
	::EnableWindow(GetDlgItem(IDC_SERVER), checked);
	::EnableWindow(GetDlgItem(IDC_PORT), checked);

	checked = IsDlgButtonChecked(IDC_SOCKS5);
	::EnableWindow(GetDlgItem(IDC_SOCKS_SERVER), checked);
	::EnableWindow(GetDlgItem(IDC_SOCKS_PORT), checked);
	::EnableWindow(GetDlgItem(IDC_SOCKS_USER), checked);
	::EnableWindow(GetDlgItem(IDC_SOCKS_PASSWORD), checked);
	::EnableWindow(GetDlgItem(IDC_SOCKS_RESOLVE), checked);

	checked = IsDlgButtonChecked(IDC_SHOW_SPEED_CHECK);
	::EnableWindow(GetDlgItem(IDC_DOWN_COMBO), checked);
	::EnableWindow(GetDlgItem(IDC_UP_COMBO), checked);
	::EnableWindow(GetDlgItem(IDC_DU), checked);
	::EnableWindow(GetDlgItem(IDC_SLASH), checked);
}

LRESULT GeneralPage::onClickedActive(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
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

string GeneralPage::getMyIP()
{
     WORD wVersionRequested;
     WSADATA wsaData;
     struct hostent * hent ;
     char * addr ;
     char * hostnm = new char[256];
     int err; 
     wVersionRequested = MAKEWORD( 2, 2 ); 
     err = WSAStartup( wVersionRequested, &wsaData );
     if ( err != 0 ) {return "";} 
     if ( LOBYTE( wsaData.wVersion ) != 2 ||
                     HIBYTE( wsaData.wVersion ) != 2 ) {
          WSACleanup();
          return "";
     }
     gethostname(hostnm,256);

     hent=gethostbyname (hostnm);
	 addr=inet_ntoa (*(struct in_addr *) *hent->h_addr_list); 
	 delete[] hostnm;
     WSACleanup();
     return addr;
}

LRESULT GeneralPage::onGetIP(WORD /* wNotifyCode */, WORD wID, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	string IPAdresa = /*getMyIP();*/ Util::getLocalIp();

   if((IPAdresa.compare(0,3,"10.")==0) ||
	  (IPAdresa.compare(0,8,"192.168.")==0) ||
	  (IPAdresa.compare(0,8,"169.254.")==0) ||
	  ((IPAdresa.compare(0,4,"172.")==0) && (Util::toInt(IPAdresa.substr(4,2))>=16 && Util::toInt(IPAdresa.substr(4,2))<=31))

	  )
	{ CheckRadioButton(IDC_ACTIVE, IDC_SOCKS5, IDC_PASSIVE); } else { CheckRadioButton(IDC_ACTIVE, IDC_SOCKS5, IDC_ACTIVE); }
	
	fixControls();
	SetDlgItemText(IDC_SERVER,Text::toT(IPAdresa).c_str());
	return 0;
}
/**
 * @file
 * $Id$
 */
