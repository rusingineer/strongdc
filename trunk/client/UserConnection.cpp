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

const string UserConnection::FEATURE_GET_ZBLOCK = "GetZBlock";
const string UserConnection::FEATURE_MINISLOTS = "MiniSlots";
const string UserConnection::FEATURE_XML_BZLIST = "XmlBZList";
const string UserConnection::FEATURE_ADCGET = "ADCGet";
const string UserConnection::FEATURE_ZLIB_GET = "ZLIG";
const string UserConnection::FEATURE_TTHL = "TTHL";
const string UserConnection::FEATURE_TTHF = "TTHF";

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
	COMMAND_DEBUG(aLine, DebugManager::CLIENT_IN, getRemoteIp());

	if(aLine[0] != '$') {
		dcdebug("Unknown UserConnection command: %.50s\n", aLine);
		return;
	}
	char *temp;
	if(strncmp(aLine+1, "MyNick ", 7) == 0) {
		if((temp = strtok((char*)aLine+8, "\0")) != NULL)
			fire(UserConnectionListener::MyNick(), this, Text::acpToUtf8(temp));
	} else if(strncmp(aLine+1, "Direction ", 10) == 0) {
		char *temp1 = strtok((char*)aLine+11, " ");
		if(temp1 != NULL && (temp = strtok(NULL, "\0")) != NULL) {
			fire(UserConnectionListener::Direction(), this, temp1, temp);
		}
	} else if(strncmp(aLine+1, "Error ", 6) == 0) {
		if((temp = strtok((char*)aLine+7, "\0")) == NULL)
			return;

		if(stricmp(temp, "File Not Available") == 0 || 
			strstr(temp, /*path/file*/" no more exists") == 0) {
			fire(UserConnectionListener::FileNotAvailable(), this);
		} else {
			fire(UserConnectionListener::Failed(), this, temp);
		}
	} else if(strncmp(aLine+1, "FileLength ", 11) == 0) {
		if((temp = strtok((char*)aLine+12, "\0")) != NULL) {
			int64_t size = _atoi64(temp);
			fire(UserConnectionListener::FileLength(), this, size);
		}
	} else if(strcmp(aLine+1, "GetListLen") == 0) {
		fire(UserConnectionListener::GetListLength(), this);
	} else if(strncmp(aLine+1, "Get ", 4) == 0) {
		char *temp1 = strtok((char*)aLine+5, "$");
		temp = strtok(NULL, "\0");
		if(temp != NULL && temp1 != NULL) {
			int64_t size = _atoi64(temp);
			fire(UserConnectionListener::Get(), this, Text::acpToUtf8(temp1), size - (int64_t)1);
		}
	} else if(strncmp(aLine+1, "GetZBlock ", 10) == 0) {
		if((temp = strtok((char*)aLine+11, "\0")) != NULL) {
			processBlock(temp, 1);
		}
	} else if(strncmp(aLine+1, "UGetZBlock ", 11) == 0) {
		if((temp = strtok((char*)aLine+12, "\0")) != NULL) {
			processBlock(temp, 2);
		}
	} else if(strncmp(aLine+1, "UGetBlock ", 10) == 0) {
		if((temp = strtok((char*)aLine+11, "\0")) != NULL) {
			processBlock(temp, 3);
		}
	} else if(strncmp(aLine+1, "Key ", 4) == 0) {
		if((temp = strtok((char*)aLine+5, "\0")) != NULL) {
			fire(UserConnectionListener::Key(), this, temp);
		}
	} else if(strncmp(aLine+1, "Lock ", 5) == 0) {
		char *lock;
		if((lock = strtok((char*)aLine+6, " ")) != NULL) {
			if((temp = strtok(((char*)aLine+6+strlen(lock)+4), "\0")) != NULL) {
				fire(UserConnectionListener::CLock(), this, lock, temp);
			}
		}
	} else if(strncmp(aLine+1, "Sending ", 8) == 0) {
		if((temp = strtok((char*)aLine+9, " ")) != NULL) {
			int64_t bytes = _atoi64(temp);
			fire(UserConnectionListener::Sending(), this, bytes);
		}
	} else if(strcmp(aLine+1, "Send") == 0) {
		fire(UserConnectionListener::Send(), this);
	} else if(strcmp(aLine+1, "MaxedOut") == 0) {
		fire(UserConnectionListener::MaxedOut(), this);
	} else if(strncmp(aLine+1, "Supports ", 9) == 0) {
		if((temp = strtok((char*)aLine+10, "\0")) != NULL) {
			fire(UserConnectionListener::Supports(), this, StringTokenizer<string>(temp, ' ').getTokens());
		}
	} else if(strncmp(aLine+1, "ListLen ", 8) == 0) {
		if((temp = strtok((char*)aLine+9, "\0")) != NULL) {
			fire(UserConnectionListener::ListLength(), this, temp);
		}
	} else if(strncmp(aLine+1, "ADC", 3) == 0) {
		dispatch(aLine, true);
	} else {
		fire(UserConnectionListener::Unknown(), this, aLine);
		dcdebug("Unknown UserConnection command: %.50s\n", aLine);
	}
}

void UserConnection::on(BufferedSocketListener::Failed, const string& aLine) throw() {
		setState(STATE_UNCONNECTED);
	fire(UserConnectionListener::Failed(), this, aLine);
}

void UserConnection::processBlock(const char* param, int type) throw() {
	char *temp;
		if((temp = strtok((char*)param, " ")) == NULL)
			return;

		int64_t start = _atoi64(temp);
		if(start < 0) {
			disconnect();
			return;
		}
		if((temp = strtok(NULL, " ")) == NULL)
			return;

		int64_t bytes = _atoi64(temp);
		if((temp = strtok(NULL, "\0")) == NULL)
			return;

		string name = temp;
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
