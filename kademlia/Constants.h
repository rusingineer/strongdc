/*
 * Copyright (C) 2008 Big Muscle, http://strongdc.sf.net
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

namespace kademlia
{

#define DPORT						SETTING(TLS_PORT)					// UDP port for listening to

#define ADC_PACKET_HEADER			'U'									// byte which every uncompressed packet must begin with
#define ADC_PACKET_FOOTER			0x0a								// byte which every uncompressed packet must end with
#define ADC_PACKED_PACKET_HEADER	0xc1								// compressed packet detection byte

#define KADEMLIA_FILE				"kademlia.xml"						// local file with all information got from the network

#define SELF_LOOKUP_TIMER			4*60*60*1000	// 4 hours			// how often to search for self node

#define SEARCH_ALPHA				3									// maximum nodes to contant in one search request
#define SEARCH_JUMPSTART			10*1000	// 10 seconds				// how often to process done search requests				
#define SEARCHNODE_LIFETIME			2*60*1000	// 2 minutes			// how long to search for node
#define SEARCHFILE_LIFETIME			3*60*1000	// 3 minutes			// how long to search for file

#define MAX_FILESOURCES				50									// max sources which can be saved to one TTH index

} // namespace kademlia