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

#ifndef UTIL_H
#define UTIL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#endif

#include "Text.h"
#ifdef _WIN32
#define SETTINGS_DIR "Settings\\"
#else
#define SETTINGS_DIR "settings/"
#endif

template<typename T, bool flag> struct ReferenceSelector {
	typedef T ResultType;
};
template<typename T> struct ReferenceSelector<T,true> {
	typedef const T& ResultType;
};

template<typename T> class IsOfClassType {
public:
	template<typename U> static char check(int U::*);
	template<typename U> static float check(...);
public:
	enum { Result = sizeof(check<T>(0)) };
};

template<typename T> struct TypeTraits {
	typedef IsOfClassType<T> ClassType;
	typedef ReferenceSelector<T, ((ClassType::Result == 1) || (sizeof(T) > sizeof(char*)) ) > Selector;
	typedef typename Selector::ResultType ParameterType;
};

#define GETSET(type, name, name2) \
private: type name; \
public: TypeTraits<type>::ParameterType get##name2() const { return name; }; \
	void set##name2(TypeTraits<type>::ParameterType a##name2) { name = a##name2; };

#define LIT(x) x, (sizeof(x)-1)

/** Evaluates op(pair<T1, T2>.first, compareTo) */
template<class T1, class T2, class op = equal_to<T1> >
class CompareFirst {
public:
	CompareFirst(const T1& compareTo) : a(compareTo) { };
	bool operator()(const pair<T1, T2>& p) { return op()(p.first, a); };
private:
	CompareFirst& operator=(const CompareFirst&);
	const T1& a;
};

/** Evaluates op(pair<T1, T2>.second, compareTo) */
template<class T1, class T2, class op = equal_to<T2> >
class CompareSecond {
public:
	CompareSecond(const T2& compareTo) : a(compareTo) { };
	bool operator()(const pair<T1, T2>& p) { return op()(p.second, a); };
private:
	CompareSecond& operator=(const CompareSecond&);
	const T2& a;
};

template<class T>
struct PointerHash {
#if _MSC_VER >= 1300 
	static const size_t bucket_size = 4; 
	static const size_t min_buckets = 8; 
#endif 
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
	static tstring emptyStringT;
	static string emptyString;
	static wstring emptyStringW;

	static void initialize();

	/**
	 * Get the path to the application executable. 
	 * This is mainly intended for use on Windows.
	 *
	 * @return Path to executable file.
	 */
	static string getAppPath() { return appPath; }

	static string getAppName() {
#ifdef _WIN32
		TCHAR buf[MAX_PATH+1];
		DWORD x = GetModuleFileName(NULL, buf, MAX_PATH);
		return Text::wideToUtf8(wstring(buf, x));
#else // _WIN32
		char buf[PATH_MAX + 1];
		int n;
		n = readlink("/proc/self/exe", buf, PATH_MAX);
		if (n == -1) {
			return emptyString;
		}
		buf[n] = '\0';
		return string(buf);
#endif // _WIN32
	}	

	/**
	 * Get the path to where the applications settings.
	 * 
	 * @return Path to settings directory.
	 */
	static string getConfigPath();

	/**
	 * Get the directory for temporary files.
	 *
	 * @return Path to temp directory.
	 */
	static string getTempPath() {
#ifdef _WIN32
		TCHAR buf[MAX_PATH + 1];
		DWORD x = GetTempPath(MAX_PATH, buf);
		return Text::wideToUtf8(wstring(buf, x));
#else
		return "/tmp/";
#endif
	}

	/**
	 * Get the directory to the application resources.
	 *
	 * @todo On non Windows system this still returns the path to the
	 * configuration directory. Later this will be completed with a patch for
	 * Mac OS X. And the Linux(?) implementation is also wrong right now.
	 * @return Path to resource directory.
	 */
	static string getDataPath() {
#ifdef _WIN32
		return getAppPath();
#else
		char* home = getenv("HOME");
		if (home) {
			return string(home) + "/.strongdc++/";
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
		string tmp = Text::fromT((LPCTSTR)lpMsgBuf);
		// Free the buffer.
		LocalFree( lpMsgBuf );
		string::size_type i = 0;

		while( (i = tmp.find_first_of("\r\n", i)) != string::npos) {
			tmp.erase(i, 1);
		}
		return tmp;
#else // _WIN32
		return strerror(aError);
#endif // _WIN32
	}

	static int64_t getUptime() {
		return mUptimeSeconds;
	}

	static void increaseUptime() {
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
		return (j != string::npos) ? path.substr(j+1, i-j-1) : path;
	}

	static wstring getFilePath(const wstring& path) {
		wstring::size_type i = path.rfind(PATH_SEPARATOR);
		return (i != wstring::npos) ? path.substr(0, i + 1) : path;
	}
	static wstring getFileName(const wstring& path) {
		wstring::size_type i = path.rfind(PATH_SEPARATOR);
		return (i != wstring::npos) ? path.substr(i + 1) : path;
	}
	static wstring getFileExt(const wstring& path) {
		wstring::size_type i = path.rfind('.');
		return (i != wstring::npos) ? path.substr(i) : Util::emptyStringW;
	}
	static wstring getLastDir(const wstring& path) {
		wstring::size_type i = path.rfind(PATH_SEPARATOR);
		if(i == wstring::npos)
			return Util::emptyStringW;
		wstring::size_type j = path.rfind(PATH_SEPARATOR, i-1);
		return (j != wstring::npos) ? path.substr(j+1, i-j-1) : path;
	}

	static void decodeUrl(const string& aUrl, string& aServer, u_int16_t& aPort, string& aFile);
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
		if(file == "files.xml.bz2" || file == "MyList.DcLst")
			return file;

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
			_snprintf(buf, 63, "%01I64d:%02d:%02d", aSec / (60*60), (int)((aSec / 60) % 60), (int)(aSec % 60));
		else
			_snprintf(buf, 63, "%02d:%02d", (int)(aSec / 60), (int)(aSec % 60));
#else
		if (!supressHours)
			_snprintf(buf, 63, "%01lld:%02d:%02d", aSec / (60*60), (int)((aSec / 60) % 60), (int)(aSec % 60));
		else
			_snprintf(buf, 63, "%02d:%02d", (int)(aSec / 60), (int)(aSec % 60));
#endif		
		buf[63] = 0;
		return buf;
	}

	static string formatParams(const string& msg, StringMap& params);
	static string formatTime(const string &msg, const time_t t);
	static string formatRegExp(const string& msg, StringMap& params);

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
	static u_int32_t toUInt32(const string& str) {
		return toUInt32(str.c_str());
	}
	static u_int32_t toUInt32(const char* c) {
		return (u_int32_t)atoi(c);
	}

	static double toDouble(const string& aString) {
		// Work-around for atof and locales...
		lconv* lv = localeconv();
		string::size_type i = aString.find_last_of(".,");
		if(i != string::npos && aString[i] != lv->decimal_point[0]) {
			string tmp(aString);
			tmp[i] = lv->decimal_point[0];
			return atof(tmp.c_str());
		}
		return atof(aString.c_str());
	}

	static float toFloat(const string& aString) {
		return (float)toDouble(aString.c_str());
	}

	static string toString(short val) {
		char buf[8];
		_snprintf(buf, 7, "%d", (int)val);
		buf[7] = 0;
		return buf;
	}
	static string toString(unsigned short val) {
		char buf[8];
		_snprintf(buf, 7, "%u", (unsigned int)val);
		buf[7] = 0;
		return buf;
	}
	static string toString(int val) {
		char buf[16];
		_snprintf(buf, 15, "%d", val);
		buf[15] = 0;
		return buf;
	}
	static string toString(unsigned int val) {
		char buf[16];
		_snprintf(buf, 15, "%u", val);
		buf[15] = 0;
		return buf;
	}
	static string toString(long val) {
		char buf[32];
		_snprintf(buf, 31, "%ld", val);
		buf[31] = 0;
		return buf;
	}
	static string toString(unsigned long val) {
		char buf[32];
		_snprintf(buf, 31, "%lu", val);
		buf[31] = 0;
		return buf;
	}
	static string toString(long long val) {
		char buf[32];
#ifdef _MSC_VER
		_snprintf(buf, 31, "%I64d", val);
#else
		_snprintf(buf, 31, "%lld", val);
#endif
		buf[31] = 0;
		return buf;
	}
	static string toString(unsigned long long val) {
		char buf[32];
#ifdef _MSC_VER
		_snprintf(buf, 31, "%I64u", val);
#else
		_snprintf(buf, 31, "%llu", val);
#endif
		buf[31] = 0;
		return buf;
	}
	static string toString(double val) {
		char buf[16];
		_snprintf(buf, 15, "%0.2f", val);
		buf[15] = 0;
		return buf;
	}
	static string toHexEscape(char val) {
		char buf[sizeof(int)*2+1+1];
		_snprintf(buf, sizeof(int)*2+1, "%%%X", val&0x0FF);
		buf[sizeof(int)*2+1] = 0;
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
	static string::size_type findSubString(const string& aString, const string& aSubString, string::size_type start = 0) throw();

	/* Utf-8 versions of strnicmp and stricmp, unicode char code order (!) */
	static int stricmp(const char* a, const char* b);
	static int strnicmp(const char* a, const char* b, size_t n);

	static int stricmp(const wchar_t* a, const wchar_t* b) {
		while(*a && Text::toLower(*a) == Text::toLower(*b))
			++a, ++b;

		return ((int)Text::toLower(*a)) - ((int)Text::toLower(*b));
	}
	static int strnicmp(const wchar_t* a, const wchar_t* b, size_t n) {
		while(n && *a && Text::toLower(*a) == Text::toLower(*b))
			--n, ++a, ++b;

		return n == 0 ? 0 : ((int)Text::toLower(*a)) - ((int)Text::toLower(*b));
	}

	static int stricmp(const string& a, const string& b) { return stricmp(a.c_str(), b.c_str()); };
	static int strnicmp(const string& a, const string& b, size_t n) { return strnicmp(a.c_str(), b.c_str(), n); };
	static int stricmp(const wstring& a, const wstring& b) { return stricmp(a.c_str(), b.c_str()); };
	static int strnicmp(const wstring& a, const wstring& b, size_t n) { return strnicmp(a.c_str(), b.c_str(), n); };

	static string validateMessage(string tmp, bool reverse, bool checkNewLines = true);
	static string validateChatMessage(string tmp);

	static string getOsVersion();

	static string getIpCountry (string IP);

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
	static u_int64_t getDirSize(const string &sFullPath);
	static bool validatePath(const string &sPath);
	static bool fileExists(const string &aFile);


	static u_int32_t rand();
	static u_int32_t rand(u_int32_t high) { return rand() % high; };
	static u_int32_t rand(u_int32_t low, u_int32_t high) { return rand(high-low) + low; };
	static double randd() { return ((double)rand()) / ((double)0xffffffff); };
	static bool isNumeric(wchar_t c) {
		return (c >= '0' && c <= '9') ? true : false;
	}

	static string toTime(u_int32_t i) {
		char buf[64];
		if (i > 60*60) {
#ifdef _WIN32
			_snprintf(buf, 63, "%01I64d:%02d:%02d", i / (60*60), (int)((i / 60) % 60), (int)(i % 60));
#else
			_snprintf(buf, 63, "%01lld:%02d:%02d", i / (60*60), (int)((i / 60) % 60), (int)(i % 60));
#endif
		} else if (i > 60) {
#ifdef _WIN32
			_snprintf(buf, 63, "%02d:%02d", (int)((i / 60) % 60), (int)(i % 60));
#else
			_snprintf(buf, 63, "%02d:%02d", (int)((i / 60) % 60), (int)(i % 60));
#endif
		} else {
			return Util::toString(i);
		}
		buf[63] = 0;
		return buf;
	}
	static bool nlfound;
	static int nlspeed;
	static TCHAR* strstr(const TCHAR *str1, const TCHAR *str2, int *pnIdxFound);
private:
	static string appPath;
	static string dataPath;
	static bool away;
	static string awayMsg;
	static time_t awayTime;
	
	typedef map<u_int32_t, u_int16_t> CountryList;
	typedef CountryList::iterator CountryIter;

	static CountryList countries;
	
	static int64_t mUptimeSeconds;
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

	size_t operator()(const string* s) const {
		return operator()(*s);
	}

	size_t operator()(const string& s) const {
		size_t x = 0;
		const char* end = s.data() + s.size();
		for(const char* str = s.data(); str < end; ) {
			wchar_t c = 0;
			int n = Text::utf8ToWc(str, c);
			if(n == -1) {
				str++;
			} else {
				x = x*31 + (size_t)Text::toLower(c);
				str += n;
			}
		}
		return x;
	}

	size_t operator()(const wstring* s) const {
		return operator()(*s);
	}
	size_t operator()(const wstring& s) const {
		size_t x = 0;
		const wchar_t* y = s.data();
		wstring::size_type j = s.size();
		for(wstring::size_type i = 0; i < j; ++i) {
			x = x*31 + (size_t)Text::toLower(y[i]);
		}
		return x;
	}
};

/** Case insensitive string comparison */
struct noCaseStringEq {
	bool operator()(const string* a, const string* b) const {
		return a == b || Util::stricmp(*a, *b) == 0;
	}
	bool operator()(const string& a, const string& b) const {
		return Util::stricmp(a, b) == 0;
	}
	bool operator()(const wstring* a, const wstring* b) const {
		return a == b || Util::stricmp(*a, *b) == 0;
	}
	bool operator()(const wstring& a, const wstring& b) const {
		return Util::stricmp(a, b) == 0;
	}
};

/** Case insensitive string ordering */
struct noCaseStringLess {
	bool operator()(const string* a, const string* b) const {
		return Util::stricmp(*a, *b) < 0;
	}
	bool operator()(const string& a, const string& b) const {
		return Util::stricmp(a, b) < 0;
	}
	bool operator()(const wstring* a, const wstring* b) const {
		return Util::stricmp(*a, *b) < 0;
	}
	bool operator()(const wstring& a, const wstring& b) const {
		return Util::stricmp(a, b) < 0;
	}
};

#endif // UTIL_H

/**
 * @file
 * $Id$
 */