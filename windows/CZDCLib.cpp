#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "../client/StringTokenizer.h"
#include "MainFrm.h"

#include "CZDCLib.h"

#include <math.h>

bool CZDCLib::bIsXP = false;
bool CZDCLib::bGotXP = false; 
int CZDCLib::iWinVerMajor = -1;


bool CZDCLib::isXp() {
	if (!bGotXP) {
		OSVERSIONINFO osvi;
		osvi.dwOSVersionInfoSize = sizeof(osvi);
		if (GetVersionEx(&osvi) != 0) {
			if (osvi.dwMajorVersion > 5) {
				bIsXP = true;
			} else if (osvi.dwMajorVersion == 5) {
				bIsXP = (osvi.dwMinorVersion >= 1 && osvi.dwPlatformId == VER_PLATFORM_WIN32_NT);
			} else {
				bIsXP = false;
			}
			bGotXP = true;
		}
	}
	return bIsXP;
}

bool CZDCLib::shutDown() /* throw(ShutdownException) */ {
	// Prepare for shutdown
	UINT iForceIfHung = 0;
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	if (GetVersionEx(&osvi) != 0 && osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
		iForceIfHung = 0x00000010;
		HANDLE hToken;
		if (OpenProcessToken(GetCurrentProcess(), (TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY), &hToken) == 0) {
//			throw ShutdownException("OperaLib::shutDown()::OpenProcessToken() failed");
		}
		LUID luid;
		if (LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &luid) == 0) {
//			throw ShutdownException("OperaLib::shutDown()::LookupPrivilegeValue() failed");
		}
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		tp.Privileges[0].Luid = luid;
		AdjustTokenPrivileges(hToken, FALSE, &tp, 0, (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL);
		if (GetLastError() != ERROR_SUCCESS) {
//			throw ShutdownException("OperaLib::shutDown()::AdjustTokenPrivileges() failed");
		}
		CloseHandle(hToken);
	}
	// Shutdown
	if (ExitWindowsEx(EWX_POWEROFF | iForceIfHung, 0) == 0) {
//		throw ShutdownException("OperaLib::shutDown()::ExitWindowsEx() failed.\r\nGetLastError returned: " + Util::toString((int)GetLastError()));
		return false;
	} else {
		return true;
	}
}

#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))


int CZDCLib::getWinVerMajor() {
	if (iWinVerMajor == -1) {
		OSVERSIONINFO osvi;
		osvi.dwOSVersionInfoSize = sizeof(osvi);
		if (GetVersionEx(&osvi) != 0) {
			iWinVerMajor = osvi.dwMajorVersion;
		}
	}
	return iWinVerMajor;
}

bool isMDIChildActive(HWND hWnd) {
	HWND wnd = MainFrame::anyMF->MDIGetActive();
	dcassert(wnd != NULL);
	return (hWnd == wnd);
}

void handleMDIClick(int nID, HWND mdiWindow) {
	if (isMDIChildActive(mdiWindow)) {
		::PostMessage(mdiWindow, WM_CLOSE, NULL, NULL);
		// The following should be dealt with in the respective frame
		// setButtonPressed(nID, false);
	} else {
		CZDCLib::setButtonPressed(nID, true); 
		MainFrame::anyMF->MDIActivate(mdiWindow);
	}
}

#define MIN3(a, b, c) (((a) < (b)) ? ((((a) < (c)) ? (a) : (c))) : ((((b) < (c)) ? (b) : (c))))
#define MAX3(a, b, c) (((a) > (b)) ? ((((a) > (c)) ? (a) : (c))) : ((((b) > (c)) ? (b) : (c))))
#define CENTER(a, b, c) ((((a) < (b)) && ((a) < (c))) ? (((b) < (c)) ? (b) : (c)) : ((((b) < (a)) && ((b) < (c))) ? (((a) < (c)) ? (a) : (c)) : (((a) < (b)) ? (a) : (b))))
#define ABS(a) (((a) < 0) ? (-(a)): (a))

double OperaColors::RGB2HUE(double r, double g, double b) {
	double H;
	double m2 = MAX3(r, g, b);
	double m1 = CENTER(r, g, b);
	double m0 = MIN3(r, g, b);

	if (m2 == m1) {
		if (r == g) {
			H = 60;
			goto _RGB2HUE_END;
		}
		if (g == b) {
			H = 180;
			goto _RGB2HUE_END;
		}
		H = 60;
		goto _RGB2HUE_END;
	}
	double F = 60 * (m1 - m0) / (m2 - m0);
	if (r == m2) {
		H = 0 + F * (g - b);
		goto _RGB2HUE_END;
	}
	if (g == m2) {
		H = 120 + F * (b - r);
		goto _RGB2HUE_END;
	}
	H = 240 + F * (r - g);

_RGB2HUE_END:
	if (H < 0)
		H = H + 360;
	return H;
}

RGBTRIPLE OperaColors::HUE2RGB(double m0, double m2, double h) {
	RGBTRIPLE rgbt = {0, 0, 0};
	double m1, F;
	int n;
	while (h < 0)
		h += 360;
	while (h >= 360)
		h -= 360;
	n = (int)(h / 60);

	if (h < 60)
		F = h;
	else if (h < 180)
		F = h - 120;
	else if (h < 300)
		F = h - 240;
	else
		F = h - 360;
	m1 = m0 + (m2 - m0) * sqrt(ABS(F / 60));
	switch (n) {
		case 0: rgbt.rgbtRed = (BYTE)(255*m2); rgbt.rgbtGreen = (BYTE)(255*m1); rgbt.rgbtBlue = (BYTE)(255*m0); break;
		case 1: rgbt.rgbtRed = (BYTE)(255*m1); rgbt.rgbtGreen = (BYTE)(255*m2); rgbt.rgbtBlue = (BYTE)(255*m0); break;
		case 2: rgbt.rgbtRed = (BYTE)(255*m0); rgbt.rgbtGreen = (BYTE)(255*m2); rgbt.rgbtBlue = (BYTE)(255*m1); break;
		case 3: rgbt.rgbtRed = (BYTE)(255*m0); rgbt.rgbtGreen = (BYTE)(255*m1); rgbt.rgbtBlue = (BYTE)(255*m2); break;
		case 4: rgbt.rgbtRed = (BYTE)(255*m1); rgbt.rgbtGreen = (BYTE)(255*m0); rgbt.rgbtBlue = (BYTE)(255*m2); break;
		case 5: rgbt.rgbtRed = (BYTE)(255*m2); rgbt.rgbtGreen = (BYTE)(255*m0); rgbt.rgbtBlue = (BYTE)(255*m1); break;
	}
	return rgbt;
}

HLSTRIPLE OperaColors::RGB2HLS(BYTE red, BYTE green, BYTE blue) {
	double r = (double)red / 255;
	double g = (double)green / 255;
	double b = (double)blue / 255;
	double m0 = MIN3(r, g, b), m2 = MAX3(r, g, b), d;
	HLSTRIPLE hlst = {0, -1, -1};

	hlst.hlstLightness = (m2 + m0) / 2;
	d = (m2 - m0) / 2;
	if (hlst.hlstLightness <= 0.5) {
		if(hlst.hlstLightness == 0) hlst.hlstLightness = 0.1;
		hlst.hlstSaturation = d / hlst.hlstLightness;
	} else {
		if(hlst.hlstLightness == 1) hlst.hlstLightness = 0.99;
		hlst.hlstSaturation = d / (1 - hlst.hlstLightness);
	}
	if (hlst.hlstSaturation > 0 && hlst.hlstSaturation < 1)
		hlst.hlstHue = RGB2HUE(r, g, b);
	return hlst;
}

RGBTRIPLE OperaColors::HLS2RGB(double hue, double lightness, double saturation) {
	RGBTRIPLE rgbt = {0, 0, 0};
	double d;

	if (lightness == 1) {
		rgbt.rgbtRed = rgbt.rgbtGreen = rgbt.rgbtBlue = 255; return rgbt;
	}
	if (lightness == 0)
		return rgbt;
	if (lightness <= 0.5)
		d = saturation * lightness;
	else
		d = saturation * (1 - lightness);
	if (d == 0) {
		rgbt.rgbtRed = rgbt.rgbtGreen = rgbt.rgbtBlue = (BYTE)(lightness * 255); return rgbt;
	}
	return HUE2RGB(lightness - d, lightness + d, hue);
}

const MAX_SHADE = 44;
const SHADE_LEVEL = 45;
const int blend_vector[MAX_SHADE] = {0, 4, 8, 10, 5, 2, 0, -1, -2, -3, -5, -6, -7, -8, -7, -6, -5, -4, -3, -2, -1, 0, 
1, 2, 3, 4, 5, 6, 7, 8, 7, 6, 5, 3, 2, 1, 0, -2, -5, -10, -8, -4, 0};

inline string printHex(long l) {
	char buf[256];
	sprintf(buf, "%X", l);
	buf[255] = 0;
	return buf;
}

void OperaColors::FloodFill(CDC& hDC, int x1, int y1, int x2, int y2, COLORREF c1, COLORREF c2, bool light /* = false */) {
	if (x2 <= x1 || y2 <= y1 || x2 > 10000 || y2 > 10000)
		return;

	if (!light)
		for (int _x = x1; _x <= x2; ++_x) {
			CBrush hBr(CreateSolidBrush(blendColors(c2, c1, (double)(_x - x1) / (double)(x2 - x1))));
			CRect r(_x, y1, _x + 1, y2);
			hDC.FillRect(&r, hBr.m_hBrush);
		}
	else {
		int height = y2 - y1;
		double calc_index;
		size_t ci_1, ci_2;
		// Allocate shade-constants
		double* c = new double[height];
		// Calculate constants
		for (int i = 0; i < height; ++i) {
			calc_index = ((double)(i + 1) / height) * MAX_SHADE - 1;
			ci_1 = (size_t)floor(calc_index);
			ci_2 = (size_t)ceil(calc_index);
			c[i] = (double)(blend_vector[ci_1] + blend_vector[ci_1]) / (double)(SHADE_LEVEL * 2);
			//c*sqrt(x)/(x * x * x + 1);
		}
		for (int _x = x1; _x <= x2; ++_x) {
			COLORREF cr = blendColors(c2, c1, (double)(_x - x1) / (double)(x2 - x1));
			for (int _y = y1; _y < y2; ++_y) {
				hDC.SetPixelV(_x, _y, brightenColor(cr, c[_y - y1]));
			}
		}
		delete[] c;
	}
}
void OperaColors::FloodFill(CDC& hDC, int x1, int y1, int x2, int y2, COLORREF c) {
	CBrush hBr(CreateSolidBrush(c));
	CRect r(x1, y1, x2 + 1, y2);
	hDC.FillRect(&r, hBr.m_hBrush);
}

void OperaColors::FloodFill(CDC& hDC, int x1, int y1, int x2, int y2) {
	if (BOOLSETTING(MENUBAR_TWO_COLORS))
		FloodFill(hDC, x1, x2, y1, y2, (COLORREF)SETTING(MENUBAR_LEFT_COLOR), (COLORREF)SETTING(MENUBAR_RIGHT_COLOR), BOOLSETTING(MENUBAR_BUMPED));
	else
		hDC.FillSolidRect(x1, y1, x2-x1, y2-y1, (COLORREF)SETTING(MENUBAR_LEFT_COLOR));
}

void OperaColors::EnlightenFlood(const COLORREF& clr, COLORREF& a, COLORREF& b) {
	HLSCOLOR hls_a = ::RGB2HLS(clr);
	HLSCOLOR hls_b = hls_a;
	BYTE buf = HLS_L(hls_a);
	if (buf < 38)
		buf = 0;
	else
		buf -= 38;
	a = ::HLS2RGB(HLS(HLS_H(hls_a), buf, HLS_S(hls_a)));
	buf = HLS_L(hls_b);
	if (buf > 217)
		buf = 255;
	else
		buf += 38;
	b = ::HLS2RGB(HLS(HLS_H(hls_b), buf, HLS_S(hls_b)));
}

COLORREF OperaColors::TextFromBackground(COLORREF bg) {
	HLSTRIPLE hlst = RGB2HLS(bg);
	if (hlst.hlstLightness > 0.63)
		return RGB(0, 0, 0);
	else
		return RGB(255, 255, 255);
}

int CZDCLib::getFirstSelectedIndex(CListViewCtrl& list) {
	int items = list.GetItemCount();
	list.SetRedraw(FALSE);
	for(int i = 0; i < items; ++i) {
		if (list.GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED) {
			return i;
		}
	}
	return -1;
}

void CZDCLib::CalcTextSize(const string& text, HFONT font, LPSIZE size) {
	HDC dc = CreateCompatibleDC(NULL);
	HGDIOBJ old = SelectObject(dc, font);
	::GetTextExtentPoint32(dc, text.c_str(), MIN(text.size(), 8192), size);
	SelectObject(dc, old);
	DeleteDC(dc);
}

int CZDCLib::setButtonPressed(int nID, bool bPressed /* = true */) {
	if (nID == -1)
		return -1;
	if (!MainFrame::anyMF->ctrlToolbar.IsWindow())
		return -1;

	MainFrame::anyMF->ctrlToolbar.CheckButton(nID, bPressed);
	return 0;
}
