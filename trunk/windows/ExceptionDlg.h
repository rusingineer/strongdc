#pragma once

#include "WinUtil.h"

// CExceptionDlg dialog

class CExceptionDlg : public CDialogImpl<CExceptionDlg>
{
public:
	enum { IDD = IDD_EXCEPTION };
	CExceptionDlg() {};   // standard constructor
	virtual ~CExceptionDlg() {};

	BEGIN_MSG_MAP(CExceptionDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_HANDLER(IDC_COPY_EXCEPTION, BN_CLICKED, OnCopyException)
		COMMAND_HANDLER(IDOK, BN_CLICKED, OnContinue)
		COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnTerminate)
	END_MSG_MAP()

	void CopyEditToClipboard(HWND hWnd)
	{
		SendMessage(hWnd, EM_SETSEL, 0, 65535L);
		SendMessage(hWnd, WM_COPY, 0 , 0);
		SendMessage(hWnd, EM_SETSEL, 0, 0);
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		CenterWindow();
		CEdit ctrlEI(GetDlgItem(IDC_EXCEPTION_DETAILS));
		ctrlEI.FmtLines(TRUE);
		ctrlEI.AppendText(WinUtil::exceptioninfo.c_str(), FALSE);
		ctrlEI.Detach();

		return TRUE;
	}

	LRESULT OnCopyException(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		CopyEditToClipboard(GetDlgItem(IDC_EXCEPTION_DETAILS));
		return 0;
	}

	LRESULT OnContinue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		EndDialog(IDOK);
		return 0;
	}

	LRESULT OnTerminate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(::IsDlgButtonChecked((HWND)* this, IDC_CHECK1) == BST_CHECKED) {
			char* arg_list[] = { NULL };
			shutdown();
			execv("strongdc.exe", arg_list);
		}
		EndDialog(IDCANCEL);
		return 0;
	}


private:
	CExceptionDlg(const CExceptionDlg&) { dcassert(0); };

};
