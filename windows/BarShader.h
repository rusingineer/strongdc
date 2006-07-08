#pragma once
#include "atlcoll.h"

class CBarShader
{
public:
	CBarShader(u_int32_t dwHeight, u_int32_t dwWidth, COLORREF crColor = 0, u_int64_t dwFileSize = 1ui64);
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

	//sets new file size and resets the shader
	void SetFileSize(u_int64_t qwFileSize);

	//fills in a range with a certain color, new ranges overwrite old
	void FillRange(u_int64_t qwStart, u_int64_t qwEnd, COLORREF crColor);

	//fills in entire range with a certain color
	void Fill(COLORREF crColor);

	//draws the bar
	void Draw(CDC& dc, int iLeft, int iTop, int);

protected:
	void BuildModifiers();
	void FillRect(CDC& dc, LPCRECT rectSpan, COLORREF crColor);

	u_int64_t		m_qwFileSize;
	int			m_iWidth;
	int			m_iHeight;
	double		m_dblPixelsPerByte;
	double		m_dblBytesPerPixel;

private:
	CRBMap<u_int64_t, COLORREF> m_Spans;
	double	*m_pdblModifiers;
	byte	m_used3dlevel;
	bool	m_bIsPreview;
};
