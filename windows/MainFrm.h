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

#if !defined(AFX_MAINFRM_H__E73C3806_489F_4918_B986_23DCFBD603D5__INCLUDED_)
#define AFX_MAINFRM_H__E73C3806_489F_4918_B986_23DCFBD603D5__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "../client/TimerManager.h"
#include "../client/HttpConnection.h"
#include "../client/HubManager.h"
#include "../client/QueueManagerListener.h"
#include "../client/Util.h"
#include "../client/Client.h"
#include "../client/ShareManager.h"
#include "../client/LogManager.h"
#include "../client/DownloadManager.h"
#include "../client/SettingsManager.h"

#include "atlctrlxp.h"
#include "atlctrlxp2.h"

#include "FlatTabCtrl.h"
#include "SingleInstance.h"
#include "CZDCLib.h"
#include "TransferView.h"
#include "WinUtil.h"

#define SERVER_SOCKET_MESSAGE (WM_APP + 1235)

class MainFrame : public CMDIFrameWindowImpl<MainFrame>, public CUpdateUI<MainFrame>,
		public CMessageFilter, public CIdleHandler, public CSplitterImpl<MainFrame, false>,
		private TimerManagerListener, private HttpConnectionListener, private QueueManagerListener,
		private LogManagerListener
{
public:
	MainFrame() : trayMessage(0), maximized(false), lastUpload(-1), lastUpdate(0), 
		lastUp(0), lastDown(0), oldshutdown(false), stopperThread(NULL), c(new HttpConnection()), 
		closing(false) 
	{ 
		memset(statusSizes, 0, sizeof(statusSizes));
		anyMF = this;
	};
	virtual ~MainFrame();
	DECLARE_FRAME_WND_CLASS("StrongDC++", IDR_MAINFRAME)

	CMDICommandBarXPCtrl m_CmdBar;

	enum {
		DOWNLOAD_LISTING,
		STATS,
		AUTO_CONNECT,
		PARSE_COMMAND_LINE,
		VIEW_FILE_AND_DELETE, 
		SET_STATUSTEXT,
		STATUS_MESSAGE,
		CHECK_LISTING
	};

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		if (!IsWindow())
			return FALSE;

		if(CMDIFrameWindowImpl<MainFrame>::PreTranslateMessage(pMsg))
			return TRUE;
		
		HWND hWnd = MDIGetActive();
		if(hWnd != NULL)
			return (BOOL)::SendMessage(hWnd, WM_FORWARDMSG, 0, (LPARAM)pMsg);
		
		return FALSE;
	}
	
	virtual BOOL OnIdle()
	{
		UIUpdateToolBar();
		return FALSE;
	}
	typedef CSplitterImpl<MainFrame, false> splitterBase;
	BEGIN_MSG_MAP(MainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(FTM_SELECTED, onSelected)
		MESSAGE_HANDLER(FTM_ROWS_CHANGED, onRowsChanged)
		MESSAGE_HANDLER(WM_APP+242, onTrayIcon)
		MESSAGE_HANDLER(WM_DESTROY, onDestroy)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		MESSAGE_HANDLER(WM_ENDSESSION, onEndSession)
		MESSAGE_HANDLER(trayMessage, onTray)
		MESSAGE_HANDLER(WM_COPYDATA, onCopyData)
		MESSAGE_HANDLER(WMU_WHERE_ARE_YOU, onWhereAreYou)
		MESSAGE_HANDLER(SERVER_SOCKET_MESSAGE, onServerSocket)
		MESSAGE_HANDLER(WM_ACTIVATEAPP, onActivateApp)
		MESSAGE_HANDLER(WM_APPCOMMAND, onAppCommand)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_FILE_CONNECT, OnFileConnect)
		COMMAND_ID_HANDLER(ID_FILE_SETTINGS, OnFileSettings)
		COMMAND_ID_HANDLER(ID_FILE_SEARCH, OnFileSearch)
		COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_VIEW_TRANSFERS, OnViewTransfers)
		COMMAND_ID_HANDLER(ID_GET_TTH, onGetTTH)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(ID_WINDOW_CASCADE, OnWindowCascade)
		COMMAND_ID_HANDLER(ID_WINDOW_TILE_HORZ, OnWindowTile)
		COMMAND_ID_HANDLER(ID_WINDOW_TILE_VERT, OnWindowTileVert)
		COMMAND_ID_HANDLER(ID_WINDOW_ARRANGE, OnWindowArrangeIcons)
// iDC++
		COMMAND_ID_HANDLER(IDC_RECENTS, onRecents)
// iDC++
		COMMAND_ID_HANDLER(IDC_FAVORITES, onFavorites)
		COMMAND_ID_HANDLER(IDC_FAVUSERS, onFavoriteUsers)
		COMMAND_ID_HANDLER(IDC_NOTEPAD, onNotepad)
		COMMAND_ID_HANDLER(IDC_AWAY, onAway)
		COMMAND_ID_HANDLER(IDC_LIMITER, onLimiter)
		COMMAND_ID_HANDLER(IDC_QUEUE, onQueue)
		COMMAND_ID_HANDLER(IDC_SEARCH_SPY, onSearchSpy)
		COMMAND_ID_HANDLER(IDC_FILE_ADL_SEARCH, onFileADLSearch)
		COMMAND_ID_HANDLER(IDC_NET_STATS, onNetStats)
		// CDM EXTENSION BEGINS
		COMMAND_ID_HANDLER(IDC_CDMDEBUG_WINDOW, onCDMDebugWindow)
		// CDM EXTENSION ENDS
		COMMAND_ID_HANDLER(IDC_HELP_HOMEPAGE, onLink)
		COMMAND_ID_HANDLER(IDC_HELP_DISCUSS, onLink)
		COMMAND_ID_HANDLER(IDC_IMPORT_QUEUE, onImport)
		COMMAND_ID_HANDLER(IDC_OPEN_FILE_LIST, onOpenFileList)
		COMMAND_ID_HANDLER(IDC_TRAY_SHOW, onAppShow)
		COMMAND_ID_HANDLER(ID_WINDOW_MINIMIZE_ALL, onWindowMinimizeAll)
		COMMAND_ID_HANDLER(IDC_FINISHED, onFinished)
		COMMAND_ID_HANDLER(IDC_FINISHEDMP3, onFinishedMP3)
		COMMAND_ID_HANDLER(IDC_FINISHED_UL, onFinishedUploads)
		COMMAND_ID_HANDLER(IDC_UPLOAD_QUEUE, onUploadQueue);
		COMMAND_ID_HANDLER(IDC_SHUTDOWN, onShutDown)
		COMMAND_ID_HANDLER(IDC_UPDATE, onUpdate)
		COMMAND_ID_HANDLER(IDC_CLOSE_DISCONNECTED, onCloseDisconnected)
		COMMAND_ID_HANDLER(IDC_OPEN_DOWNLOADS, onOpenDownloads)
		COMMAND_ID_HANDLER(IDC_REFRESH_FILE_LIST, onRefreshFileList)
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFO, onGetToolTip)
		CHAIN_MDI_CHILD_COMMANDS()
		CHAIN_MSG_MAP(CUpdateUI<MainFrame>)
		CHAIN_MSG_MAP(CMDIFrameWindowImpl<MainFrame>)
		CHAIN_MSG_MAP(splitterBase);
	END_MSG_MAP()

	BEGIN_UPDATE_UI_MAP(MainFrame)
		UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_TRANSFERS, UPDUI_MENUPOPUP)
	END_UPDATE_UI_MAP()

	LRESULT onSize(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onNotepad(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onFavoriteUsers(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onEndSession(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnFileConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchSpy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onNetStats(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onFileADLSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onGetTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onLink(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onImport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenFileList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onTrayIcon(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGetToolTip(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onFinished(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onFinishedMP3(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onUploadQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onFinishedUploads(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyData(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onShutDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		setShutDown(!getShutDown());
		return S_OK;
	}
	LRESULT onCloseDisconnected(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onServerSocket(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onRefreshFileList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);	
	LRESULT onActivateApp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onAppCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onAway(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onLimiter(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRecents(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCDMDebugWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	static DWORD WINAPI stopper(void* p);
	void UpdateLayout(BOOL bResizeBars = TRUE);
	void parseCommandLine(const string& cmdLine);

	LRESULT onWhereAreYou(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return WMU_WHERE_ARE_YOU;
	}

	LRESULT onTray(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { 
		WinUtil::trayIcon = false;
		updateTray(true); 
		return 0;
	};

	LRESULT onRowsChanged(UINT /*uMsg*/, WPARAM /* wParam */, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		UpdateLayout();
		Invalidate();
		return 0;
	}

	LRESULT onSelected(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		HWND hWnd = (HWND)wParam;
		if(::IsIconic(hWnd))
			::ShowWindow(hWnd, SW_RESTORE);
		MDIActivate(hWnd);
		return 0;
	}
	
	LRESULT onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if(WinUtil::trayIcon) {
			updateTray(false);
		}
		bHandled = FALSE;
		return 0;
	}
	
	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return 0;
	}

	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}
	
	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
		::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
		UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
		UpdateLayout();
		return 0;
	}

	LRESULT OnViewTransfers(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		showTransfers = !showTransfers;
		UISetCheck(ID_VIEW_TRANSFERS, showTransfers);
		UpdateLayout();
		return 0;
	}

	LRESULT onOpenDownloads(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		WinUtil::openFile(SETTING(DOWNLOAD_DIRECTORY));
		return 0;
	}

	LRESULT OnWindowCascade(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		MDICascade();
		return 0;
	}

	LRESULT OnWindowTile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		MDITile();
		return 0;
	}

	LRESULT OnWindowTileVert(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		MDITile(MDITILE_VERTICAL);
		return 0;
	}

	LRESULT OnWindowArrangeIcons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		MDIIconArrange();
		return 0;
	}

	LRESULT onWindowMinimizeAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		HWND tmpWnd = GetWindow(GW_CHILD); //getting client window
		tmpWnd = ::GetWindow(tmpWnd, GW_CHILD); //getting first child window
		while (tmpWnd!=NULL) {
			::CloseWindow(tmpWnd);
			tmpWnd = ::GetWindow(tmpWnd, GW_HWNDNEXT);
		}
		return 0;
	}
	static void setShutDown(bool b) {
		if (b)
			iCurrentShutdownTime = (unsigned long) (TimerManager::getTick() / 1000);
		bShutdown = b;
		if (anyMF != NULL) {
			if (b) { if(!anyMF->ctrlToolbar.IsButtonChecked(IDC_SHUTDOWN)) { anyMF->ctrlToolbar.CheckButton(IDC_SHUTDOWN,true); } }
			else { if(anyMF->ctrlToolbar.IsButtonChecked(IDC_SHUTDOWN)) {  anyMF->ctrlToolbar.CheckButton(IDC_SHUTDOWN,false); } }
			anyMF->UpdateLayout();
		}
	}
	static bool getShutDown() {
		return bShutdown;
	}
	static MainFrame* getMainFrame() {
		return anyMF;
	}
	
	static void setAwayButton(bool a) {
		if(a) {
			if(!anyMF->ctrlToolbar.IsButtonChecked(IDC_AWAY)) anyMF->ctrlToolbar.CheckButton(IDC_AWAY,true);
		}
		else
		{
			if(anyMF->ctrlToolbar.IsButtonChecked(IDC_AWAY)) anyMF->ctrlToolbar.CheckButton(IDC_AWAY,false);
		}
	}

	void SendCheatMessage(Client* client, User::Ptr u);
	CImageList largeImages, largeImagesHot;

private:
	friend bool isMDIChildActive(HWND hWnd);
	friend void handleMDIClick(int nID, HWND mdiWindow);
	friend int CZDCLib::setButtonPressed(int iPos, bool bPressed);
	
	static MainFrame* anyMF;
	NOTIFYICONDATA normalicon;
	
	class DirectoryListInfo {
	public:
		DirectoryListInfo(LPARAM lp = NULL) : lParam(lp) { };
		User::Ptr user;
		string file;
		string start;
		LPARAM lParam;
	};
	
	TransferView transferView;

	enum { MAX_CLIENT_LINES = 10 };
	StringList lastLinesList;
	string lastLines;
	CToolTipCtrl ctrlLastLines;

	CStatusBarCtrl ctrlStatus;
	FlatTabCtrl ctrlTab;
	HttpConnection* c;
	string versionInfo;
	CImageList images;
	CToolBarCtrl ctrlToolbar;

	bool showTransfers;
	bool bVisible;
	bool tbarcreated;
	bool awaybyminimize;
	
	static bool bShutdown;
	static u_int32_t iCurrentShutdownTime;
	static bool bIsShuttingDown;
	HICON hShutdownIcon;

	CMenu trayMenu;

	UINT trayMessage;
	/** Was the window maximized when minimizing it? */
	bool maximized;
	u_int32_t lastMove;
	u_int32_t lastUpdate;
	int64_t lastUp;
	int64_t lastDown;

	bool oldshutdown;

	bool closing;

	int lastUpload;

	int statusSizes[9];
	
	HANDLE stopperThread;

	HWND createToolbar();
	void buildMenu();
	void updateTray(bool add = true);
	void setNormalTrayIcon();

	LRESULT onAppShow(WORD /*wNotifyCode*/,WORD /*wParam*/, HWND, BOOL& /*bHandled*/) {
		if (::IsIconic(m_hWnd)) {
			ShowWindow(SW_SHOW);
			ShowWindow(maximized ? SW_MAXIMIZE : SW_RESTORE);
		}
		return 0;
	}

	void autoConnect(const FavoriteHubEntry::List& fl);
	void startSocket();

	MainFrame(const MainFrame&) { dcassert(0); };

	// LogManagerListener
	virtual void on(LogManagerListener::Message, const string& m) throw() { PostMessage(WM_SPEAKER, STATUS_MESSAGE, (LPARAM)new string(m)); };

	// TimerManagerListener
	virtual void on(TimerManagerListener::Second type, u_int32_t aTick) throw();
	virtual void on(TimerManagerListener::Minute type, u_int32_t aTick) throw();
	
	void updateShutdown(u_int32_t aTick);

	// HttpConnectionListener
	virtual void on(HttpConnectionListener::Complete, HttpConnection* conn, string const& /*aLine*/) throw();
	virtual void on(HttpConnectionListener::Data, HttpConnection* /*conn*/, const u_int8_t* buf, size_t len) throw();	

	// QueueManagerListener
	virtual void on(QueueManagerListener::Finished, QueueItem* qi) throw();
	void checkFileList(string file, User::Ptr u);
};

#endif // !defined(AFX_MAINFRM_H__E73C3806_489F_4918_B986_23DCFBD603D5__INCLUDED_)

/**
 * @file
 * $Id$
 */
