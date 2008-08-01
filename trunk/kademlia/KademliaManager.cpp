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

#include "stdafx.h"
#include <DCPlusPlus.h>

// kademlia includes
#include "Constants.h"
#include "IndexManager.h"
#include "KademliaManager.h"
#include "KadUtils.h"
#include "RoutingTable.h"
#include "SearchManager.h"

// external includes
#include <ClientManager.h>
#include <CryptoManager.h>
#include <File.h>
#include <LogManager.h>
#include <SettingsManager.h>
#include <ShareManager.h>
#include <SimpleXML.h>
#include <Socket.h>
#include <TimerManager.h>
#include <Util.h>

#include "../zlib/zlib.h"
#include <mswsock.h>

namespace kademlia
{

const uint32_t POLL_TIMEOUT = 250;

KademliaManager::KademliaManager(void) : stop(false), port(0), nextSelfLookup(GET_TICK() + 15*1000),
	nextSearchJumpStart(GET_TICK()), nextFirewallCheck(GET_TICK() + FIREWALL_CHECK_TIMER), 
	lastPacket(0), ip("0.0.0.0")
#ifdef _DEBUG
	, sentBytes(0), receivedBytes(0)
#endif
{
	httpConnection.addListener(this);
	IndexManager::newInstance();
	SearchManager::newInstance();
	
	searchQueue.interval = SETTING(MINIMUM_SEARCH_INTERVAL) * 1000 + 2000;
}

KademliaManager::~KademliaManager(void)
{
	httpConnection.removeListener(this);
	disconnect();
	
	SearchManager::deleteInstance();
	IndexManager::deleteInstance();
	
#ifdef _DEBUG
	dcdebug("Kademlia stats, received: %d bytes, sent: %d bytes\n", receivedBytes, sentBytes);
#endif
}

void KademliaManager::listen() throw(SocketException)
{
	disconnect();
	
	if(!BOOLSETTING(USE_KADEMLIA))
		return;
	
	try
	{
		socket.reset(new Socket);
		socket->create(Socket::TYPE_UDP);
		
		port = socket->bind(KAD_PORT, SETTING(BIND_ADDRESS));
	
		bool bootstrap = false;
		
		try
		{
			dcpp::File file(Util::getConfigPath() + KADEMLIA_FILE, dcpp::File::READ, dcpp::File::OPEN);
			bootstrap =	file.getSize() < 10000 || file.getLastModified() < GET_TICK() - 7 * 24 * 3600;
		}
		catch(FileException&)
		{
			bootstrap = true;
		}
		
		if(bootstrap)
		{
			dcdebug("MyCID: %s\n", ClientManager::getInstance()->getMyCID().toBase32().c_str());
			// TODO: make URL settable
			// TODO: add supported features
			string url = "http://strongdc.sourceforge.net/bootstrap/?cid=" + ClientManager::getInstance()->getMyCID().toBase32() +
				"&udp=" + Util::toString(port);
			
			httpConnection.downloadFile(url);
		}
		else
		{							
			start();
		}
	}
	catch(...)
	{
		socket.reset();
		throw;
	}		
}

void KademliaManager::disconnect() throw()
{
	if(socket.get())
	{
		stop = true;
#ifdef _WIN32
		join();
#endif
		socket->disconnect();
		socket.reset();

		port = 0;		
		stop = false;	
	}
}

void KademliaManager::loadData()
{
	try
	{
		SimpleXML xml;
		xml.fromXML(dcpp::File(Util::getConfigPath() + KADEMLIA_FILE, dcpp::File::READ, dcpp::File::OPEN).read());
		
		xml.stepIn();
		routingTable->loadNodes(xml);
		IndexManager::getInstance()->loadIndexes(xml);
		xml.stepOut();
	}
	catch(Exception& e)
	{
		dcdebug("%s\n", e.getError().c_str());
	}
}

void KademliaManager::saveData()
{
	SimpleXML xml;
	xml.addTag("Kademlia");
	xml.stepIn();
	
	routingTable->saveNodes(xml);
	IndexManager::getInstance()->saveIndexes(xml);
	
	xml.stepOut();
	
	try
	{
		dcpp::File file(Util::getConfigPath() + KADEMLIA_FILE + ".tmp", dcpp::File::WRITE, dcpp::File::CREATE | dcpp::File::TRUNCATE);
		BufferedOutputStream<false> bos(&file);
		bos.write(SimpleXML::utf8Header);
		xml.toXML(&bos);
		bos.flush();
		file.close();
		dcpp::File::deleteFile(Util::getConfigPath() + KADEMLIA_FILE);
		dcpp::File::renameFile(Util::getConfigPath() + KADEMLIA_FILE + ".tmp", Util::getConfigPath() + KADEMLIA_FILE);
	}
	catch(const FileException&)
	{
	}	
}

#define BUFSIZE 32768
/* There should be done some mechanism for sending acks. */
int KademliaManager::run()
{
	boost::scoped_array<uint8_t> buf(new uint8_t[BUFSIZE]);

	// fix receiving when sending fails
	DWORD value = FALSE;
	ioctlsocket(socket->sock, SIO_UDP_CONNRESET, &value);
	
	routingTable = new RoutingTable();
	loadData();
	
	TimerManager::getInstance()->addListener(this);
	ShareManager::getInstance()->publish();	// TODO: we should republish files every REPUBLISH_TIME
	
	uint64_t timer = GET_TICK();
	uint64_t delay = 100;
	while(!stop)
	{
		try
		{
			Packet* p = NULL;
			{
				Lock l(cs);
				if(!sendQueue.empty() && (GET_TICK() - timer > delay))
				{
					// take the first packet in queue
					p = sendQueue.front();
					sendQueue.pop_front();
					
					if(sendQueue.size() > 9)
						delay = 1000 / sendQueue.size();
						
					timer += delay;
				}
			}
			
			if(p != NULL)
			{
				// TODO: introduce some delay for sending packets not to flood UDP socket
				try
				{
					dcdrun(sentBytes += p->length);
					socket->writeTo(p->ip, p->port, p->data, p->length);
				}
				catch(SocketException& e)
				{
					dcdebug("KademliaManager::run Write error: %s\n", e.getError());
				}
				
				delete[] p->data;
				delete p;
			}
			
			// check for incoming data
			if(socket->wait(POLL_TIMEOUT, Socket::WAIT_READ) == Socket::WAIT_READ)
			{
				// TODO: check flooding
				sockaddr_in remoteAddr = { 0 };
				int len = socket->read(&buf[0], BUFSIZE, remoteAddr);
				dcdrun(receivedBytes += len);
				
				if(len > 1)
				{
					uLongf destLen = BUFSIZE; // what size should be reserved?
					uint8_t* destBuf;
					
					if(buf[0] == ADC_PACKED_PACKET_HEADER) // is this compressed packet?
					{
						destBuf = new uint8_t[destLen];
						
						// decompress incoming packet
						int result = uncompress(destBuf, &destLen, &buf[0] + 1, len - 1);
						if(result != Z_OK)
						{
							// decompression error!!!
							delete[] destBuf;
							continue;
						}
					}
					else
					{
						destBuf = &buf[0];
						destLen = len;
					}
					
					// process decompressed packet
					string s((char*)destBuf, destLen);
					if(s[0] == ADC_PACKET_HEADER && s[s.length() - 1] == ADC_PACKET_FOOTER)	// is it valid ADC command?
					{
						AdcCommand cmd(s.substr(0, s.length() - 1));
						
						// hack:	because ADC UDP command don't have "to" and "from" set, we can abuse it
						//			to set IP and port
						cmd.setFrom(ntohl(remoteAddr.sin_addr.s_addr));
						cmd.setTo(ntohs(remoteAddr.sin_port));
						
						lastPacket = GET_TICK();
						dispatch(cmd);

						// TODO: send ack that packet received successfully
					}
				}
			}
		}
		catch(const SocketException& e)
		{
			dcdebug("KademliaManager::run Error: %s\n", e.getError().c_str());
			
			bool failed = false;
			while(!stop)
			{
				try
				{
					socket->disconnect();
					socket->create(Socket::TYPE_UDP);
					socket->bind(port, SETTING(BIND_ADDRESS));
					if(failed)
					{
						LogManager::getInstance()->message("Decentralization network enabled again"); // TODO: translate
						failed = false;
					}
					break;
				}
				catch(const SocketException& e)
				{
					dcdebug("KademliaManager::run Stopped listening: %s\n", e.getError().c_str());

					if(!failed)
					{
						LogManager::getInstance()->message("Decentralization network disabled: " + e.getError()); // TODO: translate
						failed = true;
					}

					// Spin for 60 seconds
					for(int i = 0; i < 60 && !stop; ++i)
					{
						Thread::sleep(1000);
					}
				}
			}
		}		
	}
	
	TimerManager::getInstance()->removeListener(this);
	
	saveData();
		
	delete routingTable;
	
	return 0;
}

void KademliaManager::send(const AdcCommand& cmd, const string& ip, uint16_t port)
{
	string command = cmd.toString(ClientManager::getInstance()->getMyCID());
	
	// compress data to have at least some kind of "encryption"
	uLongf destSize = compressBound(command.length()) + 1;
	
	uint8_t* srcBuf = (uint8_t*)command.data();
	uint8_t* destBuf = new uint8_t[destSize];
	
	// packets are small, so compress them using fast method
	int result = compress2(destBuf + 1, &destSize, srcBuf, command.length(), SETTING(MAX_COMPRESSION));
	if(result == Z_OK && destSize <= command.length())
	{
		destBuf[0] = ADC_PACKED_PACKET_HEADER;
		destSize += 1;
		
		dcdebug("Packet compressed successfuly, original size = %d bytes, new size %d bytes\n", command.length() + 1, destSize);
	}
	else
	{
		// compression failed, send uncompressed packet
		destSize = command.length();
		memcpy(destBuf, srcBuf, destSize);
	}
		
	Lock l(cs);
	sendQueue.push_back(new Packet(ip, port, destBuf, destSize)); 
}

void KademliaManager::info(const string& ip, uint16_t port, bool request, bool firewallCheck)
{
	AdcCommand cmd(AdcCommand::CMD_INF, AdcCommand::TYPE_UDP);
	cmd.addParam("VE", Util::toString(KADEMLIA_VERSION));
	cmd.addParam("NI", SETTING(NICK));
	cmd.addParam("I4", firewallCheck ? "0.0.0.0" : this->ip);
	cmd.addParam("U4", Util::toString(this->port));
	cmd.addParam("RE", request ? "1" : "0");
	
	// TODO: add more info?
	if(firewallCheck)
	{
		Lock l(cs);
		uint64_t now = GET_TIME();
		requestedFirewalledChecks[ip] = now;
		
		std::tr1::unordered_map<string, uint64_t>::iterator i = requestedFirewalledChecks.begin();
		while(i != requestedFirewalledChecks.end())
		{
			if(now - i->second > NODE_RESPONSE_TIMEOUT)
				requestedFirewalledChecks.erase(i++);
			else
				++i;
		}
	}
	
	send(cmd, ip, port);
}

void KademliaManager::handle(AdcCommand::INF, AdcCommand& cmd) throw()
{
	if(cmd.getParameters().empty())
		return;
	
	string cid = cmd.getParam(0);
	if(cid.size() != 39)
		return;
	
	in_addr addr; addr.s_addr = cmd.getFrom();
	string senderIp = inet_ntoa(addr);
	uint16_t senderPort = static_cast<uint16_t>(cmd.getTo());

	string ip;	
	if(!cmd.getParam("I4", 0, ip))
	{
		send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_GENERIC, "IP unknown", AdcCommand::TYPE_UDP), senderIp, senderPort);
		return; // no IP supplied
	}
	
	// TODO: should we check port too?
	if(Util::isPrivateIp(ip) || ip != senderIp)
	{
		if(ip == "0.0.0.0")
		{
			// send correct public IP to the node
			send(AdcCommand(AdcCommand::SEV_SUCCESS, AdcCommand::ERROR_GENERIC, senderIp, AdcCommand::TYPE_UDP).addParam("FC", "UINF"), senderIp, senderPort);
		}
		else
		{
			send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_BAD_IP, "Your client supplied invalid IP: " + ip, AdcCommand::TYPE_UDP), senderIp, senderPort);
			return; // invalid ip supplied
		}
	}
	
	OnlineUserPtr ou = routingTable->add(CID(cid));
	ou->getIdentity().setIp(senderIp);
	ou->getIdentity().setUdpPort(Util::toString(senderPort));
	
	for(StringIterC i = cmd.getParameters().begin(); i != cmd.getParameters().end(); ++i) {
		if(i->length() < 2)
			continue;

		if(i->substr(0, 2) != "RE") {
			ou->getIdentity().set(i->c_str(), i->substr(2));
		}
	}
	
	// ignore stated values
	ou->getIdentity().setIp(senderIp);
	ou->getIdentity().setUdpPort(Util::toString(senderPort));

	string request;
	if(cmd.getParam("RE", 0, request) && request[0] == '1')
	{
		info(senderIp, senderPort, false, recheckIP < MAX_RECHECK_IP);
	}
	else
	{
		// TODO: set node alive
	}
}

void KademliaManager::handle(AdcCommand::REQ, AdcCommand& cmd) throw()
{
	if(cmd.getParameters().empty())
		return;
	
	string cid = cmd.getParam(0);
	if(cid.size() != 39)
		return;
	
	in_addr addr; addr.s_addr = cmd.getFrom();
	string senderIp = inet_ntoa(addr);
	uint16_t senderPort = static_cast<uint16_t>(cmd.getTo());
	
	if(recheckIP < MAX_RECHECK_IP)
	{
		// send my info
		info(senderIp, senderPort, true, true);
	}
	
	string token;
	if(!cmd.getParam("TO", 0, token))
		return;	// missing search token?
	
	string term;
	if(!cmd.getParam("TR", 0, term))
		return;	// nothing to search?
		
	string maximumNodes;
	if(!cmd.getParam("MN", 0, maximumNodes))
		return; // no nodes?
	
	// this is possible nodes request	
	NodeMap results;
	const CID& myCID = ClientManager::getInstance()->getMyCID();
	routingTable->getClosestTo(KadUtils::getDistance(CID(term), myCID), Util::toInt(maximumNodes), results);
	
	AdcCommand res(AdcCommand::CMD_RES, AdcCommand::TYPE_UDP);
	res.addParam("TR", term);
	res.addParam("TO", token);
		
	if(!results.empty())
	{
		SimpleXML xml;
		xml.addTag("Nodes");
		xml.stepIn();
		
		for(NodeMap::const_iterator i = results.begin(); i != results.end(); i++)
		{
			const OnlineUserPtr& ou = i->second;
			
			xml.addTag("Node");
			xml.addChildAttrib("CID", ou->getUser()->getCID().toBase32());
			xml.addChildAttrib("IP", ou->getIdentity().getIp());
			xml.addChildAttrib("UDP", ou->getIdentity().getUdpPort());			
		}
		
		xml.stepOut();
		
		string nodes;
		StringOutputStream sos(nodes);
		sos.write(SimpleXML::utf8Header);
		xml.toXML(&sos);
		
		res.addParam("NX", nodes);
	}
	
	KademliaManager::getInstance()->send(res, senderIp, senderPort);
}

void KademliaManager::handle(AdcCommand::SCH, AdcCommand& c) throw()
{
	SearchManager::getInstance()->processSearchRequest(c);
}

void KademliaManager::handle(AdcCommand::PUB, AdcCommand& cmd) throw()
{
	if(cmd.getParameters().empty())
		return;
	
	string cid = cmd.getParam(0);
	if(cid.size() != 39)
		return;
	
	in_addr addr; addr.s_addr = cmd.getFrom();
	string senderIp = inet_ntoa(addr);
	uint16_t senderPort = static_cast<uint16_t>(cmd.getTo());
	
	string tth;
	if(!cmd.getParam("TR", 0, tth))
		return;	// nothing to search?
		
	TTHValue distance = KadUtils::getDistance(ClientManager::getInstance()->getMyCID(), TTHValue(tth));
	if(KadUtils::toBinaryString(distance.data).substr(0, 5) != "00000")
		return; // we are too far from this file
		
	string size;
	if(!cmd.getParam("SI", 0, tth))
		return;	// no file size?
		
	Identity identity(ClientManager::getInstance()->getUser(CID(cid)), 0);
	identity.setIp(senderIp);
	identity.setUdpPort(Util::toString(senderPort));
	identity.set("SS", size);
					
	IndexManager::getInstance()->addIndex(TTHValue(tth), identity);
	
	// TODO: send response?
}

void KademliaManager::handle(AdcCommand::RES, AdcCommand& c) throw()
{
	if(c.getParameters().empty())
		return;
		
	in_addr addr; addr.s_addr = c.getFrom();
	string senderIp = inet_ntoa(addr);
	uint16_t senderPort = static_cast<uint16_t>(c.getTo());
	
	if(recheckIP < MAX_RECHECK_IP) // TODO: do this only when RES is response to REQ
	{
		// send my info
		info(senderIp, senderPort, true, true);
	}
	
	SearchManager::getInstance()->processSearchResponse(c);
}

void KademliaManager::handle(AdcCommand::STA, AdcCommand& c) throw()
{
	if(c.getParameters().size() < 2)
		return;
		
	in_addr addr; addr.s_addr = c.getFrom();
	string senderIp = inet_ntoa(addr);		
	
	string fc;
	c.getParam("FC", 0, fc);
	
	if(fc == "UINF")
	{
		// check that we have really requested firewalled check from this IP
		Lock l(cs);
		if(requestedFirewalledChecks.count(senderIp) > 0)
		{
			// ip updated, we could use this to update Direct Connect IP aswell
			ip = c.getParam(1);
			requestedFirewalledChecks.erase(senderIp);
			recheckIP++;
		} // TODO: else IP spoofing
	}
	else
	{
		// we should disconnect from network when severity is fatal, but it could be possible exploit
		LogManager::getInstance()->message("Kademlia error (" + senderIp + "): " + c.getParam(1)); // TODO: translate
	}
}

void KademliaManager::on(HttpConnectionListener::Data, HttpConnection*, const uint8_t* buf, size_t len) throw()
{
	nodesXML += string((const char*)buf, len);
}

void KademliaManager::on(HttpConnectionListener::Complete, HttpConnection*, string const&) throw()
{
	if(!nodesXML.empty())
	{
		try
		{
			uLongf destLen = BUFSIZE;
			boost::scoped_array<uint8_t> destBuf(new uint8_t[destLen]);
			
			// decompress incoming packet
			int result = uncompress(&destBuf[0], &destLen, (Bytef*)nodesXML.data(), nodesXML.length());
			if(result != Z_OK)
			{
				// decompression error!!!
				throw Exception("Decompress error.");
			}
			
			// now copy remote xml to local one
			dcpp::File file(Util::getConfigPath() + KADEMLIA_FILE, dcpp::File::READ, dcpp::File::CREATE | dcpp::File::OPEN);
			
			SimpleXML localXml, remoteXml;
			
			string fileContent = file.read();
			if(fileContent.empty())
			{
				localXml.addTag("Kademlia");
			}
			else
			{
				localXml.fromXML(fileContent);
			}
			
			remoteXml.fromXML(string((char*)&destBuf[0], destLen));
			
			localXml.stepIn();
			remoteXml.stepIn();
			
			if(!localXml.findChild("Nodes"))
			{
				localXml.addTag("Nodes");
			}
			
			localXml.stepIn();
				
			while(remoteXml.findChild("Node"))
			{
				localXml.addTag("Node");
				localXml.addChildAttrib("CID", remoteXml.getChildAttrib("CID"));
				localXml.addChildAttrib("IP", remoteXml.getChildAttrib("IP"));
				localXml.addChildAttrib("UDP", remoteXml.getChildAttrib("UDP"));					
			}

			localXml.stepOut();
			localXml.stepOut();
			remoteXml.stepOut();
			file.close();
			
			// write merged file
			dcpp::File f(Util::getConfigPath() + KADEMLIA_FILE + ".tmp", dcpp::File::WRITE, dcpp::File::CREATE | dcpp::File::TRUNCATE);	
			BufferedOutputStream<false> bos(&f);
			bos.write(SimpleXML::utf8Header);
			localXml.toXML(&bos);
			bos.flush();
			f.close();
			dcpp::File::deleteFile(Util::getConfigPath() + KADEMLIA_FILE);
			dcpp::File::renameFile(Util::getConfigPath() + KADEMLIA_FILE + ".tmp", Util::getConfigPath() + KADEMLIA_FILE);			
		}
		catch(Exception& e)
		{
			dcdebug("Kademlia bootstrap error: %s\n", e.getError().c_str());
		}
	}
	
	start();
}

void KademliaManager::on(HttpConnectionListener::Failed, HttpConnection*, const string& aLine) throw()
{
	dcdebug("Bootstrapping failed: %s\n", aLine.c_str());
	start();
}
	
void KademliaManager::on(TimerManagerListener::Second, uint64_t aTick) throw()
{
	if(aTick - lastPacket < CONNECTED_TIMEOUT)
	{
		// publish next file in queue
		IndexManager::getInstance()->publishNextFile();
	}
	
	if(aTick >= nextSelfLookup) // search for self
	{
		SearchManager::getInstance()->findNode(ClientManager::getInstance()->getMyCID());
		nextSelfLookup = aTick + SELF_LOOKUP_TIMER;
	}
	if(aTick >= nextFirewallCheck) // check if we are behind firewall
	{
		recheckIP = 0;
		nextFirewallCheck = aTick + FIREWALL_CHECK_TIMER;
	}
	if(aTick >= nextSearchJumpStart) // send next search request
	{
		SearchManager::getInstance()->processSearches();
		nextSearchJumpStart = aTick + SEARCH_JUMPSTART;
	}
	
	dcpp::Search s;
	if(searchQueue.pop(s))
	{
		SearchManager::getInstance()->findFile(s.query);
	}
}

void KademliaManager::on(TimerManagerListener::Minute, uint64_t aTick) throw()
{
}

uint64_t KademliaManager::search(const TTHValue& tth)
{
	dcpp::Search s;
	s.query    = tth.toBase32();
	s.token    = "auto";
	s.owners.insert(NULL);

	searchQueue.add(s);

	return searchQueue.getSearchTime(NULL) - GET_TICK();
}

} // namespace kademlia
