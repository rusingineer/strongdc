/* 
 * Copyright (C) 2001-2004 Jacek Sieka, j_s at telia com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#define COMPILE_MULTIMON_STUBS 1
#include <MultiMon.h>
#include <psapi.h>

#include "WinUtil.h"
#include "PrivateFrame.h"
#include "SearchFrm.h"
#include "LineDlg.h"
#include "MainFrm.h"

#include "../client/Util.h"
#include "../client/StringTokenizer.h"
#include "../client/ShareManager.h"
#include "../client/ClientManager.h"
#include "../client/TimerManager.h"
#include "../client/HubManager.h"
#include "../client/ResourceManager.h"
#include "../client/QueueManager.h"
#include "../client/UploadManager.h"
#include "../client/HashManager.h"
#include "../client/LogManager.h"
#include "../client/version.h"

#include "HubFrame.h"
#include "MagnetDlg.h"
#include "winamp.h"
#include <strmif.h>
#include <control.h>
#include <Windows.h>
#include "../client/cvsversion.h"

WinUtil::ImageMap WinUtil::fileIndexes;
int WinUtil::fileImageCount;
HBRUSH WinUtil::bgBrush = NULL;
COLORREF WinUtil::textColor = 0;
COLORREF WinUtil::bgColor = 0;
HFONT WinUtil::font = NULL;
int WinUtil::fontHeight = 0;
HFONT WinUtil::boldFont = NULL;
HFONT WinUtil::systemFont = NULL;
HFONT WinUtil::monoFont = NULL;
HFONT WinUtil::tinyFont = NULL;
HFONT WinUtil::smallBoldFont = NULL;
CMenu WinUtil::mainMenu;
CMenu WinUtil::grantMenu;
CImageList WinUtil::fileImages;
CImageList WinUtil::userImages;
CImageList WinUtil::flagImages;
int WinUtil::dirIconIndex = 0;
TStringList WinUtil::lastDirs;
HWND WinUtil::mainWnd = NULL;
HWND WinUtil::mdiClient = NULL;
FlatTabCtrl* WinUtil::tabCtrl = NULL;
HHOOK WinUtil::hook = NULL;
tstring WinUtil::tth;
StringPairList WinUtil::initialDirs;
tstring WinUtil::exceptioninfo;
bool WinUtil::isPM = false;
bool WinUtil::isAppActive = false;
bool WinUtil::trayIcon = false;
bool WinUtil::isMinimized = false;
CHARFORMAT2 WinUtil::m_TextStyleTimestamp;
CHARFORMAT2 WinUtil::m_ChatTextGeneral;
CHARFORMAT2 WinUtil::m_TextStyleMyNick;
CHARFORMAT2 WinUtil::m_ChatTextMyOwn;
CHARFORMAT2 WinUtil::m_ChatTextServer;
CHARFORMAT2 WinUtil::m_ChatTextSystem;
CHARFORMAT2 WinUtil::m_TextStyleBold;
CHARFORMAT2 WinUtil::m_TextStyleFavUsers;
CHARFORMAT2 WinUtil::m_TextStyleOPs;
CHARFORMAT2 WinUtil::m_TextStyleURL;
CHARFORMAT2 WinUtil::m_ChatTextPrivate;
CHARFORMAT2 WinUtil::m_ChatTextLog;

WinUtil::tbIDImage WinUtil::ToolbarButtons[] = {
	{ID_FILE_CONNECT, 0, true, ResourceManager::MENU_PUBLIC_HUBS},
	{ID_FILE_RECONNECT, 1, false, ResourceManager::MENU_RECONNECT},
	{IDC_FOLLOW, 2, false, ResourceManager::MENU_FOLLOW_REDIRECT},
	{IDC_FAVORITES, 3, true, ResourceManager::MENU_FAVORITE_HUBS},
	{IDC_FAVUSERS, 4, true, ResourceManager::MENU_FAVORITE_USERS},
	{IDC_RECENTS, 5, true, ResourceManager::MENU_FILE_RECENT_HUBS},
	{IDC_QUEUE, 6, true, ResourceManager::MENU_DOWNLOAD_QUEUE},
	{IDC_FINISHED, 7, true, ResourceManager::FINISHED_DOWNLOADS},
	{IDC_FINISHEDMP3, 8, true, ResourceManager::FINISHED_MP3_DOWNLOADS},
	{IDC_UPLOAD_QUEUE, 9, true, ResourceManager::UPLOAD_QUEUE},
	{IDC_FINISHED_UL, 10, true, ResourceManager::FINISHED_UPLOADS},
	{ID_FILE_SEARCH, 11, false, ResourceManager::MENU_SEARCH},
	{IDC_FILE_ADL_SEARCH, 12, true, ResourceManager::MENU_ADL_SEARCH},
	{IDC_SEARCH_SPY, 13, true, ResourceManager::MENU_SEARCH_SPY},
	{IDC_NET_STATS, 14, true, ResourceManager::NETWORK_STATISTICS},
	{IDC_OPEN_FILE_LIST, 15, false, ResourceManager::MENU_OPEN_FILE_LIST},
	{ID_FILE_SETTINGS, 16, false, ResourceManager::MENU_SETTINGS},
	{IDC_NOTEPAD, 17, true, ResourceManager::MENU_NOTEPAD},
	{IDC_AWAY, 18, true, ResourceManager::AWAY},
	{IDC_SHUTDOWN, 19, true, ResourceManager::SHUTDOWN},
	{IDC_LIMITER, 20, true, ResourceManager::SETCZDC_ENABLE_LIMITING},
	{IDC_UPDATE, 21, false, ResourceManager::UPDATE_CHECK},
	{IDC_DISABLE_SOUNDS, 22, true, ResourceManager::DISABLE_SOUNDS},
	{0, 0, false, ResourceManager::MENU_NOTEPAD}
};

static const char* CountryNames[] = { "ANDORRA", "UNITED ARAB EMIRATES", "AFGHANISTAN", "ANTIGUA AND BARBUDA", 
"ANGUILLA", "ALBANIA", "ARMENIA", "NETHERLANDS ANTILLES", "ANGOLA", "ANTARCTICA", "ARGENTINA", "AMERICAN SAMOA", 
"AUSTRIA", "AUSTRALIA", "ARUBA", "ALAND", "AZERBAIJAN", "BOSNIA AND HERZEGOVINA", "BARBADOS", "BANGLADESH", 
"BELGIUM", "BURKINA FASO", "BULGARIA", "BAHRAIN", "BURUNDI", "BENIN", "BERMUDA", "BRUNEI DARUSSALAM", "BOLIVIA", 
"BRAZIL", "BAHAMAS", "BHUTAN", "BOUVET ISLAND", "BOTSWANA", "BELARUS", "BELIZE", "CANADA", "COCOS ISLANDS", 
"THE DEMOCRATIC REPUBLIC OF THE CONGO", "CENTRAL AFRICAN REPUBLIC", "CONGO", "COTE D'IVOIRE", "COOK ISLANDS", 
"CHILE", "CAMEROON", "CHINA", "COLOMBIA", "COSTA RICA", "SERBIA AND MONTENEGRO", "CUBA", "CAPE VERDE", 
"CHRISTMAS ISLAND", "CYPRUS", "CZECH REPUBLIC", "GERMANY", "DJIBOUTI", "DENMARK", "DOMINICA", "DOMINICAN REPUBLIC", 
"ALGERIA", "ECUADOR", "ESTONIA", "EGYPT", "WESTERN SAHARA", "ERITREA", "SPAIN", "ETHIOPIA", "FINLAND", "FIJI", 
"FALKLAND ISLANDS", "MICRONESIA", "FAROE ISLANDS", "FRANCE", "GABON", "UNITED KINGDOM", "GRENADA", "GEORGIA", 
"FRENCH GUIANA", "GHANA", "GIBRALTAR", "GREENLAND", "GAMBIA", "GUINEA", "GUADELOUPE", "EQUATORIAL GUINEA", 
"GREECE", "SOUTH GEORGIA AND THE SOUTH SANDWICH ISLANDS", "GUATEMALA", "GUAM", "GUINEA-BISSAU", "GUYANA", 
"HONG KONG", "HEARD ISLAND AND MCDONALD ISLANDS", "HONDURAS", "CROATIA", "HAITI", "HUNGARY", "SWITZERLAND", 
"INDONESIA", "IRELAND", "ISRAEL", "INDIA", "BRITISH INDIAN OCEAN TERRITORY", "IRAQ", "IRAN", "ICELAND", 
"ITALY", "JAMAICA", "JORDAN", "JAPAN", "KENYA", "KYRGYZSTAN", "CAMBODIA", "KIRIBATI", "COMOROS", 
"SAINT KITTS AND NEVIS", "DEMOCRATIC PEOPLE'S REPUBLIC OF KOREA", "SOUTH KOREA", "KUWAIT", "CAYMAN ISLANDS", 
"KAZAKHSTAN", "LAO PEOPLE'S DEMOCRATIC REPUBLIC", "LEBANON", "SAINT LUCIA", "LIECHTENSTEIN", "SRI LANKA", 
"LIBERIA", "LESOTHO", "LITHUANIA", "LUXEMBOURG", "LATVIA", "LIBYAN ARAB JAMAHIRIYA", "MOROCCO", "MONACO", 
"MOLDOVA", "MADAGASCAR", "MARSHALL ISLANDS", "MACEDONIA", "MALI", "MYANMAR", "MONGOLIA", "MACAO", 
"NORTHERN MARIANA ISLANDS", "MARTINIQUE", "MAURITANIA", "MONTSERRAT", "MALTA", "MAURITIUS", "MALDIVES", 
"MALAWI", "MEXICO", "MALAYSIA", "MOZAMBIQUE", "NAMIBIA", "NEW CALEDONIA", "NIGER", "NORFOLK ISLAND", 
"NIGERIA", "NICARAGUA", "NETHERLANDS", "NORWAY", "NEPAL", "NAURU", "NIUE", "NEW ZEALAND", "OMAN", "PANAMA", 
"PERU", "FRENCH POLYNESIA", "PAPUA NEW GUINEA", "PHILIPPINES", "PAKISTAN", "POLAND", "SAINT PIERRE AND MIQUELON", 
"PITCAIRN", "PUERTO RICO", "PALESTINIAN TERRITORY", "PORTUGAL", "PALAU", "PARAGUAY", "QATAR", "REUNION", 
"ROMANIA", "RUSSIAN FEDERATION", "RWANDA", "SAUDI ARABIA", "SOLOMON ISLANDS", "SEYCHELLES", "SUDAN", 
"SWEDEN", "SINGAPORE", "SAINT HELENA", "SLOVENIA", "SVALBARD AND JAN MAYEN", "SLOVAKIA", "SIERRA LEONE", 
"SAN MARINO", "SENEGAL", "SOMALIA", "SURINAME", "SAO TOME AND PRINCIPE", "EL SALVADOR", "SYRIAN ARAB REPUBLIC", 
"SWAZILAND", "TURKS AND CAICOS ISLANDS", "CHAD", "FRENCH SOUTHERN TERRITORIES", "TOGO", "THAILAND", "TAJIKISTAN", 
"TOKELAU", "TIMOR-LESTE", "TURKMENISTAN", "TUNISIA", "TONGA", "TURKEY", "TRINIDAD AND TOBAGO", "TUVALU", "TAIWAN", 
"TANZANIA", "UKRAINE", "UGANDA", "UNITED STATES MINOR OUTLYING ISLANDS", "UNITED STATES", "URUGUAY", "UZBEKISTAN", 
"VATICAN", "SAINT VINCENT AND THE GRENADINES", "VENEZUELA", "BRITISH VIRGIN ISLANDS", "U.S. VIRGIN ISLANDS", 
"VIET NAM", "VANUATU", "WALLIS AND FUTUNA", "SAMOA", "YEMEN", "MAYOTTE", "YUGOSLAVIA", "SOUTH AFRICA", "ZAMBIA", 
"ZIMBABWE", "EUROPEAN UNION" };

static const char* CountryCodes[] = { "AD", "AE", "AF", "AG", "AI", "AL", "AM", "AN", "AO", "AQ", "AR", "AS", 
"AT", "AU", "AW", "AX", "AZ", "BA", "BB", "BD", "BE", "BF", "BG", "BH", "BI", "BJ", "BM", "BN", "BO", "BR", 
"BS", "BT", "BV", "BW", "BY", "BZ", "CA", "CC", "CD", "CF", "CG", "CI", "CK", "CL", "CM", "CN", "CO", "CR", 
"CS", "CU", "CV", "CX", "CY", "CZ", "DE", "DJ", "DK", "DM", "DO", "DZ", "EC", "EE", "EG", "EH", "ER", "ES", 
"ET", "FI", "FJ", "FK", "FM", "FO", "FR", "GA", "GB", "GD", "GE", "GF", "GH", "GI", "GL", "GM", "GN", "GP", 
"GQ", "GR", "GS", "GT", "GU", "GW", "GY", "HK", "HM", "HN", "HR", "HT", "HU", "CH", "ID", "IE", "IL", "IN", 
"IO", "IQ", "IR", "IS", "IT", "JM", "JO", "JP", "KE", "KG", "KH", "KI", "KM", "KN", "KP", "KR", "KW", "KY", 
"KZ", "LA", "LB", "LC", "LI", "LK", "LR", "LS", "LT", "LU", "LV", "LY", "MA", "MC", "MD", "MG", "MH", "MK", 
"ML", "MM", "MN", "MO", "MP", "MQ", "MR", "MS", "MT", "MU", "MV", "MW", "MX", "MY", "MZ", "NA", "NC", "NE", 
"NF", "NG", "NI", "NL", "NO", "NP", "NR", "NU", "NZ", "OM", "PA", "PE", "PF", "PG", "PH", "PK", "PL", "PM", 
"PN", "PR", "PS", "PT", "PW", "PY", "QA", "RE", "RO", "RU", "RW", "SA", "SB", "SC", "SD", "SE", "SG", "SH", 
"SI", "SJ", "SK", "SL", "SM", "SN", "SO", "SR", "ST", "SV", "SY", "SZ", "TC", "TD", "TF", "TG", "TH", "TJ", 
"TK", "TL", "TM", "TN", "TO", "TR", "TT", "TV", "TW", "TZ", "UA", "UG", "UM", "US", "UY", "UZ", "VA", "VC", 
"VE", "VG", "VI", "VN", "VU", "WF", "WS", "YE", "YT", "YU", "ZA", "ZM", "ZW", "EU" };
	
HLSCOLOR RGB2HLS (COLORREF rgb) {
	unsigned char minval = min(GetRValue(rgb), min(GetGValue(rgb), GetBValue(rgb)));
	unsigned char maxval = max(GetRValue(rgb), max(GetGValue(rgb), GetBValue(rgb)));
	float mdiff  = float(maxval) - float(minval);
	float msum   = float(maxval) + float(minval);

	float luminance = msum / 510.0f;
	float saturation = 0.0f;
	float hue = 0.0f; 

	if ( maxval != minval ) { 
		float rnorm = (maxval - GetRValue(rgb)  ) / mdiff;      
		float gnorm = (maxval - GetGValue(rgb)) / mdiff;
		float bnorm = (maxval - GetBValue(rgb) ) / mdiff;   

		saturation = (luminance <= 0.5f) ? (mdiff / msum) : (mdiff / (510.0f - msum));

		if (GetRValue(rgb) == maxval) hue = 60.0f * (6.0f + bnorm - gnorm);
		if (GetGValue(rgb) == maxval) hue = 60.0f * (2.0f + rnorm - bnorm);
		if (GetBValue(rgb) == maxval) hue = 60.0f * (4.0f + gnorm - rnorm);
		if (hue > 360.0f) hue = hue - 360.0f;
	}
	return HLS ((hue*255)/360, luminance*255, saturation*255);
}

static BYTE _ToRGB (float rm1, float rm2, float rh) {
	if      (rh > 360.0f) rh -= 360.0f;
	else if (rh <   0.0f) rh += 360.0f;

	if      (rh <  60.0f) rm1 = rm1 + (rm2 - rm1) * rh / 60.0f;   
	else if (rh < 180.0f) rm1 = rm2;
	else if (rh < 240.0f) rm1 = rm1 + (rm2 - rm1) * (240.0f - rh) / 60.0f;      

	return (BYTE)(rm1 * 255);
}

COLORREF HLS2RGB (HLSCOLOR hls) {
	float hue        = ((int)HLS_H(hls)*360)/255.0f;
	float luminance  = HLS_L(hls)/255.0f;
	float saturation = HLS_S(hls)/255.0f;

	if ( saturation == 0.0f ) {
		return RGB (HLS_L(hls), HLS_L(hls), HLS_L(hls));
	}
	float rm1, rm2;

	if ( luminance <= 0.5f ) rm2 = luminance + luminance * saturation;  
	else                     rm2 = luminance + saturation - luminance * saturation;
	rm1 = 2.0f * luminance - rm2;   
	BYTE red   = _ToRGB (rm1, rm2, hue + 120.0f);   
	BYTE green = _ToRGB (rm1, rm2, hue);
	BYTE blue  = _ToRGB (rm1, rm2, hue - 120.0f);

	return RGB (red, green, blue);
}

COLORREF HLS_TRANSFORM (COLORREF rgb, int percent_L, int percent_S) {
	HLSCOLOR hls = RGB2HLS (rgb);
	BYTE h = HLS_H(hls);
	BYTE l = HLS_L(hls);
	BYTE s = HLS_S(hls);

	if ( percent_L > 0 ) {
		l = BYTE(l + ((255 - l) * percent_L) / 100);
	} else if ( percent_L < 0 )	{
		l = BYTE((l * (100+percent_L)) / 100);
	}
	if ( percent_S > 0 ) {
		s = BYTE(s + ((255 - s) * percent_S) / 100);
	} else if ( percent_S < 0 ) {
		s = BYTE((s * (100+percent_S)) / 100);
	}
	return HLS2RGB (HLS(h, l, s));
}

void UserInfoBase::matchQueue() {
	try {
		QueueManager::getInstance()->addList(user, QueueItem::FLAG_MATCH_QUEUE);
	} catch(const Exception&) {
	}
}

void UserInfoBase::getUserResponses() {
	try {
		QueueManager::getInstance()->addTestSUR(user, false);
	} catch(const Exception&) {
	}
}

void UserInfoBase::doReport() {
	user->addLine("*** Info on " + user->getNick() + " ***" + "\r\n" + user->getReport() + "\r\n");
}

void UserInfoBase::getList() {
	try {
		QueueManager::getInstance()->addList(user, QueueItem::FLAG_CLIENT_VIEW);
	} catch(const Exception&) {
	}
}
void UserInfoBase::checkList() {
	try {
		QueueManager::getInstance()->addList(user, QueueItem::FLAG_CHECK_FILE_LIST);
	} catch(const Exception&) {
	}
}
void UserInfoBase::addFav() {
	HubManager::getInstance()->addFavoriteUser(user);
}
void UserInfoBase::pm() {
	PrivateFrame::openWindow(user);
}
void UserInfoBase::grant() {
	UploadManager::getInstance()->reserveSlot(user);
}
void UserInfoBase::removeAll() {
	QueueManager::getInstance()->removeSources(user, QueueItem::Source::FLAG_REMOVED);
}
void UserInfoBase::grantSlotHour() {
	UploadManager::getInstance()->reserveSlotHour(user);
}
void UserInfoBase::grantSlotDay() {
	UploadManager::getInstance()->reserveSlotDay(user);
}
void UserInfoBase::grantSlotWeek() {
	UploadManager::getInstance()->reserveSlotWeek(user);
}
void UserInfoBase::ungrantSlot() {
	UploadManager::getInstance()->unreserveSlot(user);
}

bool WinUtil::getVersionInfo(OSVERSIONINFOEX& ver) {
	memset(&ver, 0, sizeof(OSVERSIONINFOEX));
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if(!GetVersionEx((OSVERSIONINFO*)&ver)) {
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if(!GetVersionEx((OSVERSIONINFO*)&ver)) {
			return false;
		}
	}
	return true;
}

static LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
	if(code == HC_ACTION) {
		if(wParam == VK_CONTROL && LOWORD(lParam) == 1) {
			if(lParam & 0x80000000) {
				WinUtil::tabCtrl->endSwitch();
			} else {
				WinUtil::tabCtrl->startSwitch();
			}
		}
	}
	return CallNextHookEx(WinUtil::hook, code, wParam, lParam);
}

void createImageList1(CImageList &imglst, string file) {
	HBITMAP hBitmap = (HBITMAP)::LoadImage(
		NULL, Text::toT(file).c_str(), IMAGE_BITMAP,
		0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
	
	if( !hBitmap )
		return;
    CBitmap b;
	b.Attach(hBitmap);
	
	imglst.Create(16, 16, ILC_MASK | ILC_COLOR32, 0, 0);
	imglst.Add(b, RGB(255,0,255)); 
}
void WinUtil::reLoadImages(){
	userImages.Destroy();
	if(SETTING(USERLIST_IMAGE) == "")
		userImages.CreateFromImage(IDB_USERS, 16, 9, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	else 
		createImageList1(userImages, SETTING(USERLIST_IMAGE));
}// User Icon End

void WinUtil::init(HWND hWnd) {
	mainWnd = hWnd;

	mainMenu.CreateMenu();

	CMenuHandle file;
	file.CreatePopupMenu();

	file.AppendMenu(MF_STRING, IDC_OPEN_FILE_LIST, CTSTRING(MENU_OPEN_FILE_LIST));
	file.AppendMenu(MF_STRING, IDC_OPEN_MY_LIST, CTSTRING(MENU_OPEN_OWN_LIST));
	file.AppendMenu(MF_STRING, IDC_REFRESH_FILE_LIST, CTSTRING(MENU_REFRESH_FILE_LIST));
	file.AppendMenu(MF_STRING, IDC_OPEN_DOWNLOADS, CTSTRING(MENU_OPEN_DOWNLOADS_DIR));
	file.AppendMenu(MF_SEPARATOR);
	file.AppendMenu(MF_STRING, ID_FILE_QUICK_CONNECT, CTSTRING(MENU_QUICK_CONNECT));
	file.AppendMenu(MF_STRING, IDC_FOLLOW, CTSTRING(MENU_FOLLOW_REDIRECT));
	file.AppendMenu(MF_STRING, ID_FILE_RECONNECT, CTSTRING(MENU_RECONNECT));
	file.AppendMenu(MF_SEPARATOR);
	file.AppendMenu(MF_STRING, ID_FILE_SETTINGS, CTSTRING(MENU_SETTINGS));
	file.AppendMenu(MF_STRING, ID_GET_TTH, CTSTRING(MENU_TTH));
	file.AppendMenu(MF_STRING, IDC_UPDATE, CTSTRING(UPDATE_CHECK));
	file.AppendMenu(MF_SEPARATOR);
	file.AppendMenu(MF_STRING, ID_APP_EXIT, CTSTRING(MENU_EXIT));

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)file, CTSTRING(MENU_FILE));

	CMenuHandle view;
	view.CreatePopupMenu();

	view.AppendMenu(MF_STRING, ID_FILE_CONNECT, CTSTRING(MENU_PUBLIC_HUBS));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, IDC_RECENTS, CTSTRING(MENU_FILE_RECENT_HUBS));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, IDC_FAVORITES, CTSTRING(MENU_FAVORITE_HUBS));
	view.AppendMenu(MF_STRING, IDC_FAVUSERS, CTSTRING(MENU_FAVORITE_USERS));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, ID_FILE_SEARCH, CTSTRING(MENU_SEARCH));
	view.AppendMenu(MF_STRING, IDC_FILE_ADL_SEARCH, CTSTRING(MENU_ADL_SEARCH));
	view.AppendMenu(MF_STRING, IDC_SEARCH_SPY, CTSTRING(MENU_SEARCH_SPY));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, IDC_CDMDEBUG_WINDOW, CTSTRING(MENU_CDMDEBUG_MESSAGES));
	view.AppendMenu(MF_STRING, IDC_NOTEPAD, CTSTRING(MENU_NOTEPAD));
	view.AppendMenu(MF_STRING, IDC_HASH_PROGRESS, CTSTRING(MENU_HASH_PROGRESS));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, ID_VIEW_TOOLBAR, CTSTRING(MENU_TOOLBAR));
	view.AppendMenu(MF_STRING, ID_VIEW_STATUS_BAR, CTSTRING(MENU_STATUS_BAR));
	view.AppendMenu(MF_STRING, ID_VIEW_TRANSFER_VIEW, CTSTRING(MENU_TRANSFER_VIEW));

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)view, CTSTRING(MENU_VIEW));

	CMenuHandle transfers;
	transfers.CreatePopupMenu();

	transfers.AppendMenu(MF_STRING, IDC_QUEUE, CTSTRING(MENU_DOWNLOAD_QUEUE));
	transfers.AppendMenu(MF_STRING, IDC_FINISHED, CTSTRING(FINISHED_DOWNLOADS));
	transfers.AppendMenu(MF_STRING, IDC_FINISHEDMP3, CTSTRING(FINISHED_MP3_DOWNLOADS));
	transfers.AppendMenu(MF_SEPARATOR);
	transfers.AppendMenu(MF_STRING, IDC_UPLOAD_QUEUE, CTSTRING(UPLOAD_QUEUE));
	transfers.AppendMenu(MF_STRING, IDC_FINISHED_UL, CTSTRING(FINISHED_UPLOADS));
	transfers.AppendMenu(MF_SEPARATOR);
	transfers.AppendMenu(MF_STRING, IDC_NET_STATS, CTSTRING(MENU_NETWORK_STATISTICS));

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)transfers, CTSTRING(MENU_TRANSFERS));

	CMenuHandle window;
	window.CreatePopupMenu();

	window.AppendMenu(MF_STRING, ID_WINDOW_CASCADE, CTSTRING(MENU_CASCADE));
	window.AppendMenu(MF_STRING, ID_WINDOW_TILE_HORZ, CTSTRING(MENU_HORIZONTAL_TILE));
	window.AppendMenu(MF_STRING, ID_WINDOW_TILE_VERT, CTSTRING(MENU_VERTICAL_TILE));
	window.AppendMenu(MF_STRING, ID_WINDOW_ARRANGE, CTSTRING(MENU_ARRANGE));
	window.AppendMenu(MF_STRING, ID_WINDOW_MINIMIZE_ALL, CTSTRING(MENU_MINIMIZE_ALL));
	window.AppendMenu(MF_STRING, ID_WINDOW_RESTORE_ALL, CTSTRING(MENU_RESTORE_ALL));
	window.AppendMenu(MF_SEPARATOR);
	window.AppendMenu(MF_STRING, IDC_CLOSE_DISCONNECTED, CTSTRING(MENU_CLOSE_DISCONNECTED));

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)window, CTSTRING(MENU_WINDOW));

	CMenuHandle help;
	help.CreatePopupMenu();

	help.AppendMenu(MF_STRING, ID_APP_ABOUT, CTSTRING(MENU_ABOUT));
	help.AppendMenu(MF_SEPARATOR);
	help.AppendMenu(MF_STRING, IDC_HELP_HOMEPAGE, CTSTRING(MENU_HOMEPAGE));
	help.AppendMenu(MF_STRING, IDC_HELP_DISCUSS, CTSTRING(MENU_DISCUSS));

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)help, CTSTRING(MENU_HELP));

	if(BOOLSETTING(USE_SYSTEM_ICONS)) {
		SHFILEINFO fi;
		memset(&fi, 0, sizeof(SHFILEINFO));
		fileImages.Create(16, 16, ILC_COLOR32 | ILC_MASK, 16, 16);
		::SHGetFileInfo(_T(""), FILE_ATTRIBUTE_DIRECTORY, &fi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		fileImages.AddIcon(fi.hIcon);
		::DestroyIcon(fi.hIcon);
		dirIconIndex = fileImageCount++;	
	} else {
		fileImages.CreateFromImage(IDB_FOLDERS, 16, 3, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
		dirIconIndex = 0;
	}

	flagImages.CreateFromImage(IDB_FLAGS, 25, 8, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);

	if(SETTING(USERLIST_IMAGE) == "")
		userImages.CreateFromImage(IDB_USERS, 16, 9, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	else 
		createImageList1(userImages, SETTING(USERLIST_IMAGE));
	LOGFONT lf, lf2;
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
	SettingsManager::getInstance()->setDefault(SettingsManager::TEXT_FONT, Text::fromT(encodeFont(lf)));
	decodeFont(Text::toT(SETTING(TEXT_FONT)), lf);
	::GetObject((HFONT)GetStockObject(ANSI_FIXED_FONT), sizeof(lf2), &lf2);
	
	lf2.lfHeight = lf.lfHeight;
	lf2.lfWeight = lf.lfWeight;
	lf2.lfItalic = lf.lfItalic;

	bgBrush = CreateSolidBrush(SETTING(BACKGROUND_COLOR));
	textColor = SETTING(TEXT_COLOR);
	bgColor = SETTING(BACKGROUND_COLOR);
	font = ::CreateFontIndirect(&lf);
	fontHeight = WinUtil::getTextHeight(mainWnd, font);
	lf.lfWeight = FW_BOLD;
	boldFont = ::CreateFontIndirect(&lf);
	LONG _lfh = lf.lfHeight;
	lf.lfHeight *= 5;
	lf.lfHeight /= 6;
	smallBoldFont = ::CreateFontIndirect(&lf);
	lf.lfHeight = _lfh;
	lf.lfHeight *= 3;
	lf.lfHeight /= 4;
	tinyFont = ::CreateFontIndirect(&lf);
	systemFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	monoFont = (HFONT)::GetStockObject(BOOLSETTING(USE_OEM_MONOFONT)?OEM_FIXED_FONT:ANSI_FIXED_FONT);

	if(BOOLSETTING(URL_HANDLER)) {
		registerDchubHandler();
		registerADChubHandler();
	}
	registerMagnetHandler();

	hook = SetWindowsHookEx(WH_KEYBOARD, &KeyboardProc, NULL, GetCurrentThreadId());
	
	grantMenu.CreatePopupMenu();
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CTSTRING(GRANT_EXTRA_SLOT));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_HOUR, CTSTRING(GRANT_EXTRA_SLOT_HOUR));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_DAY, CTSTRING(GRANT_EXTRA_SLOT_DAY));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_WEEK, CTSTRING(GRANT_EXTRA_SLOT_WEEK));
	grantMenu.AppendMenu(MF_SEPARATOR);
	grantMenu.AppendMenu(MF_STRING, IDC_UNGRANTSLOT, CTSTRING(REMOVE_EXTRA_SLOT));

	initColors();
}

void WinUtil::initColors() {
	bgBrush = CreateSolidBrush(SETTING(BACKGROUND_COLOR));
	textColor = SETTING(TEXT_COLOR);
	bgColor = SETTING(BACKGROUND_COLOR);

	CHARFORMAT2 cf;
	memset(&cf, 0, sizeof(CHARFORMAT2));
	cf.cbSize = sizeof(cf);
	cf.dwReserved = 0;
	cf.dwMask = CFM_BACKCOLOR | CFM_COLOR | CFM_BOLD | CFM_ITALIC;
	cf.dwEffects = 0;
	cf.crBackColor = SETTING(BACKGROUND_COLOR);
	cf.crTextColor = SETTING(TEXT_COLOR);

	memset(&m_TextStyleBold, 0, sizeof(CHARFORMAT2));
	m_TextStyleBold.cbSize = sizeof(m_TextStyleBold);
	m_TextStyleBold.dwMask = CFM_BOLD;
	m_TextStyleBold.dwReserved = 0;
	m_TextStyleBold.crBackColor = 0;
	m_TextStyleBold.crTextColor = 0;
	m_TextStyleBold.dwEffects = CFE_BOLD;

	m_TextStyleTimestamp = cf;
	m_TextStyleTimestamp.crBackColor = SETTING(TEXT_TIMESTAMP_BACK_COLOR);
	m_TextStyleTimestamp.crTextColor = SETTING(TEXT_TIMESTAMP_FORE_COLOR);
	if(SETTING(TEXT_TIMESTAMP_BOLD))
		m_TextStyleTimestamp.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_TIMESTAMP_ITALIC))
		m_TextStyleTimestamp.dwEffects |= CFE_ITALIC;

	m_ChatTextGeneral = cf;
	m_ChatTextGeneral.crBackColor = SETTING(TEXT_GENERAL_BACK_COLOR);
	m_ChatTextGeneral.crTextColor = SETTING(TEXT_GENERAL_FORE_COLOR);
	if(SETTING(TEXT_GENERAL_BOLD))
		m_ChatTextGeneral.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_GENERAL_ITALIC))
		m_ChatTextGeneral.dwEffects |= CFE_ITALIC;

	m_TextStyleMyNick = cf;
	m_TextStyleMyNick.crBackColor = SETTING(TEXT_MYNICK_BACK_COLOR);
	m_TextStyleMyNick.crTextColor = SETTING(TEXT_MYNICK_FORE_COLOR);
	if(SETTING(TEXT_MYNICK_BOLD))
		m_TextStyleMyNick.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_MYNICK_ITALIC))
		m_TextStyleMyNick.dwEffects |= CFE_ITALIC;

	m_ChatTextMyOwn = cf;
	m_ChatTextMyOwn.crBackColor = SETTING(TEXT_MYOWN_BACK_COLOR);
	m_ChatTextMyOwn.crTextColor = SETTING(TEXT_MYOWN_FORE_COLOR);
	if(SETTING(TEXT_MYOWN_BOLD))
		m_ChatTextMyOwn.dwEffects       |= CFE_BOLD;
	if(SETTING(TEXT_MYOWN_ITALIC))
		m_ChatTextMyOwn.dwEffects       |= CFE_ITALIC;

	m_ChatTextPrivate = cf;
	m_ChatTextPrivate.crBackColor = SETTING(TEXT_PRIVATE_BACK_COLOR);
	m_ChatTextPrivate.crTextColor = SETTING(TEXT_PRIVATE_FORE_COLOR);
	if(SETTING(TEXT_PRIVATE_BOLD))
		m_ChatTextPrivate.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_PRIVATE_ITALIC))
		m_ChatTextPrivate.dwEffects |= CFE_ITALIC;

	m_ChatTextSystem = cf;
	m_ChatTextSystem.crBackColor = SETTING(TEXT_SYSTEM_BACK_COLOR);
	m_ChatTextSystem.crTextColor = SETTING(TEXT_SYSTEM_FORE_COLOR);
	if(SETTING(TEXT_SYSTEM_BOLD))
		m_ChatTextSystem.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_SYSTEM_ITALIC))
		m_ChatTextSystem.dwEffects |= CFE_ITALIC;

	m_ChatTextServer = cf;
	m_ChatTextServer.crBackColor = SETTING(TEXT_SERVER_BACK_COLOR);
	m_ChatTextServer.crTextColor = SETTING(TEXT_SERVER_FORE_COLOR);
	if(SETTING(TEXT_SERVER_BOLD))
		m_ChatTextServer.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_SERVER_ITALIC))
		m_ChatTextServer.dwEffects |= CFE_ITALIC;

	m_ChatTextLog = m_ChatTextGeneral;
	m_ChatTextLog.crTextColor = CZDCLib::blendColors(SETTING(TEXT_GENERAL_BACK_COLOR), SETTING(TEXT_GENERAL_FORE_COLOR), 0.4);

	m_TextStyleFavUsers = cf;
	m_TextStyleFavUsers.crBackColor = SETTING(TEXT_FAV_BACK_COLOR);
	m_TextStyleFavUsers.crTextColor = SETTING(TEXT_FAV_FORE_COLOR);
	if(SETTING(TEXT_FAV_BOLD))
		m_TextStyleFavUsers.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_FAV_ITALIC))
		m_TextStyleFavUsers.dwEffects |= CFE_ITALIC;

	m_TextStyleOPs = cf;
	m_TextStyleOPs.crBackColor = SETTING(TEXT_OP_BACK_COLOR);
	m_TextStyleOPs.crTextColor = SETTING(TEXT_OP_FORE_COLOR);
	if(SETTING(TEXT_OP_BOLD))
		m_TextStyleOPs.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_OP_ITALIC))
		m_TextStyleOPs.dwEffects |= CFE_ITALIC;

	m_TextStyleURL = cf;
	m_TextStyleURL.dwMask = CFM_COLOR | CFM_BOLD | CFM_ITALIC | CFM_BACKCOLOR | CFM_LINK | CFM_UNDERLINE;
	m_TextStyleURL.crBackColor = SETTING(TEXT_URL_BACK_COLOR);
	m_TextStyleURL.crTextColor = SETTING(TEXT_URL_FORE_COLOR);
	m_TextStyleURL.dwEffects = CFE_LINK | CFE_UNDERLINE;
	if(SETTING(TEXT_URL_BOLD))
		m_TextStyleURL.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_URL_ITALIC))
		m_TextStyleURL.dwEffects |= CFE_ITALIC;
}

void WinUtil::uninit() {
	fileImages.Destroy();
	userImages.Destroy();
	flagImages.Destroy();
	::DeleteObject(font);
	::DeleteObject(boldFont);
	::DeleteObject(smallBoldFont);
	::DeleteObject(tinyFont);
	::DeleteObject(bgBrush);
	::DeleteObject(monoFont);

	mainMenu.DestroyMenu();
	grantMenu.DestroyMenu();

	UnhookWindowsHookEx(hook);	

}

void WinUtil::decodeFont(const tstring& setting, LOGFONT &dest) {
	StringTokenizer<tstring> st(setting, _T(','));
	TStringList &sl = st.getTokens();
	
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(dest), &dest);
	tstring face;
	if(sl.size() == 4)
	{
		face = sl[0];
		dest.lfHeight = Util::toInt(Text::fromT(sl[1]));
		dest.lfWeight = Util::toInt(Text::fromT(sl[2]));
		dest.lfItalic = (BYTE)Util::toInt(Text::fromT(sl[3]));
	}
	
	if(!face.empty()) {
		::ZeroMemory(dest.lfFaceName, LF_FACESIZE);
		_tcscpy(dest.lfFaceName, face.c_str());
	}
}

int CALLBACK WinUtil::browseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /*lp*/, LPARAM pData) {
	switch(uMsg) {
	case BFFM_INITIALIZED: 
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
		break;
	}
	return 0;
}

bool WinUtil::browseDirectory(tstring& target, HWND owner /* = NULL */) {
	TCHAR buf[MAX_PATH];
	BROWSEINFO bi;
	LPMALLOC ma;
	
	ZeroMemory(&bi, sizeof(bi));
	
	bi.hwndOwner = owner;
	bi.pszDisplayName = buf;
	bi.lpszTitle = CTSTRING(CHOOSE_FOLDER);
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
	bi.lParam = (LPARAM)target.c_str();
	bi.lpfn = &browseCallbackProc;
	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if(pidl != NULL) {
		SHGetPathFromIDList(pidl, buf);
		target = buf;
		
		if(target.size() > 0 && target[target.size()-1] != L'\\')
			target+=L'\\';
		
		if(SHGetMalloc(&ma) != E_FAIL) {
			ma->Free(pidl);
			ma->Release();
		}
		return true;
	}
	return false;
}

bool WinUtil::browseFile(tstring& target, HWND owner /* = NULL */, bool save /* = true */, const tstring& initialDir /* = Util::emptyString */, const TCHAR* types /* = NULL */, const TCHAR* defExt /* = NULL */) {
	TCHAR buf[MAX_PATH];
	OPENFILENAME ofn;       // common dialog box structure
	target = Text::toT(Util::validateFileName(Text::fromT(target)));
	_tcscpy(buf, target.c_str());
	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = owner;
	ofn.lpstrFile = buf;
	ofn.lpstrFilter = types;
	ofn.lpstrDefExt = defExt;
	ofn.nFilterIndex = 1;

	if(!initialDir.empty()) {
		ofn.lpstrInitialDir = initialDir.c_str();
	}
	ofn.nMaxFile = sizeof(buf);
	ofn.Flags = (save ? 0: OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST);
	
	// Display the Open dialog box. 
	if ( (save ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn) ) ==TRUE) {
		target = ofn.lpstrFile;
		return true;
	}
	return false;
}

tstring WinUtil::encodeFont(LOGFONT const& font)
{
	tstring res(font.lfFaceName);
	res += L',';
	res += Text::toT(Util::toString(font.lfHeight));
	res += L',';
	res += Text::toT(Util::toString(font.lfWeight));
	res += L',';
	res += Text::toT(Util::toString(font.lfItalic));
	return res;
}


void WinUtil::setClipboard(const tstring& str) {
	if(!::OpenClipboard(mainWnd)) {
		return;
	}

	EmptyClipboard();

#ifdef UNICODE	
	OSVERSIONINFOEX ver;
	if( WinUtil::getVersionInfo(ver) ) {
		if( ver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ) {
			string tmp = Text::wideToAcp(str);

			HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (tmp.size() + 1) * sizeof(char)); 
			if (hglbCopy == NULL) { 
				CloseClipboard(); 
				return; 
			} 

			// Lock the handle and copy the text to the buffer. 
			char* lptstrCopy = (char*)GlobalLock(hglbCopy); 
			strcpy(lptstrCopy, tmp.c_str());
			GlobalUnlock(hglbCopy);

			SetClipboardData(CF_TEXT, hglbCopy);

			CloseClipboard();

			return;
		}
	}
#endif

	// Allocate a global memory object for the text. 
	HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (str.size() + 1) * sizeof(TCHAR)); 
	if (hglbCopy == NULL) { 
		CloseClipboard(); 
		return; 
	} 

	// Lock the handle and copy the text to the buffer. 
	TCHAR* lptstrCopy = (TCHAR*)GlobalLock(hglbCopy); 
	_tcscpy(lptstrCopy, str.c_str());
	GlobalUnlock(hglbCopy); 

	// Place the handle on the clipboard. 
#ifdef UNICODE
	SetClipboardData(CF_UNICODETEXT, hglbCopy); 
#else
	SetClipboardData(CF_TEXT hglbCopy);
#endif

	CloseClipboard();
}

void WinUtil::splitTokens(int* array, const string& tokens, int maxItems /* = -1 */) throw() {
	StringTokenizer<string> t(tokens, _T(','));
	StringList& l = t.getTokens();
	if(maxItems == -1)
		maxItems = l.size();
	
	int k = 0;
	for(StringList::const_iterator i = l.begin(); i != l.end() && k < maxItems; ++i, ++k) {
		array[k] = Util::toInt(*i);
	}
}

bool WinUtil::getUCParams(HWND parent, const UserCommand& uc, StringMap& sm) throw() {
	string::size_type i = 0;
	StringMap done;

	while( (i = uc.getCommand().find("%[line:", i)) != string::npos) {
		i += 7;
		string::size_type j = uc.getCommand().find(']', i);
		if(j == string::npos)
			break;

		string name = uc.getCommand().substr(i, j-i);
		if(done.find(name) == done.end()) {
			LineDlg dlg;
			dlg.title = Text::toT(uc.getName());
			dlg.description = Text::toT(name);
			dlg.line = Text::toT(sm["line:" + name]);
			if(dlg.DoModal(parent) == IDOK) {
				sm["line:" + name] = Text::fromT(dlg.line);
				done[name] = Text::fromT(dlg.line);
			} else {
				return false;
			}
		}
		i = j + 1;
	}
	i = 0;
	while( (i = uc.getCommand().find("%[kickline:", i)) != string::npos) {
		i += 11;
		string::size_type j = uc.getCommand().find(']', i);
		if(j == string::npos)
			break;

		string name = uc.getCommand().substr(i, j-i);
		if(done.find(name) == done.end()) {
			KickDlg dlg;
			dlg.title = Text::toT(uc.getName());
			dlg.description = Text::toT(name);
			if(dlg.DoModal(parent) == IDOK) {
				sm["kickline:" + name] = Text::fromT(dlg.line);
				done[name] = Text::fromT(dlg.line);
			} else {
				return false;
			}
		}
		i = j + 1;
	}
	return true;
}

#ifdef isCVS
#define LINE2 _T("-- http://strongdc.berlios.de  <StrongDC++ ") _T(VERSIONSTRING) _T("") _T(STRONGDCVERSIONSTRING) _T("") _T(CVSVERSION) _T(">")
#else
#define LINE2 _T("-- http://strongdc.berlios.de  <StrongDC++ ") _T(VERSIONSTRING) _T("") _T(STRONGDCVERSIONSTRING) _T(">")
#endif
TCHAR *msgs[] = { _T("\r\n-- I'm a happy StrongDC++ user. You could be happy too.\r\n") LINE2,
_T("\r\n-- Neo-...what? Nope...never heard of it...\r\n") LINE2,
_T("\r\n-- Evolution of species: Ape --> Man\r\n-- Evolution of science: \"The Earth is Flat\" --> \"The Earth is Round\"\r\n-- Evolution of sharing: DC++ --> StrongDC++\r\n") LINE2,
_T("\r\n-- I share, therefore I am.\r\n") LINE2,
_T("\r\n-- I came, I searched, I found...\r\n") LINE2,
_T("\r\n-- I came, I shared, I sent...\r\n") LINE2,
_T("\r\n-- I can set away mode, can't you?\r\n") LINE2,
_T("\r\n-- I don't have to see any ads, do you?\r\n") LINE2,
_T("\r\n-- I don't have to see those annoying kick messages, do you?\r\n") LINE2,
_T("\r\n-- I can resume my files to a different filename, can you?\r\n") LINE2,
_T("\r\n-- I can share huge amounts of files, can you?\r\n") LINE2,
_T("\r\n-- My client doesn't spam the chat with useless debug messages, does yours?\r\n") LINE2,
_T("\r\n-- I can add multiple users to the same file and download from them simultaneously :)\r\n") LINE2,
_T("\r\n-- These addies are pretty annoying, aren't they? Get revenge by sending them yourself!\r\n") LINE2,
_T("\r\n-- My client supports TTH hashes, does yours?\r\n") LINE2,
_T("\r\n-- My client supports XML file lists, does yours?\r\n") LINE2,
_T("\r\n-- I support segmented downloading and I can't lose precious slot between segments :-P?\r\n") LINE2,
_T("\r\n-- Nepodporuju klienty bez TTH proto jim nedam extra slot na filelist ;)\r\n") LINE2,
_T("\r\n-- Umim stahovat segmentove bez poskozeni souboru :-))\r\n") LINE2,
_T("\r\n-- Dokazu seskupovat vysledky hledani se stejnym TTH pod jednu polozku ;)\r\n") LINE2,
_T("\r\n-- Nedovolim michat soubory s TTH a bez TTH a predejdu tak poskozeni souboru :-)\r\n") LINE2,
_T("\r\n-- Kontroluji data behem prenosu a zarucim spravnou integritu dat :)\r\n") LINE2,
_T("\r\n-- Nekdo ma a nekdo nema....ja mam (ale nedam :-)) )\r\n") LINE2,
_T("\r\n-- Podporuju magnet-linky, takze muzu zarucit, ze nestahnu zadne falesne soubory :-))\r\n") LINE2,
_T("\r\n-- Muzu omezit rychlost sveho downloadu aby mi zbyla linka pro brouzdani na webu :-D\r\n") LINE2
};

#define MSGS 25

string WinUtil::commands = "\t\t\t\t\t HELP \t\t\t\t\t\t\t\t\n------------------------------------------------------------------------------------------------------------------------------------------------------------\n/refresh \t\t\t\t(obnoveni share) \t\t\t\t\t\t\n/savequeue \t\t\t\t(ulozi Download Queue) \t\t\t\t\t\t\n------------------------------------------------------------------------------------------------------------------------------------------------------------\n/search <string> \t\t\t(hledat neco...) \t\t\t\t\t\t\t\n/g <searchstring> \t\t\t(hledat Googlem) \t\t\t\t\t\t\n/imdb <imdbquery> \t\t\t(hledat film v IMDB databazi) \t\t\t\t\t\n/whois [IP] \t\t\t\t(hledat podrobnosti o IP) \t\t\t\t\t\n------------------------------------------------------------------------------------------------------------------------------------------------------------\n/slots # \t\t\t\t(upload sloty) \t\t\t\t\t\t\t\n/extraslots # \t\t\t\t(extra sloty pro male soubory) \t\t\t\t\t\n/smallfilesize # \t\t\t\t(maximalni velikost malych souboru) \t\t\t\t\n/ts \t\t\t\t\t(zobrazi datum a cas u zprav v mainchatu) \t\t\t\n/connection \t\t\t\t(zobrazi IP a port prez ktery jste pripojen) \t\t\t\t\n/showjoins \t\t\t\t(zapne/vypne zobrazovani a odpojovani useru v mainchatu) \t\n/showblockedipports \t\t\t(zobrazi zablokovane porty-mozna:)) \t\t\t\t\n/shutdown \t\t\t\t(vypne pocitac po urcitem timeoutu) \t\t\t\t\n------------------------------------------------------------------------------------------------------------------------------------------------------------\n/dc++ \t\t\t\t\t(zobrazi verzi DC++ v mainchatu) \t\t\t\t\t\n/strongdc++ \t\t\t\t(zobrazi verzi StrongDC++ v mainchatu) \t\t\t\t\n------------------------------------------------------------------------------------------------------------------------------------------------------------\n/away <msg> \t\t\t\t(zapne/vypne away mod) \t\t\t\t\t\n/winamp \t\t\t\t(Winamp spam v mainchatu) \t\t\t\t\t\n/w \t\t\t\t\t(Winamp spam v mainchatu) \t\t\t\t\t\n/clear,/c \t\t\t\t(smazat obsah mainchatu) \t\t\t\t\t\n/ignorelist \t\t\t\t(zobrazi ignorelist v mainchatu) \t\t\t\t\t\n";

bool WinUtil::checkCommand(tstring& cmd, tstring& param, tstring& message, tstring& status) {
	string::size_type i = cmd.find(' ');
	if(i != string::npos) {
		param = cmd.substr(i+1);
		cmd = cmd.substr(1, i - 1);
	} else {
		cmd = cmd.substr(1);
	}

	if(Util::stricmp(cmd.c_str(), _T("log")) == 0) {
		if(Util::stricmp(param.c_str(), _T("system")) == 0) {
			WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + "system.log")));
		} else if(Util::stricmp(param.c_str(), _T("downloads")) == 0) {
			WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatTime(SETTING(LOG_FILE_DOWNLOAD), time(NULL)))));
		} else if(Util::stricmp(param.c_str(), _T("uploads")) == 0) {
			WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatTime(SETTING(LOG_FILE_UPLOAD), time(NULL)))));
		} else {
			return false;
		}
	} else if(Util::stricmp(cmd.c_str(), _T("refresh"))==0) {
		try {
			ShareManager::getInstance()->setDirty();
			ShareManager::getInstance()->refresh(true);
		} catch(const ShareException& e) {
			status = Text::toT(e.getError());
		}
	} else if(Util::stricmp(cmd.c_str(), _T("slots"))==0) {
		int j = Util::toInt(Text::fromT(param));
		if(j > 0) {
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, j);
			status = TSTRING(SLOTS_SET);
			ClientManager::getInstance()->infoUpdated(false);
		} else {
			status = TSTRING(INVALID_NUMBER_OF_SLOTS);
		}
	} else if(Util::stricmp(cmd.c_str(), _T("search")) == 0) {
		if(!param.empty()) {
			SearchFrame::openWindow(param);
		} else {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		}
	} else if(Util::stricmp(cmd.c_str(), _T("strongdc++")) == 0) {
		message = msgs[GET_TICK() % MSGS];
	} else if(Util::stricmp(cmd.c_str(), _T("away")) == 0) {
		if(Util::getAway() && param.empty()) {
			Util::setAway(false);
			MainFrame::setAwayButton(false);
			status = TSTRING(AWAY_MODE_OFF);
		} else {
			Util::setAway(true);
			MainFrame::setAwayButton(true);
			Util::setAwayMessage(Text::fromT(param));
			status = TSTRING(AWAY_MODE_ON) + Text::toT(Util::getAwayMessage());
		}
		ClientManager::getInstance()->infoUpdated(true);
	} else if(Util::stricmp(cmd.c_str(), _T("g")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://www.google.com/search?q=") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else if(Util::stricmp(cmd.c_str(), _T("imdb")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://www.imdb.com/find?q=") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else if(Util::stricmp(cmd.c_str(), _T("rebuild")) == 0) {
		HashManager::getInstance()->rebuild();
	} else if(Util::stricmp(cmd.c_str(), _T("shutdown")) == 0) {
		MainFrame::setShutDown(!(MainFrame::getShutDown()));
		if (MainFrame::getShutDown()) {
			status = TSTRING(SHUTDOWN_ON);
		} else {
			status = TSTRING(SHUTDOWN_OFF);
			}
	} else if((Util::stricmp(cmd.c_str(), _T("winamp")) == 0) || (Util::stricmp(cmd.c_str(), _T("w")) == 0)) {
		HWND hwndWinamp = FindWindow(_T("Winamp v1.x"), NULL);
		if (hwndWinamp) {
			StringMap params;
			int waVersion = SendMessage(hwndWinamp,WM_USER, 0, IPC_GETVERSION),
				majorVersion, minorVersion;
			majorVersion = waVersion >> 12;
			if (((waVersion & 0x00F0) >> 4) == 0) {
				minorVersion = ((waVersion & 0x0f00) >> 8) * 10 + (waVersion & 0x000f);
			} else {
				minorVersion = ((waVersion & 0x00f0) >> 4) * 10 + (waVersion & 0x000f);
			}
			params["version"] = Util::toString(majorVersion + minorVersion / 100.0);
			int state = SendMessage(hwndWinamp,WM_USER, 0, IPC_ISPLAYING);
			switch (state) {
				case 0: params["state"] = "stopped";
					break;
				case 1: params["state"] = "playing";
					break;
				case 3: params["state"] = "paused";
			};
			TCHAR titleBuffer[2048];
			int buffLength = sizeof(titleBuffer);
			GetWindowText(hwndWinamp, titleBuffer, buffLength);
			tstring title = titleBuffer;
			params["rawtitle"] = Text::fromT(title);
			// fix the title if scrolling is on, so need to put the stairs to the end of it
			tstring titletmp = title.substr(title.find(_T("***"))+2, title.size());
			title = titletmp + title.substr(0, title.size()-titletmp.size());
			title = title.substr(title.find(_T("."))+2, title.size());
			if (title.rfind(_T("-")) != string::npos) {
				params["title"] = Text::fromT(title.substr(0, title.rfind(_T('-')) - 1));
			}
			int curPos = SendMessage(hwndWinamp,WM_USER, 0, IPC_GETOUTPUTTIME);
			int length = SendMessage(hwndWinamp,WM_USER, 1, IPC_GETOUTPUTTIME);
			if (curPos == -1) {
				curPos = 0;
			} else {
				curPos /= 1000;
			}
			int intPercent;
			if (length > 0 ) {
				intPercent = curPos * 100 / length;
			} else {
				length = 0;
				intPercent = 0;
			}
			params["percent"] = Util::toString(intPercent) + "%";
			params["elapsed"] = Util::formatSeconds(curPos, true);
			params["length"] = Util::formatSeconds(length, true);
			int numFront = min(max(intPercent / 10, 0), 10),
				numBack = min(max(10 - 1 - numFront, 0), 10);
			string inFront = string(numFront, '-'),
				inBack = string(numBack, '-');
			params["bar"] = "[" + inFront + (state ? "|" : "-") + inBack + "]";
			int waSampleRate = SendMessage(hwndWinamp,WM_USER, 0, IPC_GETINFO),
				waBitRate = SendMessage(hwndWinamp,WM_USER, 1, IPC_GETINFO),
				waChannels = SendMessage(hwndWinamp,WM_USER, 2, IPC_GETINFO);
			params["bitrate"] = Util::toString(waBitRate) + "kbps";
			params["sample"] = Util::toString(waSampleRate) + "kHz";
			params["channels"] = (waChannels==2?"stereo":"mono"); // 3+ channels? 0 channels?
			message = Text::toT(Util::formatParams(SETTING(WINAMP_FORMAT), params));
		} else {
			status = _T("Supported version of Winamp is not running");
		}
	} else if(Util::stricmp(cmd.c_str(), _T("tvtome")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else
			WinUtil::openLink(_T("http://www.tvtome.com/tvtome/servlet/Search?searchType=all&searchString=") + Text::toT(Util::encodeURI(Text::fromT(param))));
	} else {
		return false;
	}

	return true;
}

void WinUtil::bitziLink(const TTHValue* aHash) {
	// to use this free service by bitzi, we must not hammer or request information from bitzi
	// except when the user requests it (a mass lookup isn't acceptable), and (if we ever fetch
	// this data within DC++, we must identify the client/mod in the user agent, so abuse can be 
	// tracked down and the code can be fixed
	if(aHash != NULL) {
		openLink(_T("http://bitzi.com/lookup/tree:tiger:") + Text::toT(aHash->toBase32()));
	}
}

 void WinUtil::copyMagnet(const TTHValue* aHash, const tstring& aFile, int64_t aSize) {
	if(aHash != NULL && !aFile.empty()) {
		setClipboard(Text::toT("magnet:?xt=urn:tree:tiger:" + aHash->toBase32() + "&xl=" + Util::toString(aSize) + "&dn=" + Util::encodeURI(Text::fromT(aFile))));
	}
}

 void WinUtil::searchHash(const TTHValue* aHash) {
	 if(aHash != NULL) {
		SearchFrame::openWindow(Text::toT(aHash->toBase32()), 0, SearchManager::SIZE_DONTCARE, SearchManager::TYPE_TTH);
 	}
 }

 void WinUtil::registerDchubHandler() {
	HKEY hk;
	TCHAR Buf[512];
	tstring app = _T("\"") + Text::toT(Util::getAppName()) + _T("\" %1");
	Buf[0] = 0;

	if(::RegOpenKeyEx(HKEY_CLASSES_ROOT, _T("dchub\\Shell\\Open\\Command"), 0, KEY_WRITE | KEY_READ, &hk) == ERROR_SUCCESS) {
		DWORD bufLen = sizeof(Buf);
		DWORD type;
		::RegQueryValueEx(hk, NULL, 0, &type, (LPBYTE)Buf, &bufLen);
		::RegCloseKey(hk);
	}

	if(Util::stricmp(app.c_str(), Buf) != 0) {
		::RegCreateKey(HKEY_CLASSES_ROOT, _T("dchub"), &hk);
		TCHAR* tmp = _T("URL:Direct Connect Protocol");
		::RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)tmp, _tcslen(tmp) + 1);
		::RegSetValueEx(hk, _T("URL Protocol"), 0, REG_SZ, (LPBYTE)_T(""), sizeof(TCHAR));
		::RegCloseKey(hk);

		::RegCreateKey(HKEY_CLASSES_ROOT, _T("dchub\\Shell\\Open\\Command"), &hk);
		::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		::RegCloseKey(hk);

		::RegCreateKey(HKEY_CLASSES_ROOT, _T("dchub\\DefaultIcon"), &hk);
		app = Text::toT(Util::getAppName());
		::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		::RegCloseKey(hk);
	}
}

 void WinUtil::registerADChubHandler() {
	 HKEY hk;
	 TCHAR Buf[512];
	 tstring app = _T("\"") + Text::toT(Util::getAppName()) + _T("\" %1");
	 Buf[0] = 0;

	 if(::RegOpenKeyEx(HKEY_CLASSES_ROOT, _T("adc\\Shell\\Open\\Command"), 0, KEY_WRITE | KEY_READ, &hk) == ERROR_SUCCESS) {
		 DWORD bufLen = sizeof(Buf);
		 DWORD type;
		 ::RegQueryValueEx(hk, NULL, 0, &type, (LPBYTE)Buf, &bufLen);
		 ::RegCloseKey(hk);
	 }

	 if(Util::stricmp(app.c_str(), Buf) != 0) {
		 ::RegCreateKey(HKEY_CLASSES_ROOT, _T("adc"), &hk);
		 TCHAR* tmp = _T("URL:Direct Connect Protocol");
		 ::RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)tmp, _tcslen(tmp) + 1);
		 ::RegSetValueEx(hk, _T("URL Protocol"), 0, REG_SZ, (LPBYTE)_T(""), sizeof(TCHAR));
		 ::RegCloseKey(hk);

		 ::RegCreateKey(HKEY_CLASSES_ROOT, _T("adc\\Shell\\Open\\Command"), &hk);
		 ::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		 ::RegCloseKey(hk);

		 ::RegCreateKey(HKEY_CLASSES_ROOT, _T("adc\\DefaultIcon"), &hk);
		 app = Text::toT(Util::getAppName());
		 ::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		 ::RegCloseKey(hk);
	 }
 }

void WinUtil::registerMagnetHandler() {
	HKEY hk;
	TCHAR buf[512];
	tstring openCmd, magnetLoc, magnetExe;
	buf[0] = 0;
	bool haveMagnet = true;

	// what command is set up to handle magnets right now?
	if(::RegOpenKeyEx(HKEY_CLASSES_ROOT, _T("magnet\\shell\\open\\command"), 0, KEY_READ, &hk) == ERROR_SUCCESS) {
		DWORD bufLen = sizeof(TCHAR) * sizeof(buf);
		::RegQueryValueEx(hk, NULL, NULL, NULL, (LPBYTE)buf, &bufLen);
		::RegCloseKey(hk);
	}
	openCmd = buf;
	buf[0] = 0;
	// read the location of magnet.exe
	if(::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Magnet"), NULL, KEY_READ, &hk) == ERROR_SUCCESS) {
		DWORD bufLen = sizeof(buf) * sizeof(TCHAR);
		::RegQueryValueEx(hk, _T("Location"), NULL, NULL, (LPBYTE)buf, &bufLen);
		::RegCloseKey(hk);
	}
	magnetLoc = buf;
	string::size_type i;
	if (magnetLoc[0]==_T('"') && string::npos != (i = magnetLoc.find(_T('"'), 1))) {
		magnetExe = magnetLoc.substr(1, i-1);
	}
	// check for the existence of magnet.exe
	if(File::getSize(Text::fromT(magnetExe)) == -1) {
		magnetExe = Text::toT(Util::getAppPath() + "magnet.exe");
		if(File::getSize(Text::fromT(magnetExe)) == -1) {
			// gracefully fall back to registering DC++ to handle magnets
			magnetExe = Text::toT(Util::getAppName());
			haveMagnet = false;
		} else {
			// set Magnet\Location
			::RegCreateKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Magnet"), &hk);
			::RegSetValueEx(hk, _T("Location"), NULL, REG_SZ, (LPBYTE)magnetExe.c_str(), sizeof(TCHAR) * (magnetExe.length()+1));
			::RegCloseKey(hk);
		}
		magnetLoc = _T('"') + magnetExe + _T('"');
	}
	// (re)register the handler if magnet.exe isn't the default, or if DC++ is handling it
	if(BOOLSETTING(MAGNET_URI_HANDLER) && (Util::strnicmp(openCmd, magnetLoc, magnetLoc.size()) != 0 || !haveMagnet)) {
		SHDeleteKey(HKEY_CLASSES_ROOT, _T("magnet"));
		::RegCreateKey(HKEY_CLASSES_ROOT, _T("magnet"), &hk);
		::RegSetValueEx(hk, NULL, NULL, REG_SZ, (LPBYTE)CTSTRING(MAGNET_SHELL_DESC), sizeof(TCHAR)*(TSTRING(MAGNET_SHELL_DESC).length()+1));
		::RegSetValueEx(hk, _T("URL Protocol"), NULL, REG_SZ, NULL, NULL);
		::RegCloseKey(hk);
		::RegCreateKey(HKEY_CLASSES_ROOT, _T("magnet\\DefaultIcon"), &hk);
		::RegSetValueEx(hk, NULL, NULL, REG_SZ, (LPBYTE)magnetLoc.c_str(), sizeof(TCHAR)*(magnetLoc.length()+1));
		::RegCloseKey(hk);
		magnetLoc += _T(" %1");
		::RegCreateKey(HKEY_CLASSES_ROOT, _T("magnet\\shell\\open\\command"), &hk);
		::RegSetValueEx(hk, NULL, NULL, REG_SZ, (LPBYTE)magnetLoc.c_str(), sizeof(TCHAR)*(magnetLoc.length()+1));
		::RegCloseKey(hk);
	}
	// magnet-handler specific code
	// clean out the DC++ tree first
	SHDeleteKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Magnet\\Handlers\\CZDC++"));
	// add DC++ to magnet-handler's list of applications
	::RegCreateKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Magnet\\Handlers\\CZDC++"), &hk);
	::RegSetValueEx(hk, NULL, NULL, REG_SZ, (LPBYTE)CTSTRING(MAGNET_HANDLER_ROOT), sizeof(TCHAR) * (TSTRING(MAGNET_HANDLER_ROOT).size()+1));
	::RegSetValueEx(hk, _T("Description"), NULL, REG_SZ, (LPBYTE)CTSTRING(MAGNET_HANDLER_DESC), sizeof(TCHAR) * (STRING(MAGNET_HANDLER_DESC).size()+1));
	// set ShellExecute
	tstring app = Text::toT("\"" + Util::getAppName() + "\" %URL");
	::RegSetValueEx(hk, _T("ShellExecute"), NULL, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length()+1));
	// set DefaultIcon
	app = Text::toT('"' + Util::getAppName() + '"');
	::RegSetValueEx(hk, _T("DefaultIcon"), NULL, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR)*(app.length()+1));
	::RegCloseKey(hk);

	// These two types contain a tth root, and are in common use.  The other two are variations picked up
	// from Shareaza's source, which come second hand from Gordon Mohr.  -GargoyleMT
	// Reference: http://forums.shareaza.com/showthread.php?threadid=23731
	// Note: the three part hash types require magnethandler >= 1.0.0.3
	DWORD nothing = 0;
	::RegCreateKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Magnet\\Handlers\\StrongDC++\\Type"), &hk);
	::RegSetValueEx(hk, _T("urn:bitprint"), NULL, REG_DWORD, (LPBYTE)&nothing, sizeof(nothing));
	::RegSetValueEx(hk, _T("urn:tree:tiger"), NULL, REG_DWORD, (LPBYTE)&nothing, sizeof(nothing));
	::RegSetValueEx(hk, _T("urn:tree:tiger/"), NULL, REG_DWORD, (LPBYTE)&nothing, sizeof(nothing));
	::RegSetValueEx(hk, _T("urn:tree:tiger/1024"), NULL, REG_DWORD, (LPBYTE)&nothing, sizeof(nothing));
	::RegCloseKey(hk);
}

void WinUtil::openLink(const tstring& url) {
	CRegKey key;
	TCHAR regbuf[MAX_PATH];
	ULONG len = MAX_PATH;
	if(strnicmp(Text::fromT(url).c_str(), "magnet:?", 8) == 0) {
		parseMagnetUri(url);
		return;
	}
	if(strnicmp(Text::fromT(url).c_str(), "dchub://", 8) == 0) {
		parseDchubUrl(url);
		return;
	}
	tstring x;

	tstring::size_type i = url.find(_T("://"));
	if(i != string::npos) {
		x = url.substr(0, i);
	} else {
		x = _T("http");
	}
	x += _T("\\shell\\open\\command");
	if(key.Open(HKEY_CLASSES_ROOT, x.c_str(), KEY_READ) == ERROR_SUCCESS) {
		if(key.QueryStringValue(NULL, regbuf, &len) == ERROR_SUCCESS) {
			/*
			 * Various values (for http handlers):
			 *  C:\PROGRA~1\MOZILL~1\FIREFOX.EXE -url "%1"
			 *  "C:\Program Files\Internet Explorer\iexplore.exe" -nohome
			 *  "C:\Apps\Opera7\opera.exe"
			 *  C:\PROGRAMY\MOZILLA\MOZILLA.EXE -url "%1" <= to je mojeeeeeeee :-D
			 *  C:\PROGRA~1\NETSCAPE\NETSCAPE\NETSCP.EXE -url "%1"
			 */
			tstring cmd(regbuf); // otherwise you consistently get two trailing nulls
			
			if(cmd.length() > 1) {
				string::size_type start,end;
				if(cmd[0] == '"') {
					start = 1;
					end = cmd.find('"', 1);
				} else {
					start = 0;
					end = cmd.find(' ', 1);
				}
				if(end == string::npos)
					end = cmd.length();

				tstring cmdLine(cmd);
				cmd = cmd.substr(start, end-start);
				size_t arg_pos;
				if((arg_pos = cmdLine.find(_T("%1"))) != string::npos) {
					cmdLine.replace(arg_pos, 2, url);
				} else {
					cmdLine.append(_T(" \"") + url + _T('\"'));
				}

				STARTUPINFO si = { sizeof(si), 0 };
				PROCESS_INFORMATION pi = { 0 };
				AutoArray<TCHAR> buf(cmdLine.length() + 1);
				_tcscpy(buf, cmdLine.c_str());
				if(::CreateProcess(cmd.c_str(), buf, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
					::CloseHandle(pi.hThread);
					::CloseHandle(pi.hProcess);
					return;
				}
			}
		}
	}

	::ShellExecute(NULL, NULL, url.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void WinUtil::parseDchubUrl(const tstring& aUrl) {
	string server, file;
	u_int16_t port = 411;
	Util::decodeUrl(Text::fromT(aUrl), server, port, file);
	if(!server.empty()) {
		HubFrame::openWindow(Text::toT(server + ":" + Util::toString(port)));
	}
	if(!file.empty()) {
		if(file[0] == '/') // Remove any '/' in from of the file
			file = file.substr(1);
		try {
			QueueManager::getInstance()->addList(ClientManager::getInstance()->getUser(file), QueueItem::FLAG_CLIENT_VIEW);
		} catch(const Exception&) {
			// ...
		}
	}
}

void WinUtil::parseADChubUrl(const tstring& aUrl) {
	string server, file;
	u_int16_t port = 0; //make sure we get a port since adc doesn't have a standard one
	Util::decodeUrl(Text::fromT(aUrl), server, port, file);
	if(!server.empty() && port > 0) {
		HubFrame::openWindow(Text::toT("adc://" + server + ":" + Util::toString(port)));
	}
}

void WinUtil::parseMagnetUri(const tstring& aUrl, bool /*aOverride*/) {
	// official types that are of interest to us
	//  xt = exact topic
	//  xs = exact substitute
	//  as = acceptable substitute
	//  dn = display name
	//  xl = exact length
	if (Util::strnicmp(aUrl.c_str(), _T("magnet:?"), 8) == 0) {
		LogManager::getInstance()->message(STRING(MAGNET_DLG_TITLE) + ": " + Text::fromT(aUrl), true);
		StringTokenizer<tstring> mag(aUrl.substr(8), _T('&'));
		typedef map<tstring, tstring> MagMap;
		MagMap hashes;
		tstring fname, fhash, type, param;
		int64_t fsize = 0;
		for(TStringList::iterator idx = mag.getTokens().begin(); idx != mag.getTokens().end(); ++idx) {
			// break into pairs
			string::size_type pos = idx->find(_T('='));
			if(pos != string::npos) {
				type = Text::toT(Text::toLower(Util::encodeURI(Text::fromT(idx->substr(0, pos)), true)));
				param = Text::toT(Util::encodeURI(Text::fromT(idx->substr(pos+1)), true));
			} else {
				type = Text::toT(Util::encodeURI(Text::fromT(*idx), true));
				param.clear();
			}
			// extract what is of value
			if(param.length() == 85 && Util::strnicmp(param.c_str(), _T("urn:bitprint:"), 13) == 0) {
				hashes[type] = param.substr(46);
			} else if(param.length() == 54 && Util::strnicmp(param.c_str(), _T("urn:tree:tiger:"), 15) == 0) {
				hashes[type] = param.substr(15);
			} else if(param.length() == 55 && Util::strnicmp(param.c_str(), _T("urn:tree:tiger/:"), 16) == 0) {
				hashes[type] = param.substr(16);
			} else if(param.length() == 59 && Util::strnicmp(param.c_str(), _T("urn:tree:tiger/1024:"), 20) == 0) {
				hashes[type] = param.substr(20);
			} else if(type.length() == 2 && Util::strnicmp(type.c_str(), _T("dn"), 2) == 0) {
				fname = param;
			} else if(type.length() == 2 && Util::strnicmp(type.c_str(), _T("xl"), 2) == 0) {
				fsize = _tstoi64(param.c_str());
			}
		}
		// pick the most authoritative hash out of all of them.
		if(hashes.find(_T("as")) != hashes.end()) {
			fhash = hashes[_T("as")];
		}
		if(hashes.find(_T("xs")) != hashes.end()) {
			fhash = hashes[_T("xs")];
		}
		if(hashes.find(_T("xt")) != hashes.end()) {
			fhash = hashes[_T("xt")];
		}
		if(!fhash.empty() && Encoder::isBase32(Text::fromT(fhash).c_str())){
			// ok, we have a hash, and maybe a filename.
			if(!BOOLSETTING(MAGNET_ASK) && fsize > 0 && fname.length() > 0) {
				switch(SETTING(MAGNET_ACTION)) {
					case SettingsManager::MAGNET_AUTO_DOWNLOAD:
						QueueManager::getInstance()->add(Text::fromT(fname), fsize, Text::fromT(fhash));
						break;
					case SettingsManager::MAGNET_AUTO_SEARCH:
						SearchFrame::openWindow(fhash, 0, SearchManager::SIZE_DONTCARE, SearchManager::TYPE_TTH);
						break;
				};
			} else {
			// use aOverride to force the display of the dialog.  used for auto-updating
				MagnetDlg dlg(fhash, fname, fsize);
				dlg.DoModal(mainWnd);
			}
		} else {
			MessageBox(mainWnd, CTSTRING(MAGNET_DLG_TEXT_BAD), CTSTRING(MAGNET_DLG_TITLE), MB_OK | MB_ICONEXCLAMATION);
		}
	}
}

int WinUtil::textUnderCursor(POINT p, CEdit& ctrl, tstring& x) {
	
	int i = ctrl.CharFromPos(p);
	int line = ctrl.LineFromChar(i);
	int c = LOWORD(i) - ctrl.LineIndex(line);
	int len = ctrl.LineLength(i) + 1;
	if(len < 3) {
		return 0;
	}

	AutoArray<TCHAR> buf(len);
	ctrl.GetLine(line, buf, len);
	x = tstring(buf, len-1);

	string::size_type start = x.find_last_of(_T(" <\t\r\n"), c);
	if(start == string::npos)
		start = 0;
	else
		start++;

	return start;
}

bool WinUtil::parseDBLClick(const tstring& aString, string::size_type start, string::size_type end) {
	if( (Util::strnicmp(aString.c_str() + start, _T("http://"), 7) == 0) || 
		(Util::strnicmp(aString.c_str() + start, _T("www."), 4) == 0) ||
		(Util::strnicmp(aString.c_str() + start, _T("ftp://"), 6) == 0) ||
		(Util::strnicmp(aString.c_str() + start, _T("irc://"), 6) == 0) ||
		(Util::strnicmp(aString.c_str() + start, _T("https://"), 8) == 0) ||	
		(Util::strnicmp(aString.c_str() + start, _T("file://"), 7) == 0) ||
		(Util::strnicmp(aString.c_str() + start, _T("mailto:"), 7) == 0) )
	{

		openLink(aString.substr(start, end-start));
		return true;
	} else if(Util::strnicmp(aString.c_str() + start, _T("dchub://"), 8) == 0) {
		parseDchubUrl(aString.substr(start, end-start));
		return true;
	} else if(Util::strnicmp(aString.c_str() + start, _T("magnet:?"), 8) == 0) {
		parseMagnetUri(aString.substr(start, end-start));
		return true;
	} else if(Util::strnicmp(aString.c_str() + start, _T("adc://"), 6) == 0) {
		parseADChubUrl(aString.substr(start, end-start));
		return true;
	}
	return false;
}

void WinUtil::saveHeaderOrder(CListViewCtrl& ctrl, SettingsManager::StrSetting order, 
							  SettingsManager::StrSetting widths, int n, 
							  int* indexes, int* sizes) throw() {
	string tmp;

	ctrl.GetColumnOrderArray(n, indexes);
	int i;
	for(i = 0; i < n; ++i) {
		tmp += Util::toString(indexes[i]);
		tmp += ',';
	}
	tmp.erase(tmp.size()-1, 1);
	SettingsManager::getInstance()->set(order, tmp);
	tmp.clear();
	int nHeaderItemsCount = ctrl.GetHeader().GetItemCount();
	for(i = 0; i < n; ++i) {
		sizes[i] = ctrl.GetColumnWidth(i);
		if (i >= nHeaderItemsCount) // Not exist column
			sizes[i] = 0;
		tmp += Util::toString(sizes[i]);
		tmp += ',';
	}
	tmp.erase(tmp.size()-1, 1);
	SettingsManager::getInstance()->set(widths, tmp);
}

int WinUtil::getIconIndex(const tstring& aFileName) {
	if(BOOLSETTING(USE_SYSTEM_ICONS)) {
		SHFILEINFO fi;
		memset(&fi, 0, sizeof(SHFILEINFO));
		string x = Text::toLower(Util::getFileExt(Text::fromT(aFileName)));
		if(!x.empty()) {
			ImageIter j = fileIndexes.find(x);
			if(j != fileIndexes.end())
				return j->second;
		}
		tstring fn = Text::toT(Text::toLower(Util::getFileName(Text::fromT(aFileName))));
		::SHGetFileInfo(fn.c_str(), FILE_ATTRIBUTE_NORMAL, &fi, sizeof(fi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		fileImages.AddIcon(fi.hIcon);
		::DestroyIcon(fi.hIcon);

		fileIndexes[x] = fileImageCount++;
		return fileImageCount - 1;
	} else {
		return 2;
	}
}

int WinUtil::getOsMajor() {
	OSVERSIONINFOEX ver;
	memset(&ver, 0, sizeof(OSVERSIONINFOEX));
	if(!GetVersionEx((OSVERSIONINFO*)&ver)) 
	{
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	}
	GetVersionEx((OSVERSIONINFO*)&ver);
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	return ver.dwMajorVersion;
}

int WinUtil::getOsMinor() 
{
	OSVERSIONINFOEX ver;
	memset(&ver, 0, sizeof(OSVERSIONINFOEX));
	if(!GetVersionEx((OSVERSIONINFO*)&ver)) 
	{
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	}
	GetVersionEx((OSVERSIONINFO*)&ver);
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	return ver.dwMinorVersion;
}

void WinUtil::ClearPreviewMenu(OMenu &previewMenu){
	while(previewMenu.GetMenuItemCount() > 0) {
		previewMenu.RemoveMenu(0, MF_BYPOSITION);
	}
}

int WinUtil::SetupPreviewMenu(CMenu &previewMenu, string extension){
	int PreviewAppsSize = 0;
	PreviewApplication::List lst = HubManager::getInstance()->getPreviewApps();
	if(lst.size()>0){		
		PreviewAppsSize = 0;
		for(PreviewApplication::Iter i = lst.begin(); i != lst.end(); i++){
			StringList tok = StringTokenizer<string>((*i)->getExtension(), ';').getTokens();
			bool add = false;
			if(tok.size()==0)add = true;

			
			for(StringIter si = tok.begin(); si != tok.end(); ++si) {
				if(stricmp(extension.c_str(), si->c_str())==0){
					add = true;
					break;
				}
			}
							
			if (add) previewMenu.AppendMenu(MF_STRING, IDC_PREVIEW_APP + PreviewAppsSize, Text::toT(((*i)->getName())).c_str());
			PreviewAppsSize++;
		}
	}
	return PreviewAppsSize;
}

void WinUtil::RunPreviewCommand(int index, string target){
	PreviewApplication::List lst = HubManager::getInstance()->getPreviewApps();

	if(index <= lst.size()) {
	string application = lst[index]->getApplication();
	string arguments = lst[index]->getArguments();
	StringMap ucParams;				
	
	ucParams["file"] = "\"" + target + "\"";
	ucParams["dir"] = "\"" + Util::getFilePath(target) + "\"";

		::ShellExecute(NULL, NULL, Text::toT(application).c_str(), Text::toT(Util::formatParams(arguments, ucParams)).c_str(), Text::toT(ucParams["dir"]).c_str(), SW_SHOWNORMAL);
	}
}

string WinUtil::formatTime(long rest) {
	char buf[128];
	string formatedTime;
	long n, i;
	i = 0;
	n = rest / (24*3600*7);
	rest %= (24*3600*7);
	if(n) {
		if(n >= 2)
			_snprintf(buf, 127, "%d weeks ", n);
		else
			_snprintf(buf, 127, "%d week ", n);
		buf[127] = 0;
		formatedTime += (string)buf;
		i++;
	}
	n = rest / (24*3600);
	rest %= (24*3600);
	if(n) {
		if(n >= 2)
			_snprintf(buf, 127, "%d days ", n); 
		else
			_snprintf(buf, 127, "%d day ", n);
		buf[127] = 0;
		formatedTime += (string)buf;
		i++;
	}
	n = rest / (3600);
	rest %= (3600);
	if(n) {
		if(n >= 2)
			_snprintf(buf, 127, "%d hours ", n);
		else
			_snprintf(buf, 127, "%d hour ", n);
		buf[127] = 0;
		formatedTime += (string)buf;
		i++;
	}
	n = rest / (60);
	rest %= (60);
	if(n) {
		_snprintf(buf, 127, "%d min ", n);
		buf[127] = 0;
		formatedTime += (string)buf;
		i++;
	}
	n = rest;
	if(++i <= 3) {
		_snprintf(buf, 127,"%d sec ", n); 
		buf[127] = 0;
		formatedTime += (string)buf;
	}
	return formatedTime;
}

int WinUtil::getImage(const User::Ptr& u) {
	int image = u->getcType();
	if(u->isSet(User::OP)) {
		image = 0;
	} else if(u->isSet(User::FIREBALL)) {
		image = 9; // 8
	} else if(u->isSet(User::SERVER)) {
		image = 8; // 7
	}
	if(u->isSet(User::AWAY)) {
		image+=11; // 10
	}
	if(u->isSet(User::DCPLUSPLUS)) {
		image+=22; // 20
	}
	if(u->isSet(User::PASSIVE) || (u->getMode() == "P") || (u->getMode() == "5")) {
		image+=44; // 40
	}
	return image;	
}

int WinUtil::getFlagImage(const char* country, bool fullname) {
	if(fullname) {
		for(size_t i = 1; i <= (sizeof(CountryNames) / sizeof(CountryNames[0])); i++) {
			if(stricmp(country, CountryNames[i-1]) == 0) {
				return i;
			}
		}
	} else {
		for(size_t i = 1; i <= (sizeof(CountryCodes) / sizeof(CountryCodes[0])); i++) {
			if(stricmp(country,CountryCodes[i-1]) == 0) {
				return i;
			}
		}
	}
	return 0;
}


string WinUtil::generateStats() {
	char buf[1024];
	PROCESS_MEMORY_COUNTERS pmc;
	pmc.cb = sizeof(pmc);
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
	FILETIME tmpa, tmpb, kernelTimeFT, userTimeFT;
	GetProcessTimes(GetCurrentProcess(), &tmpa, &tmpb, &kernelTimeFT, &userTimeFT);
	int64_t kernelTime = kernelTimeFT.dwLowDateTime | (((int64_t)kernelTimeFT.dwHighDateTime) << 32);
	int64_t userTime = userTimeFT.dwLowDateTime | (((int64_t)userTimeFT.dwHighDateTime) << 32);  
	sprintf(buf, "\n-=[ StrongDC++ %s %s ]=-\r\n-=[ Uptime: %s ][ Cpu time: %s ]=-\r\n-=[ Memory usage (peak): %s (%s) ]=-\r\n-=[ Virtual memory usage (peak): %s (%s) ]=-\r\n-=[ Downloaded: %s ][ Uploaded: %s ]=-\r\n-=[ Total download: %s ][ Total upload: %s ]=-\r\n-=[ System Uptime: %s ]=-", 
		VERSIONSTRING, STRONGDCVERSIONSTRING, formatTime(Util::getUptime()).c_str(), Util::formatSeconds((kernelTime + userTime) / (10I64 * 1000I64 * 1000I64)).c_str(), 
		Util::formatBytes(pmc.WorkingSetSize).c_str(), Util::formatBytes(pmc.PeakWorkingSetSize).c_str(), 
		Util::formatBytes(pmc.PagefileUsage).c_str(), Util::formatBytes(pmc.PeakPagefileUsage).c_str(), 
		Util::formatBytes(Socket::getTotalDown()).c_str(), Util::formatBytes(Socket::getTotalUp()).c_str(), 
		Util::formatBytes(SETTING(TOTAL_DOWNLOAD)).c_str(), Util::formatBytes(SETTING(TOTAL_UPLOAD)).c_str(), 
		formatTime(::GetTickCount()/1000).c_str());
	return buf;
} 

/**
 * @file
 * $Id$
 */