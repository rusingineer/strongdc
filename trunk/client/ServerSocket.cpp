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

#include "ServerSocket.h"

#define MAX_CONNECTIONS 20

void ServerSocket::waitForConnections(short aPort) throw(SocketException) {
	disconnect();
	
	sockaddr_in tcpaddr;
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sock == -1) {
		throw SocketException(errno);
	}

	// Set reuse address option...
	int x = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&x, sizeof(x));

	tcpaddr.sin_family = AF_INET;
	tcpaddr.sin_port = htons(aPort);
#ifndef _DEBUG
	tcpaddr.sin_addr.s_addr = htonl(INADDR_ANY);
#else
	char sBuf[32];
	string iniFile = Util::getAppPath() + "debug.ini";
	::GetPrivateProfileString("net", "IP", "0", sBuf, sizeof(sBuf), iniFile.c_str());
	unsigned long addr = inet_addr(sBuf);
	if(addr != INADDR_NONE)
		tcpaddr.sin_addr.s_addr = addr;
	else
		tcpaddr.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
	
	if(bind(sock, (sockaddr *)&tcpaddr, sizeof(tcpaddr)) == SOCKET_ERROR) {
		throw SocketException(errno);
	}
	if(listen(sock, MAX_CONNECTIONS) == SOCKET_ERROR) {
		throw SocketException(errno);
	}
}

/**
 * @file
 * $Id$
 */

