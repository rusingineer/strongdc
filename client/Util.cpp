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

#include "Util.h"
#include "File.h"

#include "SettingsManager.h"
#include "ResourceManager.h"
#include "StringTokenizer.h"
#include "SettingsManager.h"
#include "version.h"

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/utsname.h>
#include <ctype.h>
#endif
#include <locale.h>

#include "CID.h"

#include "FastAlloc.h"

#ifndef _DEBUG
FastCriticalSection FastAllocBase::cs;
#endif

int64_t Util::mUptimeSeconds = 0;
string Util::emptyString;
wstring Util::emptyStringW;
tstring Util::emptyStringT;

bool Util::away = false;
string Util::awayMsg;
time_t Util::awayTime;

Util::CountryList Util::countries;
string Util::appPath;

bool Util::nlfound = false;
int Util::nlspeed;
static void sgenrand(unsigned long seed);

int arrayutf[96] = {-61, -127, -60, -116, -60, -114, -61, -119, -60, -102, -61, -115, -60, -67, -59, -121, -61, -109, -59, -104, -59, -96, -59, -92, -61, -102, -59, -82, -61, -99, -59, -67, -61, -95, -60, -115, -60, -113, -61, -87, -60, -101, -61, -83, -60, -66, -59, -120, -61, -77, -59, -103, -59, -95, -59, -91, -61, -70, -59, -81, -61, -67, -59, -66, -61, -124, -61, -117, -61, -106, -61, -100, -61, -92, -61, -85, -61, -74, -61, -68, -61, -76, -61, -108, -60, -71, -60, -70, -60, -67, -60, -66, -59, -108, -59, -107}; 
int arraywin[48] = {65, 67, 68, 69, 69, 73, 76, 78, 79, 82, 83, 84, 85, 85, 89, 90, 97, 99, 100, 101, 101, 105, 108, 110, 111, 114, 115, 116, 117, 117, 121, 122, 65, 69, 79, 85, 97, 101, 111, 117, 111, 111, 76, 108, 76, 108, 82, 114}; 

string disableCzChars(string message) { 
   string s = ""; 

   for(int j = 0; j < message.length(); j++) { 
      int zn = (int)message[j]; 
      int zzz = -1; 
      for(int l = 0; l < 96; l+=2) { 
         int zn2 = (int)message[j+1]; 
         if ((zn == arrayutf[l])&&(zn2 == arrayutf[l+1])) { 
            zzz = (int)(l/2); 
            break; 
         } 
      } 
      if (zzz >= 0) { 
         s += (char)(arraywin[zzz]); 
      } else { 
         s += message[j]; 
      } 
   } 

   return s; 
}

BOOL CALLBACK GetWOkna(HWND handle, LPARAM lparam) {
	TCHAR buf[256];
	buf[0] = NULL;
	if (!handle) {
		Util::nlfound = false;
		return TRUE;// Not a window
	}
	SendMessageTimeout(handle, WM_GETTEXT, 255, (LPARAM)buf, SMTO_ABORTIFHUNG | SMTO_BLOCK, 500, NULL);
	buf[255] = NULL;

	if(buf[0] != NULL) {
		if(_tcsnicmp(buf, _T("NetLimiter"), 10) == 0/* || _tcsnicmp(buf, _T("DU Super Controler"), 18) == 0*/) {
			Util::nlfound = true;
			return false;
		}
	}

	Util::nlfound = false;
	return true;
}

unsigned char HEX_2_INT_TABLE[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 
            6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int hexstr2int(char *hexstr) {
    register unsigned int length, i, value, shift;
    for (length = 0; length < 9; length++) if (!hexstr[length]) break;
    shift = (length - 1) * 4;
    for (i = value = 0; i < length; i++, shift -= 4)
        value += HEX_2_INT_TABLE[(unsigned int)hexstr[i] & 127] << shift;
    return value;
}

void Util::initialize() {
	setlocale(LC_ALL, "");

	Text::initialize();

	sgenrand((unsigned long)time(NULL));

#ifdef _WIN32
	TCHAR buf[MAX_PATH+1];
	GetModuleFileName(NULL, buf, MAX_PATH);
	appPath = Text::fromT(buf);
	appPath.erase(appPath.rfind('\\') + 1);
#else // _WIN32
	char* home = getenv("HOME");
	if (home) {
		appPath = Text::fromT(home);
		appPath += "/.strongdc++/";
	}
#endif // _WIN32

	try {
		// This product includes GeoIP data created by MaxMind, available from http://maxmind.com/
		// Updates at http://www.maxmind.com/app/geoip_country
		string file = Util::getAppPath() + SETTINGS_DIR + "GeoIpCountryWhois.csv";
		string data = File(file, File::READ, File::OPEN).read();

		const char* start = data.c_str();
		string::size_type i = 0;
		string::size_type j = 0;
		string::size_type k = 0;
		CountryIter last = countries.end();

		for(;;) {
			i = data.find(',', k);
			if(i == string::npos)
				break;

			j = data.find('\n', i);
			if(j == string::npos)
				break;

			u_int16_t* country = (u_int16_t*)(start + i + 1);
			last = countries.insert(last, make_pair(Util::toUInt32(start + k), *country));

			k = j + 1;
		}
	} catch(const FileException&) {
		// ...
	}

	try {
		TCHAR AppData[256];
		GetEnvironmentVariable(_T("APPDATA"), AppData, 255);

		File f(Text::fromT(AppData) + "\\LockTime\\NetLimiter\\history\\apphist.dat", File::RW, File::OPEN);

		int NetLimiter_UploadLimit = 0;
		int NetLimiter_UploadOn = 0;
		const size_t BUF_SIZE = 800;
		string cesta = Util::getAppName()+"/";
		char buf[BUF_SIZE];
		u_int32_t len;
		char* w2 = strdup(cesta.c_str());

		for(;;) {
			size_t n = BUF_SIZE;
			len = f.read(buf, n);
			string txt = "";
			for(int i = 0; i<len; ++i) {
				if (buf[i]== 0) 
				txt += "/"; else
				txt += buf[i];
			}
			char* w1 = strdup(txt.c_str());

			if(::strstr(strupr(w1),strupr(w2)) != NULL) {
				char buf1[256];
				char buf2[256];

				_snprintf(buf1, 255, "%X", u_int8_t(buf[5]));
				buf1[255] = 0;
				string a1 = buf1;

				_snprintf(buf2, 255, "%X", u_int8_t(buf[6]));
				buf2[255] = 0;
				string a2 = buf2;

				string limit_hex = "0x" + a2 + a1;

				NetLimiter_UploadLimit = 0;

				NetLimiter_UploadLimit = hexstr2int(strdup(limit_hex.c_str())) / 4;
				NetLimiter_UploadOn = u_int8_t(txt[16]);
				buf[255] = 0;

				if(NetLimiter_UploadOn == 1) {
					EnumWindows(GetWOkna,NULL);
					if(nlfound) {
						nlspeed = NetLimiter_UploadLimit;
					}
				}

				delete[] w1;
				break;
			}

			delete[] w1;

			if(len < BUF_SIZE)
				break;
		}
	
		f.close();
		delete[] w2;
	} catch(...) {
	}

	File::ensureDirectory(Util::getAppPath() + SETTINGS_DIR);
}

string Util::getConfigPath() {
#ifdef _WIN32
		return getAppPath();
#else
		char* home = getenv("HOME");
		if (home) {
#ifdef __APPLE__
			return string(home) + "/Library/Application Support/Mac StrongDC++/";
#else
			return string(home) + "/.strongdc++/";
#endif // __APPLE__
		}
		return emptyString;
#endif // _WIN32
}

string Util::validateMessage(string tmp, bool reverse, bool checkNewLines) {
	string::size_type i = 0;

	if(BOOLSETTING(CZCHARS_DISABLE))
		tmp = disableCzChars( tmp );

	if(reverse) {
		while( (i = tmp.find("&#36;", i)) != string::npos) {
			tmp.replace(i, 5, "$");
			i++;
		}
		i = 0;
		while( (i = tmp.find("&#124;", i)) != string::npos) {
			tmp.replace(i, 6, "|");
			i++;
		}
		if(checkNewLines) {
			// Check all '<' and '|' after newlines...
			i = 0;
			while( (i = tmp.find('\n', i)) != string::npos) {
				if(i + 1 < tmp.length()) {
					if(tmp[i+1] == '[' || tmp[i+1] == '<') {
						tmp.insert(i+1, "- ");
						i += 2;
					}
				}
				i++;
			}
		}
	} else {
		i = 0;
		while( (i = tmp.find('$', i)) != string::npos) {
			tmp.replace(i, 1, "&#36;");
			i += 4;
		}
		i = 0;
		while( (i = tmp.find('|', i)) != string::npos) {
			tmp.replace(i, 1, "&#124;");
			i += 5;
		}
	}
	return tmp;
}

string Util::validateChatMessage(string tmp) {
	string::size_type i = 0;

	if(BOOLSETTING(CZCHARS_DISABLE))
		tmp = disableCzChars( tmp );

	i = 0;
	while( (i = tmp.find('|', i)) != string::npos) {
		tmp.replace(i, 1, "&#124;");
		i += 5;
	}
	return tmp;
}

#ifdef _WIN32
static const char badChars[] = { 
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
		31, '<', '>', '/', '"', '|', '?', '*', 0
};
#else

static const char badChars[] = { 
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	31, '<', '>', '\\', '"', '|', '?', '*', 0
};
#endif

/**
 * Replaces all strange characters in a file with '_'
 * @todo Check for invalid names such as nul and aux...
 */
string Util::validateFileName(string tmp) {
	string::size_type i = 0;

	// First, eliminate forbidden chars
	while( (i = tmp.find_first_of(badChars, i)) != string::npos) {
		tmp[i] = '_';
		i++;
	}

	// Then, eliminate all ':' that are not the second letter ("c:\...")
	i = 0;
	while( (i = tmp.find(':', i)) != string::npos) {
		if(i == 1) {
			i++;
			continue;
		}
		tmp[i] = '_';	
		i++;
	}

	// Remove the .\ that doesn't serve any purpose
	i = 0;
	while( (i = tmp.find("\\.\\", i)) != string::npos) {
		tmp.erase(i+1, 2);
	}
	i = 0;
	while( (i = tmp.find("/./", i)) != string::npos) {
		tmp.erase(i+1, 2);
	}

	// Remove any double \\ that are not at the beginning of the path...
	i = 1;
	while( (i = tmp.find("\\\\", i)) != string::npos) {
		tmp.erase(i+1, 1);
	}
	i = 1;
	while( (i = tmp.find("//", i)) != string::npos) {
		tmp.erase(i+1, 1);
	}

	// And last, but not least, the infamous ..\! ...
	i = 0;
	while( ((i = tmp.find("\\..\\", i)) != string::npos) ) {
		tmp[i + 1] = '_';
		tmp[i + 2] = '_';
		tmp[i + 3] = '_';
		i += 2;
	}
	i = 0;
	while( ((i = tmp.find("/../", i)) != string::npos) ) {
		tmp[i + 1] = '_';
		tmp[i + 2] = '_';
		tmp[i + 3] = '_';
		i += 2;
	}

	return tmp;
}

string Util::getShortTimeString() {
	char buf[255];
	time_t _tt = time(NULL);
	tm* _tm = localtime(&_tt);
	if(_tm == NULL) {
		strcpy(buf, "xx:xx");
	} else {
		strftime(buf, 254, SETTING(TIME_STAMPS_FORMAT).c_str(), _tm);
	}
	return Text::acpToUtf8(buf);
}

/**
 * Decodes a URL the best it can...
 * Default ports:
 * http:// -> port 80
 * dchub:// -> port 411
 */
void Util::decodeUrl(const string& url, string& aServer, u_int16_t& aPort, string& aFile) {
	// First, check for a protocol: xxxx://
	string::size_type i = 0, j, k;
	
	aServer = emptyString;
	aFile = emptyString;

	if( (j=url.find("://", i)) != string::npos) {
		// Protocol found
		string protocol = url.substr(0, j);
		i = j + 3;

		if(protocol == "http") {
			aPort = 80;
		} else if(protocol == "dchub") {
			aPort = 411;
		}
	}

	if( (j=url.find('/', i)) != string::npos) {
		// We have a filename...
		aFile = url.substr(j);
	}

	if( (k=url.find(':', i)) != string::npos) {
		// Port
		if(j == string::npos) {
			aPort = (short)Util::toInt(url.substr(k+1));
		} else if(k < j) {
			aPort = (short)Util::toInt(url.substr(k+1, j-k-1));
		}
	} else {
		k = j;
	}

	if(k == string::npos)
		aServer = url;
	else
	aServer = url.substr(i, k-i);
}

void Util::setAway(bool aAway) {
	away = aAway;

	SettingsManager::getInstance()->set(SettingsManager::AWAY, aAway);

	if (away)
		awayTime = time(NULL);
}

string Util::getAwayMessage() { 
	return (formatTime(awayMsg.empty() ? SETTING(DEFAULT_AWAY_MESSAGE) : awayMsg, awayTime));
}

string Util::formatBytes(int64_t aBytes) {
	char buf[128];
	if(aBytes < 1024) {
		_snprintf(buf, 127, "%d %s", (int)(aBytes&0xffffffff), CSTRING(B));
	} else if(aBytes < 1024*1024) {
		_snprintf(buf, 127, "%.02f %s", (double)aBytes/(1024.0), CSTRING(KB));
	} else if(aBytes < 1024*1024*1024) {
		_snprintf(buf, 127, "%.02f %s", (double)aBytes/(1024.0*1024.0), CSTRING(MB));
	} else if(aBytes < (int64_t)1024*1024*1024*1024) {
		_snprintf(buf, 127, "%.02f %s", (double)aBytes/(1024.0*1024.0*1024.0), CSTRING(GB));
	} else if(aBytes < (int64_t)1024*1024*1024*1024*1024) {
		_snprintf(buf, 127, "%.02f %s", (double)aBytes/(1024.0*1024.0*1024.0*1024.0), CSTRING(TB));
	} else if(aBytes < (int64_t)1024*1024*1024*1024*1024*1024)  {
		_snprintf(buf, 127, "%.02f %s", (double)aBytes/(1024.0*1024.0*1024.0*1024.0*1024.0), CSTRING(PB));
	} else {
		_snprintf(buf, 127, "%.02f %s", (double)aBytes/(1024.0*1024.0*1024.0*1024.0*1024.0*1024.0), CSTRING(EB));
	}
	buf[127] = 0;
	return buf;
}

string Util::formatExactSize(int64_t aBytes) {
	char buf[64];
#ifdef _WIN32
		char number[64];
		NUMBERFMTA nf;
		_snprintf(number, 63, "%I64d", aBytes);
		buf[63] = 0;
		char Dummy[16];
    
		/*No need to read these values from the system because they are not
		used to format the exact size*/
		nf.NumDigits = 0;
		nf.LeadingZero = 0;
		nf.NegativeOrder = 0;
		nf.lpDecimalSep = ",";

		GetLocaleInfoA( LOCALE_SYSTEM_DEFAULT, LOCALE_SGROUPING, Dummy, 16 );
		nf.Grouping = atoi(Dummy);
		GetLocaleInfoA( LOCALE_SYSTEM_DEFAULT, LOCALE_STHOUSAND, Dummy, 16 );
		nf.lpThousandSep = " ";

		GetNumberFormatA(LOCALE_USER_DEFAULT, 0, number, &nf, buf, sizeof(buf)/sizeof(buf[0]));
#else
		_snprintf(buf, 63, "%'lld", aBytes);
		buf[63] = 0;		
#endif
		_snprintf(buf, 63, "%s %s", buf, CSTRING(B));
		buf[63] = 0;
		return buf;
}

string Util::getLocalIp() {
	string tmp;
	
	char buf[256];
	gethostname(buf, 255);
	hostent* he = gethostbyname(buf);
	if(he == NULL || he->h_addr_list[0] == 0)
		return Util::emptyString;
	sockaddr_in dest;
	int i = 0;
	
	// We take the first ip as default, but if we can find a better one, use it instead...
	memcpy(&(dest.sin_addr), he->h_addr_list[i++], he->h_length);
	tmp = inet_ntoa(dest.sin_addr);
	if(Util::isPrivateIp(tmp) || strncmp(tmp.c_str(), "169", 3) == 0) {
		while(he->h_addr_list[i]) {
			memcpy(&(dest.sin_addr), he->h_addr_list[i], he->h_length);
			string tmp2 = inet_ntoa(dest.sin_addr);
			if(!Util::isPrivateIp(tmp2) && strncmp(tmp2.c_str(), "169", 3) != 0) {
				tmp = tmp2;
			}
			i++;
		}
	}
	return tmp;
}

bool Util::isPrivateIp(string const& ip) {
	struct in_addr addr;

	addr.s_addr = inet_addr(ip.c_str());

	if (addr.s_addr  != INADDR_NONE) {
		unsigned long haddr = ntohl(addr.s_addr);
		return ((haddr & 0xff000000) == 0x0a000000 || // 10.0.0.0/8
				(haddr & 0xff000000) == 0x7f000000 || // 127.0.0.0/8
				(haddr & 0xfff00000) == 0xac100000 || // 172.16.0.0/12
				(haddr & 0xffff0000) == 0xc0a80000);  // 192.168.0.0/16
	}
	return false;
}

typedef const u_int8_t* ccp;
static wchar_t utf8ToLC(ccp& str) {
	wchar_t c = 0;
   if(str[0] & 0x80) { 
      if(str[0] & 0x40) { 
         if(str[0] & 0x20) { 
            if(str[1] == 0 || str[2] == 0 ||
				!((((unsigned char)str[1]) & ~0x3f) == 0x80) ||
					!((((unsigned char)str[2]) & ~0x3f) == 0x80))
				{
					str++;
					return 0;
				}
				c = ((wchar_t)(unsigned char)str[0] & 0xf) << 12 |
					((wchar_t)(unsigned char)str[1] & 0x3f) << 6 |
					((wchar_t)(unsigned char)str[2] & 0x3f);
				str += 3;
			} else {
				if(str[1] == 0 ||
					!((((unsigned char)str[1]) & ~0x3f) == 0x80)) 
				{
					str++;
					return 0;
				}
				c = ((wchar_t)(unsigned char)str[0] & 0x1f) << 6 |
					((wchar_t)(unsigned char)str[1] & 0x3f);
				str += 2;
			}
		} else {
			str++;
			return 0;
		}
	} else {
		wchar_t c = Text::asciiToLower((char)str[0]);
		str++;
		return c;
	}

	return Text::toLower(c);
}

string::size_type Util::findSubString(const string& aString, const string& aSubString, string::size_type start) throw() {
	if(aString.length() < start)
		return (string::size_type)string::npos;

	if(aString.length() - start < aSubString.length())
		return (string::size_type)string::npos;

	if(aSubString.empty())
		return 0;

	// Hm, should start measure in characters or in bytes? bytes for now...
	const u_int8_t* tx = (const u_int8_t*)aString.c_str() + start;
	const u_int8_t* px = (const u_int8_t*)aSubString.c_str();

	const u_int8_t* end = tx + aString.length() - aSubString.length() + 1;

	wchar_t wp = utf8ToLC(px);

	while(tx < end) {
		const u_int8_t* otx = tx;
		if(wp == utf8ToLC(tx)) {
			const u_int8_t* px2 = px;
			const u_int8_t* tx2 = tx;

			for(;;) {
				if(*px2 == 0)
					return otx - (u_int8_t*)aString.c_str();

				if(utf8ToLC(px2) != utf8ToLC(tx2))
					break;
			}
		}
	}
	return (string::size_type)string::npos;
}

int Util::stricmp(const char* a, const char* b) {
	while(*a) {
			wchar_t ca = 0, cb = 0;
			int na = Text::utf8ToWc(a, ca);
			int nb = Text::utf8ToWc(b, cb);
			ca = Text::toLower(ca);
			cb = Text::toLower(cb);
			if(ca != cb) {
			return (int)ca - (int)cb;
			}
			a+= na < 0 ? 1 : na;
			b+= nb < 0 ? 1 : nb;
	}
	wchar_t ca = 0, cb = 0;
	Text::utf8ToWc(a, ca);
	Text::utf8ToWc(b, cb);

	return (int)Text::toLower(ca) - (int)Text::toLower(cb);
}

int Util::strnicmp(const char* a, const char* b, size_t n) {
	const char* end = a + n;
	while(*a && a < end) {
		wchar_t ca = 0, cb = 0;
		int na = Text::utf8ToWc(a, ca);
		int nb = Text::utf8ToWc(b, cb);
		ca = Text::toLower(ca);
		cb = Text::toLower(cb);
		if(ca != cb) {
			return (int)ca - (int)cb;
		}
		a+= na < 0 ? 1 : na;
		b+= nb < 0 ? 1 : nb;
	}
	wchar_t ca = 0, cb = 0;
	Text::utf8ToWc(a, ca);
	Text::utf8ToWc(b, cb);
	return (a >= end) ? 0 : ((int)Text::toLower(ca) - (int)Text::toLower(cb));
}

string Util::encodeURI(const string& aString, bool reverse) {
	// reference: rfc2396
	string tmp = aString;
	if(reverse) {
		string::size_type idx;
		for(idx = 0; idx < tmp.length(); ++idx) {
			if(tmp.length() > idx + 2 && tmp[idx] == '%' && isxdigit(tmp[idx+1]) && isxdigit(tmp[idx+2])) {
				tmp[idx] = fromHexEscape(tmp.substr(idx+1,2));
				tmp.erase(idx+1, 2);
			} else { // reference: rfc1630, magnet-uri draft
				if(tmp[idx] == '+')
					tmp[idx] = ' ';
			}
		}
	} else {
		const string disallowed = ";/?:@&=+$," // reserved
			                      "<>#%\" "    // delimiters
		                          "{}|\\^[]`"; // unwise
		string::size_type idx, loc;
		for(idx = 0; idx < tmp.length(); ++idx) {
			if(tmp[idx] == ' ') {
				tmp[idx] = '+';
			} else {
				if(tmp[idx] <= 0x1F || tmp[idx] >= 0x7f || (loc = disallowed.find_first_of(tmp[idx])) != string::npos) {
					tmp.replace(idx, 1, toHexEscape(tmp[idx]));
					idx+=2;
				}
			}
		}
	}
	return tmp;
}

/**
 * This function takes a string and a set of parameters and transforms them according to
 * a simple formatting rule, similar to strftime. In the message, every parameter should be
 * represented by %[name]. It will then be replaced by the corresponding item in 
 * the params stringmap. After that, the string is passed through strftime with the current
 * date/time and then finally written to the log file. If the parameter is not present at all,
 * it is removed from the string completely...
 */
string Util::formatParams(const string& msg, StringMap& params) {
	string result = msg;

	string::size_type i, j, k;
	i = 0;
	while (( j = result.find("%[", i)) != string::npos) {
		if( (result.size() < j + 2) || ((k = result.find(']', j + 2)) == string::npos) ) {
			break;
		}
		string name = result.substr(j + 2, k - j - 2);
		StringMapIter smi = params.find(name);
		if(smi == params.end()) {
			result.erase(j, k-j + 1);
			i = j;
		} else {
			if(smi->second.find('%') != string::npos) {
				string tmp = smi->second;	// replace all % in params with %% for strftime
				string::size_type m = 0;
				while(( m = tmp.find('%', m)) != string::npos) {
					tmp.replace(m, 1, "%%");
					m+=2;
				}
				result.replace(j, k-j + 1, tmp);
				i = j + tmp.size();
			} else {
				result.replace(j, k-j + 1, smi->second);
				i = j + smi->second.size();
			}
		}
	}

	result = formatTime(result, time(NULL));
	
	return result;
}

/** Fix for wide formatting bug in wcsftime in the ms c lib for multibyte encodings of unicode in singlebyte locales */
string fixedftime(const string& format, struct tm* t) {
	string ret = format;
	const char codes[] = "aAbBcdHIjmMpSUwWxXyYzZ%";

	char tmp[4];
	tmp[0] = '%';
	tmp[1] = tmp[2] = tmp[3] = 0;

	StringMap sm;
	AutoArray<char> buf(1024);
	for(size_t i = 0; i < sizeof(codes); ++i) {
		tmp[1] = codes[i];
		tmp[2] = 0;
		strftime(buf, 1024-1, tmp, t);
		sm[tmp] = buf; 

		tmp[1] = '#';
		tmp[2] = codes[i];
		strftime(buf, 1024-1, tmp, t);
		sm[tmp] = buf; 		
	}

	for(StringMapIter i = sm.begin(); i != sm.end(); ++i) {
		for(string::size_type j = ret.find(i->first); j != string::npos; j = ret.find(i->first, j)) {
			ret.replace(j, i->first.length(), i->second);
			j += i->second.length() - i->first.length();
		}
	}

	return ret;
}

string Util::formatRegExp(const string& msg, StringMap& params) {
	string result = msg;
	string::size_type i, j, k;
	i = 0;
	while (( j = result.find("%[", i)) != string::npos) {
		if( (result.size() < j + 2) || ((k = result.find(']', j + 2)) == string::npos) ) {
			break;
		}
		string name = result.substr(j + 2, k - j - 2);
		StringMapIter smi = params.find(name);
		if(smi != params.end()) {
			result.replace(j, k-j + 1, smi->second);
			i = j + smi->second.size();
		} else {
			i = k + 1;
		}
	}
	return result;
}

u_int64_t Util::getDirSize(const string &sFullPath) {
	u_int64_t total = 0;

	WIN32_FIND_DATA fData;
	HANDLE hFind;
	
	hFind = FindFirstFile(Text::toT(sFullPath + "\\*").c_str(), &fData);

	if(hFind != INVALID_HANDLE_VALUE) {
		do {
			string name = Text::fromT(fData.cFileName);
			if(name == "." || name == "..")
				continue;
			if(name.find('$') != string::npos)
				continue;
			if(!BOOLSETTING(SHARE_HIDDEN) && (fData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
				continue;
			if(fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				string newName = sFullPath + PATH_SEPARATOR + name;
				if(Util::stricmp(newName + PATH_SEPARATOR, SETTING(TEMP_DOWNLOAD_DIRECTORY)) != 0) {
					total += getDirSize(newName);
				}
			} else {
				// Not a directory, assume it's a file...make sure we're not sharing the settings file...
				if( (Util::stricmp(name.c_str(), "DCPlusPlus.xml") != 0) && 
					(Util::stricmp(name.c_str(), "Favorites.xml") != 0)) {

					total+=(u_int64_t)fData.nFileSizeLow | ((u_int64_t)fData.nFileSizeHigh)<<32;
				}
			}
		} while(FindNextFile(hFind, &fData));
	}
	
	FindClose(hFind);

	return total;
}

bool Util::validatePath(const string &sPath) {
	if(sPath.empty())
		return false;

	if((sPath.substr(1, 2) == ":\\") || (sPath.substr(0, 2) == "\\\\")) {
		if(GetFileAttributes(Text::toT(sPath).c_str()) & FILE_ATTRIBUTE_DIRECTORY)
			return true;
	}

	return false;
}

bool Util::fileExists(const string &aFile) {
	DWORD attr = GetFileAttributes(Text::toT(aFile).c_str());
	return (attr != 0xFFFFFFFF);
}

string Util::formatTime(const string &msg, const time_t t) {
	if (!msg.empty()) {
		size_t bufsize = msg.size() + 256;
		struct tm* loc = localtime(&t);

		if(!loc) {
			return Util::emptyString;
		}
#if _WIN32
		AutoArray<TCHAR> buf(bufsize);

		if(!_tcsftime(buf, bufsize-1, Text::toT(msg).c_str(), loc)) {
			return fixedftime(msg, loc);
		}

		return Text::fromT(tstring(buf));
#else
		// will this give wide representations for %a and %A?
		// surely win32 can't have a leg up on linux/unixen in this area. - Todd
		AutoArray<char> buf(bufsize);

		while(!strftime(buf, bufsize-1, msg.c_str(), loc)) {
			bufsize+=64;
			buf = new char[bufsize];
		}

		return string(buf);
#endif
	}
	return Util::emptyString;
}

/* Below is a high-speed random number generator with much
   better granularity than the CRT one in msvc...(no, I didn't
   write it...see copyright) */ 
/* Copyright (C) 1997 Makoto Matsumoto and Takuji Nishimura.
   Any feedback is very welcome. For any question, comments,       
   see http://www.math.keio.ac.jp/matumoto/emt.html or email       
   matumoto@math.keio.ac.jp */       
/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */

/* Tempering parameters */   
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)

static unsigned long mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */

/* initializing the array with a NONZERO seed */
static void sgenrand(unsigned long seed) {
	/* setting initial seeds to mt[N] using         */
	/* the generator Line 25 of Table 1 in          */
	/* [KNUTH 1981, The Art of Computer Programming */
	/*    Vol. 2 (2nd Ed.), pp102]                  */
	mt[0]= seed & 0xffffffff;
	for (mti=1; mti<N; mti++)
		mt[mti] = (69069 * mt[mti-1]) & 0xffffffff;
}

u_int32_t Util::rand() {
	unsigned long y;
	static unsigned long mag01[2]={0x0, MATRIX_A};
	/* mag01[x] = x * MATRIX_A  for x=0,1 */

	if (mti >= N) { /* generate N words at one time */
		int kk;

		if (mti == N+1)   /* if sgenrand() has not been called, */
			sgenrand(4357); /* a default initial seed is used   */

		for (kk=0;kk<N-M;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
		}
		for (;kk<N-1;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
		}
		y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
		mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];

		mti = 0;
	}

	y = mt[mti++];
	y ^= TEMPERING_SHIFT_U(y);
	y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
	y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
	y ^= TEMPERING_SHIFT_L(y);

	return y; 
}

string Util::getOsVersion() {
#ifdef _WIN32
	string os;

	OSVERSIONINFOEX ver;
	memset(&ver, 0, sizeof(OSVERSIONINFOEX));
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if(!GetVersionEx((OSVERSIONINFO*)&ver)) {
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if(!GetVersionEx((OSVERSIONINFO*)&ver)) {
			os = "Windows (version unknown)";
		}
	}

	if(os.empty()) {
		if(ver.dwPlatformId != VER_PLATFORM_WIN32_NT) {
			os = "Win9x/ME/Junk";
		} else if(ver.dwMajorVersion == 4) {
			os = "WinNT4";
		} else if(ver.dwMajorVersion == 5) {
			if(ver.dwMinorVersion == 0) {
				os = "Win2000";
			} else if(ver.dwMinorVersion == 1) {
				os = "WinXP";
			} else if(ver.dwMinorVersion == 2) {
				os = "Win2003";
			} else {
				os = "Unknown WinNT5";
			}

			if(ver.wProductType & VER_NT_WORKSTATION)
				os += " Pro";
			else if(ver.wProductType & VER_NT_SERVER)
				os += " Server";
			else if(ver.wProductType & VER_NT_DOMAIN_CONTROLLER)
				os += " DC";
		}

		if(ver.wServicePackMajor != 0) {
			os += "SP";
			os += Util::toString(ver.wServicePackMajor);
			if(ver.wServicePackMinor != 0) {
				os += '.';
				os += Util::toString(ver.wServicePackMinor);
			}
		}
	}

	return os;

#else // _WIN32
	utsname n;

	if(uname(&n) != 0) {
		return "unix (unknown version)";
	}

	return string(n.sysname) + " " + string(n.release) + " (" + string(n.machine) + ")";

#endif // _WIN32
}

/*	getIpCountry
	This function returns the country(Abbreviation) of an ip
	for exemple: it returns "PT", whitch standards for "Portugal"
	more info: http://www.maxmind.com/app/csv
*/
string Util::getIpCountry (string IP) {
	if (BOOLSETTING(GET_USER_COUNTRY)) {
		dcassert(count(IP.begin(), IP.end(), '.') == 3);

		//e.g IP 23.24.25.26 : w=23, x=24, y=25, z=26
		string::size_type a = IP.find('.');
		string::size_type b = IP.find('.', a+1);
		string::size_type c = IP.find('.', b+2);

		u_int32_t ipnum = (Util::toUInt32(IP.c_str()) << 24) | 
			(Util::toUInt32(IP.c_str() + a + 1) << 16) | 
			(Util::toUInt32(IP.c_str() + b + 1) << 8) | 
			(Util::toUInt32(IP.c_str() + c + 1) );

		CountryIter i = countries.lower_bound(ipnum);

		if(i != countries.end()) {
			return string((char*)&(i->second), 2);
		}
	}

	return Util::emptyString; //if doesn't returned anything already, something is wrong...
}

string Util::toDOS(const string& tmp) {
	if(tmp.empty())
		return Util::emptyString;

	string tmp2(tmp);

	if(tmp2[0] == '\r' && (tmp2.size() == 1 || tmp2[1] != '\n')) {
		tmp2.insert(1, "\n");
	}
	for(string::size_type i = 1; i < tmp2.size() - 1; ++i) {
		if(tmp2[i] == '\r' && tmp2[i+1] != '\n') {
			// Mac ending
			tmp2.insert(i+1, "\n");
			i++;
		} else if(tmp2[i] == '\n' && tmp2[i-1] != '\r') {
			// Unix encoding
			tmp2.insert(i, "\r");
			i++;
		}
	}
	return tmp2;
}

TCHAR* Util::strstr(const TCHAR *str1, const TCHAR *str2, int *pnIdxFound) {
	TCHAR *s1, *s2;
	TCHAR *cp = (TCHAR *)str1;
	if (!*str2)
		return (TCHAR *)str1;
	int nIdx = 0;
	while (*cp) {
		s1 = cp;
		s2 = (TCHAR *) str2;
                while(*s1 && *s2 && !(*s1-*s2))
                        s1++, s2++;
		if (!*s2) {
			if (pnIdxFound != NULL)
				*pnIdxFound = nIdx;
			return cp;
		}
		cp++;
		nIdx++;
	}
	if (pnIdxFound != NULL)
		*pnIdxFound = -1;
	return NULL;
}

/**
 * @file
 * $Id$
 */