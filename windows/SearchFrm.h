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

#if !defined(AFX_SEARCHFRM_H__A7078724_FD85_4F39_8463_5A08A5F45E33__INCLUDED_)
#define AFX_SEARCHFRM_H__A7078724_FD85_4F39_8463_5A08A5F45E33__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"
#include "WinUtil.h"

#include "../client/Client.h"
#include "../client/SearchManager.h"
#include "../client/CriticalSection.h"
#include "../client/ClientManagerListener.h"

#include "UCHandler.h"

#define SEARCH_MESSAGE_MAP 6		// This could be any number, really...
#define SHOWUI_MESSAGE_MAP 7

class SearchFrame : public MDITabChildWindowImpl<SearchFrame, RGB(127, 127, 255), IDR_SEARCH>, 
	private SearchManagerListener, private ClientManagerListener, 
	public UCHandler<SearchFrame>, public UserInfoBaseHandler<SearchFrame>
{
public:
	static void openWindow(const string& str = Util::emptyString, LONGLONG size = 0, SearchManager::SizeModes mode = SearchManager::SIZE_ATLEAST, SearchManager::TypeModes type = SearchManager::TYPE_ANY);

	DECLARE_FRAME_WND_CLASS_EX("SearchFrame", IDR_SEARCH, 0, COLOR_3DFACE)

	typedef MDITabChildWindowImpl<SearchFrame, RGB(127, 127, 255), IDR_SEARCH> baseClass;
	typedef UCHandler<SearchFrame> ucBase;
	typedef UserInfoBaseHandler<SearchFrame> uicBase;

	BEGIN_MSG_MAP(SearchFrame)
		NOTIFY_HANDLER(IDC_RESULTS, LVN_GETDISPINFO, ctrlResults.onGetDispInfo)
		NOTIFY_HANDLER(IDC_RESULTS, LVN_COLUMNCLICK, ctrlResults.onColumnClick)
		NOTIFY_HANDLER(IDC_HUB, LVN_GETDISPINFO, ctrlHubs.onGetDispInfo)
		NOTIFY_HANDLER(IDC_RESULTS, NM_DBLCLK, onDoubleClickResults)
		NOTIFY_HANDLER(IDC_RESULTS, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_RESULTS, NM_CLICK, onLButton)
		NOTIFY_HANDLER(IDC_RESULTS, NM_CUSTOMDRAW, onCustomDraw)
		NOTIFY_HANDLER(IDC_HUB, LVN_ITEMCHANGED, onItemChangedHub)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, onCtlColor)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_DRAWITEM, onDrawItem)
		MESSAGE_HANDLER(WM_MEASUREITEM, onMeasure)
		COMMAND_ID_HANDLER(IDC_DOWNLOAD, onDownload)
		COMMAND_ID_HANDLER(IDC_DOWNLOADTO, onDownloadTo)
		COMMAND_ID_HANDLER(IDC_DOWNLOADDIR, onDownloadWhole)
		COMMAND_ID_HANDLER(IDC_DOWNLOADDIRTO, onDownloadWholeTo)
		COMMAND_ID_HANDLER(IDC_VIEW_AS_TEXT, onViewAsText)
		COMMAND_ID_HANDLER(IDC_MP3, onMP3Info)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_SEARCH, onSearch)
		COMMAND_ID_HANDLER(IDC_COPY_NICK, onCopyNick)
		COMMAND_ID_HANDLER(IDC_COPY_FILENAME, onCopyFilename)
		COMMAND_ID_HANDLER(IDC_COPY_PATH, onCopyPath)
		COMMAND_ID_HANDLER(IDC_COPY_SIZE, onCopySize)
		COMMAND_ID_HANDLER(IDC_COPY_TTH, onCopyTTH)		
		COMMAND_ID_HANDLER(IDC_FREESLOTS, onFreeSlots)
		COMMAND_ID_HANDLER(IDC_TTHONLY, onTTHOnly)
		COMMAND_ID_HANDLER(IDC_GETLIST, onGetList)
		COMMAND_ID_HANDLER(IDC_BITZI_LOOKUP, onBitziLookup)
		COMMAND_ID_HANDLER(IDC_COPY_LINK, onCopyMagnetLink)		
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_TARGET, IDC_DOWNLOAD_TARGET + targets.size() + WinUtil::lastDirs.size(), onDownloadTarget)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_WHOLE_TARGET, IDC_DOWNLOAD_WHOLE_TARGET + WinUtil::lastDirs.size(), onDownloadWholeTarget)
		CHAIN_COMMANDS(ucBase)
		CHAIN_COMMANDS(uicBase)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(SEARCH_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CHAR, onChar)
		MESSAGE_HANDLER(WM_KEYDOWN, onChar)
		MESSAGE_HANDLER(WM_KEYUP, onChar)
	ALT_MSG_MAP(SHOWUI_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onShowUI)
	END_MSG_MAP()

	SearchFrame() : 
	searchBoxContainer("COMBOBOX", this, SEARCH_MESSAGE_MAP),
		searchInProgress(false),
		searchContainer("edit", this, SEARCH_MESSAGE_MAP), 
		sizeContainer("edit", this, SEARCH_MESSAGE_MAP), 
		modeContainer("COMBOBOX", this, SEARCH_MESSAGE_MAP),
		sizeModeContainer("COMBOBOX", this, SEARCH_MESSAGE_MAP),
		fileTypeContainer("COMBOBOX", this, SEARCH_MESSAGE_MAP),
//		fileTypeSubContainer("COMBOBOX", this, SEARCH_MESSAGE_MAP),
		showUIContainer("BUTTON", this, SHOWUI_MESSAGE_MAP),
		slotsContainer("BUTTON", this, SEARCH_MESSAGE_MAP),
		tthContainer("BUTTON", this, SEARCH_MESSAGE_MAP),
		doSearchContainer("BUTTON", this, SEARCH_MESSAGE_MAP),
		resultsContainer(WC_LISTVIEW, this, SEARCH_MESSAGE_MAP),
		hubsContainer(WC_LISTVIEW, this, SEARCH_MESSAGE_MAP),
		lastSearch(0), initialSize(0), initialMode(SearchManager::SIZE_ATLEAST), initialType(SearchManager::TYPE_ANY),
		showUI(true), onlyFree(false), closed(false), isHash(false), onlyTTH(false)	
	{	
		SearchManager::getInstance()->addListener(this);
	}

	virtual ~SearchFrame() {
		images.Destroy();
		searchTypes.Destroy();
		states.Destroy();
	}
	virtual void OnFinalMessage(HWND /*hWnd*/) { delete this; }

	LRESULT onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onDrawItem(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onMeasure(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onCtlColor(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onDoubleClickResults(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onDownloadTarget(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadWholeTarget(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadWholeTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onBitziLookup(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyNick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyFilename(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyPath(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopySize(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyMagnetLink(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onLButton(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled); 
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);

	void UpdateLayout(BOOL bResizeBars = TRUE);
	void runUserCommand(UserCommand& uc);

	void removeSelected() {
		int i = -1;
		while( (i = ctrlResults.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			SearchInfo* s = (SearchInfo*)ctrlResults.getItemData(i);
			if(s->subItems.size() > 0) {
				int q = 0;
				while(q<s->subItems.size()) {
					SearchInfo* j = s->subItems[q];
					ctrlResults.deleteItem(j);
					q++;
				}
			}
			
			if(s->main != NULL)
				s->main->subItems.erase(find(s->main->subItems.begin(), s->main->subItems.end(), s));
			ctrlResults.deleteItem(i);
		}
	}
	
	LRESULT onMP3Info(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlResults.forEachSelected(&SearchInfo::GetMP3Info);
		return 0;
	}
	
	LRESULT onDownload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		//PDC {
		int i = -1;
		while( (i = ctrlResults.GetNextItem(i, LVNI_SELECTED)) != -1) {
			SearchInfo* si = ctrlResults.getItemData(i);
			string t = SettingsManager::getInstance()->getDownloadDir(Util::getFileExt(si->getFileName()));		
			(SearchInfo::Download(t))(ctrlResults.getItemData(i));
		}
		//PDC }
		return 0;
	}

	LRESULT onViewAsText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlResults.forEachSelected(&SearchInfo::view);
		return 0;
	}

	LRESULT onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onDownloadWhole(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlResults.forEachSelectedT(SearchInfo::DownloadWhole(SETTING(DOWNLOAD_DIRECTORY)));
		return 0;
	}
	
	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		removeSelected();
		return 0;
	}

	LRESULT onFreeSlots(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		onlyFree = (ctrlSlots.GetCheck() == 1);
		return 0;
	}

	LRESULT onTTHOnly(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		onlyTTH = (ctrlTTH.GetCheck() == 1);
		return 0;
	}

	LRESULT onSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(searchInProgress == true)
	 	{
 		  stopSearch();
	 	} else {
		  onEnter();
		}
 			return 0;
		}

void startSearch()
{
 searchInProgress = true;
 ctrlDoSearch.SetWindowText(CSTRING(STOP_SEARCH));
}

void stopSearch()
{
 search.clear();
 searchInProgress = false;
 ctrlDoSearch.SetWindowText(CSTRING(SEARCH));
 ctrlStatus.SetText(1, CSTRING(SEARCH_STOPPED));
}

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
		
		if(kd->wVKey == VK_DELETE) {
			removeSelected();
		} 
		return 0;
	}

	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		if(::IsWindow(ctrlSearch))
			ctrlSearch.SetFocus();
		return 0;
	}

	LRESULT onShowUI(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		showUI = (wParam == BST_CHECKED);
		UpdateLayout(FALSE);
		return 0;
	}

	void SearchFrame::setInitial(const string& str, LONGLONG size, SearchManager::SizeModes mode, SearchManager::TypeModes type) {
		initialString = str; initialSize = size; initialMode = mode; initialType = type;
	}
	
private:
	bool searchInProgress;
	class SearchInfo;

public:
	TypedListViewCtrlCleanup<SearchInfo, IDC_RESULTS>& getUserList() { return ctrlResults; };

private:
	enum {
		COLUMN_FIRST,
		COLUMN_FILENAME = COLUMN_FIRST,
		COLUMN_NICK,
		COLUMN_TYPE,
		COLUMN_SIZE,
		COLUMN_PATH,
		COLUMN_SLOTS,
		COLUMN_CONNECTION,
		COLUMN_HUB,
		COLUMN_EXACT_SIZE,
		COLUMN_UPLOAD,
		COLUMN_IP,		
		COLUMN_TTH,
		COLUMN_LAST
	};

	enum Images {
		IMAGE_UNKOWN,
		IMAGE_SLOW,
		IMAGE_NORMAL,
		IMAGE_FAST
	};

	class SearchInfo : public UserInfoBase {
	public:
		SearchResult* sr;

		typedef SearchInfo* Ptr;
		typedef vector<Ptr> List;
		typedef List::iterator Iter;

		SearchInfo::List subItems;

		SearchInfo(SearchResult* aSR) : UserInfoBase(aSR->getUser()), sr(aSR), collapsed(true), mainitem(false), main(NULL) { 
			sr->incRef(); update();
		};
		~SearchInfo() { 
			sr->decRef(); 
		};

		bool collapsed;
		bool mainitem;
		SearchInfo* main;

		void getList();
		void view();
		void GetMP3Info();
		struct Download {
			Download(const string& aTarget) : tgt(aTarget) { };
			void operator()(SearchInfo* si);
			const string& tgt;
		};
		struct DownloadWhole {
			DownloadWhole(const string& aTarget) : tgt(aTarget) { };
			void operator()(SearchInfo* si);
			const string& tgt;
		};
		struct DownloadTarget {
			DownloadTarget(const string& aTarget) : tgt(aTarget) { };
			void operator()(SearchInfo* si);
			const string& tgt;
		};
		struct CheckSize {
			CheckSize() : size(-1), op(true), oneHub(true) { };
			void operator()(SearchInfo* si);
			string ext;
			int64_t size;
			bool oneHub;
			string hub;
			bool op;
		};

		const string& getText(int col) const {
			switch(col) {
				case COLUMN_NICK: return hits;
				case COLUMN_FILENAME: return fileName;
				case COLUMN_TYPE: return type;
				case COLUMN_SIZE: return size;
				case COLUMN_PATH: return path;
				case COLUMN_SLOTS: return slots;
				case COLUMN_CONNECTION: return sr->getUser()->getConnection();
				case COLUMN_HUB: return sr->getHubName();
				case COLUMN_EXACT_SIZE: return exactSize;
				case COLUMN_UPLOAD: return uuploadSpeed;
				case COLUMN_IP: return ip;
				case COLUMN_TTH: return getTTH();
				default: return Util::emptyString;
			}
		}

		static int compareItems(SearchInfo* a, SearchInfo* b, int col) {
			bool canBeSorted = false;
			if (a->mainitem && b->mainitem)
				if(a->collapsed && b->collapsed)
					canBeSorted = true;
			if (*a->sr->getTTH() == *b->sr->getTTH())
				if((!a->mainitem) && (!b->mainitem))
					canBeSorted = true;

			if(canBeSorted){
			switch(col) {
				case COLUMN_NICK:
					
					if( (a->getHits() == Util::toString((int)a->subItems.size()+1)+" "+STRING(HUB_USERS)) &&
						(b->getHits() == Util::toString((int)b->subItems.size()+1)+" "+STRING(HUB_USERS)))
							return compare(a->subItems.size()+1, b->subItems.size()+1);
					else
						return Util::stricmp(a->getHits(), b->getHits());
				case COLUMN_FILENAME: return Util::stricmp(a->fileName, b->fileName);
				case COLUMN_TYPE: 
					if(a->sr->getType() == b->sr->getType())
						return Util::stricmp(a->type, b->type);
					else
						return(a->sr->getType() == SearchResult::TYPE_DIRECTORY) ? -1 : 1;
				case COLUMN_SIZE: return compare(a->sr->getSize(), b->sr->getSize());
				case COLUMN_PATH: return Util::stricmp(a->path, b->path);
				case COLUMN_SLOTS: 
					if(a->sr->getFreeSlots() == b->sr->getFreeSlots())
						return compare(a->sr->getSlots(), b->sr->getSlots());
					else
						return compare(a->sr->getFreeSlots(), b->sr->getFreeSlots());
				case COLUMN_CONNECTION: return Util::stricmp(a->sr->getUser()->getConnection(), b->sr->getUser()->getConnection());
				case COLUMN_HUB: return Util::stricmp(a->sr->getHubName(), b->sr->getHubName());
				case COLUMN_EXACT_SIZE: return compare(a->sr->getSize(), b->sr->getSize());
				case COLUMN_UPLOAD: return compare(a->uuploadSpeed,b->uuploadSpeed);
				case COLUMN_IP: return Util::stricmp(a->getIP(), b->getIP());			
				case COLUMN_TTH: return Util::stricmp(a->getTTH(), b->getTTH());
				default: return 0;
			}
			} else return 0;
		}

		void update() { 
			if(sr->getType() == SearchResult::TYPE_FILE) {
				if(sr->getFile().rfind('\\') == string::npos) {
					fileName = sr->getFile();
				} else {
					fileName = Util::getFileName(sr->getFile());
					path = Util::getFilePath(sr->getFile());
				}

				type = Util::getFileExt(fileName);
				if(!type.empty() && type[0] == '.')
					type.erase(0, 1);
				size = Util::formatBytes(sr->getSize());
				exactSize = Util::formatNumber(sr->getSize());
			} else {
				fileName = sr->getFileName();
				path = sr->getFile();
				type = STRING(DIRECTORY);
			}
			slots = sr->getSlotString();
			ip = sr->getIP();
			if(sr->getTTH() != NULL)
				setTTH(sr->getTTH()->toBase32());

	if(user->getDownloadSpeed()<1)
		{
			string s1=sr->getUser()->getDescription();
			if((s1.find_last_of('@')==-1) || (s1.find('/')==-1)){
			const string& tmp = sr->getUser()->getConnection();
			int status = sr->getUser()->getStatus();
			string Omezeni = user->getUpload();

			if (!Omezeni.empty())
			 { s1=Util::formatBytes(Util::toInt64(Omezeni)*1024)+"/s";}
			else			
			if( (status == 8) || (status == 9)  || (status == 10) || (status == 11)) { s1 = ">=100 kB/s"; }
			else if(tmp == "28.8Kbps") { s1 = "*max. 2.1 kB/s"; }
			else if(tmp == "33.6Kbps") { s1 = "*max. 3 kB/s"; }
			else if(tmp == "56Kbps") { s1 = "*max. 4.2 kB/s"; }
			else if(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_MODEM]) { s1 = "*max. 6 kB/s"; }
			else if(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_ISDN]) { s1 = "*max. 10 kB/s"; }
			else { s1 = "N/A"; }
					}
				else 
			{
				
				s1.erase(0,s1.find_last_of('@')+1);
			}
			uuploadSpeed = s1;
		} else uuploadSpeed = Util::formatBytes(user->getDownloadSpeed())+"/s";
		}

		GETSET(string, fileName, FileName);
		GETSET(string, path, Path);
		GETSET(string, type, Type);
		GETSET(string, size, Size);
		GETSET(string, slots, Slots);
		GETSET(string, exactSize, ExactSize);
		GETSET(string, ip, IP);
		GETSET(string, tth, TTH);
		GETSET(string, uuploadSpeed, UuploadSpeed);
		GETSET(string, hits, Hits);
	};

	struct HubInfo : public FastAlloc<HubInfo> {
		HubInfo(const string& aIpPort, const string& aName, bool aOp) : ipPort(aIpPort),
			name(aName), op(aOp) { };

		const string& getText(int col) const {
			return (col == 0) ? name : Util::emptyString;
		}
		static int compareItems(HubInfo* a, HubInfo* b, int col) {
			return (col == 0) ? Util::stricmp(a->name, b->name) : 0;
		}
		string ipPort;
		string name;
		bool op;
	};

	// WM_SPEAKER
	enum Speakers {
		ADD_RESULT,
		HUB_ADDED,
		HUB_CHANGED,
		HUB_REMOVED,
	};

	string initialString;
	int64_t initialSize;
	SearchManager::SizeModes initialMode;
	SearchManager::TypeModes initialType;

	CStatusBarCtrl ctrlStatus;
	CEdit ctrlSearch;
	CComboBox ctrlSearchBox;
	CEdit ctrlSize;
	CComboBox ctrlMode;
	CComboBox ctrlSizeMode;
	CComboBox ctrlFiletype;
	CImageList searchTypes;
	CButton ctrlDoSearch;

	BOOL ListMeasure(HWND hwnd, UINT uCtrlId, MEASUREITEMSTRUCT *mis);
	BOOL ListDraw(HWND hwnd, UINT uCtrlId, DRAWITEMSTRUCT *dis);
	
	CContainedWindow searchContainer;
	CContainedWindow searchBoxContainer;
	CContainedWindow sizeContainer;
	CContainedWindow modeContainer;
	CContainedWindow sizeModeContainer;
	CContainedWindow fileTypeContainer;
	CContainedWindow slotsContainer;
	CContainedWindow tthContainer;
	CContainedWindow showUIContainer;
	CContainedWindow doSearchContainer;
	CContainedWindow resultsContainer;
	CContainedWindow hubsContainer;
	
	CStatic searchLabel, sizeLabel, optionLabel, typeLabel, hubsLabel;
	CButton ctrlSlots, ctrlShowUI, ctrlTTH;
	bool showUI;

	CImageList images;
	CImageList states;
	TypedListViewCtrlCleanup<SearchInfo, IDC_RESULTS> ctrlResults;
	TypedListViewCtrl<HubInfo, IDC_HUB> ctrlHubs;

	OMenu grantMenu;
	OMenu resultsMenu;
	OMenu targetMenu;
	OMenu targetDirMenu;
	OMenu copyMenu;
	
	StringList search;
	StringList targets;
	StringList wholeTargets;

	SearchInfo::List mainItems;

	/** Parameter map for user commands */
	StringMap ucParams;

	bool onlyFree;
	bool onlyTTH;
	bool isHash;

	CriticalSection cs;

	static StringList lastSearches;

	DWORD lastSearch;
	bool closed;

	static int columnIndexes[];
	static int columnSizes[];

	void downloadSelected(const string& aDir, bool view = false); 
	void downloadWholeSelected(const string& aDir);
	void onEnter();
	void onTab(bool shift);
	
	void download(SearchResult* aSR, const string& aDir, bool view);
	
	virtual void on(SearchManagerListener::SR, SearchResult* aResult) throw();

	// ClientManagerListener
	virtual void on(ClientConnected, Client* c) throw() { speak(HUB_ADDED, c); }
	virtual void on(ClientUpdated, Client* c) throw() { speak(HUB_CHANGED, c); }
	virtual void on(ClientDisconnected, Client* c) throw() { speak(HUB_REMOVED, c); }

	void initHubs();
	void onHubAdded(HubInfo* info);
	void onHubChanged(HubInfo* info);
	void onHubRemoved(HubInfo* info);

	void Collapse(SearchInfo* i, int a);
	void Expand(SearchInfo* i, int a);

	LRESULT onItemChangedHub(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	
	void speak(Speakers s, Client* aClient) {
		HubInfo* hubInfo = new HubInfo(aClient->getIpPort(), aClient->getName(), aClient->getOp());
		PostMessage(WM_SPEAKER, WPARAM(s), LPARAM(hubInfo)); 
	};
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHILDFRM_H__A7078724_FD85_4F39_8463_5A08A5F45E33__INCLUDED_)

/**
 * @file
 * $Id$
 */

