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

#include "Thread.h"

#include "ResourceManager.h"

#ifdef _WIN32
void Thread::start() throw(ThreadException) {
	join();
    #if defined (_MT)
		if( (threadHandle = chBEGINTHREADEX(NULL, 0, &starter, this, 0, &threadId)) == NULL) {
			throw ThreadException(STRING(UNABLE_TO_CREATE_THREAD));
		}
	#else	
		if( (threadHandle = CreateThread(NULL, 0, &starter, this, 0, &threadId)) == NULL) {
			throw ThreadException(STRING(UNABLE_TO_CREATE_THREAD));
		}
	#endif
}

#else
void Thread::start() throw(ThreadException) { 
	join();
	if(pthread_create(&threadHandle, NULL, &starter, this) != 0) {
		throw ThreadException(STRING(UNABLE_TO_CREATE_THREAD));
	}
};
#endif
/**
 * @file
 * $Id$
 */