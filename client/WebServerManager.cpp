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

#include "stdinc.h"
#include "WebServerManager.h"
#include "QueueManager.h"
#include "FinishedManager.h"
#include "LogManager.h"
#include "ClientManager.h"
#include "UploadManager.h"


WebServerManager* Singleton<WebServerManager>::instance = NULL;

static const string WEBSERVER_AREA = "WebServer";

WebServerManager::WebServerManager(void) : started(false), page404(NULL) {
	SettingsManager::getInstance()->addListener(this);
	if(BOOLSETTING(WEBSERVER))Start();
}

WebServerManager::~WebServerManager(void){
	if(started) Stop();
}

void WebServerManager::Start(){
	if(started)return;
	Lock l(cs);
	started = true;

	socket.addListener(this);
	socket.waitForConnections(SETTING(WEBSERVER_PORT));
	fire(WebServerListener::Setup());

	page404 = new WebPageInfo(PAGE_404,"");
	pages["/"] = new WebPageInfo(INDEX, "");
	pages["/index.htm"] = new WebPageInfo(INDEX, "");	
	pages["/dlqueue.html"] = new WebPageInfo(DOWNLOAD_QUEUE, "Download Queue");
	pages["/dlfinished.html"] = new WebPageInfo(DOWNLOAD_FINISHED, "Finished Downloads");
	pages["/ulqueue.html"] = new WebPageInfo(UPLOAD_QUEUE, "Upload Queue");
	pages["/ulfinished.html"] = new WebPageInfo(UPLOAD_FINISHED, "Finished Uploads");
	pages["/weblog.html"] = new WebPageInfo(LOG, "Logs");
	pages["/syslog.html"] = new WebPageInfo(SYSLOG, "System Logs");


#ifdef _DEBUG  
	//AllocConsole();
	//freopen("con:","w",stdout);
#endif
}

void WebServerManager::Stop(){
	if(!started)return;
	started = false;
	Lock l(cs);

	delete page404;
	page404 = NULL;

	for(WebPages::iterator p = pages.begin(); p != pages.end(); ++p){
			if(p->second != NULL){
				delete p->second;
				p->second = NULL;

			}
	}

#ifdef _DEBUG 
	//FreeConsole();
#endif
	socket.removeListener(this);
	socket.disconnect();
}

void WebServerManager::on(ServerSocketListener::IncomingConnection) throw() {
	WebServerSocket *wss = new WebServerSocket();
	wss->accept(&socket);	
	wss->start();
}

string WebServerManager::getLoginPage(){
	string pagehtml = "";
    pagehtml = "<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.1//EN' 'http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd'>";
	pagehtml += "<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='cs'>";
	pagehtml += "<head>";
	pagehtml += "    	<title>Strongdc++ webserver - Login Page</title>";   
	pagehtml += "	<meta http-equiv='Content-Type' content='text/html; charset=windows-1250' />";
    pagehtml += "<meta http-equiv='pragma' content='no-cache'>";
    pagehtml += "   	<meta http-equiv='cache-control' content='no-cache, must-revalidate'>";
	pagehtml += "	<link rel='stylesheet' href='http://snail.pc.cz/StrongDC/webserver/strong.css' type='text/css' title='Default styl' media='screen' />";
    pagehtml += "</head>";
    pagehtml += "<body>";
    pagehtml += "<div id='index_obsah'>";
    pagehtml += "<h1>StrongDC++ Webserver</h1>";
    pagehtml += "<div id='index_logo'></div>";
    pagehtml += "	<div id='login'>";
    pagehtml += "		<form method='get' action='index.htm'>";
    pagehtml += "			<p><strong>Username: </strong><input type='text' name='user'  size='10'/></p>";
    pagehtml += "			<p><strong>Password: </strong><input type='password' name='pass' size='10'></p>";
    pagehtml += "			<p><input class='tlacitko' type='submit' value='Login'></p>";
    pagehtml += "		</form>";
    pagehtml += "	</div>";
    pagehtml += "	<div id='paticka'>";
    pagehtml += "		2004 | Big Muscle | <a href='http://snail.pc.cz/StrongDC'>StrongDC++</a>";
    pagehtml += "	</div>";
    pagehtml += "</div>";                                
    pagehtml += "</body>";
    pagehtml += "</html>";

	string header = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + Util::toString(pagehtml.length()) + "\r\n\r\n";

	return header + pagehtml;
}

string WebServerManager::getPage(string file){
	printf("requested: '%s'\n",file.c_str()); 
	string header = "HTTP/1.0 200 OK\r\n";
	string pagehtml = "";

	WebPageInfo *page = page404;
	WebPages::iterator find = pages.find(file.c_str());
	if(find != pages.end()) page = find->second;	
    pagehtml = "<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.1//EN' 'http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd'>";
    pagehtml += "<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='cs'>";
    pagehtml += "<head>";
    pagehtml += "    <title>Strongdc++ webserver - System Log</title>";
	
    pagehtml += "	<meta http-equiv='Content-Type' content='text/html; charset=windows-1250' />";
    pagehtml += "    <meta http-equiv='pragma' content='no-cache'>";
    pagehtml += "    <meta http-equiv='cache-control' content='no-cache, must-revalidate'>";
	
    pagehtml += "	<link rel='stylesheet' href='http://snail.pc.cz/StrongDC/webserver/strong.css' type='text/css' title='Default styl' media='screen' />";
    pagehtml += "</head>";
    pagehtml += "<body>";

    pagehtml += "<div id='obsah'>";
    pagehtml += "<h1>StrongDC++ Webserver</h1>";
    pagehtml += "	<div id='menu'>";
    pagehtml += "		<a href='weblog.html'>Webserver Log</a>";
    pagehtml += "		<a href='syslog.html'>System Log</a>";
    pagehtml += "		<a href='ulfinished.html'>Finished Uploads</a>";
    pagehtml += "		<a href='ulqueue.html'>Upload Queue  </a>";
    pagehtml += "		<a href='dlqueue.html'>Download Queue</a>";
    pagehtml += "		<a href='dlfinished.html'>Finished Downloads </a>";
    pagehtml += "	</div>";

	pagehtml += "	<div id='prava'>";
  
	switch(page->id){
		case INDEX:
			pagehtml+="Welcome to the StrongDC++ WebServer";
			break;

		case DOWNLOAD_QUEUE:
			pagehtml+=getDLQueue();
			break;

		case DOWNLOAD_FINISHED:
			pagehtml+=getFinished(false);
			break;

		case UPLOAD_QUEUE:
			pagehtml+=getULQueue();
			break;

		case UPLOAD_FINISHED:
			pagehtml+=getFinished(true);
			break;

		case LOG:
			pagehtml+=getLogs();
			break;

		case SYSLOG:
			pagehtml+=getSysLogs();
			break;

		case PAGE_404:
		default:
			int action = 0;
			if(file == "/shutdown.htm") action = 0;
			else if(file == "/reboot.htm") action = 2;
			else if(file == "/suspend.htm") action = 3;
			else if(file == "/logoff.htm") action = 1;
			else if(file == "/switch.htm") action = 5;
			else { 
				header = "HTTP/1.0 404 Not Found\r\n";
				pagehtml += "Page not found";
				break;
			}

			fire(WebServerListener::ShutdownPC(), action);
			pagehtml += "Request sent to remote PC :)";

	}

    pagehtml += "	</div>";
    pagehtml += "	<div id='ikony'>";
    pagehtml += "			<a href='switch.htm' id='switch'></a>";
    pagehtml += "			<a href='logoff.htm' id='logoff'></a>";
    pagehtml += "			<a href='suspend.htm' id='suspend'></a>";
    pagehtml += "			<a href='reboot.htm' id='reboot'></a>";
    pagehtml += "			<a href='shutdown.htm' id='shutdown'></a>";
    pagehtml += "	</div>";
    pagehtml += "	<div id='paticka'>";
    pagehtml += "		2004 | Big Muscle | StrongDC++";
    pagehtml += "	</div>";
    pagehtml += "</div>";
                                  
    pagehtml += "</body>";
    pagehtml += "</html>";

	header += "Content-Type: text/html\r\nContent-Length: " + Util::toString(pagehtml.length()) + "\r\n\r\n";

	printf("sending: %s\n",(header + pagehtml).c_str());
	return header + pagehtml;
}

string WebServerManager::getLogs(){
	string ret = "";
	ret = "	<h1>Webserver Logs</h1>";
	ret += LOGTAIL(WEBSERVER_AREA, 10);

	string::size_type i = 0;
	while( (i = ret.find('\n', i)) != string::npos) {
		ret.replace(i, 1, "<br>");
		i++;
	}

	return ret;
}

string WebServerManager::getSysLogs(){
	string ret = "";
	ret += "<h1>System Logs</h1><br>";
	ret += LOGTAIL("system", 50);

	string::size_type i = 0;
	while( (i = ret.find('\n', i)) != string::npos) {
		ret.replace(i, 1, "<br>");
		i++;
	}

	return ret;
}

string WebServerManager::getFinished(bool uploads){
	string ret;

	const FinishedItem::List& fl = FinishedManager::getInstance()->lockList(uploads);
	ret = "	<h1>Finished ";
	ret += (uploads ? "Uploads" : "Downloads");
	ret += "</h1>";
	ret += "	<table  width='100%'>";
	ret += "		<tr class='tucne'>";
	ret += "			<td>Time</td>";
	ret += "			<td>Name</td>";
	ret += "			<td>Size</td>";
	ret += "		</tr>";
	for(FinishedItem::List::const_iterator i = fl.begin(); i != fl.end(); ++i) {
		ret+="<tr>";
		ret+="	<td>" + Util::formatTime("%Y-%m-%d %H:%M:%S", (*i)->getTime()) + "</td>";
		ret+="	<td>" + Util::getFileName((*i)->getTarget()) + "</td>";
		ret+="	<td>" + Util::formatBytes((*i)->getSize()) + "</td>";			
		ret+="</tr>";
	}
	ret+="</table>";
	FinishedManager::getInstance()->unlockList();

	return ret;
}

string WebServerManager::getDLQueue(){
	string ret;

	const QueueItem::StringMap& li = QueueManager::getInstance()->lockQueue();
	ret = "	<h1>Download Queue</h1>";
	ret += "	<table  width='100%'>";
	ret += "		<tr class='tucne'>";
	ret += "			<td>Name</td>";
	ret += "			<td>Size</td>";
	ret += "			<td>Downloaded</td>";
	ret += "			<td>Speed</td>";
	ret += "			<td>Segments</td>";
	ret += "		</tr>";
	for(QueueItem::StringMap::const_iterator j = li.begin(); j != li.end(); ++j) {
		QueueItem* aQI = j->second;
		double percent = (aQI->getSize()>0)? aQI->getDownloadedBytes() * 100.0 / aQI->getSize() : 0;
		ret += "	<tr>";
		ret += "		<td>" + aQI->getTargetFileName() + "</td>";
		ret += "		<td>" + Util::formatBytes(aQI->getSize()) + "</td>";
		ret += "		<td>" + Util::formatBytes(aQI->getDownloadedBytes()) + " ("+ Util::toString(percent) + "%)</td>";
		ret += "		<td>" + Util::formatBytes(aQI->getSpeed()) + "/s</td>";
		ret += "		<td>" + Util::toString((int)aQI->getActiveSegments().size())+"/"+Util::toString(aQI->getMaxSegments()) + "</td>";
		ret += "	</tr>";
	}
	ret+="</table>";
	QueueManager::getInstance()->unlockQueue();

	return ret;
}

string WebServerManager::getULQueue(){
	string ret = "";
	ret = "	<h1>Upload Queue</h1>";
	ret += "	<table  width='100%'>";
	ret += "		<tr class='tucne'>";
	ret += "			<td>User</td>";
	ret += "			<td>Filename</td>";
	ret += "		</tr>";
	for(UploadQueueItem::UserMapIter ii = UploadManager::getInstance()->UploadQueueItems.begin(); ii != UploadManager::getInstance()->UploadQueueItems.end(); ++ii) {
		for(UploadQueueItem::Iter i = ii->second.begin(); i != ii->second.end(); ++i) {
			ret+="<tr><td>" + (*i)->User->getFullNick() + "</td>";
			ret+="<td>" + (*i)->FileName + "</td></tr>";
		}
	}
	ret+="</table>";
	return ret;
}	

StringMap WebServerSocket::getArgs(string arguments) {
	StringMap args;

	string::size_type i = 0;
	string::size_type j = 0;
	while((i = arguments.find("=", j)) != string::npos) {
		string key = arguments.substr(j, i-j);
		string value = arguments.substr(i + 1);;

		j = i + 1;
		if((i = arguments.find("&", j)) != string::npos) {
			value = arguments.substr(j, i-j);
			j = i + 1;
		} 

		args[key] = value;
	}

	return args;
}

int WebServerSocket::run(){
	char buff[512];

	ZeroMemory(&buff, sizeof(buff));
	while(true) {

	int size = recv(sock,buff,sizeof(buff),0);

	string header = buff;
	header = header.substr(0,size);

	int start = 0, end = 0;

	string IP = Util::toString(from.sin_addr.S_un.S_un_b.s_b1) + string(".") + Util::toString(from.sin_addr.S_un.S_un_b.s_b2) + string(".") + Util::toString(from.sin_addr.S_un.S_un_b.s_b3) + string(".") + Util::toString(from.sin_addr.S_un.S_un_b.s_b4);

    printf("%s\n", header.c_str());	

	if(((start = header.find("GET ")) != string::npos) && (end = header.substr(start+4).find(" ")) != string::npos ){
		if(BOOLSETTING(LOG_WEBSERVER)) {
			StringMap params;
			params["file"] = header.substr(start+4,end);
			params["ip"] = IP;
			LOG(WEBSERVER_AREA,Util::formatParams(SETTING(WEBSERVER_FORMAT), params));
		}
		header = header.substr(start+4,end);

		if((start = header.find("?")) != string::npos) {
			string arguments = header.substr(start+1);
			header = header.substr(0, start);
			StringMap m = getArgs(arguments);

			if(m["user"] == SETTING(WEBSERVER_USER) && m["pass"] == SETTING(WEBSERVER_PASS))
				WebServerManager::getInstance()->login(IP);
		}

		string toSend;
		
		if(!WebServerManager::getInstance()->isloggedin(IP)) {
			toSend = WebServerManager::getInstance()->getLoginPage();
		} else {
			toSend = WebServerManager::getInstance()->getPage(header);
		}
	
		::send(sock, toSend.c_str(), toSend.size(), 0);
		break;
	}/* else {
		if(BOOLSETTING(LOG_WEBSERVER)) {
			StringMap params;
			params["file"] = "Unknown request type";
			params["ip"] = IP;
			LOG(WEBSERVER_AREA,Util::formatParams(SETTING(WEBSERVER_FORMAT), params));
		}
	}*/
	}
	::closesocket(sock);
	return 0;
}