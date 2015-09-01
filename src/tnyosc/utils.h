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

namespace tnyosc {
		
	/// Convert 32-bit float to a big-endian network format
	inline int32_t htonf( float x ) { return (int32_t) htonl( *(int32_t*) &x ); }
	/// Convert 64-bit float (double) to a big-endian network format
	inline int64_t htond( double x ) { return (int64_t) htonll( *(int64_t*) &x ); }
	/// Convert 32-bit big-endian network format to float
	inline double ntohf( int32_t x ) { x = ntohl( x ); return *(float*) &x; }
	/// Convert 64-bit big-endian network format to double
	inline double ntohd( int64_t x ) { return (double) ntohll( x ); }
	
	/// A byte array type internally used in the tnyosc library.
	using ByteBuffer = std::vector<char>;
	template<size_t size>
	using ByteArray = std::array<char, size>;
	
	inline uint64_t get_current_ntp_time()
	{
		auto now = std::chrono::system_clock::now();
		auto sec = std::chrono::duration_cast<std::chrono::seconds>( now.time_since_epoch() ).count() + 0x83AA7E80;
		auto usec = std::chrono::duration_cast<std::chrono::microseconds>( now.time_since_epoch() ).count() + 0x7D91048BCA000;
		
		return ( sec << 32 ) + ( usec % 1000000L );
	}
}
