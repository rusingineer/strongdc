/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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

#include "MainFrm.h"
#include "ExtendedTrace.h"
#include "WinUtil.h"
#include "SingleInstance.h"


#include <delayimp.h>
CAppModule _Module;

CriticalSection cs;
enum { DEBUG_BUFSIZE = 8192 };
static char guard[DEBUG_BUFSIZE];
static int recursion = 0;
static char tth[192*8/(5*8)+2];
static bool firstException = true;

static char buf[DEBUG_BUFSIZE];

EXCEPTION_RECORD CurrExceptionRecord;
CONTEXT CurrContext;
string exceptioninfo;
int iLastExceptionDlgResult;
HINSTANCE hCurrInstance;

void SetCurrInstance(HINSTANCE hInst)
{
	hCurrInstance = hInst;
}

HINSTANCE GetCurrInstance()
{
	return hCurrInstance;
}


void CopyEditToClipboard(HWND hWnd)
{
	SendMessage(hWnd, EM_SETSEL, 0, 65535L);
	SendMessage(hWnd, WM_COPY, 0 , 0);
	SendMessage(hWnd, EM_SETSEL, 0, 0);
}

void GetScreenSize(int *pWidth, int *pHeight)
{
	HDC hDC;
	hDC = GetDC(0);
	if (pWidth != NULL) *pWidth = GetDeviceCaps(hDC, HORZRES);
	if (pHeight != NULL ) *pHeight = GetDeviceCaps(hDC, VERTRES);
	ReleaseDC(0, hDC);
}

void CenterWindow(HWND hwndDlg)
{
	int nWidth, nHeight, nLeft, nTop;
	RECT rcWin;

	GetScreenSize(&nWidth, &nHeight);
	GetWindowRect(hwndDlg, &rcWin);
	
	nLeft = (nWidth - (rcWin.right - rcWin.left + 1)) / 2;
	nTop = (nHeight - (rcWin.bottom - rcWin.top + 1)) / 2;

	MoveWindow(hwndDlg, nLeft, nTop, rcWin.right - rcWin.left + 1, rcWin.bottom - rcWin.top + 1, TRUE);
}


BOOL CALLBACK ExceptionFilterFunctionDlgProc(
  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
	WORD wNotifyCode, wID;
	
	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				CenterWindow(hwndDlg);
				SetDlgItemText(hwndDlg, IDC_EXCEPTION_DETAILS, exceptioninfo.c_str());
				SetFocus(GetDlgItem(hwndDlg, IDC_EXCEPTION_DETAILS));

			}
			break;
	
		case WM_COMMAND:
			wNotifyCode = HIWORD(wParam); // notification code 
			wID = LOWORD(wParam);         // item, control, or accelerator identifier 
			if (wNotifyCode == BN_CLICKED)
			{
				if (wID == IDOK || wID == IDCANCEL)
					EndDialog(hwndDlg, wID);

				if (wID == IDC_COPY_EXCEPTION)
					CopyEditToClipboard(GetDlgItem(hwndDlg, IDC_EXCEPTION_DETAILS));


			}

			break;
	}

	return FALSE;
}

void ExceptionFunction()
{
	if (iLastExceptionDlgResult == IDCANCEL)
	{
		//If the user selects "Terminate Application", exit from the program.
		ExitProcess(1);
	}
}


#ifndef _DEBUG


FARPROC WINAPI FailHook(unsigned /* dliNotify */, PDelayLoadInfo  /* pdli */) {
	MessageBox(NULL, "StrongDC++ just encountered an unhandled exception and will terminate. Please do not report this as a bug, as StrongDC++ was unable to collect the information needed for a useful bug report (Your Operating System doesn't support the functionality needed, probably because it's too old).", "Unhandled Exception", MB_OK | MB_ICONERROR);
	exit(-1);
}

#endif

LONG __stdcall DCUnhandledExceptionFilter( LPEXCEPTION_POINTERS e )
{
	Lock l(cs);

	if(recursion++ > 30)
		exit(-1);

#ifndef _DEBUG
#if _MSC_VER == 1200
	__pfnDliFailureHook = FailHook;
#elif _MSC_VER == 1300 || _MSC_VER == 1310
	__pfnDliFailureHook2 = FailHook;
#else
#error Unknown Compiler version
#endif

	// The release version loads the dll and pdb:s here...
	EXTENDEDTRACEINITIALIZE( Util::getAppPath().c_str() );

#endif

	if(firstException) {
		File::deleteFile(Util::getAppPath() + "exceptioninfo.txt");
		firstException = false;
	}

	if(File::getSize(Util::getAppPath() + "StrongDC.pdb") == -1) {
		// No debug symbols, we're not interested...
		::MessageBox(NULL, "DC++ has crashed and you don't have debug symbols installed. Hence, I can't find out why it crashed, so don't report this as a bug unless you find a solution...", "DC++ has crashed", MB_OK);
#ifndef _DEBUG
		exit(1);
#else
		return EXCEPTION_CONTINUE_SEARCH;
#endif
	}

	File f(Util::getAppPath() + "exceptioninfo.txt", File::WRITE, File::OPEN | File::CREATE);
	f.setEndPos(0);
	
	DWORD exceptionCode = e->ExceptionRecord->ExceptionCode ;

	sprintf(buf, "Code: %x\r\nVersion: %s\r\n", 
		exceptionCode, VERSIONSTRING CZDCVERSIONSTRING);

	f.write(buf, strlen(buf));

	exceptioninfo = buf;

	OSVERSIONINFOEX ver;
	WinUtil::getVersionInfo(ver);

	sprintf(buf, "Major: %d\r\nMinor: %d\r\nBuild: %d\r\nSP: %d\r\nType: %d\r\n",
		(DWORD)ver.dwMajorVersion, (DWORD)ver.dwMinorVersion, (DWORD)ver.dwBuildNumber,
		(DWORD)ver.wServicePackMajor, (DWORD)ver.wProductType);

	exceptioninfo += buf;
	f.write(buf, strlen(buf));
	time_t now;
	time(&now);
	strftime(buf, DEBUG_BUFSIZE, "Time: %Y-%m-%d %H:%M:%S\r\n", localtime(&now));

	exceptioninfo += buf;
	f.write(buf, strlen(buf));

	exceptioninfo += LIT("TTH: ");
	exceptioninfo += tth;
	exceptioninfo += LIT("\r\n");
	exceptioninfo += LIT("\r\n");

	f.write(LIT("TTH: "));
	f.write(tth, strlen(tth));
	f.write(LIT("\r\n"));

    f.write(LIT("\r\n"));

	STACKTRACE2(f, e->ContextRecord->Eip, e->ContextRecord->Esp, e->ContextRecord->Ebp);

	f.close();

	//MessageBox(NULL, "StrongDC++ just encountered an unhandled exception and will terminate. If you plan on reporting this bug to the bug report forum, make sure you have downloaded the debug information (DCPlusPlus.pdb) for your version of DC++. A file named \"exceptioninfo.txt\" has been generated in the same directory as DC++. Please include this file in the report or it'll be removed / ignored. If the file contains a lot of lines that end with '?', it means that the debug information is not correctly installed or your Windows doesn't support the functionality needed, and therefore, again, your report will be ignored/removed.", "DC++ Has Crashed", MB_OK | MB_ICONERROR);
	memcpy(&CurrExceptionRecord, e->ExceptionRecord, sizeof(EXCEPTION_RECORD));
	memcpy(&CurrContext, e->ContextRecord, sizeof(CONTEXT));

	exceptioninfo += StackTrace(GetCurrentThread(), _T(""), e->ContextRecord->Eip, e->ContextRecord->Esp, e->ContextRecord->Ebp);

	iLastExceptionDlgResult = DialogBoxParam(GetCurrInstance(), MAKEINTRESOURCE(IDD_EXCEPTION), 0, ExceptionFilterFunctionDlgProc, 0);
	ExceptionFunction();

#ifndef _DEBUG
	EXTENDEDTRACEUNINITIALIZE();
	
	//exit(-1);
	//return 1;
	return EXCEPTION_CONTINUE_EXECUTION;
#else
	return EXCEPTION_CONTINUE_SEARCH;
#endif
}

static void sendCmdLine(HWND hOther, LPTSTR lpstrCmdLine)
{
	string cmdLine = _T(lpstrCmdLine);
	LRESULT result;

	COPYDATASTRUCT cpd;
	cpd.dwData = 0;
	cpd.cbData = cmdLine.length() + 1;
	cpd.lpData = (void *)cmdLine.c_str();
	result = SendMessage(hOther, WM_COPYDATA, NULL,	(LPARAM)&cpd);
}

BOOL CALLBACK searchOtherInstance(HWND hWnd, LPARAM lParam) {
	DWORD result;
	LRESULT ok = ::SendMessageTimeout(hWnd, WMU_WHERE_ARE_YOU, 0, 0,
		SMTO_BLOCK | SMTO_ABORTIFHUNG, 5000, &result);
	if(ok == 0)
		return TRUE;
	if(result == WMU_WHERE_ARE_YOU) {
		// found it
		HWND *target = (HWND *)lParam;
		*target = hWnd;
		return FALSE;
	}
	return TRUE;
}

static void installUrlHandler() {
	HKEY hk; 
	char Buf[512];
	string app = "\"" + Util::getAppName() + "\" %1";
	Buf[0] = 0;

	if(::RegOpenKeyEx(HKEY_CLASSES_ROOT, "dchub\\Shell\\Open\\Command", 0, KEY_WRITE | KEY_READ, &hk) == ERROR_SUCCESS) {
		DWORD bufLen = sizeof(Buf);
		DWORD type;
		::RegQueryValueEx(hk, NULL, 0, &type, (LPBYTE)Buf, &bufLen);
		::RegCloseKey(hk);
	}

	if(Util::stricmp(app.c_str(), Buf) != 0) {
		::RegCreateKey(HKEY_CLASSES_ROOT, "dchub", &hk);
		char* tmp = "URL:Direct Connect Protocol";
		::RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)tmp, strlen(tmp) + 1);
		::RegSetValueEx(hk, "URL Protocol", 0, REG_SZ, (LPBYTE)"", 1);
		::RegCloseKey(hk);

		::RegCreateKey(HKEY_CLASSES_ROOT, "dchub\\Shell\\Open\\Command", &hk);
		::RegSetValueEx(hk, "", 0, REG_SZ, (LPBYTE)app.c_str(), app.length() + 1);
		::RegCloseKey(hk); 

		::RegCreateKey(HKEY_CLASSES_ROOT, "dchub\\DefaultIcon", &hk);
		app = Util::getAppName();
		::RegSetValueEx(hk, "", 0, REG_SZ, (LPBYTE)app.c_str(), app.length() + 1);
		::RegCloseKey(hk);
	}
} 

static void installUriHandler() {
	HKEY hk; 
	char Buf[512];
	string app = "\"" + Util::getAppName() + "\" %1";
	Buf[0] = 0;

	if(::RegOpenKeyEx(HKEY_CLASSES_ROOT, "magnet\\Shell\\Open\\Command", 0, KEY_WRITE | KEY_READ, &hk) == ERROR_SUCCESS) {
		DWORD bufLen = sizeof(Buf);
		DWORD type;
		::RegQueryValueEx(hk, NULL, 0, &type, (LPBYTE)Buf, &bufLen);
		::RegCloseKey(hk);
	}

	if(Util::stricmp(app.c_str(), Buf) != 0) {
		::RegCreateKey(HKEY_CLASSES_ROOT, "magnet", &hk);
		char* tmp = "URL:Magnet URI";
		::RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)tmp, strlen(tmp) + 1);
		::RegSetValueEx(hk, "URL Protocol", 0, REG_SZ, (LPBYTE)"", 1);
		::RegCloseKey(hk);

		::RegCreateKey(HKEY_CLASSES_ROOT, "magnet\\Shell\\Open\\Command", &hk);
		::RegSetValueEx(hk, "", 0, REG_SZ, (LPBYTE)app.c_str(), app.length() + 1);
		::RegCloseKey(hk); 

		::RegCreateKey(HKEY_CLASSES_ROOT, "magnet\\DefaultIcon", &hk);
		app = Util::getAppName();
		::RegSetValueEx(hk, "", 0, REG_SZ, (LPBYTE)app.c_str(), app.length() + 1);
		::RegCloseKey(hk);
	}
} 

static void checkCommonControls() {
#define PACKVERSION(major,minor) MAKELONG(minor,major)

	HINSTANCE hinstDll;
	DWORD dwVersion = 0;
	
	hinstDll = LoadLibrary("comctl32.dll");
	
	if(hinstDll)
	{
		DLLGETVERSIONPROC pDllGetVersion;
	
		pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");
		
		if(pDllGetVersion)
		{
			DLLVERSIONINFO dvi;
			HRESULT hr;
			
			ZeroMemory(&dvi, sizeof(dvi));
			dvi.cbSize = sizeof(dvi);
			
			hr = (*pDllGetVersion)(&dvi);
			
			if(SUCCEEDED(hr))
			{
				dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
			}
		}
		
		FreeLibrary(hinstDll);
	}

	if(dwVersion < PACKVERSION(5,80)) {
		MessageBox(NULL, "Your version of windows common controls is too old for StrongDC++ to run correctly, and you will most probably experience problems with the user interface. You should download version 5.80 or higher from the DC++ homepage or from Microsoft directly.", "User Interface Warning", MB_OK);
	}
}

static string sTitle;
static HWND hWnd;
static bool bGotTitle;

void callBack(void* x, const string& a) {
	if (!bGotTitle) {
		string sTmp = VERSIONSTRING;
		bGotTitle = true;
		sTitle = sTmp;
	}
	::RedrawWindow(hWnd, NULL, NULL, RDW_UPDATENOW | RDW_INTERNALPAINT);
}
LRESULT CALLBACK splashCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_PAINT) {
		// Get some information
		HDC dc = GetDC(hwnd);
		RECT rc;
		GetWindowRect(hwnd, &rc);
		OffsetRect(&rc, -rc.left, -rc.top);
		RECT rc1 = rc;
		rc1.bottom = rc1.bottom - 35;
		RECT rc2 = rc;
		rc2.top = rc2.bottom - 55; 
		rc2.right = rc2.right - 10;
		::SetBkMode(dc, TRANSPARENT);
		
		// Draw the icon
		HBITMAP hi;
		hi = (HBITMAP)LoadImage(_Module.get_m_hInst(), MAKEINTRESOURCE(IDB_SPLASH), IMAGE_BITMAP, 320,240, LR_SHARED);
			 
		HDC comp=CreateCompatibleDC(dc);
		SelectObject(comp,hi);	

		BitBlt(dc,0, 0 , 320, 240,comp,0,0,SRCCOPY);

		DeleteObject(hi);
		DeleteObject(comp);
		LOGFONT logFont;
		HFONT hFont;
		GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(logFont), &logFont);
		lstrcpy(logFont.lfFaceName, TEXT("Tahoma"));
		logFont.lfHeight = 15;
		logFont.lfWeight = 700;
		hFont = CreateFontIndirect(&logFont);		
		SelectObject(dc, hFont);
		::SetBkMode(dc, OPAQUE);
		::SetBkColor(dc, RGB(255, 255, 255));
		::SetTextColor(dc, RGB(0,0,0));
		::DrawText(dc, sTitle.c_str(), sTitle.length(), &rc2, DT_RIGHT);
		DeleteObject(hFont);
		ReleaseDC(hwnd, dc);
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	checkCommonControls();

	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);
	
	MainFrame wndMain;

	CEdit dummy;
	CWindow splash;
	
	CRect rc;
	rc.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
	rc.top = (rc.bottom / 2) - 80;

	rc.right = GetSystemMetrics(SM_CXFULLSCREEN);
	rc.left = rc.right / 2 - 85;
	
	dummy.Create(NULL, rc, APPNAME " " VERSIONSTRING " " CZDCVERSIONSTRING, WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_CENTER | ES_READONLY, WS_EX_STATICEDGE);
	splash.Create("Static", GetDesktopWindow(), splash.rcDefault, NULL, WS_POPUP | WS_VISIBLE | SS_USERITEM, WS_EX_TOPMOST | WS_EX_TOOLWINDOW);
	splash.SetFont((HFONT)GetStockObject(DEFAULT_GUI_FONT));
	
	HDC dc = splash.GetDC();
	rc.right = rc.left + 320;
	rc.bottom = rc.top + 240;
	splash.ReleaseDC(dc);
	splash.HideCaret();
	splash.SetWindowPos(HWND_TOPMOST, &rc, SWP_SHOWWINDOW);
	splash.SetWindowLong(GWL_WNDPROC, (LONG)&splashCallback);
	splash.CenterWindow();
	sTitle = VERSIONSTRING "" CZDCVERSIONSTRING;
	splash.SetFocus();
	splash.RedrawWindow();

	startup(callBack, (void*)splash.m_hWnd);

	splash.DestroyWindow();
	dummy.DestroyWindow();

	if(BOOLSETTING(URL_HANDLER)) {
		installUrlHandler();
	}

	if(BOOLSETTING(MAGNET_URI_HANDLER)) {
		installUriHandler();
	}

	rc = wndMain.rcDefault;

	if( (SETTING(MAIN_WINDOW_POS_X) != CW_USEDEFAULT) &&
		(SETTING(MAIN_WINDOW_POS_Y) != CW_USEDEFAULT) &&
		(SETTING(MAIN_WINDOW_SIZE_X) != CW_USEDEFAULT) &&
		(SETTING(MAIN_WINDOW_SIZE_Y) != CW_USEDEFAULT) ) {

		rc.left = SETTING(MAIN_WINDOW_POS_X);
		rc.top = SETTING(MAIN_WINDOW_POS_Y);
		rc.right = rc.left + SETTING(MAIN_WINDOW_SIZE_X);
		rc.bottom = rc.top + SETTING(MAIN_WINDOW_SIZE_Y);
		// Now, let's ensure we have sane values here...
		if( (rc.left < 0 ) || (rc.top < 0) || (rc.right - rc.left < 10) || ((rc.bottom - rc.top) < 10) ) {
			rc = wndMain.rcDefault;
		}
	}

	if(wndMain.CreateEx(NULL, rc) == NULL) {
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}

	if(BOOLSETTING(MINIMIZE_ON_STARTUP)) {
		wndMain.ShowWindow(SW_SHOWMINIMIZED);
	} else {
		wndMain.ShowWindow(((nCmdShow == SW_SHOWDEFAULT) || (nCmdShow == SW_SHOWNORMAL)) ? SETTING(MAIN_WINDOW_STATE) : nCmdShow);
	}
	int nRet = theLoop.Run();
	
	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{

	dcdebug("String: %d\n", sizeof(string));
#ifndef _DEBUG
	SingleInstance dcapp("{STRONGDC-AEE8350A-B49A-4753-AB4B-E55479A48351}");

	if(dcapp.IsAnotherInstanceRunning()) {
		// Allow for more than one instance...
				bool multiple = false;
		if(_tcslen(lpstrCmdLine) == 0) {
		if (::MessageBox(NULL, "There is already an instance of StrongDC++ running.\nDo you want to launch another instance anyway?", 
			APPNAME " " VERSIONSTRING CZDCVERSIONSTRING, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_TOPMOST) == IDYES) {
					multiple = true;
				}
		}

		if(multiple == false) {
		HWND hOther = NULL;
		EnumWindows(searchOtherInstance, (LPARAM)&hOther);

		if( hOther != NULL ) {
			// pop up
			::SetForegroundWindow(hOther);

			if( IsIconic(hOther)) {
				// restore
				::ShowWindow(hOther, SW_RESTORE);
			}
			sendCmdLine(hOther, lpstrCmdLine);
		}
		return FALSE;
		}
	}
#endif
	
	HRESULT hRes = ::CoInitialize(NULL);
#ifdef _DEBUG
	EXTENDEDTRACEINITIALIZE( Util::getAppPath().c_str() );
	//File::deleteFile(Util::getAppPath() + "exceptioninfo.txt");
#endif
	LPTOP_LEVEL_EXCEPTION_FILTER pOldSEHFilter = NULL;
	pOldSEHFilter = SetUnhandledExceptionFilter(&DCUnhandledExceptionFilter);
	
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	
	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);
	
	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES |
		ICC_UPDOWN_CLASS | ICC_USEREX_CLASSES);	// add flags to support other controls
	
	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));
	
	try {
		File f(Util::getAppName(), File::READ, File::OPEN);
		TigerTree tth(TigerTree::calcBlockSize(f.getSize(), 1));
		size_t n = 0;
		size_t n2 = DEBUG_BUFSIZE;
		while( (n = f.read(buf, n2)) > 0) {
			tth.update(buf, n);
			n2 = DEBUG_BUFSIZE;
		}
		tth.finalize();
		strcpy(::tth, tth.getRoot().toBase32().c_str());
	} catch(const FileException&) {
		dcdebug("Failed reading exe\n");
	}	

	HINSTANCE hInstRich = ::LoadLibrary(CRichEditCtrl::GetLibraryName());	

	int nRet = Run(lpstrCmdLine, nCmdShow);
 
	if ( hInstRich ) {
		::FreeLibrary(hInstRich);
	}
	
	// Return back old VS SEH handler
	if (pOldSEHFilter != NULL)
		SetUnhandledExceptionFilter(pOldSEHFilter);

	WSACleanup();

	_Module.Term();
	::CoUninitialize();
#ifdef _DEBUG
	EXTENDEDTRACEUNINITIALIZE();
#endif
	return nRet;
}

