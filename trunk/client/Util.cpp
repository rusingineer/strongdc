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

#include "Util.h"
#include "File.h"

#include "SettingsManager.h"
#include "ResourceManager.h"
#include "StringTokenizer.h"

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

FastCriticalSection FastAllocBase::cs;

int64_t Util::mUptimeSeconds = 0;
string Util::emptyString;

bool Util::away = false;
string Util::awayMsg;
time_t Util::awayTime;
char Util::upper[256];
char Util::lower[256];
int8_t Util::cmp[256][256];
int8_t Util::cmpi[256][256];
Util::CountryList Util::countries;

static void sgenrand(unsigned long seed);

char Win1250toAscii[128] = {
// 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
	'*', '*', '*', '*', '*', '*', '*', '*', '*', '*', 'S', '*', 'S', 'T', 'Z', 'Z',   // 0x80
	'*', '*', '*', '*', '*', '*', '*', '*', '*', '*', 's', '*', 's', 't', 'z', 'z',   // 0x90
	'*', '*', '*', 'L', '*', 'A', '*', '*', '*', '*', 'S', '*', '*', '*', '*', 'Z',   // 0xA0
	'*', '*', '*', 'l', '*', '*', '*', '*', '*', 'a', 's', '*', 'L', '*', 'l', 'z',   // 0xB0
	'R', 'A', 'A', 'A', 'A', 'L', 'C', 'C', 'C', 'E', 'E', 'E', 'E', 'I', 'I', 'D',   // 0xC0
	'D', 'N', 'N', 'O', 'O', 'O', 'O', '*', 'R', 'U', 'U', 'U', 'U', 'Y', 'T', '*',   // 0xD0
	'r', 'a', 'a', 'a', 'a', 'l', 'c', 'c', 'c', 'e', 'e', 'e', 'e', 'i', 'i', 'd',   // 0xE0
	'd', 'n', 'n', 'o', 'o', 'o', 'o', '*', 'r', 'u', 'u', 'u', 'u', 'y', 't', '*',   // 0xF0
};

// Converts string from charset Windows1250 to plain (7-bit) Ascii
void ConvertStringToAscii(string *s) {
	char c;

	for(int i = 0; i < s->length(); i++) {
		c = (*s)[i];
		if ( c < 0 )
			(*s)[i] = Win1250toAscii[c + 128];
	}
}

void Util::initialize() {
	int i;
	for(i = 0; i < 256; ++i) {
#ifdef _WIN32
		upper[i] = (char)CharUpper((LPSTR)i);
		lower[i] = (char)CharLower((LPSTR)i);
#else
		upper[i] = (char)toupper(i);
		lower[i] = (char)tolower(i);
#endif
	}

	setlocale(LC_ALL, "");

	// Now initialize the compare table to the current locale (hm...hopefully we
	// won't have strange problems because of this (users from different locales for instance)
	for(i = 0; i < 256; ++i) {
		for(int j = 0; j < 256; ++j) {
			cmp[i][j] = (int8_t)::strncmp((char*)&i, (char*)&j, 1);
			cmpi[i][j] = (int8_t)::strncmp((char*)&lower[i], (char*)&lower[j], 1);
		}
	}

	sgenrand(time(NULL));

	try {
		string file = Util::getAppPath() + "GeoIpCountryWhois.csv";

		StringTokenizer st(File(file , File::READ, File::OPEN).read());
		CountryIter last = countries.end();
		for(StringIter i = st.getTokens().begin(); i != st.getTokens().end(); ++i) {
			string::size_type j = i->find(',');
			if(j != string::npos && j < i->length() - 2) {
				u_int16_t* country = (u_int16_t*)(i->c_str() + j + 1);
				last = countries.insert(last, make_pair(Util::toUInt32(i->c_str()), *country));
			}
		}
	} catch(const FileException&) {
	}
}

string Util::validateMessage(string tmp, bool reverse, bool checkNewLines) {
	string::size_type i = 0;

	if(reverse) {
		while( (i = safestring::SafeFind(tmp, "&#36;", i)) != string::npos) {
			tmp.replace(i, 5, "$");
			i++;
		}
		i = 0;
		while( (i = safestring::SafeFind(tmp, "&#124;", i)) != string::npos) {
			tmp.replace(i, 6, "|");
			i++;
		}
		if(checkNewLines) {
			// Check all '<' and '|' after newlines...
			i = 0;
			while( (i = safestring::SafeFind(tmp, '\n', i)) != string::npos) {
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
		while( (i = safestring::SafeFind(tmp, '$', i)) != string::npos) {
			tmp.replace(i, 1, "&#36;");
			i += 4;
		}
		i = 0;
		while( (i = safestring::SafeFind(tmp, '|', i)) != string::npos) {
			tmp.replace(i, 1, "&#124;");
			i += 5;
		}
	}
	return tmp;
}

string Util::validateChatMessage(string tmp) {
	string::size_type i = 0;

	if(BOOLSETTING(CZCHARS_DISABLE))
		ConvertStringToAscii( &tmp );

	i = 0;
	while( (i = safestring::SafeFind(tmp, '|', i)) != string::npos) {
		tmp.replace(i, 1, "&#124;");
		i += 5;
	}
	return tmp;
}

static const char badChars[] = { 
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
		31, '<', '>', '/', '"', '|', '?', '*', 0
};

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
	while( (i = safestring::SafeFind(tmp, ':', i)) != string::npos) {
		if(i == 1) {
			i++;
			continue;
		}
		tmp[i] = '_';	
		i++;
	}

	// Remove the .\ that doesn't serve any purpose
	i = 0;
	while( (i = safestring::SafeFind(tmp, "\\.\\", i)) != string::npos) {
		tmp.erase(i+1, 2);
	}

	// Remove any double \\ that are not at the beginning of the path...
	i = 1;
	while( (i = safestring::SafeFind(tmp, "\\\\", i)) != string::npos) {
		tmp.erase(i+1, 1);
	}

	// And last, but not least, the infamous ..\! ...
	i = 0;
	while( ((i = safestring::SafeFind(tmp,"\\..\\", i)) != string::npos) ) {
		tmp[i + 1] = '_';
		tmp[i + 2] = '_';
		tmp[i + 3] = '_';
		i += 2;
	}
	return tmp;
}

/**
 * Decodes a URL the best it can...
 * Default ports:
 * http:// -> port 80
 * dchub:// -> port 411
 */
void Util::decodeUrl(const string& url, string& aServer, short& aPort, string& aFile) {
	// First, check for a protocol: xxxx://
	string::size_type i = 0, j, k;
	
	aServer = emptyString;
	aFile = emptyString;

	if( (j=safestring::SafeFind(url,"://", i)) != string::npos) {
		// Protocol found
		string protocol = url.substr(0, j);
		i = j + 3;

		if(protocol == "http") {
			aPort = 80;
		} else if(protocol == "dchub") {
			aPort = 411;
		}
	}

	if( (j=safestring::SafeFind(url,'/', i)) != string::npos) {
		// We have a filename...
		aFile = url.substr(j);
	}

	if( (k=safestring::SafeFind(url,':', i)) != string::npos) {
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

string Util::getAwayMessage() { 
	return (formatTime(awayMsg.empty() ? SETTING(DEFAULT_AWAY_MESSAGE) : awayMsg, awayTime));
}

string Util::formatBytes(int64_t aBytes) {
	char buf[128];
	if(aBytes < 1024) {
		sprintf(buf, "%d %s", (int)(aBytes&0xffffffff), CSTRING(B));
	} else if(aBytes < 1024*1024) {
		sprintf(buf, "%.02f %s", (double)aBytes/(1024.0), CSTRING(KB));
	} else if(aBytes < 1024*1024*1024) {
		sprintf(buf, "%.02f %s", (double)aBytes/(1024.0*1024.0), CSTRING(MB));
	} else if(aBytes < (int64_t)1024*1024*1024*1024) {
		sprintf(buf, "%.02f %s", (double)aBytes/(1024.0*1024.0*1024.0), CSTRING(GB));
	} else if(aBytes < (int64_t)1024*1024*1024*1024*1024) {
		sprintf(buf, "%.02f %s", (double)aBytes/(1024.0*1024.0*1024.0*1024.0), CSTRING(TB));
	} else if(aBytes < (int64_t)1024*1024*1024*1024*1024*1024)  {
		sprintf(buf, "%.02f %s", (double)aBytes/(1024.0*1024.0*1024.0*1024.0*1024.0), CSTRING(PB));
	} else {
		sprintf(buf, "%.02f %s", (double)aBytes/(1024.0*1024.0*1024.0*1024.0*1024.0*1024.0), CSTRING(EB));
	}
	return buf;
}

string Util::formatExactSize(int64_t aBytes) {
	char buf[64];
#ifdef _WIN32
		char number[64];
		NUMBERFMT nf;
		sprintf(number, "%I64d", aBytes);
		char Dummy[16];
    
		/*No need to read these values from the system because they are not
		used to format the exact size*/
		nf.NumDigits = 0;
		nf.LeadingZero = 0;
		nf.NegativeOrder = 0;
		nf.lpDecimalSep = ",";

		GetLocaleInfo( LOCALE_SYSTEM_DEFAULT, LOCALE_SGROUPING, Dummy, 16 );
		nf.Grouping = atoi(Dummy);
		GetLocaleInfo( LOCALE_SYSTEM_DEFAULT, LOCALE_STHOUSAND, Dummy, 16 );
		nf.lpThousandSep = Dummy;

		GetNumberFormatA(LOCALE_USER_DEFAULT, 0, number, &nf, buf, sizeof(buf)/sizeof(buf[0]));
#else
		sprintf(buf, "%lld", aNumber);
#endif
		sprintf(buf, "%s %s", buf, CSTRING(B));
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

static void cToUtf8(wchar_t c, string& str) {
	if(c >= 0x0800) {
		str += (char)(0x80 | 0x40 | 0x20 & (c >> 12));
		str += (char)(0x80 & ((c >> 6) & 0x3f));
		str += (char)(0x80 & (c & 0x3f));
	} else if(c >= 0x0080) {
		str += (char)(0x80 | 0x40 | (c >> 6));
		str += (char)(0x80 | (c & 0x3f)); 
	} else {
		str += (char)c;
	}
}

static int utf8ToC(const char* str, wchar_t& c) {
	int l = 0;
	if(str[0] & 0x80) {
		if(str[0] & 0x40) {
			if(str[0] & 0x20) {
				if(str[1] == 0 || str[2] ||
					!((((unsigned char)str[1]) & ~0x3f) == 0x80) ||
					!((((unsigned char)str[2]) & ~0x3f) == 0x80))
				{
					return -1;
				}
				c = ((wchar_t)(unsigned char)str[0] & 0xf) << 12 |
					((wchar_t)(unsigned char)str[1] & 0x3f) << 6 |
					((wchar_t)(unsigned char)str[2] & 0x3f);
				l = 3;
			} else {
				if(str[1] == 0 ||
					!((((unsigned char)str[1]) & ~0x3f) == 0x80)) 
				{
					return -1;
				}
				c = ((wchar_t)(unsigned char)str[0] & 0x1f) << 6 |
					((wchar_t)(unsigned char)str[1] & 0x3f);
				l = 2;
			}
		} else {
			return -1;
		}
	} else {
		c = (unsigned char)str[0];
		l = 1;
	}

	return l;
}

/**
 * Convert a string in the current locale (whatever that happens to be) to UTF-8.
 */
string& Util::toUtf8(string& str) {
	if(str.empty())
		return str;
	wstring wtmp(str.length(), 0);
#ifdef _WIN32
	int sz = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str.c_str(), str.length(),
		&wtmp[0], str.length());
	if(sz <= 0) {
		str.clear();
		return str;
	}
#else
	int sz = mbstowcs(&wtmp[0], str.c_str(), wtmp.length());
	if(sz <= 0) {
		str.clear();
		return str;
	}
	if(sz < (int)wtmp.length())
		sz--;
#endif

	wtmp.resize(sz);
	str.clear();
    for(string::size_type i = 0; i < wtmp.length(); ++i) {
		cToUtf8(wtmp[i], str);
	}
	return str;
}

string& Util::toAcp(string& str) {
	if(str.empty())
		return str;

	wstring wtmp;
	wtmp.reserve(str.length());
	for(string::size_type i = 0; i < str.length(); ) {
		wchar_t c = 0;
		int x = utf8ToC(str.c_str() + i, c);
		if(x == -1) {
			i++;
		} else {
			i+=x;
			wtmp += c;
		}
	}
#ifdef _WIN32
	int x = WideCharToMultiByte(CP_ACP, 0, wtmp.c_str(), wtmp.length(), NULL, 0, NULL, NULL);
	if(x == 0) {
		str.clear();
		return str;
	}
	str.resize(x);
	WideCharToMultiByte(CP_ACP, 0, wtmp.c_str(), wtmp.length(), &str[0], str.size(), NULL, NULL);
#else
	size_t x = wcstombs(NULL, wtmp.c_str(), 0);
	if(x == (size_t)-1) {
		str.clear();
		return str;
	}
	str.resize(x);
	wcstombs(&str[0], wtmp.c_str(), str.size());
#endif
	return str;
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
	while (( j = safestring::SafeFind(result,"%[", i)) != string::npos) {
		if( (result.size() < j + 2) || ((k = safestring::SafeFind(result,']', j + 2)) == string::npos) ) {
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
				while(( m = safestring::SafeFind(tmp,'%', m)) != string::npos) {
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

u_int64_t Util::getDirSize(const string &sFullPath) {
	u_int64_t total = 0;

	WIN32_FIND_DATA fData;
	HANDLE hFind;
	
	hFind = FindFirstFile((sFullPath + "\\*").c_str(), &fData);

	if(hFind != INVALID_HANDLE_VALUE) {
		do {
			string name = fData.cFileName;
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
		if(GetFileAttributes(sPath.c_str()) & FILE_ATTRIBUTE_DIRECTORY)
			return true;
	}

	return false;
}

bool Util::fileExists(const string &aFile) {
	DWORD attr = GetFileAttributes(aFile.c_str());
	return (attr != 0xFFFFFFFF);
}

string Util::formatTime(const string &msg, const time_t t) {
	if (!msg.empty()) {
		size_t bufsize = msg.size() + 64;
		struct tm* loc = localtime(&t);

		if(!loc) {
			return Util::emptyString;
		}

		AutoArray<char> buf(new char[bufsize]);

		while(!strftime(buf, bufsize-1, msg.c_str(), loc)) {
			bufsize+=64;
			buf = new char[bufsize];
		}

		return string(buf);
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

bool safestring::_CorrectFindPos(const string &InStr, string::size_type &pos)
{
	if (pos < 0) {
		pos = 0;
		return true;
	}
	string::size_type length = InStr.length();
	if (length == 0){
		return false;
	}
	if (pos >= length) {
		return false;
	}
	return true;
}

string::size_type safestring::SafeFind(const string &InStr, char c, string::size_type pos)
{
	if (!_CorrectFindPos(InStr, pos))
		return string::npos;
	return InStr.find(c, pos);
}
string::size_type safestring::SafeFind(const string &InStr, const char *s, string::size_type pos)
{
	if (!_CorrectFindPos(InStr, pos))
		return string::npos;
	return InStr.find(s, pos);
}
string::size_type safestring::SafeFind(const string &InStr, const char *s, string::size_type pos, string::size_type n)
{
	if (!_CorrectFindPos(InStr, pos))
		return string::npos;
	return InStr.find(s, pos, n);
}
string::size_type safestring::SafeFind(const string &InStr, const string& str, string::size_type pos)
{
	if (!_CorrectFindPos(InStr, pos))
		return string::npos;
	return InStr.find(str, pos);
}

#define TOBIN(x) (((x) >= '0' && (x) <= '9') ? ((x) - '0') : ((x) - 'A' + 0xA))
#define ISRGBBIT(x) (((x) >= '0' && (x) <= '9') || ((x) >= 'A' && (x) <= 'F'))
bool Util::RGB2Binary(string sRGB, BYTE* pbData)
{
    int i;
    const char* p = sRGB.c_str();

	for(i = 0; i < sRGB.size(); i ++)
		if(!ISRGBBIT(*(p+i))) return false;

	for(i = 0; i < sRGB.size(); i += 2)
        pbData[i/2] = (TOBIN(*(p + i)) << 4) | TOBIN(*(p + i + 1));

    return true;
}

string Util::Binary2RGB(BYTE* pbBuf, DWORD dwSize)
{
	const static char rgb[] = "0123456789ABCDEF";
	char buf[128];

	dcassert(dwSize < sizeof(buf));
	for(int i = 0; i < dwSize ; i++){
		buf[i*2]   = rgb[pbBuf[i] >> 4];
		buf[i*2+1] = rgb[pbBuf[i] & 0xf];
	}
	buf[dwSize * 2] = 0;

	return string(buf);
}

/**
 * @file
 * $Id$
 */