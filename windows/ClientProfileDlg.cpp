
#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"
#include "WinUtil.h"

#include "ClientProfileDlg.h"
#include "../pme-1.0.4/pme.h"

#define GET_TEXT(id, var) \
	GetDlgItemText(id, buf, 1024); \
	var = buf;

LRESULT ClientProfileDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if(currentProfileId != -1) {
		// FIXME disable this for now to stop potential dupes (ahh. fuck it, leave it enabled :p)
		//::EnableWindow(GetDlgItem(IDC_CLIENT_NAME), false);
		adding = false;
		getProfile();
	} else {
		adding = true;
		::EnableWindow(GetDlgItem(IDC_NEXT), false);
	}

#define ATTACH(id, var) var.Attach(GetDlgItem(id))
	ATTACH(IDC_CLIENT_NAME, ctrlName);
	ATTACH(IDC_CLIENT_VERSION, ctrlVersion);
	ATTACH(IDC_CLIENT_TAG, ctrlTag);
	ATTACH(IDC_CLIENT_EXTENDED_TAG, ctrlExtendedTag);
	ATTACH(IDC_CLIENT_LOCK, ctrlLock);
	ATTACH(IDC_CLIENT_PK, ctrlPk);
	ATTACH(IDC_CLIENT_SUPPORTS, ctrlSupports);
	ATTACH(IDC_CLIENT_TESTSUR_RESPONSE, ctrlTestSUR);
	ATTACH(IDC_CLIENT_USER_CON_COM, ctrlUserConCom);
	ATTACH(IDC_CLIENT_STATUS, ctrlStatus);
	ATTACH(IDC_CLIENT_CHEATING_DESCRIPTION, ctrlCheatingDescription);
	ATTACH(IDC_CLIENT_CONNECTION, ctrlConnection);

	//ATTACH(IDC_TAG_VERSION, ctrlTagVersion);
	ATTACH(IDC_USE_EXTRA_VERSION, ctrlUseExtraVersion);
	ATTACH(IDC_VERSION_MISMATCH, ctrlCheckMismatch);
	ATTACH(IDC_ADD_LINE, ctrlAddLine);
	ATTACH(IDC_COMMENT, ctrlComment);

	ATTACH(IDC_CLIENT_PROFILE_RAW, ctrlRaw);
	ctrlRaw.AddString("No action");
	ctrlRaw.AddString("RAW 1");
	ctrlRaw.AddString("RAW 2");
	ctrlRaw.AddString("RAW 3");
	ctrlRaw.AddString("RAW 4");
	ctrlRaw.AddString("RAW 5");

	ATTACH(IDC_REGEXP_TESTER_COMBO, ctrlRegExpCombo);
	ctrlRegExpCombo.AddString("Version");
	ctrlRegExpCombo.AddString("Tag");
	ctrlRegExpCombo.AddString("Description");
	ctrlRegExpCombo.AddString("Lock");
	ctrlRegExpCombo.AddString("Pk");
	ctrlRegExpCombo.AddString("Supports");
	ctrlRegExpCombo.AddString("TestSUR");
	ctrlRegExpCombo.AddString("Commands");
	ctrlRegExpCombo.AddString("Status");
	ctrlRegExpCombo.AddString("Connection");
	ctrlRegExpCombo.SetCurSel(0);

	updateControls();
	
	CenterWindow(GetParent());
	return FALSE;
}

LRESULT ClientProfileDlg::onChange(WORD , WORD , HWND , BOOL& ) {
	updateAddLine();
	return 0;
}

void ClientProfileDlg::updateAddLine() {
	addLine = Util::emptyString;
	char buf[BUF_LEN];

#define UPDATE \
	GetWindowText(buf, BUF_LEN-1); \
	addLine += buf; \
	addLine += ';';
	
	ctrlName.UPDATE;
	ctrlVersion.UPDATE;
	ctrlTag.UPDATE;
	ctrlExtendedTag.UPDATE;
	ctrlLock.UPDATE;
	ctrlPk.UPDATE;
	ctrlSupports.UPDATE;
	ctrlTestSUR.UPDATE;
	ctrlUserConCom.UPDATE;
	ctrlStatus.UPDATE;
	ctrlConnection.UPDATE;
	ctrlCheatingDescription.GetWindowText(buf, BUF_LEN-1);
	addLine += buf;

	ctrlAddLine.SetWindowText(addLine.c_str());
}

void ClientProfileDlg::getProfile() {
	HubManager::getInstance()->getClientProfile(currentProfileId, currentProfile);

	name = currentProfile.getName();
	version = currentProfile.getVersion();
	tag = currentProfile.getTag();
	extendedTag = currentProfile.getExtendedTag();
	lock = currentProfile.getLock();
	pk = currentProfile.getPk();
	supports = currentProfile.getSupports();
	testSUR = currentProfile.getTestSUR();
	userConCom = currentProfile.getUserConCom();
	status = currentProfile.getStatus();
	cheatingDescription = currentProfile.getCheatingDescription();
	rawToSend = currentProfile.getRawToSend();
	tagVersion = currentProfile.getTagVersion();
	useExtraVersion = currentProfile.getUseExtraVersion();
	checkMismatch = currentProfile.getCheckMismatch();
	connection = currentProfile.getConnection();
	comment = currentProfile.getComment();
}

void ClientProfileDlg::updateVars() {
	char buf[1024];

	GET_TEXT(IDC_CLIENT_NAME, name);
	GET_TEXT(IDC_CLIENT_VERSION, version);
	GET_TEXT(IDC_CLIENT_TAG, tag);
	GET_TEXT(IDC_CLIENT_EXTENDED_TAG, extendedTag);
	GET_TEXT(IDC_CLIENT_LOCK, lock);
	GET_TEXT(IDC_CLIENT_PK, pk);
	GET_TEXT(IDC_CLIENT_SUPPORTS, supports);
	GET_TEXT(IDC_CLIENT_TESTSUR_RESPONSE, testSUR);
	GET_TEXT(IDC_CLIENT_USER_CON_COM, userConCom);
	GET_TEXT(IDC_CLIENT_STATUS, status);
	GET_TEXT(IDC_CLIENT_CHEATING_DESCRIPTION, cheatingDescription);
	GET_TEXT(IDC_CLIENT_CONNECTION, connection);
	GET_TEXT(IDC_COMMENT, comment);
	tagVersion = 0;//(ctrlTagVersion.GetCheck() == BST_CHECKED) ? 1 : 0;
	useExtraVersion = (ctrlUseExtraVersion.GetCheck() == BST_CHECKED) ? 1 : 0;
	checkMismatch = (ctrlCheckMismatch.GetCheck() == BST_CHECKED) ? 1 : 0;
	rawToSend = ctrlRaw.GetCurSel();
}

void ClientProfileDlg::updateControls() {
	ctrlName.SetWindowText(name.c_str());
	ctrlVersion.SetWindowText(version.c_str());
	ctrlTag.SetWindowText(tag.c_str());
	ctrlExtendedTag.SetWindowText(extendedTag.c_str());
	ctrlLock.SetWindowText(lock.c_str());
	ctrlPk.SetWindowText(pk.c_str());
	ctrlSupports.SetWindowText(supports.c_str());
	ctrlTestSUR.SetWindowText(testSUR.c_str());
	ctrlUserConCom.SetWindowText(userConCom.c_str());
	ctrlStatus.SetWindowText(status.c_str());
	ctrlCheatingDescription.SetWindowText(cheatingDescription.c_str());
	ctrlAddLine.SetWindowText(addLine.c_str());
	ctrlConnection.SetWindowText(connection.c_str());
	ctrlComment.SetWindowText(comment.c_str());

	//ctrlTagVersion.SetCheck((tagVersion) ? BST_CHECKED : BST_UNCHECKED);
	ctrlUseExtraVersion.SetCheck((useExtraVersion) ? BST_CHECKED : BST_UNCHECKED);
	ctrlCheckMismatch.SetCheck((checkMismatch) ? BST_CHECKED : BST_UNCHECKED);
	
	ctrlRaw.SetCurSel(rawToSend);

	ctrlName.SetFocus();
}
	
LRESULT ClientProfileDlg::onNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	updateVars();

	currentProfile.setName(name);
	currentProfile.setVersion(version);
	currentProfile.setTag(tag);
	currentProfile.setExtendedTag(extendedTag);
	currentProfile.setLock(lock);
	currentProfile.setPk(pk);
	currentProfile.setSupports(supports);
	currentProfile.setTestSUR(testSUR);
	currentProfile.setUserConCom(userConCom);
	currentProfile.setStatus(status);
	currentProfile.setCheatingDescription(cheatingDescription);
	currentProfile.setRawToSend(rawToSend);
	currentProfile.setTagVersion(tagVersion);
	currentProfile.setUseExtraVersion(useExtraVersion);
	currentProfile.setCheckMismatch(checkMismatch);
	currentProfile.setConnection(connection);
	currentProfile.setComment(comment);
	HubManager::getInstance()->updateClientProfile(currentProfile);

	currentProfileId++;
	getProfile();
	updateControls();

	return 0;
}

LRESULT ClientProfileDlg::onMatch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	char buf[1024];
	string exp, text;
	GET_TEXT(IDC_REGEXP_TESTER_TEXT, text);
	switch(ctrlRegExpCombo.GetCurSel()) {
		case 0: GET_TEXT(IDC_CLIENT_VERSION, exp); break;
		case 1: GET_TEXT(IDC_CLIENT_TAG, exp); break;
		case 2: GET_TEXT(IDC_CLIENT_EXTENDED_TAG, exp); break;
		case 3: GET_TEXT(IDC_CLIENT_LOCK, exp); break;
		case 4: GET_TEXT(IDC_CLIENT_PK, exp); break;
		case 5: GET_TEXT(IDC_CLIENT_SUPPORTS, exp); break;
		case 6: GET_TEXT(IDC_CLIENT_TESTSUR_RESPONSE, exp); break;
		case 7: GET_TEXT(IDC_CLIENT_USER_CON_COM, exp); break;
		case 8: GET_TEXT(IDC_CLIENT_STATUS, exp); break;
		case 9: GET_TEXT(IDC_CLIENT_CONNECTION, exp); break;
		default: dcdebug("We shouldn't be here!\n");
	}
	PME reg(exp);
	if(reg.match(text)) {
		MessageBox("It's a match!", "RegExp Tester", MB_OK);
	} else {
		MessageBox("No match.", "RegExp Tester", MB_OK);
	}
	return 0;
}
