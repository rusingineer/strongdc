#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"
#include "../client/SimpleXML.h"

#include "PropPageTextStyles.h"
#include "../client/SettingsManager.h"
#include "WinUtil.h"
#include "OperaColorsPage.h"
#include "PropertiesDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PropPage::TextItem PropPageTextStyles::texts[] = {
	{ IDC_AVAILABLE_STYLES, ResourceManager::SETCZDC_STYLES },
	{ IDC_BACK_COLOR, ResourceManager::SETCZDC_BACK_COLOR },
	{ IDC_TEXT_COLOR, ResourceManager::SETCZDC_TEXT_COLOR },
	{ IDC_TEXT_STYLE, ResourceManager::SETCZDC_TEXT_STYLE },
	{ IDC_DEFAULT_STYLES, ResourceManager::SETCZDC_DEFAULT_STYLE },
	{ IDC_BLACK_AND_WHITE, ResourceManager::SETCZDC_BLACK_WHITE },
	{ IDC_BOLD_AUTHOR_MESS, ResourceManager::SETCZDC_BOLD },
	{ IDC_CZDC_PREVIEW, ResourceManager::SETCZDC_PREVIEW },
	{ IDC_SELTEXT, ResourceManager::SETTINGS_SELECT_TEXT_FACE },
	{ IDC_RESET_TAB_COLOR, ResourceManager::SETTINGS_RESET },
	{ IDC_SELECT_TAB_COLOR, ResourceManager::SETTINGS_SELECT_COLOR },
	{ IDC_STYLES, ResourceManager::SETTINGS_TEXT_STYLES },
	{ IDC_CHATCOLORS, ResourceManager::SETTINGS_COLORS },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
}; 

PropPage::Item PropPageTextStyles::items[] = {
	{ IDC_BOLD_AUTHOR_MESS, SettingsManager::BOLD_AUTHOR_MESS, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

PropPageTextStyles::clrs PropPageTextStyles::colours[] = {
	{ResourceManager::SETTINGS_SELECT_WINDOW_COLOR,	SettingsManager::BACKGROUND_COLOR, 0},
	{ResourceManager::SETTINGS_COLOR_ALTERNATE,	SettingsManager::SEARCH_ALTERNATE_COLOUR, 0},
	{ResourceManager::SETCZDC_ERROR_COLOR,	SettingsManager::ERROR_COLOR, 0},
};


LRESULT PropPageTextStyles::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	m_lsbList.Attach( GetDlgItem(IDC_TEXT_STYLES) );
	m_lsbList.ResetContent();
	m_Preview.Attach( GetDlgItem(IDC_PREVIEW) );

	WinUtil::decodeFont(SETTING(TEXT_FONT), m_Font);
	m_BackColor = SETTING(BACKGROUND_COLOR);
	m_ForeColor = SETTING(TEXT_COLOR);

	fg = SETTING(TEXT_COLOR);
	bg = SETTING(BACKGROUND_COLOR);

	TextStyles[ TS_GENERAL ].Init( 
	this, settings, "General text", "General chat text",
	SettingsManager::TEXT_GENERAL_BACK_COLOR, SettingsManager::TEXT_GENERAL_FORE_COLOR, 
	SettingsManager::TEXT_GENERAL_BOLD, SettingsManager::TEXT_GENERAL_ITALIC );

	TextStyles[ TS_MYNICK ].Init( 
	this, settings, "My nick", "My nick",
	SettingsManager::TEXT_MYNICK_BACK_COLOR, SettingsManager::TEXT_MYNICK_FORE_COLOR, 
	SettingsManager::TEXT_MYNICK_BOLD, SettingsManager::TEXT_MYNICK_ITALIC );

	TextStyles[ TS_MYMSG ].Init( 
	this, settings, "My own message", "My own message",
	SettingsManager::TEXT_MYOWN_BACK_COLOR, SettingsManager::TEXT_MYOWN_FORE_COLOR, 
	SettingsManager::TEXT_MYOWN_BOLD, SettingsManager::TEXT_MYOWN_ITALIC );

	TextStyles[ TS_PRIVATE ].Init( 
	this, settings, "Private message", "Private message",
	SettingsManager::TEXT_PRIVATE_BACK_COLOR, SettingsManager::TEXT_PRIVATE_FORE_COLOR, 
	SettingsManager::TEXT_PRIVATE_BOLD, SettingsManager::TEXT_PRIVATE_ITALIC );

	TextStyles[ TS_SYSTEM ].Init( 
	this, settings, "System message", "System message ",
	SettingsManager::TEXT_SYSTEM_BACK_COLOR, SettingsManager::TEXT_SYSTEM_FORE_COLOR, 
	SettingsManager::TEXT_SYSTEM_BOLD, SettingsManager::TEXT_SYSTEM_ITALIC );

	TextStyles[ TS_SERVER ].Init( 
	this, settings, "Server message", "Server message",
	SettingsManager::TEXT_SERVER_BACK_COLOR, SettingsManager::TEXT_SERVER_FORE_COLOR, 
	SettingsManager::TEXT_SERVER_BOLD, SettingsManager::TEXT_SERVER_ITALIC );

	TextStyles[ TS_TIMESTAMP ].Init( 
	this, settings, "Timestamp", "The style for timestamp",
	SettingsManager::TEXT_TIMESTAMP_BACK_COLOR, SettingsManager::TEXT_TIMESTAMP_FORE_COLOR, 
	SettingsManager::TEXT_TIMESTAMP_BOLD, SettingsManager::TEXT_TIMESTAMP_ITALIC );

	TextStyles[ TS_URL ].Init( 
	this, settings, "URL (http, mailto, ...)", "URL",
	SettingsManager::TEXT_URL_BACK_COLOR, SettingsManager::TEXT_URL_FORE_COLOR, 
	SettingsManager::TEXT_URL_BOLD, SettingsManager::TEXT_URL_ITALIC );

	TextStyles[ TS_FAVORITE ].Init( 
	this, settings, "Favorite user", "Favorite user",
	SettingsManager::TEXT_FAV_BACK_COLOR, SettingsManager::TEXT_FAV_FORE_COLOR, 
	SettingsManager::TEXT_FAV_BOLD, SettingsManager::TEXT_FAV_ITALIC );

	TextStyles[ TS_OP ].Init( 
	this, settings, "Operator", "Operator",
	SettingsManager::TEXT_OP_BACK_COLOR, SettingsManager::TEXT_OP_FORE_COLOR, 
	SettingsManager::TEXT_OP_BOLD, SettingsManager::TEXT_OP_ITALIC );

	for ( int i = 0; i < TS_LAST; i++ ) {
		TextStyles[ i ].LoadSettings();
		strcpy( TextStyles[ i ].szFaceName, m_Font.lfFaceName );
		TextStyles[ i ].bCharSet = m_Font.lfCharSet;
		TextStyles[ i ].yHeight = m_Font.lfHeight;
		m_lsbList.AddString( TextStyles[ i ].m_sText.c_str() );
	}
	m_lsbList.SetCurSel( 0 );

	ctrlTabList.Attach(GetDlgItem(IDC_TABCOLOR_LIST));
	cmdResetTab.Attach(GetDlgItem(IDC_RESET_TAB_COLOR));
	cmdSetTabColor.Attach(GetDlgItem(IDC_SELECT_TAB_COLOR));
	ctrlTabExample.Attach(GetDlgItem(IDC_SAMPLE_TAB_COLOR));

	for(int i=0; i < sizeof(colours) / sizeof(clrs); i++){				
		ctrlTabList.AddString(ResourceManager::getInstance()->getString(colours[i].name).c_str());
		onResetColor(i);
	}

	setForeColor(ctrlTabExample, GetSysColor(COLOR_3DFACE));	

	ctrlTabList.SetCurSel(0);
	BOOL bTmp;
	onTabListChange(0,0,0,bTmp);

	RefreshPreview();
	return TRUE;
}

void PropPageTextStyles::write()
{
	PropPage::write((HWND)*this, items);

	string f = WinUtil::encodeFont( m_Font );
	settings->set(SettingsManager::TEXT_FONT, f);

	m_BackColor = TextStyles[ TS_GENERAL ].crBackColor;
	m_ForeColor = TextStyles[ TS_GENERAL ].crTextColor;

	settings->set(SettingsManager::TEXT_COLOR, (int)fg);
	settings->set(SettingsManager::BACKGROUND_COLOR, (int)bg);
	
	for(int i=1; i < sizeof(colours) / sizeof(clrs); i++){
		settings->set((SettingsManager::IntSetting)colours[i].setting, (int)colours[i].value);
	}

	for ( int i = 0; i < TS_LAST; i++ ) {
		TextStyles[ i ].SaveSettings();
	}
}

LRESULT PropPageTextStyles::onEditBackColor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int iNdx = m_lsbList.GetCurSel();
	TextStyles[ iNdx ].EditBackColor();
	RefreshPreview();
	return TRUE;
}

LRESULT PropPageTextStyles::onEditForeColor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int iNdx = m_lsbList.GetCurSel();
	TextStyles[ iNdx ].EditForeColor();
	RefreshPreview();
	return TRUE;
}

LRESULT PropPageTextStyles::onEditTextStyle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int iNdx = m_lsbList.GetCurSel();
	TextStyles[ iNdx ].EditTextStyle();

	strcpy( m_Font.lfFaceName, TextStyles[ iNdx ].szFaceName );
	m_Font.lfCharSet = TextStyles[ iNdx ].bCharSet;
	m_Font.lfHeight = TextStyles[ iNdx ].yHeight;

	if ( iNdx == TS_GENERAL ) {
		if ( ( TextStyles[ iNdx ].dwEffects & CFE_ITALIC ) == CFE_ITALIC )
			m_Font.lfItalic = true;
		if ( ( TextStyles[ iNdx ].dwEffects & CFE_BOLD ) == CFE_BOLD )
			m_Font.lfWeight = FW_BOLD;
	}

	for ( int i = 0; i < TS_LAST; i++ ) {
		strcpy( TextStyles[ iNdx ].szFaceName, m_Font.lfFaceName );
		TextStyles[ i ].bCharSet = m_Font.lfCharSet;
		TextStyles[ i ].yHeight = m_Font.lfHeight;
		m_Preview.AppendText( "My nick", "12:34 ", TextStyles[ i ].m_sPreviewText.c_str(), 
		TextStyles[ i ] );
	}

	RefreshPreview();
	return TRUE;
}

void PropPageTextStyles::RefreshPreview() {
	m_Preview.SetBackgroundColor( bg );
	m_Preview.SetTextStyleMyNick( TextStyles[ TS_MYNICK ] );
	m_Preview.SetWindowText("");

	string sText;
	for ( int i = 0; i < TS_LAST; i++ ) {
		m_Preview.AppendText( "My nick", "12:34 ", TextStyles[ i ].m_sPreviewText.c_str(), 
		TextStyles[ i ] );
	}
	m_Preview.InvalidateRect( NULL );
}

LRESULT PropPageTextStyles::onDefaultStyles(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	bg = RGB(0,0,96);
	fg = RGB(255,255,255);
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(m_Font), &m_Font);
	TextStyles[ TS_GENERAL ].crBackColor = RGB(0,0,96);
	TextStyles[ TS_GENERAL ].crTextColor = RGB(255,255,255);
	TextStyles[ TS_GENERAL ].dwEffects = 0;

	TextStyles[ TS_MYNICK ].crBackColor = RGB(0,0,96);
	TextStyles[ TS_MYNICK ].crTextColor = RGB(255,255,0);
	TextStyles[ TS_MYNICK ].dwEffects = CFE_BOLD;

	TextStyles[ TS_MYMSG ].crBackColor = RGB(96,0,0);
	TextStyles[ TS_MYMSG ].crTextColor = RGB(255,255,0);
	TextStyles[ TS_MYMSG ].dwEffects = CFE_BOLD;

	TextStyles[ TS_PRIVATE ].crBackColor = RGB(0,96,0);
	TextStyles[ TS_PRIVATE ].crTextColor = RGB(255,255,255);
	TextStyles[ TS_PRIVATE ].dwEffects = CFE_BOLD;

	TextStyles[ TS_SYSTEM ].crBackColor = RGB(0,0,0);
	TextStyles[ TS_SYSTEM ].crTextColor = RGB(192,192,192);
	TextStyles[ TS_SYSTEM ].dwEffects = CFE_BOLD | CFE_ITALIC;

	TextStyles[ TS_SERVER ].crBackColor = RGB(0,0,0);
	TextStyles[ TS_SERVER ].crTextColor = RGB(128,255,128);
	TextStyles[ TS_SERVER ].dwEffects = CFE_BOLD;

	TextStyles[ TS_TIMESTAMP ].crBackColor = RGB(0,0,0);
	TextStyles[ TS_TIMESTAMP ].crTextColor = RGB(255,255,0);
	TextStyles[ TS_TIMESTAMP ].dwEffects = 0;

	TextStyles[ TS_URL ].crBackColor = RGB(192,192,192);
	TextStyles[ TS_URL ].crTextColor = RGB(0,0,255);
	TextStyles[ TS_URL ].dwEffects = 0;

	TextStyles[ TS_FAVORITE ].crBackColor = RGB(0,0,0);
	TextStyles[ TS_FAVORITE ].crTextColor = RGB(255,128,128);
	TextStyles[ TS_FAVORITE ].dwEffects = CFE_BOLD | CFE_ITALIC;

	TextStyles[ TS_OP ].crBackColor = RGB(0,0,96);
	TextStyles[ TS_OP ].crTextColor = RGB(200,0,0);
	TextStyles[ TS_OP ].dwEffects = CFE_BOLD;

	RefreshPreview();
	return TRUE;
}

LRESULT PropPageTextStyles::onBlackAndWhite(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	bg = RGB(255,255,255);
	fg = RGB(0,0,0);
	TextStyles[ TS_GENERAL ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_GENERAL ].crTextColor = RGB(0,0,96);
	TextStyles[ TS_GENERAL ].dwEffects = 0;

	TextStyles[ TS_MYNICK ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_MYNICK ].crTextColor = RGB(0,0,96);
	TextStyles[ TS_MYNICK ].dwEffects = 0;

	TextStyles[ TS_MYMSG ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_MYMSG ].crTextColor = RGB(0,0,96);
	TextStyles[ TS_MYMSG ].dwEffects = 0;

	TextStyles[ TS_PRIVATE ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_PRIVATE ].crTextColor = RGB(0,0,96);
	TextStyles[ TS_PRIVATE ].dwEffects = 0;

	TextStyles[ TS_SYSTEM ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_SYSTEM ].crTextColor = RGB(0,0,96);
	TextStyles[ TS_SYSTEM ].dwEffects = 0;

	TextStyles[ TS_SERVER ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_SERVER ].crTextColor = RGB(0,0,96);
	TextStyles[ TS_SERVER ].dwEffects = 0;

	TextStyles[ TS_TIMESTAMP ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_TIMESTAMP ].crTextColor = RGB(0,0,96);
	TextStyles[ TS_TIMESTAMP ].dwEffects = 0;

	TextStyles[ TS_URL ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_URL ].crTextColor = RGB(0,0,96);
	TextStyles[ TS_URL ].dwEffects = 0;

	TextStyles[ TS_FAVORITE ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_FAVORITE ].crTextColor = RGB(0,0,96);
	TextStyles[ TS_FAVORITE ].dwEffects = 0;

	TextStyles[ TS_OP ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_OP ].crTextColor = RGB(0,0,96);
	TextStyles[ TS_OP ].dwEffects = 0;

	RefreshPreview();
	return TRUE;
}

void PropPageTextStyles::TextStyleSettings::Init( 
	PropPageTextStyles *pParent, SettingsManager *pSM, 
	LPCTSTR sText, LPCTSTR sPreviewText,
	SettingsManager::IntSetting iBack, SettingsManager::IntSetting iFore, 
	SettingsManager::IntSetting iBold, SettingsManager::IntSetting iItalic ) {

	CHARFORMAT2 cf;
	cbSize = sizeof( cf );
	dwMask = CFM_COLOR | CFM_BOLD | CFM_ITALIC | CFM_BACKCOLOR;
	dwReserved = 0;
  
	m_pParent = pParent;
	settings = pSM;      
	m_sText = sText;
	m_sPreviewText = sPreviewText;
	m_iBackColor = iBack;
	m_iForeColor = iFore;
	m_iBold = iBold;
	m_iItalic = iItalic;
}

void PropPageTextStyles::TextStyleSettings::LoadSettings() {
	dwEffects = 0;
	crBackColor = settings->get( m_iBackColor );
	crTextColor = settings->get( m_iForeColor );
	if ( settings->get( m_iBold ) ) dwEffects |= CFE_BOLD;
	if ( settings->get( m_iItalic) ) dwEffects |= CFE_ITALIC;
}

void PropPageTextStyles::TextStyleSettings::SaveSettings() {
	settings->set( m_iBackColor, (int) crBackColor);
	settings->set( m_iForeColor, (int) crTextColor);
	BOOL boBold = ( ( dwEffects & CFE_BOLD ) == CFE_BOLD );
	settings->set( m_iBold, (int) boBold);
	BOOL boItalic = ( ( dwEffects & CFE_ITALIC ) == CFE_ITALIC );
	settings->set( m_iItalic, (int) boItalic);
}

void PropPageTextStyles::TextStyleSettings::EditBackColor() {
	CColorDialog d( crBackColor, 0, *m_pParent );
	if (d.DoModal() == IDOK) {
		crBackColor = d.GetColor();
	}
}

void PropPageTextStyles::TextStyleSettings::EditForeColor() {
	CColorDialog d( crTextColor, 0, *m_pParent );
	if (d.DoModal() == IDOK) {
		crTextColor = d.GetColor();
	}
}

void PropPageTextStyles::TextStyleSettings::EditTextStyle() {
	LOGFONT font;
	WinUtil::decodeFont( SETTING(TEXT_FONT), font );

	strcpy( font.lfFaceName, szFaceName );
	font.lfCharSet = bCharSet;
	font.lfHeight = yHeight;

	if ( dwEffects & CFE_BOLD ) 
		font.lfWeight = FW_BOLD;
	else
		font.lfWeight = FW_REGULAR;

	if ( dwEffects & CFE_ITALIC ) 
		font.lfItalic = true;
	else
		font.lfItalic = false;

	CFontDialog d( &font, CF_SCREENFONTS, NULL, *m_pParent );
	d.m_cf.rgbColors = crTextColor;
	if (d.DoModal() == IDOK) {
  	strcpy( szFaceName, font.lfFaceName );
	bCharSet = font.lfCharSet;
	yHeight = font.lfHeight;

	crTextColor = d.m_cf.rgbColors;
	if ( font.lfWeight == FW_BOLD )
		dwEffects |= CFE_BOLD;
	else
		dwEffects &= ~CFE_BOLD;

	if ( font.lfItalic )
		dwEffects |= CFE_ITALIC;
	else
		dwEffects &= ~CFE_ITALIC;
	}
}

LRESULT PropPageTextStyles::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	if (m_lsbList.m_hWnd != NULL)
		m_lsbList.Detach();
	if (m_Preview.m_hWnd != NULL)
		m_Preview.Detach();
	return 1;
}

LRESULT PropPageTextStyles::onImport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	string x = "";	
	if(WinUtil::browseFile(x, m_hWnd, false) == IDOK) {

	SimpleXML xml;
	xml.fromXML(File(x, File::READ, File::OPEN).read());
	xml.resetCurrentChild();
	xml.stepIn();
	if(xml.findChild("Settings")) {
		xml.stepIn();

		if(xml.findChild("Font")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_FONT,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("BackgroundColor")) { SettingsManager::getInstance()->set(SettingsManager::BACKGROUND_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("DownloadBarColor")) { SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_BAR_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("UploadBarColor")) { SettingsManager::getInstance()->set(SettingsManager::UPLOAD_BAR_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextGeneralBackColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_GENERAL_BACK_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextGeneralForeColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_GENERAL_FORE_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextGeneralBold")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_GENERAL_BOLD,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextGeneralItalic")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_GENERAL_ITALIC,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextMyOwnBackColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_MYOWN_BACK_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextMyOwnForeColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_MYOWN_FORE_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextMyOwnBold")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_MYOWN_BOLD,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextMyOwnItalic")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_MYOWN_ITALIC,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextPrivateBackColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_PRIVATE_BACK_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextPrivateForeColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_PRIVATE_FORE_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextPrivateBold")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_PRIVATE_BOLD,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextPrivateItalic")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_PRIVATE_ITALIC,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextSystemBackColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_SYSTEM_BACK_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextSystemForeColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_SYSTEM_FORE_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextSystemBold")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_SYSTEM_BOLD,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextSystemItalic")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_SYSTEM_ITALIC,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextServerBackColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_SERVER_BACK_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextServerForeColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_SERVER_FORE_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextServerBold")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_SERVER_BOLD,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextServerItalic")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_SERVER_ITALIC,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextTimestampBackColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_TIMESTAMP_BACK_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextTimestampForeColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_TIMESTAMP_FORE_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextTimestampBold")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_TIMESTAMP_BOLD,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextTimestampItalic")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_TIMESTAMP_ITALIC,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextMyNickBackColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_MYNICK_BACK_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextMyNickForeColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_MYNICK_FORE_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextMyNickBold")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_MYNICK_BOLD,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextMyNickItalic")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_MYNICK_ITALIC,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextFavBackColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_FAV_BACK_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextFavForeColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_FAV_FORE_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextFavBold")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_FAV_BOLD,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextFavItalic")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_FAV_ITALIC,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextURLBackColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_URL_BACK_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextURLForeColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_URL_FORE_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextURLBold")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_URL_BOLD,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextURLItalic")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_URL_ITALIC,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("BoldAuthorsMess")) { SettingsManager::getInstance()->set(SettingsManager::BOLD_AUTHOR_MESS,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("ProgressTextDown")) { SettingsManager::getInstance()->set(SettingsManager::PROGRESS_TEXT_COLOR_DOWN,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("ProgressTextUp")) { SettingsManager::getInstance()->set(SettingsManager::PROGRESS_TEXT_COLOR_UP,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("ErrorColor")) { SettingsManager::getInstance()->set(SettingsManager::ERROR_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("ProgressOverrideColors")) { SettingsManager::getInstance()->set(SettingsManager::PROGRESS_OVERRIDE_COLORS,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("MenubarTwoColors")) { SettingsManager::getInstance()->set(SettingsManager::MENUBAR_TWO_COLORS,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("MenubarLeftColor")) { SettingsManager::getInstance()->set(SettingsManager::MENUBAR_LEFT_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("MenubarRightColor")) { SettingsManager::getInstance()->set(SettingsManager::MENUBAR_RIGHT_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("MenubarBumped")) { SettingsManager::getInstance()->set(SettingsManager::MENUBAR_BUMPED,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("ProgressBumped")) { SettingsManager::getInstance()->set(SettingsManager::PROGRESS_BUMPED,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("ProgressOverrideColors2")) { SettingsManager::getInstance()->set(SettingsManager::PROGRESS_OVERRIDE_COLORS2,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextOPBackColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_OP_BACK_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextOPForeColor")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_OP_FORE_COLOR,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextOPBold")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_OP_BOLD,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("TextOPItalic")) { SettingsManager::getInstance()->set(SettingsManager::TEXT_OP_ITALIC,xml.getChildData());}
		xml.resetCurrentChild();
		if(xml.findChild("SearchAlternateColour")) { SettingsManager::getInstance()->set(SettingsManager::SEARCH_ALTERNATE_COLOUR,xml.getChildData());}
		xml.resetCurrentChild();
	}

	xml.resetCurrentChild();
	xml.stepOut();
	}

	SendMessage(WM_DESTROY,0,0);
	SettingsManager::getInstance()->save();
	PropertiesDlg::needUpdate = true;
	SendMessage(WM_INITDIALOG,0,0);

//	RefreshPreview();
	return 0;
}

LRESULT PropPageTextStyles::onExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	string x = "";	
	if(WinUtil::browseFile(x, m_hWnd, true) == IDOK) {
	SimpleXML xml;
	xml.addTag("DCPlusPlus");
	xml.stepIn();
	xml.addTag("Settings");
	xml.stepIn();

	string type("type"), curType("string");
	xml.addTag("Font", SETTING(TEXT_FONT));
	xml.addChildAttrib(type, curType);

	curType = "int";
	xml.addTag("BackgroundColor", SETTING(BACKGROUND_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextColor", SETTING(TEXT_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("DownloadBarColor", SETTING(DOWNLOAD_BAR_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("UploadBarColor", SETTING(UPLOAD_BAR_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextGeneralBackColor", SETTING(TEXT_GENERAL_BACK_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextGeneralForeColor", SETTING(TEXT_GENERAL_FORE_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextGeneralBold", SETTING(TEXT_GENERAL_BOLD));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextGeneralItalic", SETTING(TEXT_GENERAL_ITALIC));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextMyOwnBackColor", SETTING(TEXT_MYOWN_BACK_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextMyOwnForeColor", SETTING(TEXT_MYOWN_FORE_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextMyOwnBold", SETTING(TEXT_MYOWN_BOLD));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextMyOwnItalic", SETTING(TEXT_MYOWN_ITALIC));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextPrivateBackColor", SETTING(TEXT_PRIVATE_BACK_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextPrivateForeColor", SETTING(TEXT_PRIVATE_FORE_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextPrivateBold", SETTING(TEXT_PRIVATE_BOLD));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextPrivateItalic", SETTING(TEXT_PRIVATE_ITALIC));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextSystemBackColor", SETTING(TEXT_SYSTEM_BACK_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextSystemForeColor", SETTING(TEXT_SYSTEM_FORE_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextSystemBold", SETTING(TEXT_SYSTEM_BOLD));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextSystemItalic", SETTING(TEXT_SYSTEM_ITALIC));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextServerBackColor", SETTING(TEXT_SERVER_BACK_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextServerForeColor", SETTING(TEXT_SERVER_FORE_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextServerBold", SETTING(TEXT_SERVER_BOLD));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextServerItalic", SETTING(TEXT_SERVER_ITALIC));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextTimestampBackColor", SETTING(TEXT_TIMESTAMP_BACK_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextTimestampForeColor", SETTING(TEXT_TIMESTAMP_FORE_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextTimestampBold", SETTING(TEXT_TIMESTAMP_BOLD));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextTimestampItalic", SETTING(TEXT_TIMESTAMP_ITALIC));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextMyNickBackColor", SETTING(TEXT_MYNICK_BACK_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextMyNickForeColor", SETTING(TEXT_MYNICK_FORE_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextMyNickBold", SETTING(TEXT_MYNICK_BOLD));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextMyNickItalic", SETTING(TEXT_MYNICK_ITALIC));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextFavBackColor", SETTING(TEXT_FAV_BACK_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextFavForeColor", SETTING(TEXT_FAV_FORE_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextFavBold", SETTING(TEXT_FAV_BOLD));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextFavItalic", SETTING(TEXT_FAV_ITALIC));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextURLBackColor", SETTING(TEXT_URL_BACK_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextURLForeColor", SETTING(TEXT_URL_FORE_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextURLBold", SETTING(TEXT_URL_BOLD));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextURLItalic", SETTING(TEXT_URL_ITALIC));
	xml.addChildAttrib(type, curType);
	xml.addTag("BoldAuthorsMess", SETTING(BOLD_AUTHOR_MESS));
	xml.addChildAttrib(type, curType);
	xml.addTag("ProgressTextDown", SETTING(PROGRESS_TEXT_COLOR_DOWN));
	xml.addChildAttrib(type, curType);
	xml.addTag("ProgressTextUp", SETTING(PROGRESS_TEXT_COLOR_UP));
	xml.addChildAttrib(type, curType);
	xml.addTag("ErrorColor", SETTING(ERROR_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("ProgressOverrideColors", SETTING(PROGRESS_OVERRIDE_COLORS));
	xml.addChildAttrib(type, curType);
	xml.addTag("MenubarTwoColors", SETTING(MENUBAR_TWO_COLORS));
	xml.addChildAttrib(type, curType);
	xml.addTag("MenubarLeftColor", SETTING(MENUBAR_LEFT_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("MenubarRightColor", SETTING(MENUBAR_RIGHT_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("MenubarBumped", SETTING(MENUBAR_BUMPED));
	xml.addChildAttrib(type, curType);
	xml.addTag("ProgressBumped", SETTING(PROGRESS_BUMPED));
	xml.addChildAttrib(type, curType);
	xml.addTag("ProgressOverrideColors2", SETTING(PROGRESS_OVERRIDE_COLORS2));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextOPBackColor", SETTING(TEXT_OP_BACK_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextOPForeColor", SETTING(TEXT_OP_FORE_COLOR));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextOPBold", SETTING(TEXT_OP_BOLD));
	xml.addChildAttrib(type, curType);
	xml.addTag("TextOPItalic", SETTING(TEXT_OP_ITALIC));
	xml.addChildAttrib(type, curType);
	xml.addTag("SearchAlternateColour", SETTING(SEARCH_ALTERNATE_COLOUR));
	xml.addChildAttrib(type, curType);
	
	try {
		File ff(x , File::WRITE, File::CREATE | File::TRUNCATE);
		BufferedOutputStream<false> f(&ff);
		f.write(SimpleXML::w1252Header);
		xml.toXML(&f);
		f.flush();
		ff.close();
	} catch(const FileException&) {
		// ...
	}

	}
	return 0;
}

void PropPageTextStyles::onResetColor(int i){
	colours[i].value = SettingsManager::getInstance()->get((SettingsManager::IntSetting)colours[i].setting, true);
}

LRESULT PropPageTextStyles::onTabListChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	COLORREF color = colours[ctrlTabList.GetCurSel()].value;
	setForeColor(ctrlTabExample, color);
	RefreshPreview();
	return S_OK;
}

LRESULT PropPageTextStyles::onClickedResetTabColor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	onResetColor(ctrlTabList.GetCurSel());
	return S_OK;
}

LRESULT PropPageTextStyles::onClientSelectTabColor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	CColorDialog d(colours[ctrlTabList.GetCurSel()].value, 0, *this);
	if (d.DoModal() == IDOK) {
		colours[ctrlTabList.GetCurSel()].value = d.GetColor();
		switch(ctrlTabList.GetCurSel()) {
			case 0: bg = d.GetColor(); break;
		}
		setForeColor(ctrlTabExample, d.GetColor());
		RefreshPreview();
	}
	return S_OK;
}

LRESULT PropPageTextStyles::onClickedText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	LOGFONT tmp = m_Font;
	CFontDialog d(&tmp, CF_EFFECTS | CF_SCREENFONTS, NULL, *this);
	d.m_cf.rgbColors = fg;
	if(d.DoModal() == IDOK)
	{
		m_Font = tmp;
		fg = d.GetColor();
	}
	RefreshPreview();
	return TRUE;
}

LRESULT PropPageTextStyles::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	HWND hWnd = (HWND)lParam;
	if(hWnd == ctrlTabExample.m_hWnd) {
		::SetBkMode((HDC)wParam, TRANSPARENT);
		HANDLE h = GetProp(hWnd, "fillcolor");
		if (h != NULL) {
			return (LRESULT)h;
		}
		return TRUE;
	} else {
		return FALSE;
	}
}
