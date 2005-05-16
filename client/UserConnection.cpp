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
#include "UserConnection.h"
#include "StringTokenizer.h"
#include "AdcCommand.h"
#include "DebugManager.h"
#include "ConnectionManager.h"

const string UserConnection::FEATURE_GET_ZBLOCK = "GetZBlock";
const string UserConnection::FEATURE_MINISLOTS = "MiniSlots";
const string UserConnection::FEATURE_XML_BZLIST = "XmlBZList";
const string UserConnection::FEATURE_ADCGET = "ADCGet";
const string UserConnection::FEATURE_ZLIB_GET = "ZLIG";
const string UserConnection::FEATURE_TTHL = "TTHL";
const string UserConnection::FEATURE_TTHF = "TTHF";

const string UserConnection::FILE_NOT_AVAILABLE = "File Not Available";

const string UserConnection::UPLOAD = "Upload";
const string UserConnection::DOWNLOAD = "Download";

void Transfer::updateRunningAverage() {
	u_int32_t tick = GET_TICK();
	if(tick > lastTick) {
		u_int32_t diff = tick - lastTick;
		int64_t tot = getTotal();
		if(diff == 0) {
			// No time passed, don't update runningAverage;
		} else if( ((tick - getStart()) < AVG_PERIOD) ) {
			runningAverage = getAverageSpeed();
		} else {
			int64_t bdiff = tot - last;
			int64_t avg = bdiff * (int64_t)1000 / diff;
			if(diff > AVG_PERIOD) {
				runningAverage = avg;
			} else {
				// Weighted average...
				runningAverage = ((avg * diff) + (runningAverage*(AVG_PERIOD-diff)))/AVG_PERIOD;
			}
		}
		last = tot;
	}
	lastTick = tick;
}

void UserConnection::onLine(const char* aLine) throw() {
again:
	if(aLine[0] == 'C' && !isSet(FLAG_NMDC)) {
		dispatch(aLine);
		return;
	} else if(aLine[0] == '$') {
		setFlag(FLAG_NMDC);
	} else {
		// We shouldn't be here?
		dcdebug("Unknown UserConnection command: %.50s\n", aLine);
		if(getUser())
			getUser()->setUnknownCommand(aLine);
		if((aLine = strstr(aLine, "$")) == NULL) {
			return;
		} else {
			dcassert(0);
			goto again;
		}
	}
	char *temp;
	if(aLine[1] == 'M') {
    	// $MyNick
    	if(strncmp(aLine+2, "yNick ", 6) == 0) {
    		aLine += 8;
    		if(aLine+8 != NULL) {
    			fire(UserConnectionListener::MyNick(), this, Text::acpToUtf8(aLine));
            }
            return;
       }
       
    	if(strcmp(aLine+2, "axedOut") == 0) {
    		fire(UserConnectionListener::MaxedOut(), this);
    		return;
        }
	} else if(aLine[1] == 'D') {
        // $Direction
        if(strncmp(aLine+2, "irection ", 9) == 0) {
    		aLine += 11;
    		if(aLine == NULL) return;
    
    		if((temp = strchr(aLine, ' ')) != NULL && temp+1 != NULL) {
    			temp[0] = NULL, temp += 1;
    			if(aLine == NULL || temp == NULL) return;
    
    			fire(UserConnectionListener::Direction(), this, aLine, temp);
    		}
    		return;
        }
	} else if(aLine[1] == 'E') {
		// $Error
        if(strncmp(aLine+2, "rror ", 5) == 0) {
    		aLine += 7;
    		if(aLine == NULL) return;
    
    		if(stricmp(aLine, FILE_NOT_AVAILABLE.c_str()) == 0 || 
    			strstr(aLine, /*path/file*/" no more exists") != 0) {
    			fire(UserConnectionListener::FileNotAvailable(), this);
    		} else {
    			fire(UserConnectionListener::Failed(), this, aLine);
    		}
    		return;
        }
	} else if(aLine[1] == 'F') {
		// $FileLength
        if(strncmp(aLine+2, "ileLength ", 10) == 0) {
    		aLine += 12;
    		if(aLine != NULL) {
    			fire(UserConnectionListener::FileLength(), this, _atoi64(aLine));
    		}
    		return;
        }
	} else if(aLine[1] == 'G') {
        // $Get
    	if(strncmp(aLine+2, "et ", 3) == 0) {
    		aLine += 5;
    		if(aLine == NULL) return;
    
    		if((temp = strchr(aLine, '$')) != NULL && temp+1 != NULL)  {
    			temp[0] = NULL; temp += 1;
    			if(aLine == NULL || temp == NULL) return;
    
    			fire(UserConnectionListener::Get(), this, Text::acpToUtf8(aLine), _atoi64(temp) - (int64_t)1);
    		}
    		return;
        }
        
        // $GetListLen
        if(strcmp(aLine+2, "etListLen") == 0) {
    		fire(UserConnectionListener::GetListLength(), this);
    		return;
        }
        
        // GetZBlock
    	if(strncmp(aLine+2, "etZBlock ", 9) == 0) {
    		aLine += 11;
    		if(aLine != NULL) {
    			processBlock(aLine, 1);
    		}
    		return;
        }
	} else if(aLine[1] == 'U') {
        // $UGetZBLock
        if(strncmp(aLine+2, "GetZBlock ", 10) == 0) {
    		aLine += 12;
    		if(aLine != NULL) {
    			processBlock(aLine, 2);
    		}
    		return;
        }
        
        // $UGetBlock
    	if(strncmp(aLine+2, "GetBlock ", 9) == 0) {
    		aLine += 11;
    		if(aLine != NULL) {
    			processBlock(aLine, 3);
    		}
    		return;
        }
	} else if(aLine[1] == 'K') {
        // $Key
        if(strncmp(aLine+2, "ey ", 3) == 0) {
    		aLine += 5;
    		if(aLine != NULL) {
    			fire(UserConnectionListener::Key(), this, aLine);
    		}
    		return;
        }
	} else if(aLine[1] == 'L') {
        // $Lock
        if(strncmp(aLine+2, "ock ", 4) == 0) {
    		aLine += 6;
    		if(aLine == NULL) return;
     
    		if((temp = strchr(aLine, ' ')) != NULL && temp+4 != NULL) {
    			temp[0] = NULL; temp += 4;
    			if(aLine == NULL || temp == NULL) return; 
                
    			fire(UserConnectionListener::CLock(), this, aLine, temp);
    		} else {
    			fire(UserConnectionListener::CLock(), this, aLine, Util::emptyString);
    		}
    		return;
        }

		// $ListLen
		if(strncmp(aLine+2, "istLen ", 7) == 0) {	                 	
			aLine += 9;	 	
            if(aLine != NULL) {	 	
				fire(UserConnectionListener::ListLength(), this, aLine);	
			}
			return;
        }
	} else if(aLine[1] == 'S') {
        // $Send
        if(strcmp(aLine+2, "end") == 0) {
    		fire(UserConnectionListener::Send(), this);
    		return;
        }
        
        // $Sending
    	if(strncmp(aLine+2, "ending ", 7) == 0) {
    		aLine += 9;
    		if(aLine != NULL) {
    			fire(UserConnectionListener::Sending(), this, _atoi64(aLine));
    		}
    		return;
        }
        
        // $Supports
    	if(strncmp(aLine+2, "upports ", 8) == 0) {
    		aLine += 10;
    		if(aLine != NULL) {
    			fire(UserConnectionListener::Supports(), this, StringTokenizer<string>(aLine, ' ').getTokens());
    		}
    		return;
        }
	} else if(aLine[1] == 'A') {
        if(strncmp(aLine+2, "DC", 2) == 0) {
    		dispatch(aLine, true);
    		return;
        }
	} else {
		if(getUser())	 	
			getUser()->setUnknownCommand(aLine);
		dcdebug("Unknown NMDC command: %.50s\n", aLine);
		return;
	}
}

void UserConnection::on(BufferedSocketListener::Failed, const string& aLine) throw() {
	setState(STATE_UNCONNECTED);
	fire(UserConnectionListener::Failed(), this, aLine);
}

void UserConnection::processBlock(const char* param, int type) throw() {
	char *temp, *temp1;
	if((temp = strchr(param, ' ')) == NULL || temp+1 == NULL) return;

	temp[0] = NULL; temp += 1;
	if(param == NULL || temp == NULL) return;

	int64_t start = _atoi64(param);
	if(start < 0) {
		disconnect();
		return;
	}

	if((temp1 = strchr(temp, ' ')) == NULL || temp1+1 == NULL) return;

	temp1[0] = NULL; temp1 += 1;
	if(temp == NULL || temp1 == NULL) return;

	int64_t bytes = _atoi64(temp);

	string name = temp1;
	if(type == 2 || type == 3)
	Text::acpToUtf8(name);
	if(type == 3) {
		fire(UserConnectionListener::GetBlock(), this, name, start, bytes);
	} else {
		fire(UserConnectionListener::GetZBlock(), this, name, start, bytes);
	}
}

/**
 * @file
 * $Id$
 */
