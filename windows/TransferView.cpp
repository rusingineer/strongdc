/*
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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
#include "MainFrm.h"

#include "BarShader.h"

int TransferView::columnIndexes[] = { COLUMN_USER, COLUMN_HUB, COLUMN_STATUS, COLUMN_TIMELEFT, COLUMN_SPEED, COLUMN_FILE, COLUMN_SIZE, COLUMN_PATH, COLUMN_IP, COLUMN_RATIO };
int TransferView::columnSizes[] = { 150, 100, 250, 75, 75, 175, 100, 200, 50, 75 };

static ResourceManager::Strings columnNames[] = { ResourceManager::USER, ResourceManager::HUB_SEGMENTS, ResourceManager::STATUS,
ResourceManager::TIME_LEFT, ResourceManager::SPEED, ResourceManager::FILENAME, ResourceManager::SIZE, ResourceManager::PATH,
ResourceManager::IP_BARE, ResourceManager::RATIO};

TransferView::~TransferView() {
	delete[] headerBuf;
	arrows.Destroy();
}

LRESULT TransferView::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	arrows.CreateFromImage(IDB_ARROWS, 16, 3, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	ctrlTransfers.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_TRANSFERS);
	ctrlTransfers.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | 0x00010000 | LVS_EX_INFOTIP);

	WinUtil::splitTokens(columnIndexes, SETTING(MAINFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(MAINFRAME_WIDTHS), COLUMN_LAST);

	for(int j=0; j<COLUMN_LAST; ++j) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_TIMELEFT || j == COLUMN_SPEED) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlTransfers.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlTransfers.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlTransfers.setVisible(SETTING(MAINFRAME_VISIBLE));

	ctrlTransfers.SetBkColor(WinUtil::bgColor);
	ctrlTransfers.SetTextBkColor(WinUtil::bgColor);
	ctrlTransfers.SetTextColor(WinUtil::textColor);
	ctrlTransfers.setFlickerFree(WinUtil::bgBrush);

	ctrlTransfers.SetImageList(arrows, LVSIL_SMALL);
	ctrlTransfers.setSortColumn(COLUMN_USER);

	transferMenu.CreatePopupMenu();
	appendUserItems(transferMenu);
	transferMenu.AppendMenu(MF_SEPARATOR);
	transferMenu.AppendMenu(MF_STRING, IDC_FORCE, CTSTRING(FORCE_ATTEMPT));
	transferMenu.AppendMenu(MF_SEPARATOR);
	transferMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES));

	previewMenu.CreatePopupMenu();
	previewMenu.AppendMenu(MF_SEPARATOR);
	usercmdsMenu.CreatePopupMenu();

	transferMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)previewMenu, CTSTRING(PREVIEW_MENU));
	transferMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)usercmdsMenu, CTSTRING(SETTINGS_USER_COMMANDS));
	transferMenu.AppendMenu(MF_SEPARATOR);
	transferMenu.AppendMenu(MF_STRING, IDC_MENU_SLOWDISCONNECT, CTSTRING(SETCZDC_DISCONNECTING_ENABLE));
	transferMenu.AppendMenu(MF_SEPARATOR);
	transferMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(CLOSE_CONNECTION));
	transferMenu.SetMenuDefaultItem(IDC_PRIVATEMESSAGE);

	segmentedMenu.CreatePopupMenu();
	segmentedMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES));
	segmentedMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)previewMenu, CTSTRING(PREVIEW_MENU));
	segmentedMenu.AppendMenu(MF_STRING, IDC_MENU_SLOWDISCONNECT, CTSTRING(SETCZDC_DISCONNECTING_ENABLE));
	segmentedMenu.AppendMenu(MF_SEPARATOR);
	segmentedMenu.AppendMenu(MF_STRING, IDC_CONNECT_ALL, CTSTRING(CONNECT_ALL));
	segmentedMenu.AppendMenu(MF_STRING, IDC_DISCONNECT_ALL, CTSTRING(DISCONNECT_ALL));
	segmentedMenu.AppendMenu(MF_SEPARATOR);
	segmentedMenu.AppendMenu(MF_STRING, IDC_EXPAND_ALL, CTSTRING(EXPAND_ALL));
	segmentedMenu.AppendMenu(MF_STRING, IDC_COLLAPSE_ALL, CTSTRING(COLLAPSE_ALL));
	segmentedMenu.AppendMenu(MF_SEPARATOR);
	segmentedMenu.AppendMenu(MF_STRING, IDC_REMOVEALL, CTSTRING(REMOVE_ALL));

	ConnectionManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);
	UploadManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	return 0;
}

void TransferView::prepareClose() {
	ctrlTransfers.saveHeaderOrder(SettingsManager::MAINFRAME_ORDER, SettingsManager::MAINFRAME_WIDTHS,
		SettingsManager::MAINFRAME_VISIBLE);

	ConnectionManager::getInstance()->removeListener(this);
	DownloadManager::getInstance()->removeListener(this);
	UploadManager::getInstance()->removeListener(this);
	SettingsManager::getInstance()->removeListener(this);
}

LRESULT TransferView::onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	RECT rc;
	GetClientRect(&rc);
	ctrlTransfers.MoveWindow(&rc);

	return 0;
}

LRESULT TransferView::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlTransfers && ctrlTransfers.GetSelectedCount() > 0) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlTransfers, pt);
		}
		
		if(ctrlTransfers.GetSelectedCount() > 0) {
			int i = -1;
			ItemInfo* itemI;
			bool bCustomMenu = false;
			ItemInfo* ii = ctrlTransfers.getItemData(ctrlTransfers.GetNextItem(-1, LVNI_SELECTED));
			bool main = ii->subItems.size() > 1;

			if(!main && (i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
				itemI = ctrlTransfers.getItemData(i);
				bCustomMenu = true;
	
				usercmdsMenu.InsertSeparatorFirst(TSTRING(SETTINGS_USER_COMMANDS));
	
				if(itemI->user != (User::Ptr)NULL)
					prepareMenu(usercmdsMenu, UserCommand::CONTEXT_CHAT, ClientManager::getInstance()->getHubs(itemI->user->getCID()));
			}
			WinUtil::ClearPreviewMenu(previewMenu);

			segmentedMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_UNCHECKED);
			transferMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_UNCHECKED);

			if(ii->download) {
				transferMenu.EnableMenuItem(IDC_SEARCH_ALTERNATES, MFS_ENABLED);
				transferMenu.EnableMenuItem(IDC_MENU_SLOWDISCONNECT, MFS_ENABLED);
				if(!ii->Target.empty()) {
					string target = Text::fromT(ii->Target);
					string ext = Util::getFileExt(target);
					if(ext.size()>1) ext = ext.substr(1);
					PreviewAppsSize = WinUtil::SetupPreviewMenu(previewMenu, ext);

					QueueItem::StringMap& queue = QueueManager::getInstance()->lockQueue();

					QueueItem::StringIter qi = queue.find(&target);

					bool slowDisconnect = false;
					if(qi != queue.end())
						slowDisconnect = qi->second->isSet(QueueItem::FLAG_AUTODROP);

					QueueManager::getInstance()->unlockQueue();

					if(slowDisconnect) {
						segmentedMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_CHECKED);
						transferMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_CHECKED);
					}
				}
			} else {
				transferMenu.EnableMenuItem(IDC_SEARCH_ALTERNATES, MFS_DISABLED);
				transferMenu.EnableMenuItem(IDC_MENU_SLOWDISCONNECT, MFS_DISABLED);
			}

			if(previewMenu.GetMenuItemCount() > 0) {
				segmentedMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_ENABLED);
				transferMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_ENABLED);
			} else {
				segmentedMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_DISABLED);
				transferMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_DISABLED);
			}

			previewMenu.InsertSeparatorFirst(TSTRING(PREVIEW_MENU));
				
			if(!main) {
				checkAdcItems(transferMenu);
				transferMenu.InsertSeparatorFirst(TSTRING(MENU_TRANSFERS));
				transferMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
				transferMenu.RemoveFirstItem();
			} else {
				segmentedMenu.InsertSeparatorFirst(TSTRING(SETTINGS_SEGMENT));
				segmentedMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
				segmentedMenu.RemoveFirstItem();
			}

			if ( bCustomMenu ) {
				WinUtil::ClearPreviewMenu(usercmdsMenu);
			}
			return TRUE; 
		}
	}
	bHandled = FALSE;
	return FALSE; 
}

void TransferView::runUserCommand(UserCommand& uc) {
	if(!WinUtil::getUCParams(m_hWnd, uc, ucLineParams))
		return;

	StringMap ucParams = ucLineParams;

	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ItemInfo* itemI = ctrlTransfers.getItemData(i);
		if(!itemI->user->isOnline())
			continue;

		StringMap tmp = ucParams;
		ucParams["fileFN"] = Text::fromT(itemI->Target);

		// compatibility with 0.674 and earlier
		ucParams["file"] = ucParams["fileFN"];
		
		ClientManager::getInstance()->userCommand(itemI->user, uc, tmp, true);
	}
}

LRESULT TransferView::onForce(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ItemInfo *ii = ctrlTransfers.getItemData(i);
		ii->columns[COLUMN_STATUS] = CTSTRING(CONNECTING_FORCED);
		ctrlTransfers.updateItem( ii );
		ClientManager::getInstance()->connect(ii->user);
	}
	return 0;
}

void TransferView::ItemInfo::removeAll() {
	if(subItems.size() <= 1) {
		QueueManager::getInstance()->removeSource(user, QueueItem::Source::FLAG_REMOVED);
	} else {
		if(!BOOLSETTING(CONFIRM_DELETE) || ::MessageBox(0, _T("Do you really want to remove this item?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
			QueueManager::getInstance()->remove(Text::fromT(Target));
	}
}

LRESULT TransferView::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	CRect rc;
	NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
		ItemInfo* ii = reinterpret_cast<ItemInfo*>(cd->nmcd.lItemlParam);

		int colIndex = ctrlTransfers.findColumn(cd->iSubItem);
		cd->clrTextBk = WinUtil::bgColor;

		if((ii->status == ItemInfo::STATUS_RUNNING) && (colIndex == COLUMN_STATUS) ) {
			if(!BOOLSETTING(SHOW_PROGRESS_BARS)) {
				bHandled = FALSE;
				return 0;
			}

			// Get the text to draw
			// Get the color of this bar
			COLORREF clr = SETTING(PROGRESS_OVERRIDE_COLORS) ? 
				(ii->download ? SETTING(DOWNLOAD_BAR_COLOR) : SETTING(UPLOAD_BAR_COLOR)) : 
				GetSysColor(COLOR_HIGHLIGHT);

			//this is just severely broken, msdn says GetSubItemRect requires a one based index
			//but it wont work and index 0 gives the rect of the whole item
			if(cd->iSubItem == 0) {
				//use LVIR_LABEL to exclude the icon area since we will be painting over that
				//later
				ctrlTransfers.GetItemRect((int)cd->nmcd.dwItemSpec, &rc, LVIR_LABEL);
			} else {
				ctrlTransfers.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, &rc);
			}
				
			// Real rc, the original one.
			CRect real_rc = rc;
			// We need to offset the current rc to (0, 0) to paint on the New dc
			rc.MoveToXY(0, 0);

			// Text rect
			CRect rc2 = rc;
            rc2.left += 6; // indented with 6 pixels
			rc2.right -= 2; // and without messing with the border of the cell				

			CDC cdc;
			cdc.CreateCompatibleDC(cd->nmcd.hdc);
			HBITMAP hBmp = CreateCompatibleBitmap(cd->nmcd.hdc,  real_rc.Width(),  real_rc.Height());

			HBITMAP pOldBmp = cdc.SelectBitmap(hBmp);
			HDC& dc = cdc.m_hDC;

			HFONT oldFont = (HFONT)SelectObject(dc, WinUtil::font);
			SetBkMode(dc, TRANSPARENT);
		
			if(BOOLSETTING(PROGRESSBAR_ODC_STYLE)) {
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

				// Draw the outline AND the background using pen+brush
				::Rectangle(dc, rc.left, rc.top, rc.left + (LONG)(rc.Width() * ii->getRatio() + 0.5), rc.bottom);

				// Reset pen
				DeleteObject(::SelectObject(dc, pOldPen));
				// Reset bg (brush)
				DeleteObject(::SelectObject(dc, oldBg));
			}

			// Draw the background and border of the bar
			if(ii->size == 0)
				ii->size = 1;

			if(!BOOLSETTING(PROGRESSBAR_ODC_STYLE)) {
				CBarShader statusBar(rc.bottom - rc.top, rc.right - rc.left, SETTING(PROGRESS_BACK_COLOR), ii->size);

				rc.right = rc.left + (int) (rc.Width() * ii->pos / ii->size); 
				if(!ii->download) {
					statusBar.FillRange(0, ii->start, HLS_TRANSFORM(clr, -20, 30));
					statusBar.FillRange(ii->start, ii->actual,  clr);
				} else {
					statusBar.FillRange(0, ii->actual, clr);
					if(ii->main)
						statusBar.FillRange(ii->start, ii->actual, SETTING(PROGRESS_SEGMENT_COLOR));
				}
				if(ii->pos > ii->actual)
					statusBar.FillRange(ii->actual, ii->pos, SETTING(PROGRESS_COMPRESS_COLOR));

				statusBar.Draw(cdc, rc.top, rc.left, SETTING(PROGRESS_3DDEPTH));
			} else {
				int right = rc.left + (int) ((int64_t)rc.Width() * ii->actual / ii->size);
                
				COLORREF a, b;
				OperaColors::EnlightenFlood(!ii->main ? clr : SETTING(PROGRESS_SEGMENT_COLOR), a, b);
				OperaColors::FloodFill(cdc, rc.left+1, rc.top+1, right, rc.bottom-1, a, b);
			}

			// Get the color of this text bar
			COLORREF oldcol = ::SetTextColor(dc, SETTING(PROGRESS_OVERRIDE_COLORS2) ? 
				(ii->download ? SETTING(PROGRESS_TEXT_COLOR_DOWN) : SETTING(PROGRESS_TEXT_COLOR_UP)) : 
				OperaColors::TextFromBackground(clr));

			//rc2.right = right;
			LONG top = rc2.top + (rc2.Height() - WinUtil::getTextHeight(cd->nmcd.hdc) - 1)/2 + 1;
			::ExtTextOut(dc, rc2.left, top, ETO_CLIPPED, rc2, ii->getText(COLUMN_STATUS).c_str(), ii->getText(COLUMN_STATUS).length(), NULL);

			SelectObject(dc, oldFont);
			::SetTextColor(dc, oldcol);

			// New way:
			BitBlt(cd->nmcd.hdc, real_rc.left, real_rc.top, real_rc.Width(), real_rc.Height(), dc, 0, 0, SRCCOPY);
			DeleteObject(cdc.SelectBitmap(pOldBmp));

			//bah crap, if we return CDRF_SKIPDEFAULT windows won't paint the icons
			//so we have to do it
			if(cd->iSubItem == 0){
				LVITEM lvItem;
				lvItem.iItem = cd->nmcd.dwItemSpec;
				lvItem.iSubItem = 0;
				lvItem.mask = LVIF_IMAGE | LVIF_STATE;
				lvItem.stateMask = LVIS_SELECTED;
				ctrlTransfers.GetItem(&lvItem);

				HIMAGELIST imageList = (HIMAGELIST)::SendMessage(ctrlTransfers.m_hWnd, LVM_GETIMAGELIST, LVSIL_SMALL, 0);
				if(imageList) {
					//let's find out where to paint it
					//and draw the background to avoid having 
					//the selection color as background
					CRect iconRect;
					ctrlTransfers.GetSubItemRect((int)cd->nmcd.dwItemSpec, 0, LVIR_ICON, iconRect);
					ImageList_Draw(imageList, lvItem.iImage, cd->nmcd.hdc, iconRect.left, iconRect.top, ILD_TRANSPARENT);
				}
			}
			return CDRF_SKIPDEFAULT;
		} else if(BOOLSETTING(GET_USER_COUNTRY) && (colIndex == COLUMN_IP)) {
			ItemInfo* ii = (ItemInfo*)cd->nmcd.lItemlParam;
			ctrlTransfers.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);
			COLORREF color;
			if(ctrlTransfers.GetItemState((int)cd->nmcd.dwItemSpec, LVIS_SELECTED) & LVIS_SELECTED) {
				if(ctrlTransfers.m_hWnd == ::GetFocus()) {
					color = GetSysColor(COLOR_HIGHLIGHT);
					SetBkColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHT));
					SetTextColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
				} else {
					color = GetBkColor(cd->nmcd.hdc);
					SetBkColor(cd->nmcd.hdc, color);
				}				
			} else {
				color = WinUtil::bgColor;
				SetBkColor(cd->nmcd.hdc, WinUtil::bgColor);
				SetTextColor(cd->nmcd.hdc, WinUtil::textColor);
			}
			CRect rc2 = rc;
			rc2.left += 2;
			HGDIOBJ oldpen = ::SelectObject(cd->nmcd.hdc, CreatePen(PS_SOLID,0, color));
			HGDIOBJ oldbr = ::SelectObject(cd->nmcd.hdc, CreateSolidBrush(color));
			Rectangle(cd->nmcd.hdc,rc.left, rc.top, rc.right, rc.bottom);

			DeleteObject(::SelectObject(cd->nmcd.hdc, oldpen));
			DeleteObject(::SelectObject(cd->nmcd.hdc, oldbr));

			TCHAR buf[256];
			ctrlTransfers.GetItemText((int)cd->nmcd.dwItemSpec, cd->iSubItem, buf, 255);
			buf[255] = 0;
			if(_tcslen(buf) > 0) {
				LONG top = rc2.top + (rc2.Height() - 15)/2;
				if((top - rc2.top) < 2)
					top = rc2.top + 1;

				POINT p = { rc2.left, top };
				WinUtil::flagImages.Draw(cd->nmcd.hdc, ii->flagImage, p, LVSIL_SMALL);
				top = rc2.top + (rc2.Height() - WinUtil::getTextHeight(cd->nmcd.hdc) - 1)/2;
				::ExtTextOut(cd->nmcd.hdc, rc2.left + 30, top + 1, ETO_CLIPPED, rc2, buf, _tcslen(buf), NULL);
				return CDRF_SKIPDEFAULT;
			}
		} else if((colIndex != COLUMN_USER) && (colIndex != COLUMN_HUB) && (colIndex != COLUMN_STATUS) && (colIndex != COLUMN_IP) &&
			(ii->status != ItemInfo::STATUS_RUNNING)) {
			cd->clrText = OperaColors::blendColors(WinUtil::bgColor, WinUtil::textColor, 0.4);
			return CDRF_NEWFONT;
		}
		// Fall through
	}
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
		if(!i->multiSource || i->subItems.size() == 0) {
			switch(SETTING(TRANSFERLIST_DBLCLICK)) {
				case 0:
					ctrlTransfers.getItemData(item->iItem)->pm();
					break;
				case 1:
					ctrlTransfers.getItemData(item->iItem)->getList();
					break;
				case 2:
					ctrlTransfers.getItemData(item->iItem)->matchQueue();
					break;
				case 4:
					ctrlTransfers.getItemData(item->iItem)->addFav();
					break;
			}
		}
	}
	return 0;
}

LRESULT TransferView::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(wParam == ADD_ITEM) {
		auto_ptr<UpdateInfo> ui(reinterpret_cast<UpdateInfo*>(lParam));
		ItemInfo* ii = new ItemInfo(ui->user, ui->download);
		transferItems.push_back(ii);
		ii->update(*ui);
		if(ui->download) {
			ctrlTransfers.insertGroupedItem(ii, false, ui->fileList);
			ii->main->multiSource = ii->multiSource;
		} else {
			ctrlTransfers.insertItem(ii, IMAGE_UPLOAD);
		}
	} else if(wParam == REMOVE_ITEM) {
		auto_ptr<UpdateInfo> ui(reinterpret_cast<UpdateInfo*>(lParam));
		for(ItemInfo::List::iterator i = transferItems.begin(); i != transferItems.end(); ++i) {
			ItemInfo* ii = *i;
			if(*ui == *ii) {
				transferItems.erase(i);
				if(ui->download) {
					ctrlTransfers.removeGroupedItem(ii);
				} else {
					ctrlTransfers.deleteItem(ii);
					delete ii;
				}
				break;
			}
		}
	} else if(wParam == UPDATE_ITEM) {
		auto_ptr<UpdateInfo> ui(reinterpret_cast<UpdateInfo*>(lParam));
		for(ItemInfo::Iter i = transferItems.begin(); i != transferItems.end(); ++i) {
			ItemInfo* ii = *i;
			if(*ui == *ii) {
				if(ui->download) {
					ii->update(*ui);
					if(ii->main) {
						if(ui->updateMask && UpdateInfo::MASK_FILE) setMainItem(ii);
						ItemInfo* main = ii->main;

						main->multiSource = ii->multiSource;
						bool defString = false;
								
						switch(ii->status) {
							case ItemInfo::DOWNLOAD_STARTING:
								ii->status = ItemInfo::STATUS_RUNNING;
								if(main->status != ItemInfo::STATUS_RUNNING) {
									main->fileBegin = GET_TICK();

									if ((!SETTING(BEGINFILE).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
										PlaySound(Text::toT(SETTING(BEGINFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);

									if(BOOLSETTING(POPUP_DOWNLOAD_START)) {
										MainFrame::getMainFrame()->ShowBalloonTip((
											TSTRING(FILE) + _T(": ")+ Util::getFileName(ii->Target) + _T("\n")+
											TSTRING(USER) + _T(": ") + Text::toT(ii->user->getFirstNick())).c_str(), CTSTRING(DOWNLOAD_STARTING));
									}
									main->start = 0;
									main->actual = 0;
									main->pos = 0;
									main->status = ItemInfo::DOWNLOAD_STARTING;
								}
								break;
							case ItemInfo::DOWNLOAD_FINISHED:
								ii->status = ItemInfo::STATUS_WAITING;
								main->status = ItemInfo::STATUS_WAITING;
								main->columns[COLUMN_STATUS] = TSTRING(DOWNLOAD_FINISHED_IDLE);
								break;
							default:
								defString = true;
						}
						if(mainItemTick(main, true) && defString && ui->transferFailed)
							main->columns[COLUMN_STATUS] = ii->columns[COLUMN_STATUS];
					
						if(!main->collapsed)
							ctrlTransfers.updateItem(ii);
						ctrlTransfers.updateItem(main);
					}				
				} else {
					ii->update(*ui);
					ctrlTransfers.updateItem(ii);
				}
				break;
			}
		}
	} else if(wParam == UPDATE_ITEMS) {
		auto_ptr<vector<UpdateInfo*> > v(reinterpret_cast<vector<UpdateInfo*>* >(lParam));
		set<ItemInfo*> mainItems;
		ctrlTransfers.SetRedraw(FALSE);
		for(ItemInfo::Iter i = transferItems.begin(); i != transferItems.end(); ++i) {
			ItemInfo* ii = *i;
			for(vector<UpdateInfo*>::const_iterator j = v->begin(); j != v->end(); ++j) {
				UpdateInfo* ui = *j;
				if(*ui == *ii) {
					if(ii->main)
						mainItems.insert(ii->main);
					ii->update(*ui);
					ctrlTransfers.updateItem(ii);
				}
			}
		}

		for(set<ItemInfo*>::const_iterator i = mainItems.begin(); i != mainItems.end(); ++i) {
			ItemInfo* main = *i;
			mainItemTick(main, false);
			ctrlTransfers.updateItem(main);
		}
		
		if(ctrlTransfers.getSortColumn() != COLUMN_STATUS)
			ctrlTransfers.resort();
		ctrlTransfers.SetRedraw(TRUE);

		for_each(v->begin(), v->end(), DeleteFunction());
	}

	return 0;
}

LRESULT TransferView::onSearchAlternates(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ItemInfo *ii = ctrlTransfers.getItemData(i);

		string target = Text::fromT(ii->getText(COLUMN_PATH) + ii->getText(COLUMN_FILE));

		TTHValue tth;
		if(QueueManager::getInstance()->getTTH(Text::fromT(ii->Target), tth)) {
			WinUtil::searchHash(tth);
		}
	}

	return 0;
}
	
TransferView::ItemInfo::ItemInfo(const User::Ptr& u, bool aDownload) : UserInfoBase(u), download(aDownload), transferFailed(false),
	status(STATUS_WAITING), pos(0), size(0), start(0), actual(0), speed(0), timeLeft(0),
	Target(Util::emptyStringT), flagImage(0), collapsed(true), main(NULL)
{ 
	columns[COLUMN_USER] = Text::toT(u->getFirstNick());
	columns[COLUMN_HUB] = WinUtil::getHubNames(u).first;
}

void TransferView::ItemInfo::update(const UpdateInfo& ui) {
	if(ui.updateMask & UpdateInfo::MASK_STATUS) {
		status = ui.status;
	}
	if(ui.updateMask & UpdateInfo::MASK_STATUS_STRING) {
		// No slots etc from transfermanager better than disconnected from connectionmanager
		if(!transferFailed)
			columns[COLUMN_STATUS] = ui.statusString;
		transferFailed = ui.transferFailed;
	}
	if(ui.updateMask & UpdateInfo::MASK_SIZE) {
		size = ui.size;
		columns[COLUMN_SIZE] = Util::formatBytesW(size);
	}
	if(ui.updateMask & UpdateInfo::MASK_START) {
		start = ui.start;
	}
	if(ui.updateMask & UpdateInfo::MASK_POS) {
		pos = start + ui.pos;
	}
	if(ui.updateMask & UpdateInfo::MASK_ACTUAL) {
		actual = start + ui.actual;
		columns[COLUMN_RATIO] = Util::toStringW(getRatio());
	}
	if(ui.updateMask & UpdateInfo::MASK_SPEED) {
		speed = ui.speed;
		if (status == STATUS_RUNNING) {
			columns[COLUMN_SPEED] =Util::formatBytesW(speed) + _T("/s");
		} else {
			columns[COLUMN_SPEED] = Util::emptyStringT;
		}
	}
	if(ui.updateMask & UpdateInfo::MASK_FILE) {
		if(ui.download) Target = ui.target;
		columns[COLUMN_FILE] = ui.file;
		columns[COLUMN_PATH] = ui.path;
	}
	if(ui.updateMask & UpdateInfo::MASK_TIMELEFT) {
		timeLeft = ui.timeLeft;
		if (status == STATUS_RUNNING) {
			columns[COLUMN_TIMELEFT] = Util::formatSeconds(timeLeft);
		} else {
			columns[COLUMN_TIMELEFT] = Util::emptyStringT;
		}
	}
	if(ui.updateMask & UpdateInfo::MASK_IP) {
		flagImage = ui.flagImage;
		columns[COLUMN_IP] = ui.IP;
		if(main && main->subItems.size() == 1) {
			main->flagImage = flagImage;
			main->columns[COLUMN_IP] = ui.IP;
		}
	}
	if(ui.updateMask & UpdateInfo::MASK_SEGMENT) {
		multiSource = ui.multiSource;
	}
}

void TransferView::on(ConnectionManagerListener::Added, ConnectionQueueItem* aCqi) {
	UpdateInfo* ui = new UpdateInfo(aCqi->getUser(), aCqi->getDownload());

	if(ui->download) {
		string aTarget; int64_t aSize; int aFlags; bool segmented;
		if(QueueManager::getInstance()->getQueueInfo(aCqi->getUser(), aTarget, aSize, aFlags, ui->fileList, segmented)) {
			ui->setMultiSource(segmented);
			ui->setFile(Text::toT(aTarget));
			ui->setSize(aSize);
		}
	}

	ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setStatusString(TSTRING(CONNECTING));

	speak(ADD_ITEM, ui);
}

void TransferView::on(ConnectionManagerListener::StatusChanged, ConnectionQueueItem* aCqi) {
	UpdateInfo* ui = new UpdateInfo(aCqi->getUser(), aCqi->getDownload());
	string aTarget;	int64_t aSize; int aFlags = 0; bool segmented;

	if(QueueManager::getInstance()->getQueueInfo(aCqi->getUser(), aTarget, aSize, aFlags, ui->fileList, segmented)) {
		ui->setMultiSource(segmented);
		ui->setFile(Text::toT(aTarget));
		ui->setSize(aSize);
	}

	ui->setStatusString((aFlags & QueueItem::FLAG_TESTSUR) ? TSTRING(CHECKING_CLIENT) : TSTRING(CONNECTING));
	ui->setStatus(ItemInfo::STATUS_WAITING);

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(ConnectionManagerListener::Removed, ConnectionQueueItem* aCqi) {
	speak(REMOVE_ITEM, new UpdateInfo(aCqi->getUser(), aCqi->getDownload()));
}

void TransferView::on(ConnectionManagerListener::Failed, ConnectionQueueItem* aCqi, const string& aReason) {
	UpdateInfo* ui = new UpdateInfo(aCqi->getUser(), aCqi->getDownload());
	if(aCqi->getUser()->isSet(User::OLD_CLIENT)) {
		ui->setStatusString(TSTRING(SOURCE_TOO_OLD));
	} else {
		ui->setStatusString(Text::toT(aReason));
	}
	ui->setStatus(ItemInfo::STATUS_WAITING);
	speak(UPDATE_ITEM, ui);
}

void TransferView::on(DownloadManagerListener::Starting, Download* aDownload) {
	UpdateInfo* ui = new UpdateInfo(aDownload->getUserConnection()->getUser(), true);
	bool chunkInfo = aDownload->isSet(Download::FLAG_MULTI_CHUNK) && !aDownload->isSet(Download::FLAG_TREE_DOWNLOAD);

	ui->setStatus(ItemInfo::DOWNLOAD_STARTING);
	ui->setPos(chunkInfo ? 0 : aDownload->getTotal());
	ui->setActual(chunkInfo ? 0 : aDownload->getActual());
	ui->setStart(chunkInfo ? 0 : aDownload->getPos());
	ui->setSize(chunkInfo ? aDownload->getChunkSize() : aDownload->getSize());
	ui->setFile(Text::toT(aDownload->getTarget()));
	ui->setStatusString(TSTRING(DOWNLOAD_STARTING));
	ui->setMultiSource(aDownload->isSet(Download::FLAG_MULTI_CHUNK));
	tstring country = Text::toT(Util::getIpCountry(aDownload->getUserConnection()->getRemoteIp()));
	tstring ip = Text::toT(aDownload->getUserConnection()->getRemoteIp());
	if(country.empty()) {
		ui->setIP(ip);
	} else {
		ui->flagImage = WinUtil::getFlagImage(Text::fromT(country).c_str());
		ui->setIP(country + _T(" (") + ip + _T(")"));
	}
	if(aDownload->isSet(Download::FLAG_TREE_DOWNLOAD)) {
		ui->file = _T("TTH: ") + ui->file;
		ui->setStatus(ItemInfo::TREE_DOWNLOAD);
	}

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(DownloadManagerListener::Tick, const Download::List& dl) {
	vector<UpdateInfo*>* v = new vector<UpdateInfo*>();
	v->reserve(dl.size());

	AutoArray<TCHAR> buf(TSTRING(DOWNLOADED_BYTES).size() + 64);

	for(Download::List::const_iterator j = dl.begin(); j != dl.end(); ++j) {
		Download* d = *j;

		UpdateInfo* ui = new UpdateInfo(d->getUserConnection()->getUser(), true);
		ui->setActual(d->getActual());
		ui->setPos(d->getTotal());
		ui->setTimeLeft(d->getSecondsLeft());
		ui->setSpeed(d->getRunningAverage());

		if(d->isSet(Download::FLAG_MULTI_CHUNK) && !d->isSet(Download::FLAG_TREE_DOWNLOAD)) {
			ui->setSize(d->isSet(Download::FLAG_MULTI_CHUNK) ? d->getChunkSize() : d->getSize());
			ui->timeLeft = (ui->speed > 0) ? ((ui->size - d->getTotal()) / ui->speed) : 0;

			double progress = (double)(d->getTotal())*100.0/(double)ui->size;
			_stprintf(buf, CTSTRING(DOWNLOADED_BYTES), Util::formatBytesW(d->getTotal()).c_str(), 
				progress, Util::formatSeconds((GET_TICK() - d->getStart())/1000).c_str());
			if(progress > 100) {
				// workaround to fix > 100% percentage
				d->getUserConnection()->disconnect();
				continue;
			}
		} else {
			_stprintf(buf, CTSTRING(DOWNLOADED_BYTES), Util::formatBytesW(d->getPos()).c_str(), 
				(double)d->getPos()*100.0/(double)d->getSize(), Util::formatSeconds((GET_TICK() - d->getStart())/1000).c_str());
		}

		tstring statusString;

		if(d->isSet(Download::FLAG_PARTIAL)) {
			statusString += _T("[P]");
		}
		if(d->getUserConnection()->isSecure()) {
			if(d->getUserConnection()->isTrusted()) {
				statusString += _T("[S]");
			} else {
				statusString += _T("[U]");
			}
		}
		if(d->isSet(Download::FLAG_TTH_CHECK)) {
			statusString += _T("[T]");
		}
		if(d->isSet(Download::FLAG_ZDOWNLOAD)) {
			statusString += _T("[Z]");
		}
		if(d->isSet(Download::FLAG_CHUNKED)) {
			statusString += _T("[C]");
		}
		if(d->isSet(Download::FLAG_ROLLBACK)) {
			statusString += _T("[R]");
		}
		if(!statusString.empty()) {
			statusString += _T(" ");
		}
		statusString += buf;
		ui->setStatusString(statusString);
		if((d->getRunningAverage() == 0) && ((GET_TICK() - d->getStart()) > 1000)) {
			d->getUserConnection()->disconnect();
		}
			
		v->push_back(ui);
	}

	speak(UPDATE_ITEMS, v);
}

void TransferView::on(DownloadManagerListener::Failed, Download* aDownload, const string& aReason) {
	UpdateInfo* ui = new UpdateInfo(aDownload->getUserConnection()->getUser(), true, true);
	ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setPos(0);
	ui->setStatusString(Text::toT(aReason));
	ui->setSize(aDownload->getSize());
	ui->setFile(Text::toT(aDownload->getTarget()));
	tstring country = Text::toT(Util::getIpCountry(aDownload->getUserConnection()->getRemoteIp()));
	tstring ip = Text::toT(aDownload->getUserConnection()->getRemoteIp());
	if(country.empty()) {
		ui->setIP(ip);
	} else {
		ui->flagImage = WinUtil::getFlagImage(Text::fromT(country).c_str());
		ui->setIP(country + _T(" (") + ip + _T(")"));
	}
	if(BOOLSETTING(POPUP_DOWNLOAD_FAILED)) {
		MainFrame::getMainFrame()->ShowBalloonTip((
			TSTRING(FILE)+_T(": ") + ui->file + _T("\n")+
			TSTRING(USER)+_T(": ") + Text::toT(ui->user->getFirstNick()) + _T("\n")+
			TSTRING(REASON)+_T(": ") + Text::toT(aReason)).c_str(), CTSTRING(DOWNLOAD_FAILED), NIIF_WARNING);
	}
	if(aDownload->isSet(Download::FLAG_TREE_DOWNLOAD)) {
		ui->file = _T("TTH: ") + ui->file;
	}

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(DownloadManagerListener::Status, const User::Ptr& aUser, const string& aMessage) {
	{
		Lock l(cs);
		for(ItemInfo::List::iterator i = transferItems.begin(); i != transferItems.end(); ++i) {
			ItemInfo* ii = *i;
			if(ii->download && (ii->user == aUser)) {
				if(ii->status == ItemInfo::STATUS_WAITING) return;
				break;
			}
		}
	}
	UpdateInfo* ui = new UpdateInfo(aUser, true, aMessage != STRING(WAITING_TO_RETRY));
	ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setStatusString(Text::toT(aMessage));
	speak(UPDATE_ITEM, ui);
}

void TransferView::on(UploadManagerListener::Starting, Upload* aUpload) {
	UpdateInfo* ui = new UpdateInfo(aUpload->getUserConnection()->getUser(), false);

	ui->setStatus(ItemInfo::STATUS_RUNNING);
	ui->setPos(aUpload->getTotal());
	ui->setActual(aUpload->getActual());
	ui->setStart(aUpload->getPos());
	ui->setSize(aUpload->isSet(Upload::FLAG_TTH_LEAVES) ? aUpload->getSize() : aUpload->getFileSize());
	ui->setFile(Text::toT(aUpload->getLocalFileName()));

	if(!aUpload->isSet(Upload::FLAG_RESUMED)) {
		aUpload->unsetFlag(Upload::FLAG_RESUMED);
		ui->setStatusString(TSTRING(UPLOAD_STARTING));
	}

	tstring country = Text::toT(Util::getIpCountry(aUpload->getUserConnection()->getRemoteIp()));
	tstring ip = Text::toT(aUpload->getUserConnection()->getRemoteIp());
	if(country.empty()) {
		ui->setIP(ip);
	} else {
		ui->flagImage = WinUtil::getFlagImage(Text::fromT(country).c_str());
		ui->setIP(country + _T(" (") + ip + _T(")"));
	}
	if(aUpload->isSet(Upload::FLAG_TTH_LEAVES)) {
		ui->file = _T("TTH: ") + ui->file;
	}

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(UploadManagerListener::Tick, const Upload::List& ul) {
	vector<UpdateInfo*>* v = new vector<UpdateInfo*>();
	v->reserve(ul.size());

	AutoArray<TCHAR> buf(TSTRING(UPLOADED_BYTES).size() + 64);

	for(Upload::List::const_iterator j = ul.begin(); j != ul.end(); ++j) {
		Upload* u = *j;

		if (u->getTotal() == 0) continue;

		UpdateInfo* ui = new UpdateInfo(u->getUserConnection()->getUser(), false);
		ui->setActual(u->getActual());
		ui->setPos(u->getTotal());
		ui->setTimeLeft(u->getSecondsLeft(true)); // we are interested when whole file is finished and not only one chunk
		ui->setSpeed(u->getRunningAverage());

		_stprintf(buf, CTSTRING(UPLOADED_BYTES), Util::formatBytesW(u->getPos()).c_str(), 
			(double)u->getPos()*100.0/(double)(u->isSet(Upload::FLAG_TTH_LEAVES) ? u->getSize() : u->getFileSize()), Util::formatSeconds((GET_TICK() - u->getStart())/1000).c_str());

		tstring statusString;

		if(u->isSet(Upload::FLAG_PARTIAL_SHARE)) {
			statusString += _T("[P]");
		}
		if(u->isSet(Upload::FLAG_CHUNKED)) {
			statusString += _T("[C]");
		}
		if(u->getUserConnection()->isSecure()) {
			statusString += _T("[S]");
		}
		if(u->isSet(Upload::FLAG_ZUPLOAD)) {
			statusString += _T("[Z]");
		}

		if(!statusString.empty()) {
			statusString += _T(" ");
		}			
		statusString += buf;
		ui->setStatusString(statusString);
					
		v->push_back(ui);
		if((u->getRunningAverage() == 0) && ((GET_TICK() - u->getStart()) > 1000)) {
			u->getUserConnection()->disconnect(true);
		}
	}

	speak(UPDATE_ITEMS, v);
}

void TransferView::onTransferComplete(Transfer* aTransfer, bool isUpload, const string& aFileName, bool isTree) {
	UpdateInfo* ui = new UpdateInfo(aTransfer->getUserConnection()->getUser(), !isUpload);
	if(!isUpload) {
		ui->setStatus(isTree ? ItemInfo::STATUS_WAITING : ItemInfo::DOWNLOAD_FINISHED);
		if(BOOLSETTING(POPUP_DOWNLOAD_FINISHED) && !isTree) {
			MainFrame::getMainFrame()->ShowBalloonTip((
				TSTRING(FILE) + _T(": ") + Text::toT(aFileName) + _T("\n")+
				TSTRING(USER) + _T(": ") + Text::toT(aTransfer->getUserConnection()->getUser()->getFirstNick())).c_str(), CTSTRING(DOWNLOAD_FINISHED_IDLE));
		}
	} else {
		ui->setStatus(ItemInfo::STATUS_WAITING);
		if(BOOLSETTING(POPUP_UPLOAD_FINISHED) && !isTree) {
			MainFrame::getMainFrame()->ShowBalloonTip((
				TSTRING(FILE) + _T(": ") + Text::toT(aFileName) + _T("\n")+
				TSTRING(USER) + _T(": ") + Text::toT(aTransfer->getUserConnection()->getUser()->getFirstNick())).c_str(), CTSTRING(UPLOAD_FINISHED_IDLE));
		}
	}
	
	ui->setPos(0);
	ui->setStatusString(isUpload ? TSTRING(UPLOAD_FINISHED_IDLE) : TSTRING(DOWNLOAD_FINISHED_IDLE));

	speak(UPDATE_ITEM, ui);
}

void TransferView::ItemInfo::disconnect() {
	ConnectionManager::getInstance()->disconnect(user, download);
}
		
LRESULT TransferView::onPreviewCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ItemInfo *ii = ctrlTransfers.getItemData(i);

		QueueItem::StringMap& queue = QueueManager::getInstance()->lockQueue();

		string tmp = Text::fromT(ii->Target);
		QueueItem::StringIter qi = queue.find(&tmp);

		string aTempTarget;
		if(qi != queue.end())
			aTempTarget = qi->second->getTempTarget();

		QueueManager::getInstance()->unlockQueue();

		WinUtil::RunPreviewCommand(wID - IDC_PREVIEW_APP, aTempTarget);
	}

	return 0;
}

void TransferView::CollapseAll() {
	for(int q = ctrlTransfers.GetItemCount()-1; q != -1; --q) {
		ItemInfo* m = (ItemInfo*)ctrlTransfers.getItemData(q);
		if(m->download && m->main) {
			ctrlTransfers.deleteItem(m); 
		}
		if(m->download && !m->main) {
			m->collapsed = true;
			ctrlTransfers.SetItemState(ctrlTransfers.findItem(m), INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
		 }
	}
}

void TransferView::ExpandAll() {
	for(vector<ItemInfo*>::const_iterator i = ctrlTransfers.mainItems.begin(); i != ctrlTransfers.mainItems.end(); ++i) {
		if((*i)->collapsed) {
			ctrlTransfers.Expand(*i, ctrlTransfers.findItem(*i));
		}
	}
}

LRESULT TransferView::onConnectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ItemInfo* ii = ctrlTransfers.getItemData(i);
		ctrlTransfers.SetItemText(i, COLUMN_STATUS, CTSTRING(CONNECTING_FORCED));
		for(ItemInfo::Iter j = ii->subItems.begin(); j != ii->subItems.end(); ++j) {
			int h = ctrlTransfers.findItem(*j);
			if(h != -1)
				ctrlTransfers.SetItemText(h, COLUMN_STATUS, CTSTRING(CONNECTING_FORCED));
			ClientManager::getInstance()->connect((*j)->user);
		}
	}
	return 0;
}

LRESULT TransferView::onDisconnectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ItemInfo* ii = ctrlTransfers.getItemData(i);
		ctrlTransfers.SetItemText(i, COLUMN_STATUS, CTSTRING(DISCONNECTED));
		for(ItemInfo::Iter j = ii->subItems.begin(); j != ii->subItems.end(); ++j) {
			int h = ctrlTransfers.findItem(*j);
			if(h != -1)
				ctrlTransfers.SetItemText(h, COLUMN_STATUS, CTSTRING(DISCONNECTED));
			(*j)->disconnect();
		}
	}
	return 0;
}

LRESULT TransferView::onSlowDisconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ItemInfo *ii = ctrlTransfers.getItemData(i);

		QueueItem::StringMap& queue = QueueManager::getInstance()->lockQueue();

		string tmp = Text::fromT(ii->Target);
		QueueItem::StringIter qi = queue.find(&tmp);

		if(qi != queue.end()) {
			if(qi->second->isSet(QueueItem::FLAG_AUTODROP)) {
				qi->second->unsetFlag(QueueItem::FLAG_AUTODROP);
			} else {
				qi->second->setFlag(QueueItem::FLAG_AUTODROP);
			}
		}

		QueueManager::getInstance()->unlockQueue();
	}

	return 0;
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

bool TransferView::mainItemTick(ItemInfo* main, bool smallUpdate) {
	size_t totalSpeed = 0;	double ratio = 0; int segs = 0;
	ItemInfo* l = NULL;

	for(ItemInfo::Iter k = main->subItems.begin(); k != main->subItems.end(); ++k) {
		l = *k;
		if(l->status == ItemInfo::STATUS_RUNNING && main->Target == l->Target) {
			segs++;
			if(main->multiSource) {
				totalSpeed += (size_t)l->speed;
				ratio += l->getRatio();
			} else
				break;
		}
	}

	if(segs == 0) {
		main->pos = 0;
		main->actual = 0;
		main->status = ItemInfo::STATUS_WAITING;
		main->fileBegin = 0;
		main->timeLeft = 0;
		main->speed = 0;
		main->columns[COLUMN_TIMELEFT] = Util::emptyStringT;
		main->columns[COLUMN_SPEED] = Util::emptyStringT;
		main->columns[COLUMN_RATIO] = Util::emptyStringT;
		if(main->multiSource && main->subItems.size() > 1)
			main->columns[COLUMN_HUB] = _T("0 ") + TSTRING(NUMBER_OF_SEGMENTS);
		
		return true;
	} else if(!main->multiSource) {
 		main->status = l->status;
		main->size = l->size;
		main->pos = l->pos;
		main->actual = l->actual;
		main->speed = l->speed;
		main->timeLeft = l->timeLeft;
		main->columns[COLUMN_HUB] = l->columns[COLUMN_HUB];
		main->columns[COLUMN_STATUS] = l->columns[COLUMN_STATUS];
		main->columns[COLUMN_SIZE] = Util::formatBytesW(l->size);
		main->columns[COLUMN_SPEED] = Util::formatBytesW(l->speed) + _T("/s");
		main->columns[COLUMN_TIMELEFT] = Util::formatSeconds(l->timeLeft);
		main->columns[COLUMN_RATIO] = Util::toStringW(l->getRatio());
		return false;
	} else {
		if(smallUpdate) return false;

		ratio = ratio / segs;

		int64_t total = 0;
		int64_t fileSize = -1;
		bool hasTree = false;

		string tmp = Text::fromT(main->Target);
		QueueItem::StringMap& queue = QueueManager::getInstance()->lockQueue();
		QueueItem::StringIter qi = queue.find(&tmp);
		if(qi != queue.end()) {
			total = qi->second->getDownloadedBytes();
			fileSize = qi->second->getSize();
			hasTree = qi->second->getHasTree();
			qi->second->setAverageSpeed(totalSpeed);
		}
		QueueManager::getInstance()->unlockQueue();

		if(fileSize == -1) return true;

		if(main->status == ItemInfo::DOWNLOAD_STARTING) {
			main->status = ItemInfo::STATUS_RUNNING;
			main->columns[COLUMN_STATUS] = TSTRING(DOWNLOAD_STARTING);
		} else {
			AutoArray<TCHAR> buf(TSTRING(DOWNLOADED_BYTES).size() + 64);		
				_stprintf(buf, CTSTRING(DOWNLOADED_BYTES), Util::formatBytesW(total).c_str(), 
				(double)total*100.0/(double)fileSize, Util::formatSeconds((GET_TICK() - main->fileBegin)/1000).c_str());

			tstring statusString;

			if(hasTree) {
				statusString += _T("[T]");
			}

			// hack to display whether file is compressed
			if(ratio < 1.0000) {
				statusString += _T("[Z]");
			}
			if(!statusString.empty()) {
				statusString += _T(" ");
			}
			statusString += buf;
			main->columns[COLUMN_STATUS] = statusString;
			
			main->actual = (int64_t)(total * ratio);
			main->pos = total;
			main->speed = totalSpeed;
			main->timeLeft = (totalSpeed > 0) ? ((fileSize - main->pos) / totalSpeed) : 0;
		}
		main->size = fileSize;
		
		if(main->subItems.size() > 1) {
			TCHAR buf[256];
			snwprintf(buf, sizeof(buf), _T("%d %s"), segs, CTSTRING(NUMBER_OF_SEGMENTS));

			main->columns[COLUMN_HUB] = buf;
		}
		main->columns[COLUMN_SIZE] = Util::formatBytesW(fileSize);
		main->columns[COLUMN_SPEED] = Util::formatBytesW(main->speed) + _T("/s");
		main->columns[COLUMN_TIMELEFT] = Util::formatSeconds(main->timeLeft);
		main->columns[COLUMN_RATIO] = Util::toStringW(ratio);
		return false;
	}
}

/**
 * @file
 * $Id$
 */
