/*
 * Copyright (C) 2009 Big Muscle, http://strongdc.sf.net
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

namespace dht
{

#define BOOTSTRAP_URL				"http://strongdc.sourceforge.net/bootstrap/"

#define DHT_UDPPORT					6184							// default DHT port (TODO: make as option)

#define ID_BITS						192								// size of identificator (in bits)

#define ADC_PACKET_HEADER			'U'								// byte which every uncompressed packet must begin with
#define ADC_PACKET_FOOTER			0x0a							// byte which every uncompressed packet must end with
#define ADC_PACKED_PACKET_HEADER	0xc1							// compressed packet detection byte

#define SEARCH_ALPHA				3								// degree of search parallelism
#define MAX_SEARCH_RESULTS			50								// maximum of allowed search results
#define SEARCH_PROCESSTIME			10*1000	// 10 seconds			// how often to process done search requests				
#define SEARCHNODE_LIFETIME			2*60*1000	// 2 minutes		// how long to try searching for node
#define SEARCHFILE_LIFETIME			3*60*1000	// 3 minutes		// how long to try searching for file
#define SEARCHSTOREFILE_LIFETIME	2*60*1000	// 2 minutes		// how long to try publishing a file

#define K							10								// maximum nodes in one bucket

#define MAX_PUBLISHED_FILES			200								// max local files to publish
#define MIN_PUBLISH_FILESIZE		1024 * 1024 // 1 MiB			// files below this size won't be published
#define REPUBLISH_TIME				24*60*60*1000	// 24 hours		// when our filelist should be republished

#define NODE_RESPONSE_TIMEOUT		1*60*1000	// 1 minute			// node has this time to response else we ignore him/mark him as dead node
	
using namespace dcpp;

}