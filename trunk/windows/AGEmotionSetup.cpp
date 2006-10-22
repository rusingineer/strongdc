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

HBITMAP AGLoadImage(HINSTANCE hIns, LPCTSTR BmpPath, unsigned int nFlags, int x, int y, unsigned int nLoadFlags) {
	HBITMAP hBmp = (HBITMAP) ::LoadImage(hIns, BmpPath, nFlags, x, y, nLoadFlags);
	/*if (!hBmp) {
		TCHAR buf[512];
		snwprintf(buf, sizeof(buf), _T("Unable to load '%s'. The program is unable to function without this file"), BmpPath);
		MessageBox(NULL, buf, _T("Unable to load file"), MB_ICONSTOP | MB_OK);
	}*/
	return hBmp;
}

bool GetBitmapInfo(HBITMAP hBmp, BITMAP *pBuffer) {
	return (GetObject(hBmp, sizeof(BITMAP), pBuffer) != 0);
}

CAGEmotion::CAGEmotion() {
	m_EmotionBmp = NULL;
	m_ImagePos = -1;
	m_pImagesList = NULL;
}

CAGEmotion::~CAGEmotion() {
	m_ImagePos = -1;
	m_pImagesList = NULL;
}

bool CAGEmotion::Create(tstring& strEmotionText, string& strEmotionBmpPath) {
	m_EmotionBmp = ::AGLoadImage(0, Text::toT(strEmotionBmpPath).c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	if (m_EmotionBmp == NULL) {
		dcassert(FALSE);
		return false;
	}

	m_EmotionText = strEmotionText;
	m_EmotionBmpPath = strEmotionBmpPath;
	return true;
}

const tstring& CAGEmotion::GetEmotionText() const {
	return m_EmotionText;
}

HBITMAP CAGEmotion::GetEmotionBmp() const {
	return m_EmotionBmp;
}

const string& CAGEmotion::GetEmotionBmpPath() const {
	return m_EmotionBmpPath;
}
	
const long&	CAGEmotion::GetImagePos() const {
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

// CAGEmotionSetup

CAGEmotionSetup::CAGEmotionSetup() {
	setUseEmoticons(false);
}

CAGEmotionSetup::~CAGEmotionSetup() {
	for_each(EmotionsList.begin(), EmotionsList.end(), DeleteFunction());
	m_images.Destroy();
}

bool CAGEmotionSetup::Create() {
	setUseEmoticons(false);
	m_images.Destroy();

	if (!Util::fileExists(Util::getDataPath() + "EmoPacks\\" + SETTING(EMOTICONS_FILE) + ".xml" ))
		return true;

	int nMaxSizeCX = 0;
	int nMaxSizeCY = 0;
	
	try {
		SimpleXML xml;
		xml.fromXML(File(Util::getDataPath() + "EmoPacks\\" + SETTING(EMOTICONS_FILE) + ".xml", File::READ, File::OPEN).read());
		
		if(xml.findChild("Emoticons")) {
			xml.stepIn();

			tstring strEmotionText;
			string strEmotionBmpPath;
			while(xml.findChild("Emoticon")) {
				strEmotionText = Text::toT(xml.getChildAttrib("PasteText"));
				if (strEmotionText.empty()) strEmotionText = Text::toT(xml.getChildAttrib("Expression"));
				strEmotionBmpPath = xml.getChildAttrib("Bitmap");
				if (strEmotionBmpPath.size() > 0) {
					if (strEmotionBmpPath[0] == '.') {
						// Relativni cesta - dame od aplikace
						strEmotionBmpPath = Util::getDataPath() + "EmoPacks\\" + strEmotionBmpPath;
					}
					else strEmotionBmpPath = "EmoPacks\\" + strEmotionBmpPath;
				}

				CAGEmotion* pEmotion = new CAGEmotion();
				if (!pEmotion->Create(strEmotionText, strEmotionBmpPath)) {
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

	CDC oTestDC;
	if (!oTestDC.CreateCompatibleDC(NULL))
		return FALSE;

	for(CAGEmotion::Iter pEmotion = EmotionsList.begin(); pEmotion != EmotionsList.end(); ++pEmotion) {
		HBITMAP hBmp = (*pEmotion)->GetEmotionBmp();

	 	HBITMAP poPrevSourceBmp = (HBITMAP) SelectObject(oTestDC, hBmp);
		COLORREF clrTransparent = GetPixel(oTestDC,0,0);
	 	SelectObject(oTestDC, poPrevSourceBmp);

		int nImagePos = m_images.Add(hBmp, clrTransparent);

		(*pEmotion)->SetImagePos(nImagePos);
		(*pEmotion)->SetImageList(&m_images);

		DeleteObject(hBmp);
	}

	return true;
}
