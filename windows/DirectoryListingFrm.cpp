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

#include "../client/File.h"
#include "../client/QueueManager.h"
#include "../client/StringTokenizer.h"
#include "../client/HubManager.h"

#include "Resource.h"

#include "DirectoryListingFrm.h"
#include "QueueFrame.h"
#include "WinUtil.h"
#include "LineDlg.h"
#include "PrivateFrame.h"
#include "../client/MerkleTree.h"

void DirectoryListingFrame::openWindow(const tstring& aFile, const User::Ptr& aUser) {
	DirectoryListingFrame* frame = new DirectoryListingFrame(aFile, aUser);
	if(BOOLSETTING(POPUNDER_FILELIST)) {
		WinUtil::hiddenCreateEx(frame);
	} else {
		frame->CreateEx(WinUtil::mdiClient);
	}
}

DirectoryListingFrame::DirectoryListingFrame(const tstring& aFile, const User::Ptr& aUser) :
	statusContainer(STATUSCLASSNAME, this, STATUS_MESSAGE_MAP),
	treeRoot(NULL), skipHits(0), updating(false), dl(NULL), searching(false), start(Text::toT(WinUtil::getInitialDir(aUser)))
{
	tstring tmp;
	if(aFile.size() < 4) {
		error = Text::toT(aUser->getFullNick() + ": " + STRING(UNSUPPORTED_FILELIST_FORMAT));
		return;
	}

	dl = new DirectoryListing(aUser);
	try {
		dl->loadFile(Text::fromT(aFile), true);
	} catch(const Exception& e) {
		error = Text::toT(aUser->getFullNick() + ": " + e.getError());
	}
}

LRESULT DirectoryListingFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	statusContainer.SubclassWindow(ctrlStatus.m_hWnd);

	ctrlTree.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP, WS_EX_CLIENTEDGE, IDC_DIRECTORIES);
	ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_FILES);

	DWORD styles = LVS_EX_FULLROWSELECT;
	if (BOOLSETTING(SHOW_INFOTIPS))
		styles |= LVS_EX_INFOTIP;
	ctrlList.SetExtendedListViewStyle(styles);
	
	ctrlList.SetBkColor(WinUtil::bgColor);
	ctrlList.SetTextBkColor(WinUtil::bgColor);
	ctrlList.SetTextColor(WinUtil::textColor);
	
	ctrlTree.SetBkColor(WinUtil::bgColor);
	ctrlTree.SetTextColor(WinUtil::textColor);
	
	ctrlList.InsertColumn(COLUMN_FILENAME, CTSTRING(FILENAME), LVCFMT_LEFT, 300, COLUMN_FILENAME);
	ctrlList.InsertColumn(COLUMN_TYPE, CTSTRING(FILE_TYPE), LVCFMT_LEFT, 60, COLUMN_TYPE);
	ctrlList.InsertColumn(COLUMN_SIZE, CTSTRING(SIZE), LVCFMT_RIGHT, 100, COLUMN_SIZE);
	ctrlList.InsertColumn(COLUMN_EXACT_SIZE, CTSTRING(EXACT_SIZE), LVCFMT_RIGHT, 100, COLUMN_EXACT_SIZE);
	ctrlList.InsertColumn(COLUMN_TTH, CTSTRING(TTH_ROOT), LVCFMT_LEFT, 200, COLUMN_TTH);

	ctrlList.setSortColumn(COLUMN_FILENAME);	

	ctrlTree.SetImageList(WinUtil::fileImages, TVSIL_NORMAL);
	ctrlList.SetImageList(WinUtil::fileImages, LVSIL_SMALL);

	ctrlFind.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON, 0, IDC_FIND);
	ctrlFind.SetWindowText(CTSTRING(FIND));
	ctrlFind.SetFont(WinUtil::systemFont);

	ctrlFindNext.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON, 0, IDC_NEXT);
	ctrlFindNext.SetWindowText(CTSTRING(NEXT));
	ctrlFindNext.SetFont(WinUtil::systemFont);

	ctrlMatchQueue.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON, 0, IDC_MATCH_QUEUE);
	ctrlMatchQueue.SetWindowText(CTSTRING(MATCH_QUEUE));
	ctrlMatchQueue.SetFont(WinUtil::systemFont);

	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
	SetSplitterPanes(ctrlTree.m_hWnd, ctrlList.m_hWnd);
	m_nProportionalPos = 2500;
	
	if(dl != NULL)
		treeRoot = ctrlTree.InsertItem(TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM, Text::toT(dl->getUser()->getNick()).c_str(), WinUtil::getDirIconIndex(), WinUtil::getDirIconIndex(), 0, 0, (LPARAM)dl->getRoot(), NULL, TVI_SORT);;

	updateTree(dl->getRoot(), treeRoot);
	files = dl->getTotalFileCount();
	size = Util::formatBytes(dl->getTotalSize());

	memset(statusSizes, 0, sizeof(statusSizes));	
	tstring tmp1 = Text::toT(STRING(FILES) + ": " + Util::toString(dl->getTotalFileCount(true)));
	tstring tmp2 = Text::toT(STRING(SIZE) + ": " + Util::formatBytes(dl->getTotalSize(true)));
	statusSizes[2] = WinUtil::getTextWidth(tmp1, m_hWnd);
	statusSizes[3] = WinUtil::getTextWidth(tmp2, m_hWnd);
	statusSizes[4] = WinUtil::getTextWidth(TSTRING(MATCH_QUEUE), m_hWnd) + 8;
	statusSizes[5] = WinUtil::getTextWidth(TSTRING(FIND), m_hWnd) + 8;
	statusSizes[6] = WinUtil::getTextWidth(TSTRING(NEXT), m_hWnd) + 8;

	ctrlStatus.SetParts(8, statusSizes);
	ctrlStatus.SetText(3, tmp1.c_str());
	ctrlStatus.SetText(4, tmp2.c_str());

	if(!start.empty()) {
		StringTokenizer<tstring> tok(start, _T('\\'));
		TStringIter i = tok.getTokens().begin();
		GoToDirectory(treeRoot, i, tok.getTokens().end());
	} else {
		ctrlTree.SelectItem(treeRoot);
	}
	
	fileMenu.CreatePopupMenu();
	targetMenu.CreatePopupMenu();
	directoryMenu.CreatePopupMenu();
	targetDirMenu.CreatePopupMenu();
	priorityMenu.CreatePopupMenu();
	priorityDirMenu.CreatePopupMenu();
	copyMenu.CreatePopupMenu();
	tabMenu.CreatePopupMenu();

	tabMenu.AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CTSTRING(SEND_PRIVATE_MESSAGE));
	tabMenu.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
	tabMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));
	
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_NICK, CTSTRING(COPY_NICK));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_FILENAME, CTSTRING(FILENAME));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_SIZE, CTSTRING(SIZE));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_TTH, CTSTRING(TTH_ROOT));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_LINK, CTSTRING(COPY_MAGNET_LINK));
	
	fileMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD, CTSTRING(DOWNLOAD));
	fileMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)targetMenu, CTSTRING(DOWNLOAD_TO));
	fileMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)priorityMenu, CTSTRING(DOWNLOAD_WITH_PRIORITY));
	fileMenu.AppendMenu(MF_STRING, IDC_VIEW_AS_TEXT, CTSTRING(VIEW_AS_TEXT));
	fileMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES));
	fileMenu.AppendMenu(MF_STRING, IDC_MP3, CTSTRING(GET_MP3INFO));
	fileMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	fileMenu.AppendMenu(MF_STRING,IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
	fileMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	fileMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)copyMenu, CTSTRING(COPY));
	fileMenu.SetMenuDefaultItem(IDC_DOWNLOAD);
	
	directoryMenu.AppendMenu(MF_STRING, IDC_DOWNLOADDIR, CTSTRING(DOWNLOAD));
	directoryMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)targetDirMenu, CTSTRING(DOWNLOAD_TO));
	directoryMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)priorityDirMenu, CTSTRING(DOWNLOAD_WITH_PRIORITY));
	directoryMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	directoryMenu.AppendMenu(MF_STRING,IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
	
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_PAUSED, CTSTRING(PAUSED));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_LOWEST, CTSTRING(LOWEST));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_LOW, CTSTRING(LOW));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_NORMAL, CTSTRING(NORMAL));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_HIGH, CTSTRING(HIGH));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_HIGHEST, CTSTRING(HIGHEST));

	priorityDirMenu.AppendMenu(MF_STRING, IDC_PRIORITY_PAUSED+90, CTSTRING(PAUSED));
	priorityDirMenu.AppendMenu(MF_STRING, IDC_PRIORITY_LOWEST+90, CTSTRING(LOWEST));
	priorityDirMenu.AppendMenu(MF_STRING, IDC_PRIORITY_LOW+90, CTSTRING(LOW));
	priorityDirMenu.AppendMenu(MF_STRING, IDC_PRIORITY_NORMAL+90, CTSTRING(NORMAL));
	priorityDirMenu.AppendMenu(MF_STRING, IDC_PRIORITY_HIGH+90, CTSTRING(HIGH));
	priorityDirMenu.AppendMenu(MF_STRING, IDC_PRIORITY_HIGHEST+90, CTSTRING(HIGHEST));

	closed = false;
	
	setWindowTitle();

	bHandled = FALSE;
	return 1;
}

void DirectoryListingFrame::updateTree(DirectoryListing::Directory* aTree, HTREEITEM aParent) {
	for(DirectoryListing::Directory::Iter i = aTree->directories.begin(); i != aTree->directories.end(); ++i) {
		tstring name;
		if(dl->getUtf8()) {
			name = Text::toT((*i)->getName());
		} else {
			name = Text::toT(Text::acpToUtf8((*i)->getName()));
		}
		HTREEITEM ht = ctrlTree.InsertItem(TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM, name.c_str(), WinUtil::getDirIconIndex(), WinUtil::getDirIconIndex(), 0, 0, (LPARAM)*i, aParent, TVI_SORT);;
		if((*i)->getAdls())
			ctrlTree.SetItemState(ht, TVIS_BOLD, TVIS_BOLD);
		updateTree(*i, ht);
	}
}

void DirectoryListingFrame::updateStatus() {
	if(!searching && !updating && ctrlStatus.IsWindow()) {
		int cnt = ctrlList.GetSelectedCount();
		int64_t total = 0;
		if(cnt == 0) {
			cnt = ctrlList.GetItemCount ();
			total = ctrlList.forEachT(ItemInfo::TotalSize()).total;
		} else {
			total = ctrlList.forEachSelectedT(ItemInfo::TotalSize()).total;
		}

		tstring tmp1 = Text::toT(STRING(ITEMS) + ": " + Util::toString(cnt));
		tstring tmp2 = Text::toT(STRING(SIZE) + ": " + Util::formatBytes(total));
		bool u = false;

		int w = WinUtil::getTextWidth(tmp1, ctrlStatus.m_hWnd);
		if(statusSizes[0] < w) {
			statusSizes[0] = w;
			u = true;
		}
		ctrlStatus.SetText(1, tmp1.c_str());
		w = WinUtil::getTextWidth(tmp2, ctrlStatus.m_hWnd);
		if(statusSizes[1] < w) {
			statusSizes[1] = w;
			u = true;
		}
		ctrlStatus.SetText(2, tmp2.c_str());

		if(u)
			UpdateLayout(TRUE);
	}
}

LRESULT DirectoryListingFrame::onSelChangedDirectories(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMTREEVIEW* p = (NMTREEVIEW*) pnmh;

	if(p->itemNew.state & TVIS_SELECTED) {
		DirectoryListing::Directory* d = (DirectoryListing::Directory*)p->itemNew.lParam;
		changeDir(d, TRUE);
	}
	return 0;
}

void DirectoryListingFrame::changeDir(DirectoryListing::Directory* d, BOOL enableRedraw)
{
	ctrlList.SetRedraw(FALSE);
	updating = true;
	clearList();

	for(DirectoryListing::Directory::Iter i = d->directories.begin(); i != d->directories.end(); ++i) {
		ctrlList.insertItem(ctrlList.GetItemCount(), new ItemInfo(*i, dl->getUtf8()), WinUtil::getDirIconIndex());
	}
	for(DirectoryListing::File::Iter j = d->files.begin(); j != d->files.end(); ++j) {
		ItemInfo* ii = new ItemInfo(*j, dl->getUtf8());
		ctrlList.insertItem(ctrlList.GetItemCount(), ii, WinUtil::getIconIndex(ii->getText(COLUMN_FILENAME)));
	}
	ctrlList.resort();
	ctrlList.SetRedraw(enableRedraw);
	updating = false;
	updateStatus();
}

LRESULT DirectoryListingFrame::onDoubleClickFiles(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*) pnmh;

	HTREEITEM t = ctrlTree.GetSelectedItem();
	if(t != NULL && item->iItem != -1) {
		ItemInfo* ii = (ItemInfo*) ctrlList.GetItemData(item->iItem);

		if(ii->type == ItemInfo::FILE) {
			try {
				dl->download(ii->file, SETTING(DOWNLOAD_DIRECTORY) + Text::fromT(ii->getText(COLUMN_FILENAME)), false, (GetKeyState(VK_SHIFT) & 0x8000) > 0);
			} catch(const Exception& e) {
				ctrlStatus.SetText(0, Text::toT(e.getError()).c_str());
			}
		} else {
			HTREEITEM ht = ctrlTree.GetChildItem(t);
			while(ht != NULL) {
				if((DirectoryListing::Directory*)ctrlTree.GetItemData(ht) == ii->dir) {
					ctrlTree.SelectItem(ht);
					break;
				}
				ht = ctrlTree.GetNextSiblingItem(ht);
			}
		} 
	}
	return 0;
}

LRESULT DirectoryListingFrame::onDownloadDir(WORD , WORD , HWND , BOOL& ) {
	HTREEITEM t = ctrlTree.GetSelectedItem();
	if(t != NULL) {
		DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
		try {
			dl->download(dir, SETTING(DOWNLOAD_DIRECTORY), (GetKeyState(VK_SHIFT) & 0x8000) > 0);
		} catch(const Exception& e) {
			ctrlStatus.SetText(0, Text::toT(e.getError()).c_str());
		}
	}
	return 0;
}

LRESULT DirectoryListingFrame::onDownloadDirWithPrio(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	HTREEITEM t = ctrlTree.GetSelectedItem();
	if(t != NULL) {
		DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);

		QueueItem::Priority p;
		switch(wID-90) {
			case IDC_PRIORITY_PAUSED: p = QueueItem::PAUSED; break;
			case IDC_PRIORITY_LOWEST: p = QueueItem::LOWEST; break;
			case IDC_PRIORITY_LOW: p = QueueItem::LOW; break;
			case IDC_PRIORITY_NORMAL: p = QueueItem::NORMAL; break;
			case IDC_PRIORITY_HIGH: p = QueueItem::HIGH; break;
			case IDC_PRIORITY_HIGHEST: p = QueueItem::HIGHEST; break;
			default: p = QueueItem::DEFAULT; break;
		}

		try {
			dl->download(dir, SETTING(DOWNLOAD_DIRECTORY), (GetKeyState(VK_SHIFT) & 0x8000) > 0, p);
		} catch(const Exception& e) {
			ctrlStatus.SetText(0, Text::toT(e.getError()).c_str());
		}
	}
	return 0;
}

LRESULT DirectoryListingFrame::onDownloadDirTo(WORD , WORD , HWND , BOOL& ) {
	HTREEITEM t = ctrlTree.GetSelectedItem();
	if(t != NULL) {
		DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
		tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
		if(WinUtil::browseDirectory(target, m_hWnd)) {
			WinUtil::addLastDir(target);
			
			try {
				dl->download(dir, Text::fromT(target), (GetKeyState(VK_SHIFT) & 0x8000) > 0);
			} catch(const Exception& e) {
				ctrlStatus.SetText(0, Text::toT(e.getError()).c_str());
			}
		}
	}
	return 0;
}

void DirectoryListingFrame::downloadList(const tstring& aTarget, bool view /* = false */, QueueItem::Priority prio /* = QueueItem::Priority::DEFAULT */) {
	int i=-1;
	while( (i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ItemInfo* ii = (ItemInfo*)ctrlList.GetItemData(i);
		tstring target = aTarget.empty() ? Text::toT(SETTING(DOWNLOAD_DIRECTORY)) : aTarget;

		try {
			if(ii->type == ItemInfo::FILE) {
				if(view) {
					File::deleteFile(Text::fromT(target) + Util::validateFileName(ii->file->getName()));
				}
				dl->download(ii->file, Text::fromT(target + ii->getText(COLUMN_FILENAME)), view, (GetKeyState(VK_SHIFT) & 0x8000) > 0, prio);
			} else if(!view) {
				dl->download(ii->dir, Text::fromT(target), (GetKeyState(VK_SHIFT) & 0x8000) > 0, prio);
			} 
		} catch(const Exception& e) {
			ctrlStatus.SetText(0, Text::toT(e.getError()).c_str());
		}
	}
}

void DirectoryListingFrame::downloadMP3List(const tstring& aTarget) {
	int i=-1;
	while( (i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ItemInfo* ii = (ItemInfo*)ctrlList.GetItemData(i);

		tstring target = aTarget.empty() ? Text::toT(SETTING(DOWNLOAD_DIRECTORY)) : aTarget;

		try {
			if(ii->type == ItemInfo::FILE) {
				File::deleteFile(Text::fromT(target) + Util::validateFileName(ii->file->getName()));
				dl->downloadMP3(ii->file, Text::fromT(target + ii->getText(COLUMN_FILENAME)));
			} 
		} catch(const Exception& e) {
			ctrlStatus.SetText(0, Text::toT(e.getError()).c_str());
		}
	}
}

LRESULT DirectoryListingFrame::onDownload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	downloadList(Text::toT(SETTING(DOWNLOAD_DIRECTORY)));
	return 0;
}

LRESULT DirectoryListingFrame::onDownloadWithPrio(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	QueueItem::Priority p;

	switch(wID) {
		case IDC_PRIORITY_PAUSED: p = QueueItem::PAUSED; break;
		case IDC_PRIORITY_LOWEST: p = QueueItem::LOWEST; break;
		case IDC_PRIORITY_LOW: p = QueueItem::LOW; break;
		case IDC_PRIORITY_NORMAL: p = QueueItem::NORMAL; break;
		case IDC_PRIORITY_HIGH: p = QueueItem::HIGH; break;
		case IDC_PRIORITY_HIGHEST: p = QueueItem::HIGHEST; break;
		default: p = QueueItem::DEFAULT; break;
	}

	downloadList(Text::toT(SETTING(DOWNLOAD_DIRECTORY)), false, p);

	return 0;
}

LRESULT DirectoryListingFrame::onDownloadTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlList.GetSelectedCount() == 1) {
		ItemInfo* ii = (ItemInfo*)ctrlList.GetItemData(ctrlList.GetNextItem(-1, LVNI_SELECTED));

		try {
			if(ii->type == ItemInfo::FILE) {
				tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY)) + ii->getText(COLUMN_FILENAME);
				if(WinUtil::browseFile(target, m_hWnd)) {
					WinUtil::addLastDir(Util::getFilePath(target));
					dl->download(ii->file, Text::fromT(target), false, (GetKeyState(VK_SHIFT) & 0x8000) > 0);
				}
			} else {
				tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
				if(WinUtil::browseDirectory(target, m_hWnd)) {
					WinUtil::addLastDir(target);
					dl->download(ii->dir, Text::fromT(target), (GetKeyState(VK_SHIFT) & 0x8000) > 0);
				}
			} 
		} catch(const Exception& e) {
			ctrlStatus.SetText(0, Text::toT(e.getError()).c_str());
		}
	} else {
		tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
		if(WinUtil::browseDirectory(target, m_hWnd)) {
			WinUtil::addLastDir(target);			
			downloadList(target);
		}
	}
	return 0;
}

LRESULT DirectoryListingFrame::onViewAsText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	downloadList(Text::toT(Util::getTempPath()), true);
	return 0;
}

LRESULT DirectoryListingFrame::onMP3Info(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	downloadMP3List(Text::toT(Util::getTempPath()));
	return 0;
}

LRESULT DirectoryListingFrame::onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(dl != NULL) {
		User::Ptr pUser = dl->getUser();
		if(pUser != (User*) NULL)
			HubManager::getInstance()->addFavoriteUser(pUser);
	}
	return 0;
}

LRESULT DirectoryListingFrame::onPM(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(dl != NULL) {
		User::Ptr pUser = dl->getUser();
		if(pUser != (User*) NULL)
			PrivateFrame::openWindow(pUser);
	}
	return 0;
}
LRESULT DirectoryListingFrame::onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int x = QueueManager::getInstance()->matchListing(dl);
	TCHAR* buf = new TCHAR[STRING(MATCHED_FILES).length() + 32];
	_stprintf(buf, CTSTRING(MATCHED_FILES), x);
	ctrlStatus.SetText(0, buf);
	delete[] buf;
	return 0;
}

LRESULT DirectoryListingFrame::onGoToDirectory(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlList.GetSelectedCount() != 1) 
		return 0;

	tstring fullPath;
	ItemInfo* ii = (ItemInfo*)ctrlList.GetItemData(ctrlList.GetNextItem(-1, LVNI_SELECTED));
	if(ii->type == ItemInfo::FILE) {
		if(!ii->file->getAdls())
			return 0;
		DirectoryListing::Directory* pd = ii->file->getParent();
		while(pd != NULL && pd != dl->getRoot()) {
			fullPath = _T("\\") + Text::toT(pd->getName()) + fullPath;
			pd = pd->getParent();
		}
	} else if(ii->type == ItemInfo::DIRECTORY)	{
		if(!(ii->dir->getAdls() && ii->dir->getParent() != dl->getRoot()))
			return 0;
		fullPath = Text::toT(((DirectoryListing::AdlDirectory*)ii->dir)->getFullPath());
	}

	// Break full path
	TStringList brokenPath;
	while(1) {
		if(fullPath.size() == 0 || fullPath[0] != '\\') 
			break;
		fullPath.erase(0, 1);
		tstring subPath = fullPath.substr(0, fullPath.find_first_of('\\'));
		fullPath.erase(0, subPath.size());
		brokenPath.push_back(subPath);
	}
	
	// Go to directory (recursive)
	TStringList::iterator iPath = brokenPath.begin();
	GoToDirectory(ctrlTree.GetRootItem(), iPath, brokenPath.end());
	
	return 0;
}

void DirectoryListingFrame::GoToDirectory(
	HTREEITEM hItem, 
	TStringList::iterator& iPath, 
	const TStringList::iterator& iPathEnd)
{
	if(iPath == iPathEnd)
		return;	// unexpected
	if(!ctrlTree.ItemHasChildren(hItem))
		return; // unexpected

	// Check on tree children
	HTREEITEM hChild = ctrlTree.GetChildItem(hItem);
	TCHAR itemText[256];
	while(hChild != NULL) {
		if(!ctrlTree.GetItemText(hChild, itemText, 255))
			return; // unexpected
		if(Util::stricmp(*iPath, itemText) == 0) {
			++iPath;
			if(iPath == iPathEnd)			{
				ctrlTree.SelectItem(hChild);
				ctrlTree.EnsureVisible(hChild);
				return;
			}
			GoToDirectory(hChild, iPath, iPathEnd);
			return;
		}
		hChild = ctrlTree.GetNextItem(hChild, TVGN_NEXT);
	}
}

LRESULT DirectoryListingFrame::onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	RECT rc;                    // client area of window 
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 
	
	fileMenu.RemoveMenu(IDC_GO_TO_DIRECTORY, MF_BYCOMMAND);

	// Get the bounding rectangle of the client area. 
	ctrlList.GetClientRect(&rc);
	ctrlList.ScreenToClient(&pt); 

	if (PtInRect(&rc, pt) && ctrlList.GetSelectedCount() > 0) {
		int n = 0;
		ctrlList.ClientToScreen(&pt);

		ItemInfo* ii = (ItemInfo*)ctrlList.GetItemData(ctrlList.GetNextItem(-1, LVNI_SELECTED));

		while(targetMenu.GetMenuItemCount() > 0) {
			targetMenu.DeleteMenu(0, MF_BYPOSITION);
		}

		if(ctrlList.GetSelectedCount() == 1 && ii->type == ItemInfo::FILE) {
			targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOADTO, CTSTRING(BROWSE));
			targets.clear();
			if(ii->file->getTTH() != NULL) {
				QueueManager::getInstance()->getTargetsByRoot(targets, *ii->file->getTTH());
			} else {
				QueueManager::getInstance()->getTargetsBySize(targets, ii->file->getSize(), Util::getFileExt(ii->file->getName()));
			}
			if(targets.size() > 0) {
				targetMenu.AppendMenu(MF_SEPARATOR);
				for(StringIter i = targets.begin(); i != targets.end(); ++i) {
					targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_TARGET + (n++), Text::toT(*i).c_str());
				}
			}
			if(ii->file->getTTH()) {
				copyMenu.EnableMenuItem(IDC_COPY_TTH, MF_BYCOMMAND | MF_ENABLED);
				copyMenu.EnableMenuItem(IDC_COPY_LINK, MF_BYCOMMAND | MF_ENABLED);
				fileMenu.EnableMenuItem(IDC_SEARCH_ALTERNATES, MF_BYCOMMAND | MF_ENABLED);
			} else {
				copyMenu.EnableMenuItem(IDC_COPY_TTH, MF_BYCOMMAND | MF_GRAYED);
				copyMenu.EnableMenuItem(IDC_COPY_LINK, MF_BYCOMMAND | MF_GRAYED);
				fileMenu.EnableMenuItem(IDC_SEARCH_ALTERNATES, MF_BYCOMMAND | MF_GRAYED);
			}
			if(WinUtil::lastDirs.size() > 0) {
				targetMenu.AppendMenu(MF_SEPARATOR);
				for(TStringIter i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i) {
					targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_TARGET + (n++), i->c_str());
				}
			}
			if(ii->file->getAdls())			{
				fileMenu.AppendMenu(MF_STRING, IDC_GO_TO_DIRECTORY, CTSTRING(GO_TO_DIRECTORY));
			}
			fileMenu.EnableMenuItem((UINT)(HMENU)copyMenu, MF_BYCOMMAND | MF_ENABLED);
			fileMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		} else {
			fileMenu.EnableMenuItem((UINT)(HMENU)copyMenu, MF_BYCOMMAND | MF_GRAYED);
			fileMenu.EnableMenuItem(IDC_SEARCH_ALTERNATES, MF_BYCOMMAND | MF_GRAYED);
			targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOADTO, CTSTRING(BROWSE));
			if(WinUtil::lastDirs.size() > 0) {
				targetMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
				for(TStringIter i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i) {
					targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_TARGET + (n++), i->c_str());
				}
			}
			if(ii->dir->getAdls() && ii->dir->getParent() != dl->getRoot())			{
				fileMenu.AppendMenu(MF_STRING, IDC_GO_TO_DIRECTORY, CTSTRING(GO_TO_DIRECTORY));
			}
			fileMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		}
		
		return TRUE; 
	} else { 
		
		ctrlList.ClientToScreen(&pt);
		
		ctrlTree.GetClientRect(&rc);
		ctrlTree.ScreenToClient(&pt); 
		
		if (PtInRect(&rc, pt) && ctrlTree.GetSelectedItem() != NULL) 		{ 
			// Strange, windows doesn't change the selection on right-click... (!)
			UINT a = 0;
			HTREEITEM ht = ctrlTree.HitTest(pt, &a);
			if(ht != NULL && ht != ctrlTree.GetSelectedItem())
				ctrlTree.SelectItem(ht);
			
			while(targetDirMenu.GetMenuItemCount() > 0) {
				targetDirMenu.DeleteMenu(0, MF_BYPOSITION);
			}

			targetDirMenu.AppendMenu(MF_STRING, IDC_DOWNLOADDIRTO, CTSTRING(BROWSE));

			if(WinUtil::lastDirs.size() > 0) {
				targetDirMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
				int n = 0;
				for(TStringIter i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i) {
					targetDirMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_TARGET_DIR + (n++), i->c_str());
				}
			}
			
			ctrlTree.ClientToScreen(&pt);
			directoryMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
			
			return TRUE; 
		} 
	}
	
	return FALSE; 
}

LRESULT DirectoryListingFrame::onDownloadTarget(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int newId = wID - IDC_DOWNLOAD_TARGET;
	dcassert(newId >= 0);
	
	if(ctrlList.GetSelectedCount() == 1) {
		ItemInfo* ii = (ItemInfo*)ctrlList.GetItemData(ctrlList.GetNextItem(-1, LVNI_SELECTED));

		if(ii->type == ItemInfo::FILE) {
			if(newId < (int)targets.size()) {
			try {
					dl->download(ii->file, targets[newId], false, (GetKeyState(VK_SHIFT) & 0x8000) > 0);
			} catch(const Exception& e) {
					ctrlStatus.SetText(0, Text::toT(e.getError()).c_str());
			} 
		} else {
				newId -= (int)targets.size();
				dcassert(newId < (int)WinUtil::lastDirs.size());
				downloadList(WinUtil::lastDirs[newId]);
			}
		} else {
			dcassert(newId < (int)WinUtil::lastDirs.size());
			downloadList(WinUtil::lastDirs[newId]);
		}
	} else if(ctrlList.GetSelectedCount() > 1) {
		dcassert(newId < (int)WinUtil::lastDirs.size());
		downloadList(WinUtil::lastDirs[newId]);
	}
	return 0;
}

LRESULT DirectoryListingFrame::onDownloadTargetDir(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int newId = wID - IDC_DOWNLOAD_TARGET_DIR;
	dcassert(newId >= 0);
	
	HTREEITEM t = ctrlTree.GetSelectedItem();
	if(t != NULL) {
		DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
		string target = SETTING(DOWNLOAD_DIRECTORY);
		try {
			dcassert(newId < (int)WinUtil::lastDirs.size());
			dl->download(dir, Text::fromT(WinUtil::lastDirs[newId]), (GetKeyState(VK_SHIFT) & 0x8000) > 0);
		} catch(const Exception& e) {
			ctrlStatus.SetText(0, Text::toT(e.getError()).c_str());
		}
	}
	return 0;
}

LRESULT DirectoryListingFrame::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	if(kd->wVKey == VK_BACK) {
		HTREEITEM cur = ctrlTree.GetSelectedItem();
		if(cur != NULL)
		{
			HTREEITEM parent = ctrlTree.GetParentItem(cur);
			if(parent != NULL)
				ctrlTree.SelectItem(parent);
		}
	} else if(kd->wVKey == VK_TAB) {
		onTab();
	} else if(kd->wVKey == VK_RETURN) {
		if(ctrlList.GetSelectedCount() == 1) {
			ItemInfo* ii = (ItemInfo*)ctrlList.GetItemData(ctrlList.GetNextItem(-1, LVNI_SELECTED));
			if(ii->type == ItemInfo::DIRECTORY) {
				HTREEITEM ht = ctrlTree.GetChildItem(ctrlTree.GetSelectedItem());
				while(ht != NULL) {
					if((DirectoryListing::Directory*)ctrlTree.GetItemData(ht) == ii->dir) {
						ctrlTree.SelectItem(ht);
						break;
					}
					ht = ctrlTree.GetNextSiblingItem(ht);
				}
			} else {
				downloadList(Text::toT(SETTING(DOWNLOAD_DIRECTORY)));
			}
		} else {
			downloadList(Text::toT(SETTING(DOWNLOAD_DIRECTORY)));
		}
	}
	return 0;
}

void DirectoryListingFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[8];
		ctrlStatus.GetClientRect(sr);
		w[7] = sr.right - 16;
#define setw(x) w[x] = max(w[x+1] - statusSizes[x], 0)
		setw(6); setw(5); setw(4); setw(3); setw(2); setw(1); setw(0);

		ctrlStatus.SetParts(8, w);

		ctrlStatus.GetRect(6, sr);

		sr.left = w[4];
		sr.right = w[5];
		ctrlMatchQueue.MoveWindow(sr);

		sr.left = w[5];
		sr.right = w[6];
		ctrlFind.MoveWindow(sr);

		sr.left = w[6];
		sr.right = w[7];
		ctrlFindNext.MoveWindow(sr);
	}

	SetSplitterRect(&rect);
}

HTREEITEM DirectoryListingFrame::findFile(const StringSearch& str, HTREEITEM root,
										  int &foundFile, int &skipHits)
{
	// Check dir name for match
	DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(root);
	if(str.match(dir->getName()))
	{
		if(skipHits == 0)
		{
			foundFile = -1;
			return root;
		}
		else
			skipHits--;
	}

	// Force list pane to contain files of current dir
	changeDir(dir, FALSE);

	// Check file names in list pane
	for(int i=0; i<ctrlList.GetItemCount(); i++)
	{
		ItemInfo* ii = (ItemInfo*)ctrlList.GetItemData(i);
		if(ii->type == ItemInfo::FILE)
		{
			if(str.match(ii->file->getName()))
			{
				if(skipHits == 0)
				{
					foundFile = i;
					return root;
				}
				else
					skipHits--;
			}
		}
	}

	dcdebug("looking for directories...\n");
	// Check subdirs recursively
	HTREEITEM item = ctrlTree.GetChildItem(root);
	while(item != NULL)
	{
		HTREEITEM srch = findFile(str, item, foundFile, skipHits);
		if(srch)
			return srch;
		else
			item = ctrlTree.GetNextSiblingItem(item);
	}

	return 0;
}

void DirectoryListingFrame::findFile(bool findNext)
{
	if(!findNext)	{
		// Prompt for substring to find
		LineDlg dlg;
		dlg.title = TSTRING(SEARCH_FOR_FILE);
		dlg.description = TSTRING(ENTER_SEARCH_STRING);
		dlg.line = Util::emptyStringT;

		if(dlg.DoModal() != IDOK)
			return;

		findStr = Text::fromT(dlg.line);
		if(!dl->getUtf8())
			findStr = Text::utf8ToAcp(findStr);
		skipHits = 0;
	}	else {
		skipHits++;
	}

	if(findStr.empty())
		return;

	// Do a search
	int foundFile = -1, skipHitsTmp = skipHits;
	HTREEITEM const oldDir = ctrlTree.GetSelectedItem();
	HTREEITEM const foundDir = findFile(StringSearch(findStr), ctrlTree.GetRootItem(), foundFile, skipHitsTmp);
	ctrlTree.SetRedraw(TRUE);

	if(foundDir)	{
		// Highlight the directory tree and list if the parent dir/a matched dir was found
		if(foundFile >= 0)		{
			// SelectItem won't update the list if SetRedraw was set to FALSE and then
			// to TRUE and the item selected is the same as the last one... workaround:
			if(oldDir == foundDir)
				ctrlTree.SelectItem(NULL);

			ctrlTree.SelectItem(foundDir);
		}		else		{
			// Got a dir; select its parent directory in the tree if there is one
			HTREEITEM parentItem = ctrlTree.GetParentItem(foundDir);
			if(parentItem)			{
				// Go to parent file list
				ctrlTree.SelectItem(parentItem);

				// Locate the dir in the file list
				DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(foundDir);

				foundFile = ctrlList.findItem(Text::toT(dir->getName()), -1, false);
				}	else	{
				// If no parent exists, just the dir tree item and skip the list highlighting
				ctrlTree.SelectItem(foundDir);
			}
		}

		// Remove prev. selection from file list
		if(ctrlList.GetSelectedCount() > 0)		{
			for(int i=0; i<ctrlList.GetItemCount(); i++)
				ctrlList.SetItemState(i, 0, LVIS_SELECTED);
		}

		// Highlight and focus the dir/file if possible
		if(foundFile >= 0)		{
			ctrlList.SetFocus();
			ctrlList.EnsureVisible(foundFile, FALSE);
			ctrlList.SetItemState(foundFile, LVIS_SELECTED | LVIS_FOCUSED, (UINT)-1);
		}		else {
			ctrlTree.SetFocus();
	}
	} else	{
		ctrlTree.SelectItem(oldDir);
		MessageBox(CTSTRING(NO_MATCHES), CTSTRING(SEARCH_FOR_FILE));
	}
}

LRESULT DirectoryListingFrame::onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	string sCopy;
	if(ctrlList.GetSelectedCount() == 1) {
		ItemInfo* ii = ctrlList.getSelectedItem();
		if(ii->type != ItemInfo::FILE)
			return 0;
		switch (wID) {
			case IDC_COPY_NICK:
				sCopy = dl->getUser()->getNick();
				break;
			case IDC_COPY_FILENAME:
				sCopy = Util::getFileName(ii->file->getName());
				break;
			case IDC_COPY_SIZE:
				sCopy = Util::formatBytes(ii->file->getSize());
				break;
			case IDC_COPY_LINK:
				if(ii->type == ItemInfo::FILE) {
					WinUtil::copyMagnet(ii->file->getTTH(), Text::toT(ii->file->getName()), ii->file->getSize());
				}
				break;
			case IDC_COPY_TTH:
				if(ii->type == ItemInfo::FILE && ii->file->getTTH())
					sCopy = ii->file->getTTH()->toBase32();
				break;
			default:
				dcdebug("DIRECTORYLISTINGFRAME DON'T GO HERE\n");
				return 0;
		}
		if (!sCopy.empty())
			WinUtil::setClipboard(Text::toT(sCopy));
	}
	return S_OK;
}

LRESULT DirectoryListingFrame::onSearchAlternates(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlList.GetSelectedCount() == 1) {
		ItemInfo* ii = ctrlList.getSelectedItem();
		if(ii->file->getTTH() != NULL)
			WinUtil::searchHash(ii->file->getTTH());
	} 
	return 0;
}

LRESULT DirectoryListingFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		ctrlList.SetRedraw(FALSE);
		clearList();
		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		bHandled = FALSE;
		return 0;
	}
}

LRESULT DirectoryListingFrame::onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 

	string nick = "";
	if(dl != NULL) {
		User::Ptr pUser = dl->getUser();
		if(pUser != (User*) NULL)
			nick = pUser->getNick();
	}
	tabMenu.InsertSeparatorFirst(nick);
	tabMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	tabMenu.RemoveFirstItem();
	return TRUE;
}

/**
 * @file
 * $Id$
 */