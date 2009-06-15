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

#include "../client/AdcCommand.h"
#include "../client/ClientManager.h"
#include "../client/LogManager.h"
#include "../client/SettingsManager.h"

#include "../zlib/zlib.h"

#include <mswsock.h>

namespace dht
{

	const uint32_t POLL_TIMEOUT = 250;
	#define BUFSIZE 16384	

	UDPSocket::UDPSocket(void) : stop(false), port(0)
	{
	}

	UDPSocket::~UDPSocket(void)
	{
		disconnect();
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
			port = socket->bind(static_cast<uint16_t>(DHT_UDPPORT), SETTING(BIND_ADDRESS));
		
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
						return;
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
					DHT::getInstance()->dispatch(s.substr(0, s.length() - 1), inet_ntoa(remoteAddr.sin_addr), ntohs(remoteAddr.sin_port));
				}
			}				
		}	
	}
	
	void UDPSocket::checkOutgoing(uint64_t& timer, uint64_t& delay) throw(SocketException)
	{
		std::auto_ptr<Packet> packet;
		{
			Lock l(cs);
			if(!sendQueue.empty() && (GET_TICK() - timer > delay))
			{
				// take the first packet in queue
				packet.reset(sendQueue.front());
				sendQueue.pop_front();
				
				// control to avoid flooding UDP interface
				if(sendQueue.size() > 9)
					delay = 1000 / sendQueue.size();
					
				timer += delay;
			}
		}
		
		if(packet.get())
		{
			try
			{
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
		
		// antiflood variables
		uint64_t timer = GET_TICK();
		uint64_t delay = 100;
				
		while(!stop)
		{
			try
			{
				// check outgoing queue
				checkOutgoing(timer, delay);
							
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
	void UDPSocket::send(const AdcCommand& cmd, const string& ip, uint16_t port)
	{
		string command = cmd.toString(ClientManager::getInstance()->getMe()->getCID());
		
		// compress data to have at least some kind of "encryption"
		uLongf destSize = compressBound(command.length()) + 1;
		
		uint8_t* srcBuf = (uint8_t*)command.data();
		uint8_t* destBuf = new uint8_t[destSize];
		
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

}