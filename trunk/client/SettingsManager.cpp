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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "SettingsManager.h"
#include "ResourceManager.h"

#include "SimpleXML.h"
#include "Util.h"
#include "File.h"
#include "HubManager.h"

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
	"DefaultAwayMessage", "TimeStampsFormat", "ADLSearchFrameOrder", "ADLSearchFrameWidths", 
	"FinishedULWidths", "FinishedULOrder", "CID", "SpyFrameWidths", "SpyFrameOrder", 
	"BeepFile", "BeginFile", "FinishedFile", "SourceFile", "UploadFile", "FakerFile", "ChatNameFile", "WinampFormat",
	"KickMsgRecent01", "KickMsgRecent02", "KickMsgRecent03", "KickMsgRecent04", "KickMsgRecent05", 
	"KickMsgRecent06", "KickMsgRecent07", "KickMsgRecent08", "KickMsgRecent09", "KickMsgRecent10", 
	"KickMsgRecent11", "KickMsgRecent12", "KickMsgRecent13", "KickMsgRecent14", "KickMsgRecent15", 
	"KickMsgRecent16", "KickMsgRecent17", "KickMsgRecent18", "KickMsgRecent19", "KickMsgRecent20",
	"OneSegmentExtensions", "Toolbar", "ToolbarImage", "ToolbarHot", "UserListImage",
	"UploadQueueFrameOrder", "UploadQueueFrameWidths", "DownSpeed", "UpSpeed",
	"MinBlockSize", "UpdateURL", "SoundTTH", "SoundException", "SoundHubConnected", "SoundHubDisconnected", "SoundFavUserOnline",
	"BackgroundImage", "WebServerLogFormat", "WebServerUser", "WebServerPass",
	"SENTRY", 
	// Ints
	"ConnectionType", "InPort", "Slots", "Rollforward", "AutoFollow", "ClearSearch",
	"BackgroundColor", "TextColor", "UseOemMonoFont", "ShareHidden", "FilterMessages", "MinimizeToTray", 
	"OpenPublic", "OpenQueue", "AutoSearch", "TimeStamps", "ConfirmExit", "IgnoreOffline", "PopupOffline", 
	"ListDuplicates", "BufferSize", "DownloadSlots", "MaxDownloadSpeed", "LogMainChat", "LogPrivateChat", 
	"LogDownloads", "LogUploads", "StatusInChat", "ShowJoins", "PrivateMessageBeep", "PrivateMessageBeepOpen", 
	"UseSystemIcons", "PopupPMs", "MinUploadSpeed", "GetUserInfo", "UrlHandler", "MainWindowState", 
	"MainWindowSizeX", "MainWindowSizeY", "MainWindowPosX", "MainWindowPosY", "AutoAway", 
	"SmallSendBuffer", "SocksPort", "SocksResolve", "KeepLists", "AutoKick", "QueueFrameShowTree", 
	"CompressTransfers", "ShowProgressBars", "SFVCheck", "MaxTabRows", "AutoUpdateList", 
	"MaxCompression", "FinishedDirty", "QueueDirty", "TabDirty", "AntiFrag", "MDIMaxmimized", "NoAwayMsgToBots", 
	"SkipZeroByte", "AdlsBreakOnFirst", "TabCompletion", "OpenFavoriteHubs", "OpenFinishedDownloads", 
	"HubUserCommands", "AutoSearchAutoMatch", "DownloadBarColor", "UploadBarColor", "LogSystem", 
	"LogFilelistTransfers", "EmptyWorkingSet",
	"ShowStatusbar",
	"ShowToolbar", "ShowTransferview", 
	"GetUpdateInfo", "SearchPassiveAlways", "SmallFileSize", "ShutdownInterval", 
	"CzertHiddenSettingA", "CzertHiddenSettingB", "ExtraSlots", 
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
	"ShowHubs", "ShowSlots", "ShowUpload", "ExpandQueue", "TransferSplitSize", "ShowIP", "ShowISP", "IDownSpeed", "HDownSpeed", "DownTime", 
	"ProgressOverrideColors", "Progress3DDepth", "ProgressOverrideColors2",
	"MenubarTwoColors", "MenubarLeftColor", "MenubarRightColor", "MenubarBumped", 
	"DisconnectingEnable", "MinFileSize", "RemoveSlowUser", "UploadQueueFrameShowTree",
	"SetAutoSegment", "SetMin2", "SetMax2", "SetMin3", "SetMax3",
	"SetMin4", "SetMax4", "SetMin6", "SetMax6", "SetMin8", "SetMaxSpeed",
	"SegmentsType", "NumberOfSegments", "PercentFakeShareTolerated", "MaxSources",
	"ClientEmulation", "ShowPK", "ShowLock", "ShowSupports", "UseEmoticons", "MaxEmoticons", "SendUnknownCommands", "Disconnect",
	"AutoUpdateIP", "CheckTTH", "MaxHashSpeed", "SearchTTHOnly", "MagnetHandler", "GetUserCountry",
	"DebugCommands", "AutoSaveQueue", "UseAutoPriorityByDefault", "UseOldSharingUI", "ShowDescriptionSpeed",
	"FavShowJoins", "LogStatusMessages", "ShowPMLog", "PMLogLines", "SearchAlternateColour", "SoundsDisabled",
	"ReportFoundAlternates", "CheckNewUsers", "GarbageIn", "GarbageOut", 
	"SearchTime", "DontBeginSegment", "DontBeginSegmentSpeed", "PopunderPm", "PopunderFilelist",
	"AutoDropSource", "DisplayCheatsInMainChat", "MagnetAsk", "MagnetAction",
	"DisconnectRaw", "TimeoutRaw", "FakeShareRaw", "ListLenMismatch", "FileListTooSmall", "FileListUnavailable",
	"AddFinishedInstantly", "Away", "UseUPnP", "UseCTRLForLineHistory",
	"PopupHubConnected", "PopupHubDisconnected", "PopupFavoriteConnected", "PopupCheatingUser", "PopupDownloadStart", 
	"PopupDownloadFailed", "PopupDownloadFinished", "PopupUploadFinished", "PopupPm", "PopupNewPM", 
	"PopupType", "WebServer", "WebServerPort", "WebServerLog", "ShutdownAction", "MinimumSearchInterval",
	"PopupAway", "PopupMinimized", "ShowShareCheckedUsers", "MaxAutoMatchSource",
    "ReservedSlotColor", "IgnoredColor", "FavoriteColor",
	"NormalColour", "ClientCheckedColour", "FileListCheckedColour",
	"FileListAndClientCheckedColour", "BadClientColour", "BadFilelistColour", "DontDLAlreadyShared", "RealTimeQueueUpdate",
	"ConfirmHubRemoval", "SuppressMainChat", "ProgressBackColor", "ProgressCompressColor", "ProgressSegmentColor",
	"SpeedUsers", "UseVerticalView", "OpenNewWindow", "FileSlots",  "UDPPort",
	"SENTRY",
	// Int64
	"TotalUpload", "TotalDownload",
	"SENTRY"
};

const string SettingsManager::connectionSpeeds[] = { "Modem", "ISDN", 
"Satellite", "Wireless", "Cable", "DSL", "LAN(T1)", "LAN(T3)" };

const string SettingsManager::clientEmulations[] = { "StrongDC++", "CZDC++", "DC++" };

const string SettingsManager::speeds[] = {"64K","128K","150K","192K",
"256K","384K","512K","600K","768K","1M","1.5M","2M","4M+" };

const string SettingsManager::blockSizes[] = { "64K", "128K", "256K", "512K", "1024K" };

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
	
	setDefault(DOWNLOAD_DIRECTORY, Util::getAppPath() + "Downloads" PATH_SEPARATOR_STR);
	setDefault(SLOTS, 1);
		//setDefault(SERVER, Util::getLocalIp());
	setDefault(SEARCH_TTH_ONLY, false);
	setDefault(CHECK_TTH, true);
	setDefault(IN_PORT, Util::rand(1025, 32000));
	setDefault(UDP_PORT, Util::rand(1025, 32000));
	setDefault(ROLLBACK, 0);
	setDefault(EMPTY_WORKING_SET, false);
	setDefault(MIN_BLOCK_SIZE, SettingsManager::blockSizes[SIZE_64]);
	setDefault(DONT_EXTENSIONS, "");
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
	setDefault(TIME_STAMPS, false);
	setDefault(CONFIRM_EXIT, false);
	setDefault(IGNORE_OFFLINE, false);
	setDefault(POPUP_OFFLINE, false);
	setDefault(LIST_DUPES, true);
	setDefault(BUFFER_SIZE, 64);
	setDefault(HUBLIST_SERVERS, "http://www.hublist.org/PublicHubList.xml.bz2;http://dc.selwerd.nl/hublist.xml.bz2");
	setDefault(DOWNLOAD_SLOTS, 50);
	setDefault(MAX_DOWNLOAD_SPEED, 0);
	setDefault(HUB_SLOTS, 0);
	setDefault(LOG_DIRECTORY, Util::getAppPath() + "Logs" PATH_SEPARATOR_STR);
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
	setDefault(TIME_STAMPS_FORMAT, "%H:%M");
	setDefault(MAX_TAB_ROWS, 2);
	setDefault(AUTO_UPDATE_LIST, true);
	setDefault(MAX_COMPRESSION, 6);
	setDefault(FINISHED_DIRTY, true);
	setDefault(QUEUE_DIRTY, true);
	setDefault(TAB_DIRTY, true);
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
	setDefault(AUTO_DROP_SOURCE, true);
	setDefault(DEBUG_COMMANDS, true);
	setDefault(MAX_HASH_SPEED, 0);
	setDefault(GET_USER_COUNTRY, true);
	setDefault(UPDATE_URL, "http://www.fin-overclocking.net/");
	setDefault(NORMAL_COLOUR, RGB(255,255,255));
	setDefault(RESERVED_SLOT_COLOR, RGB(255,51,255));
	setDefault(IGNORED_COLOR, RGB(192,192,192));	
	setDefault(FAVORITE_COLOR, RGB(51,51,255));	
	setDefault(CLIENT_CHECKED_COLOUR, RGB(160, 160, 160));
	setDefault(FILELIST_CHECKED_COLOUR, RGB(0, 160, 255));
	setDefault(FULL_CHECKED_COLOUR, RGB(0, 160, 0));
	setDefault(BAD_CLIENT_COLOUR, RGB(204,0,0));
	setDefault(BAD_FILELIST_COLOUR, RGB(204,0,204));	
	setDefault(FAV_SHOW_JOINS, false);
	setDefault(LOG_STATUS_MESSAGES, false);
	setDefault(SHOW_TRANSFERVIEW, true);
	setDefault(SHOW_STATUSBAR, true);
	setDefault(SHOW_TOOLBAR, true);
	setDefault(POPUNDER_PM, false);
	setDefault(POPUNDER_FILELIST, false);
	setDefault(MAGNET_ASK, true);
	setDefault(MAGNET_ACTION, MAGNET_AUTO_SEARCH);
	setDefault(ADD_FINISHED_INSTANTLY, false);
	setDefault(SETTINGS_USE_UPNP, false);
	setDefault(DONT_DL_ALREADY_SHARED, false);
	setDefault(CONFIRM_HUB_REMOVAL, false);
	setDefault(SETTINGS_USE_CTRL_FOR_LINE_HISTORY, true);
	setDefault(SETTINGS_OPEN_NEW_WINDOW, false);

	setDefault(EXTRA_SLOTS, 3);
	setDefault(SMALL_FILE_SIZE, 256);
	setDefault(SHUTDOWN_TIMEOUT, 150);
	setDefault(GET_UPDATE_INFO, true);
	setDefault(SEARCH_PASSIVE, false);
	setDefault(MAX_UPLOAD_SPEED_LIMIT_NORMAL, 0);
	setDefault(MAX_DOWNLOAD_SPEED_LIMIT_NORMAL, 0);
	setDefault(MAX_UPLOAD_SPEED_LIMIT_TIME, 0);
	setDefault(MAX_DOWNLOAD_SPEED_LIMIT_TIME, 0);
	setDefault(TOOLBAR, "0,-1,1,2,-1,3,4,5,-1,6,7,8,9,10,-1,11,12,13,-1,14,-1,15,-1,16,-1,17,-1,18,19,20,21,22");
	setDefault(SEARCH_ALTERNATE_COLOUR, RGB(255,200,0));
	setDefault(WEBSERVER, false);
	setDefault(WEBSERVER_PORT, 80);
	setDefault(WEBSERVER_FORMAT,"%Y-%m-%d %H:%M: %[ip] tried getting %[file]");
	setDefault(LOG_WEBSERVER, true);
	setDefault(WEBSERVER_USER, "strongdc");
	setDefault(WEBSERVER_PASS, "strongdc");
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
    setDefault(FILE_SLOTS, 15);
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

	setDefault(PERCENT_FAKE_SHARE_TOLERATED, 20);
	setDefault(MAX_SOURCES, 80);
	setDefault(CLIENT_EMULATION, CLIENT_STRONGDC);
	setDefault(USE_EMOTICONS, true);
	setDefault(MAX_EMOTICONS, 256);
	setDefault(MAGNET_URI_HANDLER, true);
	setDefault(REPORT_ALTERNATES, true);	
	setDefault(AUTOSAVE_QUEUE, 30);
	setDefault(USE_OLD_SHARING_UI, false);
	setDefault(SHOW_DESCRIPTION_SPEED, false);
	setDefault(SOUNDS_DISABLED, false);
	setDefault(CHECK_NEW_USERS, false);
	setDefault(GARBAGE_COMMAND_INCOMING, false);
	setDefault(GARBAGE_COMMAND_OUTGOING, false);
	setDefault(UPLOADQUEUEFRAME_SHOW_TREE, true);	
	setDefault(DONT_BEGIN_SEGMENT, true);
	setDefault(DONT_BEGIN_SEGMENT_SPEED, 200);

	setDefault(DISCONNECT_RAW, 0);
	setDefault(TIMEOUT_RAW, 0);
	setDefault(FAKESHARE_RAW, 0);
	setDefault(LISTLEN_MISMATCH, 0);
	setDefault(FILELIST_TOO_SMALL, 0);
	setDefault(FILELIST_UNAVAILABLE, 0);
	setDefault(USE_VERTICAL_VIEW, true);
	setDefault(DISPLAY_CHEATS_IN_MAIN_CHAT, true);	
	setDefault(SEARCH_TIME, 10);
	setDefault(REALTIME_QUEUE_UPDATE, true);
	setDefault(SUPPRESS_MAIN_CHAT, false);
	
	setDefault(BEGINFILE, "");
	setDefault(BEEPFILE, "");
	setDefault(FINISHFILE, "");
	setDefault(SOURCEFILE, "");
	setDefault(UPLOADFILE, "");
	setDefault(FAKERFILE, "");
	setDefault(CHATNAMEFILE, "");
	setDefault(SOUND_TTH, "");
	setDefault(SOUND_EXC, "");
	setDefault(SOUND_HUBCON, "");
	setDefault(SOUND_HUBDISCON, "");
	setDefault(SOUND_FAVUSER, "");

	setDefault(POPUP_HUB_CONNECTED, false);
	setDefault(POPUP_HUB_DISCONNECTED, false);
	setDefault(POPUP_FAVORITE_CONNECTED, true);
	setDefault(POPUP_CHEATING_USER, true);
	setDefault(POPUP_DOWNLOAD_START, true);
	setDefault(POPUP_DOWNLOAD_FAILED, false);
	setDefault(POPUP_DOWNLOAD_FINISHED, true);
	setDefault(POPUP_UPLOAD_FINISHED, false);
	setDefault(POPUP_PM, false);
	setDefault(POPUP_NEW_PM, true);
	setDefault(POPUP_TYPE, 0);
	setDefault(POPUP_AWAY, false);
	setDefault(POPUP_MINIMIZED, true);

	setDefault(AWAY, false);
	setDefault(SHUTDOWN_ACTION, 0);
	setDefault(MINIMUM_SEARCH_INTERVAL, 30);

	setDefault(BACKGROUND_IMAGE, "");
	setDefault(PROGRESS_3DDEPTH, 4);
	setDefault(MAX_AUTO_MATCH_SOURCES, 5);
	setDefault(SPEED_USERS, true);

#ifdef _WIN32
	setDefault(MAIN_WINDOW_STATE, SW_SHOWNORMAL);
	setDefault(MAIN_WINDOW_SIZE_X, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_SIZE_Y, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_POS_X, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_POS_Y, CW_USEDEFAULT);
	setDefault(MDI_MAXIMIZED, true);
	setDefault(UPLOAD_BAR_COLOR, RGB(150, 0, 0));
	setDefault(DOWNLOAD_BAR_COLOR, RGB(0, 150, 0));
	setDefault(PROGRESS_BACK_COLOR, RGB(95, 95, 95));
	setDefault(PROGRESS_COMPRESS_COLOR, RGB(222, 160, 0));
	setDefault(PROGRESS_SEGMENT_COLOR, RGB(55, 170, 85));
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
		HubManager::getInstance()->addPreviewApp("AVI Preview",Util::getAppPath() + "\\AVIPreview.exe","%[file]","avi;divx;mpg;mpeg");
		setTemp();
		return;
	}

	if(xmltext.empty()) {
		// Nothing to load...
		HubManager::getInstance()->addPreviewApp("AVI Preview",Util::getAppPath() + "\\AVIPreview.exe","%[file]","avi;divx;mpg;mpeg");
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
			
			for(i=STR_FIRST; i<STR_LAST; i++)
			{
				const string& attr = settingTags[i];
				dcassert(attr.find("SENTRY") == string::npos);
				
				if(xml.findChild(attr))
					set(StrSetting(i), xml.getChildData());
				xml.resetCurrentChild();
			}
			for(i=INT_FIRST; i<INT_LAST; i++)
			{
				const string& attr = settingTags[i];
				dcassert(attr.find("SENTRY") == string::npos);
				
				if(xml.findChild(attr))
					set(IntSetting(i), Util::toInt(xml.getChildData()));
				xml.resetCurrentChild();
			}
			for(i=INT64_FIRST; i<INT64_LAST; i++)
			{
				const string& attr = settingTags[i];
				dcassert(attr.find("SENTRY") == string::npos);
				
				if(xml.findChild(attr))
					set(Int64Setting(i), Util::toInt64(xml.getChildData()));
				xml.resetCurrentChild();
			}
			
			xml.stepOut();
		}
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
	
	fire(SettingsManagerListener::Save(), &xml);

	try {
		File out(aFileName + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		BufferedOutputStream<false> f(&out);
		f.write(SimpleXML::utf8Header);
		xml.toXML(&f);
		f.flush();
		out.close();
		File::deleteFile(aFileName);
		File::renameFile(aFileName + ".tmp", aFileName);
	} catch(const FileException&) {
		// ...
	}
}

/**
 * @file
 * $Id$
 */
