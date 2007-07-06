/*
 * Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_CLIENT_USER_CONNECTION_H
#define DCPLUSPLUS_CLIENT_USER_CONNECTION_H

#include "forward.h"
#include "TimerManager.h"
#include "UserConnectionListener.h"
#include "BufferedSocketListener.h"
#include "BufferedSocket.h"
#include "CriticalSection.h"
#include "File.h"
#include "User.h"
#include "AdcCommand.h"
#include "MerkleTree.h"
#include "DebugManager.h"
#include "ClientManager.h"

class UserConnection : public Speaker<UserConnectionListener>, 
	private BufferedSocketListener, public Flags, private CommandHandler<UserConnection>
{
public:
	friend class ConnectionManager;
	
	static const string FEATURE_GET_ZBLOCK;
	static const string FEATURE_MINISLOTS;
	static const string FEATURE_XML_BZLIST;
	static const string FEATURE_ADCGET;
	static const string FEATURE_ZLIB_GET;
	static const string FEATURE_TTHL;
	static const string FEATURE_TTHF;
	static const string FEATURE_ADC_BASE;
	static const string FEATURE_ADC_BZIP;

	static const string FILE_NOT_AVAILABLE;
	
	enum Modes {	
		MODE_COMMAND = BufferedSocket::MODE_LINE,
		MODE_DATA = BufferedSocket::MODE_DATA
	};

	enum Flags {
		FLAG_NMDC					= 0x01,
		FLAG_OP						= 0x02,
		FLAG_UPLOAD					= 0x04,
		FLAG_DOWNLOAD				= 0x08,
		FLAG_INCOMING				= 0x10,
		FLAG_ASSOCIATED				= 0x20,
		FLAG_HASSLOT				= 0x40,
		FLAG_HASEXTRASLOT			= 0x80,
		FLAG_SUPPORTS_MINISLOTS		= 0x100,
		FLAG_SUPPORTS_XML_BZLIST	= 0x200,
		FLAG_SUPPORTS_ADCGET		= 0x400,
		FLAG_SUPPORTS_ZLIB_GET		= 0x800,
		FLAG_SUPPORTS_TTHL			= 0x1000,
		FLAG_SUPPORTS_TTHF			= 0x2000,
		FLAG_STEALTH				= 0x4000,
		FLAG_SECURE					= 0x8000
	};
	
	enum States {
		// ConnectionManager
		STATE_UNCONNECTED,
		STATE_CONNECT,

		// Handshake
		STATE_SUPNICK,		// ADC: SUP, Nmdc: $Nick
		STATE_INF,
		STATE_LOCK,
		STATE_DIRECTION,
		STATE_KEY,

		// UploadManager
		STATE_GET,			// Waiting for GET
		STATE_SEND,			// Waiting for $Send
		STATE_RUNNING,		// Transmitting data

		// DownloadManager
		STATE_FILELENGTH,
		STATE_TREE

	};

	short getNumber() const { return (short)((((size_t)this)>>2) & 0x7fff); }

	// NMDC stuff
	void myNick(const string& aNick) { send("$MyNick " + Text::fromUtf8(aNick, *encoding) + '|'); }
	void lock(const string& aLock, const string& aPk) { send ("$Lock " + aLock + " Pk=" + aPk + '|'); }
	void key(const string& aKey) { send("$Key " + aKey + '|'); }
	void direction(const string& aDirection, int aNumber) { send("$Direction " + aDirection + " " + Util::toString(aNumber) + '|'); }
	void get(const string& aFile, int64_t aResume) { send("$Get " + aFile + "$" + Util::toString(aResume + 1) + '|'); } 	// No acp - utf conversion here...
	void fileLength(const string& aLength) { send("$FileLength " + aLength + '|'); }
	void startSend() { send("$Send|"); }
	void sending(int64_t bytes) { send(bytes == -1 ? string("$Sending|") : "$Sending " + Util::toString(bytes) + "|"); }
	void error(const string& aError) { send("$Error " + aError + '|'); }
	void listLen(const string& aLength) { send("$ListLen " + aLength + '|'); }
	void maxedOut() { isSet(FLAG_NMDC) ? send("$MaxedOut|") : send(AdcCommand(AdcCommand::SEV_RECOVERABLE, AdcCommand::ERROR_SLOTS_FULL, "Slots full")); }
	void fileNotAvail(const std::string& msg = FILE_NOT_AVAILABLE) { isSet(FLAG_NMDC) ? send("$Error " + msg + "|") : send(AdcCommand(AdcCommand::SEV_RECOVERABLE, AdcCommand::ERROR_FILE_NOT_AVAILABLE, msg)); }
	void getListLen() { send("$GetListLen|"); }

	// ADC Stuff
	void sup(const StringList& features) { 
		AdcCommand c(AdcCommand::CMD_SUP);
		for(StringIterC i = features.begin(); i != features.end(); ++i)
			c.addParam(*i);
		send(c);
	}
	void inf(bool withToken);
	void get(const string& aType, const string& aName, const int64_t aStart, const int64_t aBytes) {  send(AdcCommand(AdcCommand::CMD_GET).addParam(aType).addParam(aName).addParam(Util::toString(aStart)).addParam(Util::toString(aBytes))); }
	void snd(const string& aType, const string& aName, const int64_t aStart, const int64_t aBytes) {  send(AdcCommand(AdcCommand::CMD_SND).addParam(aType).addParam(aName).addParam(Util::toString(aStart)).addParam(Util::toString(aBytes))); }

	void send(const AdcCommand& c) { send(c.toString(0, isSet(FLAG_NMDC))); }

	void supports(const StringList& feat) { 
		string x;
		for(StringList::const_iterator i = feat.begin(); i != feat.end(); ++i) {
			x+= *i + ' ';
		}
		send("$Supports " + x + '|');
	}
	void setDataMode(int64_t aBytes = -1) { dcassert(socket); socket->setDataMode(aBytes); }
	void setLineMode(size_t rollback) { dcassert(socket); socket->setLineMode(rollback); }

	void connect(const string& aServer, uint16_t aPort) throw(SocketException, ThreadException);
	void accept(const Socket& aServer) throw(SocketException, ThreadException);

	void disconnect(bool graceless = false) { if(socket) socket->disconnect(graceless); }
	void transmitFile(InputStream* f) { socket->transmitFile(f); }

	const string& getDirectionString() const {
		dcassert(isSet(FLAG_UPLOAD) ^ isSet(FLAG_DOWNLOAD));
		return isSet(FLAG_UPLOAD) ? UPLOAD : DOWNLOAD;
	}

	const UserPtr& getUser() const { return user; }
	UserPtr& getUser() { return user; }
	bool isSecure() const { return socket && socket->isSecure(); }
	bool isTrusted() const { return socket && socket->isTrusted(); }

	const string& getRemoteIp() const { if(socket) return socket->getIp(); else return Util::emptyString; }
	Download* getDownload() { dcassert(isSet(FLAG_DOWNLOAD)); return download; }
	uint16_t getPort() const { if(socket) return socket->getPort(); else return 0; }
	void setDownload(Download* d) { dcassert(isSet(FLAG_DOWNLOAD)); download = d; }
	Upload* getUpload() { dcassert(isSet(FLAG_UPLOAD)); return upload; }
	void setUpload(Upload* u) { dcassert(isSet(FLAG_UPLOAD)); upload = u; }

	void reconnect() {
		disconnect();
		Thread::sleep(100);
		ClientManager::getInstance()->connect(user, Util::toString(Util::rand()));
	}
	
	void handle(AdcCommand::SUP t, const AdcCommand& c) { fire(t, this, c); }
	void handle(AdcCommand::INF t, const AdcCommand& c) { fire(t, this, c); }
	void handle(AdcCommand::GET t, const AdcCommand& c) { fire(t, this, c); }
	void handle(AdcCommand::SND t, const AdcCommand& c) { fire(t, this, c);	}
	void handle(AdcCommand::STA t, const AdcCommand& c) { fire(t, this, c);	}
	void handle(AdcCommand::RES t, const AdcCommand& c) { fire(t, this, c); }
	void handle(AdcCommand::GFI t, const AdcCommand& c) { fire(t, this, c);	}

	// Ignore any other ADC commands for now
	template<typename T> void handle(T , const AdcCommand& ) { }

	GETSET(string, hubUrl, HubUrl);
	GETSET(string, token, Token);
	GETSET(uint64_t, lastActivity, LastActivity);
	GETSET(States, state, State);

	GETSET(string*, encoding, Encoding);
	
	BufferedSocket const* getSocket() { return socket; } 
	void garbageCommand() { 
		string tmp;
		tmp.reserve(20);
		for(int i = 0; i < 20; i++) {
			tmp.append(1, (char)Util::rand('a', 'z'));
		}
		send("$"+tmp+"|");
	}

private:
	BufferedSocket* socket;
	UserPtr user;

	static const string UPLOAD, DOWNLOAD;
	
	union {
		Download* download;
		Upload* upload;
	};

	// We only want ConnectionManager to create this...
	UserConnection(bool secure_) throw() : encoding(const_cast<string*>(&Text::systemCharset)), state(STATE_UNCONNECTED),
		lastActivity(0), socket(0), download(NULL) {
		if(secure_) {
			setFlag(FLAG_SECURE);
		}
	}

	~UserConnection() throw() {
		BufferedSocket::putSocket(socket);
	}
	friend struct DeleteFunction;

	UserConnection(const UserConnection&);
	UserConnection& operator=(const UserConnection&);

	void setUser(const UserPtr& aUser) {
		user = aUser;
	}

	void onLine(const string& aLine) throw();
	
	void send(const string& aString) {
		lastActivity = GET_TICK();
		COMMAND_DEBUG(aString, DebugManager::CLIENT_OUT, getRemoteIp());
		socket->write(aString);
	}

	void on(Connected) throw();
	void on(Line, const string&) throw();
	void on(Data, uint8_t* data, size_t len) throw();
	void on(BytesSent, size_t bytes, size_t actual) throw() ;
	void on(ModeChange) throw();
	void on(TransmitDone) throw();
	void on(Failed, const string&) throw();
};

#endif // !defined(USER_CONNECTION_H)

/**
 * @file
 * $Id$
 */
