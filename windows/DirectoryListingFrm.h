/*
 * Copyright (C) 2001-2005 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(DIRECTORY_LISTING_FRM_H)
#define DIRECTORY_LISTING_FRM_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "../client/User.h"
#include "../client/FastAlloc.h"

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"
#include "WinUtil.h"
#include "UCHandler.h"
#include "QueueFrame.h"
#include "SearchFrm.h"

#include "../client/DirectoryListing.h"
#include "../client/StringSearch.h"
#include "../client/ShareManager.h"
#include "../client/ADLSearch.h"

class ThreadedDirectoryListing;

#define STATUS_MESSAGE_MAP 9
#define CONTROL_MESSAGE_MAP 10
class DirectoryListingFrame : public MDITabChildWindowImpl<DirectoryListingFrame, RGB(255, 0, 255), IDR_DIRECTORY>, public CSplitterImpl<DirectoryListingFrame>, 
	public UCHandler<DirectoryListingFrame>, private SettingsManagerListener

{
public:
	static void openWindow(const tstring& aFile, const User::Ptr& aUser, int64_t aSpeed);
	static void openWindow(const User::Ptr& aUser, const string& txt, int64_t aSpeed);

	typedef MDITabChildWindowImpl<DirectoryListingFrame, RGB(255, 0, 255), IDR_DIRECTORY> baseClass;
	typedef UCHandler<DirectoryListingFrame> ucBase;

	enum {
		COLUMN_FILENAME,
		COLUMN_TYPE,
		COLUMN_EXACTSIZE,
		COLUMN_SIZE,
		COLUMN_TTH,
		COLUMN_LAST
	};
	
	enum {
		STATUS_TEXT,
		STATUS_SPEED,
		STATUS_TOTAL_FILES,
		STATUS_TOTAL_SIZE,
		STATUS_SELECTED_FILES,
		STATUS_SELECTED_SIZE,
		STATUS_FILE_LIST_DIFF,
		STATUS_MATCH_QUEUE,
		STATUS_FIND,
		STATUS_NEXT,
		STATUS_DUMMY,
		STATUS_LAST
	};
	
	enum {
		FINISHED,
		ABORTED
	};	
	
	DirectoryListingFrame(const User::Ptr& aUser, int64_t aSpeed);
	virtual ~DirectoryListingFrame() { 
		dcassert(lists.find(dl->getUser()) != lists.end());
		lists.erase(dl->getUser());
	}


	DECLARE_FRAME_WND_CLASS(_T("DirectoryListingFrame"), IDR_DIRECTORY)

	BEGIN_MSG_MAP(DirectoryListingFrame)
		NOTIFY_HANDLER(IDC_FILES, LVN_GETDISPINFO, ctrlList.onGetDispInfo)
		NOTIFY_HANDLER(IDC_FILES, LVN_COLUMNCLICK, ctrlList.onColumnClick)
		NOTIFY_HANDLER(IDC_FILES, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_FILES, NM_DBLCLK, onDoubleClickFiles)
		NOTIFY_HANDLER(IDC_FILES, LVN_ITEMCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_KEYDOWN, onKeyDownDirs)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_SELCHANGED, onSelChangedDirectories)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(FTM_CONTEXTMENU, onTabContextMenu)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		COMMAND_ID_HANDLER(IDC_DOWNLOAD, onDownload)
		COMMAND_ID_HANDLER(IDC_DOWNLOADDIR, onDownloadDir)
		COMMAND_ID_HANDLER(IDC_DOWNLOADDIRTO, onDownloadDirTo)
		COMMAND_ID_HANDLER(IDC_DOWNLOADTO, onDownloadTo)
		COMMAND_ID_HANDLER(IDC_GO_TO_DIRECTORY, onGoToDirectory)
		COMMAND_ID_HANDLER(IDC_VIEW_AS_TEXT, onViewAsText)
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchByTTH)
		COMMAND_ID_HANDLER(IDC_COPY_LINK, onCopy)
		COMMAND_ID_HANDLER(IDC_COPY_TTH, onCopy)
		COMMAND_ID_HANDLER(IDC_ADD_TO_FAVORITES, onAddToFavorites)
		COMMAND_ID_HANDLER(IDC_PRIVATEMESSAGE, onPM)
		COMMAND_ID_HANDLER(IDC_COPY_NICK, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_FILENAME, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_SIZE, onCopy);
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_TARGET, IDC_DOWNLOAD_TARGET + targets.size() + WinUtil::lastDirs.size(), onDownloadTarget)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_TARGET_DIR, IDC_DOWNLOAD_TARGET_DIR + WinUtil::lastDirs.size(), onDownloadTargetDir)
		COMMAND_RANGE_HANDLER(IDC_PRIORITY_PAUSED, IDC_PRIORITY_HIGHEST, onDownloadWithPrio)
		COMMAND_RANGE_HANDLER(IDC_PRIORITY_PAUSED+90, IDC_PRIORITY_HIGHEST+90, onDownloadDirWithPrio)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_FAVORITE_DIRS, IDC_DOWNLOAD_FAVORITE_DIRS + FavoriteManager::getInstance()->getFavoriteDirs().size(), onDownloadFavoriteDirs)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS, IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS + FavoriteManager::getInstance()->getFavoriteDirs().size(), onDownloadWholeFavoriteDirs)
		CHAIN_COMMANDS(ucBase)
		CHAIN_MSG_MAP(baseClass)
		CHAIN_MSG_MAP(CSplitterImpl<DirectoryListingFrame>)
	ALT_MSG_MAP(STATUS_MESSAGE_MAP)
		COMMAND_ID_HANDLER(IDC_FIND, onFind)
		COMMAND_ID_HANDLER(IDC_NEXT, onNext)
		COMMAND_ID_HANDLER(IDC_MATCH_QUEUE, onMatchQueue)
		COMMAND_ID_HANDLER(IDC_FILELIST_DIFF, onListDiff)
	ALT_MSG_MAP(CONTROL_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_XBUTTONUP, onXButtonUp)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onDownload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadWithPrio(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadDirWithPrio(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadDirTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onViewAsText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchByTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPM(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGoToDirectory(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadTarget(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadTargetDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDoubleClickFiles(int idCtrl, LPNMHDR pnmh, BOOL& bHandled); 
	LRESULT onSelChangedDirectories(int idCtrl, LPNMHDR pnmh, BOOL& bHandled); 
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onXButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onDownloadFavoriteDirs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadWholeFavoriteDirs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/); 

	void downloadList(const tstring& aTarget, bool view = false,  QueueItem::Priority prio = QueueItem::DEFAULT);
	void updateTree(DirectoryListing::Directory* tree, HTREEITEM treeItem);
	void UpdateLayout(BOOL bResizeBars = TRUE);
	void findFile(bool findNext);
	void runUserCommand(UserCommand& uc);
	void loadFile(const tstring& name);
	void loadXML(const string& txt);
	void refreshTree(const tstring& root);

	HTREEITEM findItem(HTREEITEM ht, const tstring& name);
	void selectItem(const tstring& name);
	
	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
		updateStatus();
		return 0;
	}

	LRESULT onSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlList.SetFocus();
		return 0;
	}

	void setWindowTitle() {
		if(error.empty())
			SetWindowText((WinUtil::getNicks(dl->getUser()) + _T(" - ") + WinUtil::getHubNames(dl->getUser()).first).c_str());
		else
			SetWindowText(error.c_str());		
	}

	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return 1;
	}

	void clearList() {
		int j = ctrlList.GetItemCount();
		for(int i = 0; i < j; i++) {
			delete (ItemInfo*)ctrlList.GetItemData(i);
		}
		ctrlList.DeleteAllItems();
	}

	LRESULT onFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		searching = true;
		findFile(false);
		searching = false;
		updateStatus();
		return 0;
	}
	LRESULT onNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		searching = true;
		findFile(true);
		searching = false;
		updateStatus();
		return 0;
	}

	LRESULT onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onListDiff(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	LRESULT onKeyDownDirs(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMTVKEYDOWN* kd = (NMTVKEYDOWN*) pnmh;
		if(kd->wVKey == VK_TAB) {
			onTab();
		}
		return 0;
	}

	void onTab() {
		HWND focus = ::GetFocus();
		if(focus == ctrlTree.m_hWnd) {
			ctrlList.SetFocus();
		} else if(focus == ctrlList.m_hWnd) {
			ctrlTree.SetFocus();
		}
	}
	
	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}

private:
	friend class ThreadedDirectoryListing;
	
	void changeDir(DirectoryListing::Directory* d, BOOL enableRedraw);
	HTREEITEM findFile(const StringSearch& str, HTREEITEM root, int &foundFile, int &skipHits);
	void updateStatus();
	void initStatus();
	void addHistory(const string& name);
	void up();
	void back();
	void forward();

	class ItemInfo : public FastAlloc<ItemInfo> {
	public:
		enum ItemType {
			FILE,
			DIRECTORY
		} type;
		
		union {
			DirectoryListing::File* file;
			DirectoryListing::Directory* dir;
		};

		ItemInfo(DirectoryListing::File* f, bool utf8) : type(FILE), file(f) { 
			if(utf8) {
				columns[COLUMN_FILENAME] = Text::toT(f->getName());
			} else {
				columns[COLUMN_FILENAME] = Text::toT(Text::acpToUtf8(f->getName()));
			}
			columns[COLUMN_TYPE] = Util::getFileExt(columns[COLUMN_FILENAME]);
			if(columns[COLUMN_TYPE].size() > 0 && columns[COLUMN_TYPE][0] == '.')
				columns[COLUMN_TYPE].erase(0, 1);

			columns[COLUMN_EXACTSIZE] = Text::toT(Util::formatExactSize(f->getSize()));
			columns[COLUMN_SIZE] =  Text::toT(Util::formatBytes(f->getSize()));
			if(f->getTTH() != NULL)
				columns[COLUMN_TTH] = Text::toT(f->getTTH()->toBase32());
		}
		ItemInfo(DirectoryListing::Directory* d, bool utf8) : type(DIRECTORY), dir(d) { 
			if(utf8) {
				columns[COLUMN_FILENAME] = Text::toT(d->getName());
			} else {
				columns[COLUMN_FILENAME] = Text::toT(Text::acpToUtf8(d->getName()));
			}
			columns[COLUMN_EXACTSIZE] = Text::toT(Util::formatExactSize(d->getTotalSize()));
			columns[COLUMN_SIZE] = Text::toT(Util::formatBytes(d->getTotalSize()));
		}

		const tstring& getText(int col) {
			return columns[col];
		}
		
		struct TotalSize {
			TotalSize() : total(0) { }
			void operator()(ItemInfo* a) { total += a->type == DIRECTORY ? a->dir->getTotalSize() : a->file->getSize(); }
			int64_t total;
		};

		static int compareItems(ItemInfo* a, ItemInfo* b, int col) {
			if(a->type == DIRECTORY) {
				if(b->type == DIRECTORY) {
					switch(col) {
					case COLUMN_EXACTSIZE: return compare(a->dir->getTotalSize(), b->dir->getTotalSize());
					case COLUMN_SIZE: return compare(a->dir->getTotalSize(), b->dir->getTotalSize());
					default: return lstrcmpi(a->columns[col].c_str(), b->columns[col].c_str());
					}
				} else {
					return -1;
				}
			} else if(b->type == DIRECTORY) {
				return 1;
			} else {
				switch(col) {
				case COLUMN_EXACTSIZE: return compare(a->file->getSize(), b->file->getSize());
				case COLUMN_SIZE: return compare(a->file->getSize(), b->file->getSize());
				default: return lstrcmp(a->columns[col].c_str(), b->columns[col].c_str());
				}
			}
		}
		int imageIndex() {
			if(type == DIRECTORY)
				return WinUtil::getDirIconIndex();
			else
				return WinUtil::getIconIndex(getText(COLUMN_FILENAME));
		}

	private:
		tstring columns[COLUMN_LAST];
	};
	
	OMenu targetMenu;
	OMenu targetDirMenu;
	OMenu fileMenu;
	OMenu directoryMenu;
	OMenu priorityMenu;
	OMenu priorityDirMenu;
	OMenu copyMenu;
	OMenu tabMenu;

	CContainedWindow statusContainer;
	CContainedWindow treeContainer;
	CContainedWindow listContainer;

	StringList targets;
	
	deque<string> history;
	size_t historyIndex;
	
	CTreeViewCtrl ctrlTree;
	TypedListViewCtrl<ItemInfo, IDC_FILES> ctrlList;
	CStatusBarCtrl ctrlStatus;
	HTREEITEM treeRoot;
	
	CButton ctrlFind, ctrlFindNext;
	CButton ctrlListDiff;
	CButton ctrlMatchQueue;

	string findStr;
	tstring error;
	string size;

	int skipHits;

	size_t files;
	int64_t speed;		/**< Speed at which this file list was downloaded */

	bool updating;
	bool searching;
	bool closed;
	bool loading;

	int statusSizes[10];
	
	auto_ptr<DirectoryListing> dl;

	typedef HASH_MAP_X(User::Ptr, DirectoryListingFrame*, User::HashFunction, equal_to<User::Ptr>, less<User::Ptr>) UserMap;
	typedef UserMap::const_iterator UserIter;
	
	static UserMap lists;

	static int columnIndexes[COLUMN_LAST];
	static int columnSizes[COLUMN_LAST];
	
	virtual void on(SettingsManagerListener::Save, SimpleXML* /*xml*/) throw();
};

class ThreadedDirectoryListing : public Thread
{
public:
	ThreadedDirectoryListing(DirectoryListingFrame* pWindow, 
		const string& pFile, const string& pTxt) : mWindow(pWindow),
		mFile(pFile), mTxt(pTxt)
	{ }

protected:
	DirectoryListingFrame* mWindow;
	string mFile;
	string mTxt;

private:
	virtual int run()
	{
		try
		{
			if(!mFile.empty()) {
				mWindow->dl->loadFile(mFile);
				ADLSearchManager::getInstance()->matchListing(*mWindow->dl);
				mWindow->refreshTree(Text::toT(WinUtil::getInitialDir(mWindow->dl->getUser())));
			} else {
				mWindow->refreshTree(Text::toT(Util::toNmdcFile(mWindow->dl->loadXML(mTxt, true))));
			}

			mWindow->PostMessage(WM_SPEAKER, DirectoryListingFrame::FINISHED);
		}catch(const AbortException) {
			mWindow->PostMessage(WM_SPEAKER, DirectoryListingFrame::ABORTED);
		} catch(const Exception& e) {
			mWindow->error = WinUtil::getNicks(mWindow->dl->getUser()) + Text::toT(": " + e.getError());
		}

		//cleanup the thread object
		delete this;

		return 0;
	}
};

#endif // !defined(DIRECTORY_LISTING_FRM_H)

/**
 * @file
 * $Id$
 */
