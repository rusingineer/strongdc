#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "CZDCPage.h"
#include "../client/SettingsManager.h"
#include "WinUtil.h"

PropPage::TextItem CZDCPage::texts[] = {
	{ IDC_CZDC_WINAMP, ResourceManager::SETCZDC_WINAMP },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
}; 

CZDCPage* current_page;

PropPage::Item CZDCPage::items[] = {
	{ IDC_WINAMP, SettingsManager::WINAMP_FORMAT, PropPage::T_STR },
	{ 0, 0, PropPage::T_END }
};

CZDCPage::ListItem CZDCPage::listItems[] = {
	{ SettingsManager::EMPTY_WORKING_SET, ResourceManager::SETTINGS_EMPTY_WORKING_SET },
	{ SettingsManager::USE_EMOTICONS, ResourceManager::ENABLE_EMOTICONS },
	{ SettingsManager::CHECK_TTH, ResourceManager::CHECK_TTH_AFTER_DOWNLOAD },
	{ SettingsManager::SEARCH_TTH_ONLY, ResourceManager::SETTINGS_ONLY_TTH },
	{ SettingsManager::CZCHARS_DISABLE, ResourceManager::SETCZDC_CZCHARS_DISABLE },
	{ SettingsManager::DEBUG_COMMANDS, ResourceManager::SETTINGS_DEBUG_COMMANDS },
	{ SettingsManager::AUTO_PRIORITY_DEFAULT ,ResourceManager::SETTINGS_AUTO_PRIORITY_DEFAULT },
	{ SettingsManager::GARBAGE_COMMAND_INCOMING, ResourceManager::GARBAGE_INCOMING },
	{ SettingsManager::GARBAGE_COMMAND_OUTGOING, ResourceManager::GARBAGE_OUTGOING },
	{ SettingsManager::AUTO_DROP_SOURCE, ResourceManager::SETTINGS_AUTO_DROP_SOURCE },
	{ SettingsManager::USE_OLD_SHARING_UI, ResourceManager::SETTINGS_USE_OLD_SHARING_UI },
	{ SettingsManager::WEBSERVER, ResourceManager::SETTINGS_WEBSERVER }, 
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};


LRESULT CZDCPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_ADVANCED_BOOLEANS));

	// Do specialized reading here
	
	return TRUE;
}

void CZDCPage::write()
{
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_ADVANCED_BOOLEANS));

	// Do specialized writing here
	// settings->set(XX, YY);

}

LRESULT CZDCPage::onClickedWinampHelp(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	MessageBox(CSTRING(WINAMP_HELP), CSTRING(WINAMP_HELP_DESC), MB_OK | MB_ICONINFORMATION);
	return S_OK;
}
