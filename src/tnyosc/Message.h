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

namespace tnyosc {
	
/// This class represents an Open Sound Control message. It supports Open Sound
/// Control 1.0 and 1.1 specifications and extra non-standard arguments listed
/// in http://opensoundcontrol.org/spec-1_0.
class Message {
public:
	
	//! Create an OSC message. If address is not given, default OSC address is set
	//! to "/tnyosc".
	explicit Message( const std::string& address );
	
	~Message() {}
	
//	friend std::ostream& operator<<( std::ostream& s, const Message& msg )
//	{
//		return s << msg.toString().c_str();
//	}
	
	// Functions for appending OSC 1.0 types
	//! int32
	void append( int32_t v );
	//! float32
	void append( float v );
	//! OSC-string
	void append( const std::string& v );
	//! c-string array
	void append( const char* v, size_t len );
	//! OSC-blob
	void appendBlob( void* blob, uint32_t size );
	void appendBlob( const ci::Buffer &buffer );
	
	// Functions for appending OSC 1.1 types
	// OSC-timetag (NTP format)
	void appendTimeTag( uint64_t v );
	// appends the current UTP timestamp
	void appendCurrentTime() { appendTimeTag( get_current_ntp_time() ); }
	// 'T'(True) or 'F'(False)
	void append( bool v );
	// Null (or nil)
	void appendNull() { mIsCached = false; mDataViews.emplace_back( ArgType::NULL_T, -1, 0 ); }
	// Impulse (or Infinitum)
	void appendImpulse() { mIsCached = false; mDataViews.emplace_back( ArgType::INFINITUM, -1, 0 ); }
	
	// Functions for appending nonstandard types
	// int64
	void append( int64_t v );
	// float64 (or double)
	void append( double v );
	// ascii character
	void append( char v );
	// midi
	void appendMidi( uint8_t port, uint8_t status, uint8_t data1, uint8_t data2 );
	// array
	void appendArray( void* array, size_t size );
	
	int32_t		getInt( uint32_t index );
	float		getFloat( uint32_t index );
	std::string getString( uint32_t index );
	int64_t		getTime( uint32_t index );
	int64_t		getInt64( uint32_t index );
	double		getDouble( uint32_t index );
	bool		getBool( uint32_t index );
	char		getChar( uint32_t index );
	void		getMidi( uint32_t index, uint8_t *port, uint8_t *status, uint8_t *data1, uint8_t *data2 );
	ci::Buffer	getBlob( uint32_t index );
	
	
	class Argument {
	public:
		Argument( ArgType type, int32_t offset, uint32_t size );
		Argument( const Argument &arg ) = default;
		Argument( Argument &&arg ) = default;
		Argument& operator=( const Argument &arg ) = default;
		Argument& operator=( Argument &&arg ) = default;
		
		~Argument() = default;
		
		ArgType getArgType() const { return mType; }
		uint32_t getArgSize() const { return mSize; }
		int32_t getOffset() const { return mOffset; }
		
		char getChar() const;
		template<typename T>
		bool convertible() const;
		
	private:
		
		ArgType			mType;
		uint32_t		mSize;
		int32_t			mOffset;
		
		friend class Message;
	};
	
	/// Sets the OSC address of this message.
	/// @param[in] address The new OSC address.
	void setAddress( const std::string& address )
	{
		mIsCached = false;
		mAddress = address;
	}
	
	/// Returns the OSC address of this message.
	const std::string& getAddress() const { return mAddress; }
	
	/// Returns a complete byte array of this OSC message as a ByteArray type.
	/// The byte array is constructed lazily and is cached until the cache is
	/// obsolete. Call to |data| and |size| perform the same caching.
	///
	/// @return The OSC message as a ByteArray.
	/// @see data
	/// @see size
	const ByteBuffer& byteArray() const
	{
		if( mIsCached )
			return mCache;
		else
			return createCache();
	}
	
	/// Returns a complete byte array of this OSC message as a char
	/// pointer. This call is convenient for actually sending this OSC messager.
	///
	/// @return The OSC message as an char*.
	/// @see byte_array
	/// @see size
	///
	/// <pre>
	///   int sockfd; // initialize a socket...
	///   tnyosc::Message* msg; // create a OSC message...
	///   send_to(sockfd, msg->data(), msg->size(), 0);
	/// </pre>
	///
	const char* data() const { return byteArray().data(); }
	
	/// Returns the size of this OSC message.
	///
	/// @return Size of the OSC message in bytes.
	/// @see byte_array
	/// @see data
	size_t size() const { return byteArray().size(); }
	
	/// Clears the message.
	void clear()
	{
		mIsCached = false;
		mAddress.clear();
		mDataViews.clear();
		mDataArray.clear();
	}
	
private:
	static uint8_t getTrailingZeros( size_t bufferSize ) { return 4 - ( bufferSize % 4 ); }
	size_t getCurrentOffset() { return mDataArray.size() - 1; }
	
	template<typename T>
	Argument& getDataView( uint32_t index );
	
	std::string				mAddress;
	ByteBuffer				mDataArray;
	std::vector<Argument>	mDataViews;
	mutable bool			mIsCached;
	mutable ByteBuffer		mCache;
	
	/// Create the OSC message and store it in cache.
	const ByteBuffer& createCache() const;
	void bufferCache() const;
};
	
Message::Message( const std::string& address )
: mAddress( address ), mIsCached( false )
{
}
	
inline void Message::append( int32_t v )
{
	mIsCached = false;
	mDataViews.emplace_back( ArgType::INTEGER_32, getCurrentOffset(), 4 );
	int32_t a = htonl( v );
	ByteArray<4> b;
	memcpy( b.data(), &a, 4 );
	mDataArray.insert( mDataArray.end(), b.begin(), b.end() );
}
	
inline void Message::append( float v )
{
	mIsCached = false;
	mDataViews.emplace_back( ArgType::FLOAT, getCurrentOffset(), 4 );
	int32_t a = htonf( v );
	ByteArray<4> b;
	memcpy( b.data(), &a, 4 );
	mDataArray.insert( mDataArray.end(), b.begin(), b.end() );
}
	
inline void Message::append( const std::string& v )
{
	mIsCached = false;
	auto size = v.size() + getTrailingZeros( v.size() );
	mDataViews.emplace_back( ArgType::STRING, getCurrentOffset(), size );
	mDataArray.insert( mDataArray.end(), v.begin(), v.end() );
	mDataArray.resize( mDataArray.size() + 4 - ( v.size() % 4 ), 0 );
}

inline void Message::append( const char* v, size_t len )
{
	if( ! v || len == 0 ) return;
	mIsCached = false;
	auto size = len + getTrailingZeros( len );
	mDataViews.emplace_back( ArgType::STRING, getCurrentOffset(), size);
	ByteBuffer b( v, v + len );
	b.resize( size, 0 );
	mDataArray.insert( mDataArray.end(), b.begin(), b.end() );
}
	
inline void Message::appendBlob( void* blob, uint32_t size )
{
	mIsCached = false;
	auto totalBufferSize = 4 + size + getTrailingZeros( size );
	mDataViews.emplace_back( ArgType::BLOB, getCurrentOffset(), totalBufferSize );
	int32_t a = htonl( size );
	ByteBuffer b( totalBufferSize, 0 );
	std::copy( (uint8_t*) &a, (uint8_t*) &a + 4, b.begin() );
	std::copy( (uint8_t*) blob, (uint8_t*) blob + size, b.begin() + 4 );
	mDataArray.insert( mDataArray.end(), b.begin(), b.end() );
}
	
inline void Message::appendTimeTag( uint64_t v )
{
	mIsCached = false;
	mDataViews.emplace_back( ArgType::TIME_TAG, getCurrentOffset(), 8 );
	uint64_t a = htonll( v );
	ByteArray<8> b;
	memcpy( b.data(), &a, 8 );
	mDataArray.insert( mDataArray.end(), b.begin(), b.end() );
}
	
inline void Message::append( bool v )
{
	mIsCached = false;
	if( v )
		mDataViews.emplace_back( ArgType::BOOL_T, -1, 0 );
	else
		mDataViews.emplace_back( ArgType::BOOL_F, -1, 0 );
}
	
inline void Message::append( int64_t v )
{
	mIsCached = false;
	mDataViews.emplace_back( ArgType::INTEGER_64, getCurrentOffset(), 8 );
	int64_t a = htonll( v );
	ByteArray<8> b;
	memcpy( b.data(), &a, 8 );
	mDataArray.insert( mDataArray.end(), b.begin(), b.end() );
}
	
inline void Message::append( double v )
{
	mIsCached = false;
	mDataViews.emplace_back( ArgType::DOUBLE, getCurrentOffset(), 8 );
	int64_t a = htond( v );
	ByteArray<8> b;
	memcpy( b.data(), &a, 8 );
	mDataArray.insert( mDataArray.end(), b.begin(), b.end() );
}
	
inline void Message::append( char v )
{
	mIsCached = false;
	mDataViews.emplace_back( ArgType::CHAR, getCurrentOffset(), 4 );
	int32_t a = htonl( v );
	ByteArray<4> b;
	memcpy( b.data(), &a, 4 );
	mDataArray.insert( mDataArray.end(), b.begin(), b.end() );
}
	
inline void Message::appendMidi( uint8_t port, uint8_t status, uint8_t data1, uint8_t data2 )
{
	mIsCached = false;
	mDataViews.emplace_back( ArgType::MIDI, getCurrentOffset(), 4 );
	ByteArray<4> b;
	b[0] = port;
	b[1] = status;
	b[2] = data1;
	b[3] = data2;
	mDataArray.insert( mDataArray.end(), b.begin(), b.end() );
}
	
inline void Message::appendArray( void* array, size_t size )
{
	if( !array || size == 0 ) return;
	mIsCached = false;
//	mTypesArray.push_back( '[' );
//	mTypesArray.insert( mTypesArray.end(), (uint8_t*) &array, (uint8_t*) &array + size );
//	mTypesArray.push_back( ']' );
}
	
inline const ByteBuffer& Message::createCache() const
{
	mIsCached = true;
	std::string address( mAddress );
	size_t addressLen = address.size() + getTrailingZeros( address.size() );
	
	std::vector<char> typesArray( mDataViews.size() + getTrailingZeros( mDataViews.size() ) );
	int i = 0;
	for( auto & dataView : mDataViews ) {
		typesArray[i++] = dataView.getChar();
	}
	
	size_t typesArrayLen = typesArray.size();
	mCache.resize( addressLen + typesArrayLen + mDataArray.size() );
	std::copy( mAddress.begin(), mAddress.end(), mCache.begin() );
	std::copy( typesArray.begin(), typesArray.end(), mCache.begin() + addressLen );
	std::copy( mDataArray.begin(), mDataArray.end(), mCache.begin() + addressLen + typesArrayLen );
	return mCache;
}

template<typename T>
inline Message::Argument& Message::getDataView( uint32_t index )
{
	if( index >= mDataViews.size() )
		throw ExcIndexOutOfBounds( mAddress, index );
	
	auto &dataView = mDataViews[index];
	if( dataView.convertible<T>() ) // TODO: change the type to typeid print out.
		throw ExcNonConvertible( mAddress, index, ArgType::INTEGER_32, dataView.getArgType() );
	
	return dataView;
}
	
inline int32_t Message::getInt( uint32_t index )
{
	auto &dataView = getDataView<int32_t>( index );
	return *reinterpret_cast<int32_t*>(&mDataArray[dataView.getOffset()]);
}
	
inline float Message::getFloat( uint32_t index )
{
	auto &dataView = getDataView<float>( index );
	return *reinterpret_cast<float*>(&mDataArray[dataView.getOffset()]);
}
	
inline std::string Message::getString( uint32_t index )
{
	auto &dataView = getDataView<std::string>( index );
	const char* head = reinterpret_cast<const char*>(&mDataArray[dataView.getOffset()]);
	return std::string( head );
}
	
inline int64_t Message::getTime( uint32_t index )
{
	auto &dataView = getDataView<int64_t>( index );
	return *reinterpret_cast<int64_t*>(mDataArray[dataView.getOffset()]);
}

inline int64_t Message::getInt64( uint32_t index )
{
	auto &dataView = getDataView<int64_t>( index );
	return *reinterpret_cast<int64_t*>(mDataArray[dataView.getOffset()]);
}
	
inline double Message::getDouble( uint32_t index )
{
	auto &dataView = getDataView<double>( index );
	return *reinterpret_cast<double*>(mDataArray[dataView.getOffset()]);
}
	
inline bool	Message::getBool( uint32_t index )
{
	auto &dataView = getDataView<bool>( index );
	return dataView.getArgType() == ArgType::BOOL_T;
}
	
inline char	Message::getChar( uint32_t index )
{
	auto &dataView = getDataView<int32_t>( index );
	return mDataArray[dataView.getOffset()];
}
	
inline void	Message::getMidi( uint32_t index, uint8_t *port, uint8_t *status, uint8_t *data1, uint8_t *data2 )
{
	auto &dataView = getDataView<int32_t>( index );
	int32_t midiVal = *reinterpret_cast<int32_t*>(&mDataArray[dataView.getOffset()]);
	*port = midiVal;
	*status = midiVal >> 8;
	*data1 = midiVal >> 16;
	*data2 = midiVal >> 24;
}
	
inline ci::Buffer Message::getBlob( uint32_t index )
{
	auto &dataView = getDataView<ci::Buffer>( index );
	ci::Buffer ret( dataView.getArgSize() );
	uint8_t* data = reinterpret_cast<uint8_t*>( &mDataArray[dataView.getOffset()] );
	memcpy( ret.getData(), data, dataView.getArgSize() );
	return ret;
}
	
}