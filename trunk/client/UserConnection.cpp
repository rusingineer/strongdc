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
		} else
			goto again;
	}
	char *temp;
	if(strncmp(aLine+1, "MyNick ", 7) == 0) {
		aLine += 8;
		if(aLine+8 != NULL)
			fire(UserConnectionListener::MyNick(), this, Text::acpToUtf8(aLine));
	} else if(strncmp(aLine+1, "Direction ", 10) == 0) {
		aLine += 11;
		if(aLine == NULL) return;

		if((temp = strchr(aLine, ' ')) != NULL && temp+1 != NULL) {
			temp[0] = NULL, temp += 1;
			if(aLine == NULL || temp == NULL) return;

			fire(UserConnectionListener::Direction(), this, aLine, temp);
		}
	} else if(strncmp(aLine+1, "Error ", 6) == 0) {
		aLine += 7;
		if(aLine == NULL) return;

		if(stricmp(aLine, FILE_NOT_AVAILABLE.c_str()) == 0 || 
			strstr(aLine, /*path/file*/" no more exists") != 0) {
			fire(UserConnectionListener::FileNotAvailable(), this);
		} else {
			Lock l(ConnectionManager::getInstance()->cs_deadlock_fix);
			fire(UserConnectionListener::Failed(), this, aLine);
		}
	} else if(strncmp(aLine+1, "FileLength ", 11) == 0) {
		aLine += 12;
		if(aLine != NULL) {
			fire(UserConnectionListener::FileLength(), this, _atoi64(aLine));
		}
	} else if(strcmp(aLine+1, "GetListLen") == 0) {
		fire(UserConnectionListener::GetListLength(), this);
	} else if(strncmp(aLine+1, "Get ", 4) == 0) {
		aLine += 5;
		if(aLine == NULL) return;

		if((temp = strchr(aLine, '$')) != NULL && temp+1 != NULL)  {
			temp[0] = NULL; temp += 1;
			if(aLine == NULL || temp == NULL) return;

			fire(UserConnectionListener::Get(), this, Text::acpToUtf8(aLine), _atoi64(temp) - (int64_t)1);
		}
	} else if(strncmp(aLine+1, "GetZBlock ", 10) == 0) {
		aLine += 11;
		if(aLine != NULL) {
			processBlock(aLine, 1);
		}
	} else if(strncmp(aLine+1, "UGetZBlock ", 11) == 0) {
		aLine += 12;
		if(aLine != NULL) {
			processBlock(aLine, 2);
		}
	} else if(strncmp(aLine+1, "UGetBlock ", 10) == 0) {
		aLine += 11;
		if(aLine != NULL) {
			processBlock(aLine, 3);
		}
	} else if(strncmp(aLine+1, "Key ", 4) == 0) {
		aLine += 5;
		if(aLine != NULL) {
			fire(UserConnectionListener::Key(), this, aLine);
		}
	} else if(strncmp(aLine+1, "Lock ", 5) == 0) {
		aLine += 6;
		if(aLine == NULL) return;

		if((temp = strchr(aLine, ' ')) != NULL && temp+4 != NULL) {
			temp[0] = NULL; temp += 4;
			if(aLine == NULL || temp == NULL) return; 

			fire(UserConnectionListener::CLock(), this, aLine, temp);
		} else {
			fire(UserConnectionListener::CLock(), this, aLine, Util::emptyString);
		}
	} else if(strcmp(aLine+1, "Send") == 0) {
		fire(UserConnectionListener::Send(), this);
	} else if(strncmp(aLine+1, "Sending ", 8) == 0) {
		aLine += 9;
		if(aLine != NULL) {
			fire(UserConnectionListener::Sending(), this, _atoi64(aLine));
		}
	} else if(strcmp(aLine+1, "MaxedOut") == 0) {
		fire(UserConnectionListener::MaxedOut(), this);
	} else if(strncmp(aLine+1, "Supports ", 9) == 0) {
		aLine += 10;
		if(aLine != NULL) {
			fire(UserConnectionListener::Supports(), this, StringTokenizer<string>(aLine, ' ').getTokens());
		}
	} else if(strncmp(aLine+1, "ListLen ", 8) == 0) {
		aLine += 9;
		if(aLine != NULL) {
			fire(UserConnectionListener::ListLength(), this, aLine);
		}
	} else if(strncmp(aLine+1, "ADC", 3) == 0) {
		dispatch(aLine, true);
	} else {
		if(getUser())
			getUser()->setUnknownCommand(aLine);
		dcdebug("Unknown NMDC command: %.50s\n", aLine);
	}
}

void UserConnection::on(BufferedSocketListener::Failed, const string& aLine) throw() {
	setState(STATE_UNCONNECTED);
	Lock l(ConnectionManager::getInstance()->cs_deadlock_fix);
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
