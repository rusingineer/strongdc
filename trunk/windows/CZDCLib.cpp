#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "../client/StringTokenizer.h"
#include "CZDCLib.h"
#include "MainFrm.h"

#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
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


