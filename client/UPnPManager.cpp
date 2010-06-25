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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "UPnPManager.h"

#include "ConnectionManager.h"
#include "SearchManager.h"
#include "LogManager.h"
#include "version.h"

#include "../dht/dht.h"

namespace dcpp {

void UPnPManager::addImplementation(UPnP* impl) {
	impls.push_back(impl);
}

void UPnPManager::open() {
	if(opened)
		return;

	if(impls.empty()) {
		log(STRING(UPNP_NO_IMPLEMENTATION));
		return;
	}

	start();
}

void UPnPManager::close() {
	for(Impls::iterator i = impls.begin(); i != impls.end(); ++i)
		close(*i);
	opened = false;
}

int UPnPManager::run() {
	// cache these
	const unsigned short
		conn_port = ConnectionManager::getInstance()->getPort(),
		secure_port = ConnectionManager::getInstance()->getSecurePort(),
		search_port = SearchManager::getInstance()->getPort(),
		dht_port = dht::DHT::getInstance()->getPort();

	for(Impls::iterator i = impls.begin(); i != impls.end(); ++i) {
		UPnP& impl = *i;

		close(impl);

		if(!impl.init())
			continue;

		if(conn_port != 0 && !impl.open(conn_port, UPnP::PROTOCOL_TCP, APPNAME " Transfer Port (" + Util::toString(conn_port) + " TCP)"))
			continue;

		if(secure_port != 0 && !impl.open(secure_port, UPnP::PROTOCOL_TCP, APPNAME " Encrypted Transfer Port (" + Util::toString(secure_port) + " TCP)"))
			continue;

		if(search_port != 0 && !impl.open(search_port, UPnP::PROTOCOL_UDP, APPNAME " Search Port (" + Util::toString(search_port) + " UDP)"))
			continue;

		if(dht_port != 0 && !impl.open(dht_port, UPnP::PROTOCOL_UDP, APPNAME " DHT Port (" + Util::toString(dht_port) + " UDP)"))
			continue;

		opened = true;
		log(STRING(UPNP_SUCCESSFULLY_CREATED_MAPPINGS));

		if(!BOOLSETTING(NO_IP_OVERRIDE)) {
			// now lets configure the external IP (connect to me) address
			string ExternalIP = impl.getExternalIP();
			if(!ExternalIP.empty()) {
				// woohoo, we got the external IP from the UPnP framework
				SettingsManager::getInstance()->set(SettingsManager::EXTERNAL_IP, ExternalIP);
			} else {
				//:-( Looks like we have to rely on the user setting the external IP manually
				// no need to do cleanup here because the mappings work
				log(STRING(UPNP_FAILED_TO_GET_EXTERNAL_IP));
			}
		}

		break;
	}

	if(!opened) {
		log(STRING(UPNP_FAILED_TO_CREATE_MAPPINGS));
	}

	return 0;
}

void UPnPManager::close(UPnP& impl) {
	if(!impl.close()) {
		log(STRING(UPNP_FAILED_TO_REMOVE_MAPPINGS));
	}
}

void UPnPManager::log(const string& message) {
	LogManager::getInstance()->message("UPnP: " + message);
}

} // namespace dcpp
