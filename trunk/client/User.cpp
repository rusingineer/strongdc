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

#include "ResourceManager.h"

#include "User.h"

#include "Client.h"
#include "FavoriteUser.h"

#include "HubManager.h"
#include "QueueManager.h"
#include "../pme/pme.h"
User::~User() throw() {
	delete favoriteUser;
}

void User::connect() {
	RLock l(cs);
	if(client) {
		if(SETTING(CONNECTION_TYPE) == SettingsManager::CONNECTION_ACTIVE) {
			client->connectToMe(this);
		} else {
			client->revConnectToMe(this);
		}
	}
}

const string& User::getClientNick() const {
	RLock l(cs);
	if(client) {
		return client->getNick();
	} else {
		return SETTING(NICK);
	}
}

void User::updated(User::Ptr& aUser) {
	RLock l(aUser->cs);
	if(aUser->client) {
		aUser->client->updated(aUser);
	}
}

const string& User::getClientName() const {
	RLock l(cs);
	if(client) {
		return client->getName();
	} else if(!getLastHubName().empty()) {
		return getLastHubName();
	} else {
		return STRING(OFFLINE);
	}
}

string User::getClientAddressPort() const {
	RLock l(cs);
	if(client) {
		return client->getAddressPort();
	} else {
		return Util::emptyString;
	}
}

void User::privateMessage(const string& aMsg) {
	RLock l(cs);
	if(client) {
		client->privateMessage(this, aMsg);
	}
}

bool User::isClientOp() const {
	RLock l(cs);
	if(client) {
		return client->getOp();
	}
	return false;
}

void User::send(const string& aMsg) {
	RLock l(cs);
	if(client) {
		client->send(aMsg);
	}
}

void User::clientMessage(const string& aMsg) {
	RLock l(cs);
	if(client) {
		client->sendMessage(aMsg);
	}
}

void User::clientPM(const string& aTo, const string& aMsg) {
	RLock l(cs);
	if(client) {
		client->privateMessage(aTo, aMsg);
	}
}

void User::setClient(Client* aClient) { 
	WLock l(cs); 
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

void User::getParams(StringMap& ucParams) {
	ucParams["nick"] = getNick();
	ucParams["tag"] = getTag();
	ucParams["description"] = getDescription();
	ucParams["email"] = getEmail();
	ucParams["share"] = Util::toString(getBytesShared());
	ucParams["shareshort"] = Util::formatBytes(getBytesShared());
	ucParams["ip"] = getIp();
}

// favorite user stuff
void User::setFavoriteUser(FavoriteUser* aUser) {
	WLock l(cs);
	delete favoriteUser;
	favoriteUser = aUser;
}

bool User::isFavoriteUser() const {
	RLock l(cs);
	return (favoriteUser != NULL);
}

void User::setFavoriteLastSeen(u_int32_t anOfflineTime) {
	WLock l(cs);
	if (favoriteUser != NULL) {
		if (anOfflineTime != 0)
			favoriteUser->setLastSeen(anOfflineTime);
		else
			favoriteUser->setLastSeen(GET_TIME());
	}
}

u_int32_t User::getFavoriteLastSeen() const {
	RLock l(cs);
	if (favoriteUser != NULL)
		return favoriteUser->getLastSeen();
	else
		return 0;
}

const string& User::getUserDescription() const {
	RLock l(cs);
	if (favoriteUser != NULL)
		return favoriteUser->getDescription();
	else
		return Util::emptyString;
}

void User::setUserDescription(const string& aDescription) {
	WLock l(cs);
	if (favoriteUser != NULL)
		favoriteUser->setDescription(aDescription);
}

const string User::GetTagPart(string sPart, string ePart) {
	string sTag = getTag();
	string TagPart;
	int iPos = 0;
	int rPos = 0;
	iPos = sTag.find( sPart );
	if (iPos != string::npos) {
		iPos = iPos + ((string)sPart).size();
	}
	rPos = safestring::SafeFind(sTag, ePart, iPos );
	if ( (iPos != string::npos) && (rPos != string::npos) ) {
		TagPart = sTag.substr( iPos, rPos-iPos);
	}
	return TagPart;
}


const string User::GetPart(string zTag, string sPart, string ePart) {  //zTag
	string zTagPart;
	int iPos = 0;
	int rPos = 0;
	iPos = zTag.find( sPart );
	if (iPos != string::npos) {
		iPos = iPos + ((string)sPart).size();
	}
	rPos = safestring::SafeFind(zTag, ePart, iPos );
	if ( (iPos != string::npos) && (rPos != string::npos) ) {
		zTagPart = zTag.substr( iPos, rPos-iPos);
	}
	return zTagPart;
}

void User::TagParts(string sTag ) {
	setTag(sTag);
	string zTag;
	string client;
	string supports = getSupports();
//MessageBox(0,supports.c_str(),"",MB_OK);
	int descLen = description.size();
	if (sTag == Util::emptyString) {
		string sConn = getConnection();
		if ((sConn == Util::emptyString) || (sConn == "Bot") || (sConn == "BOT") || (sConn == "bot"))
			client = "BOT";
		else
			client = "NMDC?";
	} else {
		if (sTag.find("<++")!= string::npos) {
			if (description.find("<o5") != string::npos) {
				client = "oDC";
			} else if (description.find("<CDM ") != string::npos ) {
				client = "DC++k CDM";  
			} else if (description.find("<iDC") != string::npos) {
				client = "iDC++";
			} else if (description.find("zDC") != string::npos) {
				zTag = GetPart( description, "++[", "<+");
				client = "zDC++";
			} else if (description.find("<MS++V>") != string::npos ) {
				client = "MS++V";  
			} else if (description.find("<.P>")!= string::npos) {
				client = "PhantomDC";
			} else if ((descLen >= 3) && ((unsigned(description.find("[A]")) == unsigned(descLen - 3))
				|| (unsigned(description.find("[B]")) == unsigned(descLen - 3)) 
				|| (unsigned(description.find("[C]")) == unsigned(descLen - 3))
				|| (unsigned(description.find("[D]")) == unsigned(descLen - 3))
				|| (unsigned(description.find("[F]")) == unsigned(descLen - 3))
				|| (unsigned(description.find("[M]")) == unsigned(descLen - 3))
				|| (unsigned(description.find("[K]")) == unsigned(descLen - 3))
				|| (unsigned(description.find("[pre E]")) == unsigned(descLen - 7)))) {
				client = "CZDC++";
			} else {
				if (sTag.find("L:")!= string::npos) {
					client = "CZDC++";
				} else if (sTag.find("B:")!= string::npos) {
					client = "BCDC++";
				} else if (sTag.find("0.241")!= string::npos) {
					client = "DC++Stealth??";
				} else if ( supports.find("TTH") != string::npos )	{
					if ( description.find("<.P>") != string::npos ) { client = "PhantomDC"; }
					else { client = "BCDC++"; } 
				} else {
					client = "DC++";
				}
			}
			if (sTag.find("<++V")!= string::npos) { client = "(TAG ERROR!!) " + client; }
	    } else {
			client = GetPart( sTag, "<", " ");
			if (client == "DC") { client = "NMDC2"; }
			if (client == "StrgDC++") { client = "StrongDC++"; }
		}
	}
//	setClientType(client);
	string ver;
	if (description.find("<o5") != string::npos) {
		ver = GetPart( description, "<o", ">");
	} else if (description.find("<iDC") != string::npos) {
		ver = GetPart( description, "<iDC", ">");
	} else if (description.find("zDC++") != string::npos) {
		if (description.find("[V:") != string::npos)
			ver = GetPart( sTag, "V:", ",") + GetPart( description, "[V:", "]");
		else
			ver = GetPart( description, "++[", "]");
	} else {
		ver = GetPart( sTag, "V:", ",");
	}

	setVersion(ver);
	string m = GetTagPart( "M:", ",");
	if(m == "P" || m == "5") { setFlag(User::PASSIVE); }
	setMode(m);
	string hs = GetTagPart( "H:", ",");
	string n, r, o;
	int iPos = 0;
	int rPos;
	rPos = safestring::SafeFind( hs, "/", iPos );
	if (rPos == string::npos) {
		setHubs(hs);
	} else {
		n = hs.substr( iPos, rPos-iPos);
		iPos = rPos + 1;
		rPos = safestring::SafeFind( hs, "/", iPos );
		if (rPos == string::npos) {
			setHubs(hs);
		} else {
			r = hs.substr( iPos, rPos-iPos);
			o = hs.substr( rPos + 1, hs.size());
			int h = Util::toInt(n) + Util::toInt(r) + Util::toInt(o);
			setHubs(Util::toString(h));
		}
	}
	string s = GetTagPart( "S:", ",");
	if (s == "") {
		s = GetTagPart( "S:", ">");
	}
	setSlots(s);
	string l;
	iPos = 0;
	rPos = 0;
	iPos = sTag.find( "L:" );
	if (iPos == string::npos) {
		iPos = sTag.find( "B:" );
		if (iPos == string::npos) {
			iPos = sTag.find( "U:" );
			if (iPos == string::npos) {
				return;
			}
		}
	}
	if (iPos != string::npos) {
		iPos = iPos + 2;
	}
	rPos = safestring::SafeFind(sTag, ",", iPos );
	if (rPos == string::npos) {
		rPos = safestring::SafeFind(sTag, ">", iPos );
	}
	if ( (iPos != string::npos) && (rPos != string::npos) ) {
		l = sTag.substr( iPos, rPos-iPos);
	}
	setUpload(l);
} 

// CDM EXTENSION BEGINS
string User::getReport()
{
	string temp = getGenerator();
	string report = "\r\nClient:		" + getClientType();
	if(temp=="") {
		temp = "N/A";
	}
	report += "\r\nXML Generator:	" + temp;
	report += "\r\nLock:		" + getLock();
	report += "\r\nPk:		" + getPk();
	report += "\r\nTag:		" + getTag();
	report += "\r\nSupports:		" + getSupports();
	report += "\r\nTestSUR:		" + getTestSUR();

	report += "\r\nStatus:		" + Util::formatStatus(getStatus());
	//report += "\r\nDisconnects:	" + Util::toString(getFileListDisconnected());
	//report += "\r\nTimeouts:		" + Util::toString(getFileListTimeout());
	temp = Util::formatBytes(getDownloadSpeed());
	if (temp == "0 B") {
		temp = "N/A";
	} else {
		temp += "/s";
	}
	report += "\r\nDownspeed:	" + temp;
	report += "\r\nIP:		" + getIp();
	/*if ( getConnectedHubs() == -1 ) {
		report += "\r\nKnown hubs:	Failed";
	} else {
		report += "\r\nKnown hubs:	" + Util::toString(getConnectedHubs()) + "   " + getKnownHubsString();
	}
	report += "\r\nStated hubs:	" + getTotalHubs();*/
	report += "\r\nHost:		" + getHost();
	report += "\r\nDescription:	" + getDescription();
	report += "\r\nEmail:		" + getEmail();
	report += "\r\nConnection:	" + getConnection();
	report += "\r\nCommands:	" + getUnknownCommand();

	if ( getFileListSize() > 0 ) {
		temp = Util::formatBytes(getFileListSize()) + "  (" + Util::formatNumber(getFileListSize()) + " B)";
	} else {
		temp = "N/A";
	}
	report += "\r\nFilelist size:	" + temp;
	report += "\r\nStated Share:	" + Util::formatBytes(getBytesShared()) + "  (" + Util::formatNumber(getBytesShared()) + " B)";
	if ( getRealBytesShared() > -1 ) {
		temp = Util::formatBytes(getRealBytesShared()) + "  (" + Util::formatNumber(getRealBytesShared()) + " B)";
	} else {
		temp = "N/A";
	}
	report += "\r\nReal Share:	" + temp;
	if ( getJunkBytesShared() > -1 ) {
		temp = Util::formatBytes(getJunkBytesShared()) + "  (" + Util::formatNumber(getJunkBytesShared()) + " B)";
	} else {
		temp = "N/A";
	}
	report += "\r\nJunk Share:	" + temp;
	temp = getCheatingString();
	if (temp.empty()) {
		temp = "N/A";
	}
	report += "\r\nCheat status:	" + temp;
	return report;
}



void User::updateClientType()
{
	ClientProfile::List lst = HubManager::getInstance()->getClientProfiles();
	for(ClientProfile::Iter i = lst.begin(); i != lst.end(); ++i) {
		ClientProfile& cp = *i;	
		StringMap paramMap;
		paramMap["version"] = ".*";
		paramMap["version2"] = ".*";
		string version, pkVersion, extraVersion;
		string tagExp = cp.getTag();
		string pkExp = cp.getPk();
		string extTagExp = cp.getExtendedTag();

		if (!matchProfile(lock, cp.getLock())) { continue; }
		if (!matchProfile(tag, Util::formatParams(tagExp, paramMap))) { continue; } 
		if (!matchProfile(pk, Util::formatParams(pkExp, paramMap))) { continue; }
		if (!matchProfile(supports, cp.getSupports())) { continue; }
		if (!matchProfile(testSUR, cp.getTestSUR())) { continue; }
		if (!matchProfile(Util::toString(status), cp.getStatus())) { continue; }
		if (!matchProfile(unknownCommand, cp.getUserConCom())) { continue; }
		if (!matchProfile(description, Util::formatParams(extTagExp, paramMap))) { continue; }

		if (tagExp.find("%[version]") != string::npos) { version = getVersion(tagExp, tag); }
		if (extTagExp.find("%[version2]") != string::npos) { extraVersion = getVersion(extTagExp, description); }
		if (pkExp.find("%[version]") != string::npos) { pkVersion = getVersion(pkExp, getPk()); }

		if (!(cp.getVersion().empty()) && !matchProfile(version, cp.getVersion())) { continue; }


		if (cp.getUseExtraVersion()) {
			setClientType(cp.getName() + " " + extraVersion );
		} else {
			setClientType(cp.getName() + " " + version);
		}
		if (cp.getCheckMismatch() && version.compare(pkVersion) != 0) { 
			clientType += " Version mis-match";
			cheatingString += " Version mis-match";
			updated();
			return;
		}
		if (!(cp.getCheatingDescription().empty())) {
			cheatingString = cp.getCheatingDescription();
		}
		updated();
		if(cp.getRawToSend() > 0) {
			sendRawCommand(cp.getRawToSend());
		}
		return;
	}
	setClientType("Unknown");
	updated();
}

bool User::matchProfile(const string& aString, const string& aProfile) {
	PME reg(aProfile);
	if(reg.match(aString) > 0) { return true; }
	return false;
}

string User::getVersion(const string& aExp, const string& aTag) {
	string::size_type i = aExp.find("%[version]");
	if (i == string::npos) { 
		i = aExp.find("%[version2]"); 
		return splitVersion(aExp.substr(i + 11), splitVersion(aExp.substr(0, i), aTag));
	}
	return splitVersion(aExp.substr(i + 10), splitVersion(aExp.substr(0, i), aTag));
}
string User::splitVersion(const string& aExp, const string& aTag) {
	PME reg(aExp);
	reg.split(aTag);
	return reg[0];
}
void User::update() {
	RLock l(cs);
	if(client) {
		client->getInfo(this);
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
		if(junkBytesShared > -1)
		{
			fakeShareParams["junkshare"] = Util::toString(junkBytesShared);
			fakeShareParams["junkshareformat"] = Util::formatBytes(junkBytesShared);
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

string User::insertUserData(const string& s, Client* aClient /* = NULL */)
{
	StringMap userParams = getPreparedFormatedStringMap(aClient);
	return Util::formatParams(s, userParams);
}


void User::sendRawCommand(const int aRawCommand) {
	RLock l(cs);
	if(client) {
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
//			if(realShare > -1) {
//				paramMap["realshare"] = Util::toString(realShare);
//				paramMap["realshareformat"] = Util::formatBytes(realShare);
//			}
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
	RLock l(cs);
	if(client) {
		User::Ptr user = this;
		client->updated(user);
	}
}
// CDM EXTENSION ENDS

/**
 * @file
 * $Id$
 */

