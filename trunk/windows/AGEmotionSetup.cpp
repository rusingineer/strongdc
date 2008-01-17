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

CAGEmotion::CAGEmotion(const tstring& strEmotionText, const string& strEmotionBmpPath) : 
	m_EmotionText(strEmotionText), m_EmotionBmpPath(strEmotionBmpPath)
{
	m_EmotionBmp = (HBITMAP) ::LoadImage(0, Text::toT(strEmotionBmpPath).c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
	if(!m_EmotionBmp)
		return;
	
	BITMAP bm = { 0 };
	GetObject(m_EmotionBmp, sizeof(bm), &bm);
	
	if(bm.bmBitsPixel == 32) {
		BYTE *pBits = new BYTE[bm.bmWidth * bm.bmHeight * 4];
		GetBitmapBits(m_EmotionBmp, bm.bmWidth * bm.bmHeight * 4, pBits);
		
		// oprava alfa kanalu	
		for (int y = 0; y < bm.bmHeight; y++) {
			BYTE * pPixel = (BYTE *) pBits + bm.bmWidth * 4 * y;

			for (int x = 0; x < bm.bmWidth; x++) {
				pPixel[0] = pPixel[0] * pPixel[3] / 255; 
				pPixel[1] = pPixel[1] * pPixel[3] / 255; 
				pPixel[2] = pPixel[2] * pPixel[3] / 255; 

				pPixel += 4;
			}
		}
		SetBitmapBits(m_EmotionBmp, bm.bmWidth * bm.bmHeight * 4, pBits);
	    
		delete[] pBits;
	}
}

HBITMAP CAGEmotion::getEmotionBmp(const COLORREF &clrBkColor) {
	if(!m_EmotionBmp)
		return NULL;

	HDC DirectDC = CreateCompatibleDC(NULL);
	HDC memDC = CreateCompatibleDC(DirectDC);
	
	BITMAP bm = { 0 };
	GetObject(m_EmotionBmp, sizeof(bm), &bm);

	HBITMAP DirectBitmap = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 32, NULL);

	SelectObject(memDC, m_EmotionBmp);
	SelectObject(DirectDC, DirectBitmap);

	SetBkColor(DirectDC, clrBkColor);
	
	RECT rc = { 0, 0, bm.bmWidth, bm.bmHeight };
	ExtTextOut(DirectDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

	if(bm.bmBitsPixel == 32) {
		BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
		AlphaBlend(DirectDC, 0, 0, bm.bmWidth, bm.bmHeight, memDC, 0, 0, bm.bmWidth, bm.bmHeight, bf);
	} else {
		TransparentBlt(DirectDC, 0, 0, bm.bmWidth, bm.bmHeight, memDC, 0, 0, bm.bmWidth, bm.bmHeight, GetPixel(memDC, 0, 0));	
	}

	DeleteDC(memDC);
	DeleteDC(DirectDC);

    return DirectBitmap;
}

void CAGEmotionSetup::Load() {
	setUseEmoticons(false);

	if((SETTING(EMOTICONS_FILE) == "Disabled") || !Util::fileExists(Util::getDataPath() + "EmoPacks\\" + SETTING(EMOTICONS_FILE) + ".xml" )) {
		return;
	}

	
	try {
		SimpleXML xml;
		xml.fromXML(File(Util::getDataPath() + "EmoPacks\\" + SETTING(EMOTICONS_FILE) + ".xml", File::READ, File::OPEN).read());
		
		if(xml.findChild("Emoticons")) {
			xml.stepIn();

			while(xml.findChild("Emoticon")) {
				tstring strEmotionText = Text::toT(xml.getChildAttrib("PasteText"));
				if (strEmotionText.empty()) {
					strEmotionText = Text::toT(xml.getChildAttrib("Expression"));
				}
				
				string strEmotionBmpPath = xml.getChildAttrib("Bitmap");
				if (!strEmotionBmpPath.empty()) {
					if (strEmotionBmpPath[0] == '.') {
						// Relativni cesta - dame od aplikace
						strEmotionBmpPath = Util::getDataPath() + "EmoPacks\\" + strEmotionBmpPath;
					} else {
						strEmotionBmpPath = "EmoPacks\\" + strEmotionBmpPath;
					}
				}

				EmotionsList.push_back(new CAGEmotion(strEmotionText, strEmotionBmpPath));
			}
			xml.stepOut();
		}
	} catch(const Exception& e) {
		dcdebug("CAGEmotionSetup::Create: %s\n", e.getError().c_str());
		return;
	}
	
	setUseEmoticons(true);
}

void CAGEmotionSetup::Unload() {
	for_each(EmotionsList.begin(), EmotionsList.end(), DeleteFunction());
	EmotionsList.clear();
}
