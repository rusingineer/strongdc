
#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "ClientProfileDlg.h"

LRESULT ClientProfileDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{

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

	ATTACH(IDC_TAG_VERSION, ctrlTagVersion);
	ATTACH(IDC_USE_EXTRA_VERSION, ctrlUseExtraVersion);
	ATTACH(IDC_VERSION_MISMATCH, ctrlCheckMismatch);
	ATTACH(IDC_ADD_LINE, ctrlAddLine);
	ATTACH(IDC_CLIENT_NO_ACTION, ctrlNoAction);
	ATTACH(IDC_CLIENT_RAW_1, ctrlRaw1);
	ATTACH(IDC_CLIENT_RAW_2, ctrlRaw2);
	ATTACH(IDC_CLIENT_RAW_3, ctrlRaw3);
	ATTACH(IDC_CLIENT_RAW_4, ctrlRaw4);
	ATTACH(IDC_CLIENT_RAW_5, ctrlRaw5);

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
	if(tagVersion == 1) { ctrlTagVersion.SetCheck(BST_CHECKED); }
	if(useExtraVersion == 1) { ctrlUseExtraVersion.SetCheck(BST_CHECKED); }
	if(checkMismatch == 1) { ctrlCheckMismatch.SetCheck(BST_CHECKED); }
	switch(rawToSend) {
		case 1: ctrlRaw1.SetCheck(BST_CHECKED); break;
		case 2: ctrlRaw2.SetCheck(BST_CHECKED); break;
		case 3: ctrlRaw3.SetCheck(BST_CHECKED); break;
		case 4: ctrlRaw4.SetCheck(BST_CHECKED); break;
		case 5: ctrlRaw5.SetCheck(BST_CHECKED); break;
		default: ctrlNoAction.SetCheck(BST_CHECKED); break;
	}

	ctrlName.SetFocus();
	
	CenterWindow(GetParent());
	return FALSE;
}

LRESULT ClientProfileDlg::onChange(WORD , WORD , HWND , BOOL& ) {
	updateAddLine();
	return 0;
}
