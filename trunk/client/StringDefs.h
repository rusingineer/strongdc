// @Prolog: #include "stdinc.h"
// @Prolog: #include "DCPlusPlus.h"
// @Prolog: #include "ResourceManager.h"
// @Strings: string ResourceManager::strings[]
// @Names: string ResourceManager::names[]

enum Strings { // @DontAdd
	ACTIVE, // "Active"
	ACTIVE_SEARCH_STRING, // "Active / Search String"
	ADD, // "&Add"
	ADDED, // "Added"
	ADD_TO_FAVORITES, // "Add To Favorites"
	ADDRESS_ALREADY_IN_USE, // "Address already in use"
	ADDRESS_NOT_AVAILABLE, // "Address not available"
	ADL_DISCARD, // "Discard"
	ADL_SEARCH, // "Automatic Directory Listing Search"
	ALL_DOWNLOAD_SLOTS_TAKEN, // "All download slots taken"
	ALL_USERS_OFFLINE, // "All %d users offline"
	ALL_3_USERS_OFFLINE, // "All 3 users offline"
	ALL_4_USERS_OFFLINE, // "All 4 users offline"
	ANY, // "Any"
	AT_LEAST, // "At least"
	AT_MOST, // "At most"
	AUDIO, // "Audio"
	AUTO_CONNECT, // "Auto connect / Name"
	AUTO_GRANT, // "Auto grant slot / Nick"
	AVERAGE, // "Average/s: "
	AWAY, // "AWAY"
	AWAY_MODE_OFF, // "Away mode off"
	AWAY_MODE_ON, // "Away mode on: "
	B, // "B"
	BITZI_LOOKUP, // "Bitzi Lookup"
	BOTH_USERS_OFFLINE, // "Both users offline"
	BROWSE, // "Browse..."
	BROWSE_ACCEL, // "&Browse..."
	CHOOSE_FOLDER, // "Choose folder"
	CLOSE_CONNECTION, // "Close connection"
	CLOSE, // "Close"
	COMPRESSED, // "Compressed"
	COMPRESSION_ERROR, // "Error during compression"
	CONNECT, // "&Connect"
	CONNECTED, // "Connected"
	CONNECTING, // "Connecting..."
	CONNECTING_FORCED, // "Connecting (forced)..."
	CONNECTING_TO, // "Connecting to "
	CONNECTION, // "Connection"
	CONNECTION_CLOSED, // "Connection closed"
	CONNECTION_REFUSED, // "Connection refused by target machine"
	CONNECTION_RESET, // "Connection reset by server"
	CONNECTION_TIMEOUT, // "Connection timeout"
	COPY_HUB, // "Copy address to clipboard"
	COPY_NICK, // "Copy nick"
	COULD_NOT_OPEN_TARGET_FILE, // "Could not open target file: "
	COUNT, // "Count"
	CRC_CHECKED, // "CRC Checked"
	DECOMPRESSION_ERROR, // "Error during decompression"
	DESCRIPTION, // "Description"
	DESTINATION, // "Destination"
	DIRECTORY, // "Directory"
	DIRECTORY_ALREADY_SHARED, // "Directory already shared"
	DISCONNECTED, // "Disconnected"
	DISCONNECTED_USER, // "Disconnected user leaving the hub: "
	DISC_FULL, // "Disk full(?)"
	DOCUMENT, // "Document"
	DONT_SHARE_TEMP_DIRECTORY, // "The temporary download directory cannot be shared"
	DOWNLOAD, // "Download"
	DOWNLOAD_FAILED, // "Download failed: "
	DOWNLOAD_FINISHED_IDLE, // "Download finished, idle..."
	DOWNLOAD_STARTING, // "Download starting..."
	DOWNLOAD_TO, // "Download to..."
	DOWNLOAD_QUEUE, // "Download queue"
	DOWNLOAD_WHOLE_DIR, // "Download whole directory"
	DOWNLOAD_WHOLE_DIR_TO, // "Download whole directory to..."
	DOWNLOADED, // "Downloaded"
	DOWNLOADED_BYTES, // "Downloaded %s (%.01f%%) in %s"
	DOWNLOADED_FROM, // " downloaded from "
	DOWNLOADING, // "Downloading..."
	DOWNLOADING_HUB_LIST, // "Downloading public hub list..."
	DUPLICATE_FILE_NOT_SHARED, // "Duplicate file will not be shared: "
	DUPLICATE_SOURCE, // "Duplicate source"
	EMAIL, // "E-Mail"
	ENTER_NICK, // "Please enter a nickname in the settings dialog!"
	ENTER_SEARCH_STRING, // "Enter search string"
	ERROR_OPENING_FILE, // "Error opening file"
	ERRORS, // "Errors"
	EXACT_SIZE, // "Exact size"
	EXECUTABLE, // "Executable"
	FAVORITE_HUBS, // "Favorite Hubs"
	FAVORITE_HUB_ADDED, // "Favorite hub added"
	FAVORITE_USERS, // "Favorite Users"
	FAVORITE_USER_ADDED, // "Favorite user added"
	FAV_JOIN_SHOWING_OFF, // "Join/part of favorite users showing off"
	FAV_JOIN_SHOWING_ON, // "Join/part of favorite users showing on"
	FILE, // "File"
	FILES, // "Files"
	FILE_LIST_REFRESH_FINISHED, // "File list refresh finished"
	FILE_LIST_REFRESH_INITIATED, // "File list refresh initiated"
	FILE_NOT_AVAILABLE, // "File not available"
	FILE_TYPE, // "File type"
	FILE_WITH_DIFFERENT_SIZE, // "A file with a different size already exists in the queue"
	FILE_WITH_DIFFERENT_TTH, // "A file with diffent tth root already exists in the queue"
	FILENAME, // "Filename"
	FILTER, // "F&ilter"
	FIND, // "Find"
	FINISHED_DOWNLOADS, // "Finished Downloads"
	FINISHED_UPLOADS, // "Finished Uploads"
	FOLDER, // "Folder"
	FORBIDDEN_DOLLAR_DIRECTORY, // "Directory with '$' cannot be downloaded and will not be shared: "
	FORBIDDEN_DOLLAR_FILE, // "File with '$' cannot be downloaded and will not be shared: "
	FORCE_ATTEMPT, // "Force attempt"
	GB, // "GB"
	GET_FILE_LIST, // "Get file list"
	GO_TO_DIRECTORY, // "Go to directory"
	GRANT_EXTRA_SLOT, // "Grant extra slot (10 min)"
	HASH_DATABASE, // "Hash database"
	HASH_REBUILT, // "Hash database rebuilt"
	HASHING_FINISHED, // "Finished hashing "
	HIGH, // "High"
	HIGHEST, // "Highest"
	HIT_RATIO, // "Hit Ratio: "
	HIT_COUNT, // "Hits"
	HITS, // "Hits: "
	HOST_UNREACHABLE, // "Host unreachable"
	HUB, // "Hub"
	HUBS, // "Hubs"
	HUB_ADDRESS, // "Address"
	HUB_LIST_DOWNLOADED, // "Hub list downloaded..."
	HUB_NAME, // "Name"
	HUB_USERS, // "Users"
	IGNORED_MESSAGE, // "Ignored message: "
	INVALID_NUMBER_OF_SLOTS, // "Invalid number of slots"
	INVALID_TARGET_FILE, // "Invalid target file (missing directory, check default download directory setting)"
	INVALID_TREE, // "Downloaded tree does not match TTH root"
	IP, // "Ip: "
	IP_BARE, // "Ip"
	ITEMS, // "Items"
	JOINS, // "Joins: "
	JOIN_SHOWING_OFF, // "Join/part showing off"
	JOIN_SHOWING_ON, // "Join/part showing on"
	KB, // "kB"
	KBPS, // "kB/s"
	KICK_USER, // "Kick user(s)"
	LARGER_TARGET_FILE_EXISTS, // "A file of equal or larger size already exists at the target location"
	LAST_CHANGE, // "Last change: "
	LAST_HUB, // "Hub (last seen on if offline)"
	LAST_SEEN, // "Time last seen"
	LOADING, // "Loading StrongDC++, please wait..."
	LOW, // "Low"
	LOWEST, // "Lowest"
	MANUAL_ADDRESS, // "Manual connect address"
	MATCH_QUEUE, // "Match queue"
	MATCHED_FILES, // "Matched %d file(s)"
	MB, // "MB"
	MBPS, // "MB/s"
	MENU_FILE, // "&File"
	MENU_ADL_SEARCH, // "ADL Search"
	MENU_DOWNLOAD_QUEUE, // "&Download Queue\tCtrl+D"
	MENU_EXIT, // "&Exit"
	MENU_FAVORITE_HUBS, // "&Favorite Hubs\tCtrl+F"
	MENU_FAVORITE_USERS, // "Favorite &Users\tCtrl+U"
	MENU_FOLLOW_REDIRECT, // "Follow last redirec&t\tCtrl+T"
	MENU_IMPORT_QUEUE, // "Import queue from NMDC..."
	MENU_NETWORK_STATISTICS, // "Network Statistics"
	MENU_NOTEPAD, // "&Notepad\tCtrl+N"
	MENU_OPEN_FILE_LIST, // "Open file list..."
	MENU_PUBLIC_HUBS, // "&Public Hubs\tCtrl+P"
	MENU_RECONNECT, // "&Reconnect\tCtrl+R"
	MENU_REFRESH_FILE_LIST, // "Refresh file list"
	MENU_SHOW, // "Show"
	MENU_SEARCH, // "&Search\tCtrl+S"
	MENU_SEARCH_SPY, // "Search spy"
	MENU_SETTINGS, // "Settings..."
	MENU_HELP, // "&Help"
	MENU_ABOUT, // "About StrongDC++"
	MENU_DISCUSS, // "StrongDC++ Discussion forum"
	MENU_HOMEPAGE, // "StrongDC++ Homepage"
	MENU_OPEN_DOWNLOADS_DIR, // "Open downloads directory"
	MENU_VIEW, // "&View"
	MENU_STATUS_BAR, // "&Status bar\tCtrl+2"
	MENU_TOOLBAR, // "&Toolbar\tCtrl+1"
	MENU_TRANSFER_VIEW, // "T&ransfers\tCtrl+3"
	MENU_WINDOW, // "&Window"
	MENU_ARRANGE, // "Arrange icons"
	MENU_CASCADE, // "Cascade"
	MENU_HORIZONTAL_TILE, // "Horizontal Tile"
	MENU_VERTICAL_TILE, // "Vertical Tile"
	MENU_CLOSE_DISCONNECTED, // "Close disconnected"
	MENU_MINIMIZE_ALL, // "Minimize &All"
	MOVE, // "Move/Rename"
	MOVE_DOWN, // "Move &Down"
	MOVE_UP, // "Move &Up"
	NETWORK_STATISTICS, // "Network Statistics"
	NETWORK_UNREACHABLE, // "Network unreachable (are you connected to the internet?)"
	NEXT, // "Next"
	NEW, // "&New..."
	NICK, // "Nick"
	NICK_TAKEN, // "Your nick was already taken, please change to something else!"
	NICK_UNKNOWN, // " (Nick unknown)"
	NON_BLOCKING_OPERATION, // "Non-blocking operation still in progress"
	NOT_CONNECTED, // "Not connected"
	NOT_SOCKET, // "Not a socket"
	NO, // "No"
	NO_DIRECTORY_SPECIFIED, // "No directory specified"
	NO_DOWNLOADS_FROM_SELF, // "You're trying to download from yourself!"
	NO_ERRORS, // "No errors"
	NO_MATCHES, // "No matches"
	NO_SLOTS_AVAILABLE, // "No slots available"
	NO_USERS, // "No users"
	NO_USERS_TO_DOWNLOAD_FROM, // "No users to download from"
	NORMAL, // "Normal"
	NOTEPAD, // "Notepad"
	OFFLINE, // "Offline"
	ONLINE, // "Online"
	ONLY_FREE_SLOTS, // "Only users with free slots"
	ONLY_WHERE_OP, // "Only where I'm op"
	OPEN, // "Open"
	OPEN_DOWNLOAD_PAGE, // "Open download page?"
	OPEN_FOLDER, // "Open folder"
	OPERATION_WOULD_BLOCK_EXECUTION, // "Operation would block execution"
	OUT_OF_BUFFER_SPACE, // "Out of buffer space"
	PASSIVE_USER, // "Passive user"
	PASSWORD, // "Password"
	PARTS, // "Parts: "
	PATH, // "Path"
	PAUSED, // "Paused"
	PERMISSION_DENIED, // "Permission denied"
	PICTURE, // "Picture"
	PORT, // "Port: "
	PORT_IS_BUSY, // "Port %d is busy, please choose another one in the settings dialog, or disable any other application that might be using it and restart StrongDC++"
	PRESS_FOLLOW, // "Press the follow redirect button to connect to "
	PRIORITY, // "Priority"
	PRIVATE_MESSAGE_FROM, // "Private message from "
	PROPERTIES, // "&Properties"
	PUBLIC_HUBS, // "Public Hubs"
	RATIO, // "Ratio"
	READD_SOURCE, // "Readd source"
	REALLY_EXIT, // "Really exit?"
	REDIRECT_ALREADY_CONNECTED, // "Redirect request received to a hub that's already connected"
	REDIRECT_USER, // "Redirect user(s)"
	REFRESH, // "&Refresh"
	REFRESH_USER_LIST, // "Refresh user list"
	REMOVE, // "&Remove"
	REMOVE_ALL, // "Remove all"
	REMOVE_ALL_SUBDIRECTORIES, // "Remove all subdirectories before adding this one"
	REMOVE_FROM_ALL, // "Remove user from queue"
	REMOVE_SOURCE, // "Remove source"
	ROLLBACK_INCONSISTENCY, // "Rollback inconsistency, existing file does not match the one being downloaded"
	RUNNING, // "Running..."
	SEARCH, // "Search"
	SEARCH_BY_TTH, // "Search by TTH"
	SEARCH_FOR, // "Search for"
	SEARCH_FOR_ALTERNATES, // "Search for alternates"
	SEARCH_FOR_FILE, // "Search for file"
	SEARCH_OPTIONS, // "Search options"
	SEARCH_SPAM_FROM, // "Search spam detected from "
	SEARCH_SPY, // "Search Spy"
	SEARCH_STRING, // "Search String"
	SEARCH_STRING_INEFFICIENT, // "Specifying the same search string for more than 5 files for a passive connection or 10 files for an active connection is inefficient. Would you like to continue with the operation?"
	SEARCHING_FOR, // "Searching for "
	SEND_PRIVATE_MESSAGE, // "Send private message"
	SEPARATOR, // "Separator"
	SERVER, // "Server"
	SET_PRIORITY, // "Set priority"
	SETTINGS_ADD_FOLDER, // "&Add folder"
	SETTINGS_ADLS_BREAK_ON_FIRST, // "Break on first ADLSearch match"
	SETTINGS_ADVANCED, // "Advanced"
	SETTINGS_ADVANCED_SETTINGS, // "Advanced settings"
	SETTINGS_ANTI_FRAG, // "Use antifragmentation method for downloads"
	SETTINGS_APPEARANCE, // "Appearance"
	SETTINGS_AUTO_AWAY, // "Auto-away on minimize (and back on restore)"
	SETTINGS_AUTO_FOLLOW, // "Automatically follow redirects"
	SETTINGS_AUTO_KICK, // "Automatically disconnect users who leave the hub (does not disconnect when hub goes down / you leave it)"
	SETTINGS_AUTO_SEARCH, // "Automatically search for alternative download locations"
	SETTINGS_AUTO_SEARCH_AUTO_MATCH, // "Automatically match queue for auto search hits"
	SETTINGS_AUTO_SEARCH_AUTO_STRING, // "Use default search strings in auto search when no string is specified"
	SETTINGS_AUTO_UPDATE_LIST, // "Automatically refresh share list every hour"
	SETTINGS_CHANGE, // "&Change"
	SETTINGS_CLEAR_SEARCH, // "Clear search box after each search"
	SETTINGS_CLIENT_VER, // "Client version"
	SETTINGS_COMPRESS_TRANSFERS, // "Enable safe and compressed transfers"
	SETTINGS_COMMAND, // "Command"
	SETTINGS_CONFIRM_EXIT, // "Confirm application exit"
	SETTINGS_CONNECTION_SETTINGS, // "Connection Settings (see the readme / newbie help if unsure)"
	SETTINGS_CONNECTION_TYPE, // "Connection Type"
	SETTINGS_DEFAULT_AWAY_MSG, // "Default away message"
	SETTINGS_DIRECTORIES, // "Directories"
	SETTINGS_DOWNLOAD_DIRECTORY, // "Default download directory"
	SETTINGS_DOWNLOAD_LIMITS, // "Limits"
	SETTINGS_DOWNLOADS, // "Downloads"
	SETTINGS_DOWNLOADS_MAX, // "Maximum simultaneous downloads (0 = infinite)"
	SETTINGS_DOWNLOADS_SPEED_PAUSE, // "No new downloads if speed exceeds (kB/s, 0 = disable)"
	SETTINGS_FAV_SHOW_JOINS, // "Only show joins / parts for favorite users"
	SETTINGS_FILTER_MESSAGES, // "Filter kick and NMDC debug messages"
	SETTINGS_FINISHED_DIRTY, // "Set Finished Manager(s) tab bold when an entry is added"
	SETTINGS_FORMAT, // "Format"
	SETTINGS_GENERAL, // "General"
	SETTINGS_HUB_USER_COMMANDS, // "Accept custom user commands from hub"
	SETTINGS_IGNORE_OFFLINE, // "Ignore messages from users that are not online (effective against bots)"
	SETTINGS_IP, // "IP"
	SETTINGS_KEEP_LISTS, // "Don't delete file lists when exiting"
	SETTINGS_LANGUAGE_FILE, // "Language file"
	SETTINGS_LIST_DUPES, // "Include duplicate files in your file list (duplicates never count towards your share size)"	
	SETTINGS_LOG_DIR, // "Log directory"
	SETTINGS_LOG_DOWNLOADS, // "Log downloads"
	SETTINGS_LOG_FILELIST_TRANSFERS, // "Log filelist transfers"
	SETTINGS_LOG_MAIN_CHAT, // "Log main chat"
	SETTINGS_LOG_PRIVATE_CHAT, // "Log private chat"
	SETTINGS_LOG_UPLOADS, // "Log uploads"
	SETTINGS_LOG_SYSTEM_MESSAGES, // "Log system messages"
	SETTINGS_LOGGING, // "Logging"
	SETTINGS_LOGS, // "Logs and Sound"
	SETTINGS_MAX_HASH_SPEED, // "Max hash speed"
	SETTINGS_MAX_TAB_ROWS, // "Max tab rows"
	SETTINGS_MEMORY_MAPPED_FILE, // "Use memory mapped file"
	SETTINGS_MINIMIZE_TRAY, // "Minimize to tray"
	SETTINGS_NAME, // "Name"
	SETTINGS_NO_AWAYMSG_TO_BOTS, // "Don't send the away message to bots"
	SETTINGS_OPEN_FAVORITE_HUBS, // "Open the favorite hubs window at startup"
	SETTINGS_OPEN_FINISHED_DOWNLOADS, // "Open the finished downloads window at startup"
	SETTINGS_OPEN_PUBLIC, // "Open the public hubs window at startup"
	SETTINGS_OPEN_QUEUE, // "Open the download queue window at startup"
	SETTINGS_OPTIONS, // "Options"
	SETTINGS_PASSIVE, // "Passive"
	SETTINGS_PERSONAL_INFORMATION, // "Personal Information"
	SETTINGS_PM_BEEP, // "Make an annoying sound every time a private message is received"
	SETTINGS_PM_BEEP_OPEN, // "Make an annoying sound when a private message window is opened"
	SETTINGS_POPUP_OFFLINE, // "Popup messages from users that are not online (if not ignoring, messages go to main chat if enabled)"
	SETTINGS_POPUP_PMS, // "Popup private messages"
	SETTINGS_PORT, // "Port (empty=random)"
	SETTINGS_PUBLIC_HUB_LIST, // "Public Hubs list"
	SETTINGS_PUBLIC_HUB_LIST_URL, // "Public Hubs list URL"
	SETTINGS_PUBLIC_HUB_LIST_HTTP_PROXY, // "HTTP Proxy (for hublist only)"
	SETTINGS_QUEUE_DIRTY, // "Set Download Queue tab bold when contents change"
	SETTINGS_REQUIRES_RESTART, // "Note; most of these options require that you restart StrongDC++"
	SETTINGS_ROLLBACK, // "Rollback"
	SETTINGS_SELECT_TEXT_FACE, // "&Text style"
	SETTINGS_SELECT_WINDOW_COLOR, // "&Window color"
	SETTINGS_SEND_UNKNOWN_COMMANDS, // "Send unknown /commands to the hub"
	SETTINGS_SFV_CHECK, // "Enable automatic SFV checking"
	SETTINGS_SHARE_HIDDEN, // "Share hidden files"
	SETTINGS_SHARE_SIZE, // "Total size:"
	SETTINGS_SHARED_DIRECTORIES, // "Shared directories"
	SETTINGS_SHOW_JOINS, // "Show joins / parts in chat by default"
	SETTINGS_SHOW_PROGRESS_BARS, // "Show progress bars for transfers (uses some CPU)"
	SETTINGS_SKIP_ZERO_BYTE, // "Skip zero-byte files"
	SETTINGS_SMALL_SEND_BUFFER, // "Use small send buffer (enable if uploads slow downloads a lot)"
	SETTINGS_SOCKS5, // "SOCKS5"
	SETTINGS_SOCKS5_IP, // "Socks IP"
	SETTINGS_SOCKS5_PORT, // "Port"
	SETTINGS_SOCKS5_RESOLVE, // "Use SOCKS5 server to resolve hostnames"
	SETTINGS_SOCKS5_USERNAME, // "Username"
	SETTINGS_SOUNDS, // "Sounds"
	SETTINGS_SPEEDS_NOT_ACCURATE, // "Note; because of changing download speeds, this is not 100% accurate..."
	SETTINGS_STATUS_IN_CHAT, // "View status messages in main chat"
	SETTINGS_TAB_COMPLETION, // "Tab completion of nicks in chat"
	SETTINGS_TIME_STAMPS, // "Show timestamps in chat by default"
	SETTINGS_UNFINISHED_DOWNLOAD_DIRECTORY, // "Unfinished downloads directory (empty = download directly to target)"
	SETTINGS_UPLOADS, // "Sharing"
	SETTINGS_UPLOADS_MIN_SPEED, // "Automatically open an extra slot if speed is below (0 = disable)"
	SETTINGS_UPLOADS_SLOTS, // "Upload slots"
	SETTINGS_URL_HANDLER, // "Install URL handler on startup (to handle dchub:// links)"
	SETTINGS_USE_OEM_MONOFONT, // "Use OEM monospaced font for viewing text files"
	SETTINGS_USE_SYSTEM_ICONS, // "Use system icons when browsing files (slows browsing down a bit)"
	SETTINGS_USER_COMMANDS, // "User Commands"
	SETTINGS_USER_MENU, // "User Menu Items"
	SETTINGS_WRITE_BUFFER, // "Write buffer size"
	SETTINGS_GET_USER_COUNTRY, // "Get User Country"
	SETTINGS_LOG_STATUS_MESSAGES, // "Log status messages"
	SETTINGS, // "Settings"
	SFV_INCONSISTENCY, // "CRC32 inconsistency (SFV-Check)"
	SHARED, // "Shared"
	SHARED_FILES, // "Shared Files"
	SIZE, // "Size"
	SIZE_MAX, // "Max Size"
	SIZE_MIN, // "Min Size"
	SLOT_GRANTED, // "Slot granted"
	SLOTS, // "Slots"
	SLOTS_SET, // "Slots set"
	SOCKET_SHUT_DOWN, // "Socket has been shut down"
	SOCKS_AUTH_FAILED, // "Socks server authentication failed (bad username / password?)"
	SOCKS_AUTH_UNSUPPORTED, // "The socks server doesn't support user / password authentication"
	SOCKS_FAILED, // "The socks server failed establish a connection"
	SOCKS_NEEDS_AUTH, // "The socks server requires authentication"
	SOCKS_SETUP_ERROR, // "Failed to set up the socks server for UDP relay (check socks address and port)"
	SOURCE_TYPE, // "Source Type"
	SPECIFY_SERVER, // "Specify a server to connect to"
	SPECIFY_SEARCH_STRING, // "Specify a search string"
	SPEED, // "Speed"
	STATUS, // "Status"
	STORED_PASSWORD_SENT, // "Stored password sent..."
	TAG, // "Tag"
	TARGET_FILENAME_TOO_LONG, // "Target filename too long"
	TB, // "TB"
	TIME, // "Time"
	TIME_LEFT, // "Time left"
	TIMESTAMPS_DISABLED, // "Timestamps disabled"
	TIMESTAMPS_ENABLED, // "Timestamps enabled"
	TOTAL, // "Total: "
	TTH_ROOT, // "TTH Root"
	TYPE, // "Type"
	UNABLE_TO_CREATE_THREAD, // "Unable to create thread"
	UNKNOWN, // "Unknown"
	UNKNOWN_ADDRESS, // "Unknown address"
	UNKNOWN_COMMAND, // "Unknown command: "
	UNKNOWN_ERROR, // "Unknown error: 0x%x"
	UNSUPPORTED_FILELIST_FORMAT, // "Unsupported filelist format"
	UPLOAD_FINISHED_IDLE, // "Upload finished, idle..."
	UPLOAD_STARTING, // "Upload starting..."
	UPLOADED_BYTES, // "Uploaded %s (%.01f%%) in %s"
	UPLOADED_TO, // " uploaded to "
	USER, // "User"
	USER_DESCRIPTION, // "User Description"
	USER_OFFLINE, // "User offline"
	USER_WENT_OFFLINE, // "User went offline"
	USERS, // "Users"
	VIDEO, // "Video"
	VIEW_AS_TEXT, // "View as text"
	WAITING_USER_ONLINE, // "Waiting (User online)"
	WAITING_USERS_ONLINE, // "Waiting (%d of %d users online)"
	WAITING_TO_RETRY, // "Waiting to retry..."
	WHATS_THIS, // "What's &this?"
	YES, // "Yes"
	COPY_EMAIL_ADDRESS, // "Copy email address"
	COPY_DESCRIPTION, // "Copy description"
	SEND_PUBLIC_MESSAGE, // "Send public message"
	EXTRA_SLOTS_SET, // "Extra slots set"
	SMALL_FILE_SIZE_SET, // "Small file size set"
	INVALID_SIZE, // "Invalid size"
	SETTINGS_CZDC, // "StrongDC++"
	SETTINGS_TEXT_STYLES, // "Colors & Fonts"
	KICK_USER_FILE, // "Kick user(s) with filename"
	COUNTRY, // "Country"
	MINSHARE, // "Min share"
	MINSLOTS, // "Min slots"
	MAXHUBS, // "Max hubs"
	MAXUSERS, // "Max users"
	RELIABILITY, // "Reliability"
	RATING, // "Rating"
	CLIENTID, // "Client"
	VERSION, // "Version"
	MODE, // "Mode"
	LIMIT, // "Upload Limit"
	GRANT_EXTRA_SLOT_HOUR, // "Grant extra slot (hour)"
	GRANT_EXTRA_SLOT_DAY, // "Grant extra slot (day)"
	GRANT_EXTRA_SLOT_WEEK, // "Grant extra slot (week)"
	COPY, // "Copy"
	COPY_LINE, // "Copy actual line"
	COPY_URL, // "Copy URL"
	SELECT_ALL, // "Select all"
	CLEAR, // "Clear chat"
	ASCROLL_CHAT, // "Autoscrool chat"
	UPLOAD_QUEUE, // "Upload Queue"
	GRANT_SLOTS_MENU, // "Extra Slots"
	ANTI_PASSIVE_SEARCH, // "-- WARNING --\r\n-- You are in active mode, but have activated passive search. --\r\n-- Don't use passive search if you have search results without this option --\r\n-- because you don't get search result from passive clients !!! --\r\n-- Turn it off in settings => StrongDC++ => Always use Passive Mode for Search --\r\n"
	SETCZDC_FEAT, // "StrongDC++ Features"
	SETCZDC_SEND_TAG, // "Send StrongDC++ tag in description (always send if more than 10 hubs)"
	SETCZDC_GET_DC_UPDATE, // "Get information on new dc++ versions"
	SETCZDC_POPUP_FILELIST, // "PoPup filelists"
	SETCZDC_FILTER_SEARCH, // "Filter Duplicate Search Results"
	SETCZDC_REMOVE_FORBIDDEN, // "Remove Forbidden (Unfinished Kazaa, Win MX, GetRight, eMule, StrongDC++)"
	SETCZDC_PRIVATE_SOUND, // "Private message sound"
	SETCZDC_ENABLE_LIMITING, // "Enable Transfer Rate Limiting"
	SETCZDC_TRANSFER_LIMITING, // "Transfer Rate Limiting"
	SETCZDC_UPLOAD_SPEED, // "Upload Speed"
	SETCZDC_DOWNLOAD_SPEED, // "Download Speed"
	SETCZDC_ALTERNATE_LIMITING, // "Use Alternate Limiting from"
	SETCZDC_TO, // "to"
	SETCZDC_SECONDARY_LIMITING, // "Secondary Transfer Rate Limiting"
	SETCZDC_NOTE_UPLOAD, // "* Upload limit for first slot minimal 5 kB/s. Next slots 2 kB/s per slot !!!"
	SETCZDC_NOTE_DOWNLOAD, // "* If upload limit is set, download limit is max 6 x upload limit !!!"
	SETCZDC_PASSIVE_SEARCH, // "Always use passive mode for Search (Use only if you know what this doing !!!)"
	SETCZDC_PROGRESSBAR_COLORS, // "Progressbar colors"
	SETCZDC_UPLOAD, // "Upload"
	SETCZDC_PROGRESSBAR_TEXT, // "Progressbar text colors"
	SETCZDC_WINAMP, // "Winamp"
	SETCZDC_STYLES, // "Available styles"
	SETCZDC_BACK_COLOR, // "Back color"
	SETCZDC_TEXT_COLOR, // "Text color"
	SETCZDC_TEXT_STYLE, // "Text style"
	SETCZDC_DEFAULT_STYLE, // "Default styles"
	SETCZDC_BLACK_WHITE, // "Black & white"
	SETCZDC_BOLD, // "Bold Authors of messages in chat"
	SETCZDC_PREVIEW, // "Preview"
	SELECT_USER_LIST, // "Select user in list"
	SETCZDC_SMALL_UP_SLOTS, // "Small Upload Slots"
	SETCZDC_SMALL_FILES, // "Small file size"
	SETCZDC_NOTE_SMALL_UP, // "* Small Uploads Slots => Slots for filelist and Small Files."
	DOWNLOAD_WITH_PRIORITY, // "Download with priority..."
	READD_ALL_SOURCES, // "Readd all sources"
	ADLSEARCH_ACTIVE_SEARCH, // "Active Search"
	ADLSEARCH_DESTINATION_DIRECTORY, // "Destination Directory"
	ADLSEARCH_MAX_SIZE, // "Max Filesize"
	ADLSEARCH_MIN_SIZE, // "Min Filesize"
	ADLSEARCH_PROPERTIES, // "ADLSearch Properties"
	ADLSEARCH_SIZE_TYPE, // "Size Type"
	ADLSEARCH_SEARCH_STRING, // "Search String"
	ADLSEARCH_SOURCE_TYPE, // "Source Type"
	CANCEL, // "Cancel"
	OK, // "OK"
	SETTINGS_SHOW_INFO_TIPS, // "Show InfoTips in lists"
	USER_WENT_ONLINE, // "User went online"
	SETTINGS_CZDC_EXTRA_DOWNLOADS, // "Highest Priority Extra Download Slots"
	COPY_TAG, // "Copy TAG"
	MENU_VIEW_TRANSFERS, // "Transfers"
	SETTINGS_OPEN_NETWORK_STATISTIC, // "Open the network statistic window at startup"
	SETTINGS_MINIMIZE_ON_STARTUP, // "Minimize at program startup"
	SETTINGS_CONFIRM_DELETE, // "Confirm items removing from queue"
	SETTINGS_FREE_SLOT_DEFAULT, // "Search only users with free slots by default"
	OPEN_HUB_LOG, // "Open hub log"
	OPEN_USER_LOG, // "Open user log"
	NO_LOG_FOR_USER, // "User log not exist"
	NO_LOG_FOR_HUB, // "Hub log not exist"
	OPEN_DOWNLOAD_LOG, // "Open downloads log"
	NO_DOWNLOAD_LOG, // "Downloads log not exist"
	OPEN_UPLOAD_LOG, // "Open uploads log"
	NO_UPLOAD_LOG, // "Uploads log not exist"
	WHO_IS, // "Whois "
	SETTINGS_USE_EXTENSION_DOWNTO, // "Use file extension for Download to in search"
	SETCZDC_ERROR_COLOR, // "Error color"
	SETTINGS_SHOW_SHARED, // "Enable Shared column in userlist"
	SETTINGS_SHOW_EXACT_SHARED, // "Enable Exact Shared column in userlist"
	SETTINGS_SHOW_DESCRIPTION, // "Enable Description column in userlist"
	SETTINGS_SHOW_TAG, // "Enable Tag column in userlist"
	SETTINGS_SHOW_CONNECTION, // "Enable Connection column in userlist"
	SETTINGS_SHOW_EMAIL, // "Enable Email column in userlist"
	SETTINGS_CLIENT, // "Enable Client column in userlist"
	SETTINGS_SHOW_VERSION, // "Enable Version column in userlist"
	SETTINGS_SHOW_MODE, // "Enable Mode column in userlist"
	SETTINGS_SHOW_HUBS, // "Enable Hubs column in userlist"
	SETTINGS_SHOW_SLOTS, // "Enable Slots column in userlist"
	SETTINGS_SHOW_UPLOAD, // "Enable Upload column in userlist"
	SETTINGS_EXPAND_QUEUE, // "Automatically expand folders in Queue"
	REMOVE_EXTRA_SLOT, // "Remove extra slot"
	FINISHED_DOWNLOAD, // "Download finished: %s"
	EXACT_SHARED, // "Exact Share"
	COPY_EXACT_SHARE, // "Copy Exact Share"
	SETTINGS_SHOW_IP, // "Enable IP column in userlist"
	COPY_IP, // "Copy IP"
	SLOW_USER, // "Slow user"
	SETTINGS_LIMIT, // "Limits"
	MINUTES, // "Minutes"
	SETCZDC_SLOW_DISCONNECT, // "Disconnecting slow downloads"
	SETCZDC_I_DOWN_SPEED, // "Disconnect download if speed is below"
	SETCZDC_TIME_DOWN, // "More than"
	SETCZDC_H_DOWN_SPEED, // "And all downloads not exceeds"
	KBPS_DISABLE, // "kB/s (0 = disable)"
	ISP, // "ISP"
	SETTINGS_ZDC_PROGRESS_OVERRIDE, // "Override system colors"
	SETTINGS_ZDC_PROGRESS_OVERRIDE2, // "Override system colors"
	MENUBAR, // "Menu bar"
	LEFT_COLOR, // "Left color"
	RIGHT_COLOR, // "Right color"
	TWO_COLORS, // "Two colors"
	BUMPED, // "Bumped"
	IGNORE_USER, // "Ignore User"
	UNIGNORE_USER, // "Unignore User"
	WINAMP_HELP_DESC, // "Winamp Help"
	WINAMP_HELP, // "/winamp - Works with 1.x, 2.x, 5.x (no WinAmp 3 support)\r\n- %[version]	Numerical Version (ex: 2.91)\r\n- %[state]	Playing state (ex: stopped/paused/playing)\r\n- %[title]		Window title from Winamp - if you want to change this for mp3s, Winamp > Pref > Input > MPEG > Title\r\n- %[rawtitle]	Window title from Winamp (if %[title] not working propertly)\r\n- %[percent]	Percentage (ex. 40%)\r\n- %[length]	Length in minutes:seconds (ex: 04:09)\r\n- %[elapsed]	Time elapsed in minutes:seconds (ex. 03:51)\r\n- %[bar]		ASCII progress bar, 10 characters wide no including brackets (ex. [----|-----])\r\n- %[bitrate]	Bitrate (ex. 128kbps)\r\n- %[sample]	Sample frequency (ex. 22kHz)\r\n- %[channels]	Number of channels (ex. stereo / mono)\r\nEmpty = Default String -> winamp(%[version]) %[state](%[title]) stats(%[percent] of %[length] %[bar])"
	SETCZDC_DISCONNECTING_ENABLE, // "Enable slow downloads disconnecting"
	SETCZDC_MIN_FILE_SIZE, // "And file size is more than"
	SETCZDC_REMOVE_SLOW_USER, // "Remove slow user from Queue"
	AVERAGE_UPLOAD, // "Upload Speed"
	SETTINGS_SOUND, // "Sounds"
	SOUND_DOWNLOAD_BEGINS, // "Download begins"
	SOUND_DOWNLOAD_FINISHED, // "Download finished"
	SOUND_SOURCE_ADDED, // "Alternate source added"
	SOUND_UPLOAD_FINISHED, // "Upload finished"
	SOUND_FAKER_FOUND, // "Faker found"
	SOUND_TTH_INVALID, // "File is corrupted"
	SOUND_EXCEPTION, // "Unhandled exception"
	EXTRA_HUB_SLOTS, // "Open X extra slots for each hub (when hubs = 2 or more)"
	SETTINGS_SEGMENT, // "Segment Downloading"
	AUTO_SEGMENT_TEXT, // "Automatically do Accelerated (Segmented) downloading"
	SEGMENT2_TEXT, // "2 segments if file size is between                 and                  MB"
	SEGMENT3_TEXT, // "3 segments if file size is between                 and                  MB"
	SEGMENT4_TEXT, // "4 segments if file size is between                 and                  MB"
	SEGMENT6_TEXT, // "6 segments if file size is between                 and                  MB"
	SEGMENT8_TEXT, // "8 segments if file size is greater than                  MB"
	DONT_ADD_SEGMENT_TEXT, // "Don't add any other segment if speed is above"
	WARN_SEGMENT_TEXT, // "(Too low or too high settings can decrease download speed!)"
	DISABLE_COMPRESSION, // "Disable safe && compressed transfer"
	SETTINGS_MAX_COMPRESS, // "Max compression level"
	SETTINGS_EMPTY_WORKING_SET, // "Empty process working set (experimental)."
	SETTINGS_ODC_FLOOD_CACHE, // "Use image cache (lowers CPU usage, but might distort the graphics)"
	TEXT_FILESIZE, // "Number of segments based on the file size"
	TEXT_CONNECTION, // "Number of segments based on the connection type"
	TEXT_MANUAL, // "Manual settings of number of segments"
	TEXT_EXTENSION, // "Don't use segment downloading for this filetypes:\n(use regular expressions)"
	TEXT_MINIMUM, // "Minimum download block (segment) size"
	SETTINGS_FAKEDETECT, // "Fake detector"
	SETTINGS_OPERACOLORS, // "Progressbar colors"
	SETTINGS_AVIPREVIEW, // "File Preview"
	CREATING_HASH, // "Hashing"
	TEXT_IGNORE_JUNK, // "Ignore junk files altogether"
	TEXT_FAKEINFO, // "Set size to zero to disable, otherwise files larger than this will be considered junk files"
	TEXT_JUNK, // "Junk file size"
	TEXT_BINJUNK, // "BIN file size"
	TEXT_VOBJUNK, // "VOB file size"
	TEXT_FAKEPERCENT, // "Percent fake share accepted"
	SETTINGS_ARGUMENT, // "Arguments"
	SETTINGS_EXTENSIONS, // "Extensions"
	PREVIEW_MENU, // "Preview"
	SETTINGS_MAX_SOURCES, // "Max number of alternate sources"
	CLIENT_EMU, // "Client Emulation"
	GET_IP, // "Get IP Address"
	SHUTDOWN, // "Shutdown computer"
	SHUTDOWN_OFF, // "Shutdown sequence deactivated..."
	SHUTDOWN_ON, // "Shutdown sequence activated..."
	FAILED_TO_SHUTDOWN, // "Failed to shutdown!"
	SETTINGS_ODC_SHUTDOWNTIMEOUT, // "Shutdown timeout (in seconds)"
	USER_ONLINE, // "Running... (user online)"
	USERS_ONLINE, // "Running... (%d of %d users online)"
	RECENT_HUBS, // "Recent Hubs"
	MENU_FILE_RECENT_HUBS, // "&Recent Hubs"
	SHOW_ISP, // "Enable ISP column in userlist"
	COPY_ALL, // "Copy All"
	COPY_NICK_IP, // "Copy Nick+IP"
	COPY_ISP, // "Copy Nick+IP"
	SETTINGS_TOOLBAR, // "Toolbar"	
	SETTINGS_TOOLBAR_IMAGE, // "Toolbar Images"
	SETTINGS_TOOLBAR_ADD, // "Add -->"
	SETTINGS_TOOLBAR_REMOVE, // "<-- Remove"
	SETTINGS_MOUSE_OVER, // "Mouse Over"
	SETTINGS_NORMAL, // "Normal"
	SETTINGS_DIRS_DESCRIPTION, // "The Following Directories are automatically appended to the \"Download to...\" menu.  When an extension is supplied it becomes the default download directory for that type of file."
	SETTINGS_DOWNLOADDIRS, // "Directories"
	PK, // "PK String"
	LOCK, // "Lock"
	SUPPORTS, // "Supports"
	SHOW_PK, // "Enable PKString column in userlist"
	SHOW_LOCK, // "Enable Lock column in userlist"
	SHOW_SUPPORTS, // "Enable Supports column in userlist"
	GET_USER_RESPONSES, // "Get user response"
	REPORT, // "Report user"
	USERLIST_ICONS, // "Userlist Icons"
	SETTINGS_CLIENTS, // "Clients"
	ENABLE_EMOTICONS, // "Enable emoticons"
	CHECK_MISMATCHED_SHARE_SIZE, // "Mismatched share size - "
	CHECK_0BYTE_SHARE, // "zero bytes real size"
	CHECK_INFLATED, // "filelist was inflated %s times"
	CHECK_SHOW_REAL_SHARE, // ", stated size = %[statedshareformat], real size = %[realshareformat]"
	CHECK_JUNK_FILES, // "Junk files found - %[junkshareformat] was junk, Stated share = %[statedshareformat], Real share = %[realshareformat]."
	NEW_DISCONNECT, // "Remove user from queue, if speed is below"
	MENU_CDMDEBUG_MESSAGES, // "CDM Debug Messages"
	GET_MP3INFO, // "Get MP3 Info"
	CHECK_FILELIST, // "Check File List"
	SIZE_EXACT, // "Exactly"
	DOWNLOAD_CORRUPTED, // "File was corrupted, redownloading in %d seconds"
	BLOCK_FINISHED, // "Segment Block Finished, waiting..."
	STOP_SEARCH, // "Stop Search"
	SEARCH_STOPPED, // "Search stopped"
	MYNICK_IN_CHAT, // "My nick in mainchat"
	NICK_MENU, // "Nick"
	DESCRIPTION_MENU, // "Description"
	TAG_MENU, // "Tag"
	CLIENT_MENU, // "Client"
	CONNECTION_MENU, // "Client"
	EMAIL_MENU, // "E-Mail"
	ANY_MENU, // "Any"
	UPDATE_IP, // "AutoUpdate IP on every startup"
	TRANSFERRED, // "Transferred"
	TTH_CHECKED, // "TTH Checked"
	FINISHED_MP3_DOWNLOADS, // "Downloaded MP3 Info"
	BITRATE, // "Bitrate"
	FREQUENCY, // "Frequency"
	MPEG_VER, // "MPEG Version"
	CHANNEL_MODE, // "Channel mode"
	MENU_TTH, // "Get TTH for file..."
	CHECK_TTH_AFTER_DOWNLOAD, // "Check TTH after downloading file"
	NUMBER_OF_SEGMENTS, // "segment(s)"
	ONLY_TTH, // "Only files with TTH"
	SETTINGS_ONLY_TTH, // "Search only files with TTH by default"
	TTH_INCONSISTENCY, // "TTH Inconsistency"
	CHECKING_TTH, // "Verifying TTH..."
	SETCZDC_MAX_EMOTICONS, // "Max Emoticons in one chat message"
	SETCZDC_CZCHARS_DISABLE, // "Remove diacritic marks from Czech chars"
	SETCZDC_MAGNET_URI_HANDLER, // "Install Magnet URI handler on startup"
	COPY_MAGNET_LINK, // "Copy magnet link to clipboard"
	SECONDS, // "Seconds"
	SETTINGS_DEBUG_COMMANDS, // "Debug commands"
	SETTINGS_SAVEQUEUE, // "Autosave queue every"
	HUB_SEGMENTS, // "Hub / Segments"
	SETTINGS_AUTO_PRIORITY_DEFAULT, // "Use Auto Priority by default"
	AUTO, // "Auto"	
	SEGMENTBAR, // "Segmented bar colors"
	SHOW_SEGMENT_PART, // "Show progressbar on inactive segment part"
	SEGMENT_PART_COLOR, // "Color of inactive part"
	STRING_NOT_FOUND, // "String not found: "
	SHOW_SPEED, // "Show speed"
	DU, // "Down/Up:"
	SETCZDC_PM_LOG, // "Load few last lines from log on new PM"
	SETCZDC_PM_LINES, // "Lines from log on new PM"
	SETTINGS_COLOR_ALTERNATE, // "Search Alternate"
	UPDATE_CHECK, // "Update check"
	HISTORY, // "History"
	CURRENT_VERSION, // "Current version"
	LATEST_VERSION, // "Latest version"
	CONNECTING_TO_SERVER, // "Connecting to server"
	RETRIEVING_DATA, // "Retrieving data"
	CONNECTION_ERROR, // "Connection error"
	DATA_RETRIEVED, // "Data retrieved"
	REMOVE_OFFLINE, // "Remove offline users"
	DISABLE_SOUNDS, // "Disable sounds"
	CONNECT_ALL, // "Connect to all users"
	DISCONNECT_ALL, // "Disconnect all users"
	REPORT_ALTERNATES, // "Report auto search for alternates in status bar"
	ALTERNATES_SEND, // "Send alternate search for: "
	COLLAPSE_ALL, // "Collapse All"
	EXPAND_ALL, // "Expand All"
	PB, // "PB"
	EB, // "EB"
	CHECK_ON_CONNECT, // "Check user on join"
	LAST // @DontAdd
};