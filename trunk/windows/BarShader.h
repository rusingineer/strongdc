#pragma once
//#include "types.h"

#include "atlcoll.h"

class CBarShader
{
public:
	CBarShader(int64_t height = 1, int64_t width = 1);
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
	void SetFileSize(int64_t fileSize);

	//fills in a range with a certain color, new ranges overwrite old
	void FillRange(int64_t m_dwStartOffset, int64_t m_dwEndOffset, COLORREF crColor);

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
	int64_t		m_dwFileSize;
	bool		m_bIsPreview;

private:
	CRBMap<int64_t, COLORREF> m_Spans;
	double	*m_pdblModifiers;
	byte	m_used3dlevel;
};
