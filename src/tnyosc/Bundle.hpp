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

#include "utils.h"
#include "Message.hpp"

namespace tnyosc {
	
/// This class represents an Open Sound Control bundle message. A bundle can
/// contain any number of Message and Bundle.
class Bundle {
public:
	
	/// Creates a OSC bundle with timestamp set to immediate. Call set_timetag to
	/// set a custom timestamp.
	Bundle()
	{
		static std::string id = "#bundle";
		mDataArray.resize( 16 );
		std::copy( id.begin(), id.end(), mDataArray.begin() );
		mDataArray[15] = 1;
	}
	~Bundle() {}
	
	// Functions for adding Message or Bundle.
	
	/// Appends an OSC message to this bundle. The message is immediately copied
	/// into this bundle and any changes to the message after the call to this
	/// function does not affect this bundle.
	///
	/// @param[in] message A pointer to tnyosc::Message.
	void append( const Message *message ) { append_data( message->byte_array() ); }
	
	/// Appends an OSC bundle to this bundle. The bundle may include any number
	/// of messages or bundles and are immediately copied into this bundle. Any
	/// changes to the bundle
	void append( const Bundle *bundle ) { append_data( bundle->byte_array() ); }
	void append( const Message &message ) { append_data( message.byte_array() ); }
	void append( const Bundle &bundle ) { append_data( bundle.byte_array() ); }
	
	/// Sets timestamp of the bundle.
	///
	/// @param[in] ntp_time NTP Timestamp
	/// @see get_current_ntp_time
	void set_timetag( uint64_t ntp_time )
	{
		uint64_t a = htonll( ntp_time );
		ByteArray<8> b;
		memcpy( &b[0], (char*) &a, 8 );
		mDataArray.insert( mDataArray.begin() + 8, b.begin(), b.end() );
	}
	
	/// Returns a complete byte array of this OSC bundle as a tnyosc::ByteBuffer
	/// type.
	///
	/// @return The OSC bundle as a tnyosc::ByteBuffer.
	/// @see data
	/// @see size
	const ByteBuffer& byte_array() const { return mDataArray; }
	
	/// Returns a pointer to the byte array of this OSC bundle. This call is
	/// convenient for actually sending this OSC bundle.
	///
	/// @return The OSC bundle as an char*.
	/// @see byte_array
	/// @see size
	///
	/// <pre>
	///   int sockfd; // initialize UDP socket...
	///   tnyosc::Bundle* bundle; // create a OSC bundle...
	///   send_to(sockfd, bundle->data(), bundle->size(), 0);
	/// </pre>
	///
	const char* data() const { return mDataArray.data(); }
	
	/// Returns the size of this OSC bundle.
	///
	/// @return Size of the OSC bundle in bytes.
	/// @see byte_array
	/// @see data
	size_t size() const { return mDataArray.size(); }
	
	/// Clears the bundle.
	void clear() { mDataArray.clear(); }
	
private:
	ByteBuffer mDataArray;
	
	void append_data( const ByteBuffer& data )
	{
		int32_t a = htonl( data.size() );
		ByteBuffer b( 4 + data.size() );
		memcpy( &b[0], (char*) &a, 4 );
		std::copy( data.begin(), data.end(), b.begin() + 4 );
		mDataArray.insert( mDataArray.end(), b.begin(), b.end() );
	}
};
	
}
