/*
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_FILE_H
#define DCPLUSPLUS_DCPP_FILE_H

#include "SettingsManager.h"

#include "Util.h"
#include "Text.h"
#include "Streams.h"

#ifndef _WIN32
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <fnmatch.h>
#endif

#ifdef _WIN32
#include "../zlib/zlib.h"
#else
#include <zlib.h>
#endif

namespace dcpp {

class File : public IOStream {
public:
	enum {
		OPEN = 0x01,
		CREATE = 0x02,
		TRUNCATE = 0x04,
		SHARED = 0x08
	};

#ifdef _WIN32
	enum {
		READ = GENERIC_READ,
		WRITE = GENERIC_WRITE,
		RW = READ | WRITE
	};

	static uint32_t convertTime(FILETIME* f);

#else // !_WIN32

	enum {
		READ = 0x01,
		WRITE = 0x02,
		RW = READ | WRITE
	};

	// some ftruncate implementations can't extend files like SetEndOfFile,
	// not sure if the client code needs this...
	int extendFile(int64_t len) throw();

#endif // !_WIN32

	File(const string& aFileName, int access, int mode) throw(FileException);

	bool isOpen() const throw();
	void close() throw();
	int64_t getSize() const throw();
	void setSize(int64_t newSize) throw(FileException);

	int64_t getPos() const throw();
	void setPos(int64_t pos) throw();
	void setEndPos(int64_t pos) throw();
	void movePos(int64_t pos) throw();
	void setEOF() throw(FileException);

	size_t read(void* buf, size_t& len) throw(FileException);
	size_t write(const void* buf, size_t len) throw(FileException);
	size_t flush() throw(FileException);

	uint32_t getLastModified() const throw();

	static void copyFile(const string& src, const string& target) throw(FileException);
	static void renameFile(const string& source, const string& target) throw(FileException);
	static void deleteFile(const string& aFileName) throw();

	static int64_t getSize(const string& aFileName) throw();

	static void ensureDirectory(const string& aFile) throw();
	static bool isAbsolute(const string& path) throw();

	~File() throw() { File::close(); }

	string read(size_t len) throw(FileException);
	string read() throw(FileException);
	void write(const string& aString) throw(FileException) { write((void*)aString.data(), aString.size()); }
	static StringList findFiles(const string& path, const string& pattern);

protected:
#ifdef _WIN32
	HANDLE h;
#else
	int h;
#endif
private:
	File(const File&);
	File& operator=(const File&);
};

class FileFindIter {
#ifdef _WIN32
public:
	/** End iterator constructor */
	FileFindIter() : handle(INVALID_HANDLE_VALUE) { }
	/** Begin iterator constructor, path in utf-8 */
	FileFindIter(const string& path) : handle(INVALID_HANDLE_VALUE) {
		handle = ::FindFirstFile(Text::toT(path).c_str(), &data);
	}

	~FileFindIter() {
		if(handle != INVALID_HANDLE_VALUE) {
			::FindClose(handle);
		}
	}

	FileFindIter& operator++() {
		if(!::FindNextFile(handle, &data)) {
			::FindClose(handle);
			handle = INVALID_HANDLE_VALUE;
		}
		return *this;
	}
	bool operator!=(const FileFindIter& rhs) const { return handle != rhs.handle; }

	struct DirData : public WIN32_FIND_DATA {
		string getFileName() {
			return Text::fromT(cFileName);
		}

		bool isDirectory() {
			return (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0;
		}

		bool isHidden() {
			return ((dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) || (cFileName[0] == L'.'));
		}

		bool isLink() {
			return false;
		}

		int64_t getSize() {
			return (int64_t)nFileSizeLow | ((int64_t)nFileSizeHigh)<<32;
		}

		uint32_t getLastWriteTime() {
			return File::convertTime(&ftLastWriteTime);
		}
	};


private:
	HANDLE handle;
#else
	// This code has been cleaned up/fixed a little.
public:
	FileFindIter() {
		dir = NULL;
		data.ent = NULL;
	}

	~FileFindIter() {
		if (dir) closedir(dir);
	}

	FileFindIter(const string& name) {
		string filename = Text::fromUtf8(name);
		dir = opendir(filename.c_str());
		if (!dir)
			return;
		data.base = filename;
		data.ent = readdir(dir);
		if (!data.ent) {
			closedir(dir);
			dir = NULL;
		}
	}

	FileFindIter& operator++() {
		if (!dir)
			return *this;
		data.ent = readdir(dir);
		if (!data.ent) {
			closedir(dir);
			dir = NULL;
		}
		return *this;
	}

	// good enough to to say if it's null
	bool operator !=(const FileFindIter& rhs) const {
		return dir != rhs.dir;
	}

	struct DirData {
		DirData() : ent(NULL) {}
		string getFileName() {
			if (!ent) return Util::emptyString;
			return Text::toUtf8(ent->d_name);
		}
		bool isDirectory() {
			struct stat inode;
			if (!ent) return false;
			if (stat((base + PATH_SEPARATOR + ent->d_name).c_str(), &inode) == -1) return false;
			return S_ISDIR(inode.st_mode);
		}
		bool isHidden() {
			if (!ent) return false;
			return ent->d_name[0] == '.';
		}
		bool isLink() {
			struct stat inode;
			if (!ent) return false;
			if (lstat((base + PATH_SEPARATOR + ent->d_name).c_str(), &inode) == -1) return false;
			return S_ISLNK(inode.st_mode);
		}
		int64_t getSize() {
			struct stat inode;
			if (!ent) return false;
			if (stat((base + PATH_SEPARATOR + ent->d_name).c_str(), &inode) == -1) return 0;
			return inode.st_size;
		}
		uint32_t getLastWriteTime() {
			struct stat inode;
			if (!ent) return false;
			if (stat((base + PATH_SEPARATOR + ent->d_name).c_str(), &inode) == -1) return 0;
			return inode.st_mtime;
		}
		struct dirent* ent;
		string base;
	};
private:
	DIR* dir;
#endif

public:

	DirData& operator*() { return data; }
	DirData* operator->() { return &data; }

private:
	DirData data;
};

} // namespace dcpp

#endif // !defined(FILE_H)

/**
 * @file
 * $Id$
 */
