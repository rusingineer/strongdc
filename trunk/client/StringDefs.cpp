#include "stdinc.h"
#include "DCPlusPlus.h"
#include "ResourceManager.h"
string ResourceManager::strings[] = {
"Active", 
"Active / Search String", 
"&Add", 
"Added", 
"Add To Favorites", 
"Address already in use", 
"Address not available", 
"Discard", 
"Automatic Directory Listing Search", 
"All download slots taken", 
"All %d users offline", 
"All 3 users offline", 
"All 4 users offline", 
"Any", 
"At least", 
"At most", 
"Audio", 
"Auto connect / Name", 
"Auto grant slot / Nick", 
"Average/s: ", 
"AWAY", 
"Away mode off", 
"Away mode on: ", 
"B", 
"Bitzi Lookup", 
"Both users offline", 
"Browse...", 
"&Browse...", 
"Choose folder", 
"Close connection", 
"Close", 
"Compressed", 
"Error during compression", 
"&Connect", 
"Connected", 
"Connecting...", 
"Connecting (forced)...", 
"Connecting to ", 
"Connection", 
"Connection closed", 
"Connection refused by target machine", 
"Connection reset by server", 
"Connection timeout", 
"Copy address to clipboard", 
"Copy nick", 
"Could not open target file: ", 
"Count", 
"CRC Checked", 
"Error during decompression", 
"Description", 
"Destination", 
"Directory", 
"Directory already shared", 
"Disconnected", 
"Disconnected user leaving the hub: ", 
"Disk full(?)", 
"Document", 
"The temporary download directory cannot be shared", 
"Download", 
"Download failed: ", 
"Download finished, idle...", 
"Download starting...", 
"Download to...", 
"Download queue", 
"Download whole directory", 
"Download whole directory to...", 
"Downloaded", 
"Downloaded %s (%.01f%%) in %s", 
" downloaded from ", 
"Downloading...", 
"Downloading public hub list...", 
"Duplicate file will not be shared: ", 
"Duplicate source", 
"E-Mail", 
"Please enter a nickname in the settings dialog!", 
"Enter search string", 
"Error opening file", 
"Errors", 
"Exact size", 
"Executable", 
"Favorite Hubs", 
"Favorite hub added", 
"Favorite Users", 
"Favorite user added", 
"Join/part of favorite users showing off", 
"Join/part of favorite users showing on", 
"File", 
"Files", 
"File list refresh finished", 
"File list refresh initiated", 
"File not available", 
"File type", 
"A file with a different size already exists in the queue", 
"A file with diffent tth root already exists in the queue", 
"Filename", 
"F&ilter", 
"Find", 
"Finished Downloads", 
"Finished Uploads", 
"Folder", 
"Directory with '$' cannot be downloaded and will not be shared: ", 
"File with '$' cannot be downloaded and will not be shared: ", 
"Force attempt", 
"GB", 
"Get file list", 
"Go to directory", 
"Grant extra slot (10 min)", 
"Hash database", 
"Hash database rebuilt", 
"Finished hashing ", 
"High", 
"Highest", 
"Hit Ratio: ", 
"Hits", 
"Hits: ", 
"Host unreachable", 
"Hub", 
"Hubs", 
"Address", 
"Hub list downloaded...", 
"Name", 
"Users", 
"Ignored message: ", 
"Invalid number of slots", 
"Invalid target file (missing directory, check default download directory setting)", 
"Downloaded tree does not match TTH root", 
"Ip: ", 
"Ip", 
"Items", 
"Joins: ", 
"Join/part showing off", 
"Join/part showing on", 
"kB", 
"kB/s", 
"Kick user(s)", 
"A file of equal or larger size already exists at the target location", 
"Last change: ", 
"Hub (last seen on if offline)", 
"Time last seen", 
"Loading StrongDC++, please wait...", 
"Low", 
"Lowest", 
"Manual connect address", 
"Match queue", 
"Matched %d file(s)", 
"MB", 
"MiB/s",
"&File", 
"ADL Search", 
"&Download Queue\tCtrl+D", 
"&Exit", 
"&Favorite Hubs\tCtrl+F", 
"Favorite &Users\tCtrl+U", 
"Follow last redirec&t\tCtrl+T", 
"Import queue from NMDC...", 
"Network Statistics", 
"&Notepad\tCtrl+N", 
"Open file list...", 
"&Public Hubs\tCtrl+P", 
"&Reconnect\tCtrl+R", 
"Refresh file list", 
"Show", 
"&Search\tCtrl+S", 
"Search spy", 
"Settings...", 
"&Help", 
"About StrongDC++", 
"StrongDC++ Discussion forum", 
"StrongDC++ Homepage", 
"Open downloads directory", 
"&View", 
"&Status bar\tCtrl+2", 
"&Toolbar\tCtrl+1", 
"T&ransfers", 
"&Window", 
"Arrange icons", 
"Cascade", 
"Horizontal Tile", 
"Vertical Tile", 
"Close disconnected", 
"Minimize &All", 
"Move/Rename", 
"Move &Down", 
"Move &Up", 
"Network Statistics", 
"Network unreachable (are you connected to the internet?)", 
"Next", 
"&New...", 
"Nick", 
"Your nick was already taken, please change to something else!", 
" (Nick unknown)", 
"Non-blocking operation still in progress", 
"Not connected", 
"Not a socket", 
"No", 
"No directory specified", 
"You're trying to download from yourself!", 
"No errors", 
"No matches", 
"No slots available", 
"No users", 
"No users to download from", 
"Normal", 
"Notepad", 
"Offline", 
"Online", 
"Only users with free slots", 
"Only where I'm op", 
"Open", 
"Open download page?", 
"Open folder", 
"Operation would block execution", 
"Out of buffer space", 
"Passive user", 
"Password", 
"Parts: ", 
"Path", 
"Paused", 
"Permission denied", 
"Picture", 
"Port: ", 
"Port %d is busy, please choose another one in the settings dialog, or disable any other application that might be using it and restart StrongDC++", 
"Press the follow redirect button to connect to ", 
"Priority", 
"Private message from ", 
"&Properties", 
"Public Hubs", 
"Ratio", 
"Readd source", 
"Really exit?", 
"Redirect request received to a hub that's already connected", 
"Redirect user(s)", 
"&Refresh", 
"Refresh user list", 
"&Remove", 
"Remove all", 
"Remove all subdirectories before adding this one", 
"Remove user from queue", 
"Remove source", 
"Rollback inconsistency, existing file does not match the one being downloaded", 
"Running...", 
"Search", 
"Search by TTH", 
"Search for", 
"Search for alternates", 
"Search for file", 
"Search options", 
"Search spam detected from ", 
"Search Spy", 
"Search String", 
"Specifying the same search string for more than 5 files for a passive connection or 10 files for an active connection is inefficient. Would you like to continue with the operation?", 
"Searching for ", 
"Send private message", 
"Separator", 
"Server", 
"Set priority", 
"&Add folder", 
"Break on first ADLSearch match", 
"Advanced", 
"Advanced settings", 
"Use antifragmentation method for downloads", 
"Appearance", 
"Auto-away on minimize (and back on restore)", 
"Automatically follow redirects", 
"Automatically disconnect users who leave the hub (does not disconnect when hub goes down / you leave it)", 
"Automatically search for alternative TTH source every 5 minutes", 
"Automatically match queue for auto search hits", 
"Use default search strings in auto search when no string is specified", 
"Automatically refresh share list every hour", 
"&Change", 
"Clear search box after each search", 
"Client version", 
"Enable safe and compressed transfers", 
"Command", 
"Confirm application exit", 
"Connection Settings (see the readme / newbie help if unsure)", 
"Connection Type", 
"Default away message", 
"Directories", 
"Default download directory", 
"Limits", 
"Downloads", 
"Maximum simultaneous downloads (0 = infinite)", 
"No new downloads if speed exceeds (kB/s, 0 = disable)", 
"Only show joins / parts for favorite users", 
"Filter kick and NMDC debug messages", 
"Set Finished Manager(s) tab bold when an entry is added", 
"Format", 
"General", 
"Accept custom user commands from hub", 
"Ignore messages from users that are not online (effective against bots)", 
"IP", 
"Don't delete file lists when exiting", 
"Language file", 
"Include duplicate files in your file list (duplicates never count towards your share size)"	, 
"Log directory", 
"Log downloads", 
"Log filelist transfers", 
"Log main chat", 
"Log private chat", 
"Log uploads", 
"Log system messages", 
"Logging", 
"Logs and Sound", 
"Max hash speed", 
"Max tab rows", 
"Minimize to tray", 
"Name", 
"Don't send the away message to bots", 
"Open the favorite hubs window at startup", 
"Open the finished downloads window at startup", 
"Open the public hubs window at startup", 
"Open the download queue window at startup", 
"Options", 
"Passive", 
"Personal Information", 
"Make an annoying sound every time a private message is received", 
"Make an annoying sound when a private message window is opened", 
"Popup messages from users that are not online (if not ignoring, messages go to main chat if enabled)", 
"Popup private messages", 
"Port (empty=random)", 
"Public Hubs list", 
"Public Hubs list URL", 
"HTTP Proxy (for hublist only)", 
"Set Download Queue tab bold when contents change", 
"Note; most of these options require that you restart StrongDC++", 
"Rollback", 
"&Text style", 
"&Window color", 
"Send unknown /commands to the hub", 
"Enable automatic SFV checking", 
"Share hidden files", 
"Total size:", 
"Shared directories", 
"Show joins / parts in chat by default", 
"Show progress bars for transfers (uses some CPU)", 
"Skip zero-byte files", 
"Use small send buffer (enable if uploads slow downloads a lot)", 
"SOCKS5", 
"Socks IP", 
"Port", 
"Use SOCKS5 server to resolve hostnames", 
"Username", 
"Sounds", 
"Note; because of changing download speeds, this is not 100% accurate...", 
"View status messages in main chat", 
"Tab completion of nicks in chat", 
"Show timestamps in chat by default", 
"Unfinished downloads directory (empty = download directly to target)", 
"Sharing", 
"Automatically open an extra slot if speed is below (0 = disable)", 
"Upload slots", 
"Install URL handler on startup (to handle dchub:// links)", 
"Use OEM monospaced font for viewing text files", 
"Use system icons when browsing files (slows browsing down a bit)", 
"User Commands", 
"User Menu Items", 
"Write buffer size", 
"Get User Country", 
"Log status messages", 
"Settings", 
"CRC32 inconsistency (SFV-Check)", 
"Shared", 
"Shared Files", 
"Size", 
"Max Size", 
"Min Size", 
"Slot granted", 
"Slots", 
"Slots set", 
"Socket has been shut down", 
"Socks server authentication failed (bad username / password?)", 
"The socks server doesn't support user / password authentication", 
"The socks server failed establish a connection", 
"The socks server requires authentication", 
"Failed to set up the socks server for UDP relay (check socks address and port)", 
"Source Type", 
"Specify a server to connect to", 
"Specify a search string", 
"Speed", 
"Status", 
"Stored password sent...", 
"Tag", 
"Target filename too long", 
"TB", 
"Time", 
"Time left", 
"Timestamps disabled", 
"Timestamps enabled", 
"Total: ", 
"TTH Root", 
"Type", 
"Unable to create thread", 
"Unknown", 
"Unknown address", 
"Unknown command: ", 
"Unknown error: 0x%x", 
"Unsupported filelist format", 
"Upload finished, idle...", 
"Upload starting...", 
"Uploaded %s (%.01f%%) in %s", 
" uploaded to ", 
"User", 
"User Description", 
"User offline", 
"User went offline", 
"Users", 
"Video", 
"View as text", 
"Waiting (User online)", 
"Waiting (%d of %d users online)", 
"Waiting to retry...", 
"What's &this?", 
"Yes", 
"Copy email address", 
"Copy description", 
"Send public message", 
"Extra slots set", 
"Small file size set", 
"Invalid size", 
"StrongDC++", 
"Colors & Fonts", 
"Kick user(s) with filename", 
"Country", 
"Min share", 
"Min slots", 
"Max hubs", 
"Max users", 
"Reliability", 
"Rating", 
"Client", 
"Version", 
"Mode", 
"Upload Limit", 
"Grant extra slot (hour)", 
"Grant extra slot (day)", 
"Grant extra slot (week)", 
"Copy", 
"Copy actual line", 
"Copy URL", 
"Select all", 
"Clear chat", 
"Autoscrool chat", 
"Upload Queue", 
"Extra Slots", 
"-- WARNING --\r\n-- You are in active mode, but have activated passive search. --\r\n-- Don't use passive search if you have search results without this option --\r\n-- because you don't get search result from passive clients !!! --\r\n-- Turn it off in settings => StrongDC++ => Always use Passive Mode for Search --\r\n", 
"StrongDC++ Features", 
"Send StrongDC++ tag in description (always send if more than 10 hubs)", 
"Get information on new dc++ versions", 
"PoPup filelists", 
"Filter Duplicate Search Results", 
"Remove Forbidden (Unfinished Kazaa, Win MX, GetRight, eMule, StrongDC++)", 
"Private message sound", 
"Enable Transfer Rate Limiting", 
"Transfer Rate Limiting", 
"Upload Speed", 
"Download Speed", 
"Use Alternate Limiting from", 
"to", 
"Secondary Transfer Rate Limiting", 
"* Upload limit for first slot minimal 5 kB/s. Next slots 2 kB/s per slot !!!", 
"* If upload limit is set, download limit is max 6 x upload limit !!!", 
"Always use passive mode for Search (Use only if you know what this doing !!!)", 
"Progressbar colors", 
"Upload", 
"Progressbar text colors", 
"Winamp", 
"Available styles", 
"Back color", 
"Text color", 
"Text style", 
"Default styles", 
"Black & white", 
"Bold Authors of messages in chat", 
"Preview", 
"Select user in list", 
"Small Upload Slots", 
"Small file size", 
"* Small Uploads Slots => Slots for filelist and Small Files.", 
"Download with priority...", 
"Readd all sources", 
"Active Search", 
"Destination Directory", 
"Max Filesize", 
"Min Filesize", 
"ADLSearch Properties", 
"Size Type", 
"Search String", 
"Source Type", 
"Cancel", 
"OK", 
"Show InfoTips in lists", 
"User went online", 
"Highest Priority Extra Download Slots", 
"Copy TAG", 
"Transfers", 
"Open the network statistic window at startup", 
"Minimize at program startup", 
"Confirm items removing from queue", 
"Search only users with free slots by default", 
"Open hub log", 
"Open user log", 
"User log not exist", 
"Hub log not exist", 
"Open downloads log", 
"Downloads log not exist", 
"Open uploads log", 
"Uploads log not exist", 
"Whois ", 
"Use file extension for Download to in search", 
"Error color", 
"Enable Shared column in userlist", 
"Enable Exact Shared column in userlist", 
"Enable Description column in userlist", 
"Enable Tag column in userlist", 
"Enable Connection column in userlist", 
"Enable Email column in userlist", 
"Enable Client column in userlist", 
"Enable Version column in userlist", 
"Enable Mode column in userlist", 
"Enable Hubs column in userlist", 
"Enable Slots column in userlist", 
"Enable Upload column in userlist", 
"Automatically expand folders in Queue", 
"Remove extra slot", 
"Download finished: %s", 
"Exact Share", 
"Copy Exact Share", 
"Enable IP column in userlist", 
"Copy IP", 
"Slow user", 
"Limits", 
"Minutes", 
"Disconnecting slow downloads", 
"Disconnect download if speed is below", 
"More than", 
"And all downloads not exceeds", 
"kB/s (0 = disable)", 
"ISP", 
"Override system colors", 
"Override system colors", 
"Menu bar", 
"Left color", 
"Right color", 
"Two colors", 
"Bumped", 
"Ignore User", 
"Unignore User", 
"Winamp Help", 
"/winamp - Works with 1.x, 2.x, 5.x (no WinAmp 3 support)\r\n- %[version]	Numerical Version (ex: 2.91)\r\n- %[state]	Playing state (ex: stopped/paused/playing)\r\n- %[title]		Window title from Winamp - if you want to change this for mp3s, Winamp > Pref > Input > MPEG > Title\r\n- %[rawtitle]	Window title from Winamp (if %[title] not working propertly)\r\n- %[percent]	Percentage (ex. 40%)\r\n- %[length]	Length in minutes:seconds (ex: 04:09)\r\n- %[elapsed]	Time elapsed in minutes:seconds (ex. 03:51)\r\n- %[bar]		ASCII progress bar, 10 characters wide no including brackets (ex. [----|-----])\r\n- %[bitrate]	Bitrate (ex. 128kbps)\r\n- %[sample]	Sample frequency (ex. 22kHz)\r\n- %[channels]	Number of channels (ex. stereo / mono)\r\nEmpty = Default String -> winamp(%[version]) %[state](%[title]) stats(%[percent] of %[length] %[bar])", 
"Enable slow downloads disconnecting", 
"And file size is more than", 
"Remove slow user from Queue", 
"Upload Speed", 
"Sounds", 
"Download begins", 
"Download finished", 
"Alternate source added", 
"Upload finished", 
"Faker found", 
"Open X extra slots for each hub (when hubs = 2 or more)", 
"Segment Downloading", 
"Automatically do Accelerated (Segmented) downloading", 
"2 segments if file size is between                 and                  MB", 
"3 segments if file size is between                 and                  MB", 
"4 segments if file size is between                 and                  MB", 
"6 segments if file size is between                 and                  MB", 
"8 segments if file size is greater than                  MB", 
"Don't add any other segment if speed is above", 
"(Too low or too high settings can decrease download speed!)", 
"Disable safe && compressed transfer", 
"Max compression level", 
"Empty process working set (experimental).", 
"Use image cache (lowers CPU usage, but might distort the graphics)", 
"Number of segments based on the file size", 
"Number of segments based on the connection type", 
"Manual settings of number of segments", 
"Don't use segment downloading for this filetypes:\n(use regular expressions)", 
"Minimum download block (segment) size", 
"Fake detector", 
"Progressbar colors", 
"File Preview", 
"Hashing", 
"Ignore junk files altogether", 
"Set size to zero to disable, otherwise files larger than this will be considered junk files", 
"Junk file size", 
"BIN file size", 
"VOB file size", 
"Percent fake share accepted", 
"Arguments", 
"Extensions", 
"Preview", 
"Max number of alternate sources", 
"Client Emulation", 
"Get IP Address", 
"Shutdown computer", 
"Shutdown sequence deactivated...", 
"Shutdown sequence activated...", 
"Failed to shutdown!", 
"Shutdown timeout (in seconds)", 
"Running... (user online)", 
"Running... (%d of %d users online)", 
"Recent Hubs", 
"&Recent Hubs", 
"Enable ISP column in userlist", 
"Copy All", 
"Copy Nick+IP", 
"Copy Nick+IP", 
"Toolbar", 
"Toolbar Images", 
"Add -->", 
"<-- Remove", 
"Mouse Over", 
"Normal", 
"The Following Directories are automatically appended to the \"Download to...\" menu.  When an extension is supplied it becomes the default download directory for that type of file.", 
"Directories", 
"PK String", 
"Lock", 
"Supports", 
"Enable PKString column in userlist", 
"Enable Lock column in userlist", 
"Enable Supports column in userlist", 
"Get user response", 
"Report user", 
"Userlist Icons", 
"Clients", 
"Enable emoticons", 
"Mismatched share size - ", 
"zero bytes real size", 
"filelist was inflated %s times", 
", stated size = %[statedshareformat], real size = %[realshareformat]", 
"Junk files found - %[junkshareformat] was junk, Stated share = %[statedshareformat], Real share = %[realshareformat].", 
"Remove user from queue, if speed is below", 
"CDM Debug Messages", 
"Get MP3 Info", 
"Check File List", 
"Exactly", 
"File was corrupted, redownloading in %d seconds", 
"Segment Block Finished, waiting...", 
"Stop Search", 
"Search stopped", 
"My nick in mainchat", 
"Nick", 
"Description", 
"Tag", 
"Client", 
"Client", 
"E-Mail", 
"Any", 
"AutoUpdate IP on every startup", 
"Transferred", 
"TTH Checked", 
"Downloaded MP3 Info", 
"Bitrate", 
"Frequency", 
"MPEG Version", 
"Channel mode", 
"Get TTH for file...", 
"Check TTH after downloading file", 
"segment(s)", 
"Only files with TTH", 
"Search only files with TTH by default", 
"TTH Inconsistency", 
"Verifying TTH...", 
"Max Emoticons in one chat message", 
"Remove diacritic marks from Czech chars", 
"Install Magnet URI handler on startup", 
"Copy magnet link to clipboard", 
"Seconds", 
"Debug commands", 
"Autosave queue every", 
"Hub / Segments", 
"Use Auto Priority by default", 
"Auto"	, 
"Segmented bar colors", 
"Show progressbar on inactive segment part", 
"Color of inactive part", 
"String not found: ", 
"Show speed", 
"Down/Up:",
"Load few last lines from log on new PM", 
"Lines from log on new PM",
"Search Alternate",
"Update check",
"History",
"Current version",
"Latest version",
"Connecting to server",
"Retrieving data",
"Connection error",
"Data retrieved",
"Remove offline users",
"Disable sounds",
"Connect to all users",
"Disconnect all users",
"Report auto search for alternates in status bar", 
"Searching TTH alternates for: ",
"Collapse All",
"Expand All"
};
string ResourceManager::names[] = {
"Active", 
"ActiveSearchString", 
"Add", 
"Added", 
"AddToFavorites", 
"AddressAlreadyInUse", 
"AddressNotAvailable", 
"AdlDiscard", 
"AdlSearch", 
"AllDownloadSlotsTaken", 
"AllUsersOffline", 
"All3UsersOffline", 
"All4UsersOffline", 
"Any", 
"AtLeast", 
"AtMost", 
"Audio", 
"AutoConnect", 
"AutoGrant", 
"Average", 
"Away", 
"AwayModeOff", 
"AwayModeOn", 
"B", 
"BitziLookup", 
"BothUsersOffline", 
"Browse", 
"BrowseAccel", 
"ChooseFolder", 
"CloseConnection", 
"Close", 
"Compressed", 
"CompressionError", 
"Connect", 
"Connected", 
"Connecting", 
"ConnectingForced", 
"ConnectingTo", 
"Connection", 
"ConnectionClosed", 
"ConnectionRefused", 
"ConnectionReset", 
"ConnectionTimeout", 
"CopyHub", 
"CopyNick", 
"CouldNotOpenTargetFile", 
"Count", 
"CrcChecked", 
"DecompressionError", 
"Description", 
"Destination", 
"Directory", 
"DirectoryAlreadyShared", 
"Disconnected", 
"DisconnectedUser", 
"DiscFull", 
"Document", 
"DontShareTempDirectory", 
"Download", 
"DownloadFailed", 
"DownloadFinishedIdle", 
"DownloadStarting", 
"DownloadTo", 
"DownloadQueue", 
"DownloadWholeDir", 
"DownloadWholeDirTo", 
"Downloaded", 
"DownloadedBytes", 
"DownloadedFrom", 
"Downloading", 
"DownloadingHubList", 
"DuplicateFileNotShared", 
"DuplicateSource", 
"Email", 
"EnterNick", 
"EnterSearchString", 
"ErrorOpeningFile", 
"Errors", 
"ExactSize", 
"Executable", 
"FavoriteHubs", 
"FavoriteHubAdded", 
"FavoriteUsers", 
"FavoriteUserAdded", 
"FavJoinShowingOff", 
"FavJoinShowingOn", 
"File", 
"Files", 
"FileListRefreshFinished", 
"FileListRefreshInitiated", 
"FileNotAvailable", 
"FileType", 
"FileWithDifferentSize", 
"FileWithDifferentTth", 
"Filename", 
"Filter", 
"Find", 
"FinishedDownloads", 
"FinishedUploads", 
"Folder", 
"ForbiddenDollarDirectory", 
"ForbiddenDollarFile", 
"ForceAttempt", 
"Gb", 
"GetFileList", 
"GoToDirectory", 
"GrantExtraSlot", 
"HashDatabase", 
"HashRebuilt", 
"HashingFinished", 
"High", 
"Highest", 
"HitRatio", 
"HitCount", 
"Hits", 
"HostUnreachable", 
"Hub", 
"Hubs", 
"HubAddress", 
"HubListDownloaded", 
"HubName", 
"HubUsers", 
"IgnoredMessage", 
"InvalidNumberOfSlots", 
"InvalidTargetFile", 
"InvalidTree", 
"Ip", 
"IpBare", 
"Items", 
"Joins", 
"JoinShowingOff", 
"JoinShowingOn", 
"Kb", 
"Kbps", 
"KickUser", 
"LargerTargetFileExists", 
"LastChange", 
"LastHub", 
"LastSeen", 
"Loading", 
"Low", 
"Lowest", 
"ManualAddress", 
"MatchQueue", 
"MatchedFiles", 
"Mb", 
"Mbps", 
"MenuFile", 
"MenuAdlSearch", 
"MenuDownloadQueue", 
"MenuExit", 
"MenuFavoriteHubs", 
"MenuFavoriteUsers", 
"MenuFollowRedirect", 
"MenuImportQueue", 
"MenuNetworkStatistics", 
"MenuNotepad", 
"MenuOpenFileList", 
"MenuPublicHubs", 
"MenuReconnect", 
"MenuRefreshFileList", 
"MenuShow", 
"MenuSearch", 
"MenuSearchSpy", 
"MenuSettings", 
"MenuHelp", 
"MenuAbout", 
"MenuDiscuss", 
"MenuHomepage", 
"MenuOpenDownloadsDir", 
"MenuView", 
"MenuStatusBar", 
"MenuToolbar", 
"MenuTransfers", 
"MenuWindow", 
"MenuArrange", 
"MenuCascade", 
"MenuHorizontalTile", 
"MenuVerticalTile", 
"MenuCloseDisconnected", 
"MenuMinimizeAll", 
"Move", 
"MoveDown", 
"MoveUp", 
"NetworkStatistics", 
"NetworkUnreachable", 
"Next", 
"New", 
"Nick", 
"NickTaken", 
"NickUnknown", 
"NonBlockingOperation", 
"NotConnected", 
"NotSocket", 
"No", 
"NoDirectorySpecified", 
"NoDownloadsFromSelf", 
"NoErrors", 
"NoMatches", 
"NoSlotsAvailable", 
"NoUsers", 
"NoUsersToDownloadFrom", 
"Normal", 
"Notepad", 
"Offline", 
"Online", 
"OnlyFreeSlots", 
"OnlyWhereOp", 
"Open", 
"OpenDownloadPage", 
"OpenFolder", 
"OperationWouldBlockExecution", 
"OutOfBufferSpace", 
"PassiveUser", 
"Password", 
"Parts", 
"Path", 
"Paused", 
"PermissionDenied", 
"Picture", 
"Port", 
"PortIsBusy", 
"PressFollow", 
"Priority", 
"PrivateMessageFrom", 
"Properties", 
"PublicHubs", 
"Ratio", 
"ReaddSource", 
"ReallyExit", 
"RedirectAlreadyConnected", 
"RedirectUser", 
"Refresh", 
"RefreshUserList", 
"Remove", 
"RemoveAll", 
"RemoveAllSubdirectories", 
"RemoveFromAll", 
"RemoveSource", 
"RollbackInconsistency", 
"Running", 
"Search", 
"SearchByTth", 
"SearchFor", 
"SearchForAlternates", 
"SearchForFile", 
"SearchOptions", 
"SearchSpamFrom", 
"SearchSpy", 
"SearchString", 
"SearchStringInefficient", 
"SearchingFor", 
"SendPrivateMessage", 
"Separator", 
"Server", 
"SetPriority", 
"SettingsAddFolder", 
"SettingsAdlsBreakOnFirst", 
"SettingsAdvanced", 
"SettingsAdvancedSettings", 
"SettingsAntiFrag", 
"SettingsAppearance", 
"SettingsAutoAway", 
"SettingsAutoFollow", 
"SettingsAutoKick", 
"SettingsAutoSearch", 
"SettingsAutoSearchAutoMatch", 
"SettingsAutoSearchAutoString", 
"SettingsAutoUpdateList", 
"SettingsChange", 
"SettingsClearSearch", 
"SettingsClientVer", 
"SettingsCompressTransfers", 
"SettingsCommand", 
"SettingsConfirmExit", 
"SettingsConnectionSettings", 
"SettingsConnectionType", 
"SettingsDefaultAwayMsg", 
"SettingsDirectories", 
"SettingsDownloadDirectory", 
"SettingsDownloadLimits", 
"SettingsDownloads", 
"SettingsDownloadsMax", 
"SettingsDownloadsSpeedPause", 
"SettingsFavShowJoins", 
"SettingsFilterMessages", 
"SettingsFinishedDirty", 
"SettingsFormat", 
"SettingsGeneral", 
"SettingsHubUserCommands", 
"SettingsIgnoreOffline", 
"SettingsIp", 
"SettingsKeepLists", 
"SettingsLanguageFile", 
"SettingsListDupes", 
"SettingsLogDir", 
"SettingsLogDownloads", 
"SettingsLogFilelistTransfers", 
"SettingsLogMainChat", 
"SettingsLogPrivateChat", 
"SettingsLogUploads", 
"SettingsLogSystemMessages", 
"SettingsLogging", 
"SettingsLogs", 
"SettingsMaxHashSpeed", 
"SettingsMaxTabRows", 
"SettingsMinimizeTray", 
"SettingsName", 
"SettingsNoAwaymsgToBots", 
"SettingsOpenFavoriteHubs", 
"SettingsOpenFinishedDownloads", 
"SettingsOpenPublic", 
"SettingsOpenQueue", 
"SettingsOptions", 
"SettingsPassive", 
"SettingsPersonalInformation", 
"SettingsPmBeep", 
"SettingsPmBeepOpen", 
"SettingsPopupOffline", 
"SettingsPopupPms", 
"SettingsPort", 
"SettingsPublicHubList", 
"SettingsPublicHubListUrl", 
"SettingsPublicHubListHttpProxy", 
"SettingsQueueDirty", 
"SettingsRequiresRestart", 
"SettingsRollback", 
"SettingsSelectTextFace", 
"SettingsSelectWindowColor", 
"SettingsSendUnknownCommands", 
"SettingsSfvCheck", 
"SettingsShareHidden", 
"SettingsShareSize", 
"SettingsSharedDirectories", 
"SettingsShowJoins", 
"SettingsShowProgressBars", 
"SettingsSkipZeroByte", 
"SettingsSmallSendBuffer", 
"SettingsSocks5", 
"SettingsSocks5Ip", 
"SettingsSocks5Port", 
"SettingsSocks5Resolve", 
"SettingsSocks5Username", 
"SettingsSounds", 
"SettingsSpeedsNotAccurate", 
"SettingsStatusInChat", 
"SettingsTabCompletion", 
"SettingsTimeStamps", 
"SettingsUnfinishedDownloadDirectory", 
"SettingsUploads", 
"SettingsUploadsMinSpeed", 
"SettingsUploadsSlots", 
"SettingsUrlHandler", 
"SettingsUseOemMonofont", 
"SettingsUseSystemIcons", 
"SettingsUserCommands", 
"SettingsUserMenu", 
"SettingsWriteBuffer", 
"SettingsGetUserCountry", 
"SettingsLogStatusMessages", 
"Settings", 
"SfvInconsistency", 
"Shared", 
"SharedFiles", 
"Size", 
"SizeMax", 
"SizeMin", 
"SlotGranted", 
"Slots", 
"SlotsSet", 
"SocketShutDown", 
"SocksAuthFailed", 
"SocksAuthUnsupported", 
"SocksFailed", 
"SocksNeedsAuth", 
"SocksSetupError", 
"SourceType", 
"SpecifyServer", 
"SpecifySearchString", 
"Speed", 
"Status", 
"StoredPasswordSent", 
"Tag", 
"TargetFilenameTooLong", 
"Tb", 
"Time", 
"TimeLeft", 
"TimestampsDisabled", 
"TimestampsEnabled", 
"Total", 
"TthRoot", 
"Type", 
"UnableToCreateThread", 
"Unknown", 
"UnknownAddress", 
"UnknownCommand", 
"UnknownError", 
"UnsupportedFilelistFormat", 
"UploadFinishedIdle", 
"UploadStarting", 
"UploadedBytes", 
"UploadedTo", 
"User", 
"UserDescription", 
"UserOffline", 
"UserWentOffline", 
"Users", 
"Video", 
"ViewAsText", 
"WaitingUserOnline", 
"WaitingUsersOnline", 
"WaitingToRetry", 
"WhatsThis", 
"Yes", 
"CopyEmailAddress", 
"CopyDescription", 
"SendPublicMessage", 
"ExtraSlotsSet", 
"SmallFileSizeSet", 
"InvalidSize", 
"SettingsCzdc", 
"SettingsTextStyles", 
"KickUserFile", 
"Country", 
"Minshare", 
"Minslots", 
"Maxhubs", 
"Maxusers", 
"Reliability", 
"Rating", 
"Clientid", 
"Version", 
"Mode", 
"Limit", 
"GrantExtraSlotHour", 
"GrantExtraSlotDay", 
"GrantExtraSlotWeek", 
"Copy", 
"CopyLine", 
"CopyUrl", 
"SelectAll", 
"Clear", 
"AscrollChat", 
"UploadQueue", 
"GrantSlotsMenu", 
"AntiPassiveSearch", 
"SetczdcFeat", 
"SetczdcSendTag", 
"SetczdcGetDcUpdate", 
"SetczdcPopupFilelist", 
"SetczdcFilterSearch", 
"SetczdcRemoveForbidden", 
"SetczdcPrivateSound", 
"SetczdcEnableLimiting", 
"SetczdcTransferLimiting", 
"SetczdcUploadSpeed", 
"SetczdcDownloadSpeed", 
"SetczdcAlternateLimiting", 
"SetczdcTo", 
"SetczdcSecondaryLimiting", 
"SetczdcNoteUpload", 
"SetczdcNoteDownload", 
"SetczdcPassiveSearch", 
"SetczdcProgressbarColors", 
"SetczdcUpload", 
"SetczdcProgressbarText", 
"SetczdcWinamp", 
"SetczdcStyles", 
"SetczdcBackColor", 
"SetczdcTextColor", 
"SetczdcTextStyle", 
"SetczdcDefaultStyle", 
"SetczdcBlackWhite", 
"SetczdcBold", 
"SetczdcPreview", 
"SelectUserList", 
"SetczdcSmallUpSlots", 
"SetczdcSmallFiles", 
"SetczdcNoteSmallUp", 
"DownloadWithPriority", 
"ReaddAllSources", 
"AdlsearchActiveSearch", 
"AdlsearchDestinationDirectory", 
"AdlsearchMaxSize", 
"AdlsearchMinSize", 
"AdlsearchProperties", 
"AdlsearchSizeType", 
"AdlsearchSearchString", 
"AdlsearchSourceType", 
"Cancel", 
"Ok", 
"SettingsShowInfoTips", 
"UserWentOnline", 
"SettingsCzdcExtraDownloads", 
"CopyTag", 
"MenuViewTransfers", 
"SettingsOpenNetworkStatistic", 
"SettingsMinimizeOnStartup", 
"SettingsConfirmDelete", 
"SettingsFreeSlotDefault", 
"OpenHubLog", 
"OpenUserLog", 
"NoLogForUser", 
"NoLogForHub", 
"OpenDownloadLog", 
"NoDownloadLog", 
"OpenUploadLog", 
"NoUploadLog", 
"WhoIs", 
"SettingsUseExtensionDownto", 
"SetczdcErrorColor", 
"SettingsShowShared", 
"SettingsShowExactShared", 
"SettingsShowDescription", 
"SettingsShowTag", 
"SettingsShowConnection", 
"SettingsShowEmail", 
"SettingsClient", 
"SettingsShowVersion", 
"SettingsShowMode", 
"SettingsShowHubs", 
"SettingsShowSlots", 
"SettingsShowUpload", 
"SettingsExpandQueue", 
"RemoveExtraSlot", 
"FinishedDownload", 
"ExactShared", 
"CopyExactShare", 
"SettingsShowIp", 
"CopyIp", 
"SlowUser", 
"SettingsLimit", 
"Minutes", 
"SetczdcSlowDisconnect", 
"SetczdcIDownSpeed", 
"SetczdcTimeDown", 
"SetczdcHDownSpeed", 
"KbpsDisable", 
"Isp", 
"SettingsZdcProgressOverride", 
"SettingsZdcProgressOverride2", 
"Menubar", 
"LeftColor", 
"RightColor", 
"TwoColors", 
"Bumped", 
"IgnoreUser", 
"UnignoreUser", 
"WinampHelpDesc", 
"WinampHelp", 
"SetczdcDisconnectingEnable", 
"SetczdcMinFileSize", 
"SetczdcRemoveSlowUser", 
"AverageUpload", 
"SettingsSound", 
"SoundDownloadBegins", 
"SoundDownloadFinished", 
"SoundSourceAdded", 
"SoundUploadFinished", 
"SoundFakerFound", 
"ExtraHubSlots", 
"SettingsSegment", 
"AutoSegmentText", 
"Segment2Text", 
"Segment3Text", 
"Segment4Text", 
"Segment6Text", 
"Segment8Text", 
"DontAddSegmentText", 
"WarnSegmentText", 
"DisableCompression", 
"SettingsMaxCompress", 
"SettingsEmptyWorkingSet", 
"SettingsOdcFloodCache", 
"TextFilesize", 
"TextConnection", 
"TextManual", 
"TextExtension", 
"TextMinimum", 
"SettingsFakedetect", 
"SettingsOperacolors", 
"SettingsAvipreview", 
"CreatingHash", 
"TextIgnoreJunk", 
"TextFakeinfo", 
"TextJunk", 
"TextBinjunk", 
"TextVobjunk", 
"TextFakepercent", 
"SettingsArgument", 
"SettingsExtensions", 
"PreviewMenu", 
"SettingsMaxSources", 
"ClientEmu", 
"GetIp", 
"Shutdown", 
"ShutdownOff", 
"ShutdownOn", 
"FailedToShutdown", 
"SettingsOdcShutdowntimeout", 
"UserOnline", 
"UsersOnline", 
"RecentHubs", 
"MenuFileRecentHubs", 
"ShowIsp", 
"CopyAll", 
"CopyNickIp", 
"CopyIsp", 
"SettingsToolbar", 
"SettingsToolbarImage", 
"SettingsToolbarAdd", 
"SettingsToolbarRemove", 
"SettingsMouseOver", 
"SettingsNormal", 
"SettingsDirsDescription", 
"SettingsDownloaddirs", 
"Pk", 
"Lock", 
"Supports", 
"ShowPk", 
"ShowLock", 
"ShowSupports", 
"GetUserResponses", 
"Report", 
"UserlistIcons", 
"SettingsClients", 
"EnableEmoticons", 
"CheckMismatchedShareSize", 
"Check0byteShare", 
"CheckInflated", 
"CheckShowRealShare", 
"CheckJunkFiles", 
"NewDisconnect", 
"MenuCdmdebugMessages", 
"GetMp3info", 
"CheckFilelist", 
"SizeExact", 
"DownloadCorrupted", 
"BlockFinished", 
"StopSearch", 
"SearchStopped", 
"MynickInChat", 
"NickMenu", 
"DescriptionMenu", 
"TagMenu", 
"ClientMenu", 
"ConnectionMenu", 
"EmailMenu", 
"AnyMenu", 
"UpdateIp", 
"Transferred", 
"TthChecked", 
"FinishedMp3Downloads", 
"Bitrate", 
"Frequency", 
"MpegVer", 
"ChannelMode", 
"MenuTth", 
"CheckTthAfterDownload", 
"NumberOfSegments", 
"OnlyTth", 
"SettingsOnlyTth", 
"TthInconsistency", 
"CheckingTth", 
"SetczdcMaxEmoticons", 
"SetczdcCzcharsDisable", 
"SetczdcMagnetUriHandler", 
"CopyMagnetLink", 
"Seconds", 
"SettingsDebugCommands", 
"SettingsSavequeue", 
"HubSegments", 
"SettingsAutoPriorityDefault", 
"Auto", 
"Segmentbar", 
"ShowSegmentPart", 
"SegmentPartColor", 
"StringNotFound", 
"ShowSpeed", 
"Du",
"SetczdcPmLog",
"SetczdcPmLines",
"SettingsColorAlternate",
"UpdateCheck",
"History",
"CurrentVersion",
"LatestVersion",
"ConnectingToServer",
"RetrievingData",
"ConnectionError",
"DataRetrieved",
"RemoveOffline",
"DisableSounds",
"ConnectAll",
"DisconnectAll",
"ReportAlternates", 
"AlternatesSend",
"CollapseAll",
"ExpandAll"
};
