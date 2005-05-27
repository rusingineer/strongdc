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

#include "NmdcHub.h"

#include "ResourceManager.h"
#include "ClientManager.h"
#include "SearchManager.h"
#include "ShareManager.h"
#include "CryptoManager.h"
#include "ConnectionManager.h"

#include "Socket.h"
#include "UserCommand.h"
#include "StringTokenizer.h"
#include "DebugManager.h"
#include "QueueManager.h"

#include "cvsversion.h"

NmdcHub::NmdcHub(const string& aHubURL) : Client(aHubURL, '|'), supportFlags(0), state(STATE_CONNECT),
	reconnect(true),
	lastbytesshared(0), validatenicksent(false), bFirstOpList(true), PtokaX(false),
    YnHub(false) {
	TimerManager::getInstance()->addListener(this);
}

NmdcHub::~NmdcHub() throw() {
	TimerManager::getInstance()->removeListener(this);
	clearUsers();
}

void NmdcHub::connect() {
	reconnect = true;
	supportFlags = 0;
	lastmyinfo.clear();
	lastbytesshared = 0;
	validatenicksent = false;
	bFirstOpList = true;
	PtokaX = false;
    YnHub = false;

	state = STATE_LOCK;

	if(getPort() == 0) {
		setPort(411);
	}
	Client::connect();
}

void NmdcHub::connect(const User* aUser) {
	checkstate(); 
	dcdebug("NmdcHub::connectToMe %s\n", aUser->getNick().c_str());
	char buf[256];
	if(ClientManager::getInstance()->isActive(this)) {
		sprintf(buf, "$ConnectToMe %s %s:%d|", toNmdc(aUser->getNick()).c_str(), getLocalIp().c_str(), SETTING(TCP_PORT));
		ConnectionManager::iConnToMeCount++;
	} else {
		sprintf(buf, "$RevConnectToMe %s %s|", toNmdc(getNick()).c_str(), toNmdc(aUser->getNick()).c_str());
	}
	send(buf); 
}

int64_t NmdcHub::getAvailable() const {
	Lock l(cs);
	int64_t x = 0;
	for(User::NickMap::const_iterator i = users.begin(); i != users.end(); ++i) {
		x+=i->second->getBytesShared();
	}
	return x;
}

void NmdcHub::refreshUserList(bool unknownOnly /* = false */) {
	if(unknownOnly) {
		Lock l(cs);
		for(User::NickIter i = users.begin(); i != users.end(); ++i) {
			if(i->second->getConnection().empty()) {
				getInfo(i->second);
			}
		}
	} else {
		clearUsers();
		getNickList();
	}
}

void NmdcHub::clearUsers() {
	User::NickMap u2;
	
	{
		Lock l(cs);
		u2 = users;
		users.clear();
	}
	
	for(User::NickIter i = u2.begin(); i != u2.end(); ++i) {
		ClientManager::getInstance()->putUserOffline(i->second);
	}
}

void NmdcHub::onLine(const char* aLine) throw() {
	char *temp, *temp1;
	updateActivity();
	
	if(aLine[0] != '$') {
		if ((BOOLSETTING(SUPPRESS_MAIN_CHAT)) && (!getOp())) {
			return;
		}

		// Check if we're being banned...
		if(state != STATE_CONNECTED) {
			if(strstr(aLine, "banned") != 0) {
				reconnect = false;
			}
		}
		fire(ClientListener::Message(), this, Util::validateMessage(fromNmdc(aLine), true).c_str());
		return;
	}

	switch(aLine[1]) {
        case 'S':
	    	// $Search
    		if(strncmp(aLine+2, "earch ", 6) == 0) {
				aLine += 8;
				if(state != STATE_CONNECTED || (temp = strchr((char*)aLine, ' ')) == NULL || temp+1 == NULL) return;

				temp[0] = NULL; temp += 1;
				if(aLine == NULL || temp+1 == NULL || temp+2 == NULL || temp+3 == NULL || temp+4 == NULL) return;

				string seeker = fromNmdc(aLine);
				bool bPassive = (strnicmp(seeker.c_str(), "Hub:", 4) == 0);
				bool isActive = ClientManager::getInstance()->isActive(this);

				// We don't wan't to answer passive searches if we're in passive mode...
				if((bPassive == true) && !isActive) {
					return;
				}
				// Filter own searches
				if(isActive && bPassive == false) {
					if((strcmp(seeker.c_str(), (getLocalIp() + ":" + Util::toString(SETTING(UDP_PORT))).c_str())) == 0) {
						return;
					}
				} else {
					if(strcmp(seeker.c_str() + 4, getNick().c_str()) == 0) {
						return;
					}
				}

				{
					Lock l(cs);
					u_int32_t tick = GET_TICK();
					seekers.push_back(make_pair(seeker, tick));
					// First, check if it's a flooder
					FloodIter fi;
					for(fi = flooders.begin(); fi != flooders.end(); ++fi) {
						if(fi->first == seeker)
							return;
					}	
					int count = 0;
					for(fi = seekers.begin(); fi != seekers.end(); ++fi) {
						if(fi->first == seeker)
							count++;
	
						if(count > 7) {
					    	// BFU don't need to know search spammers ;)
						    if(getOp()) {
								if(bPassive == true && strlen(seeker.c_str()) > 4) {
									fire(ClientListener::SearchFlood(), this, seeker.c_str()+4);
								} else {
									fire(ClientListener::SearchFlood(), this, seeker + STRING(NICK_UNKNOWN));
								}
							}				
							flooders.push_back(make_pair(seeker, tick));
							return;
						}
					}
				}
				int a;
				if(temp[0] == 'F') {
					a = SearchManager::SIZE_DONTCARE;
				} else {
					if(temp[2] == 'F') {
						a = SearchManager::SIZE_ATLEAST;
					} else {
						a = SearchManager::SIZE_ATMOST;
					}
				}
				temp += 4;
				if((temp1 = strchr(temp, '?')) == NULL || temp1+1 == NULL || temp1+2 == NULL) return;

				temp1[0] = NULL; temp1[2] = NULL;
				if(temp == NULL) return;
	
				int64_t size = _atoi64(temp);
				int type = atoi(temp1+1) - 1; temp1 += 3;
				if(temp1 != NULL) {
					fire(ClientListener::NmdcSearch(), this, seeker, a, size, type, fromNmdc(temp1), bPassive);
					if(bPassive == true && strlen(seeker.c_str()) > 4) {
						User::Ptr u;
						Lock l(cs);
						User::NickIter ni = users.find(seeker.c_str()+4);
						if(ni != users.end() && !ni->second->isSet(User::PASSIVE) && ni->second->getMode() != "A") {
							u = ni->second;
							u->setPassive();
						}
						if(u) {
							updated(u);
						}
					}
				}
    			return;
        	}
        
	        // $SR
	        if(strncmp(aLine+2, "R ", 2) == 0) {
				SearchManager::getInstance()->onSearchResult(aLine);
				return;
	        }
        
    	    // $Supports
       		if(strncmp(aLine+2, "upports", 7) == 0) {
				StringList sl;
				aLine += 10;
        		if(aLine == NULL) {
					validateNick(getNick());
					return;
				}

        		while(aLine != NULL) {
                    if((temp = strchr(aLine, ' ')) != NULL) {
                        temp[0] = NULL; temp++;
                    }

        			sl.push_back(aLine);
        			switch(aLine[0]) {
                        case 'U':
                			if((strcmp(aLine+1, "serCommand")) == 0) {
								supportFlags |= SUPPORTS_USERCOMMAND;
                			} else if((strcmp(aLine+1, "serIP2")) == 0) {
								supportFlags |= SUPPORTS_USERIP2;
                            }
                            break;
                        case 'N':
                			if((strcmp(aLine+1, "oGetINFO")) == 0) {
                				supportFlags |= SUPPORTS_NOGETINFO;
                            }
                            break;
                        case 'Q':
                			if((strcmp(aLine+1, "uickList")) == 0) {
								if(state == STATE_HELLO) {
									state = STATE_MYINFO;
									updateCounts(false);
									myInfo();
									getNickList();
									supportFlags |= SUPPORTS_QUICKLIST;
								}
							}
                			break;
                		default:
                            dcdebug("NmdcHub::onLine Unknown supports %s\n", aLine);
                            break;
                    }
        			aLine = temp;
				}
				if(!(getSupportFlags() & SUPPORTS_QUICKLIST)) {
					validateNick(getNick());
				}
	   			return;
    	    }
            
            dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
            return;
    	case 'M':
	    	// $MyINFO
    		if(strncmp(aLine+2, "yINFO ", 6) == 0) {
			    char *Description, *Tag, *Connection, *Email, *Share;
		    	aLine += 13;
	    		Description = strchr((char*)aLine, ' ');
			    if(Description == NULL || Description+1 == NULL)
					return;

				Description[0] = NULL;
				Description += 1;
	    
			    if(aLine == NULL)
		    	     return;

				User::Ptr u;
				string nick = fromNmdc(aLine);
	
				{
					Lock l(cs);
					User::NickIter ni = users.find(nick);
					if(ni == users.end()) {
						u = users[nick] = ClientManager::getInstance()->getUser(nick, this);
					} else {
						u  = ni->second;
					}
				}

				// New MyINFO received, delete all user variables... if is bad MyINFO is user problem not my ;-)
        		u->setNick(nick);
				u->setBytesShared(0);
				u->setDescription(Util::emptyString);
				u->setcType(10);
				u->setStatus(1);		
				u->setConnection(Util::emptyString);
				u->setEmail(Util::emptyString);
				u->setTag(Util::emptyString);
				u->setVersion(Util::emptyString);
				u->setMode(Util::emptyString);
				u->setHubs(Util::emptyString);
				u->setSlots(0);
				u->setUpload(Util::emptyString);

			    Connection = strchr(Description, '$');
	    		if(Connection && Connection+1 && Connection+2 && Connection+3) {
					Connection[0] = NULL;
					Connection += 1;
					if(Description) {
						if(*(Connection-2) == '>') {
							if((Tag = strrchr(Description, '<')) != NULL) {
								u->setTag(fromNmdc(Tag));
								u->TagParts();
								Tag[0] = NULL;
							} else
								u->setTag(Util::emptyString);
						}
						u->setDescription(Util::validateMessage(fromNmdc(Description), true));
					}
					Email = strchr(Connection+2, '$');
					if(Email && Email+1) {
						Email[0] = NULL;
						Email += 1;
						Share = strchr(Email, '$');
						if(Share && Share+1) {
							if((temp = strchr(Share+1, '$')) != NULL) temp[0] = NULL;
							Share[0] = NULL;
							Share += 1;
							u->setEmail(Util::validateMessage(fromNmdc(Email), true));
							u->setBytesShared(Share);
						}
						Connection[1] = NULL;
						if(Connection[0] != ' ')
							u->setMode(Connection);			    
	
						Connection += 2;
						char status = *(Email-2);
						*(Email-2) = NULL;
       					switch(status) {
                            case 1:
                                u->unsetFlag(User::AWAY);
      					        u->unsetFlag(User::SERVER);
       					        u->unsetFlag(User::FIREBALL);
                                break;
                            case 2:
                            case 3:
								u->setFlag(User::AWAY);
								u->unsetFlag(User::SERVER);
								u->unsetFlag(User::FIREBALL);
           						break;
       					    case 4:
                            case 5:
								u->setFlag(User::SERVER);
								u->unsetFlag(User::AWAY);
								u->unsetFlag(User::FIREBALL);
           						break;
       					    case 6:
                            case 7:
								u->setFlag(User::SERVER);
								u->setFlag(User::AWAY);
								u->unsetFlag(User::FIREBALL);
           						break;
           					case 8:
                            case 9:
								u->setFlag(User::FIREBALL);
								u->unsetFlag(User::AWAY);
								u->unsetFlag(User::SERVER);
           						break;
           					case 10:
                            case 11:
								u->setFlag(User::FIREBALL);
								u->setFlag(User::AWAY);
								u->unsetFlag(User::SERVER);
           						break;
           					default:
                                // WTF ?!? ok go to default...
								u->unsetFlag(User::AWAY);
								u->unsetFlag(User::SERVER);
								u->unsetFlag(User::FIREBALL);
                                break;
						}
						u->setStatus(status);
						u->setConnection(fromNmdc(Connection));
						if(strcmp(Connection, "28.8Kbps") == 0 || strcmp(Connection, "33.6Kbps") == 0 ||
							strcmp(Connection, "56Kbps") == 0 || strcmp(Connection, "Modem") == 0) {
							u->setcType(1);
						} else if(strcmp(Connection, "ISDN") == 0) {
							u->setcType(2);
						} else if(strcmp(Connection, "Satellite") == 0 || strcmp(Connection, "Microwave") == 0) {
							u->setcType(3);
						} else if(strcmp(Connection, "Wireless") == 0) {
							u->setcType(4);
						} else if(strcmp(Connection, "DSL") == 0) {
							u->setcType(5);
						} else if(strcmp(Connection, "Cable") == 0) {
							u->setcType(6);
						} else if(strcmp(Connection, "LAN(T1)") == 0 || strcmp(Connection, "LAN(T3)") == 0) {
							u->setcType(7);
						} else
							u->setcType(10);
					}
				}

				if(state == STATE_MYINFO) {
					if(stricmp(u->getNick().c_str(), getNick().c_str()) == 0) {
						setMe(u);
						state = STATE_CONNECTED;
						updateCounts(false);
						u->setFlag(User::DCPLUSPLUS);
						if(!ClientManager::getInstance()->isActive(this))
							u->setFlag(User::PASSIVE);
						else
							u->unsetFlag(User::PASSIVE);
					}
				}
				fire(ClientListener::UserUpdated(), this, u);
		    	return;
		    }
            
            dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
            return;
        case 'Q':
	    	// $Quit		    
			if(strncmp(aLine+2, "uit ", 4) == 0) {
				aLine += 6;
				if(aLine == NULL)
					return;
	
				User::Ptr u;
				{
					Lock l(cs);
					User::NickIter i = users.find(fromNmdc(aLine));
					if(i == users.end()) {
						dcdebug("C::onLine Quitting user %s not found\n", aLine);
						return;
					}
			
					u = i->second;
					users.erase(i);
				}
		
				fire(ClientListener::UserRemoved(), this, u);
				ClientManager::getInstance()->putUserOffline(u, true);
    			return;
        	}
            
            dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
            return;
        case 'C':
			// $ConnectToMe
			if(strncmp(aLine+2, "onnectToMe ", 11) == 0) {
				aLine += 13;
				if(state != STATE_CONNECTED || (temp = strchr((char*)aLine, ' ')) == NULL || temp+1 == NULL) return;
	
				temp[0] = NULL; temp += 1;
				if(aLine == NULL || temp == NULL) return;

				if(stricmp(fromNmdc(aLine).c_str(), getNick().c_str()) != 0) // Check nick... is CTM really for me ? ;o)
					return;
	
				if((temp1 = strchr(temp, ':')) == NULL || temp1+1 == NULL) return;

				temp1[0] = NULL; temp1 += 1;
				if(temp == NULL || temp1 == NULL) return;

				ConnectionManager::getInstance()->nmdcConnect(temp, (short)atoi(temp1), getNick()); 
	    		return;
    	    }
            
            dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
            return;
        case 'R':
    		// $RevConnectToMe
    		if(strncmp(aLine+2, "evConnectToMe ", 14) == 0) {
				if(state != STATE_CONNECTED) return;

				aLine += 16;
				User::Ptr u;
				bool up = false;
				{
					Lock l(cs);
					if((temp = strchr((char*)aLine, ' ')) == NULL || temp+1 == NULL) return;

					temp[0] = NULL; temp += 1;
					if(aLine == NULL || temp == NULL) return;

					User::NickIter i = users.find(fromNmdc(aLine));

					if(stricmp(fromNmdc(temp).c_str(), getNick().c_str()) != 0) // Check nick... is RCTM really for me ? ;-)
						return;

					if(i == users.end()) {
						return;
					}

					u = i->second;
					if(!u->isSet(User::PASSIVE)) {
						u->setPassive();
						up = true;
					}
				}

				if(u) {
					if(ClientManager::getInstance()->isActive(this)) {
						connectToMe(u);	
					} else {
						// Notify the user that we're passive too...
						if(up)
							revConnectToMe(u);
					}

					if(up)
						updated(u);
				}
    			return;
	        }
            
            dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
            return;
        case 'H':
	    	// $HubName
    		if(strncmp(aLine+2, "ubName ", 7) == 0) {
				aLine += 9;
				if(aLine == NULL) return;
	
				name = fromNmdc(aLine);
				fire(ClientListener::HubUpdated(), this);
				return;
			}
        
        	// $Hello
       		if(strncmp(aLine+2, "ello ", 5) == 0) {
				if(getSupportFlags() & SUPPORTS_QUICKLIST) return;
				aLine += 7;
				if(aLine == NULL) return;
	
				string nick = fromNmdc(aLine);
				User::Ptr u = ClientManager::getInstance()->getUser(nick, this);
				{
					Lock l(cs);
					users[nick] = u;
				}
		
        		u->setNick(nick);
        		
        		if(stricmp(getNick().c_str(), nick.c_str()) == NULL) {
					setMe(u);
		
					u->setFlag(User::DCPLUSPLUS);
					if(!ClientManager::getInstance()->isActive(this))
						u->setFlag(User::PASSIVE);
					else
						u->unsetFlag(User::PASSIVE);
				}

				if(state == STATE_HELLO) {
					state = STATE_CONNECTED;
					updateCounts(false);

					version();
					if(YnHub == false) {
						myInfo();
						getNickList();
	            	} else {
						getNickList();
						myInfo();
    	        	}
				}
				fire(ClientListener::UserUpdated(), this, u);
				return;
			}
        
        	// $HubIsFull
	       	if(strcmp(aLine+2, "ubIsFull") == 0) {
				fire(ClientListener::HubFull(), this);
				return;
			}
        	
        	dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
            return;
        case 'U':
	    	// $UserCommand
    		if(strncmp(aLine+2, "serCommand ", 11) == 0) {
				temp = (char*)aLine+13;
				if(temp == NULL || temp+1 == NULL) return;

				temp[1] = NULL;
				int type = atoi(temp);
				temp += 2;
				if(temp == NULL || temp+1 == NULL) return;

				temp[1] = NULL;
				int ctx = atoi(temp);
				temp += 2;
				if(temp == NULL) return;

        		switch(type) {
                    case UserCommand::TYPE_SEPARATOR:
                    case UserCommand::TYPE_CLEAR:
        			    fire(ClientListener::UserCommand(), this, type, ctx, Util::emptyString, Util::emptyString);
        			    break;
            		case UserCommand::TYPE_RAW:
                    case UserCommand::TYPE_RAW_ONCE:
						if((temp1 = strchr(temp, '$')) == NULL || temp1+1 == NULL) return;

						temp1[0] = NULL; temp1 += 1;
						if(temp == NULL || temp1 == NULL) return;

						fire(ClientListener::UserCommand(), this, type, ctx, Util::validateMessage(fromNmdc(temp), true, false), Util::validateMessage(fromNmdc(temp1), true, false));
       			    	break;
        			default:
                        // ?!?
                        break;
				}
				return;
			}
        
    	    // $UserIP
       		if(strncmp(aLine+2, "serIP ", 6) == 0) {
				User::List v;
				StringList l;
				aLine += 8;
				if(aLine == NULL) return;

				while((temp = strchr(aLine, ' ')) != NULL && temp+1 != NULL) {
					temp[0] = NULL; temp += 1; temp1 = strchr(temp, '$');
					if(aLine == NULL || temp == NULL) break;
					if(temp1 == NULL) {
						v.push_back(ClientManager::getInstance()->getUser(fromNmdc(aLine), this));
						v.back()->setIp(temp);
						break;
					}

					temp1[0] = NULL;
					if(temp == NULL) break;
			
					User::Ptr u = ClientManager::getInstance()->getUser(fromNmdc(aLine), this);
					v.push_back(u);
					v.back()->setIp(temp);
					ClientManager::getInstance()->setIPNick(temp, u);
					aLine = temp1+2;
				}
				fire(ClientListener::UsersUpdated(), this, v);
				return;
			}
            
            dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
            return;
        case 'L':
	    	// $Lock
    		if(strncmp(aLine+2, "ock ", 4) == 0) {
				aLine += 6;
				if(state != STATE_LOCK || aLine == NULL) return;

				state = STATE_HELLO;
				if((temp = strstr(aLine, " Pk=")) != NULL && temp+1 != NULL)
					temp[0] = NULL; temp += 1;
	
				if(aLine == NULL) return;

    			if(temp != NULL) {
    				if(stricmp(temp+3, "YnHub") == 0) {
    					YnHub = true;
	    			} else if(strcmp(temp+3, "PtokaX") == 0) {
    					PtokaX = true;
    				}
            	}
            		
				if(CryptoManager::getInstance()->isExtended(aLine)) {
					StringList feat;
					feat.push_back("UserCommand");
					feat.push_back("NoGetINFO");
					feat.push_back("NoHello");
					feat.push_back("UserIP2");
					feat.push_back("TTHSearch");

					if (getStealth() == false)
						feat.push_back("QuickList");

					if(BOOLSETTING(COMPRESS_TRANSFERS))
						feat.push_back("GetZBlock");
					supports(feat);
    				if(PtokaX == false) key(CryptoManager::getInstance()->makeKey(aLine));
				} else {
					key(CryptoManager::getInstance()->makeKey(aLine));
					validateNick(getNick());
				}
				if(temp != NULL) {
					if(stricmp(temp+3, "YnHub") == 0) {
						YnHub = true;
					} else if(strcmp(temp+3, "PtokaX") == 0) {
						PtokaX = true;
					}
    	    	}
        		return;
	        }
          	dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
            return;
    	case 'F':
	    	if(strncmp(aLine+2, "orceMove ", 9) == 0) {
				aLine += 11;
				if(aLine == NULL) return;

				disconnect();
				fire(ClientListener::Redirect(), this, fromNmdc(aLine));
    			return;
        	}
            
            dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
            return;
        case 'V':
	    	// $ValidateDenide
    		if(strncmp(aLine+2, "alidateDenide", 13) == 0) {
				disconnect();
				fire(ClientListener::NickTaken(), this);
	    		return;
    	    }
            
            dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
            return;
        case 'N':
	    	// $NickList
    		if(strncmp(aLine+2, "ickList ", 8) == 0) {
				User::List v;
				aLine += 10;
				if(aLine == NULL) return;

				while((temp = strchr(aLine, '$')) != NULL) {
					temp[0] = NULL;
					if(aLine == NULL) break;

					v.push_back(ClientManager::getInstance()->getUser(fromNmdc(aLine), this));
					aLine = temp+2;
				}
	
				{
					Lock l(cs);
					for(User::Iter it2 = v.begin(); it2 != v.end(); ++it2) {
						users[(*it2)->getNick()] = *it2;
					}
				}
		
				if(!(getSupportFlags() & SUPPORTS_NOGETINFO)) {
					string tmp;
					// Let's assume 10 characters per nick...
					tmp.reserve(v.size() * (11 + 10 + getNick().length())); 
					string n = ' ' +  toNmdc(getNick()) + '|';
					for(User::List::const_iterator i = v.begin(); i != v.end(); ++i) {
						tmp += "$GetINFO ";
						tmp += toNmdc((*i)->getNick());
						tmp += n;
					}
					if(!tmp.empty()) {
						send(tmp);
					}
				}
				fire(ClientListener::UsersUpdated(), this, v);
    			return;
	        }
            
            dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
            return;
        case 'O':
	    	// $OpList
    		if(strncmp(aLine+2, "pList ", 6) == 0) {
				User::List v;
				aLine += 8;
				if(aLine == NULL) return;

				while((temp = strchr(aLine, '$')) != NULL) {
					temp[0] = NULL;
					if(aLine == NULL) break;

					v.push_back(ClientManager::getInstance()->getUser(fromNmdc(aLine), this));
					v.back()->setFlag(User::OP);
					aLine = temp+2;
				}

				{
					Lock l(cs);
					for(User::Iter it2 = v.begin(); it2 != v.end(); ++it2) {
						users[(*it2)->getNick()] = *it2;
					}
				}
				fire(ClientListener::UsersUpdated(), this, v);
				updateCounts(false);
				if(bFirstOpList == true) {
					if(getRegistered() == true) {
						myInfo();
		            }
					bFirstOpList = false;
				}
	    		return;
    	    }
            
            dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
            return;
        case 'T':
	    	// $To
    		if(strncmp(aLine+2, "o: ", 3) == 0) {
				if((temp1 = strstr(aLine+5, "From:")) != NULL && strlen(temp1) > 6) {
					if((temp = strchr(temp1+6, '$')) == NULL || temp+1 == NULL) return;
	
					temp1 += 6; *(temp-1) = NULL; temp += 1;
					if(temp1 == NULL || temp == NULL) return;

					fire(ClientListener::PrivateMessage(), this, ClientManager::getInstance()->getUser(fromNmdc(temp1), this, false), Util::validateMessage(fromNmdc(temp), true));
				}
    			return;
	        }
            
            dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
            return;
        case 'G':
	    	// $GetPass
    		if(strcmp(aLine+2, "etPass") == 0) {
				setRegistered(true);
				fire(ClientListener::GetPassword(), this);
	    		return;
    	    }
            
            dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
            return;
        case 'B':
	    	// $BadPass
    		if(strcmp(aLine+2, "adPass") == 0) {
				fire(ClientListener::BadPassword(), this);
    			return;
        }
            
            dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
            return;
    	default:
			dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
			return;
	}
}

string NmdcHub::checkNick(const string& aNick) {
	string tmp = aNick;
	string::size_type i = 0;
	while( (i = tmp.find_first_of("|$ ", i)) != string::npos) {
		tmp[i++]='_';
	}
	return tmp;
}

void NmdcHub::connectToMe(const User::Ptr& aUser) {
	checkstate(); 
	dcdebug("NmdcHub::connectToMe %s\n", aUser->getNick().c_str());
	char buf[256];
	sprintf(buf, "$ConnectToMe %s %s:%d|", toNmdc(aUser->getNick()).c_str(), getLocalIp().c_str(), SETTING(TCP_PORT));
	send(buf);
	ConnectionManager::iConnToMeCount++;
}

void NmdcHub::revConnectToMe(const User::Ptr& aUser) {
	checkstate(); 
	dcdebug("NmdcHub::revConnectToMe %s\n", aUser->getNick().c_str());
	char buf[256];
	sprintf(buf, "$RevConnectToMe %s %s|", toNmdc(getNick()).c_str(), toNmdc(aUser->getNick()).c_str());
	send(buf);
}

void NmdcHub::myInfo() {
	if(state != STATE_CONNECTED && state != STATE_MYINFO) {
		return;
	}
	
	dcdebug("MyInfo %s...\n", getNick().c_str());
	char StatusMode = '\x01';
	string tmp0 = "<++";
	string tmp1 = "\x1fU9";
	string tmp2 = "+L9";
	string tmp3 = "+G9";
	string tmp4 = "+R9";
	string tmp5 = "+K9";
	string::size_type i;
	
	for(i = 0; i < 3; i++) {
		tmp1[i]++;tmp2[i]++; tmp3[i]++; tmp4[i]++; tmp5[i]++;
	}
	char modeChar = '?';
	if(SETTING(OUTGOING_CONNECTIONS) == SettingsManager::OUTGOING_SOCKS5)
		modeChar = '5';
	else if(ClientManager::getInstance()->isActive(this))
		modeChar = 'A';
	else 
		modeChar = 'P';
	
	char tag[256];
	string VERZE = DCVERSIONSTRING;	
	if (getStealth() == false) {
		tmp0 = "<StrgDC++";

#ifdef isCVS
	VERZE = VERSIONSTRING STRONGDCVERSIONSTRING CVSVERSION;
#else
	VERZE = VERSIONSTRING STRONGDCVERSIONSTRING;
#endif

	}

	int NetLimit = Util::getNetLimiterLimit();
	string connection = (NetLimit > -1) ? "NetLimiter [" + Util::toString(NetLimit) + " kB/s]" : SETTING(CONNECTION);
	string speedDescription = Util::emptyString;

	if(BOOLSETTING(SHOW_DESCRIPTION_SPEED))
		speedDescription = "["+SETTING(DOWN_SPEED)+"/"+SETTING(UP_SPEED)+"]";

	if (SETTING(THROTTLE_ENABLE) && SETTING(MAX_UPLOAD_SPEED_LIMIT) != 0) {
		sprintf(tag, "%s%s%s%s%c%s%s%s%s%s%s>", tmp0.c_str(), tmp1.c_str(), VERZE.c_str(), tmp2.c_str(), modeChar, tmp3.c_str(), 
		getCounts().c_str(), tmp4.c_str(), Util::toString(UploadManager::getInstance()->getSlots()).c_str(), tmp5.c_str(), Util::toString(SETTING(MAX_UPLOAD_SPEED_LIMIT)).c_str());
	} else {
		sprintf(tag, "%s%s%s%s%c%s%s%s%s>", tmp0.c_str(), tmp1.c_str(), VERZE.c_str(), tmp2.c_str(), modeChar, tmp3.c_str(), 
		getCounts().c_str(), tmp4.c_str(), Util::toString(UploadManager::getInstance()->getSlots()).c_str());
	}

	if(getStealth() == false) {
		if (UploadManager::getFireballStatus()) {
			StatusMode += 8;
		} else if (UploadManager::getFileServerStatus()) {
			StatusMode += 4;
		}

		if(Util::getAway()) {
			StatusMode += 2;
		}
	} else {
		if (connection == "Modem") { connection = "56Kbps"; }
		if (connection == "Wireless") { connection = "Satellite"; }
	}

	string description = getDescription();

	string::size_type m = description.find_first_not_of(' ');
	string::size_type n = description.find_last_not_of(' ');
	if(m != string::npos)
		description = description.substr(m, n-m+1);
	else
		description = Util::emptyString;


	char myinfo[512];
	sprintf(myinfo, "$MyINFO $ALL %s %s%s%s$ $%s%c$%s$", toNmdc(getNick()).c_str(), speedDescription.c_str(),
		toNmdc(Util::validateMessage(description, false)).c_str(), tag, connection.c_str(), StatusMode, 
		toNmdc(Util::validateMessage(SETTING(EMAIL), false)).c_str());
	int64_t newbytesshared = ShareManager::getInstance()->getShareSize();
	if (strcmp(myinfo, lastmyinfo.c_str()) != 0 || newbytesshared < (lastbytesshared - 1048576) || newbytesshared > (lastbytesshared + 1048576)){
		lastmyinfo = myinfo;
		lastbytesshared = newbytesshared;
		tag[0] = NULL;
		sprintf(tag, "%s$|", Util::toString(newbytesshared).c_str());
		strcat(myinfo, tag);
		send(myinfo);	
	}
}

void NmdcHub::disconnect() throw() {	
	state = STATE_CONNECT;
	Client::disconnect();
	clearUsers();
}

void NmdcHub::search(int aSizeType, int64_t aSize, int aFileType, const string& aString, const string&){
	checkstate(); 
	AutoArray<char> buf((char*)NULL);
	char c1 = (aSizeType == SearchManager::SIZE_DONTCARE || aSizeType == SearchManager::SIZE_EXACT) ? 'F' : 'T';
	char c2 = (aSizeType == SearchManager::SIZE_ATLEAST) ? 'F' : 'T';
	string tmp = Util::validateMessage(toNmdc((aFileType == SearchManager::TYPE_TTH) ? "TTH:" + aString : aString), false);
	string::size_type i;
	while((i = tmp.find(' ')) != string::npos) {
		tmp[i] = '$';
	}
	int chars = 0;
	if(ClientManager::getInstance()->isActive(this) && !BOOLSETTING(SEARCH_PASSIVE)) {
		string x = getLocalIp();
		buf = new char[x.length() + aString.length() + 64];
		chars = sprintf(buf, "$Search %s:%d %c?%c?%s?%d?%s|", x.c_str(), SETTING(UDP_PORT), c1, c2, Util::toString(aSize).c_str(), aFileType+1, tmp.c_str());
	} else {
		buf = new char[getNick().length() + aString.length() + 64];
		chars = sprintf(buf, "$Search Hub:%s %c?%c?%s?%d?%s|", toNmdc(getNick()).c_str(), c1, c2, Util::toString(aSize).c_str(), aFileType+1, tmp.c_str());
	}
	send(buf, chars);
}

// TimerManagerListener
void NmdcHub::on(TimerManagerListener::Second, u_int32_t aTick) throw() {
	if(socket && (getLastActivity() + getReconnDelay() * 1000) < aTick) {
		// Nothing's happened for ~120 seconds, check if we're connected, if not, try to connect...
		if(isConnected()) {
			// Try to send something for the fun of it...
			dcdebug("Testing writing...\n");
			send("|", 1);
		} else {
			// Try to reconnect...
			if(reconnect && !getAddress().empty())
				connect();
		}
	}
	{
		Lock l(cs);
		
		while(!seekers.empty() && seekers.front().second + (5 * 1000) < aTick) {
			seekers.pop_front();
		}
		
		while(!flooders.empty() && flooders.front().second + (120 * 1000) < aTick) {
			flooders.pop_front();
		}
	}
}

// BufferedSocketListener
void NmdcHub::on(BufferedSocketListener::Failed, const string& aLine) throw() {
	clearUsers();

	if(state == STATE_CONNECTED)
		state = STATE_CONNECT;

	fire(ClientListener::Failed(), this, aLine); 
	updateCounts(true);
}

/**
 * @file
 * $Id$
 */
