
#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "ClientsPage.h"
#include "ClientProfileDlg.h"

#include "../client/SettingsManager.h"
#include "../client/ClientProfileManager.h"

PropPage::TextItem ClientsPage::texts[] = {
	{ IDC_MOVE_CLIENT_UP, ResourceManager::MOVE_UP },
	{ IDC_MOVE_CLIENT_DOWN, ResourceManager::MOVE_DOWN },
	{ IDC_ADD_CLIENT, ResourceManager::ADD },
	{ IDC_CHANGE_CLIENT, ResourceManager::SETTINGS_CHANGE },
	{ IDC_REMOVE_CLIENT, ResourceManager::REMOVE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item ClientsPage::items[] = {
	{ IDC_UPDATE_URL, SettingsManager::UPDATE_URL, PropPage::T_STR }, 
	{ 0, 0, PropPage::T_END }
};

LRESULT ClientsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	CRect rc;

	ctrlProfiles.Attach(GetDlgItem(IDC_CLIENT_ITEMS));
	ctrlProfiles.GetClientRect(rc);

	ctrlProfiles.InsertColumn(0, CTSTRING(SETTINGS_NAME), LVCFMT_LEFT, rc.Width() / 2, 0);
	ctrlProfiles.InsertColumn(1, _T("Comment"), LVCFMT_LEFT, rc.Width() / 2, 1);

	ctrlProfiles.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);

	c.addListener(this);

	// Do specialized reading here
	ClientProfile::List lst = ClientProfileManager::getInstance()->getClientProfiles();

	for(ClientProfile::Iter i = lst.begin(); i != lst.end(); ++i) {
		ClientProfile& cp = *i;	
		addEntry(cp, ctrlProfiles.GetItemCount());
	}
	
	return TRUE;
}

LRESULT ClientsPage::onAddClient(WORD , WORD , HWND , BOOL& ) {
	ClientProfileDlg dlg;
	dlg.currentProfileId = -1;

	if(dlg.DoModal() == IDOK) {
		addEntry(ClientProfileManager::getInstance()->addClientProfile(
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
//			dlg.tagVersion, 
			dlg.useExtraVersion,
			dlg.checkMismatch,
			dlg.connection,
			dlg.comment,
			false,
			false
			), ctrlProfiles.GetItemCount());
		
	}
	return 0;
}

LRESULT ClientsPage::onChangeClient(WORD , WORD , HWND , BOOL& ) {
	if(ctrlProfiles.GetSelectedCount() == 1) {
		int sel = ctrlProfiles.GetSelectedIndex();
		
		ClientProfileDlg dlg;

		dlg.currentProfileId = ctrlProfiles.GetItemData(sel);

		if(dlg.DoModal() == IDOK) {
			ctrlProfiles.SetItemText(dlg.currentProfileId, 0, Text::toT(dlg.name).c_str());
			ctrlProfiles.SetItemText(dlg.currentProfileId, 1, Text::toT(dlg.version).c_str());
			dlg.currentProfile.setName(dlg.name);
			dlg.currentProfile.setVersion(dlg.version);
			dlg.currentProfile.setTag(dlg.tag);
			dlg.currentProfile.setExtendedTag(dlg.extendedTag);
			dlg.currentProfile.setLock(dlg.lock);
			dlg.currentProfile.setPk(dlg.pk);
			dlg.currentProfile.setSupports(dlg.supports);
			dlg.currentProfile.setTestSUR(dlg.testSUR);
			dlg.currentProfile.setUserConCom(dlg.userConCom);
			dlg.currentProfile.setStatus(dlg.status);
			dlg.currentProfile.setCheatingDescription(dlg.cheatingDescription);
			dlg.currentProfile.setRawToSend(dlg.rawToSend);
//			dlg.currentProfile.setTagVersion(dlg.tagVersion);
			dlg.currentProfile.setUseExtraVersion(dlg.useExtraVersion);
			dlg.currentProfile.setCheckMismatch(dlg.checkMismatch);
			dlg.currentProfile.setConnection(dlg.connection);
			dlg.currentProfile.setComment(dlg.comment);
//			dlg.currentProfile.setRecheck(dlg.recheck);
//			dlg.currentProfile.setSkipExtended(dlg.skipExtended);
			ClientProfileManager::getInstance()->updateClientProfile(dlg.currentProfile);
		}
		ctrlProfiles.SetRedraw(FALSE);
		ctrlProfiles.DeleteAllItems();
		ClientProfile::List lst = ClientProfileManager::getInstance()->getClientProfiles();
		for(ClientProfile::Iter j = lst.begin(); j != lst.end(); ++j) {
			ClientProfile& cp = *j;	
			addEntry(cp, ctrlProfiles.GetItemCount());
		}
		ctrlProfiles.SelectItem(sel);
		ctrlProfiles.SetRedraw(TRUE);
	}
	return 0;
}

LRESULT ClientsPage::onRemoveClient(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlProfiles.GetSelectedCount() == 1) {
		int i = ctrlProfiles.GetNextItem(-1, LVNI_SELECTED);
		ClientProfileManager::getInstance()->removeClientProfile(ctrlProfiles.GetItemData(i));
		ctrlProfiles.DeleteItem(i);
	}
	return 0;
}

LRESULT ClientsPage::onMoveClientUp(WORD , WORD , HWND , BOOL& ) {
	int i = ctrlProfiles.GetSelectedIndex();
	if(i != -1 && i != 0) {
		int n = ctrlProfiles.GetItemData(i);
		ClientProfileManager::getInstance()->moveClientProfile(n, -1, i);
		ctrlProfiles.SetRedraw(FALSE);
		ctrlProfiles.DeleteItem(i);
		ClientProfile cp;
		ClientProfileManager::getInstance()->getClientProfile(n, cp);
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
		ClientProfileManager::getInstance()->moveClientProfile(n, 1, i);
		ctrlProfiles.SetRedraw(FALSE);
		ctrlProfiles.DeleteItem(i);
		ClientProfile cp;
		ClientProfileManager::getInstance()->getClientProfile(n, cp);
		addEntry(cp, i+1);
		ctrlProfiles.SelectItem(i+1);
		ctrlProfiles.EnsureVisible(i+1, FALSE);
		ctrlProfiles.SetRedraw(TRUE);
	}
	return 0;
}

LRESULT ClientsPage::onReload(WORD , WORD , HWND , BOOL& ) {
	reload();
	return 0;
}

LRESULT ClientsPage::onUpdate(WORD , WORD , HWND , BOOL& ) {
	c.downloadFile(SETTING(UPDATE_URL) + "Profiles.xml");
	return 0;
}

LRESULT ClientsPage::onInfoTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	int item = ctrlProfiles.GetHotItem();
	if(item != -1) {
		NMLVGETINFOTIP* lpnmtdi = (NMLVGETINFOTIP*) pnmh;
		ClientProfile cp;

		ClientProfileManager::getInstance()->getClientProfile(ctrlProfiles.GetItemData(item), cp);

		tstring infoTip = Text::toT("Name: " + cp.getName() +
			"\r\nComment: " + cp.getComment() +
			"\r\nCheating description: " + cp.getCheatingDescription());
		//	"\r\nRaw command: ");

		_tcscpy(lpnmtdi->pszText, infoTip.c_str());
	}
	return 0;
}

void ClientsPage::reload() {
	ctrlProfiles.SetRedraw(FALSE);
	ctrlProfiles.DeleteAllItems();
	ClientProfile::List lst = ClientProfileManager::getInstance()->reloadClientProfiles();
	for(ClientProfile::Iter j = lst.begin(); j != lst.end(); ++j) {
		ClientProfile& cp = *j;	
		addEntry(cp, ctrlProfiles.GetItemCount());
	}
	ctrlProfiles.SetRedraw(TRUE);
}

void ClientsPage::reloadFromHttp() {
	ctrlProfiles.SetRedraw(FALSE);
	ctrlProfiles.DeleteAllItems();
	ClientProfile::List lst = ClientProfileManager::getInstance()->reloadClientProfilesFromHttp();
	for(ClientProfile::Iter j = lst.begin(); j != lst.end(); ++j) {
		ClientProfile& cp = *j;	
		addEntry(cp, ctrlProfiles.GetItemCount());
	}
	ctrlProfiles.SetRedraw(TRUE);
}

void ClientsPage::addEntry(const ClientProfile& cp, int pos) {
	TStringList lst;

	lst.push_back(Text::toT(cp.getName()));
	lst.push_back(Text::toT(cp.getComment()));
	ctrlProfiles.insert(pos, lst, 0, (LPARAM)cp.getId());
}

void ClientsPage::write() {
	ClientProfileManager::getInstance()->saveClientProfiles();
	PropPage::write((HWND)*this, items);
}
// iDC++
LRESULT ClientsPage::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		{
			try	{
				ClientProfile cp;
				ClientProfileManager::getInstance()->getClientProfile(ctrlProfiles.GetItemData(cd->nmcd.dwItemSpec), cp);
				if (!cp.getCheatingDescription().empty()) {
					cd->clrText = SETTING(ERROR_COLOR);
				}
				return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
			}
			catch(const Exception&)
			{
			}
			catch(...) 
			{
			}
		}
		return CDRF_NOTIFYSUBITEMDRAW;

	default:
		return CDRF_DODEFAULT;
	}
}
// iDC++
/**
 * @file
 * $Id$
 */
