#if !defined(__UPLOAD_QUEUE_FRAME_H__)
#define __UPLOAD_QUEUE_FRAME_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "ExListViewCtrl.h"
#include "WinUtil.h"
#include "../client/UploadManager.h"

#define UPLOADQUEUE_MESSAGE_MAP 666
#define SHOWTREE_MESSAGE_MAP 12

class UploadQueueFrame : public MDITabChildWindowImpl<UploadQueueFrame, RGB(0, 0, 0), IDR_UPLOAD_QUEUE>, public UploadManagerListener, public StaticFrame<UploadQueueFrame, ResourceManager::UPLOAD_QUEUE, IDC_UPLOAD_QUEUE>,
	public CSplitterImpl<UploadQueueFrame>
{
public:

	// Base class typedef
	typedef MDITabChildWindowImpl<UploadQueueFrame, RGB(0, 0, 0), IDR_UPLOAD_QUEUE> baseClass;
	typedef CSplitterImpl<UploadQueueFrame> splitBase;

	// Constructor/destructor
	UploadQueueFrame() : showTree(true), closed(false), 
		showTreeContainer("BUTTON", this, SHOWTREE_MESSAGE_MAP) {
		UploadManager::getInstance()->addListener(this);
	}
	virtual ~UploadQueueFrame() {
		UploadManager::getInstance()->removeListener(this);
	}

	// Frame window declaration
	DECLARE_FRAME_WND_CLASS_EX("UploadQueueFrame", IDR_UPLOAD_QUEUE, 0, COLOR_3DFACE);

	// Inline message map
	BEGIN_MSG_MAP(UploadQueueFrame)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		COMMAND_HANDLER(IDC_GETLIST, BN_CLICKED, onGetList)
		COMMAND_HANDLER(IDC_REMOVE, BN_CLICKED, onRemove)
		COMMAND_HANDLER(IDC_GRANTSLOT, BN_CLICKED, onGrantSlot)
		COMMAND_HANDLER(IDC_GRANTSLOT_HOUR, BN_CLICKED, onGrantSlotHour)
		COMMAND_HANDLER(IDC_GRANTSLOT_DAY, BN_CLICKED, onGrantSlotDay)
		COMMAND_HANDLER(IDC_GRANTSLOT_WEEK, BN_CLICKED, onGrantSlotWeek)
		COMMAND_HANDLER(IDC_UNGRANTSLOT, BN_CLICKED, onUnGrantSlot)
		COMMAND_HANDLER(IDC_ADD_TO_FAVORITES, BN_CLICKED, onAddToFavorites)
		COMMAND_HANDLER(IDC_PRIVATEMESSAGE, BN_CLICKED, onPrivateMessage)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_SELCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_UPLOAD_QUEUE_LIST, LVN_COLUMNCLICK, onColumnClickList)
		CHAIN_MSG_MAP(baseClass)
		CHAIN_MSG_MAP(splitBase)
	ALT_MSG_MAP(SHOWTREE_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onShowTree)
	ALT_MSG_MAP(UPLOADQUEUE_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_KEYDOWN, onChar)
	END_MSG_MAP()

	// Message handlers
	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onGetList(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onPrivateMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGrantSlotHour(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGrantSlotDay(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGrantSlotWeek(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onUnGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onShowTree(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		showTree = (wParam == BST_CHECKED);
		UpdateLayout(FALSE);
		return 0;
	}
	
	// Update colors
	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		HWND hWnd = (HWND)lParam;
		HDC hDC   = (HDC)wParam;
		if(hWnd == ctrlQueued.m_hWnd) 
		{
			::SetBkColor(hDC, WinUtil::bgColor);
			::SetTextColor(hDC, WinUtil::textColor);
			return (LRESULT)WinUtil::bgBrush;
		}
		bHandled = FALSE;
		return FALSE;
	};

	// Final message
	virtual void OnFinalMessage(HWND /*hWnd*/) 
	{
		delete this;
	}

	// Update control layouts
	void UpdateLayout(BOOL bResizeBars = TRUE);

	LRESULT onColumnClickList(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLISTVIEW* l = (NMLISTVIEW*)pnmh;
		if(l->iSubItem == ctrlList.getSortColumn()) {
			if (!ctrlList.isAscending())
				ctrlList.setSort(-1, ctrlList.getSortType());
			else
				ctrlList.setSortDirection(false);
		} else {
			if(l->iSubItem == 3) {
				ctrlList.setSort(l->iSubItem, ExListViewCtrl::SORT_INT);
			} else {
				ctrlList.setSort(l->iSubItem, ExListViewCtrl::SORT_STRING_NOCASE);
			}
		}
		return 0;
	}
	
private:
	enum {
		COLUMN_FIRST,
		COLUMN_FILE = COLUMN_FIRST,
		COLUMN_PATH,
		COLUMN_NICK,
		COLUMN_HUB,
		COLUMN_TRANSFERRED,
		COLUMN_SIZE,
		COLUMN_LAST
	};

	static int columnSizes[COLUMN_LAST];
	static int columnIndexes[COLUMN_LAST];
	static ResourceManager::Strings columnNames[COLUMN_LAST];
	CriticalSection cs;
	
	string getSelectedNick() {
		if(showTree) {
			HTREEITEM selectedItem = GetParentItem();
			if (!selectedItem) return Util::emptyString;
			char nickBuf[256];
			ctrlQueued.GetItemText(selectedItem, nickBuf, 255);
			return string(nickBuf);
		} else {
			int i = -1;
			while( (i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
				char nickBuf[256];
				ctrlList.GetItemText(i, COLUMN_NICK, nickBuf, 255);
				string nick = string(nickBuf);
				return nick;
			}
			return 0;
		}
	}

	// Communication with manager
	void LoadAll();
	void UpdateSearch(int index, BOOL doDelete = TRUE);

	HTREEITEM GetParentItem();

	// Contained controls
	CButton ctrlShowTree;
	CContainedWindow showTreeContainer;
	
	bool showTree;
	bool closed;
	
	ExListViewCtrl ctrlList;
	CTreeViewCtrl ctrlQueued;
	
	CStatusBarCtrl ctrlStatus;
	int statusSizes[4];
	
	OMenu grantMenu;
	OMenu contextMenu;
	
	void onRemoveUser(const string&);
	void onAddFile(const string&, const string&, const string&, const int, const int);
	void onAddFile(const string& /*aNick*/, const string &/*aFile*/);

	void addAllFiles(Upload * /*aUser*/);
	void updateStatus();

	// UploadManagerListener
	virtual void on(UploadManagerListener::QueueAdd, const string& aNick, const string& filename, const string& path, const int64_t pos, const int64_t size) throw() {
		onAddFile(aNick, filename, path, pos, size);
	}
	virtual void on(UploadManagerListener::QueueRemove, const string& aNick) throw() {
		onRemoveUser(aNick);
	}
};

#endif
