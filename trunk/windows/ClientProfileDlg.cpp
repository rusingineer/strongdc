
#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"
#include "WinUtil.h"

#include "ClientProfileDlg.h"
#include "../pme-1.0.4/pme.h"

#define GET_TEXT(id, var) \
	GetDlgItemText(id, buf, 1024); \
	var = buf;

#define ATTACH(id, var) var.Attach(GetDlgItem(id))

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

	ATTACH(IDC_USE_EXTRA_VERSION, ctrlUseExtraVersion);
	ATTACH(IDC_VERSION_MISMATCH, ctrlCheckMismatch);

	ATTACH(IDC_ADD_LINE, ctrlAddLine);
	ATTACH(IDC_COMMENT, ctrlComment);

	ATTACH(IDC_CLIENT_PROFILE_RAW, ctrlRaw);
	ctrlRaw.AddString(_T("No action"));
	ctrlRaw.AddString(_T("RAW 1"));
	ctrlRaw.AddString(_T("RAW 2"));
	ctrlRaw.AddString(_T("RAW 3"));
	ctrlRaw.AddString(_T("RAW 4"));
	ctrlRaw.AddString(_T("RAW 5"));

	ATTACH(IDC_REGEXP_TESTER_COMBO, ctrlRegExpCombo);
	ctrlRegExpCombo.AddString(_T("Version"));
	ctrlRegExpCombo.AddString(_T("Tag"));
	ctrlRegExpCombo.AddString(_T("Description"));
	ctrlRegExpCombo.AddString(_T("Lock"));
	ctrlRegExpCombo.AddString(_T("Pk"));
	ctrlRegExpCombo.AddString(_T("Supports"));
	ctrlRegExpCombo.AddString(_T("TestSUR"));
	ctrlRegExpCombo.AddString(_T("Commands"));
	ctrlRegExpCombo.AddString(_T("Status"));
	ctrlRegExpCombo.AddString(_T("Connection"));
	ctrlRegExpCombo.SetCurSel(0);

	params = ClientProfileManager::getInstance()->getParams();

	updateControls();
	
	CenterWindow(GetParent());
	return FALSE;
}

LRESULT ClientProfileDlg::onChange(WORD , WORD , HWND , BOOL& ) {
	updateAddLine();
	return 0;
}

LRESULT ClientProfileDlg::onChangeTag(WORD , WORD , HWND , BOOL& ) {
	updateAddLine();
	updateTag();
	return 0;
}

void ClientProfileDlg::updateTag() {
	TCHAR buf[BUF_LEN];
	string exp;
	GET_TEXT(IDC_CLIENT_TAG, Text::toT(exp)); 
	exp = Util::formatRegExp(exp, params);
	SetDlgItemText(IDC_CLIENT_FORMATTED_TAG, Text::toT(exp).c_str());
}
void ClientProfileDlg::updateAddLine() {
	addLine = Util::emptyStringT;
	TCHAR buf[BUF_LEN];

#define UPDATE \
	GetWindowText(buf, BUF_LEN-1); \
	addLine += buf; \
	addLine += _T(';');
	
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
	ClientProfileManager::getInstance()->getClientProfile(currentProfileId, currentProfile);

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
//	tagVersion = currentProfile.getTagVersion();
	useExtraVersion = currentProfile.getUseExtraVersion();
	checkMismatch = currentProfile.getCheckMismatch();
	connection = currentProfile.getConnection();
	comment = currentProfile.getComment();
}

void ClientProfileDlg::updateVars() {
	TCHAR buf[1024];

	GET_TEXT(IDC_CLIENT_NAME, Text::toT(name));
	GET_TEXT(IDC_CLIENT_VERSION, Text::toT(version));
	GET_TEXT(IDC_CLIENT_TAG, Text::toT(tag));
	GET_TEXT(IDC_CLIENT_EXTENDED_TAG, Text::toT(extendedTag));
	GET_TEXT(IDC_CLIENT_LOCK, Text::toT(lock));
	GET_TEXT(IDC_CLIENT_PK, Text::toT(pk));
	GET_TEXT(IDC_CLIENT_SUPPORTS, Text::toT(supports));
	GET_TEXT(IDC_CLIENT_TESTSUR_RESPONSE, Text::toT(testSUR));
	GET_TEXT(IDC_CLIENT_USER_CON_COM, Text::toT(userConCom));
	GET_TEXT(IDC_CLIENT_STATUS, Text::toT(status));
	GET_TEXT(IDC_CLIENT_CHEATING_DESCRIPTION, Text::toT(cheatingDescription));
	GET_TEXT(IDC_CLIENT_CONNECTION, Text::toT(connection));
	GET_TEXT(IDC_COMMENT, Text::toT(comment));
	//tagVersion = 0;//(ctrlTagVersion.GetCheck() == BST_CHECKED) ? 1 : 0;
	useExtraVersion = ctrlUseExtraVersion.GetCheck() == BST_CHECKED;
	checkMismatch = ctrlCheckMismatch.GetCheck() == BST_CHECKED;

	rawToSend = ctrlRaw.GetCurSel();
}

void ClientProfileDlg::updateControls() {
	ctrlName.SetWindowText(Text::toT(name).c_str());
	ctrlVersion.SetWindowText(Text::toT(version).c_str());
	ctrlTag.SetWindowText(Text::toT(tag).c_str());
	ctrlExtendedTag.SetWindowText(Text::toT(extendedTag).c_str());
	ctrlLock.SetWindowText(Text::toT(lock).c_str());
	ctrlPk.SetWindowText(Text::toT(pk).c_str());
	ctrlSupports.SetWindowText(Text::toT(supports).c_str());
	ctrlTestSUR.SetWindowText(Text::toT(testSUR).c_str());
	ctrlUserConCom.SetWindowText(Text::toT(userConCom).c_str());
	ctrlStatus.SetWindowText(Text::toT(status).c_str());
	ctrlCheatingDescription.SetWindowText(Text::toT(cheatingDescription).c_str());
	ctrlAddLine.SetWindowText(addLine.c_str());
	ctrlConnection.SetWindowText(Text::toT(connection).c_str());
	ctrlComment.SetWindowText(Text::toT(comment).c_str());

	//ctrlTagVersion.SetCheck((tagVersion) ? BST_CHECKED : BST_UNCHECKED);
	ctrlUseExtraVersion.SetCheck(useExtraVersion ? BST_CHECKED : BST_UNCHECKED);
	ctrlCheckMismatch.SetCheck(checkMismatch ? BST_CHECKED : BST_UNCHECKED);
	
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
//	currentProfile.setTagVersion(tagVersion);
	currentProfile.setUseExtraVersion(useExtraVersion);
	currentProfile.setCheckMismatch(checkMismatch);
	currentProfile.setConnection(connection);
	currentProfile.setComment(comment);
	ClientProfileManager::getInstance()->updateClientProfile(currentProfile);

	currentProfileId++;
	getProfile();
	updateControls();

	return 0;
}

LRESULT ClientProfileDlg::onMatch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TCHAR buf[1024];
	tstring exp, text;
	GET_TEXT(IDC_REGEXP_TESTER_TEXT, text);
	switch(ctrlRegExpCombo.GetCurSel()) {
		case 0: GET_TEXT(IDC_CLIENT_VERSION, exp); break;
		case 1: 
			{
				tstring version = _T("");
				tstring versionExp = _T("");
				GET_TEXT(IDC_CLIENT_TAG, exp); 
				tstring formattedExp = exp;
				tstring::size_type j = exp.find(_T("%[version]"));
				if(j != string::npos) {
					formattedExp.replace(j, 10, _T(".*"));
					version = Text::toT(getVersion(Text::fromT(exp), Text::fromT(text)));
					GET_TEXT(IDC_CLIENT_VERSION, versionExp)
				}
				switch(matchExp(Text::fromT(formattedExp), Text::fromT(text))) {
					case MATCH:		break;
					case MISMATCH:	MessageBox(_T("No match for tag."), _T("RegExp Tester"), MB_OK); return 0;
					case INVALID:	MessageBox(_T("Invalid tag RegExp."), _T("RegExp Tester"), MB_OK); return 0;
				}
				if(version.empty()) {
					MessageBox(_T("It's a match!"), _T("RegExp Tester"), MB_OK);
					return 0;
				} else {
					switch(matchExp(Text::fromT(versionExp), Text::fromT(version))) {
						case MATCH:		MessageBox(_T("It's a match!"), _T("RegExp Tester"), MB_OK);  return 0;
						case MISMATCH:	MessageBox(_T("No match for version."), _T("RegExp Tester"), MB_OK); return 0;
						case INVALID:	MessageBox(_T("Invalid version RegExp."), _T("RegExp Tester"), MB_OK); return 0;
					}
				}
			}
		case 2: 
			{
				GET_TEXT(IDC_CLIENT_EXTENDED_TAG, exp);
				string::size_type j = exp.find(_T("%[version2]"));
				if(j != string::npos) {
					exp.replace(j, 11, _T(".*"));
				}
				break;
			}
		case 3: GET_TEXT(IDC_CLIENT_LOCK, exp); break;
		case 4: 
			{
				tstring version = _T("");
				tstring versionExp = _T("");
				GET_TEXT(IDC_CLIENT_PK, exp);
				tstring formattedExp = exp;
				tstring::size_type j = exp.find(_T("%[version]"));
				if(j != string::npos) {
					formattedExp.replace(j, 10, _T(".*"));
					version = Text::toT(getVersion(Text::fromT(exp), Text::fromT(text)));
					GET_TEXT(IDC_CLIENT_VERSION, versionExp)
				}
				switch(matchExp(Text::fromT(formattedExp), Text::fromT(text))) {
					case MATCH:		break;
					case MISMATCH:	MessageBox(_T("No match for Pk."), _T("RegExp Tester"), MB_OK); return 0;
					case INVALID:	MessageBox(_T("Invalid Pk RegExp."), _T("RegExp Tester"), MB_OK); return 0;
				}
				if(version.empty()) {
					MessageBox(_T("It's a match!"), _T("RegExp Tester"), MB_OK);
					return 0;
				} else {
					switch(matchExp(Text::fromT(versionExp), Text::fromT(version))) {
						case MATCH:		MessageBox(_T("It's a match!"), _T("RegExp Tester"), MB_OK);  return 0;
						case MISMATCH:	MessageBox(_T("No match for version."), _T("RegExp Tester"), MB_OK); return 0;
						case INVALID:	MessageBox(_T("Invalid version RegExp."), _T("RegExp Tester"), MB_OK); return 0;
					}
				}
			}
		case 5: GET_TEXT(IDC_CLIENT_SUPPORTS, exp); break;
		case 6: GET_TEXT(IDC_CLIENT_TESTSUR_RESPONSE, exp); break;
		case 7: GET_TEXT(IDC_CLIENT_USER_CON_COM, exp); break;
		case 8: GET_TEXT(IDC_CLIENT_STATUS, exp); break;
		case 9: GET_TEXT(IDC_CLIENT_CONNECTION, exp); break;
		default: dcdebug("We shouldn't be here!\n");
	}
	switch(matchExp(Text::fromT(exp), Text::fromT(text))) {
		case MATCH:		MessageBox(_T("It's a match!"), _T("RegExp Tester"), MB_OK); break;
		case MISMATCH:	MessageBox(_T("No match."), _T("RegExp Tester"), MB_OK); break;
		case INVALID:	MessageBox(_T("Invalid RegExp."), _T("RegExp Tester"), MB_OK); break;
	}
	return 0;
}

int ClientProfileDlg::matchExp(const string& aExp, const string& aString) {
	PME reg(aExp);
	if(!reg.IsValid()) { return INVALID; }
	return reg.match(aString) ? MATCH : MISMATCH;
}

string ClientProfileDlg::getVersion(const string& aExp, const string& aTag) {
	string::size_type i = aExp.find("%[version]");
	if (i == string::npos) { 
		i = aExp.find("%[version2]"); 
		return splitVersion(aExp.substr(i + 11), splitVersion(aExp.substr(0, i), aTag, 1), 0);
	}
	return splitVersion(aExp.substr(i + 10), splitVersion(aExp.substr(0, i), aTag, 1), 0);
}

string ClientProfileDlg::splitVersion(const string& aExp, const string& aTag, const int part) {
	PME reg(aExp);
	if(!reg.IsValid()) { return ""; }
	reg.split(aTag, 2);
	return reg[part];
}
