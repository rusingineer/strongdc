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

#include "HubManager.h"

#include "ClientManager.h"
#include "CryptoManager.h"

#include "HttpConnection.h"
#include "StringTokenizer.h"
#include "SimpleXML.h"
#include "UserCommand.h"

#define FAVORITES_FILE "Favorites.xml"
// iDC++ //
#define RECENTS_FILE "Recents.xml"
// iDC++ //

void HubManager::onHttpFinished() throw() {
	string::size_type i, j;
	string* x;
	string bzlist;
	string xmlstr;

	// XML addition
	if((listType == TYPE_BZIP2) || (listType == TYPE_XMLBZIP2)) {
		try {
			CryptoManager::getInstance()->decodeBZ2((u_int8_t*)downloadBuf.data(), downloadBuf.size(), bzlist);
		} catch(const CryptoException&) {
			bzlist.clear();
		}
		x = &bzlist;
		xmlstr = bzlist;
	} else {
		x = &downloadBuf;
		xmlstr = downloadBuf;
	}
	// end of XML addition

	if((listType == TYPE_NORMAL) || (listType == TYPE_BZIP2)) {
		//Normal hublist
	{
		Lock l(cs);
		publicHubs.clear();
		i = 0;
		
		while( (i < x->size()) && ((j=x->find("\r\n", i)) != string::npos)) {
			StringTokenizer tok(x->substr(i, j-i), '|');
			i = j + 2;
			if(tok.getTokens().size() < 4)
				continue;

			StringList::const_iterator k = tok.getTokens().begin();
			const string& name = *k++;
			const string& server = *k++;
			const string& desc = *k++;
			const string& users = *k++;
			publicHubs.push_back(HubEntry(name, server, desc, users));
		}
	}
	} else {
		// XML hublist
		{
			Lock l(cs);
			publicHubs.clear();

			try {
				SimpleXML xml;
				xml.fromXML(xmlstr);
		
				xml.resetCurrentChild();
				
				if(xml.findChild("Hublist"))
				{
					xml.stepIn();
			
						if(xml.findChild("Hubs"))
						{
							xml.stepIn();
							if(xml.findChild("Columns"))
							{
								xml.stepIn();
								/* Get column headers, not used this time
								while(xml.findChild("Column")) {
									const string& name = xml.getChildAttrib("Name");
									const string& width = xml.getChildAttrib("Width");
									const string& type = xml.getChildAttrib("Type");
									const string& align = xml.getChildAttrib("Align");
								} */
								xml.stepOut();
							}
							while(xml.findChild("Hub")) {
								const string& name = xml.getChildAttrib("Name");
								const string& server = xml.getChildAttrib("Address");
								const string& desc = xml.getChildAttrib("Description");
								const string& users = xml.getChildAttrib("Users");
								//additional columns
								const string& shared = xml.getChildAttrib("Shared");
								const string& country = xml.getChildAttrib("Country");
								const string& status = xml.getChildAttrib("Status");
								const string& minshare = xml.getChildAttrib("Minshare");
								const string& minslots = xml.getChildAttrib("Minslots");
								const string& maxhubs = xml.getChildAttrib("Maxhubs");
								const string& maxusers = xml.getChildAttrib("Maxusers");
								const string& reliability = xml.getChildAttrib("Reliability");
								const string& rating = xml.getChildAttrib("Rating");
								const string& port = xml.getChildAttrib("Port");
								publicHubs.push_back(HubEntry(name, server, desc, users, 
									shared, country, status, minshare, minslots, 
									maxhubs, maxusers, reliability, rating, port));

							}

							//Get hubs
							xml.stepOut();
						}

					xml.stepOut();
				}

				xml.stepOut();
			} catch(const Exception&) {
				// Shouldn't happen
			}
		}
		// end of XML hublist
	}
	downloadBuf = Util::emptyString;
}

// iDC++ //
void HubManager::recentsave() {
	try {
		SimpleXML xml;

		xml.addTag("Recents");
		xml.stepIn();

		xml.addTag("Hubs");
		xml.stepIn();

		for(RecentHubEntry::Iter i = recentHubs.begin(); i != recentHubs.end(); ++i) {
			xml.addTag("Hub");
			xml.addChildAttrib("Name", (*i)->getName());
			xml.addChildAttrib("Description", (*i)->getDescription());
			xml.addChildAttrib("Users", (*i)->getUsers());
			xml.addChildAttrib("Server", (*i)->getServer());
		}

		xml.stepOut();

		xml.stepOut();
		
		string fname = Util::getAppPath() + RECENTS_FILE;

		File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		f.write("<?xml version=\"1.0\" encoding=\"windows-1252\"?>\r\n");
		f.write(xml.toXML());
		f.close();
		File::deleteFile(fname);
		File::renameFile(fname + ".tmp", fname);
	} catch(const Exception& e) {
		dcdebug("HubManager::recentsave: %s\n", e.getError().c_str());
	}
}
// iDC++ //

void HubManager::save() {
	if(dontSave)
		return;

	Lock l(cs);
	try {
		SimpleXML xml;

		xml.addTag("Favorites");
		xml.stepIn();

		xml.addTag("Hubs");
		xml.stepIn();

		for(FavoriteHubEntry::Iter i = favoriteHubs.begin(); i != favoriteHubs.end(); ++i) {
			xml.addTag("Hub");
			xml.addChildAttrib("Name", (*i)->getName());
			xml.addChildAttrib("Connect", (*i)->getConnect());
			xml.addChildAttrib("Description", (*i)->getDescription());
			xml.addChildAttrib("Nick", (*i)->getNick(false));
			xml.addChildAttrib("Password", (*i)->getPassword());
			xml.addChildAttrib("Server", (*i)->getServer());
			xml.addChildAttrib("UserDescription", (*i)->getUserDescription());
			xml.addChildAttrib("WindowPosX", (*i)->getWindowPosX());
			xml.addChildAttrib("WindowPosY", (*i)->getWindowPosY());
			xml.addChildAttrib("WindowSizeX", (*i)->getWindowSizeX());
			xml.addChildAttrib("WindowSizeY", (*i)->getWindowSizeY());
			xml.addChildAttrib("WindowType", (*i)->getWindowType());
			xml.addChildAttrib("ChatUserSplit", (*i)->getChatUserSplit());
			xml.addChildAttrib("StealthMode", (*i)->getStealth());
			// CDM EXTENSION BEGINS FAVS
			xml.addChildAttrib("RawOne", (*i)->getRawOne());
			xml.addChildAttrib("RawTwo", (*i)->getRawTwo());
			xml.addChildAttrib("RawThree", (*i)->getRawThree());
			xml.addChildAttrib("RawFour", (*i)->getRawFour());
			xml.addChildAttrib("RawFive", (*i)->getRawFive());
			// CDM EXTENSION ENDS
		}
		xml.stepOut();
		xml.addTag("Users");
		xml.stepIn();
		for(User::Iter j = users.begin(); j != users.end(); ++j) {
			xml.addTag("User");
			xml.addChildAttrib("Nick", (*j)->getNick());
			xml.addChildAttrib("LastHubAddress", (*j)->getLastHubAddress());
			xml.addChildAttrib("LastHubName", (*j)->getLastHubName());
			xml.addChildAttrib("LastSeen", (*j)->getFavoriteLastSeen());
			xml.addChildAttrib("GrantSlot", (*j)->getAutoExtraSlot());
			xml.addChildAttrib("UserDescription", (*j)->getUserDescription());
		}
		xml.stepOut();
		xml.addTag("UserCommands");
		xml.stepIn();
		for(UserCommand::Iter k = userCommands.begin(); k != userCommands.end(); ++k) {
			if(!k->isSet(UserCommand::FLAG_NOSAVE)) {
				xml.addTag("UserCommand");
				xml.addChildAttrib("Type", k->getType());
				xml.addChildAttrib("Context", k->getCtx());
				xml.addChildAttrib("Name", k->getName());
				xml.addChildAttrib("Command", k->getCommand());
				xml.addChildAttrib("Hub", k->getHub());
			}
		}
		xml.stepOut();

		xml.stepOut();

		string fname = Util::getAppPath() + FAVORITES_FILE;

		File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		f.write(SimpleXML::w1252Header);
		f.write(xml.toXML());
		f.close();
		File::deleteFile(fname);
		File::renameFile(fname + ".tmp", fname);


		// CDM EXTENSION BEGINS (profiles)
		SimpleXML xml1;

		xml1.addTag("ClientProfiles");
		xml1.stepIn();
		for(int l = 1; l <= getProfileListSize(); l++) {
			xml1.addTag("ClientProfile");
			ClientProfile cp;
			HubManager::getInstance()->getClientProfileByPosition(l, cp);
			xml1.addChildAttrib("Name", cp.getName());
			xml1.addChildAttrib("Version", cp.getVersion());
			xml1.addChildAttrib("Tag", cp.getTag());
			xml1.addChildAttrib("ExtendedTag", cp.getExtendedTag());
			xml1.addChildAttrib("Lock", cp.getLock());
			xml1.addChildAttrib("Pk", cp.getPk());
			xml1.addChildAttrib("Supports", cp.getSupports());
			xml1.addChildAttrib("TestSUR", cp.getTestSUR());
			xml1.addChildAttrib("UserConCom", cp.getUserConCom());
			xml1.addChildAttrib("Status", cp.getStatus());
			xml1.addChildAttrib("CheatingDescription", cp.getCheatingDescription());
			xml1.addChildAttrib("RawToSend", Util::toString(cp.getRawToSend()));
			xml1.addChildAttrib("TagVersion", Util::toString(cp.getTagVersion()));
			xml1.addChildAttrib("UseExtraVersion", Util::toString(cp.getUseExtraVersion()));
			xml1.addChildAttrib("CheckMismatch", Util::toString(cp.getCheckMismatch()));
		}
		xml1.stepOut();
		// CDM EXTENSION ENDS

		fname = Util::getAppPath() + "ClientProfiles.xml";

		File g(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		g.write(SimpleXML::w1252Header);
		g.write(xml1.toXML());
		g.close();
		File::deleteFile(fname);
		File::renameFile(fname + ".tmp", fname);

	} catch(const Exception& e) {
		dcdebug("HubManager::save: %s\n", e.getError().c_str());
	}
}

// iDC++ //
void HubManager::recentload() {
	try {
		SimpleXML xml;
		xml.fromXML(File(Util::getAppPath() + RECENTS_FILE, File::READ, File::OPEN).read());
		
		if(xml.findChild("Recents")) {
			xml.stepIn();
			recentload(&xml);
			xml.stepOut();
		}
	} catch(const Exception& e) {
		dcdebug("HubManager::recentload: %s\n", e.getError().c_str());
	}
}

void HubManager::recentload(SimpleXML* aXml) {
	dontSave = true;

	aXml->resetCurrentChild();
	if(aXml->findChild("Hubs")) {
		aXml->stepIn();
		while(aXml->findChild("Hub")) {
			RecentHubEntry* e = new RecentHubEntry();
			e->setName(aXml->getChildAttrib("Name"));
			e->setDescription(aXml->getChildAttrib("Description"));
			e->setUsers(aXml->getChildAttrib("Users"));
			e->setServer(aXml->getChildAttrib("Server"));
			recentHubs.push_back(e);
		}
		aXml->stepOut();
	}
	dontSave = false;
}
// iDC++ //

void HubManager::load() {
	
	// Add standard op commands
	static const char kickstr[] = 
		"$To: %[nick] From: %[mynick] $<%[mynick]> You are being kicked because: %[kickline:Reason]|<%[mynick]> is kicking %[nick] because: %[kickline:Reason]|$Kick %[nick]|";
	addUserCommand(UserCommand::TYPE_RAW_ONCE, UserCommand::CONTEXT_CHAT | UserCommand::CONTEXT_SEARCH, UserCommand::FLAG_NOSAVE, 
		STRING(KICK_USER), kickstr, "op");
	static const char kickfilestr[] = 
		"$To: %[nick] From: %[mynick] $<%[mynick]> You are being kicked because: %[kickline:Reason] %[file]|<%[mynick]> is kicking %[nick] because: %[kickline:Reason] %[file]|$Kick %[nick]|";
	addUserCommand(UserCommand::TYPE_RAW_ONCE, UserCommand::CONTEXT_SEARCH, UserCommand::FLAG_NOSAVE, 
		STRING(KICK_USER_FILE), kickfilestr, "op");
	static const char redirstr[] =
		"$OpForceMove $Who:%[nick]$Where:%[line:Target Server]$Msg:%[line:Message]|";
	addUserCommand(UserCommand::TYPE_RAW_ONCE, UserCommand::CONTEXT_CHAT | UserCommand::CONTEXT_SEARCH, UserCommand::FLAG_NOSAVE, 
		STRING(REDIRECT_USER), redirstr, "op");
		
	try {
		SimpleXML xml;
		xml.fromXML(File(Util::getAppPath() + FAVORITES_FILE, File::READ, File::OPEN).read());
		
		if(xml.findChild("Favorites")) {
			xml.stepIn();
			load(&xml);
			xml.stepOut();
		}

			// CDM EXTENSION BEGINS (profiles)
	SimpleXML aXml;
	aXml.fromXML(File(Util::getAppPath() + "ClientProfiles.xml", File::READ, File::OPEN).read());
	aXml.resetCurrentChild();
	if(aXml.findChild("ClientProfiles")) {
		aXml.stepIn();
		while(aXml.findChild("ClientProfile")) {
			addClientProfile(
				aXml.getChildAttrib("Name"), 
				aXml.getChildAttrib("Version"),
				aXml.getChildAttrib("Tag"), 
				aXml.getChildAttrib("ExtendedTag"), 
				aXml.getChildAttrib("Lock"),
				aXml.getChildAttrib("Pk"), 
				aXml.getChildAttrib("Supports"),
				aXml.getChildAttrib("TestSUR"), 
				aXml.getChildAttrib("UserConCom"),
				aXml.getChildAttrib("Status"),
				aXml.getChildAttrib("CheatingDescription"),				 
				aXml.getIntChildAttrib("RawToSend"),
				aXml.getIntChildAttrib("TagVersion"), 
				aXml.getIntChildAttrib("UseExtraVersion"), 
				aXml.getIntChildAttrib("CheckMismatch")
				);

		}
		HubManager::getInstance()->sortPriorities();
		aXml.stepOut();
	}
	// CDM EXTENSION ENDS

	} catch(const Exception& e) {
		dcdebug("HubManager::load: %s\n", e.getError().c_str());
	}
}

void HubManager::load(SimpleXML* aXml) {
	dontSave = true;
	
	// Old names...load for compatibility.
	aXml->resetCurrentChild();
	if(aXml->findChild("Favorites")) {
		aXml->stepIn();
		while(aXml->findChild("Favorite")) {
			FavoriteHubEntry* e = new FavoriteHubEntry();
			e->setName(aXml->getChildAttrib("Name"));
			e->setConnect(aXml->getBoolChildAttrib("Connect"));
			e->setDescription(aXml->getChildAttrib("Description"));
			e->setNick(aXml->getChildAttrib("Nick"));
			e->setPassword(aXml->getChildAttrib("Password"));
			e->setServer(aXml->getChildAttrib("Server"));
			e->setUserDescription(aXml->getChildAttrib("UserDescription"));
			e->setWindowPosX(aXml->getIntChildAttrib("WindowPosX"));
			e->setWindowPosY(aXml->getIntChildAttrib("WindowPosY"));
			e->setWindowSizeX(aXml->getIntChildAttrib("WindowSizeX"));
			e->setWindowSizeY(aXml->getIntChildAttrib("WindowSizeY"));
			e->setWindowType(aXml->getIntChildAttrib("WindowType"));
			e->setChatUserSplit(aXml->getIntChildAttrib("ChatUserSplit"));
			e->setStealth(aXml->getBoolChildAttrib("StealthMode"));
			// CDM EXTENSION BEGINS FAVS
			e->setRawOne(aXml->getChildAttrib("RawOne"));
			e->setRawTwo(aXml->getChildAttrib("RawTwo"));
			e->setRawThree(aXml->getChildAttrib("RawThree"));
			e->setRawFour(aXml->getChildAttrib("RawFour"));
			e->setRawFive(aXml->getChildAttrib("RawFive"));
			// CDM EXTENSION ENDS
			favoriteHubs.push_back(e);
		}
		aXml->stepOut();
	}
	aXml->resetCurrentChild();
	if(aXml->findChild("Commands")) {
		aXml->stepIn();
		while(aXml->findChild("Command")) {
			const string& name = aXml->getChildAttrib("Name");
			const string& command = aXml->getChildAttrib("Command");
			const string& hub = aXml->getChildAttrib("Hub");
			const string& nick = aXml->getChildAttrib("Nick");
			if(nick.empty()) {
				// Old mainchat style command
				addUserCommand(UserCommand::TYPE_RAW, UserCommand::CONTEXT_CHAT | UserCommand::CONTEXT_SEARCH, 
					0, name, "<%[mynick]> " + command + "|", hub);
			} else {
				addUserCommand(UserCommand::TYPE_RAW, UserCommand::CONTEXT_CHAT | UserCommand::CONTEXT_SEARCH,
					0, name, "$To: " + nick + " From: %[mynick] $" + command + "|", hub);
			}
		}
		aXml->stepOut();
	}
	// End old names

	aXml->resetCurrentChild();
	if(aXml->findChild("Hubs")) {
		aXml->stepIn();
		while(aXml->findChild("Hub")) {
			FavoriteHubEntry* e = new FavoriteHubEntry();
			e->setName(aXml->getChildAttrib("Name"));
			e->setConnect(aXml->getBoolChildAttrib("Connect"));
			e->setDescription(aXml->getChildAttrib("Description"));
			e->setNick(aXml->getChildAttrib("Nick"));
			e->setPassword(aXml->getChildAttrib("Password"));
			e->setServer(aXml->getChildAttrib("Server"));
			e->setUserDescription(aXml->getChildAttrib("UserDescription"));
			e->setWindowPosX(aXml->getIntChildAttrib("WindowPosX"));
			e->setWindowPosY(aXml->getIntChildAttrib("WindowPosY"));
			e->setWindowSizeX(aXml->getIntChildAttrib("WindowSizeX"));
			e->setWindowSizeY(aXml->getIntChildAttrib("WindowSizeY"));
			e->setWindowType(aXml->getIntChildAttrib("WindowType"));
			e->setChatUserSplit(aXml->getIntChildAttrib("ChatUserSplit"));
			e->setStealth(aXml->getBoolChildAttrib("StealthMode"));
			favoriteHubs.push_back(e);
		}
		aXml->stepOut();
	}
	aXml->resetCurrentChild();
	if(aXml->findChild("Users")) {
		aXml->stepIn();
		while(aXml->findChild("User")) {
			User::Ptr u = ClientManager::getInstance()->getUser(aXml->getChildAttrib("Nick"), aXml->getChildAttrib("LastHubAddress"));
			if(!u->isOnline()) {
				u->setLastHubAddress(aXml->getChildAttrib("LastHubAddress"));
				u->setLastHubName(aXml->getChildAttrib("LastHubName"));
			}
			addFavoriteUser(u);
			u->setAutoExtraSlot(aXml->getBoolChildAttrib("GrantSlot"));
			u->setFavoriteLastSeen((u_int32_t)aXml->getIntChildAttrib("LastSeen"));
			u->setUserDescription(aXml->getChildAttrib("UserDescription"));
		}
		aXml->stepOut();
	}
	aXml->resetCurrentChild();
	if(aXml->findChild("UserCommands")) {
		aXml->stepIn();
		while(aXml->findChild("UserCommand")) {
			addUserCommand(aXml->getIntChildAttrib("Type"), aXml->getIntChildAttrib("Context"),
				0, aXml->getChildAttrib("Name"), aXml->getChildAttrib("Command"), aXml->getChildAttrib("Hub"));
		}
		aXml->stepOut();
	}

	dontSave = false;
}

void HubManager::refresh() {
	StringList l = StringTokenizer(SETTING(HUBLIST_SERVERS), ';').getTokens();
	const string& server = l[(lastServer) % l.size()];
	if(Util::strnicmp(server.c_str(), "http://", 7) != 0) {
		lastServer++;
		return;
	}

	fire(HubManagerListener::DOWNLOAD_STARTING, server);
	if(!running) {
		if(!c)
			c = new HttpConnection();
		{
			Lock l(cs);
			publicHubs.clear();
		}
		c->addListener(this);
		c->downloadFile(server);
		running = true;
	}
}

UserCommand::List HubManager::getUserCommands(int ctx, const string& hub, bool op) {
	Lock l(cs);
	UserCommand::List lst;
	for(UserCommand::Iter i = userCommands.begin(); i != userCommands.end(); ++i) {
		UserCommand& uc = *i;
		if(uc.getCtx() & ctx) {
			if( (uc.getHub().empty()) || 
				(op && uc.getHub() == "op") || 
				(Util::stricmp(hub, uc.getHub()) == 0) )
			{
				lst.push_back(*i);
			}
		}
	}
	return lst;
}

// HttpConnectionListener
void HubManager::onAction(HttpConnectionListener::Types type, HttpConnection* /*conn*/, const u_int8_t* buf, int len) throw() {
	switch(type) {
	case HttpConnectionListener::DATA:
		downloadBuf.append((char*)buf, len); break;
	default:
		dcassert(0);
	}
}

void HubManager::onAction(HttpConnectionListener::Types type, HttpConnection* /*conn*/, const string& aLine) throw() {
	switch(type) {
	case HttpConnectionListener::COMPLETE:
		dcassert(c);
		c->removeListener(this);
		onHttpFinished();
		running = false;
		fire(HubManagerListener::DOWNLOAD_FINISHED, aLine);
		break;
	case HttpConnectionListener::FAILED:
		dcassert(c);
		c->removeListener(this);
		lastServer++; 
		running = false;
		fire(HubManagerListener::DOWNLOAD_FAILED, aLine);
		break;
	case HttpConnectionListener::REDIRECTED:
		fire(HubManagerListener::DOWNLOAD_STARTING, aLine);
		break;
	default:
		break;
	}
}
void HubManager::onAction(HttpConnectionListener::Types type, HttpConnection* /*conn*/) throw() {
	switch(type) {
	case HttpConnectionListener::SET_DOWNLOAD_TYPE_BZIP2:
		listType = TYPE_BZIP2; break;
	case HttpConnectionListener::SET_DOWNLOAD_TYPE_NORMAL:
		listType = TYPE_NORMAL; break;
	// for XML
	case HttpConnectionListener::SET_DOWNLOAD_TYPE_XMLBZIP2:
		listType = TYPE_XMLBZIP2; break;
	case HttpConnectionListener::SET_DOWNLOAD_TYPE_XML:
		listType = TYPE_XML; break;
	//
	default:
		break;
	}
}

void HubManager::onAction(SettingsManagerListener::Types type, SimpleXML* xml) throw() {
	if(type == SettingsManagerListener::LOAD) {
		load(xml); 
		// iDC++
		recentload(xml);
		recentload();
	}
}

/**
 * @file
 * $Id$
 */
