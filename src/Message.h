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

namespace osc {
	
/// This class represents an Open Sound Control message. It supports Open Sound
/// Control 1.0 and 1.1 specifications and extra non-standard arguments listed
/// in http://opensoundcontrol.org/spec-1_0.
class Message {
public:
	
	//! Create an OSC message.
	Message() = default;
	explicit Message( const std::string& address );
	Message( const Message & ) = delete;
	Message& operator=( const Message & ) = delete;
	Message( Message && ) = default;
	Message& operator=( Message && ) = default;
	~Message() = default;
	
	// Functions for appending OSC 1.0 types
	
	//! Appends an int32 to the back of the message.
	void append( int32_t v );
	//! Appends a float to the back of the message.
	void append( float v );
	//! Appends a string to the back of the message.
	void append( const std::string& v );
	//! Appends a string to the back of the message.
	void append( const char* v, size_t len );
	void append( const char* v ) = delete;
	//! Appends an osc blob to the back of the message.
	void appendBlob( void* blob, uint32_t size );
	//! Appends an osc blob to the back of the message.
	void appendBlob( const ci::Buffer &buffer );
	
	// Functions for appending OSC 1.1 types
	//! Appends an OSC-timetag (NTP format) to the back of the message.
	void appendTimeTag( uint64_t v );
	//! Appends the current UTP timestamp to the back of the message.
	void appendCurrentTime() { appendTimeTag( time::get_current_ntp_time() ); }
	//! Appends a 'T'(True) or 'F'(False) to the back of the message.
	void append( bool v );
	//! Appends a Null (or nil) to the back of the message.
	void appendNull() { mIsCached = false; mDataViews.emplace_back( ArgType::NULL_T, -1, 0 ); }
	//! Appends an Impulse (or Infinitum) to the back of the message
	void appendImpulse() { mIsCached = false; mDataViews.emplace_back( ArgType::INFINITUM, -1, 0 ); }
	
	// Functions for appending nonstandard types
	//! Appends an int64_t to the back of the message.
	void append( int64_t v );
	//! Appends a float64 (or double) to the back of the message.
	void append( double v );
	//! Appends an ascii character to the back of the message.
	void append( char v );
	//! Appends a midi value to the back of the message.
	void appendMidi( uint8_t port, uint8_t status, uint8_t data1, uint8_t data2 );
	// TODO: figure out if array is useful.
	// void appendArray( void* array, size_t size );
	
	//! Returns the int32_t located at \a index.
	int32_t		getInt( uint32_t index ) const;
	//! Returns the float located at \a index.
	float		getFloat( uint32_t index ) const;
	//! Returns the string located at \a index.
	std::string getString( uint32_t index ) const;
	//! Returns the time_tag located at \a index.
	int64_t		getTime( uint32_t index ) const;
	//! Returns the time_tag located at \a index.
	int64_t		getInt64( uint32_t index ) const;
	//! Returns the double located at \a index.
	double		getDouble( uint32_t index ) const;
	//! Returns the bool located at \a index.
	bool		getBool( uint32_t index ) const;
	//! Returns the char located at \a index.
	char		getChar( uint32_t index ) const;

	void		getMidi( uint32_t index, uint8_t *port, uint8_t *status, uint8_t *data1, uint8_t *data2 ) const;
	//! Returns the blob, as a ci::Buffer, located at \a index.
	ci::Buffer	getBlob( uint32_t index ) const;
	
	//! Returns the argument type located at \a index.
	ArgType		getArgType( uint32_t index ) const;
	
	bool compareTypes( const std::string &types );
	
	/// Sets the OSC address of this message.
	/// @param[in] address The new OSC address.
	void setAddress( const std::string& address );
	
	/// Returns the OSC address of this message.
	const std::string& getAddress() const { return mAddress; }
	
	/// Returns a complete byte array of this OSC message as a ByteArray type.
	/// The byte array is constructed lazily and is cached until the cache is
	/// obsolete. Call to |data| and |size| perform the same caching.
	///
	/// @return The OSC message as a ByteArray.
	/// @see data
	/// @see size
	ByteBuffer byteArray() const;
	
	/// Returns the size of this OSC message.
	///
	/// @return Size of the OSC message in bytes.
	/// @see byte_array
	/// @see data
	size_t size() const;
	
	/// Clears the message.
	void clear();
	
private:
	static uint8_t getTrailingZeros( size_t bufferSize ) { return 4 - ( bufferSize % 4 ); }
	size_t getCurrentOffset() { return mDataArray.size(); }
	
	class Argument {
	public:
		Argument();
		Argument( ArgType type, int32_t offset, uint32_t size, bool needsSwap = false );
		Argument( const Argument &arg ) = default;
		Argument( Argument &&arg ) = default;
		Argument& operator=( const Argument &arg ) = default;
		Argument& operator=( Argument &&arg ) = default;
		
		~Argument() = default;
		
		ArgType		getArgType() const { return mType; }
		uint32_t	getArgSize() const { return mSize; }
		int32_t		getOffset() const { return mOffset; }
		bool		needsEndianSwapForTransmit() const { return mNeedsEndianSwapForTransmit; }
		void		outputValueToStream( uint8_t *buffer, std::ostream &ostream ) const;
		
		template<typename T>
		bool convertible() const;
		
		static char translateArgTypeToChar( ArgType type );
		static ArgType translateCharToArgType( char type );
		
		void swapEndianForTransmit( uint8_t* buffer ) const;
		
	private:
		
		ArgType			mType;
		int32_t			mOffset;
		uint32_t		mSize;
		bool			mNeedsEndianSwapForTransmit;
		
		friend class Message;
	};
	
	template<typename T>
	const Argument& getDataView( uint32_t index ) const;
	
	std::string				mAddress;
	ByteBuffer				mDataArray;
	std::vector<Argument>	mDataViews;
	mutable bool			mIsCached;
	mutable ByteBuffer		mCache;
	
	/// Create the OSC message and store it in cache.
	void createCache() const;
	bool bufferCache( uint8_t *data, size_t size );
	
	friend class Sender;
	friend class Bundle;
	friend class Receiver;
	friend std::ostream& operator<<( std::ostream &os, const Message &rhs );
};

	
std::ostream& operator<<( std::ostream &os, const Message &rhs );
	
}