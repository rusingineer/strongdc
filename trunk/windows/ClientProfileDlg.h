
#if !defined(AFX_ClientProfileDlg_H__A7EB85C3_1EEA_4FEC_8450_C090219B8619__INCLUDED_)
#define AFX_ClientProfileDlg_H__A7EB85C3_1EEA_4FEC_8450_C090219B8619__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/Util.h"

class ClientProfileDlg : public CDialogImpl<ClientProfileDlg>
{
	CEdit ctrlName;
	CEdit ctrlVersion;
	CEdit ctrlTag;
	CEdit ctrlExtendedTag;
	CEdit ctrlLock;
	CEdit ctrlPk;
	CEdit ctrlSupports;
	CEdit ctrlTestSUR;
	CEdit ctrlUserConCom;
	CEdit ctrlStatus;
	CEdit ctrlCheatingDescription;
	CEdit ctrlAddLine;
	CButton ctrlNoAction;
	CButton ctrlRaw1;
	CButton ctrlRaw2;
	CButton ctrlRaw3;
	CButton ctrlRaw4;
	CButton ctrlRaw5;
	CButton ctrlTagVersion;
	CButton ctrlUseExtraVersion;
	CButton ctrlCheckMismatch;

public:
	
	string name;
	string version;
	string tag;
	string extendedTag;
	string lock;
	string pk;
	string supports;
	string testSUR;
	string userConCom;
	string status;
	string cheatingDescription;
	string addLine;
	int priority;
	int rawToSend;
	int tagVersion;
	int useExtraVersion;
	int checkMismatch;

	enum { IDD = IDD_CLIENT_PROFILE };

	BEGIN_MSG_MAP(ClientProfileDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)

		COMMAND_ID_HANDLER(IDC_CLIENT_NAME, onChange)
		COMMAND_ID_HANDLER(IDC_CLIENT_VERSION, onChange)
		COMMAND_ID_HANDLER(IDC_CLIENT_TAG, onChange)
		COMMAND_ID_HANDLER(IDC_CLIENT_EXTENDED_TAG, onChange)
		COMMAND_ID_HANDLER(IDC_CLIENT_LOCK, onChange)
		COMMAND_ID_HANDLER(IDC_CLIENT_PK, onChange)
		COMMAND_ID_HANDLER(IDC_CLIENT_SUPPORTS, onChange)
		COMMAND_ID_HANDLER(IDC_CLIENT_TESTSUR_RESPONSE, onChange)
		COMMAND_ID_HANDLER(IDC_CLIENT_USER_CON_COM, onChange)
		COMMAND_ID_HANDLER(IDC_CLIENT_STATUS, onChange)
		COMMAND_ID_HANDLER(IDC_CLIENT_CHEATING_DESCRIPTION, onChange)
	END_MSG_MAP()

	ClientProfileDlg() : priority(0), rawToSend(0), tagVersion(0), useExtraVersion(0), checkMismatch(0) { };

	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		//ctrlName.SetFocus();
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT onChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if(wID == IDOK) {
			char buf[256];

#define GET_TEXT(id, var) \
			GetDlgItemText(id, buf, 256); \
			var = buf;

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
			tagVersion = (ctrlTagVersion.GetCheck() == BST_CHECKED) ? 1 : 0;
			useExtraVersion = (ctrlUseExtraVersion.GetCheck() == BST_CHECKED) ? 1 : 0;
			checkMismatch = (ctrlCheckMismatch.GetCheck() == BST_CHECKED) ? 1 : 0;
			if (ctrlRaw1.GetCheck() == BST_CHECKED) {
				rawToSend = 1;
			} else if (ctrlRaw2.GetCheck() == BST_CHECKED) {
				rawToSend = 2;
			} else if (ctrlRaw3.GetCheck() == BST_CHECKED) {
				rawToSend = 3;
			} else if (ctrlRaw4.GetCheck() == BST_CHECKED) {
				rawToSend = 4;
			} else if (ctrlRaw5.GetCheck() == BST_CHECKED){
				rawToSend = 5;
			} else {
				rawToSend = 0;
			}
		}
		EndDialog(wID);
		return 0;
	}
private:
	enum { BUF_LEN = 1024 };
	void updateAddLine() {
		addLine = Util::emptyString;
		char buf[BUF_LEN];
		ctrlName.GetWindowText(buf, BUF_LEN-1);
		addLine += buf;
		addLine += ";";
		ctrlVersion.GetWindowText(buf, BUF_LEN-1);
		addLine += buf;
		addLine += ";";
		ctrlTag.GetWindowText(buf, BUF_LEN-1);
		addLine += buf;
		addLine += ";";
		ctrlExtendedTag.GetWindowText(buf, BUF_LEN-1);
		addLine += buf;
		addLine += ";";
		ctrlLock.GetWindowText(buf, BUF_LEN-1);
		addLine += buf;
		addLine += ";";
		ctrlPk.GetWindowText(buf, BUF_LEN-1);
		addLine += buf;
		addLine += ";";
		ctrlSupports.GetWindowText(buf, BUF_LEN-1);
		addLine += buf;
		addLine += ";";
		ctrlTestSUR.GetWindowText(buf, BUF_LEN-1);
		addLine += buf;
		addLine += ";";
		ctrlUserConCom.GetWindowText(buf, BUF_LEN-1);
		addLine += buf;
		addLine += ";";
		ctrlStatus.GetWindowText(buf, BUF_LEN-1);
		addLine += buf;
		addLine += ";";
		ctrlCheatingDescription.GetWindowText(buf, BUF_LEN-1);
		addLine += buf;

		ctrlAddLine.SetWindowText(addLine.c_str());
	}
};

#endif
