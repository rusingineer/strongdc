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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "SettingsManager.h"
#include "ResourceManager.h"

#include "SimpleXML.h"
#include "Util.h"
#include "File.h"
#include "PluginManager.h"
#include "StringTokenizer.h"

const string SettingsManager::settingTags[] =
{
	// Strings
	"Connection", "Description", "DownloadDirectory", "EMail", "Nick", "Server",
	"Font", "MainFrameOrder", "MainFrameWidths", "HubFrameOrder", "HubFrameWidths", 
	"RecentFrameOrder", "RecentFrameWidths", "FinishedMP3Order", "FinishedMP3Widths",
	"LanguageFile", "SearchFrameOrder", "SearchFrameWidths", "FavoritesFrameOrder", "FavoritesFrameWidths", 
	"HublistServers", "QueueFrameOrder", "QueueFrameWidths", "PublicHubsFrameOrder", "PublicHubsFrameWidths", 
	"UsersFrameOrder", "UsersFrameWidths", "HttpProxy", "LogDirectory", "NotepadText", "LogFormatPostDownload", 
	"LogFormatPostUpload", "LogFormatMainChat", "LogFormatPrivateChat", "FinishedOrder", "FinishedWidths",	 
	"TempDownloadDirectory", "SocksServer", "SocksUser", "SocksPassword", "ConfigVersion", 
	"DefaultAwayMessage", "ADLSearchFrameOrder", "ADLSearchFrameWidths", 
	"FinishedULWidths", "FinishedULOrder", 
	"BeepFile", "BeginFile", "FinishedFile", "SourceFile", "UploadFile", "FakerFile", "ChatNameFile", "WinampFormat",
	"KickMsgRecent01", "KickMsgRecent02", "KickMsgRecent03", "KickMsgRecent04", "KickMsgRecent05", 
	"KickMsgRecent06", "KickMsgRecent07", "KickMsgRecent08", "KickMsgRecent09", "KickMsgRecent10", 
	"KickMsgRecent11", "KickMsgRecent12", "KickMsgRecent13", "KickMsgRecent14", "KickMsgRecent15", 
	"KickMsgRecent16", "KickMsgRecent17", "KickMsgRecent18", "KickMsgRecent19", "KickMsgRecent20",
	"OneSegmentExtensions", "Toolbar", "ToolbarImage", "ToolbarHot", "UserListImage",
	"UploadQueueFrameOrder", "UploadQueueFrameWidths", "CID", "DownSpeed", "UpSpeed",
	"MinBlockSize", "UpdateURL", "SoundTTH", "SoundException",
	"SENTRY", 
	// Ints
	"ConnectionType", "InPort", "Slots", "Rollforward", "AutoFollow", "ClearSearch",
	"BackgroundColor", "TextColor", "UseOemMonoFont", "ShareHidden", "FilterMessages", "MinimizeToTray", 
	"OpenPublic", "OpenQueue", "AutoSearch", "AutoAutoSearchString", "TimeStamps", "ConfirmExit", "IgnoreOffline", "PopupOffline", 
	"ListDuplicates", "BufferSize", "DownloadSlots", "MaxDownloadSpeed", "LogMainChat", "LogPrivateChat", 
	"LogDownloads", "LogUploads", "StatusInChat", "ShowJoins", "PrivateMessageBeep", "PrivateMessageBeepOpen", 
	"UseSystemIcons", "PopupPMs", "MinUploadSpeed", "GetUserInfo", "UrlHandler", "MainWindowState", 
	"MainWindowSizeX", "MainWindowSizeY", "MainWindowPosX", "MainWindowPosY", "AutoAway", 
	"SmallSendBuffer", "SocksPort", "SocksResolve", "KeepLists", "AutoKick", "QueueFrameShowTree", 
	"CompressTransfers", "ShowProgressBars", "SFVCheck", "MaxTabRows", "AutoUpdateList", 
	"MaxCompression", "FinishedDirty", "QueueDirty", "AntiFrag", "MDIMaxmimized", "NoAwayMsgToBots", 
	"SkipZeroByte", "AdlsBreakOnFirst", "TabCompletion", "OpenFavoriteHubs", "OpenFinishedDownloads", 
	"HubUserCommands", "AutoSearchAutoMatch", "DownloadBarColor", "UploadBarColor", "LogSystem", 
	"LogFilelistTransfers", "EmptyWorkingSet", "SegmentColor", "ShowSegmentColor",
	"OptionalInfo", "GetUpdateInfo", "SearchPassiveAlways", "SmallFileSize", "ShutdownInterval", 
	"PopupFilelist", "CzertHiddenSettingA", "FilterSearchResults", "CzertHiddenSettingB", "ExtraSlots", 
	"TextGeneralBackColor", "TextGeneralForeColor", "TextGeneralBold", "TextGeneralItalic", 
	"TextMyOwnBackColor", "TextMyOwnForeColor", "TextMyOwnBold", "TextMyOwnItalic", 
	"TextPrivateBackColor", "TextPrivateForeColor", "TextPrivateBold", "TextPrivateItalic", 
	"TextSystemBackColor", "TextSystemForeColor", "TextSystemBold", "TextSystemItalic", 
	"TextServerBackColor", "TextServerForeColor", "TextServerBold", "TextServerItalic", 
	"TextTimestampBackColor", "TextTimestampForeColor", "TextTimestampBold", "TextTimestampItalic", 
	"TextMyNickBackColor", "TextMyNickForeColor", "TextMyNickBold", "TextMyNickItalic", 
	"TextFavBackColor", "TextFavForeColor", "TextFavBold", "TextFavItalic", 
	"TextOPBackColor", "TextOPForeColor", "TextOPBold", "TextOPItalic", 
	"TextURLBackColor", "TextURLForeColor", "TextURLBold", "TextURLItalic", 
	"BoldAuthorsMess", "UploadLimitNormal", "ThrottleEnable", "HubSlots", "DownloadLimitNormal", 
	"UploadLimitTime", "DownloadLimitTime", "TimeThrottle", "TimeLimitStart", "TimeLimitEnd",
	"RemoveForbidden", "ProgressTextDown", "ProgressTextUp", "ShowInfoTips", "ExtraDownloadSlots",
	"OpenNetworkStatistic", "MinimizeOnStratup", "ConfirmDelete", "DefaultSearchFreeSlots", 
	"ExtensionDownTo", "ErrorColor", "ShowShared", "ShowExactShared", "ShowDescription", 
	"ShowTag", "ShowConnection", "ShowEmail", "ShowClient", "ShowVersion", "ShowMode", 
	"ShowHubs", "ShowSlots", "ShowUpload", "ExpandQueue", "ShowTransfers", "ShowToolbar", 
	"ShowStatusBar", "TransferSplitSize", "ShowIP", "ShowISP", "IDownSpeed", "HDownSpeed", "DownTime", 
	"ProgressOverrideColors", "ProgressBumped", "ProgressOverrideColors2",
	"MenubarTwoColors", "MenubarLeftColor", "MenubarRightColor", "MenubarBumped", 
	"DisconnectingEnable", "MinFileSize", "RemoveSlowUser",
	"SetAutoSegment", "SetMin2", "SetMax2", "SetMin3", "SetMax3",
	"SetMin4", "SetMax4", "SetMin6", "SetMax6", "SetMin8", "SetMaxSpeed", "FloodCache",
	"SegmentsType", "NumberOfSegments", "PercentFakeShareTolerated", "IgnoreJunkFiles", "MaxSources",
	"ClientEmulation", "ShowPK", "ShowLock", "ShowSupports", "UseEmoticons", "MaxEmoticons", "SendUnknownCommands", "Disconnect",
	"AutoUpdateIP", "CheckTTH", "MaxHashSpeed", "SearchTTHOnly", "MagnetHandler", "GetUserCountry", "DisableCZDiacritic",
	"DebugCommands", "AutoSaveQueue", "UseAutoPriorityByDefault", "UseOldSharingUI", "ShowDescriptionSpeed",
	"FavShowJoins", "LogStatusMessages", "ShowPMLog", "PMLogLines", "SearchAlternateColour", "SoundsDisabled",
	"ReportFoundAlternates", "MemoryMappedFile",
	"SENTRY",
	// Int64
	"TotalUpload", "TotalDownload", "JunkFileSize", "JunkBINFileSize", "JunkVOBFileSize",
	"SENTRY"
};

const string SettingsManager::connectionSpeeds[] = { "Modem", "ISDN", 
"Satellite", "Wireless", "Cable", "DSL", "LAN(T1)", "LAN(T3)" };

const string SettingsManager::clientEmulations[] = { "StrongDC++", "CZDC++", "DC++" };

const string SettingsManager::speeds[] = {"64K","128K","150K","192K",
"256K","384K","512K","600K","768K","1M","1.5M","2M","4M+" };

const string SettingsManager::blockSizes[] = { "64K", "128K", "256K", "512K", "1024K", "Auto" };

SettingsManager::SettingsManager()
{
	for(int i=0; i<SETTINGS_LAST; i++)
		isSet[i] = false;

	for(int j=0; j<INT_LAST-INT_FIRST; j++) {
		intDefaults[j] = 0;
		intSettings[j] = 0;
	}
	for(int k=0; k<INT64_LAST-INT64_FIRST; k++) {
		int64Defaults[k] = 0;
		int64Settings[k] = 0;
	}
	
	setDefault(DOWNLOAD_DIRECTORY, Util::getAppPath() + "Downloads\\");
	setDefault(SLOTS, 1);
		//setDefault(SERVER, Util::getLocalIp());
	setDefault(SEARCH_TTH_ONLY, false);
	setDefault(CHECK_TTH, true);
	setDefault(IN_PORT, Util::rand(1025, 32000));
	setDefault(ROLLBACK, 1024);
	setDefault(EMPTY_WORKING_SET, true);
	setDefault(MIN_BLOCK_SIZE, SettingsManager::blockSizes[SIZE_AUTO]);
	setDefault(DONT_EXTENSIONS, "(.iso)|(.bin)|(.img)|(.r(ar)|[0-9]+)");
	setDefault(NUMBER_OF_SEGMENTS, 4);
	setDefault(SEGMENTS_TYPE, SEGMENT_ON_SIZE);
	setDefault(AUTO_FOLLOW, true);
	setDefault(CLEAR_SEARCH, true);
	setDefault(SHARE_HIDDEN, false);
	setDefault(FILTER_MESSAGES, true);
	setDefault(MINIMIZE_TRAY, false);
	setDefault(OPEN_PUBLIC, false);
	setDefault(OPEN_QUEUE, false);
	setDefault(AUTO_SEARCH, true);
	setDefault(AUTO_SEARCH_AUTO_STRING, false);
	setDefault(TIME_STAMPS, false);
	setDefault(CONFIRM_EXIT, false);
	setDefault(IGNORE_OFFLINE, false);
	setDefault(POPUP_OFFLINE, false);
	setDefault(LIST_DUPES, false);
	setDefault(BUFFER_SIZE, 64);
	setDefault(HUBLIST_SERVERS, "http://www.hublist.org/PublicHubList.xml.bz2");
	setDefault(DOWNLOAD_SLOTS, 50);
	setDefault(MAX_DOWNLOAD_SPEED, 0);
	setDefault(HUB_SLOTS, 0);
	setDefault(LOG_DIRECTORY, Util::getAppPath() + "Logs\\");
	setDefault(LOG_UPLOADS, false);
	setDefault(LOG_DOWNLOADS, false);
	setDefault(LOG_PRIVATE_CHAT, false);
	setDefault(LOG_MAIN_CHAT, false);
	setDefault(STATUS_IN_CHAT, true);
	setDefault(SHOW_JOINS, false);
	setDefault(CONNECTION, connectionSpeeds[0]);
	setDefault(PRIVATE_MESSAGE_BEEP, false);
	setDefault(PRIVATE_MESSAGE_BEEP_OPEN, false);
	setDefault(USE_SYSTEM_ICONS, true);
	setDefault(USE_OEM_MONOFONT, false);
	setDefault(POPUP_PMS, true);
	setDefault(MIN_UPLOAD_SPEED, 0);
	setDefault(LOG_FORMAT_POST_DOWNLOAD, "%Y-%m-%d %H:%M: %[target]" + STRING(DOWNLOADED_FROM) + "%[user], %[size] (%[chunksize]), %[speed], %[time]");
	setDefault(LOG_FORMAT_POST_UPLOAD, "%Y-%m-%d %H:%M: %[source]" + STRING(UPLOADED_TO) + "%[user], %[size] (%[chunksize]), %[speed], %[time]");
	setDefault(LOG_FORMAT_MAIN_CHAT, "[%Y-%m-%d %H:%M] %[message]");
	setDefault(LOG_FORMAT_PRIVATE_CHAT, "[%Y-%m-%d %H:%M] %[message]");
	setDefault(GET_USER_INFO, true);
	setDefault(URL_HANDLER, false);
	setDefault(AUTO_AWAY, false);
	setDefault(SMALL_SEND_BUFFER, false);
	setDefault(SOCKS_PORT, 1080);
	setDefault(SOCKS_RESOLVE, 1);
	setDefault(CONFIG_VERSION, "0.181");		// 0.181 is the last version missing configversion
	setDefault(KEEP_LISTS, false);
	setDefault(AUTO_KICK, false);
	setDefault(QUEUEFRAME_SHOW_TREE, true);
	setDefault(COMPRESS_TRANSFERS, true);
	setDefault(SHOW_PROGRESS_BARS, true);
	setDefault(SFV_CHECK, false);
	setDefault(DEFAULT_AWAY_MESSAGE, "I'm away. I might answer later if you're lucky.");
	setDefault(MAX_TAB_ROWS, 2);
	setDefault(AUTO_UPDATE_LIST, true);
	setDefault(MAX_COMPRESSION, 6);
	setDefault(FINISHED_DIRTY, true);
	setDefault(QUEUE_DIRTY, true);
	setDefault(ANTI_FRAG, false);
	setDefault(NO_AWAYMSG_TO_BOTS, true);
	setDefault(SKIP_ZERO_BYTE, false);
	setDefault(ADLS_BREAK_ON_FIRST, false);
	setDefault(TAB_COMPLETION, true);
	setDefault(OPEN_FAVORITE_HUBS, false);
	setDefault(OPEN_FINISHED_DOWNLOADS, false);
	setDefault(HUB_USER_COMMANDS, true);
	setDefault(AUTO_SEARCH_AUTO_MATCH, false);
	setDefault(LOG_FILELIST_TRANSFERS, true);
	setDefault(LOG_SYSTEM, false);
	setDefault(SEND_UNKNOWN_COMMANDS, true);
	setDefault(MEMORY_MAPPED_FILE, false);
	
	setDefault(DEBUG_COMMANDS, true);
	setDefault(MAX_HASH_SPEED, 0);
	setDefault(GET_USER_COUNTRY, true);
	setDefault(UPDATE_URL, "http://www.fin-overclocking.net/");
	setDefault(FAV_SHOW_JOINS, false);
	setDefault(LOG_STATUS_MESSAGES, false);
	setDefault(SHOW_TRANSFERS, true);
	setDefault(SHOW_STATUS_BAR, true);
	setDefault(SHOW_TOOLBAR, true);

	setDefault(EXTRA_SLOTS, 3);
	setDefault(SMALL_FILE_SIZE, 256);
	setDefault(SEND_EXTENDED_INFO, true);
	setDefault(SHUTDOWN_TIMEOUT, 150);
	setDefault(GET_UPDATE_INFO, true);
	setDefault(SEARCH_PASSIVE, false);
	setDefault(POPUP_FILELIST, true);
	setDefault(FILTER_SEARCH, true);
	setDefault(MAX_UPLOAD_SPEED_LIMIT_NORMAL, 0);
	setDefault(MAX_DOWNLOAD_SPEED_LIMIT_NORMAL, 0);
	setDefault(MAX_UPLOAD_SPEED_LIMIT_TIME, 0);
	setDefault(MAX_DOWNLOAD_SPEED_LIMIT_TIME, 0);
	setDefault(TOOLBAR, "0,-1,1,2,-1,3,4,5,-1,6,7,8,9,-1,10,11,12,-1,13,-1,14,-1,15,-1,16,-1,17,19,-1,18");
	setDefault(SEARCH_ALTERNATE_COLOUR, RGB(255,200,0));
	setDefault(AUTO_PRIORITY_DEFAULT, false);
	setDefault(TOOLBARIMAGE,"");
	setDefault(TOOLBARHOTIMAGE,"");
	setDefault(TIME_DEPENDENT_THROTTLE, false);
	setDefault(BANDWIDTH_LIMIT_START, 0);
	setDefault(BANDWIDTH_LIMIT_END, 0);
	setDefault(REMOVE_FORBIDDEN, true);
	setDefault(THROTTLE_ENABLE, false);
	setDefault(EXTRA_DOWNLOAD_SLOTS, 3);

	setDefault(BACKGROUND_COLOR, RGB(0,0,96));
	setDefault(TEXT_COLOR, RGB(255,255,255));

	setDefault(TEXT_GENERAL_BACK_COLOR, RGB(0,0,96));
	setDefault(TEXT_GENERAL_FORE_COLOR, RGB(255,255,255));
	setDefault(TEXT_GENERAL_BOLD, false);
	setDefault(TEXT_GENERAL_ITALIC, false);

	setDefault(TEXT_MYOWN_BACK_COLOR, RGB(96,0,0));
	setDefault(TEXT_MYOWN_FORE_COLOR, RGB(255,255,0));
	setDefault(TEXT_MYOWN_BOLD, true);
	setDefault(TEXT_MYOWN_ITALIC, false);

	setDefault(TEXT_PRIVATE_BACK_COLOR, RGB(0,96,0));
	setDefault(TEXT_PRIVATE_FORE_COLOR, RGB(255,255,255));
	setDefault(TEXT_PRIVATE_BOLD, true);
	setDefault(TEXT_PRIVATE_ITALIC, false);

	setDefault(TEXT_SYSTEM_BACK_COLOR, RGB(0,0,0));
	setDefault(TEXT_SYSTEM_FORE_COLOR, RGB(192,192,192));
	setDefault(TEXT_SYSTEM_BOLD, true);
	setDefault(TEXT_SYSTEM_ITALIC, true);

	setDefault(TEXT_SERVER_BACK_COLOR, RGB(0,0,0));
	setDefault(TEXT_SERVER_FORE_COLOR, RGB(128,255,128));
	setDefault(TEXT_SERVER_BOLD, true);
	setDefault(TEXT_SERVER_ITALIC, false);

	setDefault(TEXT_TIMESTAMP_BACK_COLOR, RGB(0,0,0));
	setDefault(TEXT_TIMESTAMP_FORE_COLOR, RGB(255,255,0));
	setDefault(TEXT_TIMESTAMP_BOLD, false);
	setDefault(TEXT_TIMESTAMP_ITALIC, false);

	setDefault(TEXT_MYNICK_BACK_COLOR, RGB(0,0,96));
	setDefault(TEXT_MYNICK_FORE_COLOR, RGB(255,255,0));
	setDefault(TEXT_MYNICK_BOLD, true);
	setDefault(TEXT_MYNICK_ITALIC, false);

	setDefault(TEXT_FAV_BACK_COLOR, RGB(0,0,0));
	setDefault(TEXT_FAV_FORE_COLOR, RGB(255,128,128));
	setDefault(TEXT_FAV_BOLD, true);
	setDefault(TEXT_FAV_ITALIC, true);

	setDefault(TEXT_OP_BACK_COLOR, RGB(0,0,96));
	setDefault(TEXT_OP_FORE_COLOR, RGB(200,0,0));
	setDefault(TEXT_OP_BOLD, true);
	setDefault(TEXT_OP_ITALIC, false);

	setDefault(TEXT_URL_BACK_COLOR, RGB(192,192,192));
	setDefault(TEXT_URL_FORE_COLOR, RGB(0,0,255));
	setDefault(TEXT_URL_BOLD, false);
	setDefault(TEXT_URL_ITALIC, false);

	setDefault(BOLD_AUTHOR_MESS, true);
	setDefault(KICK_MSG_RECENT_01, "");
	setDefault(KICK_MSG_RECENT_02, "");
	setDefault(KICK_MSG_RECENT_03, "");
	setDefault(KICK_MSG_RECENT_04, "");
	setDefault(KICK_MSG_RECENT_05, "");
	setDefault(KICK_MSG_RECENT_06, "");
	setDefault(KICK_MSG_RECENT_07, "");
	setDefault(KICK_MSG_RECENT_08, "");
	setDefault(KICK_MSG_RECENT_09, "");
	setDefault(KICK_MSG_RECENT_10, "");
	setDefault(KICK_MSG_RECENT_11, "");
	setDefault(KICK_MSG_RECENT_12, "");
	setDefault(KICK_MSG_RECENT_13, "");
	setDefault(KICK_MSG_RECENT_14, "");
	setDefault(KICK_MSG_RECENT_15, "");
	setDefault(KICK_MSG_RECENT_16, "");
	setDefault(KICK_MSG_RECENT_17, "");
	setDefault(KICK_MSG_RECENT_18, "");
	setDefault(KICK_MSG_RECENT_19, "");
	setDefault(KICK_MSG_RECENT_20, "");
	setDefault(WINAMP_FORMAT, "winamp(%[version]) %[state](%[title]) stats(%[percent] of %[length] %[bar])");
	setDefault(PROGRESS_TEXT_COLOR_DOWN, RGB(255, 255, 255));
	setDefault(PROGRESS_TEXT_COLOR_UP, RGB(255, 255, 255));
	setDefault(SHOW_INFOTIPS, true);
	setDefault(OPEN_NETWORK_STATISTIC, false);
	setDefault(MINIMIZE_ON_STARTUP, false);
	setDefault(CONFIRM_DELETE, true);
	setDefault(FREE_SLOTS_DEFAULT, false);
	setDefault(USE_EXTENSION_DOWNTO, true);
	setDefault(ERROR_COLOR, RGB(255, 0, 0));
	setDefault(SHOW_SHARED, true);
	setDefault(SHOW_EXACT_SHARED, false);
	setDefault(SHOW_DESCRIPTION, true);
	setDefault(SHOW_TAG, false);
	setDefault(SHOW_CONNECTION, true);
	setDefault(SHOW_EMAIL, true);
	setDefault(SHOW_CLIENT, false);
	setDefault(SHOW_VERSION, false);
	setDefault(SHOW_MODE, false);
	setDefault(SHOW_HUBS, false);
	setDefault(SHOW_SLOTS, false);
	setDefault(SHOW_UPLOAD, false);
	setDefault(SHOW_IP, false);
	setDefault(SHOW_ISP, false);
	setDefault(SHOW_SUPPORTS, false);
	setDefault(SHOW_PK, false);
	setDefault(SHOW_LOCK, false);

	setDefault(EXPAND_QUEUE, true);
	setDefault(TRANSFER_SPLIT_SIZE, 8000);
	setDefault(I_DOWN_SPEED, 0);
	setDefault(H_DOWN_SPEED, 0);
	setDefault(DOWN_TIME, 1);
	setDefault(DISCONNECTING_ENABLE, false);
	setDefault(MIN_FILE_SIZE, 0);
	setDefault(REMOVE_SLOW_USER, false);
	setDefault(SHOW_PM_LOG, true);
	setDefault(PM_LOG_LINES, 10);
    
	setDefault(SET_AUTO_SEGMENT, true);
	setDefault(SET_MIN2, 20);
	setDefault(SET_MAX2, 40);
	setDefault(SET_MIN3, 40);
	setDefault(SET_MAX3, 60);
	setDefault(SET_MIN4, 60);
	setDefault(SET_MAX4, 80);
	setDefault(SET_MIN6, 80);
	setDefault(SET_MAX6, 100);
	setDefault(SET_MIN8, 100);
	setDefault(SET_MAXSPEED, 40);
	setDefault(MENUBAR_TWO_COLORS, true);
	setDefault(MENUBAR_LEFT_COLOR, RGB(254, 157, 27));
	setDefault(MENUBAR_RIGHT_COLOR, RGB(194, 78, 7));
	setDefault(MENUBAR_BUMPED, true);

	setDefault(JUNK_FILE_SIZE, (int64_t)(1610612736));
	setDefault(JUNK_BIN_FILE_SIZE, (int64_t)(1610612736));
	setDefault(JUNK_VOB_FILE_SIZE, (int64_t)(1610612736));
	setDefault(PERCENT_FAKE_SHARE_TOLERATED, 20);
	setDefault(MAX_SOURCES, 40);
	setDefault(CLIENT_EMULATION, CLIENT_STRONGDC);
	setDefault(USE_EMOTICONS, true);
	setDefault(MAX_EMOTICONS, 256);
	setDefault(CZCHARS_DISABLE, true);
	setDefault(MAGNET_URI_HANDLER, true);
	setDefault(REPORT_ALTERNATES, true);	
	setDefault(AUTOSAVE_QUEUE, 30);
	setDefault(SHOW_SEGMENT_COLOR, true);
	setDefault(USE_OLD_SHARING_UI, false);
	setDefault(SHOW_DESCRIPTION_SPEED, false);
	setDefault(SOUNDS_DISABLED, false);
#ifdef _WIN32
	setDefault(MAIN_WINDOW_STATE, SW_SHOWNORMAL);
	setDefault(MAIN_WINDOW_SIZE_X, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_SIZE_Y, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_POS_X, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_POS_Y, CW_USEDEFAULT);
	setDefault(MDI_MAXIMIZED, true);
	setDefault(UPLOAD_BAR_COLOR, RGB(205, 60, 55));
	setDefault(DOWNLOAD_BAR_COLOR, RGB(55, 170, 85));
	setDefault(SEGMENT_BAR_COLOR, RGB(10, 74, 138));
#endif
}

void SettingsManager::load(string const& aFileName)
{
	string xmltext;
	try {
		File f(aFileName, File::READ, File::OPEN);
		xmltext = f.read();		
	} catch(const FileException&) {
		// ...
		PluginManager::getInstance()->addPreviewApp("AVI Preview",Util::getAppPath() + "\\AVIPreview.exe","%[file]","avi;divx;mpg;mpeg");
		setTemp();
		return;
	}

	if(xmltext.empty()) {
		// Nothing to load...
		PluginManager::getInstance()->addPreviewApp("AVI Preview",Util::getAppPath() + "\\AVIPreview.exe","%[file]","avi;divx;mpg;mpeg");
		setTemp();
		return;
	}

	try {
		SimpleXML xml;
		xml.fromXML(xmltext);
		
		xml.resetCurrentChild();
		
		xml.stepIn();
		
		if(xml.findChild("Settings"))
		{
			xml.stepIn();
			
			int i;
			string attr;
			
			for(i=STR_FIRST; i<STR_LAST; i++)
			{
				attr = settingTags[i];
				dcassert(attr.find("SENTRY") == string::npos);
				
				if(xml.findChild(attr))
					set(StrSetting(i), xml.getChildData());
				xml.resetCurrentChild();
			}
			for(i=INT_FIRST; i<INT_LAST; i++)
			{
				attr = settingTags[i];
				dcassert(attr.find("SENTRY") == string::npos);
				
				if(xml.findChild(attr))
					set(IntSetting(i), Util::toInt(xml.getChildData()));
				xml.resetCurrentChild();
			}
			for(i=INT64_FIRST; i<INT64_LAST; i++)
			{
				attr = settingTags[i];
				dcassert(attr.find("SENTRY") == string::npos);
				
				if(xml.findChild(attr))
					set(Int64Setting(i), Util::toInt64(xml.getChildData()));
				xml.resetCurrentChild();
			}
			
			xml.stepOut();
		}

		if(xml.findChild("DownloadDirectories")) {
			xml.stepIn();

			while(xml.findChild("Dir")) {
				DownloadDirectory d = {xml.getChildData(), xml.getChildAttrib("ext"), xml.getChildAttrib("name")};				
				DownloadDirectories.push_back(d);
			}

			xml.stepOut();
		} 
		xml.resetCurrentChild();	

		if(xml.findChild("NeverDownload")) {
			xml.stepIn();

			while(xml.findChild("User")) {
				NeverDownloadFrom(xml.getChildData());
			}

			xml.stepOut();
		} 
		xml.resetCurrentChild();

		fire(SettingsManagerListener::Load(), &xml);

		xml.stepOut();

		setTemp();
	} catch(const Exception&) {
		// Oops, bad...
	}
}

void SettingsManager::save(string const& aFileName) {

	SimpleXML xml;
	xml.addTag("DCPlusPlus");
	xml.stepIn();
	xml.addTag("Settings");
	xml.stepIn();

	int i;
	string type("type"), curType("string");
	
	for(i=STR_FIRST; i<STR_LAST; i++)
	{
		if(i == CONFIG_VERSION) {
			xml.addTag(settingTags[i], VERSIONSTRING);
			xml.addChildAttrib(type, curType);
		} else if(isSet[i]) {
			xml.addTag(settingTags[i], get(StrSetting(i), false));
			xml.addChildAttrib(type, curType);
		}
	}

	curType = "int";
	for(i=INT_FIRST; i<INT_LAST; i++)
	{
		if(isSet[i]) {
			xml.addTag(settingTags[i], get(IntSetting(i), false));
			xml.addChildAttrib(type, curType);
		}
	}
	curType = "int64";
	for(i=INT64_FIRST; i<INT64_LAST; i++)
	{
		if(isSet[i])
		{
			xml.addTag(settingTags[i], get(Int64Setting(i), false));
			xml.addChildAttrib(type, curType);
		}
	}
	xml.stepOut();
	
	xml.addTag("DownloadDirectories");
	xml.stepIn();

	for(SettingsManager::DDList::iterator i = DownloadDirectories.begin(); i != DownloadDirectories.end(); ++i) {
		xml.addTag("Dir", i->dir);
		xml.addChildAttrib("ext", i->ext);
		xml.addChildAttrib("name", i->name);
	}
	xml.stepOut();

	xml.addTag("NeverDownload");
	xml.stepIn();

	for(vector<string>::iterator i = NeverDownload.begin(); i != NeverDownload.end(); ++i) {
		xml.addTag("User", *i);
	}
	xml.stepOut();

	fire(SettingsManagerListener::Save(), &xml);

	try {
		File ff(aFileName + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		BufferedOutputStream<false> f(&ff);
		f.write(SimpleXML::w1252Header);
		xml.toXML(&f);
		f.flush();
		ff.close();
		File::deleteFile(aFileName);
		File::renameFile(aFileName + ".tmp", aFileName);
	} catch(const FileException&) {
		// ...
	}
}

string SettingsManager::getDownloadDir(string ext){
	if(ext.size() > 1) {
		for(SettingsManager::DDList::iterator i = DownloadDirectories.begin(); i != DownloadDirectories.end(); ++i) {
			StringList tok = StringTokenizer(i->ext, ';').getTokens();
			for(StringList::iterator j = tok.begin(); j != tok.end(); ++j) {
				if(Util::stricmp(ext.substr(1), (*j)) == 0) 
					return i->dir;			
			}
		}
	}
	return SETTING(DOWNLOAD_DIRECTORY);
}

/**
 * @file
 * $Id$
 */
