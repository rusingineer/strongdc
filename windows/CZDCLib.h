#include "../client/Util.h"

#ifndef __CZDCLIB_H
#define __CZDCLIB_H

typedef struct tagHLSTRIPLE {
	DOUBLE hlstHue;
	DOUBLE hlstLightness;
	DOUBLE hlstSaturation;
} HLSTRIPLE;

class CZDCLib {

public:
	static bool shutDown();
	static bool isXp();
	static bool isNT();
	static int getWinVerMajor();
	static bool findListChild(const vector<string>& sl, const string& s);
	static bool findListChild(const vector<string>& sl, const int64_t& i);

	static inline BYTE getRValue(const COLORREF& cr) {
		return (BYTE)(cr & 0xFF);
	}
	static inline BYTE getGValue(const COLORREF& cr) {
		return (BYTE)(((cr & 0xFF00) >> 8) & 0xFF);
	}
	static inline BYTE getBValue(const COLORREF& cr) {
		return (BYTE)(((cr & 0xFF0000) >> 16) & 0xFF);
	}
	static COLORREF blendColors(const COLORREF& cr1, const COLORREF& cr2, double balance = 0.5) {
		BYTE r1 = getRValue(cr1);
		BYTE g1 = getGValue(cr1);
		BYTE b1 = getBValue(cr1);
		BYTE r2 = getRValue(cr2);
		BYTE g2 = getGValue(cr2);
		BYTE b2 = getBValue(cr2);
		return RGB(
			(r1*(balance*2) + (r2*((1-balance)*2))) / 2,
			(g1*(balance*2) + (g2*((1-balance)*2))) / 2,
			(b1*(balance*2) + (b2*((1-balance)*2))) / 2
			);
	}

	static int getFirstSelectedIndex(CListViewCtrl& list);
	bool isMDIChildActive(HWND hWnd);
	void handleMDIClick(int nID, HWND mdiWindow);
	static int setButtonPressed(int nID, bool bPressed = true);

	#define HANDLE_MDI_CLICK(nID, cls) handleMDIClick(nID, cls::frame->m_hWnd)

	static void CalcTextSize(const string& text, HFONT font, LPSIZE size);

private:
	static bool bIsXP;
	static bool bGotXP;
	static int iWinVerMajor;
};
class OperaColors {
public:
	static inline BYTE getRValue(const COLORREF& cr) { return (BYTE)(cr & 0xFF); }
	static inline BYTE getGValue(const COLORREF& cr) { return (BYTE)(((cr & 0xFF00) >> 8) & 0xFF); }
	static inline BYTE getBValue(const COLORREF& cr) { return (BYTE)(((cr & 0xFF0000) >> 16) & 0xFF); }
	static double RGB2HUE(double red, double green, double blue);
	static RGBTRIPLE HUE2RGB(double m0, double m2, double h);
	static HLSTRIPLE RGB2HLS(BYTE red, BYTE green, BYTE blue);
	static inline HLSTRIPLE RGB2HLS(COLORREF c) { return RGB2HLS(getRValue(c), getGValue(c), getBValue(c)); }
	static RGBTRIPLE HLS2RGB(double hue, double lightness, double saturation);
	static inline RGBTRIPLE HLS2RGB(HLSTRIPLE hls) { return HLS2RGB(hls.hlstHue, hls.hlstLightness, hls.hlstSaturation); }
	static inline COLORREF RGB2REF(const RGBTRIPLE& c) { return RGB(c.rgbtRed, c.rgbtGreen, c.rgbtBlue); }
	static inline COLORREF blendColors(const COLORREF& cr1, const COLORREF& cr2, double balance = 0.5) {
		BYTE r1 = getRValue(cr1);
		BYTE g1 = getGValue(cr1);
		BYTE b1 = getBValue(cr1);
		BYTE r2 = getRValue(cr2);
		BYTE g2 = getGValue(cr2);
		BYTE b2 = getBValue(cr2);
		return RGB(
			(r1*(balance*2) + (r2*((1-balance)*2))) / 2,
			(g1*(balance*2) + (g2*((1-balance)*2))) / 2,
			(b1*(balance*2) + (b2*((1-balance)*2))) / 2
			);
	}
	static inline COLORREF brightenColor(const COLORREF& c, double brightness = 0) {
		if (brightness == 0)
			return c;
		else if (brightness > 0)
			return RGB(
				(getRValue(c)+((255-getRValue(c))*brightness)),
				(getGValue(c)+((255-getGValue(c))*brightness)),
				(getBValue(c)+((255-getBValue(c))*brightness))
				);
		else
			return RGB(
				(getRValue(c)*(1+brightness)),
				(getGValue(c)*(1+brightness)),
				(getBValue(c)*(1+brightness))
				);
	}

	struct FloodCacheItem {
		FloodCacheItem();
		virtual ~FloodCacheItem();

		struct FCIMapper {
			COLORREF c1;
			COLORREF c2;
		} mapper;

		int w;
		int h;
		HDC hDC;
		HBITMAP hBitmap;
	};

	struct fci_hash {
#if _MSC_VER < 1300 
		enum {bucket_size = 4}; 
		enum {min_buckets = 8}; 
#else
		static const size_t bucket_size = 4;
		static const size_t min_buckets = 8;
#endif // _MSC_VER == 1200
		size_t operator()(FloodCacheItem::FCIMapper __x) const { return (__x.c1 ^ __x.c2); }
		bool operator()(const FloodCacheItem::FCIMapper& a, const FloodCacheItem::FCIMapper& b) {
			return a.c1 < b.c1 && a.c2 < b.c2;
		};
	};
	struct fci_equal_to : public binary_function<FloodCacheItem::FCIMapper, FloodCacheItem::FCIMapper, bool> {
		bool operator()(const FloodCacheItem::FCIMapper& __x, const FloodCacheItem::FCIMapper& __y) const {
			return (__x.c1 == __y.c1) && (__x.c2 == __y.c2);
		}
	};

	typedef HASH_MAP<FloodCacheItem::FCIMapper, FloodCacheItem*, fci_hash, fci_equal_to> FCIMap;
	typedef FCIMap::iterator FCIIter;

	static void ClearCache(); // A _lot_ easier than to clear certain cache items
	static void ForbidCache(bool forbid = true);

	static void FloodFill(CDC& hDC, int x1, int y1, int x2, int y2, COLORREF c1, COLORREF c2, bool light = false);
	static void FloodFill(CDC& hDC, int x1, int y1, int x2, int y2, COLORREF c);
	static void FloodFill(CDC& hDC, int x1, int y1, int x2, int y2);
	static void EnlightenFlood(const COLORREF& clr, COLORREF& a, COLORREF& b);
	static COLORREF TextFromBackground(COLORREF bg);

private:
	static FCIMap flood_cache;
	static bool bCacheForbidden;
};

/*class Clipboard {
public:
	static bool setText(const string& s);
};*/

#endif // __CZDCLIB_H