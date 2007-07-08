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

#if !defined(TRANSFER_VIEW_H)
#define TRANSFER_VIEW_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\client\DownloadManagerListener.h"
#include "..\client\UploadManagerListener.h"
#include "..\client\ConnectionManagerListener.h"
#include "..\client\TaskQueue.h"
#include "..\client\forward.h"
#include "..\client\Util.h"
#include "..\client\Download.h"
#include "..\client\Upload.h"

#include "OMenu.h"
#include "UCHandler.h"
#include "TypedListViewCtrl.h"
#include "WinUtil.h"
#include "resource.h"
#include "SearchFrm.h"

class TransferView : public CWindowImpl<TransferView>, private DownloadManagerListener, 
	private UploadManagerListener, private ConnectionManagerListener,
	public UserInfoBaseHandler<TransferView>, public UCHandler<TransferView>,
	private SettingsManagerListener
{
public:
	DECLARE_WND_CLASS(_T("TransferView"))

	TransferView() : PreviewAppsSize(0) { }
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
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_DESTROY, onDestroy)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		MESSAGE_HANDLER(WM_NOTIFYFORMAT, onNotifyFormat)
		COMMAND_ID_HANDLER(IDC_FORCE, onForce)
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchAlternates)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_REMOVEALL, onRemoveAll)
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchAlternates)
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
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onDoubleClickTransfers(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
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
		ctrlTransfers.deleteAllItems();
		return 0;
	}

	LRESULT onNotifyFormat(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
#ifdef _UNICODE
		return NFR_UNICODE;
#else
		return NFR_ANSI;
#endif		
	}

private:
	class ItemInfo;	
public:
	typedef TypedTreeListViewCtrl<ItemInfo, IDC_TRANSFERS> ItemInfoList;
	ItemInfoList& getUserList() { return ctrlTransfers; }
private:
	enum {
		ADD_ITEM,
		REMOVE_ITEM,
		UPDATE_ITEM,
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

	struct UpdateInfo;
	class ItemInfo : public UserInfoBase {
	public:
		typedef vector<ItemInfo*> List;
		typedef List::const_iterator Iter;

		ItemInfo::List subItems;

		enum Status {
			STATUS_RUNNING,
			STATUS_WAITING,
			// special statuses
			TREE_DOWNLOAD,
			DOWNLOAD_STARTING,
			DOWNLOAD_FINISHED,
		};

		ItemInfo(const UserPtr& u, bool aDownload);

		bool download;
		bool transferFailed;
		bool collapsed;
		bool multiSource;		
		uint8_t flagImage;
		ItemInfo* main;
		UserPtr user;
		Status status;
		int64_t pos;
		int64_t size;
		int64_t start;
		int64_t actual;
		int64_t speed;
		int64_t timeLeft;
		uint64_t fileBegin;
		tstring Target;

		tstring columns[COLUMN_LAST];
		void update(const UpdateInfo& ui);

		const UserPtr& getUser() const { return user; }

		void disconnect();
		void removeAll();

		double getRatio() const { return (pos > 0) ? (double)actual / (double)pos : 1.0; }

		const tstring& getText(uint8_t col) const {
			return columns[col];
		}

		static int compareItems(const ItemInfo* a, const ItemInfo* b, uint8_t col);

		uint8_t imageIndex() const { return static_cast<uint8_t>(!download ? IMAGE_UPLOAD : (!main ? IMAGE_DOWNLOAD : IMAGE_SEGMENT)); }

		ItemInfo* createMainItem() {
	  		ItemInfo* h = new ItemInfo(user, true);
			h->Target = Target;
			h->columns[COLUMN_FILE] = Util::getFileName(h->Target);
			h->columns[COLUMN_PATH] = Util::getFilePath(h->Target);
			h->columns[COLUMN_STATUS] = TSTRING(CONNECTING);
			h->columns[COLUMN_HUB] = _T("0 ") + TSTRING(NUMBER_OF_SEGMENTS);

			return h;
		}
		const tstring& getGroupingString() const { return Target; }
		void updateMainItem() {
			if(main->subItems.size() == 1) {
				ItemInfo* i = main->subItems.front();
				main->user = i->user;
				main->flagImage = i->flagImage;
				main->columns[COLUMN_USER] = Text::toT(main->user->getFirstNick());
				main->columns[COLUMN_HUB] = WinUtil::getHubNames(main->user).first;
				main->columns[COLUMN_IP] = i->columns[COLUMN_IP];
			} else {
				TCHAR buf[256];
				snwprintf(buf, sizeof(buf), _T("%d %s"), main->subItems.size(), CTSTRING(USERS));

				main->columns[COLUMN_USER] = buf;
				main->columns[COLUMN_IP] = Util::emptyStringT;
			}
		}
	};

	struct UpdateInfo : public Task {
		enum {
			MASK_POS = 1 << 0,
			MASK_SIZE = 1 << 1,
			MASK_START = 1 << 2,
			MASK_ACTUAL = 1 << 3,
			MASK_SPEED = 1 << 4,
			MASK_FILE = 1 << 5,
			MASK_STATUS = 1 << 6,
			MASK_TIMELEFT = 1 << 7,
			MASK_IP = 1 << 8,
			MASK_STATUS_STRING = 1 << 9,
			MASK_COUNTRY = 1 << 10,
			MASK_SEGMENT = 1 << 11
		};

		bool operator==(const ItemInfo& ii) { return download == ii.download && user == ii.user; }

		UpdateInfo(const UserPtr& aUser, bool isDownload, bool isTransferFailed = false) : updateMask(0), user(aUser), download(isDownload), transferFailed(isTransferFailed), multiSource(false), fileList(false), flagImage(0) { }

		uint32_t updateMask;

		UserPtr user;
		bool download;
		bool transferFailed;
		bool fileList;
		uint8_t flagImage;		
		void setMultiSource(bool aSeg) { multiSource = aSeg; updateMask |= MASK_SEGMENT; }
		bool multiSource;
		void setStatus(ItemInfo::Status aStatus) { status = aStatus; updateMask |= MASK_STATUS; }
		ItemInfo::Status status;
		void setPos(int64_t aPos) { pos = aPos; updateMask |= MASK_POS; }
		int64_t pos;
		void setSize(int64_t aSize) { size = aSize; updateMask |= MASK_SIZE; }
		int64_t size;
		void setStart(int64_t aStart) { start = aStart; updateMask |= MASK_START; }
		int64_t start;
		void setActual(int64_t aActual) { actual = aActual; updateMask |= MASK_ACTUAL; }
		int64_t actual;
		void setSpeed(int64_t aSpeed) { speed = aSpeed; updateMask |= MASK_SPEED; }
		int64_t speed;
		void setTimeLeft(int64_t aTimeLeft) { timeLeft = aTimeLeft; updateMask |= MASK_TIMELEFT; }
		int64_t timeLeft;
		void setStatusString(const tstring& aStatusString) { statusString = aStatusString; updateMask |= MASK_STATUS_STRING; }
		tstring statusString;
		void setFile(const tstring& aFile) { file = Util::getFileName(aFile); path = Util::getFilePath(aFile); target = aFile; updateMask|= MASK_FILE; }
		tstring file;
		tstring path;
		void setIP(const tstring& aIP) { IP = aIP; updateMask |= MASK_IP; }
		tstring IP;
		tstring target;		
	};

	void speak(uint8_t type, UpdateInfo* ui) { tasks.add(type, ui); PostMessage(WM_SPEAKER); }

	ItemInfoList ctrlTransfers;
	static int columnIndexes[];
	static int columnSizes[];

	OMenu transferMenu;
	OMenu segmentedMenu;
	OMenu usercmdsMenu;
	OMenu previewMenu;
	CImageList arrows;

	TaskQueue tasks;

	StringMap ucLineParams;
	int PreviewAppsSize;

	virtual void on(ConnectionManagerListener::Added, const ConnectionQueueItem* aCqi) throw();
	virtual void on(ConnectionManagerListener::Failed, const ConnectionQueueItem* aCqi, const string& aReason) throw();
	virtual void on(ConnectionManagerListener::Removed, const ConnectionQueueItem* aCqi) throw();
	virtual void on(ConnectionManagerListener::StatusChanged, const ConnectionQueueItem* aCqi) throw();

	virtual void on(DownloadManagerListener::Complete, const Download* aDownload, bool isTree) throw() { onTransferComplete(aDownload, false, Util::getFileName(aDownload->getTarget()), isTree);}
	virtual void on(DownloadManagerListener::Failed, const Download* aDownload, const string& aReason) throw();
	virtual void on(DownloadManagerListener::Starting, const Download* aDownload) throw();
	virtual void on(DownloadManagerListener::Tick, const DownloadList& aDownload) throw();
	virtual void on(DownloadManagerListener::Status, const UserConnection*, const string&) throw();

	virtual void on(UploadManagerListener::Starting, const Upload* aUpload) throw();
	virtual void on(UploadManagerListener::Tick, const UploadList& aUpload) throw();
	virtual void on(UploadManagerListener::Complete, const Upload* aUpload) throw() { onTransferComplete(aUpload, true, aUpload->getSourceFile(), false); }

	virtual void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw();

	void onTransferComplete(const Transfer* aTransfer, bool isUpload, const string& aFileName, bool isTree);

	void CollapseAll();
	void ExpandAll();
	bool mainItemTick(ItemInfo* main, bool);

	void setMainItem(ItemInfo* i) {
		if(i->main != NULL) {
			ItemInfo* h = i->main;		
			if(h->Target != i->Target) {
				ctrlTransfers.removeGroupedItem(i, false);
				ctrlTransfers.insertGroupedItem(i, false);
			}
		} else {
			i->main = ctrlTransfers.findMainItem(i->Target);
		}
	}
};

#endif // !defined(TRANSFER_VIEW_H)

/**
 * @file
 * $Id$
 */