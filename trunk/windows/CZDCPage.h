#ifndef CZDCPage_H
#define CZDCPage_H

#include <atlcrack.h> 
#include "PropPage.h"
#include "../client/ConnectionManager.h"

class CZDCPage : public CPropertyPage<IDD_CZDCPAGE>, public PropPage
{
public:
	CZDCPage(SettingsManager *s) : PropPage(s) { 
		SetTitle(CSTRING(SETTINGS_CZDC));
	};
	virtual ~CZDCPage() { };

	BEGIN_MSG_MAP_EX(CZDCPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_HANDLER(IDC_WINAMP_HELP, BN_CLICKED, onClickedWinampHelp)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/); 
	LRESULT onClickedWinampHelp(WORD /* wNotifyCode */, WORD wID, HWND /* hWndCtl */, BOOL& /* bHandled */);	

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	virtual void write();

private:
	static Item items[];
	static TextItem texts[];
	static ListItem listItems[];
	CComboBox cClientEmu; 
};

#endif //CZDCPage_H