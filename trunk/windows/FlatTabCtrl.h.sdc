/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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

#if !defined(AFX_FLATTABCTRL_H__FFFCBD5C_891D_44FB_B9F3_1DF83DA3EA83__INCLUDED_)
#define AFX_FLATTABCTRL_H__FFFCBD5C_891D_44FB_B9F3_1DF83DA3EA83__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/SettingsManager.h"
#include "../client/ResourceManager.h"

#include "WinUtil.h"
#include "resource.h"

#include "CZDCLib.h"
#include "OMenu.h"

enum {
	FT_FIRST = WM_APP + 700,
	/** This will be sent when the user presses a tab. WPARAM = HWND */
	FTM_SELECTED,
	/** The number of rows changed */
	FTM_ROWS_CHANGED,
	/** Set currently active tab to the HWND pointed by WPARAM */
	FTM_SETACTIVE,
	/** Display context menu and return TRUE, or return FALSE for the default one */
	FTM_CONTEXTMENU,

};

#ifndef WM_XBUTTONDOWN
	#define WM_XBUTTONDOWN      0x020B
#endif
#ifndef WM_APPCOMMAND
	#define WM_APPCOMMAND       0x0319
#endif
#ifndef WM_NCXBUTTONDOWN
	#define WM_NCXBUTTONDOWN    0x00AB
#endif
#ifndef MK_XBUTTON1
	#define MK_XBUTTON1         0x0020
#endif
#ifndef MK_XBUTTON2
	#define MK_XBUTTON2         0x0040
#endif
#ifndef FAPPCOMMAND_MASK
	#define APPCOMMAND_BROWSER_BACKWARD   1
	#define APPCOMMAND_BROWSER_FORWARD    2
	#define FAPPCOMMAND_MOUSE             0x8000
	#define FAPPCOMMAND_MASK              0xF000
	#define GET_APPCOMMAND_LPARAM(lParam) ((short)(HIWORD(lParam) & ~FAPPCOMMAND_MASK))
	#define GET_DEVICE_LPARAM(lParam)     ((WORD)(HIWORD(lParam) & FAPPCOMMAND_MASK))
	#define GET_FLAGS_LPARAM(lParam)      (LOWORD(lParam))
	#define GET_KEYSTATE_LPARAM(lParam)   GET_FLAGS_LPARAM(lParam)
#endif

#define TABDEFCLR -1

template <class T, class TBase = CWindow, class TWinTraits = CControlWinTraits>
class ATL_NO_VTABLE FlatTabCtrlImpl : public CWindowImpl< T, TBase, TWinTraits> {
public:

	enum { FT_EXTRA_SPACE = 18 };

	FlatTabCtrlImpl() : closing(NULL), rows(1), height(0), active(NULL), inTab(false), isInMenu(false) { 
        black.CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
		defaultgrey.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));
		facepen.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNFACE));
        white.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOW));
    };
	~FlatTabCtrlImpl() { }

	static LPCTSTR GetWndClassName()
	{
		return _T("FlatTabCtrl");
	}

	void addTab(HWND hWnd, /*COLORREF*/ int color = (int)RGB(0, 0, 0), long icon = 0, long stateIcon = 0) {
		TabInfo* i = new TabInfo(hWnd, color, icon, (stateIcon != 0) ? stateIcon : icon);
		dcassert(getTabInfo(hWnd) == NULL);
		tabs.push_back(i);
		viewOrder.push_back(hWnd);
		nextTab = --viewOrder.end();
		active = i;
		calcRows(false);
		Invalidate();
	}

	void removeTab(HWND aWnd) {
		TabInfo::ListIter i;
		for(i = tabs.begin(); i != tabs.end(); ++i) {
            if((*i)->hWnd == aWnd)
				break;
		}

		dcassert(i != tabs.end());
        TabInfo* ti = *i;
		if(active == ti)
			active = NULL;
        delete ti;
        tabs.erase(i);
		dcassert(find(viewOrder.begin(), viewOrder.end(), aWnd) != viewOrder.end());
		viewOrder.erase(find(viewOrder.begin(), viewOrder.end(), aWnd));
		nextTab = viewOrder.end();
		if(!viewOrder.empty())
			--nextTab;

        calcRows(false);
		Invalidate();
	}

	void startSwitch() {
		nextTab = --viewOrder.end();
		inTab = true;
	}

	void endSwitch() {
		inTab = false;
		if(active) 
		setTop(active->hWnd);
	}

	HWND getNext() {
		if(viewOrder.empty())
			return NULL;
		if(nextTab == viewOrder.begin()) {
			nextTab = --viewOrder.end();
		} else {
			--nextTab;
		}
		return *nextTab;
	}
	HWND getPrev() {
		if(viewOrder.empty())
			return NULL;
		nextTab++;
		if(nextTab == viewOrder.end()) {
			nextTab = viewOrder.begin();
		}
		return *nextTab;
	}

	void setActive(HWND aWnd) {
		if(!inTab)
			setTop(aWnd);

        TabInfo* ti = getTabInfo(aWnd);
        dcassert(ti != NULL);
        active = ti;
        ti->dirty = false;
		ti->flash = false;
        calcRows(false);
        Invalidate();
	}

	void setTop(HWND aWnd) {
		dcassert(find(viewOrder.begin(), viewOrder.end(), aWnd) != viewOrder.end());
		viewOrder.erase(find(viewOrder.begin(), viewOrder.end(), aWnd));
		viewOrder.push_back(aWnd);
		nextTab = --viewOrder.end();
	}

	void activateByPosition(int pos) {
		if (tabs.size() <= (unsigned int)pos || pos < 0)
			return;
		TabInfo* ti = tabs[pos];
		dcassert(ti != NULL);

		HWND hWnd = GetParent();
		if(hWnd) {
			::SendMessage(hWnd, FTM_SELECTED, (WPARAM)ti->hWnd, 0);
		}
	}

	void goTo(int steps) {
		if (steps == 0)
			return;
		if (tabs.size() < 2)
			return;
		TabInfo::ListIter i;
		for (i = tabs.begin(); i != tabs.end(); ++i) {
			if((*i) == active)
				break;
		}
		dcassert(i != tabs.end());
		if (steps > 0) {
			// Lets go forward
			for (int j = 0; j < steps; ++j) {
				++i;
				if (i == tabs.end())
					i = tabs.begin();
			}
		} else {
			// Lets go backward
			for (int j = 0; j > steps; --j) {
				if (i == tabs.begin())
					i = tabs.end();
				--i;
			}
		}
		if ((*i) == active)
			return;
		TabInfo* ti = *i;
		HWND hWnd = GetParent();
		if(hWnd) {
			::SendMessage(hWnd, FTM_SELECTED, (WPARAM)ti->hWnd, 0);
		}
	}

	void setDirty(HWND aWnd) {
        TabInfo* ti = getTabInfo(aWnd);
		if (ti == NULL)
			return;
        //dcassert(ti != NULL);
        bool inval = ti->update();
        
        if(active != ti) {
            if(!ti->dirty) {
                ti->dirty = true;
                inval = true;
	        }
	    }

		if(inval) {
            calcRows(false);
            Invalidate();
        }
    }

	void setIconState(HWND aWnd) {
		TabInfo* ti = getTabInfo(aWnd);
		if(ti != NULL) {
			ti->bState = true;
			Invalidate();
		}
	}
	void unsetIconState(HWND aWnd) {
		TabInfo* ti = getTabInfo(aWnd);
		if(ti != NULL) {
			ti->bState = false;
			Invalidate();
		}
	}

	void setFlash(HWND hWnd) {
		TabInfo* ti = getTabInfo(hWnd);
		if (ti != NULL && active != ti && !ti->flash) {
			ti->flash = true;
			Invalidate();
		}
		HWND mainframe = ::GetParent(::GetParent(hWnd));
		if (mainframe && (mainframe != ::GetForegroundWindow())) {
			FLASHWINFO fi;
			fi.cbSize = sizeof(FLASHWINFO);
			fi.hwnd = mainframe;
			fi.dwFlags = FLASHW_TRAY;
			fi.uCount = 6;
			fi.dwTimeout = 300;
			::FlashWindowEx(&fi);
		}
	}

	void setColor(HWND aWnd, COLORREF color) {
		TabInfo* ti = getTabInfo(aWnd);
		if(ti != NULL) {
			ti->pen.DeleteObject();
			ti->pen.CreatePen(PS_SOLID, 1, color);
			Invalidate();
		}
	}

    void updateText(HWND aWnd, LPCTSTR text) {
        TabInfo* ti = getTabInfo(aWnd);
		if (ti != NULL) {
			ti->updateText(text);
			calcRows(false);
			Invalidate();
		}
    }

	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_PAINT, onPaint)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, onLButtonDown)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		COMMAND_ID_HANDLER(IDC_CHEVRON, onChevron)
		COMMAND_RANGE_HANDLER(IDC_SELECT_WINDOW, IDC_SELECT_WINDOW+tabs.size(), onSelectWindow)
	END_MSG_MAP()

	LRESULT onLButtonDown(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
		int xPos = GET_X_LPARAM(lParam); 
        int yPos = GET_Y_LPARAM(lParam); 
        int row = getRows() - ((yPos / getTabHeight()) + 1);

        for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
			TabInfo* t = *i;
			if((row == t->row) && (xPos >= t->xpos) && (xPos < (t->xpos + t->getWidth())) ) {
				// Bingo, this was clicked
				HWND hWnd = GetParent();
				if(hWnd) {
					if(wParam & MK_SHIFT) ::SendMessage(t->hWnd, WM_CLOSE, 0, 0);
					else ::SendMessage(hWnd, FTM_SELECTED, (WPARAM)t->hWnd, 0);
				}
				break;
			}
		}
		return 0;
	}

	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 

        ScreenToClient(&pt); 
        int xPos = pt.x;
        int row = getRows() - ((pt.y / getTabHeight()) + 1);

		for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
			TabInfo* t = *i;
			if((row == t->row) && (xPos >= t->xpos) && (xPos < (t->xpos + t->getWidth())) ) {
				// Bingo, this was clicked, check if the owner wants to handle it...
				if(!::SendMessage(t->hWnd, FTM_CONTEXTMENU, 0, lParam)) {

				closing = t->hWnd;
				ClientToScreen(&pt);
				OMenu mnu;
				mnu.CreatePopupMenu();
				mnu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CSTRING(CLOSE));
					mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, m_hWnd);
				}
				break;
			}
		}
		return 0;
	}

	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		::SendMessage(closing, WM_CLOSE, 0, 0);
		return 0;
	}

    int getTabHeight() { return height; };
    int getHeight() { return (getRows() * getTabHeight())+1; };
    int getFill() { return 0 /*(getTabHeight() + 1) / 2*/; };

	int getRows() { return rows; };

	void calcRows(bool inval = true) {
		CRect rc;
		GetClientRect(rc);
        int r = 1;
        int w = 0;
        bool notify = false;
        bool needInval = false;

		for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
            TabInfo* ti = *i;
            if( (r != 0) && ((w + ti->getWidth() + getFill()) > rc.Width()) ) {
				if(r >= SETTING(MAX_TAB_ROWS)) {
					notify |= (rows != r);
                    rows = r;
                    r = 0;
                    chevron.EnableWindow(TRUE);
                } else {
                    r++;
                    w = 0;
                }
            } 
            ti->xpos = w;
            needInval |= (ti->row != (r-1));
            ti->row = r-1;
            w += ti->getWidth();
        }

		if(r != 0) {
			chevron.EnableWindow(FALSE);
            notify |= (rows != r);
            rows = r;
        }

		if(notify) {
            ::SendMessage(GetParent(), FTM_ROWS_CHANGED, 0, 0);
        }
        if(needInval && inval)
            Invalidate();
    }

    LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { 
        chevron.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
            BS_PUSHBUTTON , 0, IDC_CHEVRON);
        chevron.SetWindowText("»");

        mnu.CreatePopupMenu();

        CDC dc(::GetDC(m_hWnd));
        HFONT oldfont = dc.SelectFont(WinUtil::font);
        height = WinUtil::getTextHeight(dc) + 3;
		if (height < 16)
			height = 16;
        dc.SelectFont(oldfont);
        ::ReleaseDC(m_hWnd, dc);
        
        return 0;
    }

    LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) { 
        calcRows();
        SIZE sz = { LOWORD(lParam), HIWORD(lParam) };
        chevron.MoveWindow(sz.cx-14, 1, 14, getHeight());
        return 0;
    }
        
    LRESULT onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
        RECT rc;
        bool drawActive = false;
        RECT crc;
        GetClientRect(&crc);

        if(GetUpdateRect(&rc, FALSE)) {
            CPaintDC dc(m_hWnd);
            HFONT oldfont = dc.SelectFont(WinUtil::font);

            //ATLTRACE("%d, %d\n", rc.left, rc.right);
            for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
                TabInfo* t = *i;
                
                if(t->row != -1 && t->xpos < rc.right && t->xpos + t->getWidth() + getFill() >= rc.left ) {
                    if(t != active) {
                        drawTab(dc, t, t->xpos, t->row, false, t->flash);
                    } else {
                        drawActive = true;
					}
                }
            }
            HPEN oldpen = dc.SelectPen(facepen);
            for(int r = 0; r < rows; r++) {
				dc.MoveTo(rc.left, r*getTabHeight());
				dc.LineTo(rc.right, r*getTabHeight());
				dc.SelectPen(defaultgrey);
            }

			if(drawActive) {
                dcassert(active);
                drawTab(dc, active, active->xpos, active->row, true);
				dc.SelectPen(active->pen);
                int y = (rows - active->row -1) * getTabHeight();
				dc.MoveTo(active->xpos, y);
				dc.LineTo(active->xpos + active->getWidth() + getFill(), y);
            }
            dc.SelectPen(oldpen);
			dc.SelectFont(oldfont);
		}
        return 0;
	}

	LRESULT onChevron(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		while(mnu.GetMenuItemCount() > 0) {
			mnu.RemoveMenu(0, MF_BYPOSITION);
		}
		int n = 0;
		CRect rc;
		GetClientRect(&rc);
		CMenuItemInfo mi;
		mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA | MIIM_STATE;
        mi.fType = MFT_STRING | MFT_RADIOCHECK;
		
		for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
            TabInfo* ti = *i;
            if(ti->row == -1) {
				mi.dwTypeData = (LPSTR)ti->name;
                mi.dwItemData = (DWORD)ti->hWnd;
                mi.fState = MFS_ENABLED | (ti->dirty ? MFS_CHECKED : 0);
				mi.wID = IDC_SELECT_WINDOW + n;
				mnu.InsertMenuItem(n++, TRUE, &mi);
			}
		}

		POINT pt;
		chevron.GetClientRect(&rc);
		pt.x = rc.right - rc.left;
		pt.y = 0;
		chevron.ClientToScreen(&pt);
		
		mnu.TrackPopupMenu(TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		return 0;
	}

	LRESULT onSelectWindow(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		CMenuItemInfo mi;
		mi.fMask = MIIM_DATA;
		
		mnu.GetMenuItemInfo(wID, FALSE, &mi);
		HWND hWnd = GetParent();
		if(hWnd) {
			SendMessage(hWnd, FTM_SELECTED, (WPARAM)mi.dwItemData, 0);
		}
		return 0;		
	}
	bool isInMenu;

	void SwitchTo(bool next = true){
		TabInfo::ListIter i = tabs.begin();
		for(; i != tabs.end(); ++i){
			if((*i)->hWnd == active->hWnd){
				if(next){
					++i;
					if(i == tabs.end())
						i = tabs.begin();
				} else{
					if(i == tabs.begin())
						i = tabs.end();
					--i;
				}
				setActive((*i)->hWnd);
				::SetWindowPos((*i)->hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
				break;
			}
		}	
	}
private:
	class TabInfo {
	public:

		typedef vector<TabInfo*> List;
        typedef typename List::iterator ListIter;

		enum { MAX_LENGTH = 20 };

		TabInfo(HWND aWnd, /*COLORREF*/int c, long icon, long stateIcon) : hWnd(aWnd), len(0), xpos(0), row(0), dirty(false), hIcon(NULL), hStateIcon(NULL), bState(false), flash(false) { 
			if (icon != 0)
				hIcon = (HICON)LoadImage((HINSTANCE)::GetWindowLong(aWnd, GWL_HINSTANCE), MAKEINTRESOURCE(icon), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);
			if (icon != stateIcon)
				hStateIcon = (HICON)LoadImage((HINSTANCE)::GetWindowLong(aWnd, GWL_HINSTANCE), MAKEINTRESOURCE(stateIcon), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);
			pen.CreatePen(PS_SOLID, 1, (c == TABDEFCLR) ? GetSysColor(COLOR_BTNSHADOW) : (COLORREF)c);
			memset(&size, 0, sizeof(size));
			memset(&boldSize, 0, sizeof(boldSize));
			name[0] = 0;
            update();
        };
		~TabInfo() {
			if (hIcon == hStateIcon)
				DestroyIcon(hIcon);
			else {
				DestroyIcon(hIcon);
				DestroyIcon(hStateIcon);
			}
		}
		HWND hWnd;
		CPen pen;
		char name[MAX_LENGTH];
		int len;
		SIZE size;
        SIZE boldSize;
        int xpos;
        int row;
		bool dirty;
		HICON hIcon;
		HICON hStateIcon;
		bool bState;
		bool flash;

		bool update() {
            char name2[MAX_LENGTH];
			len = ::GetWindowTextLength(hWnd);
			int max_len = MAX_LENGTH;
			if (hIcon != NULL ? 1 : 0)
				max_len -= 3;
			if(len >= max_len) {
				::GetWindowText(hWnd, name2, max_len - 3);
                name2[max_len - 4] = '.';
                name2[max_len - 3] = '.';
                name2[max_len - 2] = '.';
                name2[max_len - 1] = 0;
                len = max_len - 1;
            } else {
                ::GetWindowText(hWnd, name2, max_len);
            }
            if(strcmp(name, name2) == 0) {
                return false;
            }
            strcpy(name, name2);
            CDC dc(::GetDC(hWnd));
            HFONT f = dc.SelectFont(WinUtil::font);
            dc.GetTextExtent(name, len, &size);
            dc.SelectFont(WinUtil::boldFont);
            dc.GetTextExtent(name, len, &boldSize);
            dc.SelectFont(f);
            ::ReleaseDC(hWnd, dc);
            return true;
        };

		bool updateText(LPCTSTR text) {
            len = strlen(text);
			int max_len = MAX_LENGTH;
			if ((hIcon != NULL) ? 1 : 0)
				max_len -= 3;
            if(len >= max_len) {
                ::strncpy(name, text, max_len - 3);
				name[max_len - 4] = '.';
				name[max_len - 3] = '.';
				name[max_len - 2] = '.';
				name[max_len - 1] = 0;
				len = max_len - 1;
			} else {
				strcpy(name, text);
			}
			CDC dc(::GetDC(hWnd));
			HFONT f = dc.SelectFont(WinUtil::font);
            dc.GetTextExtent(name, len, &size);
            dc.SelectFont(WinUtil::boldFont);
            dc.GetTextExtent(name, len, &boldSize);
            dc.SelectFont(f);       
            ::ReleaseDC(hWnd, dc);
            return true;
        };

        int getWidth() {
			return (dirty ? boldSize.cx : size.cx) + FT_EXTRA_SPACE + ((hIcon != NULL) ? 18 : 0);
        }
    };

	HWND closing;
	CButton chevron;
	CMenu mnu;

    int rows;
    int height;

	TabInfo* active;
	TabInfo::List tabs;
    CPen black;
    CPen defaultgrey;
	CPen white;
	CPen facepen;

	typedef list<HWND> WindowList;
	typedef WindowList::iterator WindowIter;

	WindowList viewOrder;
	WindowIter nextTab;

	bool inTab;

    TabInfo* getTabInfo(HWND aWnd) {
        for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
            if((*i)->hWnd == aWnd)
                return *i;
        }
        return NULL;
    }

	int getTabPosition(const TabInfo* tab) const {
		int i = 0;
		for (TabInfo::List::const_iterator j = tabs.begin(); j != tabs.end(); ++j) {
			++i;
			if ((*j) == tab)
				return i;
		}
		return 0;
	}

	/**
	 * Draws a tab
	 * @return The width of the tab
	 */
	void drawTab(CDC& dc, TabInfo* tab, int pos, int row, bool aActive = false, bool bFlash = false) {
		
		int ypos = (getRows() - row - 1) * getTabHeight();

		HPEN oldpen = dc.SelectPen(black);
		
		POINT p[4];
		dc.BeginPath();
		dc.MoveTo(pos, ypos);
		p[0].x = pos + tab->getWidth() + getFill();
        p[0].y = ypos;
		p[1].x = pos + tab->getWidth();
		p[1].y = ypos + getTabHeight() - (aActive ? 0 : 0);
        p[2].x = pos + getFill();
        p[2].y = ypos + getTabHeight() - (aActive ? 0 : 0);
		p[3].x = pos;
		p[3].y = ypos;
		
		dc.PolylineTo(p, 4);
		dc.CloseFigure();
		dc.EndPath();

		HBRUSH hBr = GetSysColorBrush(aActive ? COLOR_WINDOW : COLOR_BTNFACE);
		HBRUSH oldbrush = dc.SelectBrush(hBr);
		dc.FillPath();
		if (bFlash) {
			if (CZDCLib::getWinVerMajor() >= 5)
				OperaColors::FloodFill(dc, p[3].x + 1, p[3].y, p[0].x - 1, p[2].y, GetSysColor(COLOR_ACTIVECAPTION), GetSysColor(COLOR_GRADIENTACTIVECAPTION), true);
			else
				OperaColors::FloodFill(dc, p[3].x + 1, p[3].y, p[0].x - 1, p[2].y, GetSysColor(COLOR_ACTIVECAPTION));
		}

		dc.MoveTo(p[1].x + 1, p[1].y);
		//dc.LineTo(p[0].x + 1, p[0].y);
		dc.MoveTo(p[2]);
		//dc.LineTo(p[3]);
		dc.MoveTo(p[3]);
		if(!active || (tab->row != (rows - 1)) )
			dc.LineTo(p[0]);
		
		dc.SelectPen(active->pen);
		int sep_cut = aActive ? 0 : 2;
		dc.MoveTo(p[0].x, p[0].y + sep_cut);
		dc.LineTo(p[1].x, p[1].y - sep_cut);
		if (aActive) {
			dc.MoveTo(p[1]);
			dc.LineTo(p[2]);
			dc.SelectPen(white);
		}
		dc.MoveTo(p[2].x, p[2].y - sep_cut);
		dc.LineTo(p[3].x, p[3].y + sep_cut);

		dc.SelectPen(oldpen);
		dc.SelectBrush(oldbrush);

		dc.SetBkMode(TRANSPARENT);

		dc.SetTextColor(bFlash ? GetSysColor(COLOR_CAPTIONTEXT) : GetSysColor(aActive ? COLOR_WINDOWTEXT : COLOR_BTNTEXT));

		pos = pos + getFill() / 2 + FT_EXTRA_SPACE / 2;
		if (tab->hIcon != 0)
			pos += 18;
		if(tab->dirty) {
			HFONT f = dc.SelectFont(WinUtil::boldFont);
			dc.TextOut(pos, ypos + 1, tab->name, tab->len);
			dc.SelectFont(f);		
		} else {
			dc.TextOut(pos, ypos + 1, tab->name, tab->len);
		}
		if (tab->hIcon != 0) {
			DrawIconEx(dc.m_hDC, pos - 18, ypos, tab->bState ? tab->hStateIcon : tab->hIcon, 16, 16, NULL, NULL/*hBr*/, DI_NORMAL | DI_COMPAT);
		}
		int ipos = getTabPosition(tab);
		if (ipos > 0 && ipos < 10) {
			HFONT fTiny = dc.SelectFont(WinUtil::tinyFont);
			char cNum[2] = {'0' + (char)ipos, 0};
			if (tab->hIcon != 0) {
				dc.TextOut(pos - 24, ypos + 2, cNum, 1);
			} else {
				dc.TextOut(pos - 7, ypos + 2, cNum, 1);
			}
			dc.SelectFont(fTiny);
		}
	};
};

class FlatTabCtrl : public FlatTabCtrlImpl<FlatTabCtrl> {
public:
	DECLARE_FRAME_WND_CLASS_EX(GetWndClassName(), IDR_QUEUE, 0, COLOR_3DFACE);
};

template <class T, int C = TABDEFCLR, long I = 0, long I_state = 0, class TBase = CMDIWindow, class TWinTraits = CMDIChildWinTraits>
class ATL_NO_VTABLE MDITabChildWindowImpl : public CMDIChildWindowImpl<T, TBase, TWinTraits> {
public:

	MDITabChildWindowImpl() : created(false) { };
	FlatTabCtrl* getTab() { return WinUtil::tabCtrl; };

 	typedef MDITabChildWindowImpl<T, C, I, I_state, TBase, TWinTraits> thisClass;
	typedef CMDIChildWindowImpl<T, TBase, TWinTraits> baseClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_SYSCOMMAND, onSysCommand)
		MESSAGE_HANDLER(WM_FORWARDMSG, onForwardMsg)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_MDIACTIVATE, onMDIActivate)
		MESSAGE_HANDLER(WM_DESTROY, onDestroy)
		MESSAGE_HANDLER(WM_SETTEXT, onSetText)
		MESSAGE_HANDLER(WM_COMMAND, onCommand)
		MESSAGE_HANDLER_HWND(WM_INITMENUPOPUP, OMenu::onInitMenuPopup)
		MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
		MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()
		
	HWND Create(HWND hWndParent, ATL::_U_RECT rect = NULL, LPCTSTR szWindowName = NULL,
	DWORD dwStyle = 0, DWORD dwExStyle = 0,
	UINT nMenuID = 0, LPVOID lpCreateParam = NULL)
	{
		ATOM atom = T::GetWndClassInfo().Register(&m_pfnSuperWindowProc);

		if(nMenuID != 0)
#if (_ATL_VER >= 0x0700)
			m_hMenu = ::LoadMenu(ATL::_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(nMenuID));
#else //!(_ATL_VER >= 0x0700)
			m_hMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(nMenuID));
#endif //!(_ATL_VER >= 0x0700)

		dwStyle = T::GetWndStyle(dwStyle);
		dwExStyle = T::GetWndExStyle(dwExStyle);

		dwExStyle |= WS_EX_MDICHILD;   // force this one
		m_pfnSuperWindowProc = ::DefMDIChildProc;
		m_hWndMDIClient = hWndParent;
		ATLASSERT(::IsWindow(m_hWndMDIClient));

		if(rect.m_lpRect == NULL)
			rect.m_lpRect = &TBase::rcDefault;
		
		// If the currently active MDI child is maximized, we want to create this one maximized too
		ATL::CWindow wndParent = hWndParent;
		BOOL bMaximized = FALSE;

		if(MDIGetActive(&bMaximized) == NULL)
			bMaximized = BOOLSETTING(MDI_MAXIMIZED);
		
		if(bMaximized)
			wndParent.SetRedraw(FALSE);

		HWND hWnd = CFrameWindowImplBase<TBase, TWinTraits >::Create(hWndParent, rect.m_lpRect, szWindowName, dwStyle, dwExStyle, (UINT)0U, atom, lpCreateParam);

		if(bMaximized)
		{
			// Maximize and redraw everything
			if(hWnd != NULL)
				MDIMaximize(hWnd);
			wndParent.SetRedraw(TRUE);
			wndParent.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
			::SetFocus(GetMDIFrame());   // focus will be set back to this window
		}
		else if(hWnd != NULL && ::IsWindowVisible(m_hWnd) && !::IsChild(hWnd, ::GetFocus()))
		{
			::SetFocus(hWnd);
		}

		// Remove the "Next"-menu item from the system menu
		HMENU hSysMenu = ::GetSystemMenu(m_hWnd, FALSE);
		int iSysMenuLen = ::GetMenuItemCount(hSysMenu);
		if (iSysMenuLen > 7) { // 7 should be good. maybe 8 is better but who gives a ----
			RemoveMenu(hSysMenu, iSysMenuLen-1, MF_BYPOSITION);
			RemoveMenu(hSysMenu, iSysMenuLen-2, MF_BYPOSITION);
		}

		return hWnd;
	}

	// All MDI windows must have this in wtl it seems to handle ctrl-tab and so on...
	LRESULT onForwardMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		return baseClass::PreTranslateMessage((LPMSG)lParam);
	}

	LRESULT onSysCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		if(wParam == SC_NEXTWINDOW) {
			HWND next = getTab()->getNext();
			if(next != NULL) {
				MDIActivate(next);
				return 0;
			}
		} else if(wParam == SC_PREVWINDOW) {
			HWND next = getTab()->getPrev();
			if(next != NULL) {
				MDIActivate(next);
				return 0;
			}
		}
		bHandled = FALSE;
		return 0;
	}

	LRESULT onCreate(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		dcassert(getTab());
		getTab()->addTab(m_hWnd, C, I, I_state);
		created = true;
		return 0;
	}
	
	LRESULT onMDIActivate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
		dcassert(getTab());
		if((m_hWnd == (HWND)lParam))
			getTab()->setActive(m_hWnd);

		bHandled = FALSE;
		return 1;
	}
	
	LRESULT onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		dcassert(getTab());
		getTab()->removeTab(m_hWnd);

		BOOL bMaximized = FALSE;
		if(::SendMessage(m_hWndMDIClient, WM_MDIGETACTIVE, 0, (LPARAM)&bMaximized) != NULL)
			SettingsManager::getInstance()->set(SettingsManager::MDI_MAXIMIZED, (bMaximized>0));

		return 0;
	}

	LRESULT onSetText(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
		bHandled = FALSE;
		dcassert(getTab());
		if(created) {
            getTab()->updateText(m_hWnd, (LPCTSTR)lParam);
		}
		return 0;
	}

	LRESULT onKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		if(wParam == VK_CONTROL && LOWORD(lParam) == 1) {
			getTab()->startSwitch();
		}
		bHandled = FALSE;
		return 0;
	}

	LRESULT onKeyUp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		if(wParam == VK_CONTROL) {
			getTab()->endSwitch();
		}
		bHandled = FALSE;
		return 0;
		
	}

	LRESULT onCommand(UINT /* uMsg */, WPARAM wParam, LPARAM /* lParam */, BOOL& bHandled) {
		bHandled = FALSE;
		if (wParam >= 0xF000) {
			SendMessage(m_hWnd, WM_SYSCOMMAND, wParam, 0);
			bHandled = TRUE;
			return S_OK;
		}
		return S_OK;
	}

	void setDirty() {
		dcassert(getTab());
		getTab()->setDirty(m_hWnd);
	}
	void setFlash() {
		dcassert(getTab());
		getTab()->setFlash(m_hWnd);
	}
	void setTabColor(COLORREF color) {
		dcassert(getTab());
		getTab()->setColor(m_hWnd, color);
	}
	void setIconState() {
		dcassert(getTab());
		getTab()->setIconState(m_hWnd);
	}
	void unsetIconState() {
		dcassert(getTab());
		getTab()->unsetIconState(m_hWnd);
	}

private:
	bool created;
};

#endif // !defined(AFX_FLATTABCTRL_H__FFFCBD5C_891D_44FB_B9F3_1DF83DA3EA83__INCLUDED_)

/**
 * @file
 * $Id$
 */
