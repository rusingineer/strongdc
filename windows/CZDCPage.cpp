
#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "CZDCPage.h"
#include "../client/SettingsManager.h"
#include "WinUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PropPage::TextItem CZDCPage::texts[] = {
	{ IDC_CZDC_FEAT, ResourceManager::SETCZDC_FEAT },
	{ IDC_CZDC_WINAMP, ResourceManager::SETCZDC_WINAMP },
	{ IDC_MAXCOMPRESS, ResourceManager::SETTINGS_MAX_COMPRESS },
	{ IDC_MAXSOURCES, ResourceManager::SETTINGS_MAX_SOURCES },
	{ IDC_CLIENT_EMU, ResourceManager::CLIENT_EMU },
	{ IDC_SETCZDC_MAX_EMOTICONS, ResourceManager::SETCZDC_MAX_EMOTICONS },
	{ IDC_SETTINGS_ODC_SHUTDOWNTIMEOUT, ResourceManager::SETTINGS_ODC_SHUTDOWNTIMEOUT },
	{ IDC_SAVEQUEUE_TEXT, ResourceManager::SETTINGS_SAVEQUEUE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
}; 

CZDCPage* current_page;

PropPage::Item CZDCPage::items[] = {
	{ IDC_WINAMP, SettingsManager::WINAMP_FORMAT, PropPage::T_STR },
	{ IDC_MAX_COMPRESSION, SettingsManager::MAX_COMPRESSION, PropPage::T_INT },
	{ IDC_MAX_SOURCES, SettingsManager::MAX_SOURCES, PropPage::T_INT },
	{ IDC_EMULATION, SettingsManager::CLIENT_EMULATION, PropPage::T_INT },
	{ IDC_MAX_EMOTICONS, SettingsManager::MAX_EMOTICONS, PropPage::T_INT },
	{ IDC_SHUTDOWNTIMEOUT, SettingsManager::SHUTDOWN_TIMEOUT, PropPage::T_INT },
	{ IDC_SAVEQUEUE, SettingsManager::AUTOSAVE_QUEUE, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

CZDCPage::ListItem CZDCPage::listItems[] = {
	{ SettingsManager::FLOOD_CACHE, ResourceManager::SETTINGS_ODC_FLOOD_CACHE },
//	{ SettingsManager::MEMORY_MAPPED_FILE, ResourceManager::SETTINGS_MEMORY_MAPPED_FILE },
	{ SettingsManager::EMPTY_WORKING_SET, ResourceManager::SETTINGS_EMPTY_WORKING_SET },
	{ SettingsManager::USE_EMOTICONS, ResourceManager::ENABLE_EMOTICONS },
	{ SettingsManager::CHECK_TTH, ResourceManager::CHECK_TTH_AFTER_DOWNLOAD },
	{ SettingsManager::SEARCH_TTH_ONLY, ResourceManager::SETTINGS_ONLY_TTH },
	{ SettingsManager::CZCHARS_DISABLE, ResourceManager::SETCZDC_CZCHARS_DISABLE },
	{ SettingsManager::DEBUG_COMMANDS, ResourceManager::SETTINGS_DEBUG_COMMANDS },
	{ SettingsManager::AUTO_PRIORITY_DEFAULT ,ResourceManager::SETTINGS_AUTO_PRIORITY_DEFAULT },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};


LRESULT CZDCPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_ADVANCED_BOOLEANS));

	CUpDownCtrl updown;
	updown.Attach(GetDlgItem(IDC_MAX_COMP_SPIN));
	updown.SetRange(0, 9);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_MAX_SOURCES_SPIN));
	updown.SetRange(1, 100);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_SHUTDOWN_SPIN));
	updown.SetRange(1, 3600);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_MAX_EMOTICONSSPIN));
	updown.SetRange32(1, 999);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_SAVEQUEUE_SPIN));
	updown.SetRange32(5, 999);
	updown.Detach();
	cClientEmu.Attach(GetDlgItem(IDC_EMULATION));
	
	for(int i = 0; i < SettingsManager::CLIENT_EMULATION_LAST; i++)
		cClientEmu.AddString(SettingsManager::clientEmulations[i].c_str());
	cClientEmu.SetCurSel(SETTING(CLIENT_EMULATION));

	cClientEmu.Detach();

	// Do specialized reading here
	
	return TRUE;
}

void CZDCPage::write()
{
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_ADVANCED_BOOLEANS));

	cClientEmu.Attach(GetDlgItem(IDC_EMULATION));
	settings->set(SettingsManager::CLIENT_EMULATION, cClientEmu.GetCurSel());
	cClientEmu.Detach();

	// Do specialized writing here
	// settings->set(XX, YY);

}

LRESULT CZDCPage::onClickedWinampHelp(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	MessageBox(CSTRING(WINAMP_HELP), CSTRING(WINAMP_HELP_DESC), MB_OK | MB_ICONINFORMATION);
	return S_OK;
}
