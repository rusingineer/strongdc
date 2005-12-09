/* 
 * Copyright (C) 2001-2005 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(NMDC_HUB_H)
#define NMDC_HUB_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TimerManager.h"
#include "SettingsManager.h"

#include "ClientManager.h"

#include "BufferedSocket.h"
#include "User.h"
#include "CriticalSection.h"
#include "Text.h"
#include "Client.h"
#include "ConnectionManager.h"
#include "UploadManager.h"
#include "StringTokenizer.h"
#include "ZUtils.h"

class NmdcHub : public Client, private TimerManagerListener, private Flags
{
	friend class ClientManager;
public:
	typedef NmdcHub* Ptr;
	typedef list<Ptr> List;
	typedef List::iterator Iter;

	enum SupportFlags {
		SUPPORTS_USERCOMMAND = 0x01,
		SUPPORTS_NOGETINFO = 0x02,
		SUPPORTS_USERIP2 = 0x04,
		SUPPORTS_QUICKLIST = 0x08,
		SUPPORTS_ZLINE = 0x10
	};

#define checkstate() if(state != STATE_CONNECTED) return

	virtual void connect(const OnlineUser& aUser);
	virtual void hubMessage(const string& aMessage) {
		checkstate();
		char buf[256];
		sprintf(buf, "<%s> ", getMyNick().c_str());
		send(toNmdc(string(buf)+Util::validateChatMessage(aMessage)+"|"));
	}
	virtual void privateMessage(const OnlineUser& aUser, const string& aMessage);
	virtual void sendUserCmd(const string& aUserCmd) throw() { send(toNmdc(aUserCmd)); }
	virtual void search(int aSizeType, int64_t aSize, int aFileType, const string& aString, const string& aToken);
	virtual void password(const string& aPass) { send("$MyPass " + toNmdc(aPass) + "|"); }
	virtual void info() { myInfo(); }

	virtual void cheatMessage(const string& aLine) {
		fire(ClientListener::CheatMessage(), this, Util::validateMessage(aLine, true));
	}    

	virtual size_t getUserCount() const {  Lock l(cs); return users.size(); }
	//virtual int64_t getAvailable() const;

	virtual string escape(string const& str) const { return Util::validateMessage(str, false); };

	void disconnect() throw();
	void myInfo();

	void refreshUserList(bool unknownOnly = false);
	
	void validateNick(const string& aNick) {
		if (validatenicksent == false) {
			send("$ValidateNick " + toNmdc(aNick) + "|");
			validatenicksent = true;
		}
	};
	void key(const string& aKey) { send("$Key " + aKey + "|"); };	
	void version() { send("$Version 1,0091|"); };
	void getNickList() { checkstate(); send("$GetNickList|"); };
	void getInfo(const OnlineUser& aUser) { checkstate(); send("$GetINFO " + toNmdc(aUser.getIdentity().getNick()) + " " + toNmdc(getMyNick()) + "|"); };

	void sendRaw(const string& aRaw) { send(toNmdc(aRaw)); }
	
	void connectToMe(const OnlineUser& aUser);
	void revConnectToMe(const OnlineUser& aUser);

/*	void privateMessage(const string& aNick, const string& aMessage) {
		checkstate(); 
		char buf[512];
		sprintf(buf, "$To: %s From: %s $", toNmdc(aNick).c_str(), toNmdc(getMyNick()).c_str());
		send(string(buf)+toNmdc(Util::validateChatMessage(aMessage))+"|");
	}
*/
	void supports(const StringList& feat) { 
		string x;
		for(StringList::const_iterator i = feat.begin(); i != feat.end(); ++i) {
			x+= *i + ' ';
		}
		send("$Supports " + x + '|');
	}

	//GETSET(int, supportFlags, SupportFlags);
private:
	enum States {
		STATE_CONNECT,
		STATE_LOCK,
		STATE_HELLO,
		STATE_MYINFO,
		STATE_CONNECTED
	} state;

	mutable CriticalSection cs;

	typedef HASH_MAP_X(string, OnlineUser*, noCaseStringHash, noCaseStringEq, noCaseStringLess) NickMap;
	typedef NickMap::iterator NickIter;

	NickMap users;

	bool reconnect;
	string lastmyinfo;
	bool validatenicksent, bFirstOpList;
	int64_t lastbytesshared;
	bool PtokaX, YnHub;

	typedef list<pair<string, u_int32_t> > FloodMap;
	typedef FloodMap::iterator FloodIter;
	FloodMap seekers;
	FloodMap flooders;

	NmdcHub(const string& aHubURL);	
	virtual ~NmdcHub() throw();

	// Dummy
	NmdcHub(const NmdcHub&);
	NmdcHub& operator=(const NmdcHub&);

	void connect();

	void clearUsers();
	void onLine(const char* aLine) throw();

	OnlineUser& getUser(const string& aNick);
	OnlineUser* findUser(const string& aNick);
	void putUser(const string& aNick);
	
	string fromNmdc(const string& str) const { return Text::acpToUtf8(str); }
	string toNmdc(const string& str) const { return Text::utf8ToAcp(str); }

	void updateFromTag(Identity& id, const string& tag);

	virtual string checkNick(const string& aNick);

	// TimerManagerListener
	virtual void on(TimerManagerListener::Second, u_int32_t aTick) throw();

	virtual void on(Line, const string& l) throw() {
		if(l.substr(0, 3) == "$Z ") { 
			string::size_type i = 0;
			string rawParam = l.substr(3);
			bool corrupt = false; 

			// unescape \\ to \ and \P to |  
			while((i = rawParam.find("\\", i)) != string::npos && !corrupt) { 
				if (i + 1 < rawParam.size()) { 
					switch (rawParam[i+1]) { 
						case '\\': 
							rawParam.replace(i++, 2, "\\"); 
							break; 
						case 'P': 
							rawParam.replace(i++, 2, "|"); 
							break; 
						default: 
							corrupt = true; 
							break; 
					} 
				} else { 
					corrupt = true; 
				} 
			} 
 
			if (!corrupt) {
				// unzip the ZBlock
				UnZFilter filter;
				string lines = "";
				bool more;
				string::size_type j, readFromPos = 0, estOutSize = rawParam.size() / 4;
				AutoArray<u_int8_t> temp(estOutSize);

				while (true) {
					j = estOutSize;
					try {
						more = filter((rawParam.substr(readFromPos)).c_str(), i, temp, j);
					} catch(...){
						dcdebug("Error during Zline decompression\n");
						break;
					}
					lines += string((char*)(u_int8_t*)temp, j);
					readFromPos += i;

					// split lines up into indiviual commands
					StringTokenizer<string> st(lines, '|');
					for(StringList::iterator k = st.getTokens().begin(); k != st.getTokens().end(); ++k) {
						if (k + 1 == st.getTokens().end() && more)
						// last token, roll over if more data to decompress
							lines = *k;
						else {
							// "fire" the lines
							COMMAND_DEBUG(*k, DebugManager::HUB_IN, getIpPort()); 
							onLine((*k).c_str()); 
						}
					}

					// a nmdc command over 1mb ?
					if (lines.size() > 1048576) {
						dcdebug("Malicious data found during ZLine decompression\n");
						break;
					}

					// no more data to decompress
					if (!more)
						break;
				}
			} else {
				dcdebug("Corrupt Zline datastream\n");
			}
		} else { 
			COMMAND_DEBUG(fromNmdc(l), DebugManager::HUB_IN, getIpPort()); 
			onLine(l.c_str());
		}
	}
	virtual void on(Failed, const string&) throw();

};

#endif // !defined(NMDC_HUB_H)

/**
 * @file
 * $Id$
 */
