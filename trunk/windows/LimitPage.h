#ifndef LimitPAGE_H
#define LimitPAGE_H

#include <atlcrack.h>
#include "PropPage.h"
#include "../client/ConnectionManager.h"

class LimitPage : public CPropertyPage<IDD_LIMITPAGE>, public PropPage
{
public:
	LimitPage(SettingsManager *s) : PropPage(s) {
		title = strdup((STRING(SETTINGS_CZDC) + '\\' + STRING(SETTINGS_LIMIT)).c_str());
		SetTitle(title);
	};
	virtual ~LimitPage() {
		delete[] title;
	};

	BEGIN_MSG_MAP_EX(LimitPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_TIME_LIMITING, onChangeCont)
		COMMAND_ID_HANDLER(IDC_THROTTLE_ENABLE, onChangeCont)
		COMMAND_ID_HANDLER(IDC_DISCONNECTING_ENABLE, onChangeCont)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/); 
	LRESULT onChangeCont(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	virtual void write();

private:
	static Item items[];
	static TextItem texts[];
	CComboBox timeCtrlBegin, timeCtrlEnd;
	char* title;
	void fixControls();
};

#endif //LimitPAGE_H