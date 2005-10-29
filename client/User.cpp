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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "User.h"
#include "Client.h"
#include "ClientManager.h"
#include "DebugManager.h"
#include "ClientProfileManager.h"
#include "QueueManager.h"

#include "../pme-1.0.4/pme.h"

OnlineUser::OnlineUser(const User::Ptr& ptr, Client& client_) : user(ptr), identity(ptr, client_.getHubUrl()), client(&client_) { 
	unCacheClientInfo(); 
	user->setOnlineUser(this);
}

OnlineUser::~OnlineUser() throw() {
	user->setLastSavedHubName(getClient().getHubName());
	user->setLastSavedHubAddress(getClient().getIpPort());
	user->setOnlineUser(NULL);
};

void Identity::getParams(StringMap& map, const string& prefix) const {
	for(InfMap::const_iterator i = info.begin(); i != info.end(); ++i) {
		map[prefix + string((char*)(&i->first), 2)] = i->second;
	}
	map[prefix + "CID"] = user->getCID().toBase32();

	// for compatibility with old raw commands
	map["nick"] = getNick();
	map["tag"] = getTag();
	map["ip"] = getIp();
	OnlineUser* ou = getUser()->getOnlineUser();
	if(ou) {
		map["mynick"] =  ou->getClient().getMyNick();
		map["hub"] = ou->getClient().getHubName();
		map["hubip"] = ou->getClient().getHubUrl();
		map["hubaddr"] = ou->getClient().getAddress();
		map["realshare"] = Util::toString(ou->getRealBytesShared());
		map["realshareformat"] = Util::formatBytes(ou->getRealBytesShared());
		map["statedshare"] = Util::toString(getBytesShared());
		map["statedshareformat"] = Util::formatBytes(getBytesShared());
		map["cheatingdescription"] = ou->getCheatingString();
		map["clienttype"] = ou->getClientType();
	}
}

void OnlineUser::setCheat(const string& aCheatDescription, bool aBadClient, bool postToChat) {
	if(getIdentity().isOp() || !getClient().isOp()) return;

	if ((!SETTING(FAKERFILE).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
		PlaySound(Text::toT(SETTING(FAKERFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
		
	StringMap ucParams;
	getIdentity().getParams(ucParams, Util::emptyString);
	string cheat = Util::formatParams(aCheatDescription, ucParams);
	if(postToChat)
		user->addCheatLine("*** " + STRING(USER) + " " + getIdentity().getNick() + " - " + cheat);
	
	cheatingString = cheat;
	badClient = aBadClient;
}

void User::sendRawCommand(const int aRawCommand) {
	if(getClient()) {
		if(!getClient()->isOp()) return;
		string rawCommand = getClient()->getRawCommand(aRawCommand);
		if (!rawCommand.empty()) {
			StringMap ucParams;
			getOnlineUser()->getIdentity().getParams(ucParams, Util::emptyString);
			getClient()->sendRaw(Util::formatParams(rawCommand, ucParams));
		}
	}
}

void User::addCheatLine(const string& aLine) {
	if(getOnlineUser()) {
		getOnlineUser()->getClient().cheatMessage(aLine);
	}
}

const string& User::getClientName() const {
	if(getOnlineUser()) {
		return getOnlineUser()->getClient().getHubName();
	} else {
		return STRING(OFFLINE);
	}
}

Client* User::getClient() const {
	if(getOnlineUser()) {
		return &getOnlineUser()->getClient();
	} else {
		return NULL;
	}
}

string OnlineUser::getReport()
{
	string report = "\r\nClient:		" + getClientType();
	report += "\r\nXML Generator:	" + (generator.empty() ? "N/A" : generator);
	report += "\r\nLock:		" + lock;
	report += "\r\nPk:		" + pk;
	report += "\r\nTag:		" + getIdentity().getTag();
	report += "\r\nSupports:		" + supports;
	report += "\r\nStatus:		" + Util::formatStatus(status);
	report += "\r\nTestSUR:		" + testSUR;
	report += "\r\nDisconnects:	" + Util::toString(fileListDisconnects);
	report += "\r\nTimeouts:		" + Util::toString(connectionTimeouts);
	report += "\r\nDownspeed:	" + Util::formatBytes(Util::toInt64(getIdentity().get("US"))) + "/s";
	report += "\r\nIP:		" + getIdentity().getIp();
	report += "\r\nHost:		" + Socket::getRemoteHost(getIdentity().getIp());
	report += "\r\nDescription:	" + getIdentity().getDescription();
	report += "\r\nEmail:		" + getIdentity().getEmail();
	report += "\r\nConnection:	" + getIdentity().getConnection();
	report += "\r\nCommands:	" + unknownCommand;
	report += "\r\nFilelist size:	" + (fileListSize != -1 ? Util::formatBytes(fileListSize) + "  (" + Util::formatExactSize(fileListSize) + " )" : "N/A");
	report += "\r\nListLen:		" + (listLength != -1 ? Util::formatBytes(listLength) + "  (" + Util::formatExactSize(listLength) + " )" : "N/A");
	report += "\r\nStated Share:	" + Util::formatBytes(getIdentity().getBytesShared()) + "  (" + Util::formatExactSize(getIdentity().getBytesShared()) + " )";
	report += "\r\nReal Share:	" + (realBytesShared > -1 ? Util::formatBytes(realBytesShared) + "  (" + Util::formatExactSize(realBytesShared) + " )" : "N/A");
	report += "\r\nCheat status:	" + (cheatingString.empty() ? "N/A" : cheatingString);
	report += "\r\nComment:		" + comment;
	return report;
}

void OnlineUser::updateClientType() {
	if ( getUser()->isSet(User::DCPLUSPLUS) && (listLength == 11) && (getIdentity().getBytesShared() > 0) ) {
		setCheat("Fake file list - ListLen = 11" , true);
		clientType = "DC++ Stealth";
		badClient = true;
		badFilelist = true;
		getUser()->sendRawCommand(SETTING(LISTLEN_MISMATCH));
		return;
	}
	int64_t tick = GET_TICK();

	StringMap params;
	ClientProfile::List& lst = ClientProfileManager::getInstance()->getClientProfiles(params);

	for(ClientProfile::Iter i = lst.begin(); i != lst.end(); ++i) {
		ClientProfile& cp = *i;	
		string version, pkVersion, extraVersion, formattedTagExp, verTagExp;

		verTagExp = Util::formatRegExp(cp.getTag(), params);

		formattedTagExp = verTagExp;
		string::size_type j = formattedTagExp.find("%[version]");
		if(j != string::npos) {
			formattedTagExp.replace(j, 10, ".*");
		}

		string pkExp = cp.getPk();
		string formattedPkExp = pkExp;
		j = pkExp.find("%[version]");
		if(j != string::npos) {
			formattedPkExp.replace(j, 10, ".*");
		}
		string extTagExp = cp.getExtendedTag();
		string formattedExtTagExp = extTagExp;
		j = extTagExp.find("%[version2]");
		if(j != string::npos) {
			formattedExtTagExp.replace(j, 11, ".*");
		}

		DETECTION_DEBUG("\tChecking profile: " + cp.getName());

		if (!matchProfile(lock, cp.getLock())) { continue; }
		if (!matchProfile(getIdentity().getTag(), formattedTagExp)) { continue; } 
		if (!matchProfile(pk, formattedPkExp)) { continue; }
		if (!matchProfile(supports, cp.getSupports())) { continue; }
		if (!matchProfile(testSUR, cp.getTestSUR())) { continue; }
		if (!matchProfile(Util::toString(status), cp.getStatus())) { continue; }
		if (!matchProfile(unknownCommand, cp.getUserConCom())) { continue; }
		if (!matchProfile(getIdentity().getDescription(), formattedExtTagExp))	{ continue; }
		if (!matchProfile(getIdentity().getConnection(), cp.getConnection()))	{ continue; }

		if (verTagExp.find("%[version]") != string::npos) { version = getVersion(verTagExp, getIdentity().getTag()); }
		if (extTagExp.find("%[version2]") != string::npos) { extraVersion = getVersion(extTagExp, getIdentity().getDescription()); }
		if (pkExp.find("%[version]") != string::npos) { pkVersion = getVersion(pkExp, getPk()); }

		if (!(cp.getVersion().empty()) && !matchProfile(version, cp.getVersion())) { continue; }

		DETECTION_DEBUG("Client found: " + cp.getName() + " time taken: " + Util::toString(GET_TICK()-tick) + " milliseconds");
		if (cp.getUseExtraVersion()) {
			setClientType(cp.getName() + " " + extraVersion );
		} else {
			setClientType(cp.getName() + " " + version);
		}
			cheatingString = cp.getCheatingDescription();
			comment = cp.getComment();
		badClient = !cheatingString.empty();

		if (cp.getCheckMismatch() && version.compare(pkVersion) != 0) { 
			clientType += " Version mis-match";
			cheatingString += " Version mis-match";
			badClient = true;
			ClientManager::getInstance()->UserUpdated(getUser());
			setCheat(cheatingString, true);
			return;
		}
		if(badClient) setCheat(cheatingString, true);
		ClientManager::getInstance()->UserUpdated(getUser());
		if(cp.getRawToSend() > 0) {
			getUser()->sendRawCommand(cp.getRawToSend());
		}
		return;
	}
	setClientType("Unknown");
	cheatingString = Util::emptyString;
	badClient = false;
	ClientManager::getInstance()->UserUpdated(getUser());
}

bool OnlineUser::matchProfile(const string& aString, const string& aProfile) {
	DETECTION_DEBUG("\t\tMatching String: " + aString + " to Profile: " + aProfile);
	PME reg(aProfile);
	return reg.IsValid() ? (reg.match(aString) > 0) : false;
}

string OnlineUser::getVersion(const string& aExp, const string& aTag) {
	string::size_type i = aExp.find("%[version]");
	if (i == string::npos) { 
		i = aExp.find("%[version2]"); 
		return splitVersion(aExp.substr(i + 11), splitVersion(aExp.substr(0, i), aTag, 1), 0);
	}
	return splitVersion(aExp.substr(i + 10), splitVersion(aExp.substr(0, i), aTag, 1), 0);
}

string OnlineUser::splitVersion(const string& aExp, const string& aTag, const int part) {
	PME reg(aExp);
	if(!reg.IsValid()) { return ""; }
	reg.split(aTag, 2);
	return reg[part];
}

bool OnlineUser::fileListDisconnected() {
	fileListDisconnects++;

	if(SETTING(ACCEPTED_DISCONNECTS) == 0)
		return false;

	if(fileListDisconnects == SETTING(ACCEPTED_DISCONNECTS)) {
		setCheat("Disconnected file list " + Util::toString(fileListDisconnects) + " times", false);
		ClientManager::getInstance()->UserUpdated(getUser());
		getUser()->sendRawCommand(SETTING(DISCONNECT_RAW));
		return true;
	}
	return false;
}

bool OnlineUser::connectionTimeout() {
	connectionTimeouts++;

	if(SETTING(ACCEPTED_TIMEOUTS) == 0)
		return false;

	if(connectionTimeouts == SETTING(ACCEPTED_TIMEOUTS)) {
		setCheat("Connection timeout " + Util::toString(connectionTimeouts) + " times", false);
		ClientManager::getInstance()->UserUpdated(getUser());
		try {
			QueueManager::getInstance()->removeTestSUR(getUser());
		} catch(...) {
		}
		getUser()->sendRawCommand(SETTING(TIMEOUT_RAW));
		return true;
	}
	return false;
}

const string& User::getLastHubName() const {
	if(getOnlineUser()) {
		return getOnlineUser()->getClient().getHubName();
	} else {
		return lastSavedHubName;
	}
}

string User::getLastHubAddress() const {
	if(getOnlineUser()) {
		return getOnlineUser()->getClient().getIpPort();
	} else {
		return lastSavedHubAddress;
	}
}

/**
 * @file
 * $Id$
 */
