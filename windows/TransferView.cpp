/* 
 * Copyright (C) 2001-2004 Jacek Sieka, j_s at telia com
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
#include "Resource.h"

#include "../client/QueueManager.h"
#include "../client/QueueItem.h"

#include "WinUtil.h"
#include "TransferView.h"
#include "CZDCLib.h"
#include "MainFrm.h"

#include "BarShader.h"

int TransferView::columnIndexes[] = { COLUMN_USER, COLUMN_HUB, COLUMN_STATUS, COLUMN_TIMELEFT, COLUMN_SPEED, COLUMN_FILE, COLUMN_SIZE, COLUMN_PATH, COLUMN_IP, COLUMN_RATIO };
int TransferView::columnSizes[] = { 150, 100, 250, 75, 75, 175, 100, 200, 50, 75 };

static ResourceManager::Strings columnNames[] = { ResourceManager::USER, ResourceManager::HUB_SEGMENTS, ResourceManager::STATUS,
ResourceManager::TIME_LEFT, ResourceManager::SPEED, ResourceManager::FILENAME, ResourceManager::SIZE, ResourceManager::PATH,
ResourceManager::IP_BARE, ResourceManager::RATIO};

TransferView::~TransferView() {
	arrows.Destroy();
	states.Destroy();
}

LRESULT TransferView::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	arrows.CreateFromImage(IDB_ARROWS, 16, 3, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	states.CreateFromImage(IDB_STATE, 16, 2, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	ctrlTransfers.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_TRANSFERS);

	DWORD styles = LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | 0x00010000;
	if (BOOLSETTING(SHOW_INFOTIPS))
		styles |= LVS_EX_INFOTIP;

	ctrlTransfers.SetExtendedListViewStyle(styles);

	WinUtil::splitTokens(columnIndexes, SETTING(MAINFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(MAINFRAME_WIDTHS), COLUMN_LAST);

	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_TIMELEFT || j == COLUMN_SPEED) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlTransfers.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlTransfers.SetColumnOrderArray(COLUMN_LAST, columnIndexes);

	ctrlTransfers.SetBkColor(WinUtil::bgColor);
	ctrlTransfers.SetTextBkColor(WinUtil::bgColor);
	ctrlTransfers.SetTextColor(WinUtil::textColor);
	ctrlTransfers.setFlickerFree(WinUtil::bgBrush);

	ctrlTransfers.SetImageList(states, LVSIL_STATE); 
	ctrlTransfers.SetImageList(arrows, LVSIL_SMALL);
	ctrlTransfers.setSortColumn(COLUMN_USER);

	transferMenu.CreatePopupMenu();
	appendUserItems(transferMenu);
	transferMenu.AppendMenu(MF_STRING, IDC_FORCE, CTSTRING(FORCE_ATTEMPT));
	transferMenu.AppendMenu(MF_SEPARATOR, 0, (LPTSTR)NULL);
	transferMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES));

	previewMenu.CreatePopupMenu();
	previewMenu.AppendMenu(MF_SEPARATOR, 0, (LPTSTR)NULL);
	usercmdsMenu.CreatePopupMenu();

	transferMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)previewMenu, CTSTRING(PREVIEW_MENU));
	transferMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)usercmdsMenu, CTSTRING(SETTINGS_USER_COMMANDS));
	transferMenu.AppendMenu(MF_SEPARATOR, 0, (LPTSTR)NULL);
	transferMenu.AppendMenu(MF_STRING, IDC_MENU_SLOWDISCONNECT, CTSTRING(SETCZDC_DISCONNECTING_ENABLE));
	transferMenu.AppendMenu(MF_STRING, IDC_OFFCOMPRESS, CTSTRING(DISABLE_COMPRESSION));
	transferMenu.AppendMenu(MF_SEPARATOR, 0, (LPTSTR)NULL);
	transferMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(CLOSE_CONNECTION));
	transferMenu.SetMenuDefaultItem(IDC_PRIVATEMESSAGE);

	segmentedMenu.CreatePopupMenu();
	segmentedMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES));
	segmentedMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)previewMenu, CTSTRING(PREVIEW_MENU));
	segmentedMenu.AppendMenu(MF_STRING, IDC_MENU_SLOWDISCONNECT, CTSTRING(SETCZDC_DISCONNECTING_ENABLE));
	segmentedMenu.AppendMenu(MF_SEPARATOR, 0, (LPTSTR)NULL);
	segmentedMenu.AppendMenu(MF_STRING, IDC_CONNECT_ALL, CTSTRING(CONNECT_ALL));
	segmentedMenu.AppendMenu(MF_STRING, IDC_DISCONNECT_ALL, CTSTRING(DISCONNECT_ALL));
	segmentedMenu.AppendMenu(MF_SEPARATOR, 0, (LPTSTR)NULL);
	segmentedMenu.AppendMenu(MF_STRING, IDC_EXPAND_ALL, CTSTRING(EXPAND_ALL));
	segmentedMenu.AppendMenu(MF_STRING, IDC_COLLAPSE_ALL, CTSTRING(COLLAPSE_ALL));
	segmentedMenu.AppendMenu(MF_SEPARATOR, 0, (LPTSTR)NULL);
	segmentedMenu.AppendMenu(MF_STRING, IDC_REMOVEALL, CTSTRING(REMOVE_ALL));

	hIconCompressed = (HICON)LoadImage((HINSTANCE)::GetWindowLong(::GetParent(m_hWnd), GWL_HINSTANCE), MAKEINTRESOURCE(IDR_TRANSFER_COMPRESSED), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);

	ConnectionManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);
	UploadManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);
	HashManager::getInstance()->addListener(this);
	return 0;
}

void TransferView::prepareClose() {
	WinUtil::saveHeaderOrder(ctrlTransfers, SettingsManager::MAINFRAME_ORDER, 
		SettingsManager::MAINFRAME_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);
	
	ConnectionManager::getInstance()->removeListener(this);
	DownloadManager::getInstance()->removeListener(this);
	UploadManager::getInstance()->removeListener(this);
	SettingsManager::getInstance()->removeListener(this);
	HashManager::getInstance()->removeListener(this);
}

LRESULT TransferView::onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	RECT rc;
	GetClientRect(&rc);
	ctrlTransfers.MoveWindow(&rc);

	return 0;
}

LRESULT TransferView::onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	RECT rc;                    // client area of window 
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 

	// Get the bounding rectangle of the client area. 
	ctrlTransfers.GetClientRect(&rc);
	ctrlTransfers.ScreenToClient(&pt); 
	if (PtInRect(&rc, pt) && ctrlTransfers.GetSelectedCount() > 0) 
	{ 
		int i = -1;
		ItemInfo* itemI;
		bool bCustomMenu = false;
		if( (i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
			itemI = ctrlTransfers.getItemData(i);
			bCustomMenu = true;
	
			usercmdsMenu.InsertSeparatorFirst(STRING(SETTINGS_USER_COMMANDS));
	
			if(itemI->user != (User::Ptr)NULL)
				prepareMenu(usercmdsMenu, UserCommand::CONTEXT_CHAT, Text::toT(itemI->user->getClientAddressPort()), itemI->user->isClientOp());
		}
		ItemInfo* ii = ctrlTransfers.getItemData(ctrlTransfers.GetNextItem(-1, LVNI_SELECTED));
		WinUtil::ClearPreviewMenu(previewMenu);

		segmentedMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_UNCHECKED);
		transferMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_UNCHECKED);

		if(ii->type == ItemInfo::TYPE_DOWNLOAD && ii->file != Util::emptyStringT) {
			string ext = Util::getFileExt(Text::fromT(ii->file));
			if(ext.size()>1) ext = ext.substr(1);
			PreviewAppsSize = WinUtil::SetupPreviewMenu(previewMenu, ext);
			QueueItem* qi = ii->qi;
			if(qi && qi->getSlowDisconnect()) {
				segmentedMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_CHECKED);
				transferMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_CHECKED);
			}
		}

		if(previewMenu.GetMenuItemCount() > 0) {
			segmentedMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MF_ENABLED);
			transferMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MF_ENABLED);
		} else {
			segmentedMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MF_GRAYED);
			transferMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MF_GRAYED);
		}

		previewMenu.InsertSeparatorFirst(STRING(PREVIEW_MENU));
		ctrlTransfers.ClientToScreen(&pt);
		
		if(ii->pocetUseru <= 1) {
			transferMenu.InsertSeparatorFirst(STRING(MENU_TRANSFERS));
			transferMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
			transferMenu.RemoveFirstItem();
		} else {
			segmentedMenu.InsertSeparatorFirst(STRING(SETTINGS_SEGMENT));
			segmentedMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
			segmentedMenu.RemoveFirstItem();
		}

		if ( bCustomMenu ) {
			WinUtil::ClearPreviewMenu(usercmdsMenu);
		}
		return TRUE; 
	}
	return FALSE; 
}

void TransferView::runUserCommand(UserCommand& uc) {
	if(!WinUtil::getUCParams(m_hWnd, uc, ucParams))
		return;

	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ItemInfo* itemI = ctrlTransfers.getItemData(i);
		if(!itemI->user->isOnline())
			return;

		ucParams["mynick"] = itemI->user->getClientNick();
		ucParams["mycid"] = itemI->user->getClientCID().toBase32();
		ucParams["file"] = Text::fromT(itemI->path) + Text::fromT(itemI->file);

		StringMap tmp = ucParams;
		itemI->user->getParams(tmp);
		itemI->user->clientEscapeParams(tmp);
		itemI->user->sendUserCmd(Util::formatParams(uc.getCommand(), tmp));
	}
	return;
};

LRESULT TransferView::onForce(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ctrlTransfers.SetItemText(i, COLUMN_STATUS, CTSTRING(CONNECTING_FORCED));
		ctrlTransfers.getItemData(i)->user->connect();
	}
	return 0;
}

void TransferView::ItemInfo::removeAll() {
	if(pocetUseru == 1) {
		QueueManager::getInstance()->removeSources(user, QueueItem::Source::FLAG_REMOVED);
	} else {
		if(!BOOLSETTING(CONFIRM_DELETE) || ::MessageBox(0, _T("Do you really want to remove this item?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
			QueueManager::getInstance()->remove(Text::fromT(Target));
	}
}

LRESULT TransferView::onOffCompress(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ctrlTransfers.getItemData(i)->disconnect();
		ctrlTransfers.SetItemText(i, COLUMN_STATUS, _T("Vypínám kompresi..."));	
		
		ctrlTransfers.getItemData(i)->user->setFlag(User::FORCEZOFF);
		ctrlTransfers.getItemData(i)->user->connect();

	}
	return 0;
}

LRESULT TransferView::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	CRect rc;
    NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)pnmh;

	if (!BOOLSETTING(SHOW_PROGRESS_BARS)) {
		if (
			(cd->nmcd.dwDrawStage == (CDDS_SUBITEM | CDDS_ITEMPREPAINT)) && 
			(cd->iSubItem == COLUMN_FILE || cd->iSubItem == COLUMN_SIZE || cd->iSubItem == COLUMN_PATH)
			) {
		} else {
			bHandled = FALSE;
			return 0;
		}
	}

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREERASE:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREERASE:
		if(cd->iSubItem == COLUMN_STATUS) {
			ValidateRect(&cd->nmcd.rc);
			return CDRF_SKIPDEFAULT;
		}
		return CDRF_DODEFAULT;
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
		// Let's draw a box if needed...
		if(cd->iSubItem == COLUMN_STATUS) {
			ItemInfo* ii = (ItemInfo*)cd->nmcd.lItemlParam;
			if(ii->status == ItemInfo::STATUS_RUNNING) {
				// Get the color of this bar
				COLORREF clr = SETTING(PROGRESS_OVERRIDE_COLORS) ? 
					((ii->type == ItemInfo::TYPE_DOWNLOAD) ? 
						SETTING(DOWNLOAD_BAR_COLOR) : 
						SETTING(UPLOAD_BAR_COLOR)) : 
					GetSysColor(COLOR_HIGHLIGHT);

				// Get the text to draw
				TCHAR buf[256];
				ctrlTransfers.GetItemText((int)cd->nmcd.dwItemSpec, COLUMN_STATUS, buf, 255);
				buf[255] = 0;
				
				// Get the cell boundaries
				ctrlTransfers.GetSubItemRect((int)cd->nmcd.dwItemSpec, COLUMN_STATUS, LVIR_BOUNDS, rc);
				// Actually we steal one upper pixel to not have 2 borders together

				// Real rc, the original one.
				CRect real_rc = rc;
				// We need to offset the current rc to (0, 0) to paint on the New dc
				rc.MoveToXY(0, 0);

				// Text rect
				CRect rc2 = rc;
                rc2.left += 6; // indented with 6 pixels
				rc2.right -= 2; // and without messing with the border of the cell				

				if (ii->isSet(ItemInfo::FLAG_COMPRESSED))
					rc2.left += 9;
				
				CDC cdc;
				cdc.CreateCompatibleDC(cd->nmcd.hdc);
				BITMAPINFOHEADER bih;
				bih.biSize = sizeof(BITMAPINFOHEADER);
				bih.biWidth = real_rc.Width();
				bih.biHeight = -real_rc.Height(); // kanske minus
				bih.biPlanes = 1;
				bih.biBitCount = 32;
				bih.biCompression = BI_RGB;
				bih.biSizeImage = 0;
				bih.biXPelsPerMeter = 0;
				bih.biYPelsPerMeter = 0;
				bih.biClrUsed = 32;
				bih.biClrImportant = 0;
				HBITMAP hBmp = CreateDIBitmap(cd->nmcd.hdc, &bih, 0, NULL, NULL, DIB_RGB_COLORS);

				HBITMAP pOldBmp = cdc.SelectBitmap(hBmp);
				HDC& dc = cdc.m_hDC;

				HFONT oldFont = (HFONT)SelectObject(dc, WinUtil::font);
				SetBkMode(dc, TRANSPARENT);
		
				// Set the text color
				COLORREF hTextDefColor = cd->clrText; // Even tho we "change" background, we don't change it very much

				// Draw the text over the entire item
                COLORREF oldcol = ::SetTextColor(dc, hTextDefColor);

				// Draw the background and border of the bar
				if(ii->size == 0)
					ii->size = 1;

				CBarShader statusBar(rc.bottom - rc.top, rc.right - rc.left);
				statusBar.SetFileSize(ii->size);
				statusBar.Fill(SETTING(PROGRESS_BACK_COLOR));

				if((ii->mainItem) || (ii->type == ItemInfo::TYPE_UPLOAD)) {
					rc.right = rc.left + (int) (rc.Width() * ii->pos / ii->size); 
					if(ii->type == ItemInfo::TYPE_UPLOAD) {
						statusBar.FillRange(0, ii->start, HLS_TRANSFORM(clr, -20, 30));
						statusBar.FillRange(ii->start, ii->actual,  clr);
					} else
						statusBar.FillRange(0, ii->actual, clr);
					if(ii->pos > ii->actual)
						statusBar.FillRange(ii->actual, ii->pos, SETTING(PROGRESS_COMPRESS_COLOR));
				} else {
					rc.right = rc.left + (int) (rc.Width() * ii->pos / ii->size); 
					statusBar.FillRange(0, ii->start, clr);
					statusBar.FillRange(ii->start, ii->actual, SETTING(PROGRESS_SEGMENT_COLOR));
					if(ii->pos > ii->actual)
						statusBar.FillRange(ii->actual, ii->pos, SETTING(PROGRESS_COMPRESS_COLOR));
				}
				statusBar.Draw(cdc, rc.top, rc.left, SETTING(PROGRESS_3DDEPTH));

				// Draw the text only over the bar and with correct color
				int right = rc2.right;
				rc2.right = rc.right + 1;
				::SetTextColor(dc, OperaColors::TextFromBackground(clr));
                ::DrawText(dc, buf, _tcslen(buf), rc2, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
		
				if (ii->isSet(ItemInfo::FLAG_COMPRESSED))
					DrawIconEx(dc, rc2.left - 12, rc2.top + 2, hIconCompressed, 16, 16, NULL, NULL, DI_NORMAL | DI_COMPAT);

				// Get the color of this text bar
				int textcolor = SETTING(PROGRESS_OVERRIDE_COLORS2) ? 
					((ii->type == ItemInfo::TYPE_DOWNLOAD) ? 
						SETTING(PROGRESS_TEXT_COLOR_DOWN) : 
						SETTING(PROGRESS_TEXT_COLOR_UP)) : 
					OperaColors::TextFromBackground(clr); //GetSysColor(COLOR_HIGHLIGHT);
				::SetTextColor(dc, textcolor);
				rc2.right = right;
                ::DrawText(dc, buf, _tcslen(buf), rc2, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);

				SelectObject(dc, oldFont);
				::SetTextColor(dc, oldcol);

				// New way:
				BitBlt(cd->nmcd.hdc, real_rc.left, real_rc.top, real_rc.Width(), real_rc.Height(), dc, 0, 0, SRCCOPY);
				DeleteObject(cdc.SelectBitmap(pOldBmp));

				return CDRF_SKIPDEFAULT;
			}
		} else if(cd->iSubItem == COLUMN_IP && BOOLSETTING(GET_USER_COUNTRY)) {
			ItemInfo* ii = (ItemInfo*)cd->nmcd.lItemlParam;

			if(ctrlTransfers.GetItemState((int)cd->nmcd.dwItemSpec, LVIS_SELECTED) & LVIS_SELECTED) {
				if(ctrlTransfers.m_hWnd == ::GetFocus()) {
					barva = GetSysColor(COLOR_HIGHLIGHT);
					SetBkColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHT));
					SetTextColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
				} else {
					barva = GetBkColor(cd->nmcd.hdc);
					SetBkColor(cd->nmcd.hdc, barva);
				}				
			} else {
				barva = WinUtil::bgColor;
				SetBkColor(cd->nmcd.hdc, WinUtil::bgColor);
				SetTextColor(cd->nmcd.hdc, WinUtil::textColor);
			}

			ctrlTransfers.GetSubItemRect((int)cd->nmcd.dwItemSpec, COLUMN_IP, LVIR_BOUNDS, rc);
			CRect rc2 = rc;
			rc2.left += 2;

			HGDIOBJ oldpen = ::SelectObject(cd->nmcd.hdc, CreatePen(PS_SOLID,0, barva));
			HGDIOBJ oldbr = ::SelectObject(cd->nmcd.hdc, CreateSolidBrush(barva));
			Rectangle(cd->nmcd.hdc,rc.left, rc.top, rc.right, rc.bottom);

			DeleteObject(::SelectObject(cd->nmcd.hdc, oldpen));
			DeleteObject(::SelectObject(cd->nmcd.hdc, oldbr));

			TCHAR buf[256];
			ctrlTransfers.GetItemText((int)cd->nmcd.dwItemSpec, COLUMN_IP, buf, 255);
			buf[255] = 0;
			if(_tcslen(buf) > 0) {
				LONG top = rc2.top + (rc2.Height() - 15)/2;
				if((top - rc2.top) < 2)
					top = rc2.top + 1;

				POINT p = { rc2.left, top };
				WinUtil::flagImages.Draw(cd->nmcd.hdc, ii->flagImage, p, LVSIL_SMALL);
				top = rc2.top + (rc2.Height() - WinUtil::getTextHeight(cd->nmcd.hdc) - 1)/2;
				::ExtTextOut(cd->nmcd.hdc, rc2.left + 30, top + 1, ETO_CLIPPED, rc2, buf, _tcslen(buf), NULL);
				return CDRF_NEWFONT | CDRF_SKIPDEFAULT;
			}
		} else if (cd->iSubItem == COLUMN_FILE || cd->iSubItem == COLUMN_SIZE || cd->iSubItem == COLUMN_PATH) {
			ItemInfo* ii = (ItemInfo*)cd->nmcd.lItemlParam;
			if (ii->type == ItemInfo::TYPE_DOWNLOAD && ii->status != ItemInfo::STATUS_RUNNING) {
				cd->clrText = OperaColors::blendColors(WinUtil::bgColor, WinUtil::textColor, 0.6);
			return CDRF_NEWFONT;
		}
	}
	// Fall through
	default:
		return CDRF_DODEFAULT;
	}
}

LRESULT TransferView::onDoubleClickTransfers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;
	if (item->iItem != -1) {		
		CRect rect;
		ctrlTransfers.GetItemRect(item->iItem, rect, LVIR_ICON);

		// if double click on state icon, ignore...
		if (item->ptAction.x < rect.left)
			return 0;

		ItemInfo* i = ctrlTransfers.getItemData(item->iItem);
		if(i->pocetUseru == 1) {
			i->pm();
		}
	}
	return 0;
}

LRESULT TransferView::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(wParam == ADD_ITEM) {
		ItemInfo* i = (ItemInfo*)lParam;
		if(i->type == ItemInfo::TYPE_DOWNLOAD) {
			InsertItem(i, true); 
		} else {
			ctrlTransfers.insertItem(ctrlTransfers.GetItemCount(), i, IMAGE_UPLOAD);
		}
	} else if(wParam == REMOVE_ITEM) {
		ItemInfo* i = (ItemInfo*)lParam;
		ctrlTransfers.deleteItem(i);
		delete i;
	} else if(wParam == UPDATE_ITEM) {
		ItemInfo* i = (ItemInfo*)lParam;
		i->update();
		if((i->upper != NULL) && (i->type == ItemInfo::TYPE_DOWNLOAD))
			ctrlTransfers.updateItem(i->upper);
		ctrlTransfers.updateItem(i);
		ctrlTransfers.resort();
	} else if(wParam == UPDATE_ITEMS) {
		vector<ItemInfo*>* v = (vector<ItemInfo*>*)lParam;
		ctrlTransfers.SetRedraw(FALSE);
		for(vector<ItemInfo*>::iterator j = v->begin(); j != v->end(); ++j) {
			ItemInfo* i = *j;
			i->update();
			if((i->upper != NULL) && (i->type == ItemInfo::TYPE_DOWNLOAD))
				ctrlTransfers.updateItem(i->upper);
			ctrlTransfers.updateItem(i);
		}

		if((ctrlTransfers.getSortColumn() == COLUMN_TIMELEFT) ||
		  (ctrlTransfers.getSortColumn() == COLUMN_SPEED) ||
		  (ctrlTransfers.getSortColumn() == COLUMN_RATIO)) {
			ctrlTransfers.resort();
		}
		ctrlTransfers.SetRedraw(TRUE);
		
		delete v;
	} else if(wParam == SET_STATE) {
		ItemInfo* i = (ItemInfo*)lParam;
		ctrlTransfers.insertItem(ctrlTransfers.GetItemCount(), i, IMAGE_DOWNLOAD);
	} else if(wParam == UNSET_STATE) {
		ItemInfo* i = (ItemInfo*)lParam;
		ctrlTransfers.SetItemState(ctrlTransfers.findItem(i), INDEXTOSTATEIMAGEMASK(0), LVIS_STATEIMAGEMASK);
	} else if(wParam == REMOVE_ITEM_BUT_NOT_FREE) {
		ItemInfo* i = (ItemInfo*)lParam;
		ctrlTransfers.deleteItem(i);
	} else if(wParam == INSERT_SUBITEM) {
		ItemInfo* i = (ItemInfo*)lParam;
		int r = -1;

		if(i->upper != NULL)
			r = ctrlTransfers.findItem(i->upper);

		if(r != -1) {
			if(!i->upper->collapsed) {
				int position = 0;
				if(i->qi) {
					position = i->qi->getActiveSegments().size();
				}
				insertSubItem(i,r + position + 1);
				if(ctrlTransfers.getSortColumn() != COLUMN_STATUS)
					ctrlTransfers.resort();
			}

			if(i->upper->pocetUseru > 1) {
				ctrlTransfers.SetItemState(r, INDEXTOSTATEIMAGEMASK((i->upper->collapsed) ? 1 : 2), LVIS_STATEIMAGEMASK);
			}
		}
	}

	return 0;
}

bool TransferView::ItemInfo::canDisplayUpper() {
	if((statusString == TSTRING(DOWNLOAD_FINISHED_IDLE)) ||
		(statusString == TSTRING(UPLOAD_FINISHED_IDLE)) ||
   		(statusString.substr(1,10) == TSTRING(DOWNLOAD_CORRUPTED).substr(1,10)) ||
   		(statusString == TSTRING(TTH_INCONSISTENCY)) ||
   		(statusString.substr(1, TSTRING(CHECKING_TTH).size()) == TSTRING(CHECKING_TTH)) ||
   		(statusString == TSTRING(DISCONNECTED)) ||
   		(statusString == TSTRING(SFV_INCONSISTENCY)))
			return true;

	if(qi) {
		if((qi->isSet(QueueItem::FLAG_USER_LIST)) ||
			(qi->isSet(QueueItem::FLAG_TESTSUR)))
			return true;
 	}
  	return false;
}

void TransferView::ItemInfo::update() {
	u_int32_t colMask = updateMask;
	updateMask = 0;

	if(colMask & MASK_USER) {
		if(!mainItem || (pocetUseru == 1) || type == TYPE_UPLOAD) columns[COLUMN_USER] = Text::toT(user->getNick());
			else columns[COLUMN_USER] = Text::toT(Util::toString(pocetUseru)+" "+STRING(HUB_USERS));
		if((type == TYPE_DOWNLOAD) && (upper != NULL)) {
			upper->user = user;
			if(upper->pocetUseru > 1) {
				upper->columns[COLUMN_USER] = Text::toT(Util::toString(upper->pocetUseru)+" "+STRING(HUB_USERS));
			} else {
				upper->columns[COLUMN_USER] = Text::toT(user->getNick());
			}
		}
	}
	if(colMask & MASK_HUB) {
		if(!mainItem || type == TYPE_UPLOAD) columns[COLUMN_HUB] = Text::toT(user->getClientName());

		if((type == TYPE_DOWNLOAD) && (upper != NULL)) {
			if(upper->pocetUseru > 1) {
				int pocetSegmentu = qi ? qi->getActiveSegments().size() : 0;
				upper->columns[COLUMN_HUB] = Text::toT(Util::toString(pocetSegmentu)+" "+STRING(NUMBER_OF_SEGMENTS));
			} else {
				upper->columns[COLUMN_HUB] = Text::toT(user->getClientName());
			}
		}			
	}
	if(colMask & MASK_STATUS) {
		columns[COLUMN_STATUS] = statusString;
		if((type == TYPE_DOWNLOAD) && (!mainItem) && (upper != NULL)) {
			if(!upper->finished) {				
				if((statusString != TSTRING(ALL_SEGMENTS_TAKEN)) && (statusString != TSTRING(NO_FREE_BLOCK)) && !((status != STATUS_RUNNING) && (statusString.substr(1,10) == TSTRING(DOWNLOADED_BYTES).substr(1,10)))) {
					upper->columns[COLUMN_STATUS] = upper->statusString;
				}
			} else if(canDisplayUpper()) {
				upper->columns[COLUMN_STATUS] = upper->statusString;
			}
		}
	}
	if (status == STATUS_RUNNING) {
		if(colMask & MASK_TIMELEFT) {
			columns[COLUMN_TIMELEFT] = Text::toT(Util::formatSeconds(timeLeft));
			if((type == TYPE_DOWNLOAD) && (!mainItem) && (upper != NULL)) {
				upper->columns[COLUMN_TIMELEFT] = Text::toT(Util::formatSeconds(timeLeft));
			}
		}
		if(colMask & MASK_SPEED) {
			columns[COLUMN_SPEED] = Text::toT(Util::formatBytes(speed) + "/s");
			if((type == TYPE_DOWNLOAD) && (!mainItem) && (upper != NULL)) {
				upper->columns[COLUMN_SPEED] = Text::toT(Util::formatBytes(celkovaRychlost) + "/s");
				upper->speed = celkovaRychlost;
			}
		}
		if(colMask & MASK_RATIO) {
			columns[COLUMN_RATIO] = Text::toT(Util::toString(getRatio()));
			if((type == TYPE_DOWNLOAD) && (!mainItem) && (upper != NULL)) {
				upper->columns[COLUMN_RATIO] = Text::toT(Util::toString(upper->compressRatio));
			}
		}
	} else {
		columns[COLUMN_TIMELEFT] = Util::emptyStringT;
		columns[COLUMN_SPEED] = Util::emptyStringT;
		columns[COLUMN_RATIO] = Util::emptyStringT;
		if((upper != NULL) && (upper->status != STATUS_RUNNING)) {
			upper->columns[COLUMN_TIMELEFT] = Util::emptyStringT;
			upper->columns[COLUMN_SPEED] = Util::emptyStringT;
			upper->columns[COLUMN_RATIO] = Util::emptyStringT;
		}
	}
	if(colMask & MASK_FILE) {
		columns[COLUMN_FILE] = file;
		if((type == TYPE_DOWNLOAD) && (!mainItem) && (upper != NULL)) {
			upper->columns[COLUMN_FILE] = file;
		}
	}
	if(colMask & MASK_SIZE) {
		columns[COLUMN_SIZE] = Text::toT(Util::formatBytes(size));
		if((type == TYPE_DOWNLOAD) && (!mainItem) && (upper != NULL) && (size != -1)) {
			upper->columns[COLUMN_SIZE] = Text::toT(Util::formatBytes(size));
		}
	}
	if(colMask & MASK_PATH) {
		columns[COLUMN_PATH] = path;
		if((type == TYPE_DOWNLOAD) && (!mainItem) && (upper != NULL)) {
			upper->columns[COLUMN_PATH] = path;
		}
	}
	if(colMask & MASK_IP) {
		tstring countryIP;
		if (country == _T(""))
			countryIP = IP;
		else if (IP == _T(""))
			countryIP = country;
		else
			countryIP = country + _T(" (") + IP + _T(")");
		columns[COLUMN_IP] = countryIP;
		if((type == TYPE_DOWNLOAD) && (!mainItem) && (upper != NULL) && (upper->pocetUseru <= 1)) {
			upper->flagImage = flagImage;
			upper->columns[COLUMN_IP] = countryIP;
		}
	}
}

void TransferView::on(ConnectionManagerListener::Added, ConnectionQueueItem* aCqi) {
	ItemInfo::Types t = aCqi->getConnection() && aCqi->getConnection()->isSet(UserConnection::FLAG_UPLOAD) ? ItemInfo::TYPE_UPLOAD : ItemInfo::TYPE_DOWNLOAD;
	ItemInfo* i = new ItemInfo(aCqi->getUser(), t, ItemInfo::STATUS_WAITING);

	{
		Lock l(cs);
		dcassert(transferItems.find(aCqi) == transferItems.end());
		transferItems.insert(make_pair(aCqi, i));
		i->columns[COLUMN_STATUS] = i->statusString = TSTRING(CONNECTING);
		if (t == ItemInfo::TYPE_DOWNLOAD) {
			QueueItem* qi = QueueManager::getInstance()->lookupNext(aCqi->getUser());
			if (qi) {
				i->columns[COLUMN_FILE] = Text::toT(Util::getFileName(qi->getTarget()));
				i->columns[COLUMN_PATH] = Text::toT(Util::getFilePath(qi->getTarget()));
				i->columns[COLUMN_SIZE] = Text::toT(Util::formatBytes(qi->getSize()));
				i->size = qi->getSize();
				i->Target = Text::toT(qi->getTarget());
				i->qi = qi;
				i->downloadTarget = Text::toT(qi->getTempTarget());
 			}
		}
	}

	PostMessage(WM_SPEAKER, ADD_ITEM, (LPARAM)i);
}

void TransferView::on(ConnectionManagerListener::StatusChanged, ConnectionQueueItem* aCqi) {
	ItemInfo* i;
	{
		Lock l(cs);
		dcassert(transferItems.find(aCqi) != transferItems.end());
		i = transferItems[aCqi];		
		i->statusString = aCqi->getState() == ConnectionQueueItem::CONNECTING ? TSTRING(CONNECTING) : TSTRING(WAITING_TO_RETRY);
/*		if(i->statusString == TSTRING(CONNECTING)) {
			i->status = ItemInfo::STATUS_WAITING;
		} else {
			i->status = ItemInfo::STATUS_RUNNING;			
		}*/
		i->updateMask |= ItemInfo::MASK_STATUS;
		if (i->type == ItemInfo::TYPE_DOWNLOAD) {
			QueueItem* qi = QueueManager::getInstance()->lookupNext(aCqi->getUser());
			if (qi) {
				i->columns[COLUMN_FILE] = Text::toT(Util::getFileName(qi->getTarget()));
				i->columns[COLUMN_PATH] = Text::toT(Util::getFilePath(qi->getTarget()));
				i->columns[COLUMN_SIZE] = Text::toT(Util::formatBytes(qi->getSize()));
				i->file = Text::toT(Util::getFileName(qi->getTarget()));
				i->path = Text::toT(Util::getFilePath(qi->getTarget()));
				i->size = qi->getSize();				
				i->Target = Text::toT(qi->getTarget());
				i->downloadTarget = Text::toT(qi->getTempTarget());
				i->qi = qi;
				i->updateMask |= (ItemInfo::MASK_FILE | ItemInfo::MASK_PATH | ItemInfo::MASK_SIZE);
				i->tth = qi->getTTH();				
			}
			i->updateMask |= (ItemInfo::MASK_USER | ItemInfo::MASK_HUB);
			dcdebug("OnConnectionStatus\n");
			if(i->Target != i->oldTarget) setMainItem(i);
			i->oldTarget = i->Target;

			if(i->upper != NULL) {
				if(i->qi) {
					if(i->qi->getActiveSegments().size() < 1) {
						i->upper->status = i->status;
						i->upper->statusString = i->statusString;
					}
					i->upper->columns[COLUMN_FILE] = Text::toT(Util::getFileName(i->qi->getTarget()));
					i->upper->columns[COLUMN_PATH] = Text::toT(Util::getFilePath(i->qi->getTarget()));
					i->upper->file = Text::toT(Util::getFileName(i->qi->getTarget()));
					i->upper->path = Text::toT(Util::getFilePath(i->qi->getTarget()));
					i->upper->size = i->qi->getSize();				
				}
				i->upper->columns[COLUMN_SIZE] = i->columns[COLUMN_SIZE];
				i->upper->Target = i->Target;
				i->upper->tth = i->tth;
				i->upper->downloadTarget = i->downloadTarget;
				if(i->status == ItemInfo::STATUS_RUNNING)
					i->upper->status = ItemInfo::STATUS_RUNNING;
			}
		}
	}

	PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)i);
}

void TransferView::on(ConnectionManagerListener::Removed, ConnectionQueueItem* aCqi) {
	ItemInfo* i;
	ItemInfo* h; 
	{
		Lock l(cs);
		dcdebug("onConnectionRemoved\n");
		ItemInfo::MapIter ii = transferItems.find(aCqi);
		dcassert(ii != transferItems.end());
		i = ii->second;
		h = i->upper;
		transferItems.erase(ii);

		if((i->type == ItemInfo::TYPE_DOWNLOAD) && (h != NULL)) {
			h->pocetUseru -= 1;

			ItemInfo* lastUserItem = findLastUserItem(h->Target);
			if(lastUserItem == NULL) {
				mainItems.erase(find(mainItems.begin(), mainItems.end(), h));
				PostMessage(WM_SPEAKER, REMOVE_ITEM, (LPARAM)h);
			} else {
				if(h->pocetUseru == 1) {					
					h->user = lastUserItem->user;
					if(!h->collapsed) {
						h->collapsed = true;
						PostMessage(WM_SPEAKER, REMOVE_ITEM_BUT_NOT_FREE, (LPARAM)lastUserItem);
					}
					PostMessage(WM_SPEAKER, UNSET_STATE, (LPARAM)h);
				}
				h->updateMask |= (ItemInfo::MASK_USER | ItemInfo::MASK_HUB);
				PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)h);
			}
		}
	}
	PostMessage(WM_SPEAKER, REMOVE_ITEM, (LPARAM)i);
}

void TransferView::on(ConnectionManagerListener::Failed, ConnectionQueueItem* aCqi, const string& aReason) {
	ItemInfo* i;
	{
		Lock l(cs);
		dcassert(transferItems.find(aCqi) != transferItems.end());
		i = transferItems[aCqi];		
		i->statusString = Text::toT(aReason);
		i->status = ItemInfo::STATUS_WAITING;

		if(i->type == ItemInfo::TYPE_DOWNLOAD) {
			if(i->Target != i->oldTarget) setMainItem(i);
			i->oldTarget = i->Target;

			int pocetSegmentu = i->qi ? i->qi->getActiveSegments().size() : 0;
			if((i->upper != NULL) && (pocetSegmentu < 1)) {
				i->upper->statusString = Text::toT(aReason);
			}
		}
			
		i->updateMask |= (ItemInfo::MASK_HUB | ItemInfo::MASK_USER | ItemInfo::MASK_STATUS);
	}
	PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)i);
}

void TransferView::on(DownloadManagerListener::Starting, Download* aDownload) {
	ConnectionQueueItem* aCqi = aDownload->getUserConnection()->getCQI();
	ItemInfo* i;
	{
		Lock l(cs);
		dcassert(transferItems.find(aCqi) != transferItems.end());
		i = transferItems[aCqi];		
		i->status = ItemInfo::STATUS_RUNNING;
		i->pos = 0;
		i->start = aDownload->getPos();
		i->actual = i->start;
		i->size = aDownload->getSize();
		i->file = Text::toT(Util::getFileName(aDownload->getTarget()));
		i->path = Text::toT(Util::getFilePath(aDownload->getTarget()));
		QueueItem* qi = QueueManager::getInstance()->getRunning(aCqi->getUser());
		if(qi) {
			i->qi = qi;
		}
		i->Target = Text::toT(aDownload->getTarget());

		if(i->Target != i->oldTarget) setMainItem(i);
		i->oldTarget = i->Target;

		if(i->user != (User::Ptr)NULL) 
			if(i->upper != NULL) {	
				if(i->qi) i->upper->qi = i->qi;
				i->upper->status = ItemInfo::STATUS_RUNNING;
				i->upper->size = aDownload->getSize();
				string x = Util::getFileName(aDownload->getTarget());
				i->upper->file = Text::toT(x);
				i->upper->path = Text::toT(Util::getFilePath(aDownload->getTarget()));
				i->upper->Target = Text::toT(aDownload->getTarget());
				i->upper->actual = aDownload->getQueueTotal();
				i->upper->start = i->upper->actual;
				i->upper->pos = i->upper->actual;
				i->upper->downloadTarget = Text::toT(aDownload->getDownloadTarget());
				i->upper->finished = false;
				if(i->qi->getActiveSegments().size() <= 1)
					i->upper->statusString = TSTRING(DOWNLOAD_STARTING);
		}
				
		i->downloadTarget = Text::toT(aDownload->getDownloadTarget());
		i->statusString = TSTRING(DOWNLOAD_STARTING);
		if(i->user->isClientOp())
			i->IP = Text::toT(aDownload->getUserConnection()->getRemoteIp());
		else 
			i->IP = Util::emptyStringT;
		string country = Util::getIpCountry(aDownload->getUserConnection()->getRemoteIp());
		i->country = Text::toT(country);
		i->flagImage = WinUtil::getFlagImage(country.c_str());
		i->updateMask |= ItemInfo::MASK_HUB | ItemInfo::MASK_STATUS | ItemInfo::MASK_FILE | ItemInfo::MASK_PATH |
			ItemInfo::MASK_SIZE | ItemInfo::MASK_IP;

	}

	if(!aDownload->isSet(Download::FLAG_USER_LIST)) {
		if ((!SETTING(BEGINFILE).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
			PlaySound(Text::toT(SETTING(BEGINFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);

		if(BOOLSETTING(POPUP_DOWNLOAD_START)) {
			MainFrame::getMainFrame()->ShowBalloonTip((
				TSTRING(FILE)+_T(": ")+i->file+_T("\n")+
				TSTRING(USER)+_T(": ")+Text::toT(i->user->getNick())).c_str(), CTSTRING(DOWNLOAD_STARTING));
		}

	}

	PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)i);
}

void TransferView::on(DownloadManagerListener::Tick, const Download::List& dl) {
	vector<ItemInfo*>* v = new vector<ItemInfo*>();
	v->reserve(dl.size());

	AutoArray<TCHAR> buf(TSTRING(DOWNLOADED_BYTES).size() + 64);

	{
		Lock l(cs);
		for(Download::List::const_iterator j = dl.begin(); j != dl.end(); ++j) {
			Download* d = *j;
			int64_t total = d->getQueueTotal();

			ConnectionQueueItem* aCqi = d->getUserConnection()->getCQI();
			ItemInfo* i = transferItems[aCqi];
			i->actual = i->start + d->getActual();
			i->pos = i->start + d->getTotal();
			i->timeLeft = d->getSecondsLeft();
			i->speed = d->getRunningAverage();

			QueueItem* qi = QueueManager::getInstance()->getRunning(aCqi->getUser());

			if(qi != NULL)
				i->qi = qi;			

			int NS = 0;
			int64_t tmp = 0;
			double pomerKomprese = 0;
			double a = 0;

			for(Download::List::const_iterator h = dl.begin(); h != dl.end(); ++h) {
				Download* e = *h;
				ConnectionQueueItem* cqi = e->getUserConnection()->getCQI();
				ItemInfo* ch = transferItems[cqi];
				if (e->getTarget() == d->getTarget()) {
					tmp += e->getRunningAverage();
					a = ch->getRatio();
					if(a>0) {
						pomerKomprese += a;
						++NS;
					}
				}
			}

			if(NS>0) pomerKomprese = pomerKomprese / NS; else pomerKomprese = 1.0;
			i->timeLeft = (tmp > 0) ? ((d->getSize() - total) / tmp) : 0;

			i->celkovaRychlost = tmp;
			if(qi != NULL) {
				qi->setSpeed(tmp);
				i->tth = qi->getTTH();
			}

			if(i->upper != NULL) {
				if(i->qi != NULL)
					i->upper->qi = i->qi;
				i->upper->compressRatio = pomerKomprese;
				i->upper->speed = tmp;
				i->upper->celkovaRychlost = tmp;
				i->upper->tth = i->tth;
		
				_stprintf(buf, CTSTRING(DOWNLOADED_BYTES), Text::toT(Util::formatBytes(total)).c_str(), 
					(double)total*100.0/(double)d->getSize(), Text::toT(Util::formatSeconds((GET_TICK() - i->qi->getStart())/1000)).c_str());

				i->upper->statusString = buf;
				i->upper->actual = total * pomerKomprese;
				i->upper->pos = total;
				i->upper->size = d->getSize();
				i->upper->timeLeft = i->timeLeft;
				(pomerKomprese < 1) ? i->upper->setFlag(ItemInfo::FLAG_COMPRESSED) : i->upper->unsetFlag(ItemInfo::FLAG_COMPRESSED);
			}

			_stprintf(buf, CTSTRING(DOWNLOADED_BYTES), Text::toT(Util::formatBytes(total)).c_str(), 
				(double)total*100.0/(double)d->getSize(), Text::toT(Util::formatSeconds((GET_TICK() - d->getStart())/1000)).c_str());

			if (BOOLSETTING(SHOW_PROGRESS_BARS)) {
				d->isSet(Download::FLAG_ZDOWNLOAD) ? i->setFlag(ItemInfo::FLAG_COMPRESSED) : i->unsetFlag(ItemInfo::FLAG_COMPRESSED);
				i->statusString = buf;
			} else {
				if(d->isSet(Download::FLAG_ZDOWNLOAD)) {
					i->statusString = _T("* ") + tstring(buf);
				} else {
					i->statusString = buf;
				}
			}
			i->updateMask |= ItemInfo::MASK_HUB | ItemInfo::MASK_STATUS | ItemInfo::MASK_TIMELEFT | ItemInfo::MASK_SPEED | ItemInfo::MASK_RATIO;

			v->push_back(i);

			if(d->getRunningAverage() <= 0) {
				d->getUserConnection()->reconnect();
			}
		}
	}

	PostMessage(WM_SPEAKER, UPDATE_ITEMS, (LPARAM)v);
}

void TransferView::on(DownloadManagerListener::Failed, Download* aDownload, const string& aReason) {
	ConnectionQueueItem* aCqi = aDownload->getUserConnection()->getCQI();
	ItemInfo* i;
	{
		Lock l(cs);
		dcassert(transferItems.find(aCqi) != transferItems.end());
		i = transferItems[aCqi];		
		i->status = ItemInfo::STATUS_WAITING;
		i->pos = 0;

		i->statusString = Text::toT(aReason);
		i->size = aDownload->getSize();
		i->file = Text::toT(Util::getFileName(aDownload->getTarget()));
		i->path = Text::toT(Util::getFilePath(aDownload->getTarget()));
		i->Target = Text::toT(aDownload->getTarget());
		i->downloadTarget = Text::toT(aDownload->getDownloadTarget());
		QueueItem* qi = QueueManager::getInstance()->getRunning(aCqi->getUser());
		if(qi) i->qi = qi;

		dcdebug("OnDownloadFailed\n");
		if(i->Target != i->oldTarget) setMainItem(i);
		i->oldTarget = i->Target;

		if(i->upper) {
			i->upper->downloadTarget = Text::toT(aDownload->getDownloadTarget());
			i->upper->file = Text::toT(Util::getFileName(aDownload->getTarget()));
			i->upper->path = Text::toT(Util::getFilePath(aDownload->getTarget()));
			i->upper->size = aDownload->getSize();

			if(i->qi) {
				i->upper->qi = i->qi;
				if(i->qi->getCurrents().size() <= 1) {
					i->upper->status = ItemInfo::STATUS_WAITING;
					i->upper->statusString = Text::toT(aReason);
				}
				if(BOOLSETTING(POPUP_DOWNLOAD_FAILED) && !i->qi->isSet(QueueItem::FLAG_CHECK_FILE_LIST)
					&& (aReason != STRING(DOWNLOADING_TTHL) && (aReason != STRING(CHECKING_TTH)))
					) {
					MainFrame::getMainFrame()->ShowBalloonTip((
						TSTRING(FILE)+_T(": ")+i->file+_T("\n")+
						TSTRING(USER)+_T(": ")+Text::toT(i->user->getNick())+_T("\n")+
						TSTRING(REASON)+_T(": ")+Text::toT(aReason)).c_str(), CTSTRING(DOWNLOAD_FAILED), NIIF_WARNING);
				}
			}
		}

		i->updateMask |= ItemInfo::MASK_HUB | ItemInfo::MASK_STATUS | ItemInfo::MASK_SIZE | ItemInfo::MASK_FILE |
		ItemInfo::MASK_PATH;	
	}
	PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)i);
}

void TransferView::on(HashManagerListener::Verifying, const string& fileName, int64_t remainingBytes) {
	ItemInfo* i = NULL;

	{
		Lock l(cs);
		for(ItemInfo::Map::iterator j = transferItems.begin(); j != transferItems.end(); ++j) {
			ItemInfo* m = j->second;
			if((m->downloadTarget == Text::toT(fileName)) && (m->type == ItemInfo::TYPE_DOWNLOAD)) {	
				i = m;
				break;
			}
		}
		
		if(i == NULL)
			return;

		int64_t hashedSize = i->size - remainingBytes;

		//i->statusString = TSTRING(CHECKING_TTH) + Text::toT(Util::toString(hashedSize * 100 / i->size) + "%");
		i->upper->statusString = TSTRING(CHECKING_TTH) + Text::toT(Util::toString(hashedSize * 100 / i->size) + "%");
		i->updateMask |= ItemInfo::MASK_STATUS;
	}
	PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)i);
}

void TransferView::on(UploadManagerListener::Starting, Upload* aUpload) {
	ConnectionQueueItem* aCqi = aUpload->getUserConnection()->getCQI();
	ItemInfo* i;
	{
		Lock l(cs);
		dcassert(transferItems.find(aCqi) != transferItems.end());
		i = transferItems[aCqi];		
		i->pos = 0;
		i->start = aUpload->getPos();
		i->actual = i->start;
		i->size = aUpload->getSize();
		i->status = ItemInfo::STATUS_RUNNING;
		i->speed = 0;
		i->timeLeft = 0;
		i->file = Text::toT(Util::getFileName(aUpload->getLocalFileName()));
		i->path = Text::toT(Util::getFilePath(aUpload->getLocalFileName()));
		i->statusString = TSTRING(UPLOAD_STARTING);
		if(i->user->isClientOp())
			i->IP = Text::toT(aUpload->getUserConnection()->getRemoteIp());
		else 
			i->IP = Util::emptyStringT;
		string country = Util::getIpCountry(aUpload->getUserConnection()->getRemoteIp());
		i->country = Text::toT(country);
		i->flagImage = WinUtil::getFlagImage(country.c_str());
		i->updateMask |= ItemInfo::MASK_STATUS | ItemInfo::MASK_FILE | ItemInfo::MASK_PATH |
			ItemInfo::MASK_SIZE | ItemInfo::MASK_IP;
	}

	PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)i);
}

void TransferView::on(UploadManagerListener::Tick, const Upload::List& ul) {
	vector<ItemInfo*>* v = new vector<ItemInfo*>();
	v->reserve(ul.size());

    AutoArray<TCHAR> buf(STRING(UPLOADED_BYTES).size() + 64);

	{
		Lock l(cs);
		for(Upload::List::const_iterator j = ul.begin(); j != ul.end(); ++j) {
			Upload* u = *j;

			ConnectionQueueItem* aCqi = u->getUserConnection()->getCQI();
			ItemInfo* i = transferItems[aCqi];	
			i->actual = i->start + u->getActual();
			i->pos = i->start + u->getTotal();
			i->timeLeft = u->getSecondsLeft();
			i->speed = u->getRunningAverage();

			if (u->getPos() >= 0) {
				_stprintf(buf, CTSTRING(UPLOADED_BYTES), Text::toT(Util::formatBytes(u->getPos())).c_str(), 
				(double)u->getPos()*100.0/(double)u->getSize(), Text::toT(Util::formatSeconds((GET_TICK() - u->getStart())/1000)).c_str());
            } else _stprintf(buf, CTSTRING(UPLOAD_STARTING));
			if (BOOLSETTING(SHOW_PROGRESS_BARS)) {
				u->isSet(Upload::FLAG_ZUPLOAD) ? i->setFlag(ItemInfo::FLAG_COMPRESSED) : i->unsetFlag(ItemInfo::FLAG_COMPRESSED);
				i->statusString = buf;
			} else {
				if(u->isSet(Upload::FLAG_ZUPLOAD)) {
				i->statusString = _T("* ") + tstring(buf);
				} else {
					i->statusString = buf;
				}
			}

			i->updateMask |= ItemInfo::MASK_STATUS | ItemInfo::MASK_TIMELEFT | ItemInfo::MASK_SPEED | ItemInfo::MASK_RATIO;
			v->push_back(i);
		}
	}

	PostMessage(WM_SPEAKER, UPDATE_ITEMS, (LPARAM)v);
}

void TransferView::onTransferComplete(Transfer* aTransfer, bool isUpload) {
	ConnectionQueueItem* aCqi = aTransfer->getUserConnection()->getCQI();
	ItemInfo* i;
	{
		Lock l(cs);
		dcassert(transferItems.find(aCqi) != transferItems.end());
		i = transferItems[aCqi];		

		if(!isUpload) {
			if(i->Target != i->oldTarget) setMainItem(i);
			i->oldTarget = i->Target;
			if(i->upper != NULL) {	
				i->upper->status = ItemInfo::STATUS_WAITING;
				i->upper->statusString = TSTRING(DOWNLOAD_FINISHED_IDLE);
				i->upper->finished = true;
			}

			if(BOOLSETTING(POPUP_DOWNLOAD_FINISHED)) {
				MainFrame::getMainFrame()->ShowBalloonTip((
			TSTRING(FILE)+_T(": ")+i->file+_T("\n")+
			TSTRING(USER)+_T(": ")+Text::toT(i->user->getNick())).c_str(), CTSTRING(DOWNLOAD_FINISHED_IDLE));
			}
		} else {
			if(BOOLSETTING(POPUP_UPLOAD_FINISHED)) {
				MainFrame::getMainFrame()->ShowBalloonTip((
			TSTRING(FILE)+_T(": ")+i->file+_T("\n")+
			TSTRING(USER)+_T(": ")+Text::toT(i->user->getNick())).c_str(), CTSTRING(UPLOAD_FINISHED_IDLE));
			}
		}

		i->status = ItemInfo::STATUS_WAITING;
		i->pos = 0;

		i->statusString = isUpload ? TSTRING(UPLOAD_FINISHED_IDLE) : TSTRING(DOWNLOAD_FINISHED_IDLE);
		i->updateMask |= ItemInfo::MASK_HUB | ItemInfo::MASK_STATUS | ItemInfo::MASK_SPEED | ItemInfo::MASK_TIMELEFT;
	}
	PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)i);
}

void TransferView::ItemInfo::disconnect() {
	ConnectionManager::getInstance()->removeConnection(user, (type == TYPE_DOWNLOAD));
}

LRESULT TransferView::onSearchAlternates(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ItemInfo* ii = ctrlTransfers.getItemData(i);
		if(ii->tth != NULL)
			WinUtil::searchHash(ii->tth);
	} 
	return 0;
}
			
LRESULT TransferView::onPreviewCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	tstring target = ctrlTransfers.getItemData(ctrlTransfers.GetNextItem(-1, LVNI_SELECTED))->downloadTarget;
	WinUtil::RunPreviewCommand(wID - IDC_PREVIEW_APP, Text::fromT(target));
	return 0;
}

TransferView::ItemInfo* TransferView::findMainItem(tstring Target) {

	if(!mainItems.empty()) {
		int q = 0;
		while(q<mainItems.size()) {
			ItemInfo* m = mainItems[q];
			if(m->Target == Target) {
				return m;
			}
			q++;
		}
	}
	return NULL;
}

void TransferView::InsertItem(ItemInfo* i, bool mainThread) {
	//Lock l(cs);
	ItemInfo* mainItem = findMainItem(i->Target);

	if(!mainItem) {
	  	ItemInfo* h = new ItemInfo(i->user, ItemInfo::TYPE_DOWNLOAD, ItemInfo::STATUS_WAITING);
		h->Target = i->Target;
		h->columns[COLUMN_PATH] = i->columns[COLUMN_PATH];
		h->columns[COLUMN_FILE] = i->columns[COLUMN_FILE];
		h->columns[COLUMN_SIZE] = i->columns[COLUMN_SIZE];
		h->file = i->columns[COLUMN_FILE];
		h->path = i->columns[COLUMN_PATH];
		h->size = i->size;
		h->mainItem = true;
		h->upper = NULL;
		h->columns[COLUMN_STATUS] = h->statusString = TSTRING(CONNECTING);
		i->qi = QueueManager::getInstance()->fileQueue.find(Text::fromT(i->Target));
		h->qi = i->qi;
		if(i->qi)
			h->downloadTarget = Text::toT(i->qi->getTempTarget());
		i->upper = h;
		mainItems.push_back(h);

		if(mainThread)
			ctrlTransfers.insertItem(ctrlTransfers.GetItemCount(), h, IMAGE_DOWNLOAD);
		else
			PostMessage(WM_SPEAKER, SET_STATE, (LPARAM)h);
	} else {
		i->upper = mainItem;
		i->upper->pocetUseru += 1;			

		if(i->upper->pocetUseru == 1) {
			i->upper->columns[COLUMN_USER] = Text::toT(i->user->getNick());
			i->upper->columns[COLUMN_HUB] = Text::toT(i->user->getClientName());
		} else {
			i->upper->columns[COLUMN_USER] = Text::toT(Util::toString(mainItem->pocetUseru))+_T(" ")+TSTRING(HUB_USERS);
			if(mainItem->qi)
				i->upper->columns[COLUMN_HUB] = Text::toT(Util::toString((int)mainItem->qi->getActiveSegments().size()))+_T(" ")+TSTRING(NUMBER_OF_SEGMENTS);
			else
				i->upper->columns[COLUMN_HUB] = _T("0 ") + TSTRING(NUMBER_OF_SEGMENTS);
		}

		if(mainThread) {
			int r = ctrlTransfers.findItem(mainItem);

			if(r != -1) {
				if(!i->upper->collapsed) {
					int position = 0;
					if(i->qi) {
						position = i->qi->getActiveSegments().size();
					}
					insertSubItem(i,r + position + 1);
					if(ctrlTransfers.getSortColumn() != COLUMN_STATUS)
						ctrlTransfers.resort();
				}

				if(i->upper->pocetUseru > 1) {
					ctrlTransfers.SetItemState(r, INDEXTOSTATEIMAGEMASK((i->upper->collapsed) ? 1 : 2), LVIS_STATEIMAGEMASK);
				}
			}
		} else {
			PostMessage(WM_SPEAKER, INSERT_SUBITEM, (LPARAM)i);
		}
	}
}

void TransferView::insertSubItem(ItemInfo* j, int idx) {
	LV_ITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE | LVIF_INDENT;
	lvi.iItem = idx;
	lvi.iSubItem = 0;
	lvi.iIndent = 1;
	lvi.pszText = LPSTR_TEXTCALLBACK;
	lvi.iImage = IMAGE_SEGMENT;
	lvi.lParam = (LPARAM)j;
	lvi.state = 0;
	lvi.stateMask = 0;

	ctrlTransfers.InsertItem(&lvi);
}

void TransferView::setMainItem(ItemInfo* i) {
	if(i->type != ItemInfo::TYPE_DOWNLOAD)
		return;	
		
	dcdebug("1. setMainItem started\n");

	if(i->upper != NULL) {

		ItemInfo* h = i->upper;		
		dcdebug("2. cyklus\n");

		if(h->Target != i->Target) {
			dcdebug("3. cyklus\n");
			h->pocetUseru -= 1;

			PostMessage(WM_SPEAKER, REMOVE_ITEM_BUT_NOT_FREE, (LPARAM)i);

			i->upper = findMainItem(i->Target);
			InsertItem(i, false);

			dcdebug("4. cyklus\n");

			ItemInfo* lastUserItem = findLastUserItem(h->Target);
			if(lastUserItem == NULL) {
				mainItems.erase(find(mainItems.begin(), mainItems.end(), h));
				PostMessage(WM_SPEAKER, REMOVE_ITEM, (LPARAM)h);
			} else {
				if(h->pocetUseru == 1) {
					h->user = lastUserItem->user;
					if(!h->collapsed) {
						h->collapsed = true;
						PostMessage(WM_SPEAKER, REMOVE_ITEM_BUT_NOT_FREE, (LPARAM)lastUserItem);
					}
					PostMessage(WM_SPEAKER, UNSET_STATE, (LPARAM)h);
				}
				h->updateMask |= (ItemInfo::MASK_USER | ItemInfo::MASK_HUB);
				PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)h);
			}
		}
	} else {
		i->upper = findMainItem(i->Target);
	}
	dcdebug("6. setMainItem finished\n");
}

void TransferView::CollapseAll() {
	for(int q = ctrlTransfers.GetItemCount()-1; q != -1; --q) {
		ItemInfo* m = (ItemInfo*)ctrlTransfers.getItemData(q);
		if((m->type == ItemInfo::TYPE_DOWNLOAD) && (!m->mainItem)) {
			ctrlTransfers.deleteItem(m); 
		}
		if(m->mainItem) {
			m->collapsed = true;
			ctrlTransfers.SetItemState(ctrlTransfers.findItem(m), INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
		 }
	}
}

void TransferView::ExpandAll() {
	for(int q = mainItems.size()-1; q != -1; --q) {
		ItemInfo* m = mainItems[q];
		if(m->collapsed) {
			int i = ctrlTransfers.findItem(m);
			Expand(m,i);
		}
	}
}

void TransferView::Collapse(ItemInfo* i, int a) {
	for(int q = ctrlTransfers.GetItemCount()-1; q != -1; --q) {
		ItemInfo* m = (ItemInfo*)ctrlTransfers.getItemData(q);
		if((m->type == ItemInfo::TYPE_DOWNLOAD) && (m->Target == i->Target) && (!m->mainItem)) {
			PostMessage(WM_SPEAKER, REMOVE_ITEM_BUT_NOT_FREE, (LPARAM)m);
		}
	}
	i->collapsed = true;
	ctrlTransfers.SetItemState(a, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
}

void TransferView::Expand(ItemInfo* i, int a) {
	if(i->pocetUseru == 1)
		return;

	int q = 0;
	for(ItemInfo::Map::iterator j = transferItems.begin(); j != transferItems.end(); ++j) {
		ItemInfo* m = j->second;
		if((m->Target == i->Target) && (m->type == ItemInfo::TYPE_DOWNLOAD)) {	
			q++;
			insertSubItem(m,a+1);
		}
	}

	i->pocetUseru = q;

	if(q == 0) {
		mainItems.erase(find(mainItems.begin(), mainItems.end(), i));
		PostMessage(WM_SPEAKER, REMOVE_ITEM, (LPARAM)i);
	} else {
		i->collapsed = false;
		ctrlTransfers.SetItemState(a, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
		ctrlTransfers.resort();
	}
}

LRESULT TransferView::onLButton(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	bHandled = false;
	LPNMITEMACTIVATE item = (LPNMITEMACTIVATE)pnmh;
	if (item->iItem != -1) {
		CRect rect;
		ctrlTransfers.GetItemRect(item->iItem, rect, LVIR_ICON);

		if (item->ptAction.x < rect.left)
		{
			ItemInfo* i = (ItemInfo*)ctrlTransfers.getItemData(item->iItem);
			if((i->type == ItemInfo::TYPE_DOWNLOAD) && (i->mainItem)) {
				if(i->collapsed) Expand(i,item->iItem); else Collapse(i,item->iItem);
			}
		}
	}
	return 0;
} 

LRESULT TransferView::onConnectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlTransfers.GetSelectedCount() == 1) {
		int i = ctrlTransfers.GetNextItem(-1, LVNI_SELECTED);
		ItemInfo* ii = ctrlTransfers.getItemData(i);
		ctrlTransfers.SetItemText(i, COLUMN_STATUS, CTSTRING(CONNECTING_FORCED));
		for(ItemInfo::Map::iterator j = transferItems.begin(); j != transferItems.end(); ++j) {
			ItemInfo* m = j->second;
			if((m->Target == ii->Target) && (m->type == ItemInfo::TYPE_DOWNLOAD)) {	
				int h = ctrlTransfers.findItem(m);
				if(h != -1)
					ctrlTransfers.SetItemText(h, COLUMN_STATUS, CTSTRING(CONNECTING_FORCED));
				m->user->connect();
			}
		}
	}
	return 0;
}

LRESULT TransferView::onDisconnectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlTransfers.GetSelectedCount() == 1) {
		int i = ctrlTransfers.GetNextItem(-1, LVNI_SELECTED);
		ItemInfo* ii = ctrlTransfers.getItemData(i);
		ctrlTransfers.SetItemText(i, COLUMN_STATUS, CTSTRING(DISCONNECTED));
		for(ItemInfo::Map::iterator j = transferItems.begin(); j != transferItems.end(); ++j) {
			ItemInfo* m = j->second;
			if((m->Target == ii->Target) && (m->type == ItemInfo::TYPE_DOWNLOAD)) {	
				int h = ctrlTransfers.findItem(m);
				if(h != -1)
					ctrlTransfers.SetItemText(h, COLUMN_STATUS, CTSTRING(DISCONNECTED));
				m->disconnect();
			}
		}
	}
	return 0;
}

LRESULT TransferView::onSlowDisconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlTransfers.GetSelectedCount() == 1) {
		int i = ctrlTransfers.GetNextItem(-1, LVNI_SELECTED);
		QueueItem* qi = ctrlTransfers.getItemData(i)->qi;
		if(qi) {
			qi->setSlowDisconnect(!qi->getSlowDisconnect());
		}
	}
	return 0;
}

TransferView::ItemInfo* TransferView::findLastUserItem(tstring Target) {
	for(ItemInfo::Map::iterator j = transferItems.begin(); j != transferItems.end(); ++j) {
		ItemInfo* m = j->second;
		if((m->Target == Target) && (m->type == ItemInfo::TYPE_DOWNLOAD)) {	
			return m;
		}
	}
	return NULL;
}

void TransferView::on(SettingsManagerListener::Save, SimpleXML* /*xml*/) throw() {
	bool refresh = false;
	if(ctrlTransfers.GetBkColor() != WinUtil::bgColor) {
		ctrlTransfers.SetBkColor(WinUtil::bgColor);
		ctrlTransfers.SetTextBkColor(WinUtil::bgColor);
		ctrlTransfers.setFlickerFree(WinUtil::bgBrush);
		refresh = true;
	}
	if(ctrlTransfers.GetTextColor() != WinUtil::textColor) {
		ctrlTransfers.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

/**
 * @file
 * $Id$
 */
