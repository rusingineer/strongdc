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

#define KADEMLIA_VERSION			0.1								// this number should be changed only when there's some bigger change which can influence whole network
#define KAD_PORT					5171							// UDP port for listening to (TODO: this could make be settable)

#define ADC_PACKET_HEADER			'U'								// byte which every uncompressed packet must begin with
#define ADC_PACKET_FOOTER			0x0a							// byte which every uncompressed packet must end with
#define ADC_PACKED_PACKET_HEADER	0xc1							// compressed packet detection byte

#define KADEMLIA_FILE				"kademlia.xml"					// local file with all information got from the network

#define MAX_INF_PER_TIME			4								// how many INFs can be sent in a INF_SENT_TIMER
#define SELF_LOOKUP_TIMER			4*60*60*1000	// 4 hours		// how often to search for self node
#define INF_SENT_TIMER				60*60*1000	// 1 hour			// how often to allow sending own INF
#define CONNECTED_TIMEOUT			20*60*1000	// 20 minutes		// when there's no incoming packet for this time, network will be set offline
#define NODE_RESPONSE_TIMEOUT		3*60*1000	// 3 minutes		// node has this time to response else we ignore him/mark him as dead node
#define NODE_EXPIRATION				2*60*60*1000 // 2 hours			// when node should be marked as possibly dead

#define SEARCH_ALPHA				3								// maximum nodes to contant in one search request
#define SEARCH_JUMPSTART			10*1000	// 10 seconds			// how often to process done search requests				
#define SEARCHNODE_LIFETIME			2*60*1000	// 2 minutes		// how long to try searching for node
#define SEARCHFILE_LIFETIME			3*60*1000	// 3 minutes		// how long to try searching for file
#define SEARCHSTOREFILE_LIFETIME	2*60*1000	// 2 minutes		// how long to try publishing a file
#define SEARCH_TOLERANCE			16777216						// minimum distance for the closest nodes

#define MAX_FILESOURCES				50								// max sources which can be saved to one TTH index
#define MAX_PUBLISHED_FILES			200								// max local files to publish
#define MIN_PUBLISH_FILESIZE		1024 * 1024 // 1 MiB			// files below this size won't be published
#define REPUBLISH_TIME				24*60*60*1000	// 24 hours		// when our filelist should be republished

#define BUCKET_SIZE					10								// maximum allowed nodes stored in one bucket

#define CLIENT_PROTOCOL				"ADC/1.0"						// protocol used for file transfers
#define SECURE_CLIENT_PROTOCOL_TEST	"ADCS/0.10"						// protocol used for secure file transfers
#define ADCS_FEATURE				"ADC0"							// support for secure protocol
#define TCP4_FEATURE				"TCP4"							// support for active TCP
#define UDP4_FEATURE				"UDP4"							// support for active UDP

using namespace dcpp;

} // namespace kademlia