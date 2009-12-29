/*
 * Copyright (C) 2001-2009 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_WIN32_UPNP_H
#define DCPLUSPLUS_WIN32_UPNP_H

class UPnP
{
public:
	UPnP() { }
	virtual ~UPnP() { }

	virtual bool init() = 0;

	enum Protocol {
		PROTOCOL_TCP,
		PROTOCOL_UDP,
		PROTOCOL_LAST
	};

	virtual bool open(const unsigned short port, const Protocol protocol, const string& description) = 0;
	bool close();

	virtual string getExternalIP() = 0;

protected:
	static const char* protocols[PROTOCOL_LAST];

	void open(const unsigned short port, const Protocol protocol);
	virtual bool close(const unsigned short port, const Protocol protocol) = 0;

private:
	typedef std::pair<unsigned short, Protocol> rule;
	std::vector<rule> rules;
};

#endif
