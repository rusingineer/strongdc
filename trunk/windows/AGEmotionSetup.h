#if !defined(AGEMOTIONSETUP_H__)
#define AGEMOTIONSETUP_H__

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