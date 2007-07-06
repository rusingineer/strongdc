/* 
* Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef USERINFOBASE_H
#define USERINFOBASE_H

#include "forward.h"

class UserInfoBase {
public:
	UserInfoBase() { }
	
	void getList();
	void browseList();
	void getUserResponses();
	void checkList();
	void doReport();
	void matchQueue();
	void pm();
	void grant();
	void grantSlotHour();
	void grantSlotDay();
	void grantSlotWeek();
	void ungrantSlot();
	void addFav();
	void removeAll();
	
	virtual const UserPtr& getUser() const = 0;

	static uint8_t getImage(const Identity& u) {
		uint8_t image = 12;

		if(u.isOp()) {
			image = 0;
		} else if(u.getUser()->isSet(User::FIREBALL)) {
			image = 1;
		} else if(u.getUser()->isSet(User::SERVER)) {
			image = 2;
		} else {
			string conn = u.getConnection();
		
			if(	(conn == "28.8Kbps") ||
				(conn == "33.6Kbps") ||
				(conn == "56Kbps") ||
				(conn == "Modem") ||
				(conn == "ISDN")) {
				image = 6;
			} else
			if(	(conn == "Satellite") ||
				(conn == "Microwave") ||
				(conn == "Wireless")) {
				image = 8;
			} else
			if(	(conn == "DSL") ||
				(conn == "Cable")) {
				image = 9;
			} else
			if(	(strncmp(conn.c_str(), "LAN", 3) == 0)) {
				image = 11;
			} else
			if( (strncmp(conn.c_str(), "NetLimiter", 10) == 0)) {
				image = 3;
			} else
			if( (conn == "0.005")) {
				image = 5;
			} else {
				double us = Util::toDouble(conn);
				if(us >= 10) {
					image = 10;
				} else 
				if(us > 0.1) {
					image = 7;
				} else
				if(us >= 0.01) {
					image = 4;
				}
			}
		}
		if(u.getUser()->isSet(User::AWAY)) {
			image+=13;
		}
		if(u.getUser()->isSet(User::DCPLUSPLUS)) {
			image+=26;
		}

		if(!u.isTcpActive()) {
			// Users we can't connect to...
			image+=52;
		}		

		return image;
	}
};

#endif