
#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "ClientsPage.h"
#include "ClientProfileDlg.h"

#include "../client/SettingsManager.h"
#include "../client/HubManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PropPage::TextItem ClientsPage::texts[] = {
	{ IDC_MOVE_CLIENT_UP, ResourceManager::MOVE_UP },
	{ IDC_MOVE_CLIENT_DOWN, ResourceManager::MOVE_DOWN },
	{ IDC_ADD_CLIENT, ResourceManager::ADD },
	{ IDC_CHANGE_CLIENT, ResourceManager::SETTINGS_CHANGE },
	{ IDC_REMOVE_CLIENT, ResourceManager::REMOVE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item ClientsPage::items[] = {
	{ 0, 0, PropPage::T_END }
};

LRESULT ClientsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	CRect rc;

	ctrlProfiles.Attach(GetDlgItem(IDC_CLIENT_ITEMS));
	ctrlProfiles.GetClientRect(rc);

	ctrlProfiles.InsertColumn(0, CSTRING(SETTINGS_NAME), LVCFMT_LEFT, rc.Width() / 2, 0);
	ctrlProfiles.InsertColumn(1, CSTRING(SETTINGS_CLIENT_VER), LVCFMT_LEFT, rc.Width() / 2, 1);

	if(BOOLSETTING(FULL_ROW_SELECT))
		ctrlProfiles.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);

	// Do specialized reading here
	ClientProfile::List lst = HubManager::getInstance()->getClientProfiles();
	StringList cols;
	for(ClientProfile::Iter i = lst.begin(); i != lst.end(); ++i) {
		ClientProfile& cp = *i;	
		addEntry(cp, ctrlProfiles.GetItemCount());
	}
	
	return TRUE;
}

LRESULT ClientsPage::onAddClient(WORD , WORD , HWND , BOOL& ) {
	ClientProfileDlg dlg;

	if(dlg.DoModal() == IDOK) {
		addEntry(HubManager::getInstance()->addClientProfile(
			dlg.name, 
			dlg.version, 
			dlg.tag, 
			dlg.extendedTag, 
			dlg.lock, 
			dlg.pk, 
			dlg.supports, 
			dlg.testSUR, 
			dlg.userConCom, 
			dlg.status,
			dlg.cheatingDescription,
			dlg.rawToSend, 
			dlg.tagVersion, 
			dlg.useExtraVersion,
			dlg.checkMismatch
			), ctrlProfiles.GetItemCount());
		
	}
	return 0;
}

LRESULT ClientsPage::onChangeClient(WORD , WORD , HWND , BOOL& ) {
	if(ctrlProfiles.GetSelectedCount() == 1) {
		int sel = ctrlProfiles.GetSelectedIndex();
		ClientProfile cp;
		HubManager::getInstance()->getClientProfile(ctrlProfiles.GetItemData(sel), cp);
		
		ClientProfileDlg dlg;
		
		dlg.name = cp.getName();
		dlg.version = cp.getVersion();
		dlg.tag = cp.getTag();
		dlg.extendedTag = cp.getExtendedTag();
		dlg.lock = cp.getLock();
		dlg.pk = cp.getPk();
		dlg.supports = cp.getSupports();
		dlg.testSUR = cp.getTestSUR();
		dlg.userConCom = cp.getUserConCom();
		dlg.status = cp.getStatus();
		dlg.cheatingDescription = cp.getCheatingDescription();
		dlg.rawToSend = cp.getRawToSend();
		dlg.tagVersion = cp.getTagVersion();
		dlg.useExtraVersion = cp.getUseExtraVersion();
		dlg.checkMismatch = cp.getCheckMismatch();

		if(dlg.DoModal() == IDOK) {
			ctrlProfiles.SetItemText(sel, 0, dlg.name.c_str());
			ctrlProfiles.SetItemText(sel, 1, dlg.version.c_str());
			cp.setName(dlg.name);
			cp.setVersion(dlg.version);
			cp.setTag(dlg.tag);
			cp.setExtendedTag(dlg.extendedTag);
			cp.setLock(dlg.lock);
			cp.setPk(dlg.pk);
			cp.setSupports(dlg.supports);
			cp.setTestSUR(dlg.testSUR);
			cp.setUserConCom(dlg.userConCom);
			cp.setStatus(dlg.status);
			cp.setCheatingDescription(dlg.cheatingDescription);
			cp.setRawToSend(dlg.rawToSend);
			cp.setTagVersion(dlg.tagVersion);
			cp.setUseExtraVersion(dlg.useExtraVersion);
			cp.setCheckMismatch(dlg.checkMismatch);
			HubManager::getInstance()->updateClientProfile(cp);
		}
	}
	return 0;
}

LRESULT ClientsPage::onRemoveClient(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlProfiles.GetSelectedCount() == 1) {
		int i = ctrlProfiles.GetNextItem(-1, LVNI_SELECTED);
		HubManager::getInstance()->removeClientProfile(ctrlProfiles.GetItemData(i));
		ctrlProfiles.DeleteItem(i);
		HubManager::getInstance()->sortPriorities();
	}
	return 0;
}

LRESULT ClientsPage::onMoveClientUp(WORD , WORD , HWND , BOOL& ) {
	int i = ctrlProfiles.GetSelectedIndex();
	if(i != -1 && i != 0) {
		int n = ctrlProfiles.GetItemData(i);
		HubManager::getInstance()->moveClientProfile(n, -1, i);
		ctrlProfiles.SetRedraw(FALSE);
		ctrlProfiles.DeleteItem(i);
		ClientProfile cp;
		HubManager::getInstance()->getClientProfile(n, cp);
		addEntry(cp, i-1);
		ctrlProfiles.SelectItem(i-1);
		ctrlProfiles.EnsureVisible(i-1, FALSE);
		ctrlProfiles.SetRedraw(TRUE);
	}
	return 0;
}

LRESULT ClientsPage::onMoveClientDown(WORD , WORD , HWND , BOOL& ) {
	int i = ctrlProfiles.GetSelectedIndex();
	if(i != -1 && i != (ctrlProfiles.GetItemCount()-1) ) {
		int n = ctrlProfiles.GetItemData(i);
		HubManager::getInstance()->moveClientProfile(n, 1, i);
		ctrlProfiles.SetRedraw(FALSE);
		ctrlProfiles.DeleteItem(i);
		ClientProfile cp;
		HubManager::getInstance()->getClientProfile(n, cp);
		addEntry(cp, i+1);
		ctrlProfiles.SelectItem(i+1);
		ctrlProfiles.EnsureVisible(i+1, FALSE);
		ctrlProfiles.SetRedraw(TRUE);
	}
	return 0;
}

void ClientsPage::addEntry(const ClientProfile& cp, int pos) {
	StringList lst;

	lst.push_back(cp.getName());
	lst.push_back(cp.getVersion());
	ctrlProfiles.insert(pos, lst, 0, (LPARAM)cp.getId());
	HubManager::getInstance()->sortPriorities();
}

void ClientsPage::write() {
	HubManager::getInstance()->save();
	PropPage::write((HWND)*this, items);
}
/**
 * @file
 * $Id$
 */