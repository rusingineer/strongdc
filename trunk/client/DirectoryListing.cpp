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

#include "DirectoryListing.h"

#include "QueueManager.h"
#include "SearchManager.h"

#include "StringTokenizer.h"
#include "SimpleXML.h"
#include "FilteredFile.h"
#include "BZUtils.h"
#include "CryptoManager.h"
#include "User.h"

#include "../pme-1.0.4/pme.h"

#ifdef ff
#undef ff
#endif

void DirectoryListing::loadFile(const string& name) {
	string txt;

	// For now, we detect type by ending...
	string ext = Util::getFileExt(name);
	if(Util::stricmp(ext, ".DcLst") == 0) {
		size_t len = (size_t)::File::getSize(name);
		if(len == (size_t)-1)
			return;
		AutoArray<u_int8_t> buf(len);
		::File(name, ::File::READ, ::File::OPEN).read(buf, len);
		CryptoManager::getInstance()->decodeHuffman(buf, txt, len);
		load(txt);
	} else if(Util::stricmp(ext, ".bz2") == 0) {
		::File ff(name, ::File::READ, ::File::OPEN);
		FilteredInputStream<UnBZFilter, false> f(&ff);
		const size_t BUF_SIZE = 64*1024;
		char buf[BUF_SIZE];
		size_t len;
		for(;;) {
			size_t n = BUF_SIZE;
			len = f.read(buf, n);
			txt.append(buf, len);
			if(len < BUF_SIZE)
				break;
		}

			loadXML(txt);
	}
}

void DirectoryListing::load(const string& in) {
	StringTokenizer<string> t(in, '\n');

	StringList& tokens = t.getTokens();
	string::size_type indent = 0;

	Directory* cur = root;
	string fullPath;
	
	for(StringIter i = tokens.begin(); i != tokens.end(); ++i) 
	{
		string& tok = *i;
		string::size_type j = tok.find_first_not_of('\t');
		if(j == string::npos) {
			break;
		}

		while(j < indent) {
			// Wind up directory structure
			cur = cur->getParent();
			dcassert(cur != NULL);
			indent--;
			string::size_type l = fullPath.find_last_of('\\');
			if(l != string::npos) {
				fullPath.erase(fullPath.begin() + l, fullPath.end());
			}
		}

		string::size_type k = tok.find('|', j);
		if(k != string::npos) {
			// this must be a file...
			cur->files.push_back(new File(cur, tok.substr(j, k-j), Util::toInt64(tok.substr(k+1))));
		} else {
			// A directory
			string name = tok.substr(j, tok.length()-j-1);
			fullPath += '\\';
			fullPath += name;

			Directory::Iter di = ::find(cur->directories.begin(), cur->directories.end(), name);
			if(di != cur->directories.end()) {
				cur = *di;
			} else {
				Directory* d = new Directory(cur, name);
				cur->directories.push_back(d);
			cur = d;
			}
			indent++;
		}
	}
}

class ListLoader : public SimpleXMLReader::CallBack {
public:
	ListLoader(DirectoryListing::Directory* root, const User::Ptr& user) : cur(root), inListing(false), uziv(user) { 
		lastFileIter = cur->files.begin();
	};

	virtual ~ListLoader() { }

	virtual void startTag(const string& name, StringPairList& attribs, bool simple);
	virtual void endTag(const string& name, const string& data);
private:
	string fullPath;

	DirectoryListing::Directory* cur;
	DirectoryListing::File::Iter lastFileIter;
	User::Ptr uziv;

	StringMap params;
	bool inListing;
};

void DirectoryListing::loadXML(const string& xml) {
	setUtf8(true);

	ListLoader ll(getRoot(), getUser());
	SimpleXMLReader(&ll).fromXML(xml);
}

static const string sFileListing = "FileListing";
static const string sBase = "Base";
static const string sGenerator = "Generator";
static const string sDirectory = "Directory";
static const string sIncomplete = "Incomplete";
static const string sFile = "File";
static const string sName = "Name";
static const string sSize = "Size";
static const string sTTH = "TTH";

void ListLoader::startTag(const string& name, StringPairList& attribs, bool simple) {
	if(inListing) {
		if(name == sFile) {
			const string& n = getAttrib(attribs, sName, 0);
			if(n.empty())
				return;
			const string& s = getAttrib(attribs, sSize, 1);
			if(s.empty())
				return;
			const string& h = getAttrib(attribs, sTTH, 2);
			DirectoryListing::File* f = h.empty() ? new DirectoryListing::File(cur, n, Util::toInt64(s)) : new DirectoryListing::File(cur, n, Util::toInt64(s), h);
			cur->files.push_back(f);
		} else if(name == sDirectory) {
			const string& n = getAttrib(attribs, sName, 0);
			if(n.empty()) {
				throw SimpleXMLException("Directory missing name attribute");
			}
			bool incomp = getAttrib(attribs, sIncomplete, 1) == "1";
			DirectoryListing::Directory* d = new DirectoryListing::Directory(cur, n, false, incomp);
			cur->directories.push_back(d);
			cur = d;
			fullPath += '\\';
			fullPath += d->getName();

			if(simple) {
				// To handle <Directory Name="..." />
				endTag(name, Util::emptyString);
			}
		}
	} else if(name == sFileListing) {
		const string& base = getAttrib(attribs, sBase, 2);
		if(base.size() > 1 && base[0] == '/' && base[base.size()-1] == '/') {
			StringList sl = StringTokenizer<string>(base.substr(1), '/').getTokens();
			for(StringIter i = sl.begin(); i != sl.end(); ++i) {
				DirectoryListing::Directory* d = new DirectoryListing::Directory(cur, *i);
				cur->directories.push_back(d);
				cur = d;
				fullPath += '\\';
				fullPath += d->getName();
			}
		}
		const string& generator = getAttrib(attribs, sGenerator, 2);
		if(uziv->isOnline()) {
			PME reg("^<StrgDC\\+\\+ V:1.00 RC([89]){1}");
			if((generator == "DC++ 0.403")) {
				if(reg.match(uziv->getTag())) {
					uziv->setCheat("rmDC++ in StrongDC++ "+uziv->getVersion()+" emulation mode" , true);
					uziv->setClientType("rmDC++ 0.403");
					uziv->setBadClient(true);
					uziv->setBadFilelist(true);
				}
			}
			uziv->setGenerator(generator);
		}
		inListing = true;
	}
}

void ListLoader::endTag(const string& name, const string&) {
	if(inListing) {
		if(name == sDirectory) {
			cur = cur->getParent();
			dcassert(fullPath.find('\\') != string::npos);
			fullPath.erase(fullPath.rfind('\\'));
			lastFileIter = cur->files.begin();
		} else if(name == sFileListing) {
			// cur should be root now...
			inListing = false;
		}
	}
}

string DirectoryListing::getPath(Directory* d) {
	string dir;
	dir.reserve(128);
	dir.append(d->getName());
	dir.append(1, '\\');

	Directory* cur = d->getParent();
	while(cur!=root) {
		dir.insert(0, cur->getName() + '\\');
		cur = cur->getParent();
	}
	return dir;
}

static inline const string& escaper(const string& n, string& tmp, bool utf8) {
	return utf8 ? n : (tmp.clear(), Text::acpToUtf8(n, tmp));
}

void DirectoryListing::download(Directory* aDir, const string& aTarget, bool highPrio, QueueItem::Priority prio, bool multiSource) {
	string tmp;
	string target = (aDir == getRoot()) ? aTarget : aTarget + escaper(aDir->getName(), tmp, getUtf8()) + '\\';
	// First, recurse over the directories
	Directory::List& lst = aDir->directories;
	sort(lst.begin(), lst.end(), Directory::DirSort());
	for(Directory::Iter j = lst.begin(); j != lst.end(); ++j) {
		download(*j, target, highPrio, prio, multiSource);
	}
	// Then add the files
	File::List& l = aDir->files;
	sort(l.begin(), l.end(), File::FileSort());
	for(File::Iter i = aDir->files.begin(); i != aDir->files.end(); ++i) {
		File* file = *i;
		try {
			download(file, target + escaper(file->getName(), tmp, getUtf8()), false, highPrio, prio, multiSource);
		} catch(const QueueException&) {
			// Catch it here to allow parts of directories to be added...
		} catch(const FileException&) {
			//..
		}
	}
}

void DirectoryListing::download(const string& aDir, const string& aTarget, bool highPrio, QueueItem::Priority prio, bool multiSource) {
	dcassert(aDir.size() > 2);
	dcassert(aDir[aDir.size() - 1] == '\\');
	Directory* d = find(aDir, getRoot());
	if(d != NULL)
		download(d, aTarget, highPrio, prio, multiSource);
}

void DirectoryListing::download(File* aFile, const string& aTarget, bool view, bool highPrio, QueueItem::Priority prio, bool multiSource) {

	int flags = (getUtf8() ? QueueItem::FLAG_SOURCE_UTF8 : 0) | (multiSource ? QueueItem::FLAG_MULTI_SOURCE : 0) |
		(view ? (QueueItem::FLAG_TEXT | QueueItem::FLAG_CLIENT_VIEW) : QueueItem::FLAG_RESUME);

	QueueManager::getInstance()->add(getPath(aFile) + aFile->getName(), aFile->getSize(), user, aTarget, 
		aFile->getTTH(), flags, highPrio || view ? QueueItem::HIGHEST : prio);
}

void DirectoryListing::downloadMP3(File* aFile, const string& aTarget) {
	int flags = (getUtf8() ? QueueItem::FLAG_SOURCE_UTF8 : 0) | QueueItem::FLAG_MP3_INFO;

	QueueManager::getInstance()->add(getPath(aFile) + aFile->getName(), 2100, user, aTarget, 
		NULL, flags, QueueItem::Priority::HIGHEST);
}

DirectoryListing::Directory* DirectoryListing::find(const string& aName, Directory* current) {
	string::size_type end = aName.find('\\');
	dcassert(end != string::npos);
	string name = aName.substr(0, end);

	Directory::Iter i = ::find(current->directories.begin(), current->directories.end(), name);
	if(i != current->directories.end()) {
		if(end == (aName.size() - 1))
			return *i;
		else
			return find(aName.substr(end + 1), *i);
	}
	return NULL;
}

int64_t DirectoryListing::Directory::getTotalSize(bool adl) {
	if(parent != NULL && parent->parent == NULL) {
		PME reg("([A-Z])");
		if(reg.match(getName())) {
			parent->rmDC403D1detected = true;
		}
	}

	int64_t x = getSize();
	for(Iter i = directories.begin(); i != directories.end(); ++i) {
		if(!(adl && (*i)->getAdls()))
			x += (*i)->getTotalSize(adls);
	}
	return x;
}

size_t DirectoryListing::Directory::getTotalFileCount(bool adl) {
	size_t x = getFileCount();
	for(Iter i = directories.begin(); i != directories.end(); ++i) {
		if(!(adl && (*i)->getAdls()))
			x += (*i)->getTotalFileCount(adls);
	}
	return x;
}

/**
 * @file
 * $Id$
 */
