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

#if !defined(AFX_UTIL_H__1758F242_8D16_4C50_B40D_E59B3DD63913__INCLUDED_)
#define AFX_UTIL_H__1758F242_8D16_4C50_B40D_E59B3DD63913__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#endif

/** Evaluates op(pair<T1, T2>.first, compareTo) */
template<class T1, class T2, class op = equal_to<T1> >
class CompareFirst {
public:
	CompareFirst(const T1& compareTo) : a(compareTo) { };
	bool operator()(const pair<T1, T2>& p) { return op()(p.first, a); };
private:
	const T1& a;
};

/** Evaluates op(pair<T1, T2>.second, compareTo) */
template<class T1, class T2, class op = equal_to<T2> >
class CompareSecond {
public:
	CompareSecond(const T2& compareTo) : a(compareTo) { };
	bool operator()(const pair<T1, T2>& p) { return op()(p.second, a); };
private:
	const T2& a;
};

template<class T>
struct PointerHash {
#if _MSC_VER < 1300 
	enum {bucket_size = 4}; 
	enum {min_buckets = 8}; 
#else 
	static const size_t bucket_size = 4;
	static const size_t min_buckets = 8;
#endif // _MSC_VER < 1300
	size_t operator()(const T* a) const { return ((size_t)a)/sizeof(T); };
	bool operator()(const T* a, const T* b) { return a < b; };
};
template<>
struct PointerHash<void> {
	size_t operator()(const void* a) const { return ((size_t)a)>>2; };
};

/** 
 * Compares two values
 * @return -1 if v1 < v2, 0 if v1 == v2 and 1 if v1 > v2
 */
template<typename T1>
inline int compare(const T1& v1, const T1& v2) { return (v1 < v2) ? -1 : ((v1 == v2) ? 0 : 1); }

class Flags {
	public:
		typedef int MaskType;

		Flags() : flags(0) { };
		Flags(const Flags& rhs) : flags(rhs.flags) { };
		Flags(MaskType f) : flags(f) { };
		bool isSet(MaskType aFlag) const { return (flags & aFlag) == aFlag; };
		bool isAnySet(MaskType aFlag) const { return (flags & aFlag) != 0; };
 		void setFlag(MaskType aFlag) { flags |= aFlag; };
		void unsetFlag(MaskType aFlag) { flags &= ~aFlag; };
		Flags& operator=(const Flags& rhs) { flags = rhs.flags; return *this; };
	private:
		MaskType flags;
};

template<typename T>
class AutoArray {
	typedef T* TPtr;
	typedef T& TRef;
public:
	explicit AutoArray(TPtr t) : p(t) { };
	explicit AutoArray(size_t size) : p(new T[size]) { };
	~AutoArray() { delete[] p; };
	operator TPtr() { return p; };
	AutoArray& operator=(TPtr t) { delete[] p; p = t; return *this; };
private:
	AutoArray(const AutoArray&);
	AutoArray& operator=(const AutoArray&);

	TPtr p;
};

class Util  
{
public:
	static string emptyString;

	static void initialize();

	static void ensureDirectory(const string& aFile)
	{
		string::size_type start = 0;
		
#ifdef _WIN32
		while( (start = aFile.find_first_of("\\/", start)) != string::npos) {
			CreateDirectory(aFile.substr(0, start+1).c_str(), NULL);
			start++;
		}
#else
		while( (start = aFile.find_first_of("/", start)) != string::npos) {
			mkdir(aFile.substr(0, start+1).c_str(), 0755);
			start++;
		}
#endif
	}
	
	static string getAppPath() {
#ifdef _WIN32
		TCHAR buf[MAX_PATH+1];
		GetModuleFileName(NULL, buf, MAX_PATH);
		int i = (strrchr(buf, '\\') - buf);
		return string(buf, i + 1);
#else // _WIN32
		char* home = getenv("HOME");
		if (home) {
			return string(home) + "/.dc++/";
		}
		return emptyString;
#endif // _WIN32
	}	

	static string getAppName() {
#ifdef _WIN32
		TCHAR buf[MAX_PATH+1];
		DWORD x = GetModuleFileName(NULL, buf, MAX_PATH);
		return string(buf, x);
#else // _WIN32
		char buf[PATH_MAX + 1];
		char* path = getenv("_");
		if (!path) {
			if (readlink("/proc/self/exe", buf, sizeof (buf)) == -1) {
		return emptyString;
			}
			path = buf;
		}
		return string(path);
#endif // _WIN32
	}	

	static string getTempPath() {
#ifdef _WIN32
		TCHAR buf[MAX_PATH + 1];
		DWORD x = GetTempPath(MAX_PATH, buf);
		return string(buf, x);
#else
		return "/tmp/";
#endif
	}

	static string getDataPath() {
#ifdef _WIN32
		return getAppPath();
#else
		char* home = getenv("HOME");
		if (home) {
			return string(home) + "/dc++/";
		}
		return emptyString;
#endif
	}

	static string translateError(int aError) {
#ifdef _WIN32
		LPVOID lpMsgBuf;
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			aError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
			);
		string tmp = (LPCTSTR)lpMsgBuf;
		// Free the buffer.
		LocalFree( lpMsgBuf );
		string::size_type i;

		while( (i = tmp.find_last_of("\r\n")) != string::npos) {
			tmp.erase(i, 1);
		}
		return tmp;
#else // _WIN32
		return strerror(aError);
#endif // _WIN32
	}

	static int64_t getUptime()
	{
		return mUptimeSeconds;
	}

	static void increaseUptime()
	{
		mUptimeSeconds++;
	}

	static string getFilePath(const string& path) {
		string::size_type i = path.rfind(PATH_SEPARATOR);
		return (i != string::npos) ? path.substr(0, i + 1) : path;
	}
	static string getFileName(const string& path) {
		string::size_type i = path.rfind(PATH_SEPARATOR);
		return (i != string::npos) ? path.substr(i + 1) : path;
	}
	static string getFileExt(const string& path) {
		string::size_type i = path.rfind('.');
		return (i != string::npos) ? path.substr(i) : Util::emptyString;
	}
	static string getLastDir(const string& path) {
		string::size_type i = path.rfind(PATH_SEPARATOR);
		if(i == string::npos)
			return Util::emptyString;
		string::size_type j = path.rfind(PATH_SEPARATOR, i-1);
		return (j != string::npos) ? path.substr(j+1, j-i-1) : path;
	}
	
	static void decodeUrl(const string& aUrl, string& aServer, short& aPort, string& aFile);
	static string validateFileName(string aFile);
	static string formatStatus(int iStatus) {
		string tmp = emptyString;
		switch(iStatus) {
			case 1: tmp = "Normal (1)";	break;
			case 2: tmp = "Away (2)";	break;
			case 3: tmp = "Away (3)";	break;
			case 4: tmp = "Fileserver (4)";	break;
			case 5: tmp = "Fileserver (5)";	break;
			case 6: tmp = "Fileserver Away (6)";	break;
			case 7: tmp = "Fileserver Away (7)";	break;
			case 8: tmp = "Fireball (8)";	break;
			case 9: tmp = "Fireball (9)";	break;
			case 10: tmp = "Fireball Away (10)";	break;
			case 11: tmp = "Fireball Away (11)";	break;
			default: tmp = "Unknown (" + toString(iStatus) + ")";	break;
		}
		return tmp;
	}
	
	static string formatBytes(const string& aString) {
		return formatBytes(toInt64(aString));
	}

	static string toDOS(const string& tmp);

	static string getShortTimeString();

	static string getTimeString() {
		char buf[64];
		time_t _tt;
		time(&_tt);
		tm* _tm = localtime(&_tt);
		if(_tm == NULL) {
			strcpy(buf, "xx:xx:xx");
		} else {
			strftime(buf, 64, "%X", _tm);
		}
		return buf;
	}
	
	static string toAdcFile(const string& file) {
		string ret;
		ret.reserve(file.length() + 1);
		ret += '/';
		ret += file;
		for(string::size_type i = 0; i < ret.length(); ++i) {
			if(ret[i] == '\\') {
				ret[i] = '/';
			}
		}
		return ret;
	}
	static string toNmdcFile(const string& file) {
		if(file.empty())
			return Util::emptyString;
		
		string ret(file.substr(1));
		for(string::size_type i = 0; i < ret.length(); ++i) {
			if(ret[i] == '/') {
				ret[i] = '\\';
			}
		}
		return ret;
	}
	
	static string formatBytes(int64_t aBytes);

	static string formatExactSize(int64_t aBytes);
	
	static string formatSeconds(int64_t aSec, bool supressHours = false) {
		char buf[64];
#ifdef _WIN32
		if (!supressHours)
		sprintf(buf, "%01I64d:%02d:%02d", aSec / (60*60), (int)((aSec / 60) % 60), (int)(aSec % 60));
		else
			sprintf(buf, "%02d:%02d", (int)(aSec / 60), (int)(aSec % 60));
#else
		if (!supressHours)
		sprintf(buf, "%01lld:%02d:%02d", aSec / (60*60), (int)((aSec / 60) % 60), (int)(aSec % 60));
		else
			sprintf(buf, "%02d:%02d", (int)(aSec / 60), (int)(aSec % 60));
#endif		
		return buf;
	}

	static string formatParams(const string& msg, StringMap& params);
	static string formatTime(const string &msg, const time_t t);

	static string formatNumber(int64_t aNumber) {
		char buf[64];
#ifdef _WIN32
		char number[64];
		sprintf(number, "%I64d", aNumber);
		GetNumberFormatA(LOCALE_USER_DEFAULT, 0, number, NULL, buf, sizeof(buf)/sizeof(buf[0]));
#else
		sprintf(buf, "%'lld", aNumber);
#endif		
		return buf;
	}

	static string toLower(const string& aString) { return toLower(aString.c_str(), aString.length()); };
	static string toLower(const char* aString, int len = -1) {
		string tmp;
		tmp.resize((len == -1) ? strlen(aString) : len);
		for(string::size_type i = 0; aString[i]; i++) {
			tmp[i] = toLower(aString[i]);
		}
		return tmp;
	}
	static char toLower(char c) { return lower[(u_int8_t)c]; };
	static u_int8_t toLower(u_int8_t c) { return lower[c]; };
	static void toLower2(string& aString) {
		for(string::size_type i = 0; i < aString.length(); ++i) {
			aString[i] = toLower(aString[i]);
		}
	}
	static int64_t toInt64(const string& aString) {
#ifdef _WIN32
		return _atoi64(aString.c_str());
#else
		return atoll(aString.c_str());
#endif
	}

	static int toInt(const string& aString) {
		return atoi(aString.c_str());
	}
	static u_int32_t toUInt32(const char* c) {
		return (u_int32_t)atoi(c);
	}

	static double toDouble(const string& aString) {
		return atof(aString.c_str());
	}

	static float toFloat(const string& aString) {
		return (float)atof(aString.c_str());
	}

	static string toString(const int64_t& val) {
		char buf[32];
#ifdef _WIN32
		return _i64toa(val, buf, 10);
#else
		sprintf(buf, "%lld", val);
		return buf;
#endif
	}

	static bool needsUtf8(const string& str) {
		for(string::size_type i = 0; i < str.length(); ++i)
			if(str[i] & 0x80)
				return true;
		return false;
	}
	static bool needsAcp(const string& str) {
		return needsUtf8(str);
	}
	static const string& toUtf8(const string& str, string& tmp) {
		if(needsUtf8(str)) {
			tmp = str;
			return toUtf8(tmp);
		}
		return str;
	}
	static string& toUtf8(string& str);

	static const string& toAcp(const string& str, string& tmp) {
		if(needsAcp(str)) {
			tmp = str;
			return toAcp(tmp);
		}
		return str;
	}
	static string& toAcp(string& str);

	static string toString(u_int32_t val) {
		char buf[16];
		sprintf(buf, "%lu", (unsigned long)val);
		return buf;
	}
	static string toString(int val) {
		char buf[16];
		sprintf(buf, "%d", val);
		return buf;
	}
	static string toString(long val) {
		char buf[16];
		sprintf(buf, "%ld", val);
		return buf;
	}
	static string toString(double val) {
		char buf[16];
		sprintf(buf, "%0.2f", val);
		return buf;
	}
	static string toHexEscape(char val) {
		char buf[sizeof(int)*2+1];
		sprintf(buf, "%%%X", val);
		return buf;
	}
	static char fromHexEscape(const string aString) {
		int res = 0;
		sscanf(aString.c_str(), "%X", &res);
		return static_cast<char>(res);
	}
	static string encodeURI(const string& /*aString*/, bool reverse = false);
	static string getLocalIp();
	static bool isPrivateIp(string const& ip);
	/**
	 * Case insensitive substring search.
	 * @return First position found or string::npos
	 */
	static string::size_type findSubString(const string& aString, const string& aSubString, string::size_type start = 0) {
		if(aString.length() < start)
			return (string::size_type)string::npos;

		if(aString.length() < aSubString.length())
			return (string::size_type)string::npos;

		if(aSubString.empty())
			return 0;

		u_int8_t* tx = (u_int8_t*)aString.c_str();
		u_int8_t* px = (u_int8_t*)aSubString.c_str();

		u_int8_t p = Util::toLower(px[0]);

		u_int8_t* end = tx + aString.length() - aSubString.length() + 1;

		for (tx += start; tx < end; ++tx) {
			if(p == Util::toLower(tx[0])) {
				int i = 1;

				for(; px[i] && Util::toLower(px[i]) == Util::toLower(tx[i]); ++i)
					;	// Empty

				if(px[i] == 0)
					return tx - (u_int8_t*)aString.c_str();
			}
		}
		return (string::size_type)string::npos;
	}

	static string::size_type findSubStringCaseSensitive(const string& aString, const string& aSubString, string::size_type start = 0) {
		if(aString.length() < start)
			return (string::size_type)string::npos;

		if(aString.length() < aSubString.length())
			return (string::size_type)string::npos;

		if(aSubString.empty())
			return 0;

		u_int8_t* tx = (u_int8_t*)aString.c_str();
		u_int8_t* px = (u_int8_t*)aSubString.c_str();

		u_int8_t p = px[0];

		u_int8_t* end = tx + aString.length() - aSubString.length() + 1;

		for (tx += start; tx < end; ++tx) {
			if(p == tx[0]) {
				int i = 1;

				for(; px[i] && px[i] == tx[i]; ++i)
					;	// Empty

				if(px[i] == 0)
					return tx - (u_int8_t*)aString.c_str();
			}
		}
		return (string::size_type)string::npos;
	}

	/* Table-driven versions of strnicmp and stricmp */
	static int stricmp(const char* a, const char* b) {
		// return ::stricmp(a, b);
		while(*a && (cmpi[(u_int8_t)*a][(u_int8_t)*b] == 0)) {
			a++; b++;
		}
		return cmpi[(u_int8_t)*a][(u_int8_t)*b];
	}
	static int strnicmp(const char* a, const char* b, int n) {
		// return ::strnicmp(a, b, n);
		while(n && *a && (cmpi[(u_int8_t)*a][(u_int8_t)*b] == 0)) {
			n--; a++; b++;
		}
		return (n == 0) ? 0 : cmpi[(u_int8_t)*a][(u_int8_t)*b];
	}
	static int stricmp(const string& a, const string& b) { return stricmp(a.c_str(), b.c_str()); };
	static int strnicmp(const string& a, const string& b, int n) { return strnicmp(a.c_str(), b.c_str(), n); };
	
	static string validateNick(string tmp) {	
		string::size_type i;
		while( (i = tmp.find_first_of("|$ ")) != string::npos) {
			tmp[i]='_';
		}
		return tmp;
	}

	static string validateMessage(string tmp, bool reverse, bool checkNewLines = true);
	static string validateChatMessage(string tmp);

	static string toTime(u_int32_t i) {
		char buf[64];
		if (i > 60*60) {
#ifdef _WIN32
			sprintf(buf, "%01I64d:%02d:%02d", i / (60*60), (int)((i / 60) % 60), (int)(i % 60));
#else
			sprintf(buf, "%01lld:%02d:%02d", i / (60*60), (int)((i / 60) % 60), (int)(i % 60));
#endif
		} else if (i > 60) {
#ifdef _WIN32
			sprintf(buf, "%02d:%02d", (int)((i / 60) % 60), (int)(i % 60));
#else
			sprintf(buf, "%02d:%02d", (int)((i / 60) % 60), (int)(i % 60));
#endif
		} else {
			return Util::toString(i);
		}
		return buf;
	}

	static string getOsVersion();

	static string getIpCountry (string IP);

	static int getOsMinor();
	static int getOsMajor(); 

	static bool getAway() { return away; };
	static void setAway(bool aAway);
	static string getAwayMessage();
	static string replace(string message, string r, string rw){
		string::size_type  k = 0, j = 0;
		while(( k = message .find( r , j ) ) != string::npos ){
			message.replace(k,r.size(), rw);
			j=k+rw.size();
		}
		return message;
	}

	static void setAwayMessage(const string& aMsg) { awayMsg = aMsg; };

	static u_int32_t rand();
	static u_int32_t rand(u_int32_t high) { return rand() % high; };
	static u_int32_t rand(u_int32_t low, u_int32_t high) { return rand(high-low) + low; };
	static double randd() { return ((double)rand()) / ((double)0xffffffff); };

	static string Binary2RGB(BYTE* pbData, DWORD dwSize);
	static bool RGB2Binary(string sRGB, BYTE* pbData);
	static u_int64_t getDirSize(const string &sFullPath);
	static bool validatePath(const string &sPath);
	static bool fileExists(const string &aFile);

private:
	static bool away;
	static string awayMsg;
	static time_t awayTime;
	static char upper[];
	static char lower[];
	static int8_t cmp[256][256];
	static int8_t cmpi[256][256];
	
	typedef map<u_int32_t, u_int16_t> CountryList;
	typedef CountryList::iterator CountryIter;

	static CountryList countries;
	
	static int64_t mUptimeSeconds;
};
	
class safestring
{
public:
	static bool _CorrectFindPos(const string &InStr, string::size_type &pos);

	static string::size_type SafeFind(const string &InStr, char c, string::size_type pos = 0);
	static string::size_type SafeFind(const string &InStr, const char *s, string::size_type pos = 0);
	static string::size_type SafeFind(const string &InStr, const char *s, string::size_type pos, string::size_type n);
	static string::size_type SafeFind(const string &InStr, const string& str, string::size_type pos = 0);
};

/** Case insensitive hash function for strings */
struct noCaseStringHash {
#if _MSC_VER < 1300 
	enum {bucket_size = 4}; 
	enum {min_buckets = 8}; 
#else 
	static const size_t bucket_size = 4;
	static const size_t min_buckets = 8;
#endif // _MSC_VER < 1300

	size_t operator()(const string& s) const {
		size_t x = 0;
		const char* y = s.data();
		string::size_type j = s.size();
		for(string::size_type i = 0; i < j; ++i) {
			x = x*31 + (size_t)Util::toLower(y[i]);
		}
		return x;
	}
	bool operator()(const string& a, const string& b) const {
		return Util::stricmp(a, b) == -1;
	}
};

/** Case insensitive string comparison */
struct noCaseStringEq {
	bool operator()(const string& a, const string& b) const {
		return Util::stricmp(a.c_str(), b.c_str()) == 0;
	}
};

/** Case insensitive string ordering */
struct noCaseStringLess {
	bool operator()(const string& a, const string& b) const {
		return Util::stricmp(a.c_str(), b.c_str()) < 0;
	}
};

#endif // !defined(AFX_UTIL_H__1758F242_8D16_4C50_B40D_E59B3DD63913__INCLUDED_)

/**
 * @file
 * $Id$
 */