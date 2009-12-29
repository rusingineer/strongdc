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

#include "stdafx.h"

#include "UPnP.h"

const char* UPnP::protocols[PROTOCOL_LAST] = {
	"TCP",
	"UDP"
};

void UPnP::open(const unsigned short port, const Protocol protocol) {
	rules.push_back(make_pair(port, protocol));
}

bool UPnP::close() {
	bool ret = true;

	for(std::vector<rule>::const_iterator i = rules.begin(), iend = rules.end(); i != iend; ++i)
		ret &= close(i->first, i->second);
	rules.clear();

	return ret;
}
