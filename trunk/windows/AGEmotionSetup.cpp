/* 
 * 
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

#ifndef AGEMOTIONSETUP_H__
	#include "AGEmotionSetup.h"
#endif

#include "../client/SimpleXML.h"
#include "../client/pointer.h"
#include <math.h>

#define AGEMOTIONSETUP_FILE "Emoticons.xml"

HBITMAP AGLoadImage(HINSTANCE hIns, LPCTSTR BmpPath, unsigned int nFlags, int x, int y, unsigned int nLoadFlags) {
	HBITMAP hBmp = (HBITMAP) ::LoadImage(hIns, BmpPath, nFlags, x, y, nLoadFlags);
	if (!hBmp) {
		char buf[512];
		_snprintf(buf, 511, "Unable to load '%s'. The program is unable to function without this file", BmpPath);
		buf[511] = 0;
		MessageBox(NULL, Text::toT(buf).c_str(), _T("Unable to load file"), MB_ICONSTOP | MB_OK);
	}
	return hBmp;
}

bool GetBitmapInfo(HBITMAP hBmp, BITMAP *pBuffer) {
	return (GetObject(hBmp, sizeof(BITMAP), pBuffer) != 0);
}

CAGEmotion::CAGEmotion() {
	m_EmotionText.empty();
	m_EmotionBmpPath.empty();
	m_EmotionBmp = NULL;
	m_idCommand = -1;
	m_ImagePos = -1;
	m_pImagesList = NULL;
}

CAGEmotion::~CAGEmotion() {
	if (m_EmotionBmp != NULL) {
		DeleteObject(m_EmotionBmp);
		m_EmotionBmp = NULL;
	}
	m_EmotionText.empty();
	m_EmotionBmpPath.empty();
	m_ImagePos = -1;
	m_pImagesList = NULL;
}

bool CAGEmotion::Create(string& strEmotionText, string& strEmotionBmpPath) {
	m_EmotionBmp = ::AGLoadImage(0, Text::toT(strEmotionBmpPath).c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	if (m_EmotionBmp == NULL) {
		dcassert(FALSE);
		return false;
	}

	m_EmotionText = strEmotionText;
	m_EmotionBmpPath = strEmotionBmpPath;
	return true;
}

const string& CAGEmotion::GetEmotionText() {
	return m_EmotionText;
}

HBITMAP CAGEmotion::GetEmotionBmp() {
	return m_EmotionBmp;
}

const long&	CAGEmotion::GetImagePos() {
	return m_ImagePos;
}

void CAGEmotion::SetImagePos(const long& ImagePos) {
	m_ImagePos = ImagePos;
}

void CAGEmotion::SetImageList(CImageList* pImagesList) {
	m_pImagesList = pImagesList;
}

HBITMAP CAGEmotion::GetEmotionBmp(const COLORREF &clrBkColor) {
	if ((m_pImagesList == NULL) || (m_ImagePos <0))
		return NULL;

	CBitmap dist;
	CClientDC dc(NULL);

	IMAGEINFO ii;
	m_pImagesList->GetImageInfo(GetImagePos(), &ii);

	int nWidth = ii.rcImage.right - ii.rcImage.left;
	int nHeight = ii.rcImage.bottom - ii.rcImage.top;

	dist.CreateCompatibleBitmap(dc, nWidth, nHeight);
	CDC memDC;
	memDC.CreateCompatibleDC(dc);
	HBITMAP pOldBitmap = (HBITMAP) SelectObject(memDC, dist);
	
	memDC.FillSolidRect(0, 0, nWidth, nHeight, clrBkColor);
	m_pImagesList->Draw(memDC, GetImagePos(), CPoint(0, 0), ILD_NORMAL);

	SelectObject(memDC, pOldBitmap);

	return (HBITMAP)dist.Detach();
}

const long&	CAGEmotion::GetidCommand() {
	return m_idCommand;
}

void CAGEmotion::SetidCommand(const long& idCommand) {
	m_idCommand = idCommand;
}

// CAGEmotionSetup

CAGEmotionSetup::CAGEmotionSetup() {
	setUseEmoticons(false);
	m_toolbarsStruct = NULL;
	m_nEmotionsCnt = 0;
}

CAGEmotionSetup::~CAGEmotionSetup() {
	for_each(EmotionsList.begin(), EmotionsList.end(), DeleteFunction<CAGEmotion*>());
	if (m_toolbarsStruct != NULL) {
		delete [] m_toolbarsStruct;
		m_toolbarsStruct = NULL;
	}
	m_nEmotionsCnt = 0;
	m_images.Destroy();
}

bool CAGEmotionSetup::Create() {
	setUseEmoticons(false);
	m_images.Destroy();
	if (m_toolbarsStruct != NULL) {
		delete [] m_toolbarsStruct;
		m_toolbarsStruct = NULL;
	}
	m_nEmotionsCnt = 0;

	if (!File::existsFile(Util::getAppPath() + AGEMOTIONSETUP_FILE))
		return true;

	int nMaxSizeCX = 0;
	int nMaxSizeCY = 0;
	
	try {
		SimpleXML xml;
		xml.fromXML(File(Util::getAppPath() + AGEMOTIONSETUP_FILE, File::READ, File::OPEN).read());
		
		if(xml.findChild("Emotions")) {
			xml.stepIn();

			string strEmotionText;
			string strEmotionBmpPath;
			while(xml.findChild("Emotion")) {
				strEmotionText = xml.getChildAttrib("ReplacedText");
				strEmotionBmpPath = xml.getChildAttrib("BitmapPath");
				if (strEmotionBmpPath.size() > 0) {
					if (strEmotionBmpPath[0] == '.') {
						// Relativni cesta - dame od applikace
						strEmotionBmpPath = Util::getAppPath() + strEmotionBmpPath;
					}
				}

				CAGEmotion* pEmotion = new CAGEmotion();
				if (!pEmotion->Create(strEmotionText, strEmotionBmpPath)) {
					dcassert(false);
					continue;
				}

				BITMAP bm;
				GetBitmapInfo(pEmotion->GetEmotionBmp(), &bm);
				if (nMaxSizeCX < bm.bmWidth)
					nMaxSizeCX = bm.bmWidth;
				if (nMaxSizeCY < bm.bmHeight)
					nMaxSizeCY = bm.bmHeight;

				EmotionsList.push_back(pEmotion);
			}
			xml.stepOut();
		}
	} catch(const Exception& e) {
		dcdebug("CAGEmotionSetup::Create: %s\n", e.getError().c_str());
		return false;
	}
	if (nMaxSizeCX == 0)	// Nejsou zadne ikony ? tak to ani pouzivat nebudeme
		return true;
	setUseEmoticons(true);

	if (!m_images.Create(nMaxSizeCX, nMaxSizeCY, ILC_COLORDDB|ILC_MASK, 0, 1)) {
		dcassert(FALSE);
		return false;
	}

	DWORD dwIdCommand = WM_USER+10000;

	int nEmotionsLineBreak = 0;
	m_nEmotionsCnt = EmotionsList.size();
	if (m_nEmotionsCnt > 4) {
		// Zkusime zarovnat na 2 odmocninu aby to pekne vypadalo
		nEmotionsLineBreak = (int) sqrt((double)m_nEmotionsCnt);
		if (nEmotionsLineBreak > 10)	// Ale kdyz je to vic nez 10, tak nechame mensi sirku
			nEmotionsLineBreak = 10;
	}

	m_toolbarsStruct = new TBBUTTON[m_nEmotionsCnt+1];
	if (m_toolbarsStruct == NULL) {
		dcassert(FALSE);
		return false;
	}

	int idx = 0;
	int nEmotionInLine = 0;

	CDC oTestDC;
	if (!oTestDC.CreateCompatibleDC(NULL))
		return FALSE;

	for(CAGEmotion::Iter pEmotion = EmotionsList.begin(); pEmotion != EmotionsList.end(); ++pEmotion) {
		HBITMAP hBmp = (*pEmotion)->GetEmotionBmp();

	 	HBITMAP poPrevSourceBmp = (HBITMAP) SelectObject(oTestDC, hBmp);
		COLORREF clrTransparent = GetPixel(oTestDC,0,0);
	 	SelectObject(oTestDC, poPrevSourceBmp);

		int nImagePos = m_images.Add(hBmp, clrTransparent);

		nEmotionInLine++;
		
		(*pEmotion)->SetidCommand(dwIdCommand);
		(*pEmotion)->SetImagePos(nImagePos);
		(*pEmotion)->SetImageList(&m_images);

		m_toolbarsStruct[idx].iBitmap = nImagePos;// zero-based index of button image
		m_toolbarsStruct[idx].idCommand = dwIdCommand;  // command to be sent when button pressed
		m_toolbarsStruct[idx].fsState = TBSTATE_ENABLED;   // button state--see below
		if (nEmotionInLine > nEmotionsLineBreak) {
			m_toolbarsStruct[idx].fsState |= TBSTATE_WRAP;
			nEmotionInLine = 0;
		}
		m_toolbarsStruct[idx].fsStyle = TBSTYLE_BUTTON;   // button style--see below
		m_toolbarsStruct[idx].dwData = 0;   // application-defined value
		m_toolbarsStruct[idx].iString = -1; // zero-based index of button label string

		dwIdCommand++;
	}

	return true;
}

bool CAGEmotionSetup::CreateToolbar(HWND parentWnd) {
	m_ctrlToolbar.Create(parentWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS, 0, ATL_IDW_TOOLBAR);
	m_ctrlToolbar.SetImageList(m_images);

	m_ctrlToolbar.SetButtonStructSize();

	m_ctrlToolbar.AddButtons(m_nEmotionsCnt, m_toolbarsStruct);

	m_ctrlToolbar.AutoSize();

	CRect frame;
	GetWindowRect(parentWnd, frame);

	return true;
}