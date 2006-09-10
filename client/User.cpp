/*
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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
#include "StringTokenizer.h"
#include "FavoriteUser.h"

#include "ClientManager.h"
#include "ClientProfileManager.h"
#include "pme.h"
#include "UserCommand.h"
#include "ResourceManager.h"

OnlineUser::OnlineUser(const User::Ptr& ptr, Client& client_, u_int32_t sid_) : identity(ptr, sid_), client(&client_) { 

}

void Identity::getParams(StringMap& sm, const string& prefix, bool compatibility) const {
	{
		RLock<> l(cs);
		for(InfMap::const_iterator i = info.begin(); i != info.end(); ++i) {
			sm[prefix + string((char*)(&i->first), 2)] = i->second;
		}
	}
	if(user) {
		sm[prefix + "SID"] = getSIDString();
		sm[prefix + "CID"] = user->getCID().toBase32();
		sm[prefix + "TAG"] = getTag();
		sm[prefix + "SSshort"] = Util::formatBytes(get("SS"));

		if(compatibility) {
			if(prefix == "my") {
				sm["mynick"] = getNick();
				sm["mycid"] = user->getCID().toBase32();
			} else {
				sm["nick"] = getNick();
				sm["cid"] = user->getCID().toBase32();
				sm["ip"] = get("I4");
				sm["tag"] = getTag();
				sm["description"] = get("DE");
				sm["email"] = get("EM");
				sm["share"] = get("SS");
				sm["shareshort"] = Util::formatBytes(get("SS"));
				sm["realshareformat"] = Util::formatBytes(get("RS"));
			}
		}
	}
}

bool Identity::supports(const string& name) const {
	const string& su = get("SU");
	StringTokenizer<string> st(su, ',');
	for(StringIter i = st.getTokens().begin(); i != st.getTokens().end(); ++i) {
		if(*i == name)
			return true;
	}
	return false;
}

void FavoriteUser::update(const OnlineUser& info) { 
	setNick(info.getIdentity().getNick()); 
	setUrl(info.getClient().getHubUrl()); 
}

const string Identity::setCheat(Client& c, const string& aCheatDescription, bool aBadClient) {
	if(!c.isOp() || isOp()) return Util::emptyString;

	if ((!SETTING(FAKERFILE).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
		PlaySound(Text::toT(SETTING(FAKERFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
		
	StringMap ucParams;
	getParams(ucParams, "user", true);
	string cheat = Util::formatParams(aCheatDescription, ucParams, false);
	
	set("CS", cheat);
	set("BC", Util::toString(aBadClient));

	string report = "*** " + STRING(USER) + " " + getNick() + " - " + cheat;
	return report;
}

const string Identity::getReport() const {
	string report = "\r\nClient:		" + get("CT");
	report += "\r\nXML Generator:	" + (get("GE").empty() ? "N/A" : get("GE"));
	report += "\r\nLock:		" + get("LO");
	report += "\r\nPk:		" + get("PK");
	report += "\r\nTag:		" + getTag();
	report += "\r\nSupports:		" + get("SU");
	report += "\r\nStatus:		" + Util::formatStatus(Util::toInt(getStatus()));
	report += "\r\nTestSUR:		" + get("TS");
	report += "\r\nDisconnects:	" + get("FD");
	report += "\r\nTimeouts:		" + get("TO");
	report += "\r\nDownspeed:	" + Util::formatBytes(getUser()->getLastDownloadSpeed()) + "/s";
	report += "\r\nIP:		" + getIp();
	report += "\r\nHost:		" + Socket::getRemoteHost(getIp());
	report += "\r\nDescription:	" + getDescription();
	report += "\r\nEmail:		" + getEmail();
	report += "\r\nConnection:	" + getConnection();
	report += "\r\nCommands:	" + get("UC");

	int64_t listSize = Util::toInt64(get("LS"));
	report += "\r\nFilelist size:	" + ((listSize != -1) ? (string)(Util::formatBytes(listSize) + "  (" + Text::fromT(Util::formatExactSize(listSize)) + " )") : "N/A");
	
	int64_t listLen = Util::toInt64(get("LL"));
	report += "\r\nListLen:		" + (listLen != -1 ? (string)(Util::formatBytes(listLen) + "  (" + Text::fromT(Util::formatExactSize(listLen)) + " )") : "N/A");
	report += "\r\nStated Share:	" + Util::formatBytes(getBytesShared()) + "  (" + Text::fromT(Util::formatExactSize(getBytesShared())) + " )";
	
	int64_t realBytes = Util::toInt64(get("RS"));
	report += "\r\nReal Share:	" + (realBytes > -1 ? (string)(Util::formatBytes(realBytes) + "  (" + Text::fromT(Util::formatExactSize(realBytes)) + " )") : "N/A");
	report += "\r\nCheat status:	" + (get("CS").empty() ? "N/A" : get("CS"));
	report += "\r\nComment:		" + get("CM");
	return report;
}

const string Identity::updateClientType(OnlineUser& ou) {
	if ( getUser()->isSet(User::DCPLUSPLUS) && (get("LL") == "11") && (getBytesShared() > 0) ) {
		string report = setCheat(ou.getClient(), "Fake file list - ListLen = 11" , true);
		set("CT", "DC++ Stealth");
		set("BC", "1");
		set("BF", "1");
		sendRawCommand(ou.getClient(), SETTING(LISTLEN_MISMATCH));
		return report;
	} else if( getUser()->isSet(User::DCPLUSPLUS) &&
		strncmp(getTag().c_str(), "<++ V:0.69", 10) == 0 &&
		get("LL") != "42") {
			string report = setCheat(ou.getClient(), "Listlen mismatched" , true);
			set("CT", "Faked DC++");
			set("CM", "Supports corrupted files...");
			set("BC", "1");
			sendRawCommand(ou.getClient(), SETTING(LISTLEN_MISMATCH));
			return report;
	}
	int64_t tick = GET_TICK();

	StringMap params;
	ClientProfile::List& lst = ClientProfileManager::getInstance()->getClientProfiles(params);

	for(ClientProfile::Iter i = lst.begin(); i != lst.end(); ++i) {
		ClientProfile& cp = const_cast<ClientProfile&>(*i);
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

		if (!matchProfile(get("LO"), cp.getLock())) { continue; }
		if (!matchProfile(getTag(), formattedTagExp)) { continue; } 
		if (!matchProfile(get("PK"), formattedPkExp)) { continue; }
		if (!matchProfile(get("SU"), cp.getSupports())) { continue; }
		if (!matchProfile(get("TS"), cp.getTestSUR())) { continue; }
		if (!matchProfile(getStatus(), cp.getStatus())) { continue; }
		if (!matchProfile(get("UC"), cp.getUserConCom())) { continue; }
		if (!matchProfile(getDescription(), formattedExtTagExp))	{ continue; }
		if (!matchProfile(getConnection(), cp.getConnection()))	{ continue; }

		if (verTagExp.find("%[version]") != string::npos) { version = getVersion(verTagExp, getTag()); }
		if (extTagExp.find("%[version2]") != string::npos) { extraVersion = getVersion(extTagExp, getDescription()); }
		if (pkExp.find("%[version]") != string::npos) { pkVersion = getVersion(pkExp, get("PK")); }

		if (!(cp.getVersion().empty()) && !matchProfile(version, cp.getVersion())) { continue; }

		DETECTION_DEBUG("Client found: " + cp.getName() + " time taken: " + Util::toString(GET_TICK()-tick) + " milliseconds");
		if (cp.getUseExtraVersion()) {
			set("CT", cp.getName() + " " + extraVersion );
		} else {
			set("CT", cp.getName() + " " + version);
		}
		set("CS", cp.getCheatingDescription());
		set("CM", cp.getComment());
		set("BC", cp.getCheatingDescription().empty() ? Util::emptyString : "1");

		if (cp.getCheckMismatch() && version.compare(pkVersion) != 0) { 
			set("CT", get("CT") + " Version mis-match");
			set("CS", get("CS") + " Version mis-match");
			set("BC", "1");
			string report = setCheat(ou.getClient(), get("CS"), true);
			return report;
		}
		string report = Util::emptyString;
		if(!get("BC").empty()) report = setCheat(ou.getClient(), get("CS"), true);
		if(cp.getRawToSend() > 0) {
			sendRawCommand(ou.getClient(), cp.getRawToSend());
		}
		return report;
	}
	set("CT", "Unknown");
	set("CS", Util::emptyString);
	set("BC", Util::emptyString);

	return Util::emptyString;
}

bool Identity::matchProfile(const string& aString, const string& aProfile) const {
	DETECTION_DEBUG("\t\tMatching String: " + aString + " to Profile: " + aProfile);
	PME reg(aProfile);
	return reg.IsValid() ? (reg.match(aString) > 0) : false;
}

string Identity::getVersion(const string& aExp, const string& aTag) const {
	string::size_type i = aExp.find("%[version]");
	if (i == string::npos) { 
		i = aExp.find("%[version2]"); 
		return splitVersion(aExp.substr(i + 11), splitVersion(aExp.substr(0, i), aTag, 1), 0);
	}
	return splitVersion(aExp.substr(i + 10), splitVersion(aExp.substr(0, i), aTag, 1), 0);
}

string Identity::splitVersion(const string& aExp, const string& aTag, const int part) const {
	PME reg(aExp);
	if(!reg.IsValid()) { return ""; }
	reg.split(aTag, 2);
	return reg[part];
}

void Identity::sendRawCommand(Client& c, const int aRawCommand) {
	string rawCommand = c.getRawCommand(aRawCommand);
	if (!rawCommand.empty()) {
		StringMap ucParams;

		UserCommand uc = UserCommand(0, 0, 0, 0, "", rawCommand, "");
		ClientManager::getInstance()->userCommand(user, uc, ucParams, true);
	}
}

/**
 * @file
 * $Id$
 */
