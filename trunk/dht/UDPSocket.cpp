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
 
#include "StdAfx.h"
#include "UDPSocket.h"

#include "Constants.h"
#include "DHT.h"
#include "Utils.h"

#include "../client/AdcCommand.h"
#include "../client/ClientManager.h"
#include "../client/LogManager.h"
#include "../client/SettingsManager.h"

#include "../zlib/zlib.h"

#include <mswsock.h>
#include <openssl/rc4.h>

namespace dht
{

	const uint32_t POLL_TIMEOUT =	10;
	#define BUFSIZE					16384
	#define	MAGICVALUE_UDP			91	

	UDPSocket::UDPSocket(void) : stop(false), port(0)
#ifdef _DEBUG
		, sentBytes(0), receivedBytes(0)
#endif	
	{
	}

	UDPSocket::~UDPSocket(void)
	{
		disconnect();
		
#ifdef _DEBUG
		dcdebug("DHT stats, received: %d bytes, sent: %d bytes\n", receivedBytes, sentBytes);
#endif		
	}
	
	/* 
	 * Disconnects UDP socket 
	 */
	void UDPSocket::disconnect() throw() 
	{
		if(socket.get()) 
		{
			stop = true;
			socket->disconnect();
			port = 0;

			join();

			socket.reset();

			stop = false;
		}
	}
	
	/*
	 * Starts listening to UDP socket 
	 */
	void UDPSocket::listen() throw(SocketException) 
	{
		disconnect();

		try 
		{
			socket.reset(new Socket);
			socket->create(Socket::TYPE_UDP);
			socket->setSocketOpt(SO_REUSEADDR, 1);
			socket->setSocketOpt(SO_RCVBUF, SETTING(SOCKET_IN_BUFFER));
			port = socket->bind(static_cast<uint16_t>(SETTING(DHT_PORT)), SETTING(BIND_ADDRESS));
		
			start();
		}
		catch(...) 
		{
			socket.reset();
			throw;
		}
	}

	void UDPSocket::checkIncoming() throw(SocketException)
	{
		if(socket->wait(POLL_TIMEOUT, Socket::WAIT_READ) == Socket::WAIT_READ)
		{
			sockaddr_in remoteAddr = { 0 };
			boost::scoped_array<uint8_t> buf(new uint8_t[BUFSIZE]);	
			int len = socket->read(&buf[0], BUFSIZE, remoteAddr);
			dcdrun(receivedBytes += len);
			
			if(len > 1)
			{
				uLongf destLen = BUFSIZE; // what size should be reserved?
				boost::scoped_array<uint8_t> destBuf;
				bool isUdpKeyValid = false;
				
				if(buf[0] != ADC_PACKED_PACKET_HEADER && buf[0] != ADC_PACKET_HEADER)
				{
					// it seems to be encrypted packet
	
					// the first try decrypts with our UDP key and CID
					// if it fails, decryption will happen with CID only
					int tries = 0;
					len -= 1;
					
					destBuf.reset(new uint8_t[len]);
					do
					{
						if(++tries == 3)
						{
							// decryption error, it could be malicious packet
							return;
						}
						
						// generate key
						TigerHash th;
						if(tries == 1)
							th.update(Utils::getUdpKey(inet_ntoa(remoteAddr.sin_addr)).data(), sizeof(CID));
						th.update(ClientManager::getInstance()->getMe()->getCID().data(), sizeof(CID));
							
						RC4_KEY recvKey;
						RC4_set_key(&recvKey, TigerTree::BYTES, th.finalize());
									
						// decrypt data
						RC4(&recvKey, len, &buf[1], &destBuf[0]);
					}
					while(destBuf[0] != MAGICVALUE_UDP);
					
					len -= 1;
					memcpy(&buf[0], &destBuf[1], len);
					
					// if decryption was successful in first try, it happened via UDP key
					// it happens only when we sent our UDP key to this node some time ago
					if(tries == 1) isUdpKeyValid = true;
				}
				
				if(buf[0] == ADC_PACKED_PACKET_HEADER) // is this compressed packet?
				{
					destBuf.reset(new uint8_t[destLen]);
					
					// decompress incoming packet
					int result = uncompress(destBuf.get(), &destLen, &buf[0] + 1, len - 1);
					if(result != Z_OK)
					{
						// decompression error!!!
						return;
					}
				}
				else
				{
					destBuf.swap(buf);
					destLen = len;
				}
				
				// process decompressed packet
				string s((char*)destBuf.get(), destLen);
				if(s[0] == ADC_PACKET_HEADER && s[s.length() - 1] == ADC_PACKET_FOOTER)	// is it valid ADC command?
				{	
					string ip = inet_ntoa(remoteAddr.sin_addr);
					uint16_t port = ntohs(remoteAddr.sin_port);
					COMMAND_DEBUG(s.substr(0, s.length() - 1), DebugManager::HUB_IN,  ip + ":" + Util::toString(port));
					DHT::getInstance()->dispatch(s.substr(0, s.length() - 1), ip, port, isUdpKeyValid);
				}
			}				
		}	
	}
	
	void UDPSocket::checkOutgoing(uint64_t& timer) throw(SocketException)
	{
		std::auto_ptr<Packet> packet;
		
		{
			Lock l(cs);
			uint64_t now = GET_TICK();

			if(!sendQueue.empty() && (now - timer > POLL_TIMEOUT))
			{
				// take the first packet in queue
				packet.reset(sendQueue.front());
				sendQueue.pop_front();

				//dcdebug("Sending DHT packet: %d bytes, %d ms\n", packet->length, (uint32_t)(now - timer));
					
				timer = now;
			}
		}

		if(packet.get())
		{
			try
			{
				dcdrun(sentBytes += packet->length);
				socket->writeTo(packet->ip, packet->port, packet->data, packet->length);
			}
			catch(SocketException& e)
			{
				dcdebug("DHT::run Write error: %s\n", e.getError().c_str());
			}
			
			delete[] packet->data;
		}	
	}

	/*
	 * Thread for receiving UDP packets 
	 */
	int UDPSocket::run() 
	{
		// fix receiving when sending fails
		DWORD value = FALSE;
		ioctlsocket(socket->sock, SIO_UDP_CONNRESET, &value);
		
		// antiflood variable
		uint64_t timer = GET_TICK();
				
		while(!stop)
		{
			try
			{
				// check outgoing queue
				checkOutgoing(timer);
							
				// check for incoming data
				checkIncoming();
			}
			catch(const SocketException& e)
			{
				dcdebug("DHT::run Error: %s\n", e.getError().c_str());
				
				bool failed = false;
				while(!stop)
				{
					try
					{
						socket->disconnect();
						socket->create(Socket::TYPE_UDP);
						socket->setSocketOpt(SO_RCVBUF, SETTING(SOCKET_IN_BUFFER));
						socket->bind(port, SETTING(BIND_ADDRESS));
						if(failed)
						{
							LogManager::getInstance()->message("DHT enabled again"); // TODO: translate
							failed = false;
						}
						break;
					}
					catch(const SocketException& e)
					{
						dcdebug("DHT::run Stopped listening: %s\n", e.getError().c_str());

						if(!failed)
						{
							LogManager::getInstance()->message("DHT disabled: " + e.getError()); // TODO: translate
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
		
		return 0;
	}	

	/*
	 * Sends command to ip and port 
	 */
	void UDPSocket::send(AdcCommand& cmd, const string& ip, uint16_t port, const CID& targetCID, const CID& udpKey)
	{
		// store packet for antiflooding purposes
		Utils::trackOutgoingPacket(ip, cmd);
		
		// pack data
		cmd.addParam("UK", Utils::getUdpKey(ip).toBase32()); // add our key for the IP address
		string command = cmd.toString(ClientManager::getInstance()->getMe()->getCID());
		COMMAND_DEBUG(command, DebugManager::HUB_OUT, ip + ":" + Util::toString(port));
				
		// compress packet
		uLongf destSize = compressBound(command.length()) + 2;
		
		uint8_t* srcBuf = (uint8_t*)command.data();
		uint8_t* destBuf = new uint8_t[destSize];
		
		int result = compress2(destBuf + 1, &destSize, srcBuf, command.length(), 9);
		if(result == Z_OK && destSize <= command.length())
		{
			destBuf[0] = ADC_PACKED_PACKET_HEADER;
			destSize += 1;
		}
		else
		{
			// compression failed, send uncompressed packet
			destSize = command.length();
			memcpy(destBuf, srcBuf, destSize);
			
			dcassert(destBuf[0] == ADC_PACKET_HEADER);
		}
		
		// generate encryption key
		TigerHash th;
		if(!udpKey.isZero())
		{
			th.update(udpKey.data(), sizeof(udpKey));
			th.update(targetCID.data(), sizeof(targetCID));
			
			RC4_KEY sentKey;
			RC4_set_key(&sentKey, TigerTree::BYTES, th.finalize());
					
			// encrypt data
			memmove(destBuf + 2, destBuf, destSize);
			
			// some random character except of ADC_PACKET_HEADER or ADC_PACKED_PACKET_HEADER
			uint8_t randomByte = static_cast<uint8_t>(Util::rand(0, 256));
			destBuf[0] = (randomByte == ADC_PACKET_HEADER || randomByte == ADC_PACKED_PACKET_HEADER) ? (randomByte + 1) : randomByte;
			destBuf[1] = MAGICVALUE_UDP;
			
			RC4(&sentKey, destSize + 1, destBuf + 1, destBuf + 1);
			destSize += 2;
		}
		else
		{
			// TODO: encrypt with CID at least
			// it will be implemented later, because it would break communication with older clients
		}

		Lock l(cs);
		sendQueue.push_back(new Packet(ip, port, destBuf, destSize));
	}

}