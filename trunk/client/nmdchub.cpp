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

NmdcHub::NmdcHub(const string& aHubURL) : Client(aHubURL, '|'), state(STATE_CONNECT),
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

void NmdcHub::connect(const OnlineUser& aUser) {
	checkstate(); 
	dcdebug("NmdcHub::connectToMe %s\n", aUser.getIdentity().getNick().c_str());
	if(ClientManager::getInstance()->isActive(this)) {
		connectToMe(aUser);
	} else {
		revConnectToMe(aUser);
	}
}

/*int64_t NmdcHub::getAvailable() const {
	Lock l(cs);
	int64_t x = 0;
	for(NickMap::const_iterator i = users.begin(); i != users.end(); ++i) {
		x+=i->second->getIdentity().getBytesShared();
	}
	return x;
}*/

void NmdcHub::refreshUserList(bool unknownOnly /* = false */) {
	if(unknownOnly) {
		Lock l(cs);
		for(NickIter i = users.begin(); i != users.end(); ++i) {
			if(!i->second->getIdentity().isSet(Identity::GOT_INF)) {
				getInfo(*i->second);
			}
		}
	} else {
		clearUsers();
		getNickList();
	}
}

OnlineUser& NmdcHub::getUser(const string& aNick) {
	OnlineUser* u = NULL;
	{
		Lock l(cs);

		NickIter i = users.find(aNick);
		if(i != users.end())
			return *i->second;

		User::Ptr p = ClientManager::getInstance()->getUser(aNick, getHubUrl());

		u = users.insert(make_pair(aNick, new OnlineUser(p, *this))).first->second;
		u->getIdentity().setNick(aNick);
	}

	ClientManager::getInstance()->putOnline(*u);
	return *u;
}

OnlineUser* NmdcHub::findUser(const string& aNick) {
	Lock l(cs);
	NickIter i = users.find(aNick);
	return i == users.end() ? NULL : i->second;
}

void NmdcHub::putUser(const string& aNick) {
	OnlineUser* u = NULL;
	{
		Lock l(cs);
		NickIter i = users.find(aNick);
		if(i == users.end()) {
			dcassert(0);
			return;
		}
		u = i->second;
		users.erase(i);
	}
	ClientManager::getInstance()->putOffline(*u);
	delete u;
}

void NmdcHub::clearUsers() {
	NickMap u2;
	
	{
		Lock l(cs);
		u2 = users;
		users.clear();
	}
	
	for(NickIter i = u2.begin(); i != u2.end(); ++i) {
		ClientManager::getInstance()->putOffline(*i->second);
		delete i->second;
	}
}

void NmdcHub::updateFromTag(Identity& id, const string& tag) {
	StringTokenizer<string> tok(tag, ',');
	string::size_type j;
	for(StringIter i = tok.getTokens().begin(); i != tok.getTokens().end(); ++i) {
		if(i->length() < 2)
			continue;

		if((j = i->find("M:")) != string::npos) {
			i->erase(i->begin() + j, i->begin() + j + 2);
			if((*i == "P") || (*i == "5")) {
				User::Ptr u = id.getUser();
				u->setFlag(User::PASSIVE);
			}
		} else if(i->compare(0, 2, "H:") == 0) {
			StringTokenizer<string> t(i->substr(2), '/');
			if(t.getTokens().size() != 3)
				continue;
			id.set("HN", t.getTokens()[0]);
			id.set("HR", t.getTokens()[1]);
			id.set("HO", t.getTokens()[2]);
		} else if(i->compare(0, 2, "S:") == 0) {
			id.set("SL", i->substr(2));
		} else if((j = i->find("V:")) != string::npos) {
			i->erase(i->begin() + j, i->begin() + j + 2);
			id.set("VE", *i);
		} else if((j = i->find("L:")) != string::npos) {
			i->erase(i->begin() + j, i->begin() + j + 2);
			id.set("US", Util::toString(Util::toInt64(*i)*1024));
		}
	}
}

void NmdcHub::onLine(const char* aLine) throw() {
	char *temp, *temp1;
	updateActivity();
	
	if(aLine[0] != '$') {
		if ((BOOLSETTING(SUPPRESS_MAIN_CHAT)) && (!isOp())) {
			return;
		}

		// Check if we're being banned...
		if(state != STATE_CONNECTED) {
			if(strstr(aLine, "banned") != 0) {
				reconnect = false;
			}
		}

        string nick = Util::emptyString;

		switch(aLine[0]) {
            case '<':
                if((temp = (char*)strchr(aLine+1, '>')) != NULL) {
                    temp[0] = NULL;
                    nick = fromNmdc(aLine+1);
                    if(temp[1] == ' ' && temp[2] == '/' && tolower(temp[3]) == 'm' && tolower(temp[4]) == 'e' && temp[5] == ' ' && temp[6] != NULL) {
                        char *chatline = (char *)aLine;
                        for(int iNickLen = (temp - aLine); iNickLen > 1; iNickLen--) {
                            chatline[iNickLen+4] = aLine[iNickLen-1];
                        }
						chatline[5] = ' ';
						chatline[4] = '*';
						aLine += 4;
                    } else {
                        temp[0] = '>';
                    }
                }
                break;
            case '*':
                if((temp = (char*)strchr(aLine+2, ' ')) != NULL) {
                    temp[0] = NULL;
                    nick = fromNmdc(aLine+2);
                    temp[0] = ' ';
                }
                break;
            default:
                break;
        }

        OnlineUser* u = NULL;
        if(nick != Util::emptyString) {
        	u = findUser(nick);
        }

		fire(ClientListener::Message(), this, u, Util::validateMessage(fromNmdc(aLine), true).c_str());
		return;
	}

	switch(aLine[1]) {
        case 'S':
	    	// $Search
    		if(strncmp(aLine+2, "earch ", 6) == 0) {
				aLine += 8;
        		if(state != STATE_CONNECTED || (temp = strchr((char*)aLine, ' ')) == NULL || temp[1] == NULL) return;

				temp[0] = NULL; temp += 1;
        		if(aLine[0] == NULL || temp[1] == NULL || temp[2] == NULL || temp[3] == NULL || temp[4] == NULL) return;

				string seeker = fromNmdc(aLine);
        		int iseekerLen = (temp-aLine)-1;
        		
				bool bPassive = (_strnicmp(seeker.c_str(), "Hub:", 4) == 0);
				bool isActive = ClientManager::getInstance()->isActive(this);

				// We don't wan't to answer passive searches if we're in passive mode...
				if((bPassive == true) && !isActive) {
					return;
				}
				// Filter own searches
				if(isActive && bPassive == false) {
					if((strcmp(seeker.c_str(), (getLocalIp() + ":" + Util::toString(SearchManager::getInstance()->getPort())).c_str())) == 0) {
						return;
					}
				} else {
					if(strcmp(seeker.c_str() + 4, getMyNick().c_str()) == 0) {
						return;
					}
				}

				{
					Lock l(cs);
					u_int32_t tick = GET_TICK();
					seekers.push_back(make_pair(seeker, tick));
					// First, check if it's a flooder
        			for(FloodIter fi = flooders.begin(); fi != flooders.end(); ++fi) {
						if(fi->first == seeker)
							return;
					}	
					int count = 0;
        			for(FloodIter fi = seekers.begin(); fi != seekers.end(); ++fi) {
						if(fi->first == seeker)
							count++;
	
						if(count > 7) {
					    	// BFU don't need to know search spammers ;)
						    if(isOp()) {
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
        		if((temp1 = strchr(temp, '?')) == NULL || temp1[1] == NULL || temp1[2] == NULL) return;

				temp1[0] = NULL; temp1[2] = NULL;
        		if(temp[0] == NULL) return;
	
				int64_t size = _atoi64(temp);
				int type = atoi(temp1+1) - 1; temp1 += 3;
				if(temp1[0] != NULL) {
					if(bPassive == true && iseekerLen > 4) {
						OnlineUser* u = findUser(seeker.substr(4));

						if(u == NULL) {
							return;
						}

						if(!u->getUser()->isSet(User::PASSIVE)) {
							u->getUser()->setFlag(User::PASSIVE);
							updated(*u);
						}
					}
					fire(ClientListener::NmdcSearch(), this, seeker, a, size, type, fromNmdc(temp1), bPassive);
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
        		if(aLine[0] == NULL) {
					validateNick(getMyNick());
					return;
				}

        		while(aLine != NULL) {
                    if((temp = (char*)strchr(aLine, ' ')) != NULL) {
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
						case 'Z':
							if((strcmp(aLine+1, "Lines")) == 0) {
								supportFlags |= SUPPORTS_ZLINE;
							}
							break;

                		default:
                            dcdebug("NmdcHub::onLine Unknown supports %s\n", aLine);
                            break;
                    }
        			aLine = temp;
				}
				if(!(getSupportFlags() & SUPPORTS_QUICKLIST)) {
					validateNick(getMyNick());
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

				if(aLine[0] == NULL)
		    	     return;

				Description = strchr((char*)aLine, ' ');
        	    if(Description == NULL || Description[0] == NULL || Description[1] == NULL)
					return;

				Description[0] = NULL;
				Description += 1;
	    
				OnlineUser& u = getUser(fromNmdc(aLine));

			    Connection = strchr(Description, '$');
	    		if(Connection && Connection+1 && Connection+2 && Connection+3) {
					Connection[0] = NULL;
					Connection += 1;
					if(Description) {
						if(*(Connection-2) == '>') {
							if((Tag = strrchr(Description, '<')) != NULL) {
								u.getIdentity().setTag(fromNmdc(Tag));
								Tag[strlen(Tag)-1] = NULL;
								updateFromTag(u.getIdentity(), Tag+1);
								Tag[0] = NULL;
							}
						}
						u.getIdentity().setDescription(Util::validateMessage(fromNmdc(Description), true));
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
							u.getIdentity().setEmail(Util::validateMessage(fromNmdc(Email), true));
							u.getIdentity().setBytesShared(Share);
						}
						Connection[1] = NULL; 
						Connection += 2;
						char status = *(Email-2);
						*(Email-2) = NULL;
       					switch(status) {
                            case 1:
                                u.getUser()->unsetFlag(User::AWAY);
      					        u.getUser()->unsetFlag(User::SERVER);
       					        u.getUser()->unsetFlag(User::FIREBALL);
                                break;
                            case 2:
                            case 3:
								u.getUser()->setFlag(User::AWAY);
								u.getUser()->unsetFlag(User::SERVER);
								u.getUser()->unsetFlag(User::FIREBALL);
           						break;
       					    case 4:
                            case 5:
								u.getUser()->setFlag(User::SERVER);
								u.getUser()->unsetFlag(User::AWAY);
								u.getUser()->unsetFlag(User::FIREBALL);
           						break;
       					    case 6:
                            case 7:
								u.getUser()->setFlag(User::SERVER);
								u.getUser()->setFlag(User::AWAY);
								u.getUser()->unsetFlag(User::FIREBALL);
           						break;
           					case 8:
                            case 9:
								u.getUser()->setFlag(User::FIREBALL);
								u.getUser()->unsetFlag(User::AWAY);
								u.getUser()->unsetFlag(User::SERVER);
           						break;
           					case 10:
                            case 11:
								u.getUser()->setFlag(User::FIREBALL);
								u.getUser()->setFlag(User::AWAY);
								u.getUser()->unsetFlag(User::SERVER);
           						break;
           					default:
                                // WTF ?!? ok go to default...
								u.getUser()->unsetFlag(User::AWAY);
								u.getUser()->unsetFlag(User::SERVER);
								u.getUser()->unsetFlag(User::FIREBALL);
                                break;
						}
						u.setStatus(status);
						u.getIdentity().setConnection(fromNmdc(Connection));
					}
				}

				if(state == STATE_MYINFO) {
					if(_stricmp(u.getUser()->getFirstNick().c_str(), getMyNick().c_str()) == 0) {
						state = STATE_CONNECTED;
						updateCounts(false);
						u.getUser()->setFlag(User::DCPLUSPLUS);
						if(!ClientManager::getInstance()->isActive(this))
							u.getUser()->setFlag(User::PASSIVE);
						else
							u.getUser()->unsetFlag(User::PASSIVE);
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
        		if(aLine[0] == NULL)
					return;

				string nick = fromNmdc(aLine);
	
				OnlineUser* u = findUser(nick);
				if(u == NULL)
					return;

				fire(ClientListener::UserRemoved(), this, *u);

				putUser(nick);
    			return;
        	}
            
            dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
            return;
        case 'C':
			// $ConnectToMe
			if(strncmp(aLine+2, "onnectToMe ", 11) == 0) {
				aLine += 13;
        		if(state != STATE_CONNECTED || (temp = strchr((char*)aLine, ' ')) == NULL || temp[1] == NULL) return;
	
				temp[0] = NULL; temp += 1;
        		if(aLine[0] == NULL || temp[0] == NULL) return;

				if(_stricmp(fromNmdc(aLine).c_str(), getMyNick().c_str()) != 0) // Check nick... is CTM really for me ? ;o)
					return;
	
        		if((temp1 = strchr(temp, ':')) == NULL || temp1[1] == NULL) return;

				temp1[0] = NULL; temp1 += 1;
        		if(temp[0] == NULL || temp1[0] == NULL) return;

				ConnectionManager::getInstance()->nmdcConnect(temp, (short)atoi(temp1), getMyNick(), getHubUrl()); 
	    		return;
    	    }
            
            dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
            return;
        case 'R':
    		// $RevConnectToMe
    		if(strncmp(aLine+2, "evConnectToMe ", 14) == 0) {
				if(state != STATE_CONNECTED) return;

				aLine += 16;
        		if((temp = strchr((char*)aLine, ' ')) == NULL || temp[1] == NULL) return;

				temp[0] = NULL; temp += 1;
        		if(aLine[0] == NULL || temp[0] == NULL) return;

				OnlineUser* u = findUser(fromNmdc(aLine));
				if(u == NULL)
					return;

				if(ClientManager::getInstance()->isActive(this)) {
					connectToMe(*u);	
				} else {
					if(!u->getUser()->isSet(User::PASSIVE)) {
						u->getUser()->setFlag(User::PASSIVE);
						// Notify the user that we're passive too...
						revConnectToMe(*u);
						updated(*u);
					}
				}
    			return;
	        }
            
            dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
            return;
        case 'H':
	    	// $HubName
    		if(strncmp(aLine+2, "ubName ", 7) == 0) {
				aLine += 9;
        		if(aLine[0] == NULL) return;
	
				getHubIdentity().setNick(fromNmdc(aLine));
				getHubIdentity().setDescription(Util::emptyString);			

				fire(ClientListener::HubUpdated(), this);
				return;
			}
        
        	// $Hello
       		if(strncmp(aLine+2, "ello ", 5) == 0) {
				if(getSupportFlags() & SUPPORTS_QUICKLIST) return;
				aLine += 7;
        		if(aLine[0] == NULL) return;
	
     			string nick = fromNmdc(aLine);
				OnlineUser& u = getUser(nick);
        		
        		if(_stricmp(getMyNick().c_str(), nick.c_str()) == NULL) {
					u.getUser()->setFlag(User::DCPLUSPLUS);
					if(ClientManager::getInstance()->isActive(this))
						u.getUser()->unsetFlag(User::PASSIVE);
					else
						u.getUser()->setFlag(User::PASSIVE);
				}

				if(state == STATE_HELLO) {
					state = STATE_CONNECTED;
					updateCounts(false);

					version();
					if((YnHub == true) || (getStealth() == true)) {
						getNickList();
						myInfo();
	            	} else {
						myInfo();
						getNickList();
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
        		if(temp[0] == NULL || temp[1] == NULL) return;

				temp[1] = NULL;
				int type = atoi(temp);
				temp += 2;
        		if(temp[0] == NULL || temp[1] == NULL) return;

				temp[1] = NULL;
				int ctx = atoi(temp);
				temp += 2;
        		if(temp[0] == NULL) return;

        		switch(type) {
                    case UserCommand::TYPE_SEPARATOR:
                    case UserCommand::TYPE_CLEAR:
        			    fire(ClientListener::UserCommand(), this, type, ctx, Util::emptyString, Util::emptyString);
        			    break;
            		case UserCommand::TYPE_RAW:
                    case UserCommand::TYPE_RAW_ONCE:
        			    if((temp1 = strchr(temp, '$')) == NULL || temp1[1] == NULL) return;

						temp1[0] = NULL; temp1 += 1;
        			    if(temp[0] == NULL || temp1[0] == NULL) return;

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
				OnlineUser::List v;
				StringList l;
				aLine += 8;
        		if(aLine[0] == NULL) return;

				while((temp = (char*)strchr(aLine, ' ')) != NULL && temp+1 != NULL) {
					temp[0] = NULL; temp += 1; temp1 = strchr(temp, '$');
					if(aLine[0] == NULL || temp[0] == NULL) break;
					if(temp1 == NULL) {
						OnlineUser* u = findUser(fromNmdc(aLine));
				
						if(u == NULL)
							continue;

						u->getIdentity().setIp(temp);
						v.push_back(u);
						break;
					}

					temp1[0] = NULL;
					if(temp[0] == NULL) break;
			
					OnlineUser* u = findUser(fromNmdc(aLine));
				
					if(u == NULL)
						continue;

					u->getIdentity().setIp(temp);
					v.push_back(u);

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
        		if(state != STATE_LOCK || aLine[0] == NULL) return;

				state = STATE_HELLO;
				if((temp = (char*)strstr(aLine, " Pk=")) != NULL && temp[1] != NULL) {
					temp[0] = NULL; temp += 1;
				}
	
        		if(aLine[0] == NULL) return;

        		if(temp[0] != NULL) {
    				if(_stricmp(temp+3, "YnHub") == 0) {
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

					if (getStealth() == false) {
						feat.push_back("QuickList");
						feat.push_back("ZLine");
					}

					if(BOOLSETTING(COMPRESS_TRANSFERS))
						feat.push_back("GetZBlock");

					supports(feat);
    				if(PtokaX == false) key(CryptoManager::getInstance()->makeKey(aLine));
				} else {
					key(CryptoManager::getInstance()->makeKey(aLine));
					validateNick(getMyNick());
				}
        		return;
	        }
          	dcdebug("NmdcHub::onLine Unknown command %s\n", aLine);
            return;
    	case 'F':
	    	if(strncmp(aLine+2, "orceMove ", 9) == 0) {
				aLine += 11;
				if(aLine[0] == NULL) return;

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
				OnlineUser::List v;
				aLine += 10;
				if(aLine[0] == NULL) return;

				while((temp = (char*)strchr(aLine, '$')) != NULL) {
					temp[0] = NULL;
					if(aLine[0] == NULL) break;

					v.push_back(&getUser(fromNmdc(aLine)));
					aLine = temp+2;
				}
		
				if(!(getSupportFlags() & SUPPORTS_NOGETINFO)) {
					string tmp;
					// Let's assume 10 characters per nick...
					tmp.reserve(v.size() * (11 + 10 + getMyNick().length())); 
					string n = ' ' +  toNmdc(getMyNick()) + '|';
					for(OnlineUser::List::const_iterator i = v.begin(); i != v.end(); ++i) {
						tmp += "$GetINFO ";
						tmp += toNmdc((*i)->getIdentity().getNick());
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
				OnlineUser::List v;
				aLine += 8;
				if(aLine[0] == NULL) return;

				while((temp = (char*)strchr(aLine, '$')) != NULL) {
					temp[0] = NULL;
					if(aLine[0] == NULL) break;

					OnlineUser& ou = getUser(fromNmdc(aLine));
					ou.getIdentity().setOp(true);

					if(ou.getIdentity().getNick() == getMyIdentity().getNick())
						getMyIdentity().setOp(true);

					v.push_back(&ou);
					aLine = temp+2;
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
				if((temp1 = (char*)strstr(aLine+5, "From:")) != NULL && strlen(temp1) > 6) {
					if((temp = strchr(temp1+6, '$')) == NULL || temp[1] == NULL) return;
	
					temp1 += 6; *(temp-1) = NULL; temp += 1;
        			if(temp1[0] == NULL || temp[0] == NULL) return;

					OnlineUser* ou = findUser(fromNmdc(temp1));
					if(ou == NULL) {
						// @todo Route anonymous message to the ui
					} else {
						fire(ClientListener::PrivateMessage(), this, *ou, Util::validateMessage(fromNmdc(temp), true));
					}
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
		case 'Z':
			// $Zline
        	if(strncmp(aLine+2, "Line ", 5) == 0) { 
				aLine += 7;
				if(aLine[0] == NULL) return;

				string::size_type i = 0, j;
				string rawParam = (char*)aLine;
				bool corrupt = false;

				// unescape \\ to \ and \P to | 
				while((i = rawParam.find("\\", i)) != string::npos && !corrupt) {
					if (i + 1 < rawParam.size()) {
						switch (rawParam[i+1]) {
							case '\\':
								rawParam.replace(i++, 2, "\\");
								break;
							case 'P':
								rawParam.replace(i++, 2, "|");
								break;
							default:
								corrupt = true;
								break;
						}
					} else {
						corrupt = true;
					}
				}

				if (!corrupt) {
					// unzip the ZBlock
					i = rawParam.size();
					j = i * 10;
					AutoArray<u_int8_t> temp2(j);
					UnZFilter filter;
					try {
						filter(rawParam.c_str(), i, temp2, j);
					} catch(...){
						dcdebug("Error during Zline decompression\n");
					}
					string lines = string((char*)(u_int8_t*)temp2, j);

					// "fire" the lines
					COMMAND_DEBUG("\r\n", DebugManager::HUB_IN, "");
					StringTokenizer<string> st(lines, '|');
					for(StringList::iterator i = st.getTokens().begin(); i != st.getTokens().end(); ++i) {
						COMMAND_DEBUG("$Zline: " + (*i), DebugManager::HUB_IN, getIpPort());
						onLine((*i).c_str());
					}
				} else {
					dcdebug("Corrupt Zline datastream\n");
				}
			}

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

void NmdcHub::connectToMe(const OnlineUser& aUser) {
	checkstate(); 
	dcdebug("NmdcHub::connectToMe %s\n", aUser.getIdentity().getNick().c_str());
	ConnectionManager::getInstance()->nmdcExpect(aUser.getIdentity().getNick(), getMyNick(), getHubUrl());
	send("$ConnectToMe " + toNmdc(aUser.getIdentity().getNick()) + " " + getLocalIp() + ":" + Util::toString(ConnectionManager::getInstance()->getPort()) + "|");
	ConnectionManager::iConnToMeCount++;
}

void NmdcHub::revConnectToMe(const OnlineUser& aUser) {
	checkstate(); 
	dcdebug("NmdcHub::revConnectToMe %s\n", aUser.getIdentity().getNick().c_str());
	send("$RevConnectToMe " + toNmdc(getMyNick()) + " " + toNmdc(aUser.getIdentity().getNick()) + "|");
}

void NmdcHub::myInfo() {
	if(state != STATE_CONNECTED && state != STATE_MYINFO) {
		return;
	}
	
	dcdebug("MyInfo %s...\n", getMyNick().c_str());
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
	string version = DCVERSIONSTRING;	
	if (getStealth() == false) {
		tmp0 = "<StrgDC++";
#ifdef isCVS
		version = VERSIONSTRING STRONGDCVERSIONSTRING CVSVERSION;
#else
		version = VERSIONSTRING STRONGDCVERSIONSTRING;
#endif
	}

	if (SETTING(THROTTLE_ENABLE) && SETTING(MAX_UPLOAD_SPEED_LIMIT) != 0) {
		sprintf(tag, "%s%s%s%s%c%s%s%s%s%s%s>", tmp0.c_str(), tmp1.c_str(), version.c_str(), tmp2.c_str(), modeChar, tmp3.c_str(), 
		getCounts().c_str(), tmp4.c_str(), Util::toString(UploadManager::getInstance()->getSlots()).c_str(), tmp5.c_str(), Util::toString(SETTING(MAX_UPLOAD_SPEED_LIMIT)).c_str());
	} else {
		sprintf(tag, "%s%s%s%s%c%s%s%s%s>", tmp0.c_str(), tmp1.c_str(), version.c_str(), tmp2.c_str(), modeChar, tmp3.c_str(), 
		getCounts().c_str(), tmp4.c_str(), Util::toString(UploadManager::getInstance()->getSlots()).c_str());
	}

	int NetLimit = Util::getNetLimiterLimit();
	string connection = (NetLimit > -1) ? "NetLimiter [" + Util::toString(NetLimit) + " kB/s]" : SETTING(CONNECTION);

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

	char myinfo[512];
	sprintf(myinfo, "$MyINFO $ALL %s %s%s$ $%s%c$%s$", toNmdc(getMyNick()).c_str(),
		toNmdc(Util::validateMessage(getMyIdentity().getDescription(), false)).c_str(), tag, connection.c_str(), StatusMode, 
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
		chars = sprintf(buf, "$Search %s:%d %c?%c?%I64d?%d?%s|", x.c_str(), (int)SearchManager::getInstance()->getPort(), c1, c2, aSize, aFileType+1, tmp.c_str());
	} else {
		buf = new char[getMyNick().length() + aString.length() + 64];
		chars = sprintf(buf, "$Search Hub:%s %c?%c?%I64d?%d?%s|", toNmdc(getMyNick()).c_str(), c1, c2, aSize, aFileType+1, tmp.c_str());
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
