#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "../client/SettingsManager.h"
#include "Resource.h"
#include "LineDlg.h"
#include "atlstr.h"


string KickDlg::m_sLastMsg = "";

LRESULT KickDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	Msgs[0] = SETTING( KICK_MSG_RECENT_01 );
	Msgs[1] = SETTING( KICK_MSG_RECENT_02 );
	Msgs[2] = SETTING( KICK_MSG_RECENT_03 );
	Msgs[3] = SETTING( KICK_MSG_RECENT_04 );
	Msgs[4] = SETTING( KICK_MSG_RECENT_05 );
	Msgs[5] = SETTING( KICK_MSG_RECENT_06 );
	Msgs[6] = SETTING( KICK_MSG_RECENT_07 );
	Msgs[7] = SETTING( KICK_MSG_RECENT_08 );
	Msgs[8] = SETTING( KICK_MSG_RECENT_09 );
	Msgs[9] = SETTING( KICK_MSG_RECENT_10 );
	Msgs[10] = SETTING( KICK_MSG_RECENT_11 );
	Msgs[11] = SETTING( KICK_MSG_RECENT_12 );
	Msgs[12] = SETTING( KICK_MSG_RECENT_13 );
	Msgs[13] = SETTING( KICK_MSG_RECENT_14 );
	Msgs[14] = SETTING( KICK_MSG_RECENT_15 );
	Msgs[15] = SETTING( KICK_MSG_RECENT_16 );
	Msgs[16] = SETTING( KICK_MSG_RECENT_17 );
	Msgs[17] = SETTING( KICK_MSG_RECENT_18 );
	Msgs[18] = SETTING( KICK_MSG_RECENT_19 );
	Msgs[19] = SETTING( KICK_MSG_RECENT_20 );

	ctrlLine.Attach(GetDlgItem(IDC_LINE));
	ctrlLine.SetFocus();

	line = "";
	for ( int i = 0; i < 20; i++ ) {
		if ( Msgs[i] != "" )
			ctrlLine.AddString( Msgs[i].c_str() );
	}
	ctrlLine.SetWindowText( m_sLastMsg.c_str() );

	ctrlDescription.Attach(GetDlgItem(IDC_DESCRIPTION));
	ctrlDescription.SetWindowText(description.c_str());
	
	SetWindowText(title.c_str());
	
	CenterWindow(GetParent());
	return FALSE;
}
	
LRESULT KickDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(wID == IDOK) {
		int iLen = ctrlLine.GetWindowTextLength();
		CAtlString sText( ' ', iLen+1 );
		GetDlgItemText(IDC_LINE, sText.GetBuffer(), iLen+1);
		sText.ReleaseBuffer();
		m_sLastMsg = sText;
		line = sText;
		int i, j;

		for ( i = 0; i < 20; i++ ) {
			if ( line == Msgs[i] ) {
				for ( j = i; j > 0; j-- ) {
					Msgs[j] = Msgs[j-1];
				}
				Msgs[0] = line;
				break;
			}
		}

		if ( i >= 20 ) {
			for ( j = 19; j > 0; j-- ) {
				Msgs[j] = Msgs[j-1];
			}
			Msgs[0] = line;
		}

		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_01, Msgs[0] );
		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_02, Msgs[1] );
		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_03, Msgs[2] );
		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_04, Msgs[3] );
		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_05, Msgs[4] );
		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_06, Msgs[5] );
		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_07, Msgs[6] );
		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_08, Msgs[7] );
		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_09, Msgs[8] );
		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_10, Msgs[9] );
		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_11, Msgs[10] );
		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_12, Msgs[11] );
		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_13, Msgs[12] );
		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_14, Msgs[13] );
		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_15, Msgs[14] );
		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_16, Msgs[15] );
		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_17, Msgs[16] );
		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_18, Msgs[17] );
		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_19, Msgs[18] );
		SettingsManager::getInstance()->set( SettingsManager::KICK_MSG_RECENT_20, Msgs[19] );
	}
  
	EndDialog(wID);
	return 0;
}
