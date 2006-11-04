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
#include "../client/SimpleXML.h"
#include "../client/Pointer.h"

#include "AGEmotionSetup.h"
#include <math.h>

bool CAGEmotion::Create(tstring& strEmotionText, string& strEmotionBmpPath) {
	m_EmotionBmp = (HBITMAP) ::LoadImage(0, Text::toT(strEmotionBmpPath).c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	if (m_EmotionBmp == NULL) {
		dcassert(FALSE);
		return false;
	}

	m_EmotionText = strEmotionText;
	m_EmotionBmpPath = strEmotionBmpPath;
	return true;
}

HBITMAP CAGEmotion::getEmotionBmp(const COLORREF &clrBkColor) {
	if ((m_pImagesList == NULL) || (m_ImagePos <0))
		return NULL;

	CBitmap dist;
	CClientDC dc(NULL);

	IMAGEINFO ii;
	m_pImagesList->GetImageInfo(getImagePos(), &ii);

	int nWidth = ii.rcImage.right - ii.rcImage.left;
	int nHeight = ii.rcImage.bottom - ii.rcImage.top;

	dist.CreateCompatibleBitmap(dc, nWidth, nHeight);
	CDC memDC;
	memDC.CreateCompatibleDC(dc);
	HBITMAP pOldBitmap = (HBITMAP) SelectObject(memDC, dist);
	
	memDC.FillSolidRect(0, 0, nWidth, nHeight, clrBkColor);
	m_pImagesList->Draw(memDC, getImagePos(), CPoint(0, 0), ILD_NORMAL);

	SelectObject(memDC, pOldBitmap);

	return (HBITMAP)dist.Detach();
}

// CAGEmotionSetup
CAGEmotionSetup::CAGEmotionSetup() : useEmoticons(false) { }

CAGEmotionSetup::~CAGEmotionSetup() {
	for_each(EmotionsList.begin(), EmotionsList.end(), DeleteFunction());
	m_images.Destroy();
}

bool CAGEmotionSetup::Create() {
	setUseEmoticons(false);
	m_images.Destroy();

	if((SETTING(EMOTICONS_FILE) == "Disabled") || !Util::fileExists(Util::getDataPath() + "EmoPacks\\" + SETTING(EMOTICONS_FILE) + ".xml" ))
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
					delete pEmotion;
					continue;
				}

				BITMAP bm;
				GetObject(pEmotion->getEmotionBmp(), sizeof(BITMAP), &bm);
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

	if (!m_images.Create(nMaxSizeCX, nMaxSizeCY, ILC_COLORDDB|ILC_MASK, 0, 1)) {
		dcassert(FALSE);
		return false;
	}

	CDC oTestDC;
	if (!oTestDC.CreateCompatibleDC(NULL))
		return FALSE;

	for(CAGEmotion::Iter pEmotion = EmotionsList.begin(); pEmotion != EmotionsList.end(); ++pEmotion) {
		HBITMAP hBmp = (*pEmotion)->getEmotionBmp();

	 	HBITMAP poPrevSourceBmp = (HBITMAP) SelectObject(oTestDC, hBmp);
		COLORREF clrTransparent = GetPixel(oTestDC,0,0);
	 	SelectObject(oTestDC, poPrevSourceBmp);

		int nImagePos = m_images.Add(hBmp, clrTransparent);

		(*pEmotion)->setImagePos(nImagePos);
		(*pEmotion)->setImageList(&m_images);

		DeleteObject(hBmp);
	}
	
	setUseEmoticons(true);
	return true;
}
