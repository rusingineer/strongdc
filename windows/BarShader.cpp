#include "StdAfx.h"
#include "barshader.h"
#include "math.h"

#ifndef M_PI_2
#define M_PI_2     1.57079632679489661923
#endif

#ifndef PI
#define PI 3.14159265358979323846264338328
#endif

#define HALF(X) (((X) + 1) / 2)

CBarShader::CBarShader(int64_t dwHeight, int64_t dwWidth, COLORREF crColor /*= 0*/, int64_t dwFileSize /*= 1*/)
{
	m_iWidth = dwWidth;
	m_iHeight = dwHeight;
	m_dwFileSize = 0;
	m_Spans.SetAt(0, crColor);
	m_Spans.SetAt(dwFileSize, 0);
	m_pdblModifiers = NULL;
	m_bIsPreview=false;
	m_used3dlevel = 5;
	SetFileSize(dwFileSize);
}

CBarShader::~CBarShader(void)
{
	delete[] m_pdblModifiers;
}

void CBarShader::BuildModifiers()
{
	if (m_pdblModifiers != NULL)
	{
		delete[] m_pdblModifiers;
		m_pdblModifiers = NULL; // 'new' may throw an exception
	}

	static const double dDepths[5] = { 5.5, 4.0, 3.0, 2.50, 2.25 };		//aqua bar - smoother gradient jumps...
	double	depth = dDepths[((m_used3dlevel > 5) ? (256 - m_used3dlevel) : m_used3dlevel) - 1];
	int64_t  dwCount = HALF(static_cast<int64_t>(m_iHeight));
	double piOverDepth = PI/depth;
	double base = PI / 2 - piOverDepth;
	double increment = piOverDepth / (dwCount - 1);

	m_pdblModifiers = new double[dwCount];
	for (int64_t i = 0; i < dwCount; i++, base += increment)
		m_pdblModifiers[i] = sin(base);
}

void CBarShader::SetWidth(int width)
{
	if(m_iWidth != width) {
		m_iWidth = width;
		if (m_dwFileSize)
			m_dblPixelsPerByte = (double)m_iWidth / m_dwFileSize;
		else
			m_dblPixelsPerByte = 0.0;
		if (m_iWidth)
			m_dblBytesPerPixel = (double)m_dwFileSize / m_iWidth;
		else
			m_dblBytesPerPixel = 0.0;
	}
}

void CBarShader::SetFileSize(int64_t dwFileSize)
{
	if (m_dwFileSize != dwFileSize)
	{
		m_dwFileSize = dwFileSize;
		if (m_dwFileSize)
			m_dblPixelsPerByte = static_cast<double>(m_iWidth) / m_dwFileSize;
		else
			m_dblPixelsPerByte = 0.0;
		if (m_iWidth)
			m_dblBytesPerPixel = static_cast<double>(m_dwFileSize) / m_iWidth;
		else
			m_dblBytesPerPixel = 0.0;
	}
}

void CBarShader::SetHeight(int height)
{
	if(m_iHeight != height)
 	{
		m_iHeight = height;

		BuildModifiers();
	}
}

void CBarShader::FillRange(int64_t start, int64_t end, COLORREF crColor)
{
	if(end > m_dwFileSize)
		end = m_dwFileSize;

	if(start >= end)
		return;
	POSITION endprev, endpos = m_Spans.FindFirstKeyAfter(end+1);

	if ((endprev = endpos) != NULL)
		m_Spans.GetPrev(endpos);
	else
		endpos = m_Spans.GetTailPosition();

	//dcassert(endpos != NULL);

	COLORREF endcolor = m_Spans.GetValueAt(endpos);

	if ((endcolor == crColor) && (m_Spans.GetKeyAt(endpos) <= end) && (endprev != NULL))
		endpos = endprev;
	else
	endpos = m_Spans.SetAt(end, endcolor);

	for (POSITION pos = m_Spans.FindFirstKeyAfter(start); pos != endpos; )
	{
		POSITION pos1 = pos;
		m_Spans.GetNext(pos);
		m_Spans.RemoveAt(pos1);
	}
	
	m_Spans.GetPrev(endpos);

	if ((endpos == NULL) || (m_Spans.GetValueAt(endpos) != crColor))
		m_Spans.SetAt(start, crColor);
}

void CBarShader::Fill(COLORREF crColor)
{
	m_Spans.RemoveAll();
	m_Spans.SetAt(0, crColor);
	m_Spans.SetAt(m_dwFileSize, 0);
}

void CBarShader::Draw(CDC& dc, int iLeft, int iTop, int P3DDepth)
{
	m_used3dlevel = P3DDepth;
	COLORREF crLastColor = ~0, crPrevBkColor = dc.GetBkColor();
	POSITION pos = m_Spans.GetHeadPosition();
	RECT rectSpan;
	rectSpan.top = iTop;
	rectSpan.bottom = iTop + m_iHeight;
	rectSpan.right = iLeft;

	int iBytesInOnePixel = static_cast<int>(m_dblBytesPerPixel + 0.5);
	int64_t start = 0;
	COLORREF crColor = m_Spans.GetNextValue(pos);

	iLeft += m_iWidth;
	while ((pos != NULL) && (rectSpan.right < iLeft)) {
		int64_t uSpan = m_Spans.GetKeyAt(pos) - start;
		int64_t iPixels = static_cast<int64_t>(uSpan * m_dblPixelsPerByte + 0.5);

		if(iPixels > 0) {
			rectSpan.left = rectSpan.right;
			rectSpan.right += iPixels;
			FillRect(dc, &rectSpan, crLastColor = crColor);

			start += static_cast<int64_t>(iPixels * m_dblBytesPerPixel + 0.5);
		} else {
			double dRed = 0, dGreen = 0, dBlue = 0;
			int64_t dwRed, dwGreen, dwBlue, dwLast = start, dwEnd = start + iBytesInOnePixel;

			do {
				double	dblWeight = (min(m_Spans.GetKeyAt(pos), dwEnd) - dwLast) * m_dblPixelsPerByte;
				dRed   += GetRValue(crColor) * dblWeight;
				dGreen += GetGValue(crColor) * dblWeight;
				dBlue  += GetBValue(crColor) * dblWeight;
				if ((dwLast = m_Spans.GetKeyAt(pos)) >= dwEnd)
					break;
				crColor = m_Spans.GetValueAt(pos);
				m_Spans.GetNext(pos);
			} while(pos != NULL);
			rectSpan.left = rectSpan.right;
			rectSpan.right++;

		//	Saturation
			dwRed = static_cast<int64_t>(dRed);
			if (dwRed > 255)
				dwRed = 255;
			dwGreen = static_cast<int64_t>(dGreen);
			if (dwGreen > 255)
				dwGreen = 255;
			dwBlue = static_cast<int64_t>(dBlue);
			if (dwBlue > 255)
				dwBlue = 255;

			FillRect(dc, &rectSpan, crLastColor = RGB(dwRed, dwGreen, dwBlue));
			start += iBytesInOnePixel;
		}
		while((pos != NULL) && (m_Spans.GetKeyAt(pos) <= start))
			crColor = m_Spans.GetNextValue(pos);
		}
	if ((rectSpan.right < iLeft) && (crLastColor != ~0))
	{
		rectSpan.left = rectSpan.right;
		rectSpan.right = iLeft;
		FillRect(dc, &rectSpan, crLastColor);
	}
	dc.SetBkColor(crPrevBkColor);	//restore previous background color
}

void CBarShader::FillRect(CDC& dc, LPCRECT rectSpan, COLORREF crColor)
{
	if(!crColor)
		dc.FillSolidRect(rectSpan, crColor);
	else
{
		if (m_pdblModifiers == NULL)
			BuildModifiers();

		double	dblRed = GetRValue(crColor), dblGreen = GetGValue(crColor), dblBlue = GetBValue(crColor);
		double	dAdd, dMod;

		if (m_used3dlevel > 5)		//Cax2 aqua bar
		{
			dMod = 1.0 - .025 * (256 - m_used3dlevel);		//variable central darkness - from 97.5% to 87.5% of the original colour...
			dAdd = 255;

			dblRed = dMod * dblRed - dAdd;
			dblGreen = dMod * dblGreen - dAdd;
			dblBlue = dMod * dblBlue - dAdd;
		}
		else
			dAdd = 0;

		RECT		rect;
		int			iTop = rectSpan->top, iBot = rectSpan->bottom;
		double		*pdCurr = m_pdblModifiers, *pdLimit = pdCurr + HALF(m_iHeight);

		rect.right = rectSpan->right;
		rect.left = rectSpan->left;

		for (; pdCurr < pdLimit; pdCurr++)
		{
			crColor = RGB( static_cast<int>(dAdd + dblRed * *pdCurr),
				static_cast<int>(dAdd + dblGreen * *pdCurr),
				static_cast<int>(dAdd + dblBlue * *pdCurr) );
			rect.top = iTop++;
			rect.bottom = iTop;
			dc.FillSolidRect(&rect, crColor);

			rect.bottom = iBot--;
			rect.top = iBot;
		//	Fast way to fill, background color is already set inside previous FillSolidRect
			dc.FillSolidRect(&rect, crColor);
		}
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
