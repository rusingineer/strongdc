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

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"


#include "../client/QueueManager.h"

#include "../client/QueueItem.h"

#include "WinUtil.h"
#include "TransferView.h"
#include "CZDCLib.h"
#include "MainFrm.h"
#include "SearchFrm.h"

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

	DWORD styles = LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT;
	if (BOOLSETTING(SHOW_INFOTIPS))
		styles |= LVS_EX_INFOTIP;

	if(Util::getOsVersion().substr(0, 5) != "WinXP"){
		ctrlTransfers.setLeftEraseBackgroundMargin(40);
	}else{
		styles |= 0x00010000;
	}

	ctrlTransfers.SetExtendedListViewStyle(styles);

	WinUtil::splitTokens(columnIndexes, SETTING(MAINFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(MAINFRAME_WIDTHS), COLUMN_LAST);

	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_TIMELEFT || j == COLUMN_SPEED) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlTransfers.InsertColumn(j, CSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlTransfers.SetColumnOrderArray(COLUMN_LAST, columnIndexes);

	ctrlTransfers.SetBkColor(WinUtil::bgColor);
	ctrlTransfers.SetTextBkColor(WinUtil::bgColor);
	ctrlTransfers.SetTextColor(WinUtil::textColor);

	ctrlTransfers.SetImageList(states, LVSIL_STATE); 
	ctrlTransfers.SetImageList(arrows, LVSIL_SMALL);
	ctrlTransfers.setSortColumn(COLUMN_USER);

	transferMenu.CreatePopupMenu();
	appendUserItems(transferMenu);
	transferMenu.AppendMenu(MF_STRING, IDC_FORCE, CSTRING(FORCE_ATTEMPT));
	transferMenu.AppendMenu(MF_SEPARATOR, 0, (LPTSTR)NULL);
	transferMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CSTRING(SEARCH_FOR_ALTERNATES));

	previewMenu.CreatePopupMenu();
	transferMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)previewMenu, CSTRING(PREVIEW_MENU));
	previewMenu.AppendMenu(MF_SEPARATOR, 0, (LPTSTR)NULL);

	usercmdsMenu.CreatePopupMenu();
	transferMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)usercmdsMenu, CSTRING(SETTINGS_USER_COMMANDS));

	transferMenu.AppendMenu(MF_STRING, IDC_OFFCOMPRESS, CSTRING(DISABLE_COMPRESSION));
	transferMenu.AppendMenu(MF_SEPARATOR, 0, (LPTSTR)NULL);
	transferMenu.AppendMenu(MF_STRING, IDC_REMOVE, CSTRING(CLOSE_CONNECTION));
	transferMenu.SetMenuDefaultItem(IDC_PRIVATEMESSAGE);

	segmentedMenu.CreatePopupMenu();
	segmentedMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CSTRING(SEARCH_FOR_ALTERNATES));
	segmentedMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)previewMenu, CSTRING(PREVIEW_MENU));
	segmentedMenu.AppendMenu(MF_SEPARATOR, 0, (LPTSTR)NULL);
	segmentedMenu.AppendMenu(MF_STRING, IDC_CONNECT_ALL, CSTRING(CONNECT_ALL));
	segmentedMenu.AppendMenu(MF_STRING, IDC_DISCONNECT_ALL, CSTRING(DISCONNECT_ALL));
	segmentedMenu.AppendMenu(MF_SEPARATOR, 0, (LPTSTR)NULL);
	segmentedMenu.AppendMenu(MF_STRING, IDC_EXPAND_ALL, CSTRING(EXPAND_ALL));
	segmentedMenu.AppendMenu(MF_STRING, IDC_COLLAPSE_ALL, CSTRING(COLLAPSE_ALL));
	segmentedMenu.AppendMenu(MF_SEPARATOR, 0, (LPTSTR)NULL);
	segmentedMenu.AppendMenu(MF_STRING, IDC_REMOVEALL, CSTRING(REMOVE_ALL));

	hIconCompressed = (HICON)LoadImage((HINSTANCE)::GetWindowLong(::GetParent(m_hWnd), GWL_HINSTANCE), MAKEINTRESOURCE(IDR_TRANSFER_COMPRESSED), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);

	ConnectionManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);
	UploadManager::getInstance()->addListener(this);
#if 0
	ItemInfo* ii = new ItemInfo(ClientManager::getInstance()->getUser("test"), 
		ItemInfo::TYPE_DOWNLOAD, ItemInfo::STATUS_RUNNING, 75, 100, 25, 50);
	ctrlTransfers.insert(0, string("Test"), 0, (LPARAM)ii);
#endif
	return 0;
}

void TransferView::prepareClose() {
	WinUtil::saveHeaderOrder(ctrlTransfers, SettingsManager::MAINFRAME_ORDER, 
		SettingsManager::MAINFRAME_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);
	
	ConnectionManager::getInstance()->removeListener(this);
	DownloadManager::getInstance()->removeListener(this);
	UploadManager::getInstance()->removeListener(this);

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
	
			usercmdsMenu.InsertSeparatorFirst(CSTRING(SETTINGS_USER_COMMANDS));
	
			if(itemI->user != (User::Ptr)NULL)
			prepareMenu(usercmdsMenu, UserCommand::CONTEXT_CHAT, itemI->user->getClientAddressPort(), itemI->user->isClientOp());
	}
		ItemInfo* ii = ctrlTransfers.getItemData(ctrlTransfers.GetNextItem(-1, LVNI_SELECTED));
		WinUtil::ClearPreviewMenu(previewMenu);
		if(ii->type == ItemInfo::TYPE_DOWNLOAD && ii->file != "") {
			string ext = Util::getFileExt(ii->file);
			if(ext.size()>1) ext = ext.substr(1);
			PreviewAppsSize = WinUtil::SetupPreviewMenu(previewMenu, ext);
		}

		if(previewMenu.GetMenuItemCount() > 0) {
			segmentedMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MF_ENABLED);
			transferMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MF_ENABLED);
		} else {
			segmentedMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MF_GRAYED);
			transferMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MF_GRAYED);
		}

		previewMenu.InsertSeparatorFirst(CSTRING(PREVIEW_MENU));
		ctrlTransfers.ClientToScreen(&pt);
		
		if(ii->pocetUseru <= 1) {
			transferMenu.InsertSeparatorFirst(CSTRING(MENU_TRANSFERS));
			transferMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
			transferMenu.RemoveFirstItem();
		} else {
			segmentedMenu.InsertSeparatorFirst(CSTRING(SETTINGS_SEGMENT));
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

		ucParams["mynick"] = itemI->user->getClientNick();

		itemI->user->getParams(ucParams);

		itemI->user->send(Util::formatParams(uc.getCommand(), ucParams));
		}
	return;
};


LRESULT TransferView::onForce(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ctrlTransfers.SetItemText(i, COLUMN_STATUS, CSTRING(CONNECTING_FORCED));
		ctrlTransfers.getItemData(i)->user->connect();
	}
	return 0;
}

void TransferView::ItemInfo::removeAll() {
	if(pocetUseru == 1)
		QueueManager::getInstance()->removeSources(user, QueueItem::Source::FLAG_REMOVED);
	else
		QueueManager::getInstance()->remove(Target);
}

LRESULT TransferView::onOffCompress(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ctrlTransfers.getItemData(i)->disconnect();
		ctrlTransfers.SetItemText(i, COLUMN_STATUS, "Vypínám kompresi...");	
		
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
				char buf[256];
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
				if (hBmp == NULL)
					MessageBox(Util::translateError(GetLastError()).c_str(), "ERROR", MB_OK);
				HBITMAP pOldBmp = cdc.SelectBitmap(hBmp);
				HDC& dc = cdc.m_hDC;

				HFONT oldFont = (HFONT)SelectObject(dc, WinUtil::font);
				SetBkMode(dc, TRANSPARENT);
		
				// New style progressbar tweaks the current colors
				HLSTRIPLE hls_bk = OperaColors::RGB2HLS(cd->clrTextBk);

				// Create pen (ie outline border of the cell)
				HPEN penBorder = ::CreatePen(PS_SOLID, 1, OperaColors::blendColors(cd->clrTextBk, clr, (hls_bk.hlstLightness > 0.75) ? 0.6 : 0.4));
				HGDIOBJ pOldPen = ::SelectObject(dc, penBorder);

				// Draw the outline (but NOT the background) using pen
				HBRUSH hBrOldBg = CreateSolidBrush(cd->clrTextBk);
				hBrOldBg = (HBRUSH)::SelectObject(dc, hBrOldBg);
				::Rectangle(dc, rc.left, rc.top, rc.right, rc.bottom);
				DeleteObject(::SelectObject(dc, hBrOldBg));

				// Set the background color, by slightly changing it
				HBRUSH hBrDefBg = CreateSolidBrush(OperaColors::blendColors(cd->clrTextBk, clr, (hls_bk.hlstLightness > 0.75) ? 0.85 : 0.70));
				HGDIOBJ oldBg = ::SelectObject(dc, hBrDefBg);
				// Set the text color
				COLORREF hTextDefColor = cd->clrText; // Even tho we "change" background, we don't change it very much

				// Draw the outline AND the background using pen+brush
				::Rectangle(dc, rc.left, rc.top, rc.left + (LONG)(rc.Width() * ii->getRatio() + 0.5), rc.bottom);

				// Reset pen
				DeleteObject(::SelectObject(dc, pOldPen));
				// Reset bg (brush)
				DeleteObject(::SelectObject(dc, oldBg));

				// Draw the text over the entire item
                COLORREF oldcol = ::SetTextColor(dc, hTextDefColor);
				::DrawText(dc, buf, strlen(buf), rc2, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);

				// Draw the background and border of the bar
				if(ii->size <= 0)
					ii->size = 1;

				{
					Lock l(cs);

					if((ii->mainItem) || (ii->type == ItemInfo::TYPE_UPLOAD)) {
						int64_t w = rc2.Width();

						double ratio = ii->getRatio();
	
						if(ii->mainItem)
							w = w * ratio + 0.5;
	
						int64_t start = ii->start;
						int64_t size = ii->size;

						rc.right = rc.left + (int) (w * ii->actual / ii->size);
                
						COLORREF a, b;
						OperaColors::EnlightenFlood(clr, a, b);
						OperaColors::FloodFill(cdc, rc.left+1, rc.top+1, rc.right, rc.bottom-1, a, b, BOOLSETTING(PROGRESS_BUMPED));
					} else {
						LONG left = rc.left;
						int64_t w = rc.Width(); 						

						rc.right = left + (int) (w * ii->start / ii->size); 
						COLORREF a, b;
						OperaColors::EnlightenFlood(SETTING(SEGMENT_BAR_COLOR), a, b);
					
						if(BOOLSETTING(SHOW_SEGMENT_COLOR))
							OperaColors::FloodFill(cdc, rc.left+1, rc.top+1, rc.right, rc.bottom-1, a, b, BOOLSETTING(PROGRESS_BUMPED));

						rc.left = rc.right;
						rc.right = rc.left + (int) (w * ii->actual / ii->size); 
						OperaColors::EnlightenFlood(clr, a, b);
						OperaColors::FloodFill(cdc, rc.left+1, rc.top+1, rc.right, rc.bottom-1, a, b, BOOLSETTING(PROGRESS_BUMPED));
					}
				}
				
				// Draw the text only over the bar and with correct color
				rc2.right = rc.right + 1;
				::SetTextColor(dc, OperaColors::TextFromBackground(clr));
                ::DrawText(dc, buf, strlen(buf), rc2, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
		
				if (ii->isSet(ItemInfo::FLAG_COMPRESSED))
					DrawIconEx(dc, rc2.left - 12, rc2.top + 2, hIconCompressed, 16, 16, NULL, NULL, DI_NORMAL | DI_COMPAT);

				// Get the color of this text bar
				int textcolor = SETTING(PROGRESS_OVERRIDE_COLORS2) ? 
					((ii->type == ItemInfo::TYPE_DOWNLOAD) ? 
						SETTING(PROGRESS_TEXT_COLOR_DOWN) : 
						SETTING(PROGRESS_TEXT_COLOR_UP)) : 
					OperaColors::TextFromBackground(clr); //GetSysColor(COLOR_HIGHLIGHT);
				::SetTextColor(dc, textcolor);
                ::DrawText(dc, buf, strlen(buf), rc2, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);

				SelectObject(dc, oldFont);
				::SetTextColor(dc, oldcol);

				// New way:

				BitBlt(cd->nmcd.hdc, real_rc.left, real_rc.top, real_rc.Width(), real_rc.Height(), dc, 0, 0, SRCCOPY);
				DeleteObject(cdc.SelectBitmap(pOldBmp));

				return CDRF_SKIPDEFAULT;
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
		if(i->pocetUseru == 1)
			i->pm();
	}
	return 0;
}

LRESULT TransferView::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(wParam == ADD_ITEM) {
		ItemInfo* i = (ItemInfo*)lParam;
		if(i->type == ItemInfo::TYPE_DOWNLOAD) {
			InsertItem(i); 
		} else {
			ctrlTransfers.insertItem(ctrlTransfers.GetItemCount(),i, IMAGE_UPLOAD);
		}
	} else if(wParam == REMOVE_ITEM) {
		ItemInfo* i = (ItemInfo*)lParam;
		dcdebug(("****** TransferView: Removing "+Util::getFileName(i->Target)+" from user "+i->user->getNick()+"\n").c_str());
		ctrlTransfers.deleteItem(i);
		delete i;
	} else if(wParam == UPDATE_ITEM) {
		ItemInfo* i = (ItemInfo*)lParam;
		dcassert(i != NULL);
		i->update();
		if((i->upper != NULL) && (i->type == ItemInfo::TYPE_DOWNLOAD))
			ctrlTransfers.updateItem(i->upper);
		ctrlTransfers.updateItem(i);
		//if(ctrlTransfers.getSortColumn() != COLUMN_USER)
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

		if(ctrlTransfers.getSortColumn() != COLUMN_STATUS)
			ctrlTransfers.resort();
		ctrlTransfers.SetRedraw(TRUE);
				
		delete v;
	} else if(wParam == SET_STATE) {
		ItemInfo* i = (ItemInfo*)lParam;
		int m = ctrlTransfers.insertItem(0,i, IMAGE_DOWNLOAD);
		if(!i->qi || (!i->qi->isSet(QueueItem::FLAG_USER_LIST)) && ((!i->qi->isSet(QueueItem::FLAG_TESTSUR)))) {
			ctrlTransfers.SetItemState(m, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
		}
	} else if(wParam == REMOVE_ITEM_BUT_NOT_FREE) {
		ItemInfo* i = (ItemInfo*)lParam;
		ctrlTransfers.deleteItem(i);
	}

	return 0;
}

bool TransferView::ItemInfo::canDisplayUpper() {
	if((statusString == STRING(DOWNLOAD_FINISHED_IDLE)) ||
		(statusString == STRING(UPLOAD_FINISHED_IDLE)) ||
   		(statusString.substr(1,10) == STRING(DOWNLOAD_CORRUPTED).substr(1,10)) ||
   		(statusString == STRING(TTH_INCONSISTENCY)) ||
   		(statusString == STRING(CHECKING_TTH)) ||
   		(statusString == STRING(DISCONNECTED)) ||
   		(statusString == STRING(SFV_INCONSISTENCY)))
			return true;

	if(qi) {
		if((qi->isSet(QueueItem::FLAG_USER_LIST)) ||
		(qi->isSet(QueueItem::FLAG_TESTSUR))) return true;
 	}
  	return false;
}

void TransferView::ItemInfo::update() {
	u_int32_t colMask = updateMask;
	updateMask = 0;
	
	string spd = Util::formatBytes(speed) + "/s";
	string zc = Util::formatSeconds(timeLeft);	

	if(colMask & MASK_USER) {
		if(!mainItem || type == TYPE_UPLOAD) columns[COLUMN_USER] = user->getNick();
			else columns[COLUMN_USER] = Util::toString(pocetUseru)+" "+STRING(HUB_USERS);
		if((type == TYPE_DOWNLOAD) && (upper != NULL)) {
			upper->user = user;
			if(upper->pocetUseru > 1) {
				upper->columns[COLUMN_USER] = Util::toString(upper->pocetUseru)+" "+STRING(HUB_USERS);
			} else {
				upper->columns[COLUMN_USER] = user->getNick();
			}
		}
	}
	if(colMask & MASK_HUB) {
		if(!mainItem || type == TYPE_UPLOAD) columns[COLUMN_HUB] = user->getClientName();

		if((type == TYPE_DOWNLOAD) && (upper != NULL)) {
			if(upper->pocetUseru > 1) {
				upper->columns[COLUMN_HUB] = Util::toString((int)qi->getActiveSegments().size())+" "+STRING(NUMBER_OF_SEGMENTS);
			} else {
				upper->columns[COLUMN_HUB] = user->getClientName();
			}
		}			
	}
	if(colMask & MASK_STATUS) {
		columns[COLUMN_STATUS] = statusString;
		if((type == TYPE_DOWNLOAD) && (!mainItem) && (upper != NULL)) {
			if(!upper->finished) {
				if((statusString != STRING(ALL_SEGMENTS_TAKEN)) && (statusString != STRING(NO_FREE_BLOCK)))
					upper->columns[COLUMN_STATUS] = upper->statusString;
			} else if(canDisplayUpper()) {
				upper->columns[COLUMN_STATUS] = upper->statusString;
			}
		}
	}
	if (status == STATUS_RUNNING) {
		if(type == TYPE_DOWNLOAD) {
			zc = Util::formatSeconds((celkovaRychlost > 0) ? ((size - stazenoCelkem) / celkovaRychlost) : 0);
			if(upper != NULL) upper->status = ItemInfo::STATUS_RUNNING;
		}
		if(colMask & MASK_TIMELEFT) {
			columns[COLUMN_TIMELEFT] = zc;
			if((type == TYPE_DOWNLOAD) && (!mainItem) && (upper != NULL)) {
				upper->columns[COLUMN_TIMELEFT] = zc;
			}
		}
		if(colMask & MASK_SPEED) {
			columns[COLUMN_SPEED] = spd;
			if((type == TYPE_DOWNLOAD) && (!mainItem) && (upper != NULL)) {
				upper->columns[COLUMN_SPEED] = Util::formatBytes(celkovaRychlost) + "/s";
				upper->speed = celkovaRychlost;
			}
		}
		if(colMask & MASK_FILE) {
			columns[COLUMN_FILE] = file;
			if((type == TYPE_DOWNLOAD) && (!mainItem) && (upper != NULL)) {
				upper->columns[COLUMN_FILE] = file;
			}
		}
		if(colMask & MASK_SIZE) {
			columns[COLUMN_SIZE] = Util::formatBytes(size);
			if((type == TYPE_DOWNLOAD) && (!mainItem) && (upper != NULL)) {
				upper->columns[COLUMN_SIZE] = Util::formatBytes(size);
			}
		}
		if(colMask & MASK_PATH) {
			columns[COLUMN_PATH] = path;
			if((type == TYPE_DOWNLOAD) && (!mainItem) && (upper != NULL)) {
				upper->columns[COLUMN_PATH] = path;
			}
		}
		if(colMask & MASK_IP) {
			if (country == "") columns[COLUMN_IP] = IP;
			else columns[COLUMN_IP] = country + " (" + IP + ")";
		}
		if(colMask & MASK_RATIO) {
			columns[COLUMN_RATIO] = Util::toString(getRatio());
			if((type == TYPE_DOWNLOAD) && (!mainItem) && (upper != NULL)) {
				upper->columns[COLUMN_RATIO] = Util::toString(upper->compressRatio);
			}
		}
	} else {
		columns[COLUMN_TIMELEFT] = "";
		columns[COLUMN_SPEED] = "";
		columns[COLUMN_RATIO] = "";
		if((upper != NULL) && (upper->status != STATUS_RUNNING)) {
			upper->columns[COLUMN_TIMELEFT] = "";
			upper->columns[COLUMN_SPEED] = "";
			upper->columns[COLUMN_RATIO] = "";
			if(file != "") upper->columns[COLUMN_FILE] = file;
			if(size != -1) upper->columns[COLUMN_SIZE] = Util::formatBytes(size);
			if(path != "") upper->columns[COLUMN_PATH] = path;
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
		i->columns[COLUMN_STATUS] = i->statusString = STRING(CONNECTING);
		if (t == ItemInfo::TYPE_DOWNLOAD) {
			QueueItem* qi = QueueManager::getInstance()->lookupNext(aCqi->getUser());
			if (qi) {
				i->columns[COLUMN_FILE] = Util::getFileName(qi->getTarget());
				i->columns[COLUMN_PATH] = Util::getFilePath(qi->getTarget());
				i->columns[COLUMN_SIZE] = Util::formatBytes(qi->getSize());
				i->size = qi->getSize();
				i->Target = qi->getTarget();
				i->qi = qi;
			}
		}
	}
	dcdebug(("****** TransferView: ConnectionAdded "+Util::getFileName(i->Target)+" from user "+i->user->getNick()+"\n").c_str());
	PostMessage(WM_SPEAKER, ADD_ITEM, (LPARAM)i);
}

void TransferView::on(ConnectionManagerListener::StatusChanged, ConnectionQueueItem* aCqi) {
	ItemInfo* i;
	{
		Lock l(cs);
		dcassert(transferItems.find(aCqi) != transferItems.end());
		i = transferItems[aCqi];		
		i->statusString = aCqi->getState() == ConnectionQueueItem::CONNECTING ? STRING(CONNECTING) : STRING(WAITING_TO_RETRY);
		if(i->statusString == STRING(CONNECTING)) i->status = ItemInfo::STATUS_WAITING;
		else i->status = ItemInfo::STATUS_RUNNING;
		i->updateMask |= ItemInfo::MASK_STATUS;
		if (i->type == ItemInfo::TYPE_DOWNLOAD) {
			QueueItem* qi = QueueManager::getInstance()->lookupNext(aCqi->getUser());
			if (qi) {
				i->columns[COLUMN_FILE] = Util::getFileName(qi->getTarget());
				i->columns[COLUMN_PATH] = Util::getFilePath(qi->getTarget());
				i->columns[COLUMN_SIZE] = Util::formatBytes(qi->getSize());
				i->file = Util::getFileName(qi->getTarget());
				i->path = Util::getFilePath(qi->getTarget());
				i->size = qi->getSize();				
				i->updateMask |= (ItemInfo::MASK_USER | ItemInfo::MASK_HUB | ItemInfo::MASK_FILE | ItemInfo::MASK_PATH | ItemInfo::MASK_SIZE);
				i->Target = qi->getTarget();
				i->qi = qi;

			}
	
			dcdebug("OnConnectionStatus\n");
			if(i->Target != i->oldTarget) setMainItem(i);
			i->oldTarget = i->Target;

			if(i->upper != NULL) {
				if(i->qi && i->qi->getActiveSegments().size() < 1) {
					i->upper->statusString = i->statusString;
				}
				i->upper->columns[COLUMN_FILE] = Util::getFileName(i->qi->getTarget());
				i->upper->columns[COLUMN_PATH] = Util::getFilePath(i->qi->getTarget());
				i->upper->columns[COLUMN_SIZE] = i->columns[COLUMN_SIZE];
				i->upper->file = Util::getFileName(i->qi->getTarget());
				i->upper->path = Util::getFilePath(i->qi->getTarget());
				i->upper->size = i->qi->getSize();				
				i->upper->Target = i->Target;
			}
		}
	}

	dcdebug(("****** TransferView: StatusChanged "+Util::getFileName(i->Target)+" from user "+i->user->getNick()+"\n").c_str());	
	PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)i);
}

void TransferView::on(ConnectionManagerListener::Removed, ConnectionQueueItem* aCqi) {
	ItemInfo* i;
	ItemInfo* h; 
	bool isDownload = true;
	{
		Lock l(cs);
		dcdebug("onConnectionRemoved\n");
		ItemInfo::MapIter ii = transferItems.find(aCqi);
		dcassert(ii != transferItems.end());
		i = ii->second;
		h = i->upper;
		isDownload = (i->type == ItemInfo::TYPE_DOWNLOAD);
		transferItems.erase(ii);

		if((isDownload) && (h != NULL)) {
			dcdebug(("****** TransferView: PocetUseru "+ Util::toString(h->pocetUseru)+" -- "+Util::getFileName(i->Target)+" from user "+i->user->getNick()+"\n").c_str());
			h->pocetUseru -= 1;

			if(h->pocetUseru <= 0) {
				mainItems.erase(find(mainItems.begin(), mainItems.end(), h));
				PostMessage(WM_SPEAKER, REMOVE_ITEM, (LPARAM)h);
			} else {
				h->columns[COLUMN_USER] = Util::toString(h->pocetUseru)+" "+STRING(HUB_USERS);
				if(h->qi)
					h->columns[COLUMN_HUB] = Util::toString((int)h->qi->getActiveSegments().size())+" "+STRING(NUMBER_OF_SEGMENTS);
				h->updateMask |= (ItemInfo::MASK_USER | ItemInfo::MASK_HUB);
				PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)h);
			}
		}
	}

	dcdebug(("****** TransferView: ConnectionRemoved "+Util::getFileName(i->Target)+" from user "+i->user->getNick()+"\n").c_str());
	PostMessage(WM_SPEAKER, REMOVE_ITEM, (LPARAM)i);
}

void TransferView::on(ConnectionManagerListener::Failed, ConnectionQueueItem* aCqi, const string& aReason) {
	ItemInfo* i;
	{
		Lock l(cs);
		dcassert(transferItems.find(aCqi) != transferItems.end());
		i = transferItems[aCqi];		
		i->statusString = aReason;

		if(i->type != ItemInfo::TYPE_DOWNLOAD) {
			if(i->Target != i->oldTarget) setMainItem(i);
			i->oldTarget = i->Target;

			if((i->upper != NULL) && (i->qi->getActiveSegments().size() < 1)) {
				i->upper->statusString = aReason;
			}
		}
			
		i->updateMask |= (ItemInfo::MASK_HUB | ItemInfo::MASK_USER | ItemInfo::MASK_STATUS);
	}
	dcdebug(("****** TransferView: ConnectionFailed "+Util::getFileName(i->Target)+" from user "+i->user->getNick()+"\n").c_str());
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
		i->file = Util::getFileName(aDownload->getTarget());
		i->path = Util::getFilePath(aDownload->getTarget());
		QueueItem* qi = QueueManager::getInstance()->getRunning(aCqi->getUser());
		if(qi) i->qi = qi;
		i->Target = aDownload->getTarget();

		dcdebug(("OnDownloadStarting: " + aDownload->getTarget() +"\n").c_str());
		if(i->Target != i->oldTarget) setMainItem(i);
		i->oldTarget = i->Target;

		if(i->user != (User::Ptr)NULL) 
			if(i->upper != NULL) {	
				if(i->qi) i->upper->qi = i->qi;
				i->upper->status = ItemInfo::STATUS_RUNNING;
				i->upper->size = aDownload->getSize();
				string x = Util::getFileName(aDownload->getTarget());
				i->upper->file = x;
				i->upper->path = Util::getFilePath(aDownload->getTarget());
				i->upper->Target = aDownload->getTarget();
				i->upper->actual = aDownload->getQueueTotal();
				i->upper->downloadTarget = aDownload->getDownloadTarget();
				i->upper->finished = false;
				if(i->qi->getActiveSegments().size() <= 1)
					i->upper->statusString = STRING(DOWNLOAD_STARTING);
		}
				
		i->downloadTarget = aDownload->getDownloadTarget();
		i->statusString = STRING(DOWNLOAD_STARTING);
		i->IP = aDownload->getUserConnection()->getRemoteIp();
		i->country = Util::getIpCountry(aDownload->getUserConnection()->getRemoteIp());
		i->updateMask |= ItemInfo::MASK_USER | ItemInfo::MASK_HUB | ItemInfo::MASK_STATUS | ItemInfo::MASK_FILE | ItemInfo::MASK_PATH |
			ItemInfo::MASK_SIZE | ItemInfo::MASK_IP;

	}

	if(!aDownload->isSet(Download::FLAG_USER_LIST)) {
		if ((!SETTING(BEGINFILE).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
			PlaySound(SETTING(BEGINFILE).c_str(), NULL, SND_FILENAME | SND_ASYNC);
	}
	dcdebug(("****** TransferView: DownStarting "+Util::getFileName(i->Target)+" from user "+i->user->getNick()+"\n").c_str());
	PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)i);
}

void TransferView::on(DownloadManagerListener::Tick, const Download::List& dl) {
	vector<ItemInfo*>* v = new vector<ItemInfo*>();
	v->reserve(dl.size());

	char* buf = new char[STRING(DOWNLOADED_BYTES).size() + 64];

	{
		Lock l(cs);
		for(Download::List::const_iterator j = dl.begin(); j != dl.end(); ++j) {
			Download* d = *j;
			int64_t total = d->getQueueTotal();
			sprintf(buf, CSTRING(DOWNLOADED_BYTES), Util::formatBytes(total).c_str(), 
				(double)total*100.0/(double)d->getSize(), Util::formatSeconds((GET_TICK() - d->getStart())/1000).c_str());

			ConnectionQueueItem* aCqi = d->getUserConnection()->getCQI();
			ItemInfo* i = transferItems[aCqi];
			i->dwnldStart = d->getStart();
			i->actual = i->start + d->getActual();
			i->pos = i->start + d->getTotal();
			i->timeLeft = d->getSecondsLeft();
			i->speed = d->getRunningAverage();
			i->stazenoCelkem = total;
			QueueItem* qi = QueueManager::getInstance()->getRunning(aCqi->getUser());
			if(qi) i->qi = qi;			

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
			i->celkovaRychlost = tmp;

			if(i->upper != NULL) {
				if(i->qi) i->upper->qi = qi;
				i->upper->compressRatio = pomerKomprese;
				i->upper->celkovaRychlost = tmp;
				i->upper->statusString = buf;
				i->upper->actual = total;
				i->upper->stazenoCelkem = d->getQueueTotal();
				i->upper->pos = total;
				i->upper->size = d->getSize();
				(pomerKomprese < 1) ? i->upper->setFlag(ItemInfo::FLAG_COMPRESSED) : i->upper->unsetFlag(ItemInfo::FLAG_COMPRESSED);
			}	
			if (BOOLSETTING(SHOW_PROGRESS_BARS)) {
				d->isSet(Download::FLAG_ZDOWNLOAD) ? i->setFlag(ItemInfo::FLAG_COMPRESSED) : i->unsetFlag(ItemInfo::FLAG_COMPRESSED);
				i->statusString = buf;
			} else {
				if(d->isSet(Download::FLAG_ZDOWNLOAD)) {
					i->statusString = "* " + string(buf);
				} else {
					i->statusString = buf;
				}
			}
			i->updateMask |= ItemInfo::MASK_USER | ItemInfo::MASK_HUB | ItemInfo::MASK_STATUS | ItemInfo::MASK_TIMELEFT | ItemInfo::MASK_SPEED | ItemInfo::MASK_RATIO;

			v->push_back(i);

			if(d->getRunningAverage() <= 0) {
				d->getUserConnection()->reconnect();
			}
		}
	}
	delete[] buf;

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

		i->statusString = aReason;
		i->size = aDownload->getSize();
		i->file = Util::getFileName(aDownload->getTarget());
		i->path = Util::getFilePath(aDownload->getTarget());
		i->Target = aDownload->getTarget();
		i->downloadTarget = aDownload->getDownloadTarget();
		QueueItem* qi = QueueManager::getInstance()->getRunning(aCqi->getUser());
		if(qi) i->qi = qi;

		dcdebug("OnDownloadFailed\n");
		if(i->Target != i->oldTarget) setMainItem(i);
		i->oldTarget = i->Target;

		if(i->upper) {
			i->upper->downloadTarget = aDownload->getDownloadTarget();
			i->upper->file = Util::getFileName(aDownload->getTarget());
			i->upper->path = Util::getFilePath(aDownload->getTarget());
			i->upper->size = aDownload->getSize();

			if(i->qi) {
				i->upper->qi = i->qi;
				if(i->qi->getCurrents().size() <= 1) {
					i->upper->status = ItemInfo::STATUS_WAITING;
					i->upper->statusString = aReason;
				}
			}
		}

		i->updateMask |= ItemInfo::MASK_USER | ItemInfo::MASK_HUB | ItemInfo::MASK_STATUS | ItemInfo::MASK_SIZE | ItemInfo::MASK_FILE |
		ItemInfo::MASK_PATH;
	}
	dcdebug(("****** TransferView: DownFailed "+Util::getFileName(i->Target)+" from user "+i->user->getNick()+"\n").c_str());
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

		i->file = Util::getFileName(aUpload->getLocalFileName());
		i->path = Util::getFilePath(aUpload->getLocalFileName());
		i->statusString = STRING(UPLOAD_STARTING);
		i->IP = aUpload->getUserConnection()->getRemoteIp();
		i->country = Util::getIpCountry(aUpload->getUserConnection()->getRemoteIp());
		i->updateMask |= ItemInfo::MASK_STATUS | ItemInfo::MASK_FILE | ItemInfo::MASK_PATH |
			ItemInfo::MASK_SIZE | ItemInfo::MASK_IP;
	}

	PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)i);
}

void TransferView::on(UploadManagerListener::Tick, const Upload::List& ul) {
	vector<ItemInfo*>* v = new vector<ItemInfo*>();
	v->reserve(ul.size());

	char* buf = new char[STRING(UPLOADED_BYTES).size() + 64];

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
				sprintf(buf, CSTRING(UPLOADED_BYTES), Util::formatBytes(u->getPos()).c_str(), 
				(double)u->getPos()*100.0/(double)u->getSize(), Util::formatSeconds((GET_TICK() - u->getStart())/1000).c_str());
            } else sprintf(buf, CSTRING(UPLOAD_STARTING));
			if (BOOLSETTING(SHOW_PROGRESS_BARS)) {
				u->isSet(Upload::FLAG_ZUPLOAD) ? i->setFlag(ItemInfo::FLAG_COMPRESSED) : i->unsetFlag(ItemInfo::FLAG_COMPRESSED);
				i->statusString = buf;
			} else {
				if(u->isSet(Upload::FLAG_ZUPLOAD)) {
					i->statusString = "* " + string(buf);
				} else {
					i->statusString = buf;
				}
			}

			i->updateMask |= ItemInfo::MASK_STATUS | ItemInfo::MASK_TIMELEFT | ItemInfo::MASK_SPEED | ItemInfo::MASK_RATIO;
			v->push_back(i);
		}
	}

	delete[] buf;

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
				i->upper->statusString = STRING(DOWNLOAD_FINISHED_IDLE);
				i->upper->updateMask |= ItemInfo::MASK_STATUS | ItemInfo::MASK_HUB | ItemInfo::MASK_SPEED | ItemInfo::MASK_TIMELEFT;
				i->upper->finished = true;
			}
		}
		i->status = ItemInfo::STATUS_WAITING;
		i->pos = 0;

		i->statusString = isUpload ? STRING(UPLOAD_FINISHED_IDLE) : STRING(DOWNLOAD_FINISHED_IDLE);
		i->updateMask |= ItemInfo::MASK_HUB | ItemInfo::MASK_STATUS | ItemInfo::MASK_SPEED | ItemInfo::MASK_TIMELEFT;
	}
	dcdebug(("****** TransferView: Complete "+Util::getFileName(i->Target)+" from user "+i->user->getNick()+"\n").c_str());
	PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)i);	
}

void TransferView::ItemInfo::disconnect() {
	ConnectionManager::getInstance()->removeConnection(user, (type == TYPE_DOWNLOAD));
}

LRESULT TransferView::onSearchAlternates(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		string tmp;
		ItemInfo* ii = ctrlTransfers.getItemData(i);
		if(ii->qi != NULL) {
			if(ii->qi->getTTH() != NULL)
				SearchFrame::openWindow(ii->qi->getTTH()->toBase32(), 0, SearchManager::SIZE_DONTCARE, SearchManager::TYPE_HASH);
		} else {
			string searchString = SearchManager::clean(ii->file);
			StringList tok = StringTokenizer(searchString, ' ').getTokens();
		
			for(StringIter si = tok.begin(); si != tok.end(); ++si) {
				bool found = false;
			
				for(StringIter j = searchFilter.begin(); j != searchFilter.end(); ++j) {
					if(Util::stricmp(si->c_str(), j->c_str()) == 0) {
						found = true;
					}
				}
			
				if(!found && !si->empty()) {
					tmp += *si + ' ';
				}
			}
			if(!tmp.empty()) {
				if(ii->size > 10*1024*1024) {
					SearchFrame::openWindow(tmp, ii->size-1, SearchManager::SIZE_ATLEAST, ShareManager::getInstance()->getType(ii->file));
				} else {
					SearchFrame::openWindow(tmp, ii->size+1, SearchManager::SIZE_ATMOST, ShareManager::getInstance()->getType(ii->file));
				}
			}
		}
	}
	return 0;
}

LRESULT TransferView::onPreviewCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	string target = ctrlTransfers.getItemData(ctrlTransfers.GetNextItem(-1, LVNI_SELECTED))->downloadTarget;
	WinUtil::RunPreviewCommand(wID - IDC_PREVIEW_APP, target);
	return 0;
}

TransferView::ItemInfo* TransferView::findMainItem(string Target) {
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

void TransferView::InsertItem(ItemInfo* i) {

	Lock lck(cs);
	ItemInfo* l = findMainItem(i->Target);

	if(!l) {
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
		h->columns[COLUMN_STATUS] = h->statusString = STRING(CONNECTING);
		h->qi = i->qi;
		i->upper = h;
		mainItems.push_back(h);

		PostMessage(WM_SPEAKER, SET_STATE, (LPARAM)h);
	} else {
		i->upper = l;
		i->upper->pocetUseru += 1;

		if(i->upper->pocetUseru == 1) {
			i->upper->columns[COLUMN_USER] = i->user->getNick();
			i->upper->columns[COLUMN_HUB] = i->user->getClientName();
		} else {
			i->upper->columns[COLUMN_USER] = Util::toString(i->upper->pocetUseru)+" "+STRING(HUB_USERS);
			i->upper->columns[COLUMN_HUB] = Util::toString((int)i->qi->getActiveSegments().size())+" "+STRING(NUMBER_OF_SEGMENTS);
		}

		if(!i->upper->collapsed) {
			int r = ctrlTransfers.findItem(i->upper);
			insertSubItem(i,r+i->qi->getActiveSegments().size()+1);
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

		ItemInfo* newMainItem = findMainItem(i->Target);

		if(h->Target != i->Target) {
			dcdebug("3. cyklus\n");
			h->pocetUseru -= 1;

/*			if(h->pocetUseru > 1) {
				h->columns[COLUMN_USER] = Util::toString(h->pocetUseru)+" "+STRING(HUB_USERS);
				h->columns[COLUMN_HUB] = Util::toString((int)h->qi->getActiveSegments().size())+" "+STRING(NUMBER_OF_SEGMENTS);
			} else {
				h->columns[COLUMN_USER] = h->user->getNick();
				h->columns[COLUMN_HUB] = h->user->getClientName();
			}*/

			h->updateMask |= (ItemInfo::MASK_USER | ItemInfo::MASK_HUB);

			dcdebug(("****** TransferView: UpdateOldMainItem "+Util::getFileName(h->Target)+" from user "+h->user->getNick()+"\n").c_str());

			PostMessage(WM_SPEAKER, REMOVE_ITEM_BUT_NOT_FREE, (LPARAM)i);

			i->upper = newMainItem;
			InsertItem(i);

			dcdebug("4. cyklus\n");

			if(h->pocetUseru <= 0) {
				mainItems.erase(find(mainItems.begin(), mainItems.end(), h));
				dcdebug(("****** TransferView: RemoveOldMainItem "+Util::getFileName(h->Target)+" from user "+h->user->getNick()+"\n").c_str());
				PostMessage(WM_SPEAKER, REMOVE_ITEM, (LPARAM)h);
			} else PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)h);
		}
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
			ctrlTransfers.deleteItem(m);
		}
	}
	i->collapsed = true;
	ctrlTransfers.SetItemState(a, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
}

void TransferView::Expand(ItemInfo* i, int a) {
	int q = 0;
	for(ItemInfo::Map::iterator j = transferItems.begin(); j != transferItems.end(); ++j) {
		ItemInfo* m = j->second;
		if((m->Target == i->Target) && (m->type == ItemInfo::TYPE_DOWNLOAD)) {	
			q++;
			insertSubItem(m,a+1);
		}
	}

	if(q == 0) {
		mainItems.erase(find(mainItems.begin(), mainItems.end(), i));
		PostMessage(WM_SPEAKER, REMOVE_ITEM, (LPARAM)i);
	} else {
		i->collapsed = false;
		ctrlTransfers.SetItemState(a, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
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
			if((i->type == ItemInfo::TYPE_DOWNLOAD) && (i->mainItem) && (!i->qi || (!i->qi->isSet(QueueItem::FLAG_USER_LIST) && !i->qi->isSet(QueueItem::FLAG_TESTSUR))))
				if(i->collapsed) Expand(i,item->iItem); else Collapse(i,item->iItem);
		}
	}
	return 0;
} 

LRESULT TransferView::onConnectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlTransfers.GetSelectedCount() == 1) {
		int i = ctrlTransfers.GetNextItem(-1, LVNI_SELECTED);
		ItemInfo* ii = ctrlTransfers.getItemData(i);
		ctrlTransfers.SetItemText(i, COLUMN_STATUS, CSTRING(CONNECTING_FORCED));
		for(ItemInfo::Map::iterator j = transferItems.begin(); j != transferItems.end(); ++j) {
			ItemInfo* m = j->second;
			if((m->Target == ii->Target) && (m->type == ItemInfo::TYPE_DOWNLOAD)) {	
				int h = ctrlTransfers.findItem(m);
				if(h != -1)
					ctrlTransfers.SetItemText(h, COLUMN_STATUS, CSTRING(CONNECTING_FORCED));
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
		ctrlTransfers.SetItemText(i, COLUMN_STATUS, CSTRING(DISCONNECTED));
		for(ItemInfo::Map::iterator j = transferItems.begin(); j != transferItems.end(); ++j) {
			ItemInfo* m = j->second;
			if((m->Target == ii->Target) && (m->type == ItemInfo::TYPE_DOWNLOAD)) {	
				int h = ctrlTransfers.findItem(m);
				if(h != -1)
					ctrlTransfers.SetItemText(h, COLUMN_STATUS, CSTRING(DISCONNECTED));
				m->disconnect();
			}
		}
	}
	return 0;
}

/**
 * @file
 * $Id$
 */
