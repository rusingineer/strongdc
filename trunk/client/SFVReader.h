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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "File.h"

class SFVReader {
public:
	/** @see load */
	SFVReader(const string& aFileName) : crc32(0), crcFound(false) { load(aFileName); }; 

	/**
	 * Search for a CRC32 file in all .sfv files in the directory of fileName. 
	 * Each SFV file has a number of lines containing a filename and its CRC32 value 
	 * in the form:
	 * filename.ext xxxxxxxx
	 * where the x's represent the file's crc32 value. Lines starting with ';' are
	 * considered comments, and we throw away lines with ' ' or '#' as well
	 * (pdSFV 1.2 does this...).
	 */
	void load(const string& fileName) throw();

	bool hasCRC() throw() { return crcFound; };
	u_int32_t getCRC() throw() { return crc32; };
	
private:

	u_int32_t crc32;
	bool crcFound;

	bool tryFile(const string& sfvFile, const string& fileName) throw(FileException);
	
};

/**
 * @file
 * $Id$
 */