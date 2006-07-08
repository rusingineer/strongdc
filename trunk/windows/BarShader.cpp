#include "StdAfx.h"
#include "barshader.h"
#include "math.h"

#ifndef PI
#define PI 3.14159265358979323846264338328
#endif

#define HALF(X) (((X) + 1) / 2)

CBarShader::CBarShader(u_int32_t dwHeight, u_int32_t dwWidth, COLORREF crColor /*= 0*/, u_int64_t qwFileSize /*= 1*/)
{
	m_iWidth = dwWidth;
	m_iHeight = dwHeight;
	m_qwFileSize = 0;
	m_Spans.SetAt(0, crColor);
	m_Spans.SetAt(qwFileSize, 0);
	m_pdblModifiers = NULL;
	m_bIsPreview = false;
	m_used3dlevel = 0;
	SetFileSize(qwFileSize);
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
	u_int32_t dwCount = HALF(static_cast<u_int32_t>(m_iHeight));
	double piOverDepth = PI / depth;
	double base = PI / 2 - piOverDepth;
	double increment = piOverDepth / (dwCount - 1);

	m_pdblModifiers = new double[dwCount];
	for (u_int32_t i = 0; i < dwCount; i++, base += increment)
		m_pdblModifiers[i] = sin(base);
}

void CBarShader::SetWidth(int width)
{
	if(m_iWidth != width) {
		m_iWidth = width;
		if (m_qwFileSize)
			m_dblPixelsPerByte = (double)m_iWidth / m_qwFileSize;
		else
			m_dblPixelsPerByte = 0.0;
		if (m_iWidth)
			m_dblBytesPerPixel = (double)m_qwFileSize / m_iWidth;
		else
			m_dblBytesPerPixel = 0.0;
	}
}

void CBarShader::SetFileSize(u_int64_t qwFileSize)
{
	if (m_qwFileSize != qwFileSize)
	{
		m_qwFileSize = qwFileSize;
		if (m_qwFileSize)
			m_dblPixelsPerByte = static_cast<double>(m_iWidth) / m_qwFileSize;
		else
			m_dblPixelsPerByte = 0.0;
		if (m_iWidth)
			m_dblBytesPerPixel = static_cast<double>(m_qwFileSize) / m_iWidth;
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

void CBarShader::FillRange(u_int64_t qwStart, u_int64_t qwEnd, COLORREF crColor)
{
	if(qwEnd > m_qwFileSize)
		qwEnd = m_qwFileSize;

	if(qwStart >= qwEnd)
		return;
	POSITION endprev, endpos = m_Spans.FindFirstKeyAfter(qwEnd + 1ui64);

	if ((endprev = endpos) != NULL)
		m_Spans.GetPrev(endpos);
	else
		endpos = m_Spans.GetTailPosition();

	COLORREF endcolor = m_Spans.GetValueAt(endpos);

	if ((endcolor == crColor) && (m_Spans.GetKeyAt(endpos) <= qwEnd) && (endprev != NULL))
		endpos = endprev;
	else
	endpos = m_Spans.SetAt(qwEnd, endcolor);

	for (POSITION pos = m_Spans.FindFirstKeyAfter(qwStart); pos != endpos;)
	{
		POSITION pos1 = pos;
		m_Spans.GetNext(pos);
		m_Spans.RemoveAt(pos1);
	}
	
	m_Spans.GetPrev(endpos);

	if ((endpos == NULL) || (m_Spans.GetValueAt(endpos) != crColor))
		m_Spans.SetAt(qwStart, crColor);
}

void CBarShader::Fill(COLORREF crColor)
{
	m_Spans.RemoveAll();
	m_Spans.SetAt(0, crColor);
	m_Spans.SetAt(m_qwFileSize, 0);
}

void CBarShader::Draw(CDC& dc, int iLeft, int iTop, int P3DDepth)
{
	m_used3dlevel = (byte)P3DDepth;
	COLORREF crLastColor = (COLORREF)~0, crPrevBkColor = dc.GetBkColor();
	POSITION pos = m_Spans.GetHeadPosition();
	RECT rectSpan;
	rectSpan.top = iTop;
	rectSpan.bottom = iTop + m_iHeight;
	rectSpan.right = iLeft;

	int64_t iBytesInOnePixel = static_cast<int64_t>(m_dblBytesPerPixel + 0.5);
	u_int64_t qwStart = 0;
	COLORREF crColor = m_Spans.GetNextValue(pos);

	iLeft += m_iWidth;
	while ((pos != NULL) && (rectSpan.right < iLeft)) {
		u_int64_t qwSpan = m_Spans.GetKeyAt(pos) - qwStart;
		int iPixels = static_cast<int>(qwSpan * m_dblPixelsPerByte + 0.5);

		if(iPixels > 0) {
			rectSpan.left = rectSpan.right;
			rectSpan.right += iPixels;
			FillRect(dc, &rectSpan, crLastColor = crColor);

			qwStart += static_cast<u_int64_t>(iPixels * m_dblBytesPerPixel + 0.5);
		} else {
			double dRed = 0, dGreen = 0, dBlue = 0;
			u_int32_t dwRed, dwGreen, dwBlue; 
			u_int64_t qwLast = qwStart, qwEnd = qwStart + iBytesInOnePixel;

			do {
				double	dblWeight = (min(m_Spans.GetKeyAt(pos), qwEnd) - qwLast) * m_dblPixelsPerByte;
				dRed   += GetRValue(crColor) * dblWeight;
				dGreen += GetGValue(crColor) * dblWeight;
				dBlue  += GetBValue(crColor) * dblWeight;
				if ((qwLast = m_Spans.GetKeyAt(pos)) >= qwEnd)
					break;
				crColor = m_Spans.GetValueAt(pos);
				m_Spans.GetNext(pos);
			} while(pos != NULL);
			rectSpan.left = rectSpan.right;
			rectSpan.right++;

		//	Saturation
			dwRed = static_cast<u_int32_t>(dRed);
			if (dwRed > 255)
				dwRed = 255;
			dwGreen = static_cast<u_int32_t>(dGreen);
			if (dwGreen > 255)
				dwGreen = 255;
			dwBlue = static_cast<u_int32_t>(dBlue);
			if (dwBlue > 255)
				dwBlue = 255;

			FillRect(dc, &rectSpan, crLastColor = RGB(dwRed, dwGreen, dwBlue));
			qwStart += iBytesInOnePixel;
		}
		while((pos != NULL) && (m_Spans.GetKeyAt(pos) <= qwStart))
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
