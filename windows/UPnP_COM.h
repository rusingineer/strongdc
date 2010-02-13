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

#ifndef DCPLUSPLUS_WIN32_UPNP_COM_H
#define DCPLUSPLUS_WIN32_UPNP_COM_H

#include "UPnP.h"

// for mingw64
#ifndef interface
#define interface struct
#endif

#include <natupnp.h>

class UPnP_COM : public UPnP
{
public:
	UPnP_COM() : UPnP(), pUN(0), lastPort(0) { }

	bool init();

	bool open(const unsigned short port, const Protocol protocol, const string& description);
	bool close(const unsigned short port, const Protocol protocol);

	string getExternalIP();

private:
	IUPnPNAT* pUN;
	// this one can become invalidated so we can't cache it
	IStaticPortMappingCollection* getStaticPortMappingCollection();

	// need to save these to get the external IP...
	unsigned short lastPort;
	Protocol lastProtocol;
};

#endif
