/* 
 * Copyright (C) 2003-2005 RevConnect, http://www.revconnect.com
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

#pragma once

#include "CriticalSection.h"
#include "File.h"

struct SharedFileHandle : CriticalSection
{
    HANDLE 				handle;
    int					ref_cnt;

	SharedFileHandle(const string& name, bool shareDelete = false);

	~SharedFileHandle()
    {
        CloseHandle(handle);
    }
};

# pragma warning(disable: 4511) // copy constructor could not be generated
# pragma warning(disable: 4512) // assignment operator could not be generated

class SharedFileStream : public IOStream
{

public:

    typedef map<string, SharedFileHandle*> SharedFileHandleMap;

    SharedFileStream(const string& name, int64_t _pos, int64_t size = 0, bool shareDelete = false);

    ~SharedFileStream();

	size_t write(const void* buf, size_t len) throw(Exception);

	size_t read(void* buf, size_t& len) throw(Exception);

	size_t flush() throw(Exception) 
	{
		Lock l(*shared_handle_ptr);

		if(!FlushFileBuffers(shared_handle_ptr->handle))
			throw FileException(Util::translateError(GetLastError()));
		return 0;
	}

	void setPos(int64_t _pos) 
	{ pos = _pos; }

    static CriticalSection critical_section;
	static SharedFileHandleMap file_handle_pool;

private:
	SharedFileHandle* shared_handle_ptr;
	int64_t pos;


};
