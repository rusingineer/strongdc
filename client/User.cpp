/* 
 * Copyright (C) 2001-2004 Jacek Sieka, j_s at telia com
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

#include "SettingsManager.h"
#include "ResourceManager.h"
#include "TimerManager.h"

#include "User.h"

#include "Client.h"
#include "FavoriteUser.h"

#include "DebugManager.h"
#include "ClientProfileManager.h"
#include "QueueManager.h"
#include "../pme-1.0.4/pme.h"
User::~User() throw() {
	delete favoriteUser;
}

void User::connect() {
	RLock<> l(cs);
	if(client) {
		client->connect(this);
	}
}

const string& User::getClientNick() const {
	RLock<> l(cs);
	if(client) {
		return client->getNick();
	} else {
		return SETTING(NICK);
	}
}

CID User::getClientCID() const {
	RLock<> l(cs);
	if(client) {
		return client->getMe()->getCID();
	} else {
		return CID(SETTING(CLIENT_ID));
	}
}

void User::updated(User::Ptr& aUser) {
	RLock<> l(aUser->cs);
	if(aUser->client) {
		aUser->client->updated(aUser);
	}
}

const string& User::getClientName() const {
	RLock<> l(cs);
	if(client) {
		return client->getName();
	} else if(!getLastHubName().empty()) {
		return getLastHubName();
	} else {
		return STRING(OFFLINE);
	}
}

string User::getClientAddressPort() const {
	RLock<> l(cs);
	if(client) {
		return client->getAddressPort();
	} else {
		return Util::emptyString;
	}
}

void User::privateMessage(const string& aMsg) {
	RLock<> l(cs);
	if(client) {
		client->privateMessage(this, aMsg);
	}
}

bool User::isClientOp() const {
	RLock<> l(cs);
	if(client) {
		return client->getOp();
	}
	return false;
}

void User::send(const string& aMsg) {
	RLock<> l(cs);
	if(client) {
		client->send(aMsg);
	}
}

void User::sendUserCmd(const string& aUserCmd) {
	RLock<> l(cs);
	if(client) {
		client->sendUserCmd(aUserCmd);
	}
}

void User::clientMessage(const string& aMsg) {
	RLock<> l(cs);
	if(client) {
		client->hubMessage(aMsg);
	}
}

void User::setClient(Client* aClient) { 
	WLock<> l(cs); 
	client = aClient; 
	if(client == NULL) {
		if (isSet(ONLINE) && isFavoriteUser())
			setFavoriteLastSeen();
		unsetFlag(ONLINE);
	} else {
		setLastHubAddress(aClient->getIpPort());
		setLastHubName(aClient->getName());
		setFlag(ONLINE);
		unsetFlag(QUIT_HUB);
	}
};

void User::getParams(StringMap& ucParams, bool myNick /* = false */) {
	ucParams["nick"] = getNick();
	ucParams["cid"] = getCID().toBase32();
	ucParams["tag"] = getTag();
	ucParams["description"] = getDescription();
	ucParams["email"] = getEmail();
	ucParams["share"] = Util::toString(getBytesShared());
	ucParams["shareshort"] = Util::formatBytes(getBytesShared());
	ucParams["ip"] = getIp();
	ucParams["clienttype"] = getClientType();
	ucParams["statedshare"] = Util::toString(getBytesShared());
	ucParams["statedshareformat"] = Util::formatBytes(getBytesShared());
	if(realBytesShared > -1) {
		ucParams["realshare"] = Util::toString(realBytesShared);
		ucParams["realshareformat"] = Util::formatBytes(realBytesShared);
	}
	ucParams["cheatingdescription"] = cheatingString;
	ucParams["clientinfo"] = getReport();
	ucParams["nl"] = "\r\n";
	if(myNick && client) {
		RLock<> l(cs);
		ucParams["mynick"] = client->getNick();
	}
}

// favorite user stuff
void User::setFavoriteUser(FavoriteUser* aUser) {
	WLock<> l(cs);
	delete favoriteUser;
	favoriteUser = aUser;
}

bool User::isFavoriteUser() const {
	RLock<> l(cs);
	return (favoriteUser != NULL);
}

void User::setFavoriteLastSeen(u_int32_t anOfflineTime) {
	WLock<> l(cs);
	if (favoriteUser != NULL) {
		if (anOfflineTime != 0)
			favoriteUser->setLastSeen(anOfflineTime);
		else
			favoriteUser->setLastSeen(GET_TIME());
	}
}

u_int32_t User::getFavoriteLastSeen() const {
	RLock<> l(cs);
	if (favoriteUser != NULL)
		return favoriteUser->getLastSeen();
	else
		return 0;
}

const string& User::getUserDescription() const {
	RLock<> l(cs);
	if (favoriteUser != NULL)
		return favoriteUser->getDescription();
	else
		return Util::emptyString;
}

void User::setUserDescription(const string& aDescription) {
	WLock<> l(cs);
	if (favoriteUser != NULL)
		favoriteUser->setDescription(aDescription);
}

StringMap& User::clientEscapeParams(StringMap& sm) const {
	return client->escapeParams(sm);
}

void User::TagParts() {
	char *sTag = strdup(getTag().c_str());
	char *temp;
	if(strlen(sTag) > 2) {
		if((temp = strtok(sTag+1, " ")) != NULL) {
			if(((temp = strtok(NULL, ",")) != NULL) && (temp[0] == 'V')) {
					setVersion(temp+2);
			}
			if(((temp = strtok(NULL, ",")) != NULL) && (temp[0] == 'M')) {
					setMode(temp+2);
			}
			if(((temp = strtok(NULL, ",")) != NULL) && (temp[0] == 'H')) {
				setHubs(temp+2);
			}
			if(((temp = strtok(NULL, ",")) != NULL) && (temp[0] == 'S')) {
				setSlots(Util::toString(atoi(temp+2)));
			}
			if(((temp = strtok(NULL, ",")) != NULL) && ((temp[0] == 'L') || (temp[0] == 'B') || (temp[0] == 'U'))) {
				setUpload(Util::toString(atoi(temp+2)));
			}
		}	
	}
	free(sTag);
}

string User::getReport()
{
	string temp = getGenerator();
	string report = "\r\nClient:		" + getClientType();
	if(temp=="") {
		temp = "N/A";
	}
	report += "\r\nXML Generator:	" + temp;
	report += "\r\nLock:		" + lock;
	report += "\r\nPk:		" + pk;
	report += "\r\nTag:		" + tag;
	report += "\r\nSupports:		" + supports;
	report += "\r\nStatus:		" + Util::formatStatus(status);
	report += "\r\nTestSUR:		" + testSUR;
	report += "\r\nDisconnects:	" + Util::toString(fileListDisconnects);
	report += "\r\nTimeouts:		" + Util::toString(connectionTimeouts);
	temp = Util::formatBytes(getDownloadSpeed());
	if (temp == "-1 B") {
		temp = "N/A";
	} else {
		temp += "/s";
	}
	report += "\r\nDownspeed:	" + temp;
	report += "\r\nIP:		" + ip;
	report += "\r\nHost:		" + host;
	report += "\r\nDescription:	" + description;
	report += "\r\nEmail:		" + email;
	report += "\r\nConnection:	" + connection;
	report += "\r\nCommands:	" + unknownCommand;
	temp = (getFileListSize() != -1) ? Util::formatBytes(fileListSize) + "  (" + Util::formatExactSize(fileListSize) + " )" : "N/A";
	report += "\r\nFilelist size:	" + temp;
	if ( listLength != -1 ) {
		temp = Util::formatBytes(listLength) + "  (" + Util::toString(listLength) + " B)";
	} else {
		temp = "N/A";
	}
	report += "\r\nListLen:		" + temp;
	report += "\r\nStated Share:	" + Util::formatBytes(bytesShared) + "  (" + Util::formatExactSize(bytesShared) + " )";
	if ( getRealBytesShared() > -1 ) {
		temp = Util::formatBytes(getRealBytesShared()) + "  (" + Util::formatExactSize(getRealBytesShared()) + " )";
	} else {
		temp = "N/A";
	}
	report += "\r\nReal Share:	" + temp;
	temp = cheatingString;
	if (temp.empty()) {	temp = "N/A"; }
	report += "\r\nCheat status:	" + temp;
	report += "\r\nComment:		" + comment;
	return report;
}

void User::updateClientType() {
	if ( isSet(User::DCPLUSPLUS) && (listLength == 11) && (bytesShared > 0) ) {
		setCheat("Fake file list - ListLen = 11" , true);
		clientType = "DC++ Stealth";
		badClient = true;
		badFilelist = true;
		sendRawCommand(SETTING(LISTLEN_MISMATCH));
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
		if (!matchProfile(tag, formattedTagExp))									{ continue; } 
		if (!matchProfile(pk, formattedPkExp))										{ continue; }
		if (!matchProfile(supports, cp.getSupports())) { continue; }
		if (!matchProfile(testSUR, cp.getTestSUR())) { continue; }
		if (!matchProfile(Util::toString(status), cp.getStatus())) { continue; }
		if (!matchProfile(unknownCommand, cp.getUserConCom()))			{ continue; }
		if (!matchProfile(description, formattedExtTagExp))							{ continue; }
		if (!matchProfile(connection, cp.getConnection()))							{ continue; }

		if (verTagExp.find("%[version]") != string::npos) { version = getVersion(verTagExp, tag); }
		if (extTagExp.find("%[version2]") != string::npos) { extraVersion = getVersion(extTagExp, description); }
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
			updated();
			setCheat(cheatingString, true);
			return;
		}
		if(badClient) setCheat(cheatingString, true);
		updated();
		if(cp.getRawToSend() > 0) {
			sendRawCommand(cp.getRawToSend());
		}
		return;
	}
	setClientType("Unknown");
	cheatingString = Util::emptyString;
	badClient = false;
	updated();
}

bool User::matchProfile(const string& aString, const string& aProfile) {
	DETECTION_DEBUG("\t\tMatching String: " + aString + " to Profile: " + aProfile);
	PME reg(aProfile);
	return reg.IsValid() ? reg.match(aString) : false;
}

string User::getVersion(const string& aExp, const string& aTag) {
	string::size_type i = aExp.find("%[version]");
	if (i == string::npos) { 
		i = aExp.find("%[version2]"); 
		return splitVersion(aExp.substr(i + 11), splitVersion(aExp.substr(0, i), aTag, 1), 0);
	}
	return splitVersion(aExp.substr(i + 10), splitVersion(aExp.substr(0, i), aTag, 1), 0);
}

string User::splitVersion(const string& aExp, const string& aTag, const int part) {
	PME reg(aExp);
	if(!reg.IsValid()) { return ""; }
	reg.split(aTag, 2);
	return reg[part];
}

void User::sendRawCommand(const int aRawCommand) {
	RLock<> l(cs);
	if(client) {
		if(!client->getOp()) return;
		string rawCommand = client->getRawCommand(aRawCommand);
		if (!rawCommand.empty()) {
			StringMap paramMap;
			paramMap["mynick"] = client->getNick();
			paramMap["nick"] = getNick();
			paramMap["ip"] = getIp();
//			paramMap["userip"] = getUserIp();
			paramMap["tag"] = getTag();
			paramMap["clienttype"] = getClientType();
			paramMap["statedshare"] = Util::toString(getBytesShared());
			paramMap["statedshareformat"] = Util::formatBytes(getBytesShared());
			if(realBytesShared > -1) {
				paramMap["realshare"] = Util::toString(realBytesShared);
				paramMap["realshareformat"] = Util::formatBytes(realBytesShared);
			}
//			if(filesShared > -1) {
//				paramMap["filesshared"] = Util::toString(filesShared);
//			}
			paramMap["cheatingdescription"] = cheatingString;
			paramMap["cd"] = cheatingString;
			paramMap["clientinfo"] = getReport();

			client->sendRaw(Util::formatParams(rawCommand, paramMap));
		}
	}
}

void User::updated() {
	RLock<> l(cs);
	if(client) {
		User::Ptr user = this;
		client->updated(user);
	}
}

StringMap User::getPreparedFormatedStringMap(Client* aClient /* = NULL */)
{
	StringMap fakeShareParams;
	Client* clientToUse = aClient;
	if(clientToUse == NULL)
	{
		clientToUse = client;
	}
	try
	{
		string ip = " ";
		if(getIp().size() > 0)
		{
			ip += "IP = " + getIp() + " ";
		}
		fakeShareParams["ip"] = ip;
		if(clientToUse != NULL)
		{
			string sNick = clientToUse->getNick();
			fakeShareParams["mynick"] = sNick;
		}
		fakeShareParams["user"] = getNick();
		fakeShareParams["nick"] = getNick();
		fakeShareParams["hub"] = getLastHubName();
		fakeShareParams["hubip"] = getLastHubAddress();
		fakeShareParams["hubaddress"] = getLastHubAddress();
		if(realBytesShared > -1)
		{
			fakeShareParams["realshare"] = Util::toString(realBytesShared);
			fakeShareParams["realshareformat"] = Util::formatBytes(realBytesShared);
		}
		fakeShareParams["statedshare"] = Util::toString(getBytesShared());
		fakeShareParams["statedshareformat"] = Util::formatBytes(getBytesShared());
		fakeShareParams["cheatingdescription"] = cheatingString;
	}
	catch(...)
	{
	}
	return fakeShareParams;
}
void User::addLine(const string& aLine) {
	RLock<> l(cs);
	if(client) {
		client->cheatMessage(aLine);
	}
}
string User::insertUserData(const string& s, Client* aClient /* = NULL */)
{
	StringMap userParams = getPreparedFormatedStringMap(aClient);
	return Util::formatParams(s, userParams);
}



bool User::fileListDisconnected() {
	fileListDisconnects++;

	if(SETTING(ACCEPTED_DISCONNECTS) == 0)
		return false;

	if(fileListDisconnects == SETTING(ACCEPTED_DISCONNECTS)) {
		setCheat("Disconnected file list " + Util::toString(fileListDisconnects) + " times", false);
		updated();
		sendRawCommand(SETTING(DISCONNECT_RAW));
		return true;
	}
	return false;
}
bool User::connectionTimeout() {
	connectionTimeouts++;

	if(SETTING(ACCEPTED_TIMEOUTS) == 0)
		return false;

	if(connectionTimeouts == SETTING(ACCEPTED_TIMEOUTS)) {
		setCheat("Connection timeout " + Util::toString(connectionTimeouts) + " times", false);
		updated();
		try {
			QueueManager::getInstance()->removeTestSUR(this);
		} catch(...) {
		}
		sendRawCommand(SETTING(TIMEOUT_RAW));
		return true;
	}
	return false;
}
void User::setPassive() {
	setFlag(User::PASSIVE);
	if(tag.find(",M:A") != string::npos) {
		setCheat("Tag states active mode but is using passive commands", false);
		updated();
		QueueManager::getInstance()->removeTestSUR(this);
	}
}
// CDM EXTENSION ENDS

/**
 * @file
 * $Id$
 */

