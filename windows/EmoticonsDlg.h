#ifndef __EMOTICONS_DLG
#define __EMOTICONS_DLG

class EmoticonsDlg : public CDialogImpl<EmoticonsDlg>
{
public:
	enum { IDD = IDD_EMOTICONS_DLG };


	BEGIN_MSG_MAP(EmoticonsDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		MESSAGE_HANDLER(WM_RBUTTONUP, onClose)
		MESSAGE_HANDLER(WM_LBUTTONUP, onClose)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		COMMAND_CODE_HANDLER(BN_CLICKED, onEmoticonClick)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onEmoticonClick(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		EndDialog(0);
		return 0;
	}

	string result;
	CRect pos;

private:
	CToolTipCtrl ctrlToolTip;

	static WNDPROC m_MFCWndProc;
	static EmoticonsDlg* m_pDialog;
	static LRESULT CALLBACK NewWndProc( HWND, UINT, WPARAM, LPARAM );

};

#endif // __EMOTICONS_DLG