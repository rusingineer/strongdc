// No license, No copyright... use it if you want ;-)

#ifndef _PROP_PAGE_TEXT_STYLES_H_
#define _PROP_PAGE_TEXT_STYLES_H_

#include "PropPage.h"
#include "../client/ConnectionManager.h"
#include "ChatCtrl.h"
#include "../client/SettingsManager.h"


class PropPageTextStyles: public CPropertyPage<IDD_TEXT_STYLES>, public PropPage
{
public:
	PropPageTextStyles(SettingsManager *s) : PropPage(s) { 
		fg = 0;
		bg = 0;
		title = strdup((STRING(SETTINGS_CZDC) + '\\' + STRING(SETTINGS_TEXT_STYLES)).c_str());
		SetTitle(title);
	};
	virtual ~PropPageTextStyles() { delete[] title;};

	BEGIN_MSG_MAP(PropPageTextStyles)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_HANDLER(IDC_IMPORT, BN_CLICKED, onImport)
		COMMAND_HANDLER(IDC_EXPORT, BN_CLICKED, onExport)
		COMMAND_HANDLER(IDC_BACK_COLOR, BN_CLICKED, onEditBackColor)
		COMMAND_HANDLER(IDC_TEXT_COLOR, BN_CLICKED, onEditForeColor)
		COMMAND_HANDLER(IDC_TEXT_STYLE, BN_CLICKED, onEditTextStyle)
		COMMAND_HANDLER(IDC_DEFAULT_STYLES, BN_CLICKED, onDefaultStyles)
		COMMAND_HANDLER(IDC_BLACK_AND_WHITE, BN_CLICKED, onBlackAndWhite)
		COMMAND_HANDLER(IDC_SELWINCOLOR, BN_CLICKED, onEditBackground)
		COMMAND_HANDLER(IDC_ERROR_COLOR, BN_CLICKED, onEditError)
		COMMAND_HANDLER(IDC_ALTERNATE_COLOR, BN_CLICKED, onEditAlternate)
		COMMAND_ID_HANDLER(IDC_SELTEXT, onClickedText)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onImport(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onExport(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onEditBackColor(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onEditForeColor(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onEditTextStyle(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onDefaultStyles(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onBlackAndWhite(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onClickedText(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onEditBackground(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onEditError(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onEditAlternate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	virtual void write();

private:
	void RefreshPreview();

	class TextStyleSettings: public CHARFORMAT2 {
	public:
		TextStyleSettings() /*: ChatCtrl()*/ { };
		~TextStyleSettings() { };

	void Init( PropPageTextStyles *pParent, SettingsManager *pSM, 
               LPCTSTR sText, LPCTSTR sPreviewText,
               SettingsManager::IntSetting iBack, SettingsManager::IntSetting iFore, 
               SettingsManager::IntSetting iBold, SettingsManager::IntSetting iItalic );
	void LoadSettings();
	void SaveSettings();
	void EditBackColor();
	void EditForeColor();
	void EditTextStyle();

	string m_sText;
	string m_sPreviewText;

	PropPageTextStyles *m_pParent;
	SettingsManager *settings;
	SettingsManager::IntSetting m_iBackColor;
	SettingsManager::IntSetting m_iForeColor;
	SettingsManager::IntSetting m_iBold;
	SettingsManager::IntSetting m_iItalic;
};

protected:
	static Item items[];
	static TextItem texts[];
	enum TextStyles {
		TS_GENERAL, TS_MYNICK, TS_MYMSG, TS_PRIVATE, TS_SYSTEM, TS_SERVER, TS_TIMESTAMP, TS_URL, TS_FAVORITE, TS_OP,
	TS_LAST };

	char* title;
	TextStyleSettings TextStyles[ TS_LAST ];
	CListBox m_lsbList;
	ChatCtrl m_Preview;
	LOGFONT m_Font;
	COLORREF m_BackColor;
	COLORREF m_ForeColor;
	COLORREF fg, bg, err, alt;
};

#endif // _PROP_PAGE_TEXT_STYLES_H_
