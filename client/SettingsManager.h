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

#if !defined(SETTINGSMANAGER_H)
#define SETTINGSMANAGER_H

#include "Util.h"
#include "Speaker.h"
#include "Singleton.h"

class SimpleXML;

class SettingsManagerListener {
public:
	typedef SettingsManagerListener* Ptr;
	typedef vector<Ptr> List;
	typedef List::iterator Iter;
	
	enum Types {
		LOAD,
		SAVE
	};
	
	virtual void onAction(Types, SimpleXML*) throw() = 0;
};

class SettingsManager : public Singleton<SettingsManager>, public Speaker<SettingsManagerListener>
{
public:

	static const string connectionSpeeds[];
	static const string clientEmulations[];

	enum StrSetting { STR_FIRST,
		CONNECTION = STR_FIRST, DESCRIPTION, DOWNLOAD_DIRECTORY, EMAIL, NICK, SERVER,
		CLIENTVERSION, TEXT_FONT, MAINFRAME_ORDER, MAINFRAME_WIDTHS, HUBFRAME_ORDER, HUBFRAME_WIDTHS,
		RECENTFRAME_ORDER, RECENTFRAME_WIDTHS, FINISHEDMP3_ORDER, FINISHEDMP3_WIDTHS,
		LANGUAGE_FILE, SEARCHFRAME_ORDER, SEARCHFRAME_WIDTHS, FAVORITESFRAME_ORDER, FAVORITESFRAME_WIDTHS, 
		HUBLIST_SERVERS, QUEUEFRAME_ORDER, QUEUEFRAME_WIDTHS, PUBLICHUBSFRAME_ORDER, PUBLICHUBSFRAME_WIDTHS, 
		USERSFRAME_ORDER, USERSFRAME_WIDTHS, HTTP_PROXY, LOG_DIRECTORY, NOTEPAD_TEXT, LOG_FORMAT_POST_DOWNLOAD, 
		LOG_FORMAT_POST_UPLOAD, LOG_FORMAT_MAIN_CHAT, LOG_FORMAT_PRIVATE_CHAT, FINISHED_ORDER, FINISHED_WIDTHS, 
		TEMP_DOWNLOAD_DIRECTORY, SOCKS_SERVER, SOCKS_USER, SOCKS_PASSWORD, CONFIG_VERSION,
		DEFAULT_AWAY_MESSAGE, ADLSEARCHFRAME_ORDER, ADLSEARCHFRAME_WIDTHS, 
		FINISHED_UL_WIDTHS, FINISHED_UL_ORDER,
		BEEPFILE, BEGINFILE, FINISHFILE, SOURCEFILE, UPLOADFILE, FAKERFILE, CHATNAMEFILE, WINAMP_FORMAT,
		KICK_MSG_RECENT_01, KICK_MSG_RECENT_02, KICK_MSG_RECENT_03, KICK_MSG_RECENT_04, KICK_MSG_RECENT_05, 
		KICK_MSG_RECENT_06, KICK_MSG_RECENT_07, KICK_MSG_RECENT_08, KICK_MSG_RECENT_09, KICK_MSG_RECENT_10, 
		KICK_MSG_RECENT_11, KICK_MSG_RECENT_12, KICK_MSG_RECENT_13, KICK_MSG_RECENT_14, KICK_MSG_RECENT_15, 
		KICK_MSG_RECENT_16, KICK_MSG_RECENT_17, KICK_MSG_RECENT_18, KICK_MSG_RECENT_19, KICK_MSG_RECENT_20,
		DONT_EXTENSIONS, TOOLBAR, TOOLBARIMAGE, TOOLBARHOTIMAGE, USERLIST_IMAGE,
		UPLOADQUEUEFRAME_ORDER, UPLOADQUEUEFRAME_WIDTHS,
		STR_LAST };

	enum IntSetting { INT_FIRST = STR_LAST + 1,
		CONNECTION_TYPE = INT_FIRST, IN_PORT, SLOTS, ROLLBACK, AUTO_FOLLOW, CLEAR_SEARCH, FULL_ROW_SELECT, 
		BACKGROUND_COLOR, TEXT_COLOR, USE_OEM_MONOFONT, SHARE_HIDDEN, FILTER_MESSAGES, MINIMIZE_TRAY,
		OPEN_PUBLIC, OPEN_QUEUE, AUTO_SEARCH, AUTO_SEARCH_AUTO_STRING, TIME_STAMPS, CONFIRM_EXIT, IGNORE_OFFLINE, POPUP_OFFLINE,
		LIST_DUPES, BUFFER_SIZE, DOWNLOAD_SLOTS, MAX_DOWNLOAD_SPEED, LOG_MAIN_CHAT, LOG_PRIVATE_CHAT,
		LOG_DOWNLOADS, LOG_UPLOADS, STATUS_IN_CHAT, SHOW_JOINS, PRIVATE_MESSAGE_BEEP, PRIVATE_MESSAGE_BEEP_OPEN,
		USE_SYSTEM_ICONS, POPUP_PMS, MIN_UPLOAD_SPEED, GET_USER_INFO, URL_HANDLER, MAIN_WINDOW_STATE,
		MAIN_WINDOW_SIZE_X, MAIN_WINDOW_SIZE_Y, MAIN_WINDOW_POS_X, MAIN_WINDOW_POS_Y, AUTO_AWAY,
		SMALL_SEND_BUFFER, SOCKS_PORT, SOCKS_RESOLVE, KEEP_LISTS, AUTO_KICK, QUEUEFRAME_SHOW_TREE,
		COMPRESS_TRANSFERS, SHOW_PROGRESS_BARS, SFV_CHECK, MAX_TAB_ROWS, AUTO_UPDATE_LIST,
		MAX_COMPRESSION, FINISHED_DIRTY, QUEUE_DIRTY, ANTI_FRAG, MDI_MAXIMIZED, NO_AWAYMSG_TO_BOTS,
		SKIP_ZERO_BYTE, ADLS_BREAK_ON_FIRST, TAB_COMPLETION, OPEN_FAVORITE_HUBS, OPEN_FINISHED_DOWNLOADS, 
		HUB_USER_COMMANDS, AUTO_SEARCH_AUTO_MATCH, DOWNLOAD_BAR_COLOR, UPLOAD_BAR_COLOR, LOG_SYSTEM, 
		LOG_FILELIST_TRANSFERS, EMPTY_WORKING_SET,
		SEND_EXTENDED_INFO, GET_UPDATE_INFO, SEARCH_PASSIVE, SMALL_FILE_SIZE, SHUTDOWN_TIMEOUT, 
		POPUP_FILELIST, MAX_UPLOAD_SPEED_LIMIT, FILTER_SEARCH, MAX_DOWNLOAD_SPEED_LIMIT, EXTRA_SLOTS, 
		TEXT_GENERAL_BACK_COLOR, TEXT_GENERAL_FORE_COLOR, TEXT_GENERAL_BOLD, TEXT_GENERAL_ITALIC, 
		TEXT_MYOWN_BACK_COLOR, TEXT_MYOWN_FORE_COLOR, TEXT_MYOWN_BOLD, TEXT_MYOWN_ITALIC, 
		TEXT_PRIVATE_BACK_COLOR, TEXT_PRIVATE_FORE_COLOR, TEXT_PRIVATE_BOLD, TEXT_PRIVATE_ITALIC, 
		TEXT_SYSTEM_BACK_COLOR, TEXT_SYSTEM_FORE_COLOR, TEXT_SYSTEM_BOLD, TEXT_SYSTEM_ITALIC, 
		TEXT_SERVER_BACK_COLOR, TEXT_SERVER_FORE_COLOR, TEXT_SERVER_BOLD, TEXT_SERVER_ITALIC, 
		TEXT_TIMESTAMP_BACK_COLOR, TEXT_TIMESTAMP_FORE_COLOR, TEXT_TIMESTAMP_BOLD, TEXT_TIMESTAMP_ITALIC, 
		TEXT_MYNICK_BACK_COLOR, TEXT_MYNICK_FORE_COLOR, TEXT_MYNICK_BOLD, TEXT_MYNICK_ITALIC, 
		TEXT_FAV_BACK_COLOR, TEXT_FAV_FORE_COLOR, TEXT_FAV_BOLD, TEXT_FAV_ITALIC, 
		TEXT_OP_BACK_COLOR, TEXT_OP_FORE_COLOR, TEXT_OP_BOLD, TEXT_OP_ITALIC,
		TEXT_URL_BACK_COLOR, TEXT_URL_FORE_COLOR, TEXT_URL_BOLD, TEXT_URL_ITALIC, 
		BOLD_AUTHOR_MESS, MAX_UPLOAD_SPEED_LIMIT_NORMAL, THROTTLE_ENABLE, HUB_SLOTS,
		MAX_DOWNLOAD_SPEED_LIMIT_NORMAL, MAX_UPLOAD_SPEED_LIMIT_TIME, MAX_DOWNLOAD_SPEED_LIMIT_TIME, 
		TIME_DEPENDENT_THROTTLE, BANDWIDTH_LIMIT_START, BANDWIDTH_LIMIT_END, REMOVE_FORBIDDEN, 
		PROGRESS_TEXT_COLOR_DOWN, PROGRESS_TEXT_COLOR_UP, SHOW_INFOTIPS, EXTRA_DOWNLOAD_SLOTS, 
		OPEN_NETWORK_STATISTIC, MINIMIZE_ON_STARTUP, CONFIRM_DELETE, FREE_SLOTS_DEFAULT, 
		USE_EXTENSION_DOWNTO, ERROR_COLOR, SHOW_SHARED, SHOW_EXACT_SHARED, SHOW_DESCRIPTION, SHOW_TAG, 
		SHOW_CONNECTION, SHOW_EMAIL, SHOW_CLIENT, SHOW_VERSION, SHOW_MODE, SHOW_HUBS, SHOW_SLOTS, 
		SHOW_UPLOAD, EXPAND_QUEUE, SHOW_TRANSFERS, SHOW_TOOLBAR, SHOW_STATUS_BAR, TRANSFER_SPLIT_SIZE, 
		SHOW_IP, SHOW_ISP, I_DOWN_SPEED, H_DOWN_SPEED, DOWN_TIME, 
		PROGRESS_OVERRIDE_COLORS, PROGRESS_BUMPED, PROGRESS_OVERRIDE_COLORS2,
	MENUBAR_TWO_COLORS, MENUBAR_LEFT_COLOR, MENUBAR_RIGHT_COLOR, MENUBAR_BUMPED, 
	DISCONNECTING_ENABLE, MIN_FILE_SIZE, REMOVE_SLOW_USER,
	SET_AUTO_SEGMENT, SET_MIN2, SET_MAX2, SET_MIN3, SET_MAX3,
	SET_MIN4, SET_MAX4, SET_MIN6, SET_MAX6, SET_MIN8, SET_MAXSPEED, FLOOD_CACHE, AUTO_SEARCH_EXACT,
	SEGMENTS_TYPE, NUMBER_OF_SEGMENTS, MIN_BLOCK_SIZE, PERCENT_FAKE_SHARE_TOLERATED, IGNORE_JUNK_FILES, MAX_SOURCES,
	CLIENT_EMULATION, SHOW_PK, SHOW_LOCK, SHOW_SUPPORTS, USE_EMOTICONS, SEND_UNKNOWN_COMMANDS, DISCONNECT,
	IPUPDATE, CHECK_TTH, MAX_HASH_SPEED, SEARCH_TTH_ONLY, MAGNET_URI_HANDLER,
		INT_LAST };

	enum Int64Setting { INT64_FIRST = INT_LAST + 1,
		TOTAL_UPLOAD = INT64_FIRST, TOTAL_DOWNLOAD, JUNK_FILE_SIZE, JUNK_BIN_FILE_SIZE, JUNK_VOB_FILE_SIZE, INT64_LAST, SETTINGS_LAST = INT64_LAST };

	enum {	SPEED_MODEM, SPEED_ISDN, SPEED_SATELLITE, SPEED_WIRELESS, SPEED_CABLE,
			SPEED_DSL, SPEED_T1, SPEED_T3, SPEED_LAST };
	
	enum {	CONNECTION_ACTIVE, CONNECTION_PASSIVE, CONNECTION_SOCKS5 };

	enum { SEGMENT_ON_SIZE, SEGMENT_ON_CONNECTION, SEGMENT_MANUAL };

	enum { CLIENT_STRONGDC, CLIENT_CZDC, CLIENT_DC, CLIENT_EMULATION_LAST };

	enum { COMPRESSION_NONE, COMPRESSION_DOWN, COMPRESSION_UP, COMPRESSION_BOTH, COMPRESSION_LAST };

	const string& get(StrSetting key, bool useDefault = true) const {
		return (isSet[key] || !useDefault) ? strSettings[key - STR_FIRST] : strDefaults[key - STR_FIRST];
	}

	int get(IntSetting key, bool useDefault = true) const {
		return (isSet[key] || !useDefault) ? intSettings[key - INT_FIRST] : intDefaults[key - INT_FIRST];
	}
	int64_t get(Int64Setting key, bool useDefault = true) const {
		return (isSet[key] || !useDefault) ? int64Settings[key - INT64_FIRST] : int64Defaults[key - INT64_FIRST];
	}

	bool getBool(IntSetting key, bool useDefault = true) const {
		return (get(key, useDefault) != 0);
	}

	void set(StrSetting key, string const& value) {
		if ( (key == NICK) && (value.size() > 35) ) {
			strSettings[key - STR_FIRST] = value.substr(0, 35);
		} else if ( (key == DESCRIPTION) && (value.size() > 50) ) {
			strSettings[key - STR_FIRST] = value.substr(0, 50);
		} else {
			strSettings[key - STR_FIRST] = value;
		}
		isSet[key] = !value.empty();
	}

	int slots;
	void set(IntSetting key, int value) {
		if((key == SLOTS) && (value <= 0)) {
			value = 1;
		}
		if((key == SLOTS) ) {
			slots = value;
		}
		if((key == EXTRA_SLOTS) && (value < 3)) {
			value = 3;
		}

		if((key == SMALL_FILE_SIZE) && (value < 64)) {
			value = 64;
		}

		if((key == MAX_UPLOAD_SPEED_LIMIT_NORMAL) && (value > 0)) {
			if (value < ((2 * (slots)) + 3) ) {
				value = ((2 * (slots)) + 3);
			}
		}
		if((key == MAX_UPLOAD_SPEED_LIMIT_TIME) && (value > 0)) {
			if (value < ((2 * (slots)) + 3) ) {
				value = ((2 * (slots)) + 3);
			}
		}
		intSettings[key - INT_FIRST] = value;
		isSet[key] = true;
	}

	void set(IntSetting key, const string& value) {
		if(value.empty()) {
			intSettings[key - INT_FIRST] = 0;
			isSet[key] = false;
		} else {
			intSettings[key - INT_FIRST] = Util::toInt(value);
			isSet[key] = true;
		}
	}

	void set(Int64Setting key, int64_t value) {
		int64Settings[key - INT64_FIRST] = value;
		isSet[key] = true;
	}

	void set(Int64Setting key, const string& value) {
		if(value.empty()) {
			int64Settings[key - INT64_FIRST] = 0;
			isSet[key] = false;
		} else {
			int64Settings[key - INT64_FIRST] = Util::toInt64(value);
			isSet[key] = true;
		}
	}

	void set(IntSetting key, bool value) { set(key, (int)value); }

	void setDefault(StrSetting key, string const& value) {
		strDefaults[key - STR_FIRST] = value;
	}

	void setDefault(IntSetting key, int value) {
		intDefaults[key - INT_FIRST] = value;
	}
	void setDefault(Int64Setting key, int64_t value) {
		int64Defaults[key - INT64_FIRST] = value;
	}

	bool isDefault(int aSet) { return !isSet[aSet]; };

	void load() {
		load(Util::getAppPath() + "DCPlusPlus.xml");
	}
	void save() {
		save(Util::getAppPath() + "DCPlusPlus.xml");
	}

	void load(const string& aFileName);
	void save(const string& aFileName);

//PDC {
	struct DownloadDirectory {
		string dir, ext, name;
	};

	typedef vector<DownloadDirectory> DDList;

	DDList getDownloadDirs() {
		return DownloadDirectories;
	}

	string getDownloadDir(string ext);

	DownloadDirectory addDownloadDir(string dir, string ext, string name) {
	//string adr = dir;
	if(dir.rfind("\\") != (dir.size()-1))  { dir = dir + "\\";}

		DownloadDirectory d = {dir, ext,name };
		DownloadDirectories.push_back(d);
		return DownloadDirectories[DownloadDirectories.size()-1];
	}

	void removeDownloadDir(int index) {
		DownloadDirectories.erase(DownloadDirectories.begin() + index);
	}

	void updateDownloadDir(int index, DownloadDirectory d) {
		DownloadDirectories[index].name = d.name;
		DownloadDirectories[index].dir = d.dir;
		DownloadDirectories[index].ext = d.ext;
	}

	void NeverDownloadFrom(string user) {
		if(!isNeverDownloadFrom(user))
			NeverDownload.push_back(user);
	}

	void RemoveNeverDownloadFrom(string user) {
		vector <string>::iterator i;
		 if((i = find(NeverDownload.begin(), NeverDownload.end(), user)) == NeverDownload.end())
			 return;
		 NeverDownload.erase(i);
	}

	bool isNeverDownloadFrom(string user) {
		return (find(NeverDownload.begin(), NeverDownload.end(), user) != NeverDownload.end());
	}
	
//PDC }

private:
//PDC {
	DDList DownloadDirectories;
	vector<string> NeverDownload;
//PDC }

	friend class Singleton<SettingsManager>;
	SettingsManager();

	static const string settingTags[SETTINGS_LAST+1];

	string strSettings[STR_LAST - STR_FIRST];
	int    intSettings[INT_LAST - INT_FIRST];
	int64_t int64Settings[INT64_LAST - INT64_FIRST];
	string strDefaults[STR_LAST - STR_FIRST];
	int    intDefaults[INT_LAST - INT_FIRST];
	int64_t int64Defaults[INT64_LAST - INT64_FIRST];
	bool isSet[SETTINGS_LAST];

	void setTemp(){
		// force to use temp directory
		if(get(TEMP_DOWNLOAD_DIRECTORY, true).empty()){
			string s = get(DOWNLOAD_DIRECTORY, true);

			s.erase(s.find_last_of('\\'));

			string::size_type i = s.find_last_of('\\');
			if( i != string::npos)
				s.erase(i);

			string t = s + "\\temp\\";
			set(TEMP_DOWNLOAD_DIRECTORY, t);
		}
	}
};

// Shorthand accessor macros
#define SETTING(k) (SettingsManager::getInstance()->get(SettingsManager::k, true))
#define BOOLSETTING(k) (SettingsManager::getInstance()->getBool(SettingsManager::k, true))

#endif // SETTINGSMANAGER_H

/**
 * @file
 * $Id$
 */

