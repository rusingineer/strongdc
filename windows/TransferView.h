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

#ifndef __TRANSFERVIEW_H
#define __TRANSFERVIEW_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/DownloadManager.h"
#include "../client/UploadManager.h"
#include "../client/CriticalSection.h"
#include "../client/ConnectionManagerListener.h"
#include "../client/ConnectionManager.h"
#include "../client/HashManager.h"

#include "OMenu.h"
#include "UCHandler.h"
#include "TypedListViewCtrl.h"
#include "WinUtil.h"
#include "resource.h"
#include "SearchFrm.h"

class TransferView : public CWindowImpl<TransferView>, private DownloadManagerListener, 
	private UploadManagerListener, private ConnectionManagerListener,
	private HashManagerListener,
	public UserInfoBaseHandler<TransferView>, public UCHandler<TransferView>
{
public:
	TransferView() : PreviewAppsSize(0) {
		searchFilter.push_back("the");
		searchFilter.push_back("of");
		searchFilter.push_back("divx");
		searchFilter.push_back("frail");
	};
	~TransferView(void);

	typedef UserInfoBaseHandler<TransferView> uibBase;
	typedef UCHandler<TransferView> ucBase;

	BEGIN_MSG_MAP(TransferView)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_GETDISPINFO, ctrlTransfers.onGetDispInfo)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_COLUMNCLICK, ctrlTransfers.onColumnClick)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_GETINFOTIP, ctrlTransfers.onInfoTip)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_KEYDOWN, onKeyDownTransfers)
		NOTIFY_HANDLER(IDC_TRANSFERS, NM_CUSTOMDRAW, onCustomDraw)
		NOTIFY_HANDLER(IDC_TRANSFERS, NM_DBLCLK, onDoubleClickTransfers)
		NOTIFY_HANDLER(IDC_TRANSFERS, NM_CLICK, onLButton)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_DESTROY, onDestroy)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		COMMAND_ID_HANDLER(IDC_FORCE, onForce)
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchAlternates)
		COMMAND_ID_HANDLER(IDC_OFFCOMPRESS, onOffCompress)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_REMOVEALL, onRemoveAll)
		COMMAND_ID_HANDLER(IDC_CONNECT_ALL, onConnectAll)
		COMMAND_ID_HANDLER(IDC_DISCONNECT_ALL, onDisconnectAll)
		COMMAND_ID_HANDLER(IDC_COLLAPSE_ALL, onCollapseAll)
		COMMAND_ID_HANDLER(IDC_EXPAND_ALL, onExpandAll)
		COMMAND_ID_HANDLER(IDC_MENU_SLOWDISCONNECT, onSlowDisconnect)
		MESSAGE_HANDLER_HWND(WM_INITMENUPOPUP, OMenu::onInitMenuPopup)
		MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
		MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)
		COMMAND_RANGE_HANDLER(IDC_PREVIEW_APP, IDC_PREVIEW_APP + PreviewAppsSize, onPreviewCommand)
		CHAIN_COMMANDS(ucBase)
		CHAIN_COMMANDS(uibBase)
	END_MSG_MAP()

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onForce(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);			
	LRESULT onSearchAlternates(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOffCompress(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onDoubleClickTransfers(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onLButton(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled); 
	LRESULT onConnectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDisconnectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSlowDisconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPreviewCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onWhoisIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void runUserCommand(UserCommand& uc);
	void prepareClose();

	LRESULT onCollapseAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		CollapseAll();
		return 0;
	}

	LRESULT onExpandAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ExpandAll();
		return 0;
	}

	LRESULT onKeyDownTransfers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
		if(kd->wVKey == VK_DELETE) {
			ctrlTransfers.forEachSelected(&ItemInfo::disconnect);
		}
		return 0;
	}

	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlTransfers.forEachSelected(&ItemInfo::disconnect);
		return 0;
	}

	LRESULT onRemoveAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlTransfers.forEachSelected(&ItemInfo::removeAll);
		return 0;
	}

	LRESULT onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ItemInfo::Map::iterator i;
		for(i = transferItems.begin(); i != transferItems.end(); ++i) {
			delete i->second;
		}

		int q = 0;
		while(q<mainItems.size()) {
			ItemInfo* m = mainItems[q];
			delete m;
			q++;
		}

		return 0;
	}
private:
	/** Parameter map for user commands */
	StringMap ucParams;
	class ItemInfo;	
	int PreviewAppsSize;
public:
	TypedListViewCtrl<ItemInfo, IDC_TRANSFERS>& getUserList() { return ctrlTransfers; };
private:
	enum {
		ADD_ITEM,
		REMOVE_ITEM,
		UPDATE_ITEM,
		UPDATE_ITEMS,
		SET_STATE,
		REMOVE_ITEM_BUT_NOT_FREE,
		INSERT_SUBITEM
	};

	enum {
		COLUMN_FIRST,
		COLUMN_USER = COLUMN_FIRST,
		COLUMN_HUB,
		COLUMN_STATUS,
		COLUMN_TIMELEFT,
		COLUMN_SPEED,
		COLUMN_FILE,
		COLUMN_SIZE,
		COLUMN_PATH,
		COLUMN_IP,
		COLUMN_RATIO,
		COLUMN_LAST
	};

	enum {
		IMAGE_DOWNLOAD = 0,
		IMAGE_UPLOAD,
		IMAGE_SEGMENT
	};

	class ItemInfo : public UserInfoBase, public Flags {
	public:
		typedef HASH_MAP<ConnectionQueueItem*, ItemInfo*, PointerHash<ConnectionQueueItem> > Map;
		typedef Map::iterator MapIter;

		typedef ItemInfo* Ptr;
		typedef vector<Ptr> List;
		typedef List::iterator Iter;

		enum Flags {
			FLAG_COMPRESSED = 0x01
		};
		enum Status {
			STATUS_RUNNING,
			STATUS_WAITING
		};
		enum Types {
			TYPE_DOWNLOAD,
			TYPE_UPLOAD
		};

		ItemInfo(const User::Ptr& u, Types t = TYPE_DOWNLOAD, Status s = STATUS_WAITING, 
			int64_t p = 0, int64_t sz = 0, int st = 0, int a = 0) : UserInfoBase(u), type(t), 
			status(s), pos(p), size(sz), start(st), actual(a), speed(0), timeLeft(0), qi(NULL),
			updateMask((u_int32_t)-1), collapsed(true), mainItem(false), upper(NULL), stazenoCelkem(0),
			dwnldStart(0), pocetUseru(1), oldTarget(Util::emptyString), celkovaRychlost(0),
			compressRatio(1.0), finished(false) { update(); };

		Types type;
		Status status;
		int64_t pos;
		int64_t size;
		int64_t start;
		int64_t actual;
		int64_t speed;
		int64_t celkovaRychlost;
		int64_t timeLeft;
		int64_t stazenoCelkem;
		int64_t dwnldStart;
		string statusString;
		string file;
		string path;
		string IP;
		string country;		
		QueueItem* qi;
		ItemInfo* upper;
		string Target;
		bool collapsed;
		bool mainItem;
		int pocetUseru;
		string oldTarget;
		double compressRatio;
		string downloadTarget;
		bool finished;

		enum {
			MASK_USER = 1 << COLUMN_USER,
			MASK_HUB = 1 << COLUMN_HUB,
			MASK_STATUS = 1 << COLUMN_STATUS,
			MASK_TIMELEFT = 1 << COLUMN_TIMELEFT,
			MASK_SPEED = 1 << COLUMN_SPEED,
			MASK_FILE = 1 << COLUMN_FILE,
			MASK_SIZE = 1 << COLUMN_SIZE,
			MASK_PATH = 1 << COLUMN_PATH,
			MASK_IP = 1 << COLUMN_IP,
			MASK_RATIO = 1 << COLUMN_RATIO,
	};
		string columns[COLUMN_LAST];
		u_int32_t updateMask;
		void update();

		void disconnect();
		void removeAll();
		void deleteSelf() { delete this; }	

		double getRatio() {
			if(mainItem) {
				if((compressRatio > 1) || (compressRatio < 0)) compressRatio = 1.0;
				return compressRatio;
			}
			return (pos > 0) ? (double)actual / (double)pos : 1.0;
		}

		const string& getText(int col) const {
			return columns[col];
		}

		static bool canBeSorted(ItemInfo* a, ItemInfo* b) {
			if((a->Target == b->Target) && (!a->mainItem) && (!b->mainItem))
				return true;

			if((a->type == ItemInfo::TYPE_UPLOAD) && (b->type == ItemInfo::TYPE_UPLOAD))
				return true;

			if((a->type == ItemInfo::TYPE_UPLOAD) && (b->mainItem) && (b->collapsed))
				return true;

			if((b->type == ItemInfo::TYPE_UPLOAD) && (a->mainItem) && (a->collapsed))
				return true;

			if(a->mainItem && b->mainItem && a->collapsed && b->collapsed)
				return true;

			return false;
		}

		static int compareItems(ItemInfo* a, ItemInfo* b, int col) {
			if(!canBeSorted(a,b)) 
				return 0;

			if(a->status == b->status) {
				if(a->type != b->type) {
					return (a->type == ItemInfo::TYPE_DOWNLOAD) ? -1 : 1;					
				}
			} else {
				return (a->status == ItemInfo::STATUS_RUNNING) ? -1 : 1;
			}

			switch(col) {
				case COLUMN_USER: 
					if((a->pocetUseru == 1) || (b->pocetUseru == 1))
						return Util::stricmp(a->columns[COLUMN_USER], b->columns[COLUMN_USER]);
			
					return compare(a->pocetUseru, b->pocetUseru);
				case COLUMN_HUB: return Util::stricmp(a->columns[COLUMN_HUB], b->columns[COLUMN_HUB]);
				case COLUMN_STATUS: return 0;
				case COLUMN_TIMELEFT: return compare(a->timeLeft, b->timeLeft);
				case COLUMN_SPEED: return compare(a->speed, b->speed);
				case COLUMN_FILE: return Util::stricmp(a->columns[COLUMN_FILE], b->columns[COLUMN_FILE]);
				case COLUMN_SIZE: return compare(a->size, b->size);
				case COLUMN_PATH: return Util::stricmp(a->columns[COLUMN_PATH], b->columns[COLUMN_PATH]);
				case COLUMN_IP: return Util::stricmp(a->columns[COLUMN_IP], b->columns[COLUMN_IP]);
				case COLUMN_RATIO: return compare(a->getRatio(), b->getRatio());
			}
			return 0;
		}

		bool canDisplayUpper();
	};

	CriticalSection cs;
	ItemInfo::Map transferItems;
	ItemInfo::List mainItems;

	TypedListViewCtrl<ItemInfo, IDC_TRANSFERS> ctrlTransfers;
	static int columnIndexes[];
	static int columnSizes[];

	OMenu transferMenu;
	OMenu segmentedMenu;
	OMenu usercmdsMenu;
	OMenu previewMenu;
	CImageList arrows;
	CImageList states;
	HICON hIconCompressed;

	HDC hDCDB; // Double buffer DC
	HBITMAP hDCBitmap; // Double buffer Bitmap for above DC
	StringList searchFilter;

	virtual void on(ConnectionManagerListener::Added, ConnectionQueueItem* aCqi) throw();
	virtual void on(ConnectionManagerListener::Failed, ConnectionQueueItem* aCqi, const string& aReason) throw();
	virtual void on(ConnectionManagerListener::Removed, ConnectionQueueItem* aCqi) throw();
	virtual void on(ConnectionManagerListener::StatusChanged, ConnectionQueueItem* aCqi) throw();

	virtual void on(DownloadManagerListener::Complete, Download* aDownload) throw() { onTransferComplete(aDownload, false);}
	virtual void on(DownloadManagerListener::Failed, Download* aDownload, const string& aReason) throw();
	virtual void on(DownloadManagerListener::Starting, Download* aDownload) throw();
	virtual void on(DownloadManagerListener::Tick, const Download::List& aDownload) throw();

	virtual void on(UploadManagerListener::Starting, Upload* aUpload) throw();
	virtual void on(UploadManagerListener::Tick, const Upload::List& aUpload) throw();
	virtual void on(UploadManagerListener::Complete, Upload* aUpload) throw() { onTransferComplete(aUpload, true); }

	virtual void on(HashManagerListener::TTHDone, const string& fname, TTHValue* root) throw() { }
	virtual void on(HashManagerListener::Finished) throw() { }
	virtual void on(HashManagerListener::Verifying, const string& fileName, int64_t remainingBytes) throw();

	void onTransferComplete(Transfer* aTransfer, bool isUpload);

	void InsertItem(ItemInfo* i);
	void Collapse(ItemInfo* i, int a);
	void CollapseAll();
	void ExpandAll();
	void Expand(ItemInfo* i, int a);
	void setMainItem(ItemInfo* i);
	void insertSubItem(ItemInfo* j, int idx);

	ItemInfo* findMainItem(string Target);

};

#endif // __TRANSFERVIEW_H

/**
 * @file
 * $Id$
 */