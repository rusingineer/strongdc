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

#include "OMenu.h"
#include "UCHandler.h"

#include "TypedListViewCtrl.h"
#include "WinUtil.h"
#include "resource.h"

class TransferView : public CWindowImpl<TransferView>, private DownloadManagerListener, 
	private UploadManagerListener, private ConnectionManagerListener,
	public UserInfoBaseHandler<TransferView>, public UCHandler<TransferView>
{
public:
	TransferView() { };
	~TransferView(void);

	typedef UserInfoBaseHandler<TransferView> uibBase;
	typedef UCHandler<TransferView> ucBase;

	BEGIN_MSG_MAP(TransferView)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_GETDISPINFO, ctrlTransfers.onGetDispInfo)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_COLUMNCLICK, ctrlTransfers.onColumnClick)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_KEYDOWN, onKeyDownTransfers)
		NOTIFY_HANDLER(IDC_TRANSFERS, NM_CUSTOMDRAW, onCustomDraw)
		NOTIFY_HANDLER(IDC_TRANSFERS, NM_DBLCLK, onDoubleClickTransfers)
		NOTIFY_HANDLER(IDC_TRANSFERS, NM_CLICK, onLButton)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_DESTROY, onDestroy)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		//MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBackground)
		COMMAND_ID_HANDLER(IDC_FORCE, onForce)
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchAlternates)
		COMMAND_ID_HANDLER(IDC_OFFCOMPRESS, onOffCompress)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_REMOVEALL, onRemoveAll)
		MESSAGE_HANDLER_HWND(WM_INITMENUPOPUP, OMenu::onInitMenuPopup)
		MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
		MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)
//PDC {
		COMMAND_RANGE_HANDLER(IDC_PREVIEW_APP, IDC_PREVIEW_APP + PreviewAppsSize, onPreviewCommand)
//PDC }		
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

	void runUserCommand(UserCommand& uc);
//PDC {
	LRESULT onPreviewCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
//PDC }
	void prepareClose();

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
		ctrlTransfers.forEach(&ItemInfo::deleteSelf);
		return 0;
	}
private:
	/** Parameter map for user commands */
	StringMap ucParams;
	class ItemInfo;	
//PDC {
	int PreviewAppsSize;
//PDC }
public:
	TypedListViewCtrl<ItemInfo, IDC_TRANSFERS>& getUserList() { return ctrlTransfers; };
private:
	enum {
		ADD_ITEM,
		REMOVE_ITEM,
		UPDATE_ITEM,
		UPDATE_ITEMS,
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
		IMAGE_UPLOAD
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
			status(s), pos(p), size(sz), start(st), actual(a), speed(0), timeLeft(0), seznam(0),
			updateMask((u_int32_t)-1), collapsed(true), mainItem(false), upper(NULL), stazenoCelkem(0),
			dwnldStart(0), pocetUseru(0), oldTarget(Util::emptyString), Target(Util::emptyString)
			{ update(); };

		Types type;
		Status status;
		int64_t pos;
		int64_t size;
		int64_t start;
		int64_t actual;
		int64_t speed;
		int64_t timeLeft;
		int64_t stazenoCelkem;
		int64_t dwnldStart;
		string statusString;
		string file;
		string path;
		string IP;
		QueueItem* qi;
		ItemInfo* upper;
		string Target;
		Download::List seznam;
		bool collapsed;
		bool mainItem;
		int pocetUseru;
		ItemInfo::Map subItems;
		string oldTarget;

//PDC {
		string downloadTarget;
//PDC }

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

		double getRatio() { return (pos > 0) ? (double)actual / (double)pos : 1.0;	}

		const string& getText(int col) const {
			return columns[col];
		}

		static int compareItems(ItemInfo* a, ItemInfo* b, int col);
		static bool canBeSorted(ItemInfo* a, ItemInfo* b);
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
//PDC {
	OMenu previewMenu;
//PDC {
	CImageList arrows;
	CImageList states;
	HICON hIconCompressed;

	HDC hDCDB; // Double buffer DC
	HBITMAP hDCBitmap; // Double buffer Bitmap for above DC

	void onConnectionAdded(ConnectionQueueItem* aCqi);
	void onConnectionConnected(ConnectionQueueItem* /*aCqi*/) { };
	void onConnectionFailed(ConnectionQueueItem* aCqi, const string& aReason);
	void onConnectionRemoved(ConnectionQueueItem* aCqi);
	void onConnectionStatus(ConnectionQueueItem* aCqi);

	void onDownloadComplete(Download* aDownload);
	void onDownloadFailed(Download* aDownload, const string& aReason);
	void onDownloadStarting(Download* aDownload);
	void onDownloadTick(const Download::List& aDownload);

	void onUploadStarting(Upload* aUpload);
	void onUploadTick(const Upload::List& aUpload);
	void onUploadComplete(Upload* aUpload);

	void onTransferComplete(Transfer* aTransfer, bool isUpload);

	void InsertItem(ItemInfo* i);
	void Collapse(ItemInfo* i, int a);
	void CollapseAll();
	void Expand(ItemInfo* i, int a);
	void setMainItem(ItemInfo* i);

	// ConnectionManagerListener
	virtual void onAction(ConnectionManagerListener::Types type, ConnectionQueueItem* aCqi) throw();
	virtual void onAction(ConnectionManagerListener::Types type, ConnectionQueueItem* aCqi, const string& aLine) throw();	

	// DownloadManagerListener
	virtual void onAction(DownloadManagerListener::Types type, Download* aDownload) throw();
	virtual void onAction(DownloadManagerListener::Types type, const Download::List& dl) throw();
	virtual void onAction(DownloadManagerListener::Types type, Download* aDownload, const string& aReason) throw();

	// UploadManagerListener
	virtual void onAction(UploadManagerListener::Types type, Upload* aUpload) throw();
	virtual void onAction(UploadManagerListener::Types type, const Upload::List& ul) throw();
};

#endif // __TRANSFERVIEW_H

/**
 * @file
 * $Id$
 */