#pragma once
//#include "types.h"

#include "atlcoll.h"

class CBarShader
{
public:
	CBarShader(unsigned long height = 1, unsigned long width = 1);
	~CBarShader(void);

	//set the width of the bar
	void SetWidth(int width);

	//set the height of the bar
	void SetHeight(int height);

	//returns the width of the bar
	int GetWidth()
	{
		return m_iWidth;
	}

	//returns the height of the bar
	int GetHeight()
	{
		return m_iHeight;
	}

	//call this to blank the shaderwithout changing file size
	void Reset();

	//sets new file size and resets the shader
	void SetFileSize(unsigned long fileSize);

	//fills in a range with a certain color, new ranges overwrite old
	void FillRange(unsigned long m_dwStartOffset, unsigned long m_dwEndOffset, COLORREF crColor);

	//fills in entire range with a certain color
	void Fill(COLORREF crColor);

	//draws the bar
	void Draw(CDC& dc, int iLeft, int iTop, int);

protected:
	void BuildModifiers();
	void FillRect(CDC& dc, LPRECT rectSpan, double dblRed, double dblGreen, double dblBlue);
	void FillRect(CDC& dc, LPRECT rectSpan, COLORREF crColor);

	int			m_iWidth;
	int			m_iHeight;
	double		m_dblPixelsPerByte;
	double		m_dblBytesPerPixel;
	unsigned long	m_dwFileSize;
	bool		m_bIsPreview;

private:
	CRBMap<unsigned long, COLORREF> m_Spans;
	double	*m_pdblModifiers;
	byte	m_used3dlevel;
};
