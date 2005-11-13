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

#include "QueueFrame.h"
#include "SearchFrm.h"
#include "PrivateFrame.h"
#include "CZDCLib.h"
#include "LineDlg.h"

#include "../client/StringTokenizer.h"
#include "../client/ShareManager.h"

#include "BarShader.h"

#define FILE_LIST_NAME _T("File Lists")

int QueueFrame::columnIndexes[] = { COLUMN_TARGET, COLUMN_STATUS, COLUMN_SEGMENTS, COLUMN_SIZE, COLUMN_PROGRESS, COLUMN_DOWNLOADED, COLUMN_PRIORITY,
COLUMN_USERS, COLUMN_PATH, COLUMN_EXACT_SIZE, COLUMN_ERRORS, COLUMN_ADDED, COLUMN_TTH, COLUMN_TYPE };

int QueueFrame::columnSizes[] = { 200, 300, 70, 75, 100, 120, 75, 200, 200, 75, 200, 100, 125, 75 };

static ResourceManager::Strings columnNames[] = { ResourceManager::FILENAME, ResourceManager::STATUS, ResourceManager::SEGMENTS, ResourceManager::SIZE, 
ResourceManager::DOWNLOADED_PARTS, ResourceManager::DOWNLOADED,
ResourceManager::PRIORITY, ResourceManager::USERS, ResourceManager::PATH, ResourceManager::EXACT_SIZE, ResourceManager::ERRORS,
ResourceManager::ADDED, ResourceManager::TTH_ROOT, ResourceManager::TYPE };

LRESULT QueueFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	showTree = BOOLSETTING(QUEUEFRAME_SHOW_TREE);
	
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	
	ctrlQueue.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_QUEUE);
	ctrlQueue.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | 0x00010000 | (BOOLSETTING(SHOW_INFOTIPS) ? LVS_EX_INFOTIP : 0));


	ctrlDirs.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
		TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP, 
		 WS_EX_CLIENTEDGE, IDC_DIRECTORIES);
	
	ctrlDirs.SetImageList(WinUtil::fileImages, TVSIL_NORMAL);
	ctrlQueue.SetImageList(WinUtil::fileImages, LVSIL_SMALL);
	
	m_nProportionalPos = 2500;
	SetSplitterPanes(ctrlDirs.m_hWnd, ctrlQueue.m_hWnd);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(QUEUEFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(QUEUEFRAME_WIDTHS), COLUMN_LAST);
	
	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_DOWNLOADED || j == COLUMN_EXACT_SIZE|| j == COLUMN_SEGMENTS) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlQueue.insertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlQueue.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlQueue.setSortColumn(COLUMN_TARGET);
	
	ctrlQueue.SetBkColor(WinUtil::bgColor);
	ctrlQueue.SetTextBkColor(WinUtil::bgColor);
	ctrlQueue.SetTextColor(WinUtil::textColor);
	ctrlQueue.setFlickerFree(WinUtil::bgBrush);

	ctrlDirs.SetBkColor(WinUtil::bgColor);
	ctrlDirs.SetTextColor(WinUtil::textColor);

	ctrlShowTree.Create(ctrlStatus.m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlShowTree.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowTree.SetCheck(showTree);
	showTreeContainer.SubclassWindow(ctrlShowTree.m_hWnd);
	
	singleMenu.CreatePopupMenu();
	multiMenu.CreatePopupMenu();
	browseMenu.CreatePopupMenu();
	removeMenu.CreatePopupMenu();
	removeAllMenu.CreatePopupMenu();
	pmMenu.CreatePopupMenu();
	priorityMenu.CreatePopupMenu();
	dirMenu.CreatePopupMenu();	
	readdMenu.CreatePopupMenu();
	previewMenu.CreatePopupMenu();
	segmentsMenu.CreatePopupMenu();
	copyMenu.CreatePopupMenu();
	for(int i = 0; i <COLUMN_LAST; ++i)
		copyMenu.AppendMenu(MF_STRING, IDC_COPY + i, CTSTRING_I(columnNames[i]));

	singleMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES));
	singleMenu.AppendMenu(MF_STRING, IDC_COPY_LINK, CTSTRING(COPY_MAGNET_LINK));
	singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)previewMenu, CTSTRING(PREVIEW_MENU));	
	singleMenu.AppendMenu(MF_STRING, IDC_MOVE, CTSTRING(MOVE));
	singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)segmentsMenu, CTSTRING(MAX_SEGMENTS_NUMBER));
	singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)priorityMenu, CTSTRING(SET_PRIORITY));
	singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)browseMenu, CTSTRING(GET_FILE_LIST));
	singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)pmMenu, CTSTRING(SEND_PRIVATE_MESSAGE));
	singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)readdMenu, CTSTRING(READD_SOURCE));
	singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)copyMenu, CTSTRING(COPY));
	singleMenu.AppendMenu(MF_SEPARATOR);
	singleMenu.AppendMenu(MF_STRING, IDC_MOVE, CTSTRING(MOVE));
	singleMenu.AppendMenu(MF_SEPARATOR);
	singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)removeMenu, CTSTRING(REMOVE_SOURCE));
	singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)removeAllMenu, CTSTRING(REMOVE_FROM_ALL));
	singleMenu.AppendMenu(MF_STRING, IDC_REMOVE_OFFLINE, CTSTRING(REMOVE_OFFLINE));
	singleMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
	
	multiMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)segmentsMenu, CTSTRING(MAX_SEGMENTS_NUMBER));
	multiMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)priorityMenu, CTSTRING(SET_PRIORITY));
	multiMenu.AppendMenu(MF_STRING, IDC_MOVE, CTSTRING(MOVE));
	multiMenu.AppendMenu(MF_SEPARATOR);
	multiMenu.AppendMenu(MF_STRING, IDC_REMOVE_OFFLINE, CTSTRING(REMOVE_OFFLINE));
	multiMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
	
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_PAUSED, CTSTRING(PAUSED));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_LOWEST, CTSTRING(LOWEST));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_LOW, CTSTRING(LOW));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_NORMAL, CTSTRING(NORMAL));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_HIGH, CTSTRING(HIGH));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_HIGHEST, CTSTRING(HIGHEST));
	priorityMenu.AppendMenu(MF_STRING, IDC_AUTOPRIORITY, CTSTRING(AUTO));

	segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTONE, (_T("1 ")+TSTRING(SEGMENT)).c_str());
	segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTTWO, (_T("2 ")+TSTRING(SEGMENTS)).c_str());
	segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTTHREE, (_T("3 ")+TSTRING(SEGMENTS)).c_str());
	segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTFOUR, (_T("4 ")+TSTRING(SEGMENTS)).c_str());
	segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTFIVE, (_T("5 ")+TSTRING(SEGMENTS)).c_str());
	segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTSIX, (_T("6 ")+TSTRING(SEGMENTS)).c_str());
	segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTSEVEN, (_T("7 ")+TSTRING(SEGMENTS)).c_str());
	segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTEIGHT, (_T("8 ")+TSTRING(SEGMENTS)).c_str());
	segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTNINE, (_T("9 ")+TSTRING(SEGMENTS)).c_str());
	segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTTEN, (_T("10 ")+TSTRING(SEGMENTS)).c_str());

 	dirMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)priorityMenu, CTSTRING(SET_PRIORITY));
	dirMenu.AppendMenu(MF_STRING, IDC_MOVE, CTSTRING(MOVE));
	dirMenu.AppendMenu(MF_SEPARATOR);
	dirMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));

	removeMenu.AppendMenu(MF_STRING, IDC_REMOVE_SOURCE, CTSTRING(ALL));
	removeMenu.AppendMenu(MF_SEPARATOR);

	readdMenu.AppendMenu(MF_STRING, IDC_READD, CTSTRING(ALL));
	readdMenu.AppendMenu(MF_SEPARATOR);

	addQueueList(QueueManager::getInstance()->lockQueue());
	QueueManager::getInstance()->unlockQueue();
	QueueManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	hIconTree = (HICON)LoadImage((HINSTANCE)::GetWindowLong(::GetParent(m_hWnd), GWL_HINSTANCE), MAKEINTRESOURCE(IDR_TREE_YES), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);
	hIconNotTree = (HICON)LoadImage((HINSTANCE)::GetWindowLong(::GetParent(m_hWnd), GWL_HINSTANCE), MAKEINTRESOURCE(IDR_TREE_NO), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);

	memset2(statusSizes, 0, sizeof(statusSizes));
	statusSizes[0] = 16;
	ctrlStatus.SetParts(6, statusSizes);
	updateStatus();

	bHandled = FALSE;
	return 1;
}

void QueueFrame::QueueItemInfo::update() {
	if(display != NULL) {
		int colMask = updateMask;
		updateMask = 0;

		qi = QueueManager::getInstance()->fileQueue.find(Text::fromT(target));

		int PocetSegmentu = qi ? qi->getActiveSegments().size() : 0;
		int MaxSegmentu = qi ? qi->getMaxSegments() : 0;
		display->columns[COLUMN_SEGMENTS] = Text::toT(Util::toString(PocetSegmentu))+_T("/")+Text::toT(Util::toString(MaxSegmentu))+ _T(" ");

		if(colMask & MASK_TARGET) {
			display->columns[COLUMN_TARGET] = Util::getFileName(getTarget());
		}
		int online = 0;
		if(colMask & MASK_USERS || colMask & MASK_STATUS) {
			tstring tmp;

			SourceIter j;
			for(j = getSources().begin(); j != getSources().end(); ++j) {
				if(tmp.size() > 0)
					tmp += _T(", ");

				if(j->getUser()->isOnline())
					online++;

				tmp += Text::toT(j->getUser()->getFirstNick());
			}
			display->columns[COLUMN_USERS] = tmp.empty() ? TSTRING(NO_USERS) : tmp;
		}
		if(colMask & MASK_STATUS) {
			if(getStatus() == QueueItem::STATUS_WAITING) {

				TCHAR buf[64];
				if(online > 0) {
					if(getSources().size() == 1) {
						display->columns[COLUMN_STATUS] = TSTRING(WAITING_USER_ONLINE);
					} else {
						_stprintf(buf, CTSTRING(WAITING_USERS_ONLINE), online, getSources().size());
						display->columns[COLUMN_STATUS] = buf;
					}
				} else {
					if(getSources().size() == 0) {
						display->columns[COLUMN_STATUS] = TSTRING(NO_USERS_TO_DOWNLOAD_FROM);
					} else if(getSources().size() == 1) {
						display->columns[COLUMN_STATUS] = TSTRING(USER_OFFLINE);
					} else if(getSources().size() == 2) {
						display->columns[COLUMN_STATUS] = TSTRING(BOTH_USERS_OFFLINE);
					} else if(getSources().size() == 3) {
						display->columns[COLUMN_STATUS] = TSTRING(ALL_3_USERS_OFFLINE);
					} else if(getSources().size() == 4) {
						display->columns[COLUMN_STATUS] = TSTRING(ALL_4_USERS_OFFLINE);
					} else {
						_stprintf(buf, CTSTRING(ALL_USERS_OFFLINE), getSources().size());
						display->columns[COLUMN_STATUS] = buf;
					}
				}
			} else if(getStatus() == QueueItem::STATUS_RUNNING) {
				TCHAR buf[64];
				if(online > 0) {
					if(getSources().size() == 1) {
						display->columns[COLUMN_STATUS] = TSTRING(USER_ONLINE);
					} else {
						_stprintf(buf, CTSTRING(USERS_ONLINE), online, getSources().size());
						display->columns[COLUMN_STATUS] = buf;
					}
				} else {
					display->columns[COLUMN_STATUS] = TSTRING(RUNNING);
				}
			} 
		}
		if(colMask & MASK_SIZE) {
			display->columns[COLUMN_SIZE] = (getSize() == -1) ? TSTRING(UNKNOWN) : Text::toT(Util::formatBytes(getSize()));
			display->columns[COLUMN_EXACT_SIZE] = (getSize() == -1) ? TSTRING(UNKNOWN) : Text::toT(Util::formatExactSize(getSize()));
			}
		if(colMask & MASK_DOWNLOADED) {
				if(getSize() > 0)
					display->columns[COLUMN_DOWNLOADED] = Text::toT(Util::formatBytes(getDownloadedBytes()) + " (" + Util::toString((double)getDownloadedBytes()*100.0/(double)getSize()) + "%)");
				else
					display->columns[COLUMN_DOWNLOADED].clear();
		}
		if(colMask & MASK_PRIORITY) {
			switch(getPriority()) {
				case QueueItem::PAUSED: display->columns[COLUMN_PRIORITY] = TSTRING(PAUSED); break;
				case QueueItem::LOWEST: display->columns[COLUMN_PRIORITY] = TSTRING(LOWEST); break;
				case QueueItem::LOW: display->columns[COLUMN_PRIORITY] = TSTRING(LOW); break;
				case QueueItem::NORMAL: display->columns[COLUMN_PRIORITY] = TSTRING(NORMAL); break;
				case QueueItem::HIGH: display->columns[COLUMN_PRIORITY] = TSTRING(HIGH); break;
				case QueueItem::HIGHEST: display->columns[COLUMN_PRIORITY] = TSTRING(HIGHEST); break;
				default: dcasserta(0); break;
			}
			if(getAutoPriority()) {
				display->columns[COLUMN_PRIORITY] += _T(" (") + TSTRING(AUTO) + _T(")");
			}
		}

		if(colMask & MASK_PATH) {
			display->columns[COLUMN_PATH] = Util::getFilePath(getTarget());
		}

		if(colMask & MASK_ERRORS) {
			tstring tmp;
			SourceIter j;
			for(j = getBadSources().begin(); j != getBadSources().end(); ++j) {
				if(!j->isSet(QueueItem::Source::FLAG_REMOVED)) {
				if(tmp.size() > 0)
					tmp += _T(", ");
						tmp += Text::toT(j->getUser()->getFirstNick());
					tmp += _T(" (");
					if(j->isSet(QueueItem::Source::FLAG_FILE_NOT_AVAILABLE)) {
						tmp += TSTRING(FILE_NOT_AVAILABLE);
					} else if(j->isSet(QueueItem::Source::FLAG_PASSIVE)) {
						tmp += TSTRING(PASSIVE_USER);
					} else if(j->isSet(QueueItem::Source::FLAG_ROLLBACK_INCONSISTENCY)) {
						tmp += TSTRING(ROLLBACK_INCONSISTENCY);
					} else if(j->isSet(QueueItem::Source::FLAG_BAD_TREE)) {
						tmp += TSTRING(INVALID_TREE);
					} else if(j->isSet(QueueItem::Source::FLAG_SLOW)) {
						tmp += TSTRING(SLOW_USER);
					} else if(j->isSet(QueueItem::Source::FLAG_NO_NEED_PARTS)) {
						tmp += TSTRING(NO_NEEDED_PART);
					}
					tmp += _T(')');
				}
			}
			display->columns[COLUMN_ERRORS] = tmp.empty() ? TSTRING(NO_ERRORS) : tmp;
		}

		if(colMask & MASK_ADDED) {
			display->columns[COLUMN_ADDED] = Text::toT(Util::formatTime("%Y-%m-%d %H:%M", getAdded()));
		}
		if(colMask & MASK_TTH && getTTH() != NULL) {
			display->columns[COLUMN_TTH] = Text::toT(getTTH()->toBase32());
		}
		if(colMask & MASK_TYPE) {
			display->columns[COLUMN_TYPE] = Util::getFileExt(getTarget());
			if(display->columns[COLUMN_TYPE].size() > 0 && display->columns[COLUMN_TYPE][0] == '.')
				display->columns[COLUMN_TYPE].erase(0, 1);
		}
	}
}

void QueueFrame::on(QueueManagerListener::Added, QueueItem* aQI) {
	QueueItemInfo* ii = new QueueItemInfo(aQI);
	{
		Lock l(cs);
		dcassert(queue.find(aQI) == queue.end());
		queue[aQI] = ii;
	}

	speak(ADD_ITEM,	ii);
}

void QueueFrame::addQueueItem(QueueItemInfo* ii, bool noSort) {
	if(!ii->isSet(QueueItem::FLAG_USER_LIST) && !ii->isSet(QueueItem::FLAG_TESTSUR)) {
		queueSize+=ii->getSize();
	}
	queueItems++;
	dirty = true;
	
	const tstring& dir = ii->getPath();
	
	bool updateDir = (directories.find(dir) == directories.end());
	directories.insert(make_pair(dir, ii));
	
	if(updateDir) {
		addDirectory(dir, ii->isSet(QueueItem::FLAG_USER_LIST));
	} 
	if(!showTree || isCurDir(dir)) {
		ii->update();
		if(noSort)
			ctrlQueue.insertItem(ctrlQueue.GetItemCount(), ii, WinUtil::getIconIndex(ii->getTarget()));
		else
			ctrlQueue.insertItem(ii, WinUtil::getIconIndex(ii->getTarget()));
	}
}

void QueueFrame::addQueueList(const QueueItem::StringMap& li) {
	ctrlQueue.SetRedraw(FALSE);
	ctrlDirs.SetRedraw(FALSE);
	for(QueueItem::StringMap::const_iterator j = li.begin(); j != li.end(); ++j) {
		QueueItem* aQI = j->second;
		QueueItemInfo* ii = new QueueItemInfo(aQI);
		dcassert(queue.find(aQI) == queue.end());
		queue[aQI] = ii;
		addQueueItem(ii, true);
	}
	ctrlQueue.resort();
	ctrlQueue.SetRedraw(TRUE);
	ctrlDirs.SetRedraw(TRUE);
	ctrlDirs.Invalidate();
}

HTREEITEM QueueFrame::addDirectory(const tstring& dir, bool isFileList /* = false */, HTREEITEM startAt /* = NULL */) {
	TVINSERTSTRUCT tvi;
	tvi.hInsertAfter = TVI_SORT;
	tvi.item.mask = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT;
	tvi.item.iImage = tvi.item.iSelectedImage = WinUtil::getDirIconIndex();

	if(isFileList) {
		// We assume we haven't added it yet, and that all filelists go to the same
		// directory...
		dcassert(fileLists == NULL);
		tvi.hParent = NULL;
		tvi.item.pszText = FILE_LIST_NAME;
		tvi.item.lParam = (LPARAM) new tstring(dir);
		fileLists = ctrlDirs.InsertItem(&tvi);
		return fileLists;
	} 

	// More complicated, we have to find the last available tree item and then see...
	string::size_type i = 0;
	string::size_type j;

	HTREEITEM next = NULL;
	HTREEITEM parent = NULL;

	if(startAt == NULL) {
		// First find the correct drive letter
		dcassert(dir[1] == ':');
		dcassert(dir[2] == '\\');

		next = ctrlDirs.GetRootItem();

		while(next != NULL) {
			if(next != fileLists) {
				tstring* stmp = (tstring*)ctrlDirs.GetItemData(next);
					if(Util::strnicmp(*stmp, dir, 3) == 0)
						break;
				}
			next = ctrlDirs.GetNextSiblingItem(next);
		}

	if(next == NULL) {
		// First addition, set commonStart to the dir minus the last part...
		i = dir.rfind('\\', dir.length()-2);
			if(i != tstring::npos) {
				tstring name = dir.substr(0, i);
			tvi.hParent = NULL;
				tvi.item.pszText = const_cast<TCHAR*>(name.c_str());
				tvi.item.lParam = (LPARAM)new tstring(dir.substr(0, i+1));
				next = ctrlDirs.InsertItem(&tvi);
		} else {
				dcassert(dir.length() == 3);
				tvi.hParent = NULL;
				tvi.item.pszText = const_cast<TCHAR*>(dir.c_str());
				tvi.item.lParam = (LPARAM)new tstring(dir);
				next = ctrlDirs.InsertItem(&tvi);
			}
		} 
		
		// Ok, next now points to x:\... find how much is common

		tstring* rootStr = (tstring*)ctrlDirs.GetItemData(next);
		
			i = 0;

			for(;;) {
			j = dir.find('\\', i);
				if(j == string::npos)
					break;
				if(Util::strnicmp(dir.c_str() + i, rootStr->c_str() + i, j - i + 1) != 0)
					break;
					i = j + 1;
				}
		
		if(i < rootStr->length()) {
			HTREEITEM oldRoot = next;

			// Create a new root
			tstring name = rootStr->substr(0, i-1);
			tvi.hParent = NULL;
			tvi.item.pszText = const_cast<TCHAR*>(name.c_str());
			tvi.item.lParam = (LPARAM)new tstring(rootStr->substr(0, i));
			HTREEITEM newRoot = ctrlDirs.InsertItem(&tvi);

			parent = addDirectory(*rootStr, false, newRoot);

			next = ctrlDirs.GetChildItem(oldRoot);
			while(next != NULL) {
				moveNode(next, parent);
				next = ctrlDirs.GetChildItem(oldRoot);
				}
			delete rootStr;
			ctrlDirs.DeleteItem(oldRoot);
			parent = newRoot;
			} else {
			// Use this root as parent
			parent = next;
				next = ctrlDirs.GetChildItem(parent);
		}
	} else {
		parent = startAt;
		next = ctrlDirs.GetChildItem(parent);
		i = getDir(parent).length();
		dcassert(Util::strnicmp(getDir(parent), dir, getDir(parent).length()) == 0);
	}

	HTREEITEM firstParent = parent;

	while( i < dir.length() ) {
		while(next != NULL) {
			if(next != fileLists) {
			const tstring& n = getDir(next);
			if(Util::strnicmp(n.c_str()+i, dir.c_str()+i, n.length()-i) == 0) {
				// Found a part, we assume it's the best one we can find...
				i = n.length();

				parent = next;
				next = ctrlDirs.GetChildItem(next);
				break;
			}
			}
			next = ctrlDirs.GetNextSiblingItem(next);
		}

		if(next == NULL) {
			// We didn't find it, add...
			j = dir.find('\\', i);
			dcassert(j != string::npos);
			tstring name = dir.substr(i, j-i);
			tvi.hParent = parent;
			tvi.item.pszText = const_cast<TCHAR*>(name.c_str());
			tvi.item.lParam = (LPARAM) new tstring(dir.substr(0, j+1));
			
			parent = ctrlDirs.InsertItem(&tvi);
			
			i = j + 1;
		}
	}

	if(BOOLSETTING(EXPAND_QUEUE) && firstParent != NULL)
		ctrlDirs.Expand(firstParent);

	return parent;
}

void QueueFrame::removeDirectory(const tstring& dir, bool isFileList /* = false */) {

	// First, find the last name available
	string::size_type i = 0;

	HTREEITEM next = ctrlDirs.GetRootItem();
	HTREEITEM parent = NULL;
	
	if(isFileList) {
		dcassert(fileLists != NULL);
#if _STLPORT_MAJOR >= 5
		delete (tstring*)ctrlDirs.GetItemData(fileLists);
#else
 		delete (string*)ctrlDirs.GetItemData(fileLists);
#endif
		ctrlDirs.DeleteItem(fileLists);
		fileLists = NULL;
		return;
	} else {
		while(i < dir.length()) {
			while(next != NULL) {
				if(next != fileLists) {
				const tstring& n = getDir(next);
				if(Util::strnicmp(n.c_str()+i, dir.c_str()+i, n.length()-i) == 0) {
					// Match!
					parent = next;
					next = ctrlDirs.GetChildItem(next);
					i = n.length();
					break;
				}
				}
				next = ctrlDirs.GetNextSiblingItem(next);
			}
			if(next == NULL)
				break;
		}
	}

	next = parent;

	while((ctrlDirs.GetChildItem(next) == NULL) && (directories.find(getDir(next)) == directories.end())) {
#if _STLPORT_MAJOR >= 5
		delete (tstring*)ctrlDirs.GetItemData(next);
#else
 		delete (string*)ctrlDirs.GetItemData(next);
#endif
		parent = ctrlDirs.GetParentItem(next);
		
		ctrlDirs.DeleteItem(next);
		if(parent == NULL)
			break;
		next = parent;
	}
}

void QueueFrame::removeDirectories(HTREEITEM ht) {
	HTREEITEM next = ctrlDirs.GetChildItem(ht);
	while(next != NULL) {
		removeDirectories(next);
		next = ctrlDirs.GetNextSiblingItem(ht);
	}
#if _STLPORT_MAJOR >= 5
		delete (tstring*)ctrlDirs.GetItemData(ht);
#else
		delete (string*)ctrlDirs.GetItemData(ht);
#endif
	ctrlDirs.DeleteItem(ht);
}

void QueueFrame::on(QueueManagerListener::Removed, QueueItem* aQI) {
	QueueItemInfo* qi = NULL;
	{
		Lock l(cs);
		QueueIter i = queue.find(aQI);
		
		if(i == queue.end())
			return;

		qi = i->second;
		qi->qi = NULL;
		queue.erase(i);

		dirty = true;
	}
	
	speak(REMOVE_ITEM, qi);
}

void QueueFrame::on(QueueManagerListener::Moved, QueueItem* aQI) {
	QueueItemInfo* qi = NULL;
	QueueItemInfo* qi2 = new QueueItemInfo(aQI);
	{
		Lock l(cs);
		dcassert(queue.find(aQI) != queue.end());
		QueueIter i = queue.find(aQI);
		qi = i->second;
		i->second = qi2;
	}
	
	speak(REMOVE_ITEM, qi);
	speak(ADD_ITEM,	qi2);
}

void QueueFrame::on(QueueManagerListener::SourcesUpdated, QueueItem* aQI) {
	QueueItemInfo* ii = NULL;
	{
		Lock l(cs);

		if(queue.find(aQI) == queue.end())
			return;
		
		dcassert(queue.find(aQI) != queue.end());
		ii = queue[aQI];

		ii->setPriority(aQI->getPriority());
		ii->setStatus(aQI->getStatus());
		ii->setDownloadedBytes(aQI->chunkInfo ? aQI->chunkInfo->getDownloadedSize() : aQI->getDownloadedBytes());
		ii->setTTH(aQI->getTTH());
		ii->setAutoPriority(aQI->getAutoPriority());
		ii->qi = aQI;

		{
			for(QueueItemInfo::SourceIter i = ii->getSources().begin(); i != ii->getSources().end(); ) {
				if(!aQI->isSource(i->getUser())) {
					i = ii->getSources().erase(i);
				} else {
					++i;
				}
			}
			for(QueueItem::Source::Iter j = aQI->getSources().begin(); j != aQI->getSources().end(); ++j) {
				if(!ii->isSource((*j)->getUser())) {
					ii->getSources().push_back(QueueItemInfo::SourceInfo(*(*j)));
				}
			}
		}
		{
			for(QueueItemInfo::SourceIter i = ii->getBadSources().begin(); i != ii->getBadSources().end(); ) {
				if(!aQI->isBadSource(i->getUser())) {
					i = ii->getBadSources().erase(i);
				} else {
					++i;
				}
			}
			for(QueueItem::Source::Iter j = aQI->getBadSources().begin(); j != aQI->getBadSources().end(); ++j) {
				if(!ii->isBadSource((*j)->getUser())) {
					ii->getBadSources().push_back(QueueItemInfo::SourceInfo(*(*j)));
				}
			}
		}
		ii->updateMask |= QueueItemInfo::MASK_PRIORITY | QueueItemInfo::MASK_USERS | QueueItemInfo::MASK_ERRORS | QueueItemInfo::MASK_STATUS | QueueItemInfo::MASK_DOWNLOADED | QueueItemInfo::MASK_TTH;
	}

	speak(UPDATE_ITEM, ii);
}

LRESULT QueueFrame::onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	Lock l(cs);
	spoken = false;

	for(TaskIter ti = tasks.begin(); ti != tasks.end(); ++ti) {
		if(ti->first == ADD_ITEM) {
			QueueItemInfo* ii = (QueueItemInfo*)ti->second;
			dcassert(ctrlQueue.findItem(ii) == -1);
			addQueueItem(ii, false);
			updateStatus();
		} else if(ti->first == REMOVE_ITEM) {
			QueueItemInfo* ii = (QueueItemInfo*)ti->second;			
			
			if(!showTree || isCurDir(ii->getPath()) ) {
				dcassert(ctrlQueue.findItem(ii) != -1);
				ctrlQueue.deleteItem(ii);
			}
			
			if(!ii->isSet(QueueItem::FLAG_USER_LIST) && !ii->isSet(QueueItem::FLAG_TESTSUR)) {
				queueSize-=ii->getSize();
				dcassert(queueSize >= 0);
			}
			queueItems--;
			dcassert(queueItems >= 0);
			
			pair<DirectoryIter, DirectoryIter> i = directories.equal_range(ii->getPath());
			DirectoryIter j;
			for(j = i.first; j != i.second; ++j) {
				if(j->second == ii)
					break;
			}
			dcassert(j != i.second);
			directories.erase(j);
			if(directories.count(ii->getPath()) == 0) {
				removeDirectory(ii->getPath(), ii->isSet(QueueItem::FLAG_USER_LIST));
				if(isCurDir(ii->getPath()))
					curDir = Util::emptyStringT;
			}
			
			delete ii;
			updateStatus();
			if (BOOLSETTING(QUEUE_DIRTY)) {
				setDirty();
			}
			dirty = true;
		} else if(ti->first == UPDATE_ITEM) {
			QueueItemInfo* ii = (QueueItemInfo*)ti->second;
			if(!showTree || isCurDir(ii->getPath())) {
				dcassert(ctrlQueue.findItem(ii) != -1);
				ii->update();
				ctrlQueue.updateItem(ii);
			}
		}
	}
	if(tasks.size() > 0) {
		tasks.clear();
	}

	return 0;
}

void QueueFrame::moveSelected() {

	int n = ctrlQueue.GetSelectedCount();
	if(n == 1) {
		// Single file, get the full filename and move...
		QueueItemInfo* ii = ctrlQueue.getItemData(ctrlQueue.GetNextItem(-1, LVNI_SELECTED));
		tstring name = ii->getTarget();
		tstring ext = Util::getFileExt(name);
		tstring ext2;
		if (!ext.empty())
		{
			ext = ext.substr(1); // remove leading dot so default extension works when browsing for file
			ext2 = _T("*.") + ext;
			ext2 += (TCHAR)0;
			ext2 += _T("*.") + ext;
		}
		ext2 += _T("*.*");
		ext2 += (TCHAR)0;
		ext2 += _T("*.*");
		ext2 += (TCHAR)0;

		if(WinUtil::browseFile(name, m_hWnd, true, ii->getPath(), ext2.c_str(), ext.empty() ? NULL : ext.c_str())) {
			QueueManager::getInstance()->move(Text::fromT(ii->getTarget()), Text::fromT(name));
		}
	} else if(n > 1) {
		tstring name;
		if(showTree) {
			name = curDir;
		}

		if(WinUtil::browseDirectory(name, m_hWnd)) {
			int i = -1;
			while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
				QueueItemInfo* ii = ctrlQueue.getItemData(i);
				QueueManager::getInstance()->move(Text::fromT(ii->getTarget()), Text::fromT(name + ii->getTargetFileName()));
			}			
		}
	}
}

void QueueFrame::moveSelectedDir() {
	if(ctrlDirs.GetSelectedItem() == NULL)
		return;

	dcassert(!curDir.empty());
	tstring name = curDir;
	
	if(WinUtil::browseDirectory(name, m_hWnd)) {
		moveDir(ctrlDirs.GetSelectedItem(), name);
	}
}

void QueueFrame::moveDir(HTREEITEM ht, const tstring& target) {
	HTREEITEM next = ctrlDirs.GetChildItem(ht);
	while(next != NULL) {
		moveDir(next, target + Util::getLastDir(getDir(next)) + _T(PATH_SEPARATOR_STR));		
		next = ctrlDirs.GetNextSiblingItem(next);
	}
	tstring* s = (tstring*)ctrlDirs.GetItemData(ht);

	DirectoryPair p = directories.equal_range(*s);
	
	for(DirectoryIter i = p.first; i != p.second; ++i) {
		QueueItemInfo* qi = i->second;
		QueueManager::getInstance()->move(Text::fromT(qi->getTarget()), Text::fromT(target + qi->getTargetFileName()));
	}			
}

LRESULT QueueFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlQueue && ctrlQueue.GetSelectedCount() > 0) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	
		for(int i=0;i<10;++i) {
			segmentsMenu.CheckMenuItem(i, MF_BYPOSITION | MF_UNCHECKED);
			segmentsMenu.EnableMenuItem(i + 110, MF_ENABLED);		
		}

		for(int i=0;i<7;i++)
			priorityMenu.CheckMenuItem(i, MF_BYPOSITION | MF_UNCHECKED);
			
		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlQueue, pt);
		}
			
		if(ctrlQueue.GetSelectedCount() > 0) { 
			usingDirMenu = false;
			CMenuItemInfo mi;
		
			while(browseMenu.GetMenuItemCount() > 0) {
				browseMenu.RemoveMenu(0, MF_BYPOSITION);
			}
			while(removeMenu.GetMenuItemCount() > 2) {
				removeMenu.RemoveMenu(2, MF_BYPOSITION);
			}
			while(removeAllMenu.GetMenuItemCount() > 0) {
				removeAllMenu.RemoveMenu(0, MF_BYPOSITION);
			}
			while(pmMenu.GetMenuItemCount() > 0) {
				pmMenu.RemoveMenu(0, MF_BYPOSITION);
			}
			while(readdMenu.GetMenuItemCount() > 2) {
				readdMenu.RemoveMenu(2, MF_BYPOSITION);
			}

			while(previewMenu.GetMenuItemCount() > 0) {
				previewMenu.RemoveMenu(0, MF_BYPOSITION);
			}		

		singleMenu.InsertSeparatorFirst(STRING(FILE));
		multiMenu.InsertSeparatorFirst(STRING(FILES));			
		
		if(ctrlQueue.GetSelectedCount() == 1) {
			QueueItemInfo* ii = ctrlQueue.getItemData(ctrlQueue.GetNextItem(-1, LVNI_SELECTED));
			if(!ii->qi) return 0;

			segmentsMenu.CheckMenuItem(ii->qi->getMaxSegments()-1, MF_BYPOSITION | MF_CHECKED);

			if(ii->qi->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
				segmentsMenu.EnableMenuItem(110, MFS_DISABLED);
			} else {
				for(int i=1;i<10;++i) {
					segmentsMenu.EnableMenuItem(i + 110, MFS_DISABLED);		
				}
			}
			if((ii->isSet(QueueItem::FLAG_USER_LIST)) == false) {
				string ext = Util::getFileExt(Text::fromT(ii->getTargetFileName()));
				if(ext.size()>1) ext = ext.substr(1);
				PreviewAppsSize = WinUtil::SetupPreviewMenu(previewMenu, ext);
				if(previewMenu.GetMenuItemCount() > 0) {
					singleMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_ENABLED);
				} else {
					singleMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_DISABLED);
				}
				previewMenu.InsertSeparatorFirst(STRING(PREVIEW_MENU));
			}

			menuItems = 0;
			int pmItems = 0;
			if(ii) {
				QueueItemInfo::SourceIter i;
				for(i = ii->getSources().begin(); i != ii->getSources().end(); ++i) {
					if(!i->getUser()) {
						continue;
					}
					tstring nick = Text::toT(i->getUser()->getFirstNick());
					mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
					mi.fType = MFT_STRING;
					mi.dwTypeData = (LPTSTR)nick.c_str();
					mi.dwItemData = (ULONG_PTR)&(*i);
					mi.wID = IDC_BROWSELIST + menuItems;
					browseMenu.InsertMenuItem(menuItems, TRUE, &mi);
					mi.wID = IDC_REMOVE_SOURCE + 1 + menuItems; // "All" is before sources
					removeMenu.InsertMenuItem(menuItems + 2, TRUE, &mi); // "All" and separator come first
					mi.wID = IDC_REMOVE_SOURCES + menuItems;
					removeAllMenu.InsertMenuItem(menuItems, TRUE, &mi);
					if(i->getUser()->isOnline()) {
						mi.wID = IDC_PM + menuItems;
						pmMenu.InsertMenuItem(menuItems, TRUE, &mi);
						pmItems++;
					}
					menuItems++;
				}

				readdItems = 0;
				for(i = ii->getBadSources().begin(); i != ii->getBadSources().end(); ++i) {
					if(!i->getUser()) {
						continue;
					}
					tstring nick = Text::toT(i->getUser()->getFirstNick());
					if(i->isSet(QueueItem::Source::FLAG_FILE_NOT_AVAILABLE)) {
						nick += _T(" (") + TSTRING(FILE_NOT_AVAILABLE) + _T(")");
					} else if(i->isSet(QueueItem::Source::FLAG_PASSIVE)) {
						nick += _T(" (") + TSTRING(PASSIVE_USER) + _T(")");
					} else if(i->isSet(QueueItem::Source::FLAG_ROLLBACK_INCONSISTENCY)) {
						nick += _T(" (") + TSTRING(ROLLBACK_INCONSISTENCY) + _T(")");
					} else if(i->isSet(QueueItem::Source::FLAG_BAD_TREE)) {
						nick += _T(" (") + TSTRING(INVALID_TREE) + _T(")");
					} else if(i->isSet(QueueItem::Source::FLAG_SLOW)) {
						nick += _T(" (") + TSTRING(SLOW_USER) + _T(")");
					} else if(i->isSet(QueueItem::Source::FLAG_NO_NEED_PARTS)) {
						nick += _T(" (") + TSTRING(NO_NEEDED_PART) + _T(")");
					}
					mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
					mi.fType = MFT_STRING;
					mi.dwTypeData = (LPTSTR)nick.c_str();
					mi.dwItemData = (ULONG_PTR)&(*i);
					mi.wID = IDC_READD + 1 + readdItems;  // "All" is before sources
					readdMenu.InsertMenuItem(readdItems + 2, TRUE, &mi);  // "All" and separator come first
					readdItems++;
				}
			}

			if(menuItems == 0) {
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)browseMenu, MFS_DISABLED);
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)removeMenu, MFS_DISABLED);
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)removeAllMenu, MFS_DISABLED);
			}
			else {
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)browseMenu, MFS_ENABLED);
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)removeMenu, MFS_ENABLED);
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)removeAllMenu, MFS_ENABLED);
			}

			if(pmItems == 0) {
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)pmMenu, MFS_DISABLED);
			}
			else {
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)pmMenu, MFS_ENABLED);
			}

			if(readdItems == 0) {
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)readdMenu, MFS_DISABLED);
			}
			else {
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)readdMenu, MFS_ENABLED);
 			}

			if(ii->getTTH() == NULL) {
				singleMenu.EnableMenuItem(IDC_COPY_LINK, MFS_DISABLED);
            } else {
                singleMenu.EnableMenuItem(IDC_COPY_LINK, MFS_ENABLED);
            }

			UINT pos = 0;
			switch(ii->getPriority()) {
				case QueueItem::PAUSED: pos = 0; break;
				case QueueItem::LOWEST: pos = 1; break;
				case QueueItem::LOW: pos = 2; break;
				case QueueItem::NORMAL: pos = 3; break;
				case QueueItem::HIGH: pos = 4; break;
				case QueueItem::HIGHEST: pos = 5; break;
				default: dcassert(0); break;
			}
			priorityMenu.CheckMenuItem(pos, MF_BYPOSITION | MF_CHECKED);
			if(ii->getAutoPriority())
				priorityMenu.CheckMenuItem(6, MF_BYPOSITION | MF_CHECKED);

				browseMenu.InsertSeparatorFirst(STRING(GET_FILE_LIST));
				removeMenu.InsertSeparatorFirst(STRING(REMOVE_SOURCE));
				removeAllMenu.InsertSeparatorFirst(STRING(REMOVE_FROM_ALL));
				pmMenu.InsertSeparatorFirst(STRING(SEND_PRIVATE_MESSAGE));
				readdMenu.InsertSeparatorFirst(STRING(READD_SOURCE));
				segmentsMenu.InsertSeparatorFirst(STRING(MAX_SEGMENTS_NUMBER));
				priorityMenu.InsertSeparatorFirst(STRING(PRIORITY));

				singleMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

				segmentsMenu.RemoveFirstItem();
				priorityMenu.RemoveFirstItem();
				browseMenu.RemoveFirstItem();
				removeMenu.RemoveFirstItem();
				removeAllMenu.RemoveFirstItem();
				pmMenu.RemoveFirstItem();
				readdMenu.RemoveFirstItem();
			} else {
				priorityMenu.InsertSeparatorFirst(STRING(PRIORITY));
				multiMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
				priorityMenu.RemoveFirstItem();
			}
		
			singleMenu.RemoveFirstItem();
			multiMenu.RemoveFirstItem();

			return TRUE; 
		}
	} else if (reinterpret_cast<HWND>(wParam) == ctrlDirs && ctrlDirs.GetSelectedItem() != NULL) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlDirs, pt);
		} else {
			// Strange, windows doesn't change the selection on right-click... (!)
			UINT a = 0;
			ctrlDirs.ScreenToClient(&pt);
			HTREEITEM ht = ctrlDirs.HitTest(pt, &a);
			if(ht != NULL && ht != ctrlDirs.GetSelectedItem())
				ctrlDirs.SelectItem(ht);
			ctrlDirs.ClientToScreen(&pt);
		}
		usingDirMenu = true;
		
		priorityMenu.InsertSeparatorFirst(STRING(PRIORITY));
		dirMenu.InsertSeparatorFirst(STRING(FOLDER));
		dirMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		priorityMenu.RemoveFirstItem();
		dirMenu.RemoveFirstItem();
	
		return TRUE;
	}

	bHandled = FALSE;
	return FALSE; 
}

LRESULT QueueFrame::onSearchAlternates(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlQueue.GetSelectedCount() == 1) {
		int i = ctrlQueue.GetNextItem(-1, LVNI_SELECTED);
		QueueItemInfo* ii = ctrlQueue.getItemData(i);
		
		if(ii->getTTH() != NULL) {
			WinUtil::searchHash(ii->getTTH());
		} else {
			tstring searchString = Text::toT(SearchManager::clean(Text::fromT(ii->getTargetFileName())));
		
			if(!searchString.empty()) {
				bool bigFile = (ii->getSize() > 10*1024*1024);
				if(bigFile) {
					SearchFrame::openWindow(searchString, ii->getSize()-1, SearchManager::SIZE_ATLEAST, ShareManager::getInstance()->getType(Text::fromT(ii->getTargetFileName())));
				} else {
					SearchFrame::openWindow(searchString, ii->getSize()+1, SearchManager::SIZE_ATMOST, ShareManager::getInstance()->getType(Text::fromT(ii->getTargetFileName())));
				}		
			}
		}
	}	
	return 0;
}

LRESULT QueueFrame::onCopyMagnet(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlQueue.GetSelectedCount() == 1) {
		int i = ctrlQueue.GetNextItem(-1, LVNI_SELECTED);
		QueueItemInfo* ii = ctrlQueue.getItemData(i);
		WinUtil::copyMagnet(ii->getTTH(), ii->getTargetFileName(), ii->getSize());
	}
	return 0;
}

LRESULT QueueFrame::onBrowseList(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	if(ctrlQueue.GetSelectedCount() == 1) {
		CMenuItemInfo mi;
		mi.fMask = MIIM_DATA;
		
		browseMenu.GetMenuItemInfo(wID, FALSE, &mi);
		OMenuItem* omi = (OMenuItem*)mi.dwItemData;
		QueueItemInfo::SourceInfo* s = (QueueItemInfo::SourceInfo*)omi->data;
		try {
			QueueManager::getInstance()->addList(s->getUser(), QueueItem::FLAG_CLIENT_VIEW);
		} catch(const Exception&) {
		}
	}
	return 0;
}

LRESULT QueueFrame::onReadd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	if(ctrlQueue.GetSelectedCount() == 1) {
		int i = ctrlQueue.GetNextItem(-1, LVNI_SELECTED);
		QueueItemInfo* ii = ctrlQueue.getItemData(i);

		CMenuItemInfo mi;
		mi.fMask = MIIM_DATA;
		
		readdMenu.GetMenuItemInfo(wID, FALSE, &mi);
		if(wID == IDC_READD) {
			// re-add all sources
			for(QueueItemInfo::SourceIter s = ii->getBadSources().begin(); s != ii->getBadSources().end(); ) {
				QueueManager::getInstance()->readd(Text::fromT(ii->getTarget()), s->getUser());
			}
		} else {
			OMenuItem* omi = (OMenuItem*)mi.dwItemData;
			QueueItemInfo::SourceInfo* s = (QueueItemInfo::SourceInfo*)omi->data;
			try {
				QueueManager::getInstance()->readd(Text::fromT(ii->getTarget()), s->getUser());
			} catch(const Exception& e) {
				ctrlStatus.SetText(0, Text::toT(e.getError()).c_str());
			}
		}
	}
	return 0;
}

LRESULT QueueFrame::onRemoveSource(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	if(ctrlQueue.GetSelectedCount() == 1) {
		int i = ctrlQueue.GetNextItem(-1, LVNI_SELECTED);
		QueueItemInfo* ii = ctrlQueue.getItemData(i);
		if(wID == IDC_REMOVE_SOURCE) {
			for(QueueItemInfo::SourceIter si = ii->getSources().begin(); si != ii->getSources().end(); ) {
				QueueManager::getInstance()->removeSource(Text::fromT(ii->getTarget()), si->getUser(), QueueItem::Source::FLAG_REMOVED);
			}
		} else {
			CMenuItemInfo mi;
			mi.fMask = MIIM_DATA;
		
			removeMenu.GetMenuItemInfo(wID, FALSE, &mi);
			OMenuItem* omi = (OMenuItem*)mi.dwItemData;
			QueueItemInfo::SourceInfo* s = (QueueItemInfo::SourceInfo*)omi->data;
			QueueManager::getInstance()->removeSource(Text::fromT(ii->getTarget()), s->getUser(), QueueItem::Source::FLAG_REMOVED);
		}
	}
	return 0;
}

LRESULT QueueFrame::onRemoveSources(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CMenuItemInfo mi;
	mi.fMask = MIIM_DATA;
	removeAllMenu.GetMenuItemInfo(wID, FALSE, &mi);
	OMenuItem* omi = (OMenuItem*)mi.dwItemData;
	QueueItemInfo::SourceInfo* s = (QueueItemInfo::SourceInfo*)omi->data;
	QueueManager::getInstance()->removeSource(s->getUser(), QueueItem::Source::FLAG_REMOVED);
	return 0;
}

LRESULT QueueFrame::onPM(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlQueue.GetSelectedCount() == 1) {
		CMenuItemInfo mi;
		mi.fMask = MIIM_DATA;
		
		pmMenu.GetMenuItemInfo(wID, FALSE, &mi);
		OMenuItem* omi = (OMenuItem*)mi.dwItemData;
		QueueItemInfo::SourceInfo* s = (QueueItemInfo::SourceInfo*)omi->data;
		PrivateFrame::openWindow(s->getUser());
	}
	return 0;
}

LRESULT QueueFrame::onAutoPriority(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {	

	if(usingDirMenu) {
		setAutoPriority(ctrlDirs.GetSelectedItem(), true);
	} else {
		int i = -1;
		while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
			QueueManager::getInstance()->setAutoPriority(Text::fromT(ctrlQueue.getItemData(i)->getTarget()),!ctrlQueue.getItemData(i)->getAutoPriority());
		}
	}
	return 0;
}

LRESULT QueueFrame::onSegments(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
		QueueItemInfo* ii = ctrlQueue.getItemData(i);
		if(ii->qi && ii->qi->isSet(QueueItem::FLAG_MULTI_SOURCE))
			ii->qi->setMaxSegments(max(2, wID - 109));
		ii->update();
		ctrlQueue.updateItem(ii);
	}

	return 0;
}

LRESULT QueueFrame::onPriority(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
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

	if(usingDirMenu) {
		setPriority(ctrlDirs.GetSelectedItem(), p);
	} else {
		int i = -1;
		while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
			QueueManager::getInstance()->setAutoPriority(Text::fromT(ctrlQueue.getItemData(i)->getTarget()), false);
			QueueManager::getInstance()->setPriority(Text::fromT(ctrlQueue.getItemData(i)->getTarget()), p);
		}
	}

	return 0;
}

void QueueFrame::removeDir(HTREEITEM ht) {
	if(ht == NULL)
		return;
	HTREEITEM child = ctrlDirs.GetChildItem(ht);
	while(child) {
		removeDir(child);
		child = ctrlDirs.GetNextSiblingItem(child);
	}
	const tstring& name = getDir(ht);
	DirectoryPair dp = directories.equal_range(name);
	for(DirectoryIter i = dp.first; i != dp.second; ++i) {
		QueueManager::getInstance()->remove(Text::fromT(i->second->getTarget()));
	}
}

/*
 * @param inc True = increase, False = decrease
 */
void QueueFrame::changePriority(bool inc){
	int i = -1;
	while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1){
		QueueItem::Priority p = ctrlQueue.getItemData(i)->getPriority();

		if ((inc && p == QueueItem::HIGHEST) || (!inc && p == QueueItem::PAUSED)){
			// Trying to go higher than HIGHEST or lower than PAUSED
			// so do nothing
			continue;
		}

		switch(p){
			case QueueItem::HIGHEST: p = QueueItem::HIGH; break;
			case QueueItem::HIGH:    p = inc ? QueueItem::HIGHEST : QueueItem::NORMAL; break;
			case QueueItem::NORMAL:  p = inc ? QueueItem::HIGH    : QueueItem::LOW; break;
			case QueueItem::LOW:     p = inc ? QueueItem::NORMAL  : QueueItem::LOWEST; break;
			case QueueItem::LOWEST:  p = inc ? QueueItem::LOW     : QueueItem::PAUSED; break;
			case QueueItem::PAUSED:  p = QueueItem::LOWEST; break;
		}

		QueueManager::getInstance()->setPriority(Text::fromT(ctrlQueue.getItemData(i)->getTarget()), p);
	}
}

void QueueFrame::setPriority(HTREEITEM ht, const QueueItem::Priority& p) {
	if(ht == NULL)
		return;
	HTREEITEM child = ctrlDirs.GetChildItem(ht);
	while(child) {
		setPriority(child, p);
		child = ctrlDirs.GetNextSiblingItem(child);
	}
	const tstring& name = getDir(ht);
	DirectoryPair dp = directories.equal_range(name);
	for(DirectoryIter i = dp.first; i != dp.second; ++i) {
		QueueManager::getInstance()->setPriority(Text::fromT(i->second->getTarget()), p);
	}
}

void QueueFrame::setAutoPriority(HTREEITEM ht, const bool& ap) {
	if(ht == NULL)
		return;
	HTREEITEM child = ctrlDirs.GetChildItem(ht);
	while(child) {
		setAutoPriority(child, ap);
		child = ctrlDirs.GetNextSiblingItem(child);
	}
	const tstring& name = getDir(ht);
	DirectoryPair dp = directories.equal_range(name);
	for(DirectoryIter i = dp.first; i != dp.second; ++i) {
		QueueManager::getInstance()->setAutoPriority(Text::fromT(i->second->getTarget()), ap);
	}
}

void QueueFrame::updateStatus() {
	if(ctrlStatus.IsWindow()) {
		int64_t total = 0;
		int cnt = ctrlQueue.GetSelectedCount();
		if(cnt == 0) {
			cnt = ctrlQueue.GetItemCount();
			if(showTree) {
				for(int i = 0; i < cnt; ++i) {
					QueueItemInfo* ii = ctrlQueue.getItemData(i);
					total += (ii->getSize() > 0) ? ii->getSize() : 0;
				}
			} else {
				total = queueSize;
			}
		} else {
			int i = -1;
			while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
				QueueItemInfo* ii = ctrlQueue.getItemData(i);
				total += (ii->getSize() > 0) ? ii->getSize() : 0;
			}

		}

		tstring tmp1 = Text::toT(STRING(ITEMS) + ": " + Util::toString(cnt));
		tstring tmp2 = Text::toT(STRING(SIZE) + ": " + Util::formatBytes(total));
		bool u = false;

		int w = WinUtil::getTextWidth(tmp1, ctrlStatus.m_hWnd);
		if(statusSizes[1] < w) {
			statusSizes[1] = w;
			u = true;
		}
		ctrlStatus.SetText(2, tmp1.c_str());
		w = WinUtil::getTextWidth(tmp2, ctrlStatus.m_hWnd);
		if(statusSizes[2] < w) {
			statusSizes[2] = w;
			u = true;
		}
		ctrlStatus.SetText(3, tmp2.c_str());

		if(dirty) {
			tmp1 = Text::toT(STRING(FILES) + ": " + Util::toString(queueItems));
			tmp2 = Text::toT(STRING(SIZE) + ": " + Util::formatBytes(queueSize));

			w = WinUtil::getTextWidth(tmp2, ctrlStatus.m_hWnd);
			if(statusSizes[3] < w) {
				statusSizes[3] = w;
				u = true;
			}
			ctrlStatus.SetText(4, tmp1.c_str());

			w = WinUtil::getTextWidth(tmp2, ctrlStatus.m_hWnd);
			if(statusSizes[4] < w) {
				statusSizes[4] = w;
				u = true;
			}
			ctrlStatus.SetText(5, tmp2.c_str());

			dirty = false;
		}

		if(u)
			UpdateLayout(TRUE);
	}
}

void QueueFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[6];
		ctrlStatus.GetClientRect(sr);
		w[5] = sr.right - 16;
#define setw(x) w[x] = max(w[x+1] - statusSizes[x], 0)
		setw(4); setw(3); setw(2); setw(1);

		w[0] = 16;

		ctrlStatus.SetParts(6, w);

		ctrlStatus.GetRect(0, sr);
		ctrlShowTree.MoveWindow(sr);
	}

	if(showTree) {
		if(GetSinglePaneMode() != SPLIT_PANE_NONE) {
			SetSinglePaneMode(SPLIT_PANE_NONE);
			updateQueue();
		}
	} else {
		if(GetSinglePaneMode() != SPLIT_PANE_RIGHT) {
			SetSinglePaneMode(SPLIT_PANE_RIGHT);
			updateQueue();
		}
	}
	
	CRect rc = rect;
	SetSplitterRect(rc);
}

LRESULT QueueFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		QueueManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		closed = true;
		CZDCLib::setButtonPressed(IDC_QUEUE, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		HTREEITEM ht = ctrlDirs.GetRootItem();
		while(ht != NULL) {
			clearTree(ht);
			ht = ctrlDirs.GetNextSiblingItem(ht);
		}

		SettingsManager::getInstance()->set(SettingsManager::QUEUEFRAME_SHOW_TREE, ctrlShowTree.GetCheck() == BST_CHECKED);
		{
			Lock l(cs);
			for(QueueIter i = queue.begin(); i != queue.end(); ++i) {
				delete i->second;
			}
			queue.clear();
		}
		ctrlQueue.DeleteAllItems();

		ctrlQueue.saveHeaderOrder(SettingsManager::QUEUEFRAME_ORDER, 
			SettingsManager::QUEUEFRAME_WIDTHS, SettingsManager::QUEUEFRAME_VISIBLE);

		bHandled = FALSE;
		return 0;
	}
}

LRESULT QueueFrame::onItemChanged(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
	updateQueue();
	return 0;
}

void QueueFrame::updateQueue() {
	Lock l(cs);

	ctrlQueue.DeleteAllItems();
	pair<DirectoryIter, DirectoryIter> i;
	if(showTree) {
		i = directories.equal_range(getSelectedDir());
	} else {
		i.first = directories.begin();
		i.second = directories.end();
	}

	ctrlQueue.SetRedraw(FALSE);
	for(DirectoryIter j = i.first; j != i.second; ++j) {
		QueueItemInfo* ii = j->second;
		ii->update();
		ctrlQueue.insertItem(ctrlQueue.GetItemCount(), ii, WinUtil::getIconIndex(ii->getTarget()));
	}
	ctrlQueue.resort();
	ctrlQueue.SetRedraw(TRUE);
	curDir = getSelectedDir();
	updateStatus();
}

// Put it here to avoid a copy for each recursion...
static TCHAR tmpBuf[1024];
void QueueFrame::moveNode(HTREEITEM item, HTREEITEM parent) {
	TVINSERTSTRUCT tvis;
	memset2(&tvis, 0, sizeof(tvis));
	tvis.itemex.hItem = item;
	tvis.itemex.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_IMAGE | TVIF_INTEGRAL | TVIF_PARAM |
		TVIF_SELECTEDIMAGE | TVIF_STATE | TVIF_TEXT;
	tvis.itemex.pszText = tmpBuf;
	tvis.itemex.cchTextMax = 1024;
	ctrlDirs.GetItem((TVITEM*)&tvis.itemex);
	tvis.hInsertAfter =	TVI_SORT;
	tvis.hParent = parent;
	HTREEITEM ht = ctrlDirs.InsertItem(&tvis);
	HTREEITEM next = ctrlDirs.GetChildItem(item);
	while(next != NULL) {
		moveNode(next, ht);
		next = ctrlDirs.GetChildItem(item);
	}
	ctrlDirs.DeleteItem(item);
}

LRESULT QueueFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	CRect rc;
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;

	if(!BOOLSETTING(SHOW_PROGRESS_BARS)) {
		if (cd->nmcd.dwDrawStage != (CDDS_ITEMPREPAINT)) {
			bHandled = FALSE;
			return 0;
		}
	}

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		{
			QueueItemInfo *ii = (QueueItemInfo*)cd->nmcd.lItemlParam;
			if(ii->getText(COLUMN_ERRORS) != (TSTRING(NO_ERRORS))){
				cd->clrText = SETTING(ERROR_COLOR);
				return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
			}				
		}
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
		// Let's draw a box if needed...
		LVCOLUMN lvc;
		lvc.mask = LVCF_TEXT;
		lvc.pszText = headerBuf;
		lvc.cchTextMax = 128;
		ctrlQueue.GetColumn(cd->iSubItem, &lvc);
		if(Util::stricmp(headerBuf, CTSTRING_I(columnNames[COLUMN_PROGRESS])) == 0) {
			QueueItemInfo *qi = (QueueItemInfo*)cd->nmcd.lItemlParam;
			// draw something nice...
			if(!qi->qi || qi->qi->isSet(QueueItem::FLAG_TESTSUR) || qi->qi->isSet(QueueItem::FLAG_USER_LIST)) {
				bHandled = FALSE;
				return 0;
			}
			ctrlQueue.GetSubItemRect((int)cd->nmcd.dwItemSpec, COLUMN_PROGRESS, LVIR_BOUNDS, rc);

			CRect real_rc = rc;
			rc.MoveToXY(0, 0);
			
			CRect rc2 = rc;
            rc2.left += 6; // indented with 6 pixels
			rc2.right -= 2; // and without messing with the border of the cell				

			CDC cdc;
			cdc.CreateCompatibleDC(cd->nmcd.hdc);
			HBITMAP hBmp = CreateCompatibleBitmap(cd->nmcd.hdc,  real_rc.Width(),  real_rc.Height());
			HBITMAP pOldBmp = cdc.SelectBitmap(hBmp);
			HDC& dc = cdc.m_hDC;

			SetBkMode(dc, TRANSPARENT);
		
			CBarShader statusBar(rc.bottom - rc.top, rc.right - rc.left, SETTING(PROGRESS_BACK_COLOR), qi->getSize());

			FileChunksInfo::Ptr filedatainfo = qi->qi->chunkInfo;

			COLORREF crDownloaded = SETTING(COLOR_DOWNLOADED);
			COLORREF crVerified = SETTING(COLOR_VERIFIED);
			COLORREF crPending = SETTING(COLOR_RUNNING);

			if(filedatainfo) {
				vector<int64_t> v;

				// running chunks
				filedatainfo->getAllChunks(v, 1);
				for(vector<int64_t>::iterator i = v.begin(); (i+1) < v.end(); ++i, ++i) {
					statusBar.FillRange(*i, *(i+1), crPending);
				}
				v.clear();

				// downloaded chunks
				v.push_back(0);
				filedatainfo->getAllChunks(v, 0);
				v.push_back(qi->getSize());
				for(vector<int64_t>::iterator i = v.begin(); (i+1) < v.end(); ++i, ++i) {
					statusBar.FillRange(*i, *(i+1), crDownloaded);
				}
				v.clear();

				// verified chunks
				filedatainfo->getAllChunks(v, 2);
				for(vector<int64_t>::iterator i = v.begin(); (i+1) < v.end(); ++i, ++i) {
					statusBar.FillRange(*i, *(i+1), crVerified);
				}
			} else {
				int64_t possibleVerified = qi->getDownloadedBytes() - (qi->getDownloadedBytes() % 65536);
				statusBar.FillRange(0, possibleVerified, crVerified);
				statusBar.FillRange(possibleVerified, qi->getDownloadedBytes(), crDownloaded);
			}
			statusBar.Draw(cdc, rc.top, rc.left, SETTING(PROGRESS_3DDEPTH));
			BitBlt(cd->nmcd.hdc, real_rc.left, real_rc.top, real_rc.Width(), real_rc.Height(), dc, 0, 0, SRCCOPY);
			DeleteObject(cdc.SelectBitmap(pOldBmp));

			return CDRF_SKIPDEFAULT;
		} else if(Util::stricmp(headerBuf, CTSTRING_I(columnNames[COLUMN_SEGMENTS])) == 0) {
			QueueItemInfo *qi = (QueueItemInfo*)cd->nmcd.lItemlParam;
			if(ctrlQueue.GetItemState((int)cd->nmcd.dwItemSpec, LVIS_SELECTED) & LVIS_SELECTED) {
				if(ctrlQueue.m_hWnd == ::GetFocus()) {
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
				if(qi->getText(COLUMN_ERRORS) != (TSTRING(NO_ERRORS))) {
					SetTextColor(cd->nmcd.hdc, SETTING(ERROR_COLOR));
				} else {
					SetTextColor(cd->nmcd.hdc, WinUtil::textColor);
				}
			}
						
			TCHAR buf[256];
			ctrlQueue.GetItemText((int)cd->nmcd.dwItemSpec, COLUMN_SEGMENTS, buf, 255);
			buf[255] = 0;
			
			ctrlQueue.GetSubItemRect((int)cd->nmcd.dwItemSpec, COLUMN_SEGMENTS, LVIR_BOUNDS, rc);			
	
			HGDIOBJ oldpen = ::SelectObject(cd->nmcd.hdc, CreatePen(PS_SOLID,0, barva));
			HGDIOBJ oldbr = ::SelectObject(cd->nmcd.hdc, CreateSolidBrush(barva));
			Rectangle(cd->nmcd.hdc,rc.left, rc.top, rc.right, rc.bottom);

			DeleteObject(::SelectObject(cd->nmcd.hdc, oldpen));
			DeleteObject(::SelectObject(cd->nmcd.hdc, oldbr));

			if(!qi || !qi->qi || !qi->qi->getHasTree()) {
				DrawIconEx(cd->nmcd.hdc, rc.left, rc.top, hIconNotTree, 16, 16, NULL, NULL, DI_NORMAL | DI_COMPAT);
			} else {
				DrawIconEx(cd->nmcd.hdc, rc.left, rc.top, hIconTree, 16, 16, NULL, NULL, DI_NORMAL | DI_COMPAT);
			}
			
			::DrawText(cd->nmcd.hdc,buf, _tcslen(buf), rc, DT_RIGHT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);

			return CDRF_NEWFONT | CDRF_SKIPDEFAULT;
		}
	default:
		return CDRF_DODEFAULT;
	}
}			

LRESULT QueueFrame::onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	QueueItemInfo *ii = (QueueItemInfo*)ctrlQueue.GetItemData(ctrlQueue.GetNextItem(-1, LVNI_SELECTED));

	if(ii != NULL) {
		int tmp = wID - IDC_COPY;
	
		WinUtil::setClipboard(ii->getText(tmp));
	}
	return 0;
}

LRESULT QueueFrame::onPreviewCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {	
	if(ctrlQueue.GetSelectedCount() == 1) {
		QueueItemInfo* i = ctrlQueue.getItemData(ctrlQueue.GetNextItem(-1, LVNI_SELECTED));
		if(i->qi)
			WinUtil::RunPreviewCommand(wID - IDC_PREVIEW_APP, i->qi->getTempTarget());
	}
	return 0;
}

LRESULT QueueFrame::onRemoveOffline(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
		QueueItemInfo* ii = ctrlQueue.getItemData(i);

		for(QueueItemInfo::SourceIter i = ii->getSources().begin(); i != ii->getSources().end();) {
			if(!i->getUser()->isOnline()) {
				QueueManager::getInstance()->removeSource(Text::fromT(ii->getTarget()), i->getUser(), QueueItem::Source::FLAG_REMOVED);
			} else {
				i++;
			}
		}
	}
	return 0;
}

void QueueFrame::on(SettingsManagerListener::Save, SimpleXML* /*xml*/) throw() {
	bool refresh = false;
	if(ctrlQueue.GetBkColor() != WinUtil::bgColor) {
		ctrlQueue.SetBkColor(WinUtil::bgColor);
		ctrlQueue.SetTextBkColor(WinUtil::bgColor);
		ctrlQueue.setFlickerFree(WinUtil::bgBrush);
		ctrlDirs.SetBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlQueue.GetTextColor() != WinUtil::textColor) {
		ctrlQueue.SetTextColor(WinUtil::textColor);
		ctrlDirs.SetTextColor(WinUtil::textColor);
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
