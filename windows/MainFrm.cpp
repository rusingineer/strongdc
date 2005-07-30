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
#include "LineDlg.h"
#include "HashProgressDlg.h"
#include "UPnP.h"
#include "UploadQueueFrame.h"
#include "WinUtil.h"
#include "CDMDebugFrame.h"
#include "InputBox.h"
#include "PopupManager.h"

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
#include "../client/LogManager.h"
#include "../client/cvsversion.h"
#include "../client/WebServerManager.h"
#include "../client/Thread.h"


MainFrame* MainFrame::anyMF = NULL;
bool MainFrame::bShutdown = false;
u_int32_t MainFrame::iCurrentShutdownTime = 0;
bool MainFrame::isShutdownStatus = false;
CAGEmotionSetup* g_pEmotionsSetup = NULL;

MainFrame::MainFrame() : trayMessage(0), maximized(false), lastUpload(-1), lastUpdate(0), 
lastUp(0), lastDown(0), oldshutdown(false), stopperThread(NULL), c(new HttpConnection()), 
closing(false), awaybyminimize(false), missedAutoConnect(false), lastTTHdir(Util::emptyStringT),
bTrayIcon(false), bAppMinimized(false), bIsPM(false), UPnP_TCPConnection(NULL), UPnP_UDPConnection(NULL) { 
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
	WebServerManager::getInstance()->addListener(this);

	WinUtil::init(m_hWnd);

	// Register server socket message
	WSAAsyncSelect(ConnectionManager::getInstance()->getServerSocket().getSocket(),
		m_hWnd, SERVER_SOCKET_MESSAGE, FD_ACCEPT);
 	WSAAsyncSelect(WebServerManager::getInstance()->getServerSocket().getSocket(),m_hWnd, WEBSERVER_SOCKET_MESSAGE, FD_ACCEPT);

	trayMessage = RegisterWindowMessage(_T("TaskbarCreated"));

	TimerManager::getInstance()->start();

	// Set window name
#ifdef isCVS
	SetWindowText(_T(APPNAME) _T(" ") _T(VERSIONSTRING) _T("") _T(STRONGDCVERSIONSTRING) _T(CVSVERSION));
#else
	SetWindowText(_T(APPNAME) _T(" ") _T(VERSIONSTRING) _T("") _T(STRONGDCVERSIONSTRING));
#endif

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
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_RESTORE_ALL);
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
	statusSizes[0] = WinUtil::getTextWidth(TSTRING(AWAY), ::GetDC(ctrlStatus.m_hWnd)); // for "AWAY" segment

	CToolInfo ti(TTF_SUBCLASS, ctrlStatus.m_hWnd);

	ctrlLastLines.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON, WS_EX_TOPMOST);
	ctrlLastLines.SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	ctrlLastLines.AddTool(&ti);
	ctrlLastLines.SetDelayTime(TTDT_AUTOPOP, 30000);

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
	trayMenu.AppendMenu(MF_STRING, IDC_TRAY_SHOW, CTSTRING(MENU_SHOW));
	trayMenu.AppendMenu(MF_STRING, IDC_OPEN_DOWNLOADS, CTSTRING(MENU_OPEN_DOWNLOADS_DIR));
	trayMenu.AppendMenu(MF_SEPARATOR);
	trayMenu.AppendMenu(MF_STRING, IDC_REFRESH_FILE_LIST, CTSTRING(MENU_REFRESH_FILE_LIST));
	trayMenu.AppendMenu(MF_SEPARATOR);
	trayMenu.AppendMenu(MF_STRING, ID_APP_ABOUT, CTSTRING(MENU_ABOUT));
	trayMenu.AppendMenu(MF_STRING, ID_APP_EXIT, CTSTRING(MENU_EXIT));

	if(BOOLSETTING(GET_UPDATE_INFO)) {
		c->addListener(this);
		c->downloadFile("http://snail.pc.cz/StrongDC/version.xml");
	}

	// ZoneAlarm - ref: http://www.unixwiz.net/backstealth/
	if (BOOLSETTING(DETECT_ZONEALARM)) {
		if (FindWindow(NULL, _T("ZoneAlarm")) || FindWindow(NULL, _T("ZoneAlarm Pro"))) {
			int times_detected = SETTING(ZONEALARM_DETECTIONS);
			if (times_detected % 13 == 0) {
				::MessageBox(m_hWnd, CTSTRING(ZONEALARM_WARNING), _T(APPNAME) _T(" ") _T(VERSIONSTRING) _T("") _T(STRONGDCVERSIONSTRING), MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
			}
			SettingsManager::getInstance()->set(SettingsManager::ZONEALARM_DETECTIONS, ++times_detected);
		}
	}
	
	if(BOOLSETTING(OPEN_PUBLIC)) PostMessage(WM_COMMAND, ID_FILE_CONNECT);
	if(BOOLSETTING(OPEN_FAVORITE_HUBS)) PostMessage(WM_COMMAND, IDC_FAVORITES);
	if(BOOLSETTING(OPEN_FAVORITE_USERS)) PostMessage(WM_COMMAND, IDC_FAVUSERS);
	if(BOOLSETTING(OPEN_QUEUE)) PostMessage(WM_COMMAND, IDC_QUEUE);
	if(BOOLSETTING(OPEN_FINISHED_DOWNLOADS)) PostMessage(WM_COMMAND, IDC_FINISHED);
	if(BOOLSETTING(OPEN_FINISHED_UPLOADS)) PostMessage(WM_COMMAND, IDC_FINISHED_UL);
	if(BOOLSETTING(OPEN_SEARCH_SPY)) PostMessage(WM_COMMAND, IDC_SEARCH_SPY);
	if(BOOLSETTING(OPEN_NETWORK_STATISTICS)) PostMessage(WM_COMMAND, IDC_NET_STATS);
	if(BOOLSETTING(OPEN_NOTEPAD)) PostMessage(WM_COMMAND, IDC_NOTEPAD);

	if(!BOOLSETTING(SHOW_STATUSBAR)) PostMessage(WM_COMMAND, ID_VIEW_STATUS_BAR);
	if(!BOOLSETTING(SHOW_TOOLBAR)) PostMessage(WM_COMMAND, ID_VIEW_TOOLBAR);
	if(!BOOLSETTING(SHOW_TRANSFERVIEW))	PostMessage(WM_COMMAND, ID_VIEW_TRANSFER_VIEW);

	if(!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
		PostMessage(WM_SPEAKER, AUTO_CONNECT);

	PostMessage(WM_SPEAKER, PARSE_COMMAND_LINE);

	File::ensureDirectory(SETTING(LOG_DIRECTORY));

	startSocket();
	
	if (CZDCLib::isXp()) {
		normalicon.hIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MAINFRAME_XP), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
		pmicon.hIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TRAY_PM_XP), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	} else {
		normalicon.hIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
		pmicon.hIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TRAY_PM), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	}
	updateTray( BOOLSETTING( MINIMIZE_TRAY ) );


	if(BOOLSETTING(IPUPDATE))
		SettingsManager::getInstance()->set(SettingsManager::EXTERNAL_IP, Util::getLocalIp());

	Util::setAway(BOOLSETTING(AWAY));

	ctrlToolbar.CheckButton(IDC_AWAY,BOOLSETTING(AWAY));
	ctrlToolbar.CheckButton(IDC_LIMITER,BOOLSETTING(THROTTLE_ENABLE));
	ctrlToolbar.CheckButton(IDC_DISABLE_SOUNDS, BOOLSETTING(SOUNDS_DISABLED));

	if(SETTING(NICK).empty()) {
		PostMessage(WM_COMMAND, ID_FILE_SETTINGS);
	}
	m_PictureWindow.SubclassWindow(m_hWndMDIClient);
	m_PictureWindow.m_nMessageHandler = CPictureWindow::BackGroundPaint;
	currentPic = SETTING(BACKGROUND_IMAGE);
	m_PictureWindow.Load(Text::toT(currentPic).c_str());

	// We want to pass this one on to the splitter...hope it get's there...
	bHandled = FALSE;
	return 0;
}

void MainFrame::startSocket() {
	SearchManager::getInstance()->disconnect();
	ConnectionManager::getInstance()->disconnect();

//	if(ClientManager::getInstance()->isActive()) {
		try {
			ConnectionManager::getInstance()->listen();
			WSAAsyncSelect(ConnectionManager::getInstance()->getServerSocket().getSocket(), m_hWnd, SERVER_SOCKET_MESSAGE, FD_ACCEPT);
		} catch(const Exception&) {
			MessageBox(CTSTRING(TCP_PORT_BUSY), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
		}
		try {
			SearchManager::getInstance()->listen();
		} catch(const Exception&) {
			MessageBox(CTSTRING(TCP_PORT_BUSY), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
		}
//	}

	startUPnP();
}

void MainFrame::startUPnP() {
	stopUPnP();

	if( SETTING(INCOMING_CONNECTIONS) == SettingsManager::INCOMING_FIREWALL_UPNP ) {
		UPnP_TCPConnection = new UPnP( Util::getLocalIp(), "TCP", APPNAME " Download Port (" + Util::toString(ConnectionManager::getInstance()->getPort()) + " TCP)", ConnectionManager::getInstance()->getPort() );
		UPnP_UDPConnection = new UPnP( Util::getLocalIp(), "UDP", APPNAME " Search Port (" + Util::toString(SearchManager::getInstance()->getPort()) + " UDP)", SearchManager::getInstance()->getPort() );
		
		if ( FAILED(UPnP_UDPConnection->OpenPorts()) || FAILED(UPnP_TCPConnection->OpenPorts()) )
		{
			LogManager::getInstance()->message(STRING(UPNP_FAILED_TO_CREATE_MAPPINGS), true);
			MessageBox(CTSTRING(UPNP_FAILED_TO_CREATE_MAPPINGS), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONWARNING);
				
			// We failed! thus reset the objects
			delete UPnP_TCPConnection;
			delete UPnP_UDPConnection;
			UPnP_TCPConnection = UPnP_UDPConnection = NULL;
		}
		else
		{
			if(!BOOLSETTING(NO_IP_OVERRIDE)) {
				// now lets configure the external IP (connect to me) address
				string ExternalIP = UPnP_TCPConnection->GetExternalIP();
				if ( !ExternalIP.empty() ) {
					// woohoo, we got the external IP from the UPnP framework
					SettingsManager::getInstance()->set(SettingsManager::EXTERNAL_IP, ExternalIP );
				} else {
					//:-(  Looks like we have to rely on the user setting the external IP manually
					// no need to do cleanup here because the mappings work
					LogManager::getInstance()->message(STRING(UPNP_FAILED_TO_GET_EXTERNAL_IP), true);
					MessageBox(CTSTRING(UPNP_FAILED_TO_GET_EXTERNAL_IP), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONWARNING);
				}
			}
		}
	}
}

void MainFrame::stopUPnP() {
	// Just check if the port mapping objects are initialized (NOT NULL)
	if ( UPnP_TCPConnection != NULL )
	{
		if (FAILED(UPnP_TCPConnection->ClosePorts()) )
		{
			LogManager::getInstance()->message(STRING(UPNP_FAILED_TO_REMOVE_MAPPINGS), true);
		}
		delete UPnP_TCPConnection;
	}
	if ( UPnP_UDPConnection != NULL )
	{
		if (FAILED(UPnP_UDPConnection->ClosePorts()) )
		{
			LogManager::getInstance()->message(STRING(UPNP_FAILED_TO_REMOVE_MAPPINGS), true);
		}
		delete UPnP_UDPConnection;
	}
	// Not sure this is required (i.e. Objects are checked later in execution)
	// But its better being on the save side :P
	UPnP_TCPConnection = UPnP_UDPConnection = NULL;
}

HWND MainFrame::createToolbar() {
	if(!tbarcreated) {
	if(SETTING(TOOLBARIMAGE) == "")
		largeImages.CreateFromImage(IDB_TOOLBAR20, 20, 20, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	else
		WinUtil::createImageList1(largeImages, SETTING(TOOLBARIMAGE), 20);

	if(SETTING(TOOLBARHOTIMAGE) == "")
		largeImagesHot.CreateFromImage(IDB_TOOLBAR20_HOT, 20, 20, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	else
		WinUtil::createImageList1(largeImagesHot, SETTING(TOOLBARHOTIMAGE), 20);

	ctrlToolbar.Create(m_hWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS, 0, ATL_IDW_TOOLBAR);
	ctrlToolbar.SetImageList(largeImages);
	ctrlToolbar.SetHotImageList(largeImagesHot);
		tbarcreated = true;
	}

	while(ctrlToolbar.GetButtonCount()>0)
		ctrlToolbar.DeleteButton(0);

	ctrlToolbar.SetButtonStructSize();
	StringTokenizer<string> t(SETTING(TOOLBAR), ',');
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
		if(Util::fileExists(Text::fromT(i->file))) {
			DirectoryListingFrame::openWindow(i->file, i->user);
		}
		delete i;
	} else if(wParam == CHECK_LISTING) {
		DirectoryListInfo* i = (DirectoryListInfo*)lParam;
		if(Util::fileExists(Text::fromT(i->file))) {
			checkFileList(i->file, i->user);
			i->user->updated();
		}
		delete i;
	} else if(wParam == BROWSE_LISTING) {
		DirectoryBrowseInfo* i = (DirectoryBrowseInfo*)lParam;
		DirectoryListingFrame::openWindow(i->user, i->text);
		delete i;
	}  else if(wParam == VIEW_FILE_AND_DELETE) {
		tstring* file = (tstring*)lParam;
		TextFrame::openWindow(*file);
		File::deleteFile(Text::fromT(*file));
		delete file;
	} else if(wParam == STATS) {
		TStringList& str = *(TStringList*)lParam;
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

		if (bShutdown) {
			u_int32_t aTick = (u_int32_t)GET_TICK();
			u_int32_t iSec = (aTick / 1000);
			if (ctrlStatus.IsWindow()) {
				if(!isShutdownStatus) {
					ctrlStatus.SetIcon(9, hShutdownIcon);
					isShutdownStatus = true;
				}
				if (DownloadManager::getInstance()->getDownloadCount() > 0) {
					iCurrentShutdownTime = iSec;
					ctrlStatus.SetText(9, _T(""));
				} else {
					ctrlStatus.SetText(9, Text::toT(' ' + Util::toTime(SETTING(SHUTDOWN_TIMEOUT) - (iSec - iCurrentShutdownTime))).c_str(), SBT_POPOUT);
					if (iCurrentShutdownTime + SETTING(SHUTDOWN_TIMEOUT) <= iSec) {
						bool bDidShutDown = false;
						bDidShutDown = CZDCLib::shutDown(SETTING(SHUTDOWN_ACTION));
						if (bDidShutDown) {
							// Should we go faster here and force termination?
							// We "could" do a manual shutdown of this app...
						} else {
							ctrlStatus.SetText(0, CTSTRING(FAILED_TO_SHUTDOWN));
							ctrlStatus.SetText(9, _T(""));
						}
						// We better not try again. It WON'T work...
						bShutdown = false;
					}
				}
			}
		} else {
			if (ctrlStatus.IsWindow()) {
				if(isShutdownStatus) {
					ctrlStatus.SetText(9, _T(""));
					ctrlStatus.SetIcon(9, NULL);
					isShutdownStatus = false;
				}
			}
		}
	} else if(wParam == AUTO_CONNECT) {
		autoConnect(FavoriteManager::getInstance()->getFavoriteHubs());
	} else if(wParam == PARSE_COMMAND_LINE) {
		parseCommandLine(GetCommandLine());
	} else if(wParam == STATUS_MESSAGE) {
		tstring* msg = (tstring*)lParam;
		if(ctrlStatus.IsWindow()) {
			tstring line = Text::toT("[" + Util::getShortTimeString() + "] ") + *msg;

			ctrlStatus.SetText(0, line.c_str());
			while(lastLinesList.size() + 1 > MAX_CLIENT_LINES)
				lastLinesList.erase(lastLinesList.begin());
			if (line.find(_T('\r')) == tstring::npos) {
				lastLinesList.push_back(line);
			} else {
				lastLinesList.push_back(line.substr(0, line.find(_T('\r'))));
			}
		}
		delete msg;
	} else if(wParam == SHOW_POPUP) {
		Popup* msg = (Popup*)lParam;
		PopupManager::getInstance()->Show(Text::fromT(msg->Message), Text::fromT(msg->Title), msg->Icon);
		delete msg;
	} else if(wParam == SET_NORMAL_TRAY_ICON) {
		if(bIsPM && bTrayIcon == true) {
			NOTIFYICONDATA nid;
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = m_hWnd;
			nid.uID = 0;
			nid.uFlags = NIF_ICON;
			nid.hIcon = normalicon.hIcon;
			::Shell_NotifyIcon(NIM_MODIFY, &nid);
			bIsPM = false;
		}
	} else if(wParam == SET_PM_TRAY_ICON) {
		if(bIsPM == false && (!WinUtil::isAppActive || bAppMinimized) && bTrayIcon == true) {
			NOTIFYICONDATA nid;
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = m_hWnd;
			nid.uID = 0;
			nid.uFlags = NIF_ICON;
			nid.hIcon = pmicon.hIcon;
			::Shell_NotifyIcon(NIM_MODIFY, &nid);
			bIsPM = true;
		}
    }

	return 0;
}

void MainFrame::parseCommandLine(const tstring& cmdLine)
{
	string::size_type i = 0;
	string::size_type j;

	if( (j = cmdLine.find(_T("dchub://"), i)) != string::npos) {
		WinUtil::parseDchubUrl(cmdLine.substr(j));
		}
	if( (j = cmdLine.find(_T("adc://"), i)) != string::npos) {
		WinUtil::parseADChubUrl(cmdLine.substr(j));
	}
	if( (j = cmdLine.find(_T("magnet:?"), i)) != string::npos) {
		WinUtil::parseMagnetUri(cmdLine.substr(j));
	}
}

LRESULT MainFrame::onCopyData(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	tstring cmdLine = (LPCTSTR) (((COPYDATASTRUCT *)lParam)->lpData);
	parseCommandLine(Text::toT(Util::getAppName() + " ") + cmdLine);
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

LRESULT MainFrame::onHashProgress(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	HashProgressDlg(false).DoModal(m_hWnd);
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
	AboutDlg dlg;
	dlg.DoModal(m_hWnd);
	return 0;
}

LRESULT MainFrame::OnFileSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	PropertiesDlg dlg(SettingsManager::getInstance());

	short lastPort = (short)SETTING(TCP_PORT);
	short lastUDP = (short)SETTING(UDP_PORT);
	int lastConn = SETTING(INCOMING_CONNECTIONS);

	if(dlg.DoModal(m_hWnd) == IDOK) {
		SettingsManager::getInstance()->save();
		if(missedAutoConnect && !SETTING(NICK).empty()) {
			PostMessage(WM_SPEAKER, AUTO_CONNECT);
		}
		if(SETTING(INCOMING_CONNECTIONS) != lastConn || SETTING(TCP_PORT) != lastPort || SETTING(UDP_PORT) != lastUDP) {
			startSocket();
		}
		ClientManager::getInstance()->infoUpdated(false);

		if(BOOLSETTING(URL_HANDLER)) {
			WinUtil::registerDchubHandler();
			WinUtil::registerADChubHandler();
			WinUtil::urlDcADCRegistered = true;
		} else if(WinUtil::urlDcADCRegistered) {
			WinUtil::unRegisterDchubHandler();
			WinUtil::unRegisterADChubHandler();
			WinUtil::urlDcADCRegistered = false;
		}
		if(BOOLSETTING(MAGNET_REGISTER)) {
			WinUtil::registerMagnetHandler();
			WinUtil::urlMagnetRegistered = true; 
		} else if(WinUtil::urlMagnetRegistered) {
			WinUtil::unRegisterMagnetHandler();
			WinUtil::urlMagnetRegistered = false;
		}

		if(BOOLSETTING(THROTTLE_ENABLE)) ctrlToolbar.CheckButton(IDC_LIMITER, true);
		else ctrlToolbar.CheckButton(IDC_LIMITER, false);

		if(Util::getAway()) ctrlToolbar.CheckButton(IDC_AWAY, true);
		else ctrlToolbar.CheckButton(IDC_AWAY, false);

		if(getShutDown()) ctrlToolbar.CheckButton(IDC_SHUTDOWN, true);
		else ctrlToolbar.CheckButton(IDC_SHUTDOWN, false);
	
		updateTray(BOOLSETTING(MINIMIZE_TRAY));
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
							MessageBox(Text::toT(msg).c_str(), Text::toT(title).c_str(), MB_OK);
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
						MessageBox(Text::toT(msg + "\r\nPlease get a new one at " + url).c_str());
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
						MessageBox(Text::toT(msg + "\r\nPlease get a new one at " + url).c_str(), _T("Bad DC++ version"), MB_OK | MB_ICONEXCLAMATION);
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

LRESULT MainFrame::onWebServerSocket(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	WebServerManager::getInstance()->getServerSocket().incoming();
	return 0;
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
			_tcsncpy(pDispInfo->lpszText, CTSTRING_I((ResourceManager::Strings)stringId), 79);
			pDispInfo->uFlags |= TTF_DI_SETITEM;
		}
	} else { // if we're really in the status bar, this should be detected intelligently
		lastLines.clear();
		for(TStringIter i = lastLinesList.begin(); i != lastLinesList.end(); ++i) {
			lastLines += *i;
			lastLines += _T("\r\n");
		}
		if(lastLines.size() > 2) {
			lastLines.erase(lastLines.size() - 2);
		}
		pDispInfo->lpszText = const_cast<TCHAR*>(lastLines.c_str());
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
				FavoriteManager::getInstance()->addRecent(r);
				HubFrame::openWindow(Text::toT(entry->getServer())
					, Text::toT(entry->getRawOne())
					, Text::toT(entry->getRawTwo())
					, Text::toT(entry->getRawThree())
					, Text::toT(entry->getRawFour())
					, Text::toT(entry->getRawFive())
					, entry->getWindowPosX(), entry->getWindowPosY(), entry->getWindowSizeX(), entry->getWindowSizeY(), entry->getWindowType(), entry->getChatUserSplit(), entry->getStealth(), entry->getUserListState());
 			} else
 				missedAutoConnect = true;
 		}				
	}
}

void MainFrame::updateTray(bool add /* = true */) {
	if(add) {
		if(bTrayIcon == false) {
			NOTIFYICONDATA nid;
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = m_hWnd;
			nid.uID = 0;
			nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
			nid.uCallbackMessage = WM_APP + 242;
			nid.hIcon = normalicon.hIcon;
			_tcsncpy(nid.szTip, _T(APPNAME), 64);
			nid.szTip[63] = '\0';
			lastMove = GET_TICK() - 1000;
			::Shell_NotifyIcon(NIM_ADD, &nid);
			bTrayIcon = true;
		}
	} else {
		if(bTrayIcon) {
			NOTIFYICONDATA nid;
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = m_hWnd;
			nid.uID = 0;
			nid.uFlags = 0;
			::Shell_NotifyIcon(NIM_DELETE, &nid);
			ShowWindow(SW_SHOW);
			bTrayIcon = false;
		}
	}
}

LRESULT MainFrame::onSize(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	if(wParam == SIZE_MINIMIZED) {
		SetProcessWorkingSetSize(GetCurrentProcess(), 0xffffffff, 0xffffffff);
		if(BOOLSETTING(AUTO_AWAY)) {
			if(bAppMinimized == false)
			if(Util::getAway() == true) {
				awaybyminimize = false;
			} else {
				awaybyminimize = true;
				Util::setAway(true);
				setAwayButton(true);
				ClientManager::getInstance()->infoUpdated(false);
			}
		}

		bAppMinimized = true;
		if(BOOLSETTING(MINIMIZE_TRAY)) {
			updateTray(true);
			ShowWindow(SW_HIDE);
		} else {
			updateTray(false);
		}
		maximized = IsZoomed() > 0;
	} else if( (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED) ) {
		if(BOOLSETTING(AUTO_AWAY)) {
			if(awaybyminimize == true) {
				awaybyminimize = false;
				Util::setAway(false);
				setAwayButton(false);
				ClientManager::getInstance()->infoUpdated(false);
			}
		}
		setNormalTrayIcon();
		bAppMinimized = false;
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
		if( oldshutdown ||(!BOOLSETTING(CONFIRM_EXIT)) || (MessageBox(CTSTRING(REALLY_EXIT), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) ) {
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
			
			WebServerManager::getInstance()->removeListener(this);
			TimerManager::getInstance()->removeListener(this);
			QueueManager::getInstance()->removeListener(this);
			LogManager::getInstance()->removeListener(this);
			SearchManager::getInstance()->disconnect();
			ConnectionManager::getInstance()->disconnect();

			stopUPnP();

			DWORD id;
			stopperThread = CreateThread(NULL, 0, stopper, this, 0, &id);
			closing = true;
		}
		bHandled = TRUE;
	} else {
		// This should end immediately, as it only should be the stopper that sends another WM_CLOSE
		WaitForSingleObject(stopperThread, 60*1000);
		CloseHandle(stopperThread);
		stopperThread = NULL;
		DestroyIcon(normalicon.hIcon);
		DestroyIcon(hShutdownIcon); 	
		DestroyIcon(pmicon.hIcon);
		bHandled = FALSE;
	}

	return 0;
}

LRESULT MainFrame::onLink(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	tstring site;
	bool isFile = false;
	switch(wID) {
	case IDC_HELP_HOMEPAGE: site = _T("http://strongdc.berlios.de"); break;
	case IDC_HELP_DISCUSS: site = _T("http://strongdc.berlios.de/forum/index.php"); break;
	case IDC_HELP_GEOIPFILE: site = _T("http://www.maxmind.com/download/geoip/database/GeoIPCountryCSV.zip"); break;
	default: dcassert(0);
	}

	if(isFile)
		WinUtil::openFile(site);
	else
		WinUtil::openLink(site);

	return 0;
}

int MainFrame::run() {
	WinUtil::mainMenu.EnableMenuItem(ID_GET_TTH, MF_GRAYED);
	tstring file = Text::toT(Util::getAppPath()) + _T("*.*");
	if(WinUtil::browseFile(file, m_hWnd, false, lastTTHdir) == IDOK) {
		lastTTHdir = Util::getFilePath(file);

		char TTH[192*8/(5*8)+2];

		char buf[512*1024];

		File f(Text::fromT(file), File::READ, File::OPEN);
		TigerTree tth(TigerTree::calcBlockSize(f.getSize(), 1));

		size_t n = 0;
		size_t n2 = 512*1024;
		while( (n = f.read(buf, n2)) > 0) {
			tth.update(buf, n);
			n2 = 512*1024;
		}
		tth.finalize();
		f.close();

		strcpy(TTH, tth.getRoot().toBase32().c_str());

		CInputBox ibox(m_hWnd);

		string magnetlink = "magnet:?xt=urn:tree:tiger:"+ string(TTH) +"&xl="+Util::toString(f.getSize())+"&dn="+Util::encodeURI(Text::fromT(Util::getFileName(file)));
		ibox.DoModal(_T("Tiger Tree Hash"), file.c_str(), Text::toT(TTH).c_str(), Text::toT(magnetlink).c_str());
	}
	WinUtil::mainMenu.EnableMenuItem(ID_GET_TTH, MF_ENABLED);
	return 0;
}

LRESULT MainFrame::onGetTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Thread::start();
	Thread::setThreadPriority(Thread::NORMAL); // change as you wish
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
	}
	CRect rc = rect;
	rc.top = rc.bottom - ctrlTab.getHeight();
	if(ctrlTab.IsWindow())
		ctrlTab.MoveWindow(rc);
	
	CRect rc2 = rect;
	rc2.bottom = rc.top;
	SetSplitterRect(rc2);
}

static const TCHAR types[] = _T("File Lists\0*.DcLst;*.xml.bz2\0All Files\0*.*\0");

LRESULT MainFrame::onOpenFileList(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring file;
	
	if(wID == IDC_OPEN_MY_LIST){
		if(!ShareManager::getInstance()->getOwnListFile().empty()){
			DirectoryListingFrame::openWindow(Text::toT(ShareManager::getInstance()->getOwnListFile()), ClientManager::getInstance()->getUser(SETTING(NICK)));
		}
		return 0;
	}


	if(WinUtil::browseFile(file, m_hWnd, false, Text::toT(Util::getAppPath() + "FileLists\\"), types)) {
		tstring username;
		if(file.rfind('\\') != string::npos) {
			username = file.substr(file.rfind('\\') + 1);
			if(username.rfind('.') != string::npos) {
				username.erase(username.rfind('.'));
			}
			if(username.length() > 4 && Util::stricmp(username.c_str() + username.length() - 4, _T(".xml")) == 0)
				username.erase(username.length()-4);
			DirectoryListingFrame::openWindow(file, ClientManager::getInstance()->getUser(Text::fromT(username)));
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
		if(bAppMinimized) {
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
		_tcsncpy(nid.szTip, Text::toT("D: " + Util::formatBytes(DownloadManager::getInstance()->getAverageSpeed()) + "/s (" + 
			Util::toString(DownloadManager::getInstance()->getDownloadCount()) + ")\r\nU: " +
			Util::formatBytes(UploadManager::getInstance()->getAverageSpeed()) + "/s (" + 
			Util::toString(UploadManager::getInstance()->getUploadCount()) + ")"
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

void MainFrame::ShowBalloonTip(LPCTSTR szMsg, LPCTSTR szTitle, DWORD dwInfoFlags) {
	Popup* p = new Popup;
	p->Title = szTitle;
	p->Message = szMsg;
	p->Icon = dwInfoFlags;

	PostMessage(WM_SPEAKER, SHOW_POPUP, (LPARAM)p);
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

LRESULT MainFrame::onQuickConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	LineDlg dlg;
	dlg.description = TSTRING(HUB_ADDRESS);
	dlg.title = TSTRING(QUICK_CONNECT);
	if(dlg.DoModal(m_hWnd) == IDOK){
		if(SETTING(NICK).empty())
			return 0;

		tstring tmp = dlg.line;
		// Strip out all the spaces
		string::size_type i;
		while((i = tmp.find(' ')) != string::npos)
			tmp.erase(i, 1);

		RecentHubEntry r;
		r.setName("*");
		r.setDescription("*");
		r.setUsers("*");
		r.setShared("*");
		r.setServer(Text::fromT(tmp));
		FavoriteManager::getInstance()->addRecent(r);
		HubFrame::openWindow(tmp);
	}
	return 0;
}

void MainFrame::on(TimerManagerListener::Second, u_int32_t aTick) throw() {
		Util::increaseUptime();
		int64_t diff = (int64_t)((lastUpdate == 0) ? aTick - 1000 : aTick - lastUpdate);
		int64_t updiff = Socket::getTotalUp() - lastUp;
		int64_t downdiff = Socket::getTotalDown() - lastDown;

		if(BOOLSETTING(THROTTLE_ENABLE)) {
			// Limitery sem a tam, vsude kam se podivam :o)
			if( SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL) > 0) {
				if( SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL) < ((5 * UploadManager::getInstance()->getSlots()) + 5) ) {
					SettingsManager::getInstance()->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT_NORMAL, ((5 * UploadManager::getInstance()->getSlots()) + 5) );
				}
				if ( (SETTING(MAX_DOWNLOAD_SPEED_LIMIT_NORMAL) > ( SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL) * 7)) || ( SETTING(MAX_DOWNLOAD_SPEED_LIMIT_NORMAL) == 0) ) {
					SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT_NORMAL, (SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL)*7) );
				}
			}

			if( SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME) > 0) {
				if( SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME) < ((5 * UploadManager::getInstance()->getSlots()) + 5) ) {
					SettingsManager::getInstance()->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT_TIME, ((5 * UploadManager::getInstance()->getSlots()) + 5) );
				}
				if ( (SETTING(MAX_DOWNLOAD_SPEED_LIMIT_TIME) > ( SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME) * 7)) || ( SETTING(MAX_DOWNLOAD_SPEED_LIMIT_TIME) == 0) ) {
					SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT_TIME, (SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME)*7) );
				}
			}

			time_t currentTime;
			time(&currentTime);
			int currentHour = localtime(&currentTime)->tm_hour;
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

		TStringList* str = new TStringList();
		str->push_back(Util::getAway() ? TSTRING(AWAY) : _T(""));
		str->push_back(Text::toT(STRING(SHARED) + ": " + Util::formatBytes(ShareManager::getInstance()->getSharedSize())));
		str->push_back(Text::toT("H: " + Client::getCounts()));
		str->push_back(Text::toT(STRING(SLOTS) + ": " + Util::toString(UploadManager::getInstance()->getFreeSlots()) + '/' + Util::toString(UploadManager::getInstance()->getSlots()) + " (" + Util::toString(UploadManager::getInstance()->getFreeExtraSlots()) + '/' + Util::toString(SETTING(EXTRA_SLOTS)) + ")"));
		str->push_back(Text::toT("D: " + Util::formatBytes(Socket::getTotalDown())));
		str->push_back(Text::toT("U: " + Util::formatBytes(Socket::getTotalUp())));
		str->push_back(Text::toT("D: [" + Util::toString(DownloadManager::getInstance()->getDownloadCount()) + "][" + (SETTING(MAX_DOWNLOAD_SPEED_LIMIT) == 0 ? string("N") : Util::toString((int)SETTING(MAX_DOWNLOAD_SPEED_LIMIT)) + "k") + "] " + Util::formatBytes(downdiff*1000I64/diff) + "/s"));
		str->push_back(Text::toT("U: [" + Util::toString(UploadManager::getInstance()->getUploadCount()) + "][" + (SETTING(MAX_UPLOAD_SPEED_LIMIT) == 0 ? string("N") : Util::toString((int)SETTING(MAX_UPLOAD_SPEED_LIMIT)) + "k") + "] " + Util::formatBytes(updiff*1000I64/diff) + "/s"));
		PostMessage(WM_SPEAKER, STATS, (LPARAM)str);
		SettingsManager::getInstance()->set(SettingsManager::TOTAL_UPLOAD, SETTING(TOTAL_UPLOAD) + updiff);
		SettingsManager::getInstance()->set(SettingsManager::TOTAL_DOWNLOAD, SETTING(TOTAL_DOWNLOAD) + downdiff);
		lastUpdate = aTick;
		lastUp = Socket::getTotalUp();
		lastDown = Socket::getTotalDown();
		if(currentPic != SETTING(BACKGROUND_IMAGE)) {
			currentPic = SETTING(BACKGROUND_IMAGE);
			m_PictureWindow.Load(Text::toT(currentPic).c_str());
		}
}

void MainFrame::on(TimerManagerListener::Minute, u_int32_t aTick) throw() {
	if(BOOLSETTING(EMPTY_WORKING_SET))
		SetProcessWorkingSetSize(GetCurrentProcess(), 0xffffffff, 0xffffffff);
}

void MainFrame::on(HttpConnectionListener::Data, HttpConnection* /*conn*/, const u_int8_t* buf, size_t len) throw() {
	versionInfo += string((const char*)buf, len);
}

void MainFrame::on(PartialList, const User::Ptr& aUser, const string& text) throw() {
	PostMessage(WM_SPEAKER, BROWSE_LISTING, (LPARAM)new DirectoryBrowseInfo(aUser, text));
}

void MainFrame::on(QueueManagerListener::Finished, QueueItem* qi) throw() {
	if(qi->isSet(QueueItem::FLAG_CLIENT_VIEW)) {
		if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
			// This is a file listing, show it...

			DirectoryListInfo* i = new DirectoryListInfo();
			i->file = Text::toT(qi->getListName());
			i->user = qi->getCurrents()[0]->getUser(); 

			PostMessage(WM_SPEAKER, DOWNLOAD_LISTING, (LPARAM)i);
		} else if(qi->isSet(QueueItem::FLAG_TEXT)) {
			PostMessage(WM_SPEAKER, VIEW_FILE_AND_DELETE, (LPARAM) new tstring(Text::toT(qi->getTarget())));
		}
	} else if(qi->isSet(QueueItem::FLAG_USER_LIST) && qi->isSet(QueueItem::FLAG_CHECK_FILE_LIST)) {
		DirectoryListInfo* i = new DirectoryListInfo();
		i->file = Text::toT(qi->getListName());
		i->user = qi->getCurrents()[0]->getUser(); 
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

void MainFrame::checkFileList(tstring file, User::Ptr u) {
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
				dl->loadFile(Text::fromT(file));
				hubFrame->checkCheating(u, dl);
			} catch(...) {
			}
			delete dl;
		}
	}
}

/*void MainFrame::SendCheatMessage(Client* client, User::Ptr u) {
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

		hubFrame->addLine(_T("*** ")+TSTRING(USER)+_T(" ")+Text::toT(u->getNick())+_T(": ")+Text::toT(u->getCheatingString()),cf);
	}
}*/

LRESULT MainFrame::onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UpdateDlg dlg;
	dlg.DoModal();
	return S_OK;
}

LRESULT MainFrame::onDisableSounds(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	SettingsManager::getInstance()->set(SettingsManager::SOUNDS_DISABLED, !BOOLSETTING(SOUNDS_DISABLED));
	return 0;
}

void MainFrame::on(WebServerListener::Setup) throw() {
	WSAAsyncSelect(WebServerManager::getInstance()->getServerSocket().getSocket(),m_hWnd, WEBSERVER_SOCKET_MESSAGE, FD_ACCEPT);
}

void MainFrame::on(WebServerListener::ShutdownPC, int action) throw() {
	CZDCLib::shutDown(action);
}
/**
 * @file
 * $Id: MainFrm.cpp,v 1.20 2004/07/21 13:15:15 bigmuscle Exp
 */