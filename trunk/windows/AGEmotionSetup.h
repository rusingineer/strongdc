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

#if !defined(AGEMOTIONSETUP_H__)
#define AGEMOTIONSETUP_H__

#include "../client/Util.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// CAGEmotion

class CAGEmotion {
public:
	typedef CAGEmotion* Ptr;
	typedef vector<Ptr> List;
	typedef List::iterator Iter;

	CAGEmotion();
	virtual ~CAGEmotion();
	bool Create(string& strEmotionText, string& strEmotionBmpPath);

	const string& GetEmotionText();
	HBITMAP GetEmotionBmp();
	HBITMAP GetEmotionBmp(const COLORREF &clrBkColor);
	const string& GetEmotionBmpPath();

	const long&	GetidCommand();
	void SetidCommand(const long& idCommand);

	const long&	GetImagePos();
	void SetImagePos(const long& ImagePos);

	void SetImageList(CImageList* pImagesList);

protected:
	string		m_EmotionText;
	string		m_EmotionBmpPath;
	HBITMAP		m_EmotionBmp;

	long		m_idCommand;
	long		m_ImagePos;
	CImageList*	m_pImagesList;
};

// CAGEmotionSetup

class CAGEmotionSetup {
public:
	CAGEmotionSetup();
	virtual ~CAGEmotionSetup();

	bool Create();

	bool CreateToolbar(HWND parentWnd);

// Variables
	GETSET(bool, UseEmoticons, UseEmoticons);

// Emotion list
	CAGEmotion::List EmotionsList;

protected:
	CImageList		m_images;
	CToolBarCtrl	m_ctrlToolbar;
	TBBUTTON*		m_toolbarsStruct;
	int				m_nEmotionsCnt;
};

#endif // AGEMOTIONSETUP_H__