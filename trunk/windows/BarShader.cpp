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

CBarShader::CBarShader(unsigned long height, unsigned long width) 
{
	m_iWidth = width;
	m_iHeight = height;
	m_dwFileSize = 1;
	m_Spans.SetAt(0, 0);
	m_pdblModifiers = NULL;
	m_bIsPreview=false;
	m_used3dlevel = 5;
}

CBarShader::~CBarShader(void) {
	delete[] m_pdblModifiers;
	m_Spans.RemoveAll();
}

void CBarShader::Reset() {
	Fill(0);
}

void CBarShader::BuildModifiers()
{
	if (m_pdblModifiers != NULL)
	{
		delete[] m_pdblModifiers;
		m_pdblModifiers = NULL; // 'new' may throw an exception
	}

	double	depths[5] = { 1.5, 3.0, 4.0, 4.50, 4.75 };		//Cax2 aqua bar - smoother gradient jumps...
	double	depth = 7 - depths[((m_used3dlevel>10)?256-m_used3dlevel:m_used3dlevel) -1];		//Cax2 aqua bar
	int count = HALF(m_iHeight);
	double piOverDepth = PI/depth;
	double base = piOverDepth * (depth / 2 - 1);
	double increment = piOverDepth / (count - 1);

	m_pdblModifiers = new double[count];
	for (int i = 0; i < count; i++)
		m_pdblModifiers[i] = static_cast<double>(sin(base + i * increment));
}

void CBarShader::SetWidth(int width) {
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

void CBarShader::SetFileSize(unsigned long dwFileSize)
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

void CBarShader::FillRange(unsigned long start, unsigned long end, COLORREF crColor)
{
	if(end > m_dwFileSize)
		end = m_dwFileSize;

	if(start >= end)
		return;
	POSITION endpos = m_Spans.FindFirstKeyAfter(end+1);

	if (endpos)
		m_Spans.GetPrev(endpos);
	else
		endpos = m_Spans.GetTailPosition();

	_ASSERT(endpos != NULL);

	COLORREF endcolor = m_Spans.GetValueAt(endpos);
	endpos = m_Spans.SetAt(end, endcolor);

	for (POSITION pos = m_Spans.FindFirstKeyAfter(start+1); pos != endpos; ) {
		POSITION pos1 = pos;
		m_Spans.GetNext(pos);
		m_Spans.RemoveAt(pos1);
	}
	
	m_Spans.GetPrev(endpos);

	if (m_Spans.GetValueAt(endpos) != crColor)
		m_Spans.SetAt(start, crColor);
}

void CBarShader::Fill(COLORREF crColor) {
	m_Spans.RemoveAll();
	m_Spans.SetAt(0, crColor);
	m_Spans.SetAt(m_dwFileSize, 0);
}

void CBarShader::Draw(CDC& dc, int iLeft, int iTop, int P3DDepth) {
	m_used3dlevel = P3DDepth;
	POSITION pos = m_Spans.GetHeadPosition();
	RECT rectSpan;
	rectSpan.top = iTop;
	rectSpan.bottom = iTop + m_iHeight;
	rectSpan.right = iLeft;

	int iBytesInOnePixel = static_cast<int>(m_dblBytesPerPixel + 0.5);
	unsigned long start = 0;
	COLORREF m_crColor = m_Spans.GetValueAt(pos);
	m_Spans.GetNext(pos);
	while(pos != NULL && rectSpan.right < (iLeft + m_iWidth)) {
		unsigned long uSpan = m_Spans.GetKeyAt(pos) - start;
		int iPixels = static_cast<int>(uSpan * m_dblPixelsPerByte + 0.5);
		if(iPixels > 0) {
			rectSpan.left = rectSpan.right;
			rectSpan.right += iPixels;
			FillRect(dc, &rectSpan, m_crColor);

			start += static_cast<int>(iPixels * m_dblBytesPerPixel + 0.5);
		} else {
			double dblRed = 0;
			double dblGreen = 0;
			double dblBlue = 0;
			unsigned long iEnd = start + iBytesInOnePixel;
			int iLast = start;
			do {
				double	dblWeight = (min(m_Spans.GetKeyAt(pos), iEnd) - iLast) * m_dblPixelsPerByte;
				dblRed   += GetRValue(m_crColor) * dblWeight;
				dblGreen += GetGValue(m_crColor) * dblWeight;
				dblBlue  += GetBValue(m_crColor) * dblWeight;
				if(m_Spans.GetKeyAt(pos) > iEnd)
					break;
				iLast = m_Spans.GetKeyAt(pos);
				m_crColor = m_Spans.GetValueAt(pos);
				m_Spans.GetNext(pos);
			} while(pos != NULL);
			rectSpan.left = rectSpan.right;
			rectSpan.right++;
			FillRect(dc, &rectSpan, dblRed, dblGreen, dblBlue);
			start += iBytesInOnePixel;
		}
		while(pos != NULL && m_Spans.GetKeyAt(pos) < start) {
			m_crColor = m_Spans.GetValueAt(pos);
			m_Spans.GetNext(pos);
		}
	}
}

void CBarShader::FillRect(CDC& dc, LPRECT rectSpan, COLORREF crColor) {
	FillRect(dc, rectSpan, GetRValue(crColor), GetGValue(crColor), GetBValue(crColor));
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CBarShader::FillRect(CDC& dc, LPRECT rectSpan, double dblRed, double dblGreen, double dblBlue) 
{
	CBrush		   *pOldBrush;

	BuildModifiers();

	RECT		rect = *rectSpan;

	int		iTop = rect.top;
	int		iBot = rect.bottom;
	int		iMax = HALF(m_iHeight);
	double	dblRed1,dblRed2,dblBlue1,dblBlue2,dblGreen1,dblGreen2;

	if (m_used3dlevel > 10)
	{
		dblBlue2 = 1.0 - .025 * (256 - m_used3dlevel);		//variable central darkness - from 97.5% to 87.5% of the original colour...

		dblRed1 = dblGreen1 = dblBlue1 = 255;
		dblRed2 = dblBlue2 * dblRed - dblRed1;
		dblGreen2 = dblBlue2 * dblGreen - dblGreen1;
		dblBlue2 = dblBlue2 * dblBlue - dblBlue1;
	} else {
		dblRed1 = dblGreen1 = dblBlue1 = .5;
		dblRed2 = dblRed;
		dblGreen2 = dblGreen;
		dblBlue2 = dblBlue;
	}
		
	for (int i = 0; i < iMax; i++) {

		CBrush		cbNew(CreateSolidBrush(RGB( static_cast<int>(dblRed1 + dblRed2*m_pdblModifiers[i]),
							   static_cast<int>(dblGreen1 + dblGreen2*m_pdblModifiers[i]),
							   static_cast<int>(dblBlue1 + dblBlue2*m_pdblModifiers[i]) )));

		pOldBrush = (CBrush*)SelectObject(dc, &cbNew);
		rect.top = iTop + i;
		rect.bottom = iTop + i + 1;
		dc.FillRect(&rect, cbNew.m_hBrush);

		rect.top = iBot - i - 1;
		rect.bottom = iBot - i;
		dc.FillRect(&rect, cbNew.m_hBrush);
		::SelectObject(dc, pOldBrush);
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
