/* 
 * Copyright (C) 2003 Twink, spm7@waikato.ac.nz
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

#pragma once
#include "DCPlusPlus.h"
#include "ServerSocket.h"
#include "SettingsManager.h"
#include "Singleton.h"
#include "Thread.h"
#include "Speaker.h"
#include "CriticalSection.h"

#include <hash_set>


class WebServerListener{
public:
	template<int I>	struct X { enum { TYPE = I };  };

	typedef X<0> Setup;
	typedef X<1> ShutdownPC;

	virtual void on(Setup) throw() = 0;
	virtual void on(ShutdownPC, int action) throw() = 0;

};

class WebServerManager :  public Singleton<WebServerManager>, public ServerSocketListener, public Speaker<WebServerListener>, private SettingsManagerListener
{
public:
	ServerSocket& getServerSocket() {
		return socket;
	}

	string getPage(string file);
	string getLoginPage();


	// SettingsManagerListener
	virtual void on(SettingsManagerListener::Save, SimpleXML* xml) throw() {
		if(BOOLSETTING(WEBSERVER)){
			Restart();
		} else {
			Stop();
		}
	}

	void Restart(){		
		Stop();
		Start();
	}

private:
	friend Singleton<WebServerManager>;
	WebServerManager(void);
	~WebServerManager(void);

	void Start();
	void Stop(); 


	enum PageID{
		INDEX,
		DOWNLOAD_QUEUE,
		DOWNLOAD_FINISHED,
		UPLOAD_QUEUE,
		UPLOAD_FINISHED,
		LOG,
		SYSLOG,
		PAGE_404
	};

	struct WebPageInfo {
		WebPageInfo(PageID n,string t) : id(n), title(t) {}
		PageID id;
		string title;
	};

	WebPageInfo *page404;

	struct eqstr{
		bool operator()(const char* s1, const char* s2) const {  
			return stricmp(s1, s2) == 0;
		}
	};

	typedef hash_map <const char *, WebPageInfo*, hash<const char *>, eqstr> WebPages;
	WebPages pages;

	string getDLQueue();
	string getULQueue();
	string getFinished(bool);
	string getLogs();
	string getSysLogs();

	bool started;
	CriticalSection cs;
	// ServerSocketListener
	virtual void on(ServerSocketListener::IncomingConnection) throw();

	ServerSocket socket;
	HWND m_hWnd;


	map<string,time_t> loggedin;

public:
	void login(string ip){
		loggedin[ip] = time(NULL);
	}

	bool isloggedin(string ip) {
		map<string,time_t>::iterator i;
		if((i = loggedin.find(ip)) != loggedin.end()) {
            time_t elapsed = time(NULL) - loggedin[ip];
			if(elapsed > 300) {
				loggedin.erase(i);
				return false;
			}

			return true;
		}

		return false;
	}

};

class WebServerSocket : public Thread {
public:

	void accept(ServerSocket *s){
		int fromlen=sizeof(from);

		printf("accepting\n");
		sock = ::accept(s->getSocket(), (struct sockaddr*)&from,&fromlen);
		u_long b = 1;
		ioctlsocket(sock, FIONBIO, &b);		
	}

	StringMap getArgs(string arguments); 
	virtual int run();

private:
	sockaddr_in from;
	SOCKET sock;
	HANDLE thread;
};
