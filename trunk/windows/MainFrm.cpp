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

#include "MainFrm.h"
#include "AboutDlg.h"
#include "HubFrame.h"
#include "SearchFrm.h"
#include "PublicHubsFrm.h"
#include "PropertiesDlg.h"
#include "UsersFrame.h"
#include "DirectoryListingFrm.h"
#include "RecentsFrm.h"
#include "FavoritesFrm.h"
#include "NotepadFrame.h"
#include "QueueFrame.h"
#include "SpyFrame.h"
#include "FinishedFrame.h"
#include "FinishedMP3Frame.h"
#include "ADLSearchFrame.h"
#include "FinishedULFrame.h"
#include "TextFrame.h"
#include "UpdateDlg.h"
#include "StatsFrame.h"
#include "UploadQueueFrame.h"
#include "WinUtil.h"
#include "CDMDebugFrame.h"
#include "InputBox.h"

#ifndef AGEMOTIONSETUP_H__
	#include "AGEmotionSetup.h"
#endif

#include "../client/ConnectionManager.h"
#include "../client/DownloadManager.h"
#include "../client/HashManager.h"
#include "../client/UploadManager.h"
#include "../client/StringTokenizer.h"
#include "../client/SimpleXML.h"
#include "../client/ShareManager.h"

MainFrame* MainFrame::anyMF = NULL;
bool MainFrame::bShutdown = false;
u_int32_t MainFrame::iCurrentShutdownTime = 0;
bool MainFrame::isShutdownStatus = false;
CAGEmotionSetup* g_pEmotionsSetup;

MainFrame::MainFrame() : trayMessage(0), maximized(false), lastUpload(-1), lastUpdate(0), 
	lastUp(0), lastDown(0), oldshutdown(false), stopperThread(NULL), c(new HttpConnection()), 
	closing(false), awaybyminimize(false), missedAutoConnect(false), lastTTHdir(Util::emptyString)
	{ 
		memset(statusSizes, 0, sizeof(statusSizes));
		anyMF = this;
	};

MainFrame::~MainFrame() {
	m_CmdBar.m_hImageList = NULL;

	delete g_pEmotionsSetup;
	g_pEmotionsSetup = NULL;

	images.Destroy();
	largeImages.Destroy();
	largeImagesHot.Destroy();

	WinUtil::uninit();
}

DWORD WINAPI MainFrame::stopper(void* p) {
	MainFrame* mf = (MainFrame*)p;
	HWND wnd, wnd2 = NULL;

	while( (wnd=::GetWindow(mf->m_hWndMDIClient, GW_CHILD)) != NULL) {
		if(wnd == wnd2) 
			Sleep(100);
		else { 
			::PostMessage(wnd, WM_CLOSE, 0, 0);
			wnd2 = wnd;
		}
	}

	shutdown();
	
	mf->PostMessage(WM_CLOSE);	
	return 0;
}

LRESULT MainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	g_pEmotionsSetup = new CAGEmotionSetup;
	if ((g_pEmotionsSetup == NULL)||
		(!g_pEmotionsSetup->Create())){
		dcassert(FALSE);
		return -1;
	}

	TimerManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);
	LogManager::getInstance()->addListener(this);

	WinUtil::init(m_hWnd);

	// Register server socket message
	WSAAsyncSelect(ConnectionManager::getInstance()->getServerSocket().getSocket(),
		m_hWnd, SERVER_SOCKET_MESSAGE, FD_ACCEPT);

	trayMessage = RegisterWindowMessage("TaskbarCreated");

	TimerManager::getInstance()->start();

	// Set window name
	SetWindowText(APPNAME " " VERSIONSTRING "" CZDCVERSIONSTRING);

	// Load images
	// create command bar window
	HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);

	m_hMenu = WinUtil::mainMenu;

	hShutdownIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SHUTDOWN), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

	// attach menu
	m_CmdBar.AttachMenu(m_hMenu);
	// load command bar images
	images.CreateFromImage(IDB_TOOLBAR, 16, 16, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	m_CmdBar.m_hImageList = images;

	m_CmdBar.m_arrCommand.Add(ID_FILE_CONNECT);
	m_CmdBar.m_arrCommand.Add(ID_FILE_RECONNECT);
	m_CmdBar.m_arrCommand.Add(IDC_FOLLOW);
	m_CmdBar.m_arrCommand.Add(IDC_RECENTS);
	m_CmdBar.m_arrCommand.Add(IDC_FAVORITES);
	m_CmdBar.m_arrCommand.Add(IDC_FAVUSERS);
	m_CmdBar.m_arrCommand.Add(IDC_QUEUE);
	m_CmdBar.m_arrCommand.Add(IDC_FINISHED);
	m_CmdBar.m_arrCommand.Add(IDC_UPLOAD_QUEUE);
	m_CmdBar.m_arrCommand.Add(IDC_FINISHED_UL);
	m_CmdBar.m_arrCommand.Add(ID_FILE_SEARCH);
	m_CmdBar.m_arrCommand.Add(IDC_FILE_ADL_SEARCH);
	m_CmdBar.m_arrCommand.Add(IDC_SEARCH_SPY);
	m_CmdBar.m_arrCommand.Add(IDC_OPEN_FILE_LIST);
	m_CmdBar.m_arrCommand.Add(ID_FILE_SETTINGS);
	m_CmdBar.m_arrCommand.Add(IDC_NOTEPAD);
	m_CmdBar.m_arrCommand.Add(IDC_NET_STATS);
	m_CmdBar.m_arrCommand.Add(IDC_CDMDEBUG_WINDOW);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_CASCADE);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_TILE_HORZ);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_TILE_VERT);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_MINIMIZE_ALL);	
	m_CmdBar.m_arrCommand.Add(IDC_FINISHEDMP3);	
	m_CmdBar.m_arrCommand.Add(ID_GET_TTH);	
	m_CmdBar.m_arrCommand.Add(IDC_UPDATE);	

	// remove old menu
	SetMenu(NULL);

	tbarcreated = false;
	HWND hWndToolBar = createToolbar();

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndCmdBar);
	AddSimpleReBarBand(hWndToolBar, NULL, TRUE);
	CreateSimpleStatusBar();

	//Crea XP like Menu
	m_CmdBar.AddToolbar(hWndToolBar);
    m_CmdBar.Prepare();

	ctrlStatus.Attach(m_hWndStatusBar);
	ctrlStatus.SetSimple(FALSE);
	int w[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	ctrlStatus.SetParts(10, w);
	statusSizes[0] = WinUtil::getTextWidth(STRING(AWAY), ::GetDC(ctrlStatus.m_hWnd)); // for "AWAY" segment

	CToolInfo ti(TTF_SUBCLASS, ctrlStatus.m_hWnd);

	ctrlLastLines.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, WS_EX_TOPMOST);
	ctrlLastLines.AddTool(&ti);

	CreateMDIClient();
	m_CmdBar.SetMDIClient(m_hWndMDIClient);
	WinUtil::mdiClient = m_hWndMDIClient;

	ctrlTab.Create(m_hWnd, rcDefault);
	WinUtil::tabCtrl = &ctrlTab;

	transferView.Create(m_hWnd);

	SetSplitterPanes(m_hWndMDIClient, transferView.m_hWnd);
	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
	m_nProportionalPos = SETTING(TRANSFER_SPLIT_SIZE);
	UIAddToolBar(hWndToolBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);
	UISetCheck(ID_VIEW_TRANSFER_VIEW, 1);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	trayMenu.CreatePopupMenu();
	trayMenu.AppendMenu(MF_STRING, IDC_TRAY_SHOW, CSTRING(MENU_SHOW));
	trayMenu.AppendMenu(MF_STRING, IDC_OPEN_DOWNLOADS, CSTRING(MENU_OPEN_DOWNLOADS_DIR));
	trayMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	trayMenu.AppendMenu(MF_STRING, IDC_REFRESH_FILE_LIST, CSTRING(MENU_REFRESH_FILE_LIST));
	trayMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	trayMenu.AppendMenu(MF_STRING, ID_APP_ABOUT, CSTRING(MENU_ABOUT));
	trayMenu.AppendMenu(MF_STRING, ID_APP_EXIT, CSTRING(MENU_EXIT));

	if(BOOLSETTING(GET_UPDATE_INFO)) {
		c->addListener(this);
		c->downloadFile("http://snail.pc.cz/StrongDC/version.xml");
	}

	SettingsManager::DDList d = SettingsManager::getInstance()->getDownloadDirs();
	for(SettingsManager::DDList::iterator i = d.begin(); i != d.end(); ++i) {
		WinUtil::addLastDir(i->dir);
	}

	if(BOOLSETTING(OPEN_PUBLIC))
		PostMessage(WM_COMMAND, ID_FILE_CONNECT);
	if(BOOLSETTING(OPEN_QUEUE))
		PostMessage(WM_COMMAND, IDC_QUEUE);
	if(BOOLSETTING(OPEN_FAVORITE_HUBS))
		PostMessage(WM_COMMAND, IDC_FAVORITES);
	if(BOOLSETTING(OPEN_FINISHED_DOWNLOADS))
		PostMessage(WM_COMMAND, IDC_FINISHED);
	if(BOOLSETTING(OPEN_NETWORK_STATISTIC))
		PostMessage(WM_COMMAND, IDC_NET_STATS);

	if(!BOOLSETTING(SHOW_STATUSBAR))
		PostMessage(WM_COMMAND, ID_VIEW_STATUS_BAR);
	if(!BOOLSETTING(SHOW_TOOLBAR))
		PostMessage(WM_COMMAND, ID_VIEW_TOOLBAR);
	if(!BOOLSETTING(SHOW_TRANSFERVIEW))
		PostMessage(WM_COMMAND, ID_VIEW_TRANSFER_VIEW);

	if(!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
		PostMessage(WM_SPEAKER, AUTO_CONNECT);

	PostMessage(WM_SPEAKER, PARSE_COMMAND_LINE);

	Util::ensureDirectory(SETTING(LOG_DIRECTORY));

	if (CZDCLib::isXp()) {
		normalicon.hIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MAINFRAME_XP), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	} else {
		normalicon.hIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	}
	updateTray( BOOLSETTING( MINIMIZE_TRAY ) );
	startSocket();

	if(BOOLSETTING(IPUPDATE)) SettingsManager::getInstance()->set(SettingsManager::SERVER, Util::getLocalIp());

	ctrlToolbar.CheckButton(IDC_LIMITER,BOOLSETTING(THROTTLE_ENABLE));
	ctrlToolbar.CheckButton(IDC_DISABLE_SOUNDS, BOOLSETTING(SOUNDS_DISABLED));

	if(SETTING(NICK).empty()) {
		PostMessage(WM_COMMAND, ID_FILE_SETTINGS);
	}

	// We want to pass this one on to the splitter...hope it get's there...
	bHandled = FALSE;
	return 0;
}

void MainFrame::startSocket() {
	SearchManager::getInstance()->disconnect();
	ConnectionManager::getInstance()->disconnect();

	if(SETTING(CONNECTION_TYPE) == SettingsManager::CONNECTION_ACTIVE) {

		short lastPort = (short)SETTING(IN_PORT);
		short firstPort = lastPort;

		while(true) {
			try {
				ConnectionManager::getInstance()->setPort(lastPort);
				WSAAsyncSelect(ConnectionManager::getInstance()->getServerSocket().getSocket(), m_hWnd, SERVER_SOCKET_MESSAGE, FD_ACCEPT);
				SearchManager::getInstance()->setPort(lastPort);
				break;
			} catch(const Exception& e) {
				dcdebug("MainFrame::OnCreate caught %s\n", e.getError().c_str());
				short newPort = (short)((lastPort == 32000) ? 1025 : lastPort + 1);
				SettingsManager::getInstance()->setDefault(SettingsManager::IN_PORT, newPort);
				if(SETTING(IN_PORT) == lastPort || (firstPort == newPort)) {
					// Changing default didn't change port, a fixed port must be in use...(or we
					// tried all ports
					char* buf = new char[STRING(PORT_IS_BUSY).size() + 8];
					sprintf(buf, CSTRING(PORT_IS_BUSY), SETTING(IN_PORT));
					MessageBox(buf, APPNAME " " VERSIONSTRING, MB_ICONSTOP | MB_OK);
					delete[] buf;
					break;
				}
				lastPort = newPort;
			}
		}
	}
}

HWND MainFrame::createToolbar() {
	if(!tbarcreated) {
	if(SETTING(TOOLBARIMAGE) == "")
		largeImages.CreateFromImage(IDB_TOOLBAR20, 20, 20, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);

	if(SETTING(TOOLBARHOTIMAGE) == "")
		largeImagesHot.CreateFromImage(IDB_TOOLBAR20_HOT, 20, 20, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);

	ctrlToolbar.Create(m_hWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS, 0, ATL_IDW_TOOLBAR);
	ctrlToolbar.SetImageList(largeImages);
	ctrlToolbar.SetHotImageList(largeImagesHot);
		tbarcreated = true;
	}

	while(ctrlToolbar.GetButtonCount()>0)
		ctrlToolbar.DeleteButton(0);

	ctrlToolbar.SetButtonStructSize();
	StringTokenizer t(SETTING(TOOLBAR), ',');
	StringList& l = t.getTokens();
	
	for(StringList::const_iterator k = l.begin(); k != l.end(); ++k) {
		int i = Util::toInt(*k);		
		
	TBBUTTON nTB;
		memset(&nTB, 0, sizeof(TBBUTTON));

		if(i == -1) {
			nTB.fsStyle = TBSTYLE_SEP;			
		} else {
			nTB.iBitmap = WinUtil::ToolbarButtons[i].image;
			nTB.idCommand = WinUtil::ToolbarButtons[i].id;
			nTB.fsState = TBSTATE_ENABLED;
			nTB.fsStyle = TBSTYLE_AUTOSIZE | ((WinUtil::ToolbarButtons[i].check == true)? TBSTYLE_CHECK : TBSTYLE_BUTTON);
		}
			ctrlToolbar.AddButtons(1, &nTB);
	}	

	ctrlToolbar.AutoSize();

	return ctrlToolbar.m_hWnd;
}



LRESULT MainFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
		
	if(wParam == DOWNLOAD_LISTING) {
		DirectoryListInfo* i = (DirectoryListInfo*)lParam;
		checkFileList(i->file, i->user);
		DirectoryListingFrame::openWindow(i->file, i->user, i->start);
		delete i;
	} else if(wParam == CHECK_LISTING) {
		DirectoryListInfo* i = (DirectoryListInfo*)lParam;
		checkFileList(i->file, i->user);
		delete i;
	} else if(wParam == VIEW_FILE_AND_DELETE) {
		string* file = (string*)lParam;
		TextFrame::openWindow(*file);
		File::deleteFile(*file);
		delete file;
	} else if(wParam == STATS) {
		StringList& str = *(StringList*)lParam;
		if(ctrlStatus.IsWindow()) {
			HDC dc = ::GetDC(ctrlStatus.m_hWnd);
			bool u = false;
			ctrlStatus.SetText(1, str[0].c_str());
			for(int i = 1; i < 8; i++) {
				int w = WinUtil::getTextWidth(str[i], dc);
				
			if(statusSizes[i] < w) {
					statusSizes[i] = w;
						  u = true;
						}
					ctrlStatus.SetText(i+1, str[i].c_str());
				}
			::ReleaseDC(ctrlStatus.m_hWnd, dc);
			if(u)
				UpdateLayout(TRUE);
		}
		delete &str;
	} else if(wParam == AUTO_CONNECT) {
		autoConnect(HubManager::getInstance()->getFavoriteHubs());
	} else if(wParam == PARSE_COMMAND_LINE) {
		parseCommandLine(GetCommandLine());
	} else if(wParam == STATUS_MESSAGE) {
		string* msg = (string*)lParam;
		if(ctrlStatus.IsWindow()) {
			string line = "[" + Util::getShortTimeString() + "] " + *msg;

			ctrlStatus.SetText(0, line.c_str());
			while(lastLinesList.size() + 1 > MAX_CLIENT_LINES)
				lastLinesList.erase(lastLinesList.begin());
			if (line.find('\r') == string::npos) {
				lastLinesList.push_back(line);
			} else {
				lastLinesList.push_back(line.substr(0, line.find('\r')));
			}
		}
		delete msg;
	}

	return 0;
}

void MainFrame::parseCommandLine(const string& cmdLine)
{
	string::size_type i = 0;
	string::size_type j;

	if( (j = safestring::SafeFind(cmdLine,"dchub://", i)) != string::npos) {
		i = j + 8;
		string server;
		string user;
		if( (j = safestring::SafeFind(cmdLine,'/', i)) == string::npos) {
			server = cmdLine.substr(i);
		} else {
			server = cmdLine.substr(i, j-i);
			i = j + 1;
			if( (j = cmdLine.find_first_of("\\/ ", i)) == string::npos) {
				user = cmdLine.substr(i);
			} else {
				user = cmdLine.substr(i, j-i);
			}
		}

		if(!server.empty()) {
			HubFrame::openWindow(server);
		}
		if(!user.empty()) {
			try {
				QueueManager::getInstance()->addList(ClientManager::getInstance()->getUser(user), QueueItem::FLAG_CLIENT_VIEW);
			} catch(const Exception&) {
				// ...
			}
		}
	} else if( (j = cmdLine.find("magnet:?", i)) != string::npos) {
		i = j + 8;
		string tth;
		if( (j = cmdLine.find("xt=urn:tree:tiger:", i)) != string::npos) {
			tth = cmdLine.substr(j + 18, 39);
			if(Encoder::isBase32(tth.c_str()))
				SearchFrame::openWindow(tth, 0, SearchManager::SIZE_DONTCARE, SearchManager::TYPE_HASH);
		} else if( (j = cmdLine.find("xt=urn:bitprint:", i)) != string::npos) {
			i = j + 16;
			if( (j = cmdLine.find(".", i)) != string::npos) {
				tth = cmdLine.substr(j + 1, 39);
				if(Encoder::isBase32(tth.c_str()))
					SearchFrame::openWindow(tth, 0, SearchManager::SIZE_DONTCARE, SearchManager::TYPE_HASH);
			}
		}
	}
}

LRESULT MainFrame::onCopyData(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	string cmdLine = (LPCSTR) (((COPYDATASTRUCT *)lParam)->lpData);
	parseCommandLine(Util::getAppName() + " " + cmdLine);
	return true;
}

LRESULT MainFrame::OnFileSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	SearchFrame::openWindow();
	return 0;
}	

LRESULT MainFrame::onRecents(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	RecentHubsFrame::openWindow();
	return 0;
}

LRESULT MainFrame::onFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	FavoriteHubsFrame::openWindow();
	return 0;
}

LRESULT MainFrame::onFavoriteUsers(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UsersFrame::openWindow();
	return 0;
}

LRESULT MainFrame::onNotepad(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	NotepadFrame::openWindow();
	return 0;
}

LRESULT MainFrame::onQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	QueueFrame::openWindow();
	return 0;
}

LRESULT MainFrame::OnFileConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PublicHubsFrame::openWindow();
	return 0;
}

LRESULT MainFrame::onSearchSpy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	SpyFrame::openWindow();
	return 0;
}
	
LRESULT MainFrame::onNetStats(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	StatsFrame::openWindow();
	return 0;
}

LRESULT MainFrame::onFileADLSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ADLSearchFrame::openWindow();
	return 0;
}

LRESULT MainFrame::onCDMDebugWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) 
{
	CDMDebugFrame::openWindow();
	return 0;
}

LRESULT MainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CAboutDlg dlg;
	dlg.DoModal(m_hWnd);
	return 0;
}

LRESULT MainFrame::OnFileSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	PropertiesDlg dlg(SettingsManager::getInstance());

	short lastPort = (short)SETTING(IN_PORT);
	int lastConn = SETTING(CONNECTION_TYPE);

	if(dlg.DoModal(m_hWnd) == IDOK)
	{		
		SettingsManager::getInstance()->save();
 		if(missedAutoConnect && !SETTING(NICK).empty()) {
 			PostMessage(WM_SPEAKER, AUTO_CONNECT);
 		}
		if(SETTING(CONNECTION_TYPE) != lastConn || SETTING(IN_PORT) != lastPort) {
			startSocket();
		}
		ClientManager::getInstance()->infoUpdated(false);
		if(BOOLSETTING(THROTTLE_ENABLE)) ctrlToolbar.CheckButton(IDC_LIMITER, true);
		else ctrlToolbar.CheckButton(IDC_LIMITER, false);

		if(Util::getAway()) ctrlToolbar.CheckButton(IDC_AWAY, true);
		else ctrlToolbar.CheckButton(IDC_AWAY, false);

		if(getShutDown()) ctrlToolbar.CheckButton(IDC_SHUTDOWN, true);
		else ctrlToolbar.CheckButton(IDC_SHUTDOWN, false);

	}
	return 0;
}

void MainFrame::on(HttpConnectionListener::Complete, HttpConnection* /*aConn*/, const string&) throw() {
	try {
		SimpleXML xml;
		xml.fromXML(versionInfo);
		xml.stepIn();

				string url;
				if(xml.findChild("URL")) {
					url = xml.getChildData();
				}

		xml.resetCurrentChild();
		if(xml.findChild("DCVersion")) {
			if(atof(xml.getChildData().c_str()) > DCVERSIONFLOAT) {
				xml.resetCurrentChild();
				xml.resetCurrentChild();
				if(xml.findChild("Title")) {
					const string& title = xml.getChildData();
					xml.resetCurrentChild();
				if(xml.findChild("Message")) {
						if(url.empty()) {
					const string& msg = xml.getChildData();
							MessageBox(msg.c_str(), title.c_str(), MB_OK);
						} else {
							string msg = xml.getChildData() + "\r\n" + STRING(OPEN_DOWNLOAD_PAGE);
							UpdateDlg dlg;
							dlg.DoModal();
						}
					}
				}
				xml.resetCurrentChild();
				if(xml.findChild("VeryOldVersion")) {
					if(atof(xml.getChildData().c_str()) >= VERSIONFLOAT) {
						string msg = xml.getChildAttrib("Message", "Your version of StrongDC++ contains a serious bug that affects all users of the DC network or the security of your computer.");
						MessageBox((msg + "\r\nPlease get a new one at " + url).c_str());
						oldshutdown = true;
						PostMessage(WM_CLOSE);
					}
				}
						xml.resetCurrentChild();
				if(xml.findChild("BadVersions")) {
					xml.stepIn();
					while(xml.findChild("BadVersion")) {
						double v = atof(xml.getChildAttrib("Version").c_str());
						if(v == VERSIONFLOAT) {
							string msg = xml.getChildAttrib("Message", "Your version of DC++ contains a serious bug that affects all users of the DC network or the security of your computer.");
							MessageBox((msg + "\r\nPlease get a new one at " + url).c_str());
							oldshutdown = true;
							PostMessage(WM_CLOSE);
						}
					}
				}
			}
		}
	} catch (const Exception&) {
		// ...
	}
}

LRESULT MainFrame::onServerSocket(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ConnectionManager::getInstance()->getServerSocket().incoming();
	return 0;
}

LRESULT MainFrame::onGetToolTip(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMTTDISPINFO pDispInfo = (LPNMTTDISPINFO)pnmh;
	pDispInfo->szText[0] = 0;

	if((idCtrl != 0) && !(pDispInfo->uFlags & TTF_IDISHWND))
	{
		int stringId = -1;

	for(int i = 0; WinUtil::ToolbarButtons[i].id != 0; i++) {
			if(WinUtil::ToolbarButtons[i].id == idCtrl) {
				stringId = WinUtil::ToolbarButtons[i].tooltip;
				break;
			}
		}
		if(stringId != -1) {
			strncpy(pDispInfo->lpszText, ResourceManager::getInstance()->getString((ResourceManager::Strings)stringId).c_str(), 79);
			pDispInfo->uFlags |= TTF_DI_SETITEM;
		}
	} else { // if we're really in the status bar, this should be detected intelligently
		lastLines.clear();
		for(StringIter i = lastLinesList.begin(); i != lastLinesList.end(); ++i) {
			lastLines += *i;
			lastLines += "\r\n";
		}
		if(lastLines.size() > 2) {
			lastLines.erase(lastLines.size() - 2);
		}
		pDispInfo->lpszText = const_cast<char*>(lastLines.c_str());
	}
	return 0;
}

void MainFrame::autoConnect(const FavoriteHubEntry::List& fl) {
 	missedAutoConnect = false;
	for(FavoriteHubEntry::List::const_iterator i = fl.begin(); i != fl.end(); ++i) {
		FavoriteHubEntry* entry = *i;
		if(entry->getConnect()) {
 			if(!entry->getNick().empty() || !SETTING(NICK).empty()) {
				RecentHubEntry r;
				r.setName(entry->getName());
				r.setDescription(entry->getDescription());
				r.setUsers("*");
				r.setShared("*");
				r.setServer(entry->getServer());
				HubManager::getInstance()->addRecent(r);
				HubFrame::openWindow(entry->getServer(), entry->getNick(), entry->getPassword(), entry->getUserDescription()
					, entry->getRawOne()
					, entry->getRawTwo()
					, entry->getRawThree()
					, entry->getRawFour()
					, entry->getRawFive()		
					, entry->getWindowPosX(), entry->getWindowPosY(), entry->getWindowSizeX(), entry->getWindowSizeY(), entry->getWindowType(), entry->getChatUserSplit(), entry->getStealth(), entry->getUserListState());
 			} else
 				missedAutoConnect = true;
 		}				
	}
}

void MainFrame::updateTray(bool add /* = true */) {
	if(add) {
		if ( !WinUtil::trayIcon ) {
			NOTIFYICONDATA nid;
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = m_hWnd;
			nid.uID = 0;
			nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
			nid.uCallbackMessage = WM_APP + 242;
			nid.hIcon = normalicon.hIcon;
			strncpy(nid.szTip, "StrongDC++",64);
			nid.szTip[63] = '\0';
			lastMove = GET_TICK() - 1000;
			::Shell_NotifyIcon(NIM_ADD, &nid);
			WinUtil::trayIcon = true;
		}
	} else {
		if ( WinUtil::trayIcon ) {
			NOTIFYICONDATA nid;
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = m_hWnd;
			nid.uID = 0;
			nid.uFlags = 0;
			::Shell_NotifyIcon(NIM_DELETE, &nid);
			ShowWindow(SW_SHOW);
			WinUtil::trayIcon = false;		
		}
	}
}

LRESULT MainFrame::onSize(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	if(wParam == SIZE_MINIMIZED) {
		SetProcessWorkingSetSize(GetCurrentProcess(), 0xffffffff, 0xffffffff);
		if(BOOLSETTING(AUTO_AWAY)) {
			if(!WinUtil::isMinimized)
			if(Util::getAway() == true) {
				awaybyminimize = false;
			} else {
				awaybyminimize = true;
				Util::setAway(true);
				setAwayButton(true);
				ClientManager::getInstance()->infoUpdated(true);
			}
		}

		WinUtil::isMinimized = true;
		if(BOOLSETTING(MINIMIZE_TRAY)) {
			updateTray(true);
			ShowWindow(SW_HIDE);
		} else {
			updateTray(false);
		}
		maximized = IsZoomed() > 0;
		//SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
	} else if( (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED) ) {
		if(BOOLSETTING(AUTO_AWAY)) {
			if(awaybyminimize == true) {
				awaybyminimize = false;
				Util::setAway(false);
				setAwayButton(false);
				ClientManager::getInstance()->infoUpdated(true);
			}
		}
		setNormalTrayIcon();
		WinUtil::isMinimized = false;
		//SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	}

	bHandled = FALSE;
	return 0;
}

LRESULT MainFrame::onEndSession(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if(c != NULL) {
		c->removeListener(this);
		delete c;
		c = NULL;
	}

	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	GetWindowPlacement(&wp);
	
	CRect rc;
	GetWindowRect(rc);
	
	if(wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWNORMAL) {
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_POS_X, rc.left);
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_POS_Y, rc.top);
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_SIZE_X, rc.Width());
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_SIZE_Y, rc.Height());
	}
	if(wp.showCmd == SW_SHOWNORMAL || wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_MAXIMIZE)
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_STATE, (int)wp.showCmd);
	
	QueueManager::getInstance()->saveQueue();
	SettingsManager::getInstance()->save();
	
	return 0;
}

LRESULT MainFrame::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(c != NULL) {
		c->removeListener(this);
		delete c;
		c = NULL;
	}

	if(!closing) {
		if( oldshutdown ||(!BOOLSETTING(CONFIRM_EXIT)) || (MessageBox(CSTRING(REALLY_EXIT), APPNAME " " VERSIONSTRING, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) ) {
			updateTray(false);
			string tmp1;
			string tmp2;

			WINDOWPLACEMENT wp;
			wp.length = sizeof(wp);
			GetWindowPlacement(&wp);

			CRect rc;
			GetWindowRect(rc);
			if(BOOLSETTING(SHOW_TRANSFERVIEW)) {
				SettingsManager::getInstance()->set(SettingsManager::TRANSFER_SPLIT_SIZE, m_nProportionalPos);
			}
			if(wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWNORMAL) {
				SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_POS_X, rc.left);
				SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_POS_Y, rc.top);
				SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_SIZE_X, rc.Width());
				SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_SIZE_Y, rc.Height());
			}
			if(wp.showCmd == SW_SHOWNORMAL || wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_MAXIMIZE)
				SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_STATE, (int)wp.showCmd);

			ShowWindow(SW_HIDE);
			transferView.prepareClose();
			
			SearchManager::getInstance()->disconnect();
			ConnectionManager::getInstance()->disconnect();

			DWORD id;
			stopperThread = CreateThread(NULL, 0, stopper, this, 0, &id);
			closing = true;
		}
		bHandled = TRUE;
	} else {
		// This should end immideately, as it only should be the stopper that sends another WM_CLOSE
		WaitForSingleObject(stopperThread, 60*1000);
		CloseHandle(stopperThread);
		stopperThread = NULL;
		DestroyIcon(normalicon.hIcon);
		DestroyIcon(hShutdownIcon); 	
		bHandled = FALSE;
	}

	return 0;
}

LRESULT MainFrame::onLink(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	string site;
	bool isFile = false;
	switch(wID) {
	case IDC_HELP_HOMEPAGE: site = "http://snail.pc.cz/StrongDC/index.htm"; break;
	case IDC_HELP_DISCUSS: site = "http://strongdc.berlios.de/index.php"; break;
	default: dcassert(0);
	}

	if(isFile)
		WinUtil::openFile(site);
	else
		WinUtil::openLink(site);

	return 0;
}

LRESULT MainFrame::onGetTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	string file = Util::getAppPath() + "*.*";
 	if(WinUtil::browseFile(file, m_hWnd, false, lastTTHdir) == IDOK) {
		lastTTHdir = Util::getFilePath(file);
		string hash = HashManager::getInstance()->hasher.getTTfromFile(file).getRoot().toBase32();
		CInputBox ibox(m_hWnd);
		WIN32_FIND_DATA data;
		FindFirstFile(file.c_str(), &data);
		int64_t size = (int64_t)data.nFileSizeLow | ((int64_t)data.nFileSizeHigh)<<32;
		string magnetlink = "magnet:?xt=urn:tree:tiger:"+hash+"&xl="+Util::toString(size)+"&dn="+Util::encodeURI(Util::getFileName(file));
		ibox.DoModal("Tiger Tree Hash", file.c_str(), hash.c_str(), magnetlink.c_str());
	   } 
   return 0;
}

void MainFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */)
{
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
	
	if(ctrlStatus.IsWindow() && ctrlLastLines.IsWindow()) {
		CRect sr;
		int w[10];
		ctrlStatus.GetClientRect(sr);
		w[9] = sr.right - 16;
		w[8] = w[9] - 60;
#define setw(x) w[x] = max(w[x+1] - statusSizes[x], 0)
		setw(7); setw(6); setw(5); setw(4); setw(3); setw(2); setw(1); setw(0);

		ctrlStatus.SetParts(10, w);
		ctrlLastLines.SetMaxTipWidth(w[0]);
		ctrlLastLines.SetWindowPos(HWND_TOPMOST, sr.left, sr.top, sr.Width(), sr.Height(), SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}
	CRect rc = rect;
	rc.top = rc.bottom - ctrlTab.getHeight();
	if(ctrlTab.IsWindow())
		ctrlTab.MoveWindow(rc);
	
	CRect rc2 = rect;
	rc2.bottom = rc.top;
	SetSplitterRect(rc2);
}

static const char types[] = "File Lists\0*.DcLst;*.xml.bz2\0All Files\0*.*\0";

LRESULT MainFrame::onOpenFileList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	string file;
	if(WinUtil::browseFile(file, m_hWnd, false, Util::getAppPath() + "FileLists\\", types)) {
		string username;
		if(file.rfind('\\') != string::npos) {
			username = file.substr(file.rfind('\\') + 1);
			if(username.rfind('.') != string::npos) {
				username.erase(username.rfind('.'));
			}
			if(username.length() > 4 && Util::stricmp(username.c_str() + username.length() - 4, ".xml") == 0)
				username.erase(username.length()-4);
			DirectoryListingFrame::openWindow(file, ClientManager::getInstance()->getUser(username));
		}
	}
	return 0;
}

LRESULT MainFrame::onRefreshFileList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ShareManager::getInstance()->setDirty();
	ShareManager::getInstance()->refresh(true);
	return 0;
}

LRESULT MainFrame::onTrayIcon(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	if (lParam == WM_LBUTTONUP) {
		if (WinUtil::isMinimized) {
			ShowWindow(SW_SHOW);
			ShowWindow(maximized ? SW_MAXIMIZE : SW_RESTORE);
		} else {
			SetForegroundWindow(m_hWnd);
		}
	} else if(lParam == WM_MOUSEMOVE && ((lastMove + 1000) < GET_TICK()) ) {
		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = m_hWnd;
		nid.uID = 0;
		nid.uFlags = NIF_TIP;
		strncpy(nid.szTip, ("D: " + Util::formatBytes(DownloadManager::getInstance()->getAverageSpeed()) + "/s (" + 
			Util::toString(DownloadManager::getInstance()->getDownloads()) + ")\r\nU: " +
			Util::formatBytes(UploadManager::getInstance()->getAverageSpeed()) + "/s (" + 
			Util::toString(UploadManager::getInstance()->getUploads()) + ")"
			+ "\r\nUptime: " + Util::formatSeconds(Util::getUptime())
			).c_str(), 64);
		
		::Shell_NotifyIcon(NIM_MODIFY, &nid);
		lastMove = GET_TICK();
	} else if (lParam == WM_RBUTTONUP) {
		CPoint pt(GetMessagePos());		
		SetForegroundWindow(m_hWnd);
		trayMenu.TrackPopupMenu(TPM_RIGHTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);		
		PostMessage(WM_NULL, 0, 0);
	}
	return 0;
}

LRESULT MainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_VIEW_TOOLBAR, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_TOOLBAR, bVisible);
	return 0;
}

LRESULT MainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_STATUSBAR, bVisible);
	return 0;
}

LRESULT MainFrame::OnViewTransferView(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !transferView.IsWindowVisible();
	if(!bVisible) {	
		if(GetSinglePaneMode() == SPLIT_PANE_NONE)
			SetSinglePaneMode(SPLIT_PANE_TOP);
	} else { 
		if(GetSinglePaneMode() != SPLIT_PANE_NONE)
			SetSinglePaneMode(SPLIT_PANE_NONE);
	}
	UISetCheck(ID_VIEW_TRANSFER_VIEW, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_TRANSFERVIEW, bVisible);
	return 0;
}

LRESULT MainFrame::onFinished(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	FinishedFrame::openWindow();
	return 0;
}

LRESULT MainFrame::onFinishedMP3(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	FinishedMP3Frame::openWindow();
	return 0;
}

LRESULT MainFrame::onLimiter(WORD , WORD , HWND, BOOL& ) {
	if(BOOLSETTING(THROTTLE_ENABLE)) SettingsManager::getInstance()->set(SettingsManager::THROTTLE_ENABLE,false);
	else SettingsManager::getInstance()->set(SettingsManager::THROTTLE_ENABLE,true);
 
	ClientManager::getInstance()->infoUpdated(true);
	return 0;
}

LRESULT MainFrame::onUploadQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UploadQueueFrame::openWindow();
	return 0;
}

LRESULT MainFrame::onFinishedUploads(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
 	FinishedULFrame::openWindow();
	return 0;
}

LRESULT MainFrame::onCloseDisconnected(WORD , WORD , HWND , BOOL& ) {
	HubFrame::closeDisconnected();
	return 0;
}

void MainFrame::on(TimerManagerListener::Second, u_int32_t aTick) throw() {
		Util::increaseUptime();
		int64_t diff = (int64_t)((lastUpdate == 0) ? aTick - 1000 : aTick - lastUpdate);
		int64_t updiff = Socket::getTotalUp() - lastUp;
		int64_t downdiff = Socket::getTotalDown() - lastDown;

		// Limitery sem a tam, vsude kam se podivam :o)
		if( SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL) > 0) {
			if( SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL) < ((2 * UploadManager::getInstance()->getSlots()) + 3) ) {
				SettingsManager::getInstance()->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT_NORMAL, ((2 * UploadManager::getInstance()->getSlots()) + 3) );
			}
			if ( (SETTING(MAX_DOWNLOAD_SPEED_LIMIT_NORMAL) > ( SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL) * 7)) || ( SETTING(MAX_DOWNLOAD_SPEED_LIMIT_NORMAL) == 0) ) {
				SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT_NORMAL, (SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL)*7) );
			}
		}

		if( SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME) > 0) {
			if( SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME) < ((2 * UploadManager::getInstance()->getSlots()) + 3) ) {
				SettingsManager::getInstance()->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT_TIME, ((2 * UploadManager::getInstance()->getSlots()) + 3) );
			}
			if ( (SETTING(MAX_DOWNLOAD_SPEED_LIMIT_TIME) > ( SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME) * 7)) || ( SETTING(MAX_DOWNLOAD_SPEED_LIMIT_TIME) == 0) ) {
				SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT_TIME, (SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME)*7) );
			}
		}

		time_t currentTime;
		time(&currentTime);
		int currentHour = localtime(&currentTime)->tm_hour;

		if(BOOLSETTING(THROTTLE_ENABLE)) {
			if (SETTING(TIME_DEPENDENT_THROTTLE) &&
				((SETTING(BANDWIDTH_LIMIT_START) < SETTING(BANDWIDTH_LIMIT_END) &&
					currentHour >= SETTING(BANDWIDTH_LIMIT_START) && currentHour < SETTING(BANDWIDTH_LIMIT_END)) ||
				(SETTING(BANDWIDTH_LIMIT_START) > SETTING(BANDWIDTH_LIMIT_END) &&
					(currentHour >= SETTING(BANDWIDTH_LIMIT_START) || currentHour < SETTING(BANDWIDTH_LIMIT_END))))
			) {
				//want to keep this out of the upload limiting code proper, where it might otherwise work more naturally
				SettingsManager::getInstance()->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT, SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME));
				SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT, SETTING(MAX_DOWNLOAD_SPEED_LIMIT_TIME));
			} else {
				SettingsManager::getInstance()->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT, SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL));
				SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT, SETTING(MAX_DOWNLOAD_SPEED_LIMIT_NORMAL));
			}
		} else {
			SettingsManager::getInstance()->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT, 0);
			SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT, 0);
		}		

		StringList* str = new StringList();
		str->push_back(Util::getAway() ? STRING(AWAY) : "");
		str->push_back(STRING(SHARED) + ": " + Util::formatBytes(ShareManager::getInstance()->getShareSizeString()));
		str->push_back("H: " + Client::getCounts());
		str->push_back(STRING(SLOTS) + ": " + Util::toString(UploadManager::getInstance()->getFreeSlots()) + '/' + Util::toString(UploadManager::getInstance()->getSlots()) + " (" + Util::toString(UploadManager::getInstance()->getFreeExtraSlots()) + '/' + Util::toString(SETTING(EXTRA_SLOTS)) + ")" );
		str->push_back("D: " + Util::formatBytes(Socket::getTotalDown()));
		str->push_back("U: " + Util::formatBytes(Socket::getTotalUp()));
		str->push_back("D: [" + Util::toString(DownloadManager::getInstance()->getDownloads()) + "][" + (SETTING(MAX_DOWNLOAD_SPEED_LIMIT) == 0 ? string("N") : Util::toString((int)SETTING(MAX_DOWNLOAD_SPEED_LIMIT)) + "k") + "] " + Util::formatBytes(downdiff*1000I64/diff) + "/s" );
		str->push_back("U: [" + Util::toString(UploadManager::getInstance()->getUploads()) + "][" + (SETTING(MAX_UPLOAD_SPEED_LIMIT) == 0 ? string("N") : Util::toString((int)SETTING(MAX_UPLOAD_SPEED_LIMIT)) + "k") + "] " + Util::formatBytes(updiff*1000I64/diff) + "/s" );
		PostMessage(WM_SPEAKER, STATS, (LPARAM)str);
		SettingsManager::getInstance()->set(SettingsManager::TOTAL_UPLOAD, SETTING(TOTAL_UPLOAD) + updiff);
		SettingsManager::getInstance()->set(SettingsManager::TOTAL_DOWNLOAD, SETTING(TOTAL_DOWNLOAD) + downdiff);
		lastUpdate = aTick;
		lastUp = Socket::getTotalUp();
		lastDown = Socket::getTotalDown();

		updateShutdown(aTick);
}

void MainFrame::on(TimerManagerListener::Minute, u_int32_t aTick) throw() {
	if(BOOLSETTING(EMPTY_WORKING_SET))
		SetProcessWorkingSetSize(GetCurrentProcess(), 0xffffffff, 0xffffffff);
}

void MainFrame::on(HttpConnectionListener::Data, HttpConnection* /*conn*/, const u_int8_t* buf, size_t len) throw() {
	versionInfo += string((const char*)buf, len);
}

void MainFrame::on(QueueManagerListener::Finished, QueueItem* qi) throw() {
		if(qi->isSet(QueueItem::FLAG_CLIENT_VIEW)) {
			if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
				// This is a file listing, show it...

				DirectoryListInfo* i = new DirectoryListInfo();
				i->file = qi->getListName();
				i->user = qi->getCurrents()[0]->getUser(); 
				i->start = qi->getSearchString();

				PostMessage(WM_SPEAKER, DOWNLOAD_LISTING, (LPARAM)i);
			} else if(qi->isSet(QueueItem::FLAG_TEXT)) {
				PostMessage(WM_SPEAKER, VIEW_FILE_AND_DELETE, (LPARAM) new string(qi->getTarget()));
			}
			} else if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
				DirectoryListInfo* i = new DirectoryListInfo();
				i->file = qi->getListName();
				i->user = qi->getCurrents()[0]->getUser(); 
				i->start = qi->getSearchString();

				PostMessage(WM_SPEAKER, CHECK_LISTING, (LPARAM)i);
		}	
}

LRESULT MainFrame::onActivateApp(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	bHandled = FALSE;
	WinUtil::isAppActive = wParam;	//wParam == TRUE if window is activated, FALSE if deactivated
	if(wParam == 1) {
		setNormalTrayIcon();
	}
	return 0;
}

void MainFrame::setNormalTrayIcon() {
	if(WinUtil::isPM) {
		if ( !WinUtil::trayIcon )
				return;
		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = m_hWnd;
		nid.uID = 0;
		nid.uFlags = NIF_ICON;
		nid.hIcon = normalicon.hIcon;
		::Shell_NotifyIcon(NIM_MODIFY, &nid);
		WinUtil::isPM = false;
	}
}

LRESULT MainFrame::onAppCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
		if(GET_APPCOMMAND_LPARAM(lParam) == APPCOMMAND_BROWSER_FORWARD)
		ctrlTab.SwitchTo();
	if(GET_APPCOMMAND_LPARAM(lParam) == APPCOMMAND_BROWSER_BACKWARD)
		ctrlTab.SwitchTo(false);
	
	return TRUE;
}

LRESULT MainFrame::onAway(WORD , WORD , HWND, BOOL& ) {
	if(Util::getAway()) { 
		setAwayButton(false);
		Util::setAway(false);
	} else {
		setAwayButton(true);
		Util::setAway(true);
	}
	ClientManager::getInstance()->infoUpdated(true);
	return 0;
}

void MainFrame::updateShutdown(u_int32_t aTick) {
	u_int32_t iSec = (aTick / 1000);
	if (bShutdown) {
		if (ctrlStatus.IsWindow()) {
			if(!isShutdownStatus) {
				ctrlStatus.SetIcon(9, hShutdownIcon);
				isShutdownStatus = true;
			}
			if (DownloadManager::getInstance()->getActiveDownloads() > 0) {
				iCurrentShutdownTime = iSec;
				ctrlStatus.SetText(9, string("").c_str());
			} else {
				ctrlStatus.SetText(9, string(' ' + Util::toTime(SETTING(SHUTDOWN_TIMEOUT) - (iSec - iCurrentShutdownTime))).c_str(), SBT_POPOUT);
				if (iCurrentShutdownTime + SETTING(SHUTDOWN_TIMEOUT) <= iSec) {
					bool bDidShutDown = false;
					bDidShutDown = CZDCLib::shutDown();
					if (bDidShutDown) {
						// Should we go faster here and force termination?
						// We "could" do a manual shutdown of this app...
					} else {
						ctrlStatus.SetText(0, CSTRING(FAILED_TO_SHUTDOWN));
						ctrlStatus.SetText(9, "");
					}
					// We better not try again. It WON'T work...
					bShutdown = false;
				}
			}
		}
	} else {
		if (ctrlStatus.IsWindow()) {
			if(isShutdownStatus) {
				ctrlStatus.SetText(9, "");
				ctrlStatus.SetIcon(9, NULL);
				isShutdownStatus = false;
			}
		}
	}
}

void MainFrame::checkFileList(string file, User::Ptr u) {
	if(u) {
		Client* c = u->getClient();
		if(c) {
			if(!c->getOp()) return;
			HubFrame* hubFrame = HubFrame::getHub(c);
			if(hubFrame == NULL) {
				return;
			}
			DirectoryListing* dl = new DirectoryListing(u);
			try {
				dl->loadFile(file, true);
			} catch(...) {
				delete dl;
				return;
			}
			hubFrame->checkCheating(u, dl);
			delete dl;
		}
	}
}

void MainFrame::SendCheatMessage(Client* client, User::Ptr u) {
	if(client) {
		HubFrame* hubFrame = HubFrame::getHub(client);
	
		CHARFORMAT2 cf;
		memset(&cf, 0, sizeof(CHARFORMAT2));
		cf.cbSize = sizeof(cf);
		cf.dwReserved = 0;
		cf.dwMask = CFM_BACKCOLOR | CFM_COLOR | CFM_BOLD;
		cf.dwEffects = 0;
		cf.crBackColor = SETTING(BACKGROUND_COLOR);
		cf.crTextColor = SETTING(ERROR_COLOR);

		hubFrame->addLine("*** "+STRING(USER)+" "+u->getNick()+": "+u->getCheatingString(),cf);
	}
}

LRESULT MainFrame::onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UpdateDlg dlg;
	dlg.DoModal();
	return S_OK;
}

LRESULT MainFrame::onDisableSounds(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	SettingsManager::getInstance()->set(SettingsManager::SOUNDS_DISABLED, !BOOLSETTING(SOUNDS_DISABLED));
	return 0;
}
/**
 * @file
 * $Id: MainFrm.cpp,v 1.20 2004/07/21 13:15:15 bigmuscle Exp
 */