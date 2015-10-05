// Copyright (c) 2011 Toshiro Yamada
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/// @file tnyosc.hpp
/// @brief tnyosc main (and only) header file
/// @author Toshiro Yamada
///
/// tnyosc is a header-only Open Sound Control library written in C++ for
/// creating OSC-compliant messages. tnyosc supports Open Sound Control 1.0 and
/// 1.1 types and other nonstandard types, and bundles. Note that tnyosc does not
/// include code to actually send or receive OSC messages.

#pragma once

#include <stdint.h>
#include <ctime>
#include <chrono>
#include <algorithm>
#include <functional>
#include <vector>
#include <memory>
#include <iostream>
#include "asio/asio.hpp"

// Following code snippets were taken from
// http://www.jbox.dk/sanos/source/include/net/inet.h.html
#ifndef htons
#define htons(n) (((((uint16_t)(n) & 0xFF)) << 8) | (((uint16_t)(n) & 0xFF00) >> 8))
#endif
#ifndef ntohs
#define ntohs(n) (((((uint16_t)(n) & 0xFF)) << 8) | (((uint16_t)(n) & 0xFF00) >> 8))
#endif
#ifndef htonl
#define htonl(n) (((((uint32_t)(n) & 0xFF)) << 24) | ((((uint32_t)(n) & 0xFF00)) << 8) | ((((uint32_t)(n) & 0xFF0000)) >> 8) | ((((uint32_t)(n) & 0xFF000000)) >> 24))
#endif
#ifndef ntohl
#define ntohl(n) (((((uint32_t)(n) & 0xFF)) << 24) | ((((uint32_t)(n) & 0xFF00)) << 8) | ((((uint32_t)(n) & 0xFF0000)) >> 8) | ((((uint32_t)(n) & 0xFF000000)) >> 24))
#endif

// Following ntohll() and htonll() code snippets were taken from
// http://www.codeproject.com/KB/cpp/endianness.aspx?msg=1661457
#ifndef ntohll
/// Convert 64-bit little-endian integer to a big-endian network format
#define ntohll(x) (((int64_t)(ntohl((int32_t)((x << 32) >> 32))) << 32) | (uint32_t)ntohl(((int32_t)(x >> 32))))
#endif
#ifndef htonll
/// Convert 64-bit big-endian network format to a little-endian integer
#define htonll(x) ntohll(x)
#endif

namespace osc {
	
using UDPSocketRef = std::shared_ptr<asio::ip::udp::socket>;
using TCPSocketRef = std::shared_ptr<asio::ip::tcp::socket>;
	
/// Convert 32-bit float to a big-endian network format
inline int32_t htonf( float x ) { return (int32_t) htonl( *(int32_t*) &x ); }
/// Convert 64-bit float (double) to a big-endian network format
inline int64_t htond( double x ) { return (int64_t) htonll( *(int64_t*) &x ); }
/// Convert 32-bit big-endian network format to float
inline double ntohf( int32_t x ) { x = ntohl( x ); return *(float*) &x; }
/// Convert 64-bit big-endian network format to double
inline double ntohd( int64_t x ) { return (double) ntohll( x ); }

/// A byte array type internally used in the tnyosc library.
using ByteBuffer = std::vector<uint8_t>;
template<size_t size>
using ByteArray = std::array<uint8_t, size>;
using ByteBufferRef = std::shared_ptr<ByteBuffer>;
	
class TransportData {
public:
	virtual ~TransportData() = default;
	virtual const ByteBuffer& byteArray() const = 0;
	virtual size_t size() const = 0;
	virtual void clear() = 0;
protected:
	TransportData() = default;
	
	virtual ByteBufferRef getSharedBuffer() const = 0;
	
	friend class SenderBase;
};
	
enum class ArgType {
	INTEGER_32,
	FLOAT,
	DOUBLE,
	STRING,
	BLOB,
	MIDI,
	TIME_TAG,
	INTEGER_64,
	BOOL_T,
	BOOL_F,
	CHAR,
	NULL_T,
	INFINITUM,
	NONE
};

const char* argTypeToString( ArgType type );

namespace time {
	uint64_t get_current_ntp_time();
	//! Returns the current presentation time as NTP time, which may include an offset to the system clock.
	uint64_t getClock();
	//! Returns the current presentation time as NTP time, which may include an offset to the system clock.
	uint64_t getClock( uint32_t *year, uint32_t *month, uint32_t *day, uint32_t *hours, uint32_t *minutes, uint32_t *seconds );
	//! Returns the system clock as NTP time.
	uint64_t getSystemClock() { return get_current_ntp_time(); }
	//! Returns the current presentation time as a string.
	std::string getClockString( bool includeDate = true );
	//! Sets the current presentation time as NTP time, from which an offset to the system clock is calculated.
	void setClock( uint64_t ntp_time );
	
};

class ExcIndexOutOfBounds : public ci::Exception {
public:
	ExcIndexOutOfBounds( const std::string &address, uint32_t index )
	: Exception( std::string( std::to_string( index ) + " out of bounds from address, " + address ) )
	{}
};

class ExcNonConvertible : public ci::Exception {
public:
	ExcNonConvertible( const std::string &address, uint32_t index, ArgType actualType, ArgType convertToType )
	: Exception( std::string( std::to_string( index ) + ": " + address + ", expected type: " + argTypeToString( convertToType ) + ", actual type: " + argTypeToString( actualType ) ) )
	{}
};
	
}
