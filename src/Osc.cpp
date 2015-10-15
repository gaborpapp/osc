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

#include "Osc.h"
#include "cinder/Log.h"

using namespace std;
using namespace ci;
using namespace ci::app;
using namespace asio;
using namespace asio::ip;
using namespace std::placeholders;

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
	
static int64_t sTimeOffsetSecs = 0;
static int64_t sTimeOffsetUsecs = 0;
	
/// Convert 32-bit float to a big-endian network format
inline int32_t htonf( float x ) { return (int32_t) htonl( *(int32_t*) &x ); }
/// Convert 64-bit float (double) to a big-endian network format
inline int64_t htond( double x ) { return (int64_t) htonll( *(int64_t*) &x ); }
/// Convert 32-bit big-endian network format to float
inline double ntohf( int32_t x ) { x = ntohl( x ); return *(float*) &x; }
/// Convert 64-bit big-endian network format to double
inline double ntohd( int64_t x ) { return (double) ntohll( x ); }
	
////////////////////////////////////////////////////////////////////////////////////////
//// MESSAGE
	
Message::Message( const std::string& address )
: mAddress( address ), mIsCached( false )
{
}
	
Message::Message( Message &&message ) noexcept
: mAddress( move( message.mAddress ) ), mDataBuffer( move( message.mDataBuffer ) ),
	mDataViews( move( message.mDataViews ) ), mIsCached( message.mIsCached ),
	mCache( move( message.mCache ) )
{
	for( auto & dataView : mDataViews ) {
		dataView.mOwner = this;
	}
}

Message& Message::operator=( Message &&message ) noexcept
{
	if( this != &message ) {
		mAddress = move( message.mAddress );
		mDataBuffer = move( message.mDataBuffer );
		mDataViews = move( message.mDataViews );
		mIsCached = message.mIsCached;
		mCache = move( message.mCache );
		for( auto & dataView : mDataViews ) {
			dataView.mOwner = this;
		}
	}
	return *this;
}
	
using Argument = Message::Argument;

Argument::Argument()
: mOwner( nullptr ), mType( ArgType::NULL_T ), mSize( 0 ), mOffset( -1 )
{
}

Argument::Argument( Message *owner, ArgType type, int32_t offset, uint32_t size, bool needsSwap )
: mOwner( owner ), mType( type ), mOffset( offset ),
	mSize( size ), mNeedsEndianSwapForTransmit( needsSwap )
{
}
	
Argument::Argument( Argument &&arg ) noexcept
: mOwner( arg.mOwner ), mType( arg.mType ), mOffset( arg.mOffset ),
	mSize( arg.mSize ), mNeedsEndianSwapForTransmit( arg.mNeedsEndianSwapForTransmit )
{
}
	
Argument& Argument::operator=( Argument &&arg ) noexcept
{
	if( this != &arg ) {
		mOwner = arg.mOwner;
		mType = arg.mType;
		mOffset = arg.mOffset;
		mSize = arg.mSize;
		mNeedsEndianSwapForTransmit = arg.mNeedsEndianSwapForTransmit;
	}
	return *this;
}

ArgType Argument::translateCharToArgType( char type )
{
	switch ( type ) {
		case 'i': return ArgType::INTEGER_32; break;
		case 'f': return ArgType::FLOAT; break;
		case 's': return ArgType::STRING; break;
		case 'b': return ArgType::BLOB; break;
		case 'h': return ArgType::INTEGER_64; break;
		case 't': return ArgType::TIME_TAG; break;
		case 'd': return ArgType::DOUBLE; break;
		case 'c': return ArgType::CHAR; break;
		case 'm': return ArgType::MIDI; break;
		case 'T': return ArgType::BOOL_T; break;
		case 'F': return ArgType::BOOL_F; break;
		case 'N': return ArgType::NULL_T; break;
		case 'I': return ArgType::INFINITUM; break;
		default: return ArgType::NULL_T; break;
	}
}

char Argument::translateArgTypeToChar( ArgType type )
{
	switch ( type ) {
		case ArgType::INTEGER_32: return 'i'; break;
		case ArgType::FLOAT: return 'f'; break;
		case ArgType::STRING: return 's'; break;
		case ArgType::BLOB: return 'b'; break;
		case ArgType::INTEGER_64: return 'h'; break;
		case ArgType::TIME_TAG: return 't'; break;
		case ArgType::DOUBLE: return 'd'; break;
		case ArgType::CHAR: return 'c'; break;
		case ArgType::MIDI: return 'm'; break;
		case ArgType::BOOL_T: return 'T'; break;
		case ArgType::BOOL_F: return 'F'; break;
		case ArgType::NULL_T: return 'N'; break;
		case ArgType::INFINITUM: return 'I'; break;
		case ArgType::NONE: return 'N'; break;
	}
}

void Argument::swapEndianForTransmit( uint8_t *buffer ) const
{
	auto ptr = &buffer[mOffset];
	switch ( mType ) {
		case ArgType::INTEGER_32:
		case ArgType::CHAR:
		case ArgType::BLOB: {
			int32_t a = htonl( *reinterpret_cast<int32_t*>(ptr) );
			memcpy( ptr, &a, 4 );
		}
			break;
		case ArgType::INTEGER_64:
		case ArgType::TIME_TAG: {
			uint64_t a = htonll( *reinterpret_cast<uint64_t*>(ptr) );
			memcpy( ptr, &a, 8 );
		}
			break;
		case ArgType::FLOAT: {
			int32_t a = htonf( *reinterpret_cast<float*>(ptr) );
			memcpy( ptr, &a, 4 );
		}
			break;
		case ArgType::DOUBLE: {
			int64_t a = htond( *reinterpret_cast<double*>(ptr) );
			memcpy( ptr, &a, 8 );
		}
			break;
		default: break;
	}
}

void Argument::outputValueToStream( std::ostream &ostream ) const
{
	auto ptr = &mOwner->mDataBuffer[mOffset];
	switch ( mType ) {
		case ArgType::INTEGER_32: ostream << *reinterpret_cast<int32_t*>( ptr ); break;
		case ArgType::FLOAT: ostream << *reinterpret_cast<float*>( ptr ); break;
		case ArgType::STRING: ostream << *reinterpret_cast<const char*>( ptr ); break;
		case ArgType::BLOB: ostream << "Size: " << *reinterpret_cast<int32_t*>( ptr ); break;
		case ArgType::INTEGER_64: ostream << *reinterpret_cast<int64_t*>( ptr ); break;
		case ArgType::TIME_TAG: /*ostream << *reinterpret_cast<int64_t*>( ptr )*/; break;
		case ArgType::DOUBLE: ostream << *reinterpret_cast<double*>( ptr ); break;
		case ArgType::CHAR: {
			char v = *reinterpret_cast<char*>( ptr );
			ostream << int(v);
		}
			break;
		case ArgType::MIDI: {
			ostream <<	"Port"		<< int( *( ptr + 0 ) ) <<
			" Status: " << int( *( ptr + 1 ) ) <<
			" Data1: "  << int( *( ptr + 2 ) ) <<
			" Data2: "  << int( *( ptr + 3 ) ) ;
		}
			break;
		case ArgType::BOOL_T: ostream << "True"; break;
		case ArgType::BOOL_F: ostream << "False"; break;
		case ArgType::NULL_T: ostream << "Null"; break;
		case ArgType::INFINITUM: ostream << "Infinitum"; break;
		default: ostream << "Unknown"; break;
	}
}

void Message::append( int32_t v )
{
	mIsCached = false;
	mDataViews.emplace_back( this, ArgType::INTEGER_32, getCurrentOffset(), 4, true );
	ByteArray<4> b;
	memcpy( b.data(), &v, 4 );
	mDataBuffer.insert( mDataBuffer.end(), b.begin(), b.end() );
}

void Message::append( float v )
{
	mIsCached = false;
	mDataViews.emplace_back( this, ArgType::FLOAT, getCurrentOffset(), 4, true );
	ByteArray<4> b;
	memcpy( b.data(), &v, 4 );
	mDataBuffer.insert( mDataBuffer.end(), b.begin(), b.end() );
}

void Message::append( const std::string& v )
{
	mIsCached = false;
	auto size = v.size() + getTrailingZeros( v.size() );
	mDataViews.emplace_back( this, ArgType::STRING, getCurrentOffset(), size );
	mDataBuffer.insert( mDataBuffer.end(), v.begin(), v.end() );
	mDataBuffer.resize( mDataBuffer.size() + getTrailingZeros( v.size() ), 0 );
}

void Message::append( const char* v, size_t len )
{
	if( ! v || len == 0 ) return;
	mIsCached = false;
	auto size = len + getTrailingZeros( len );
	mDataViews.emplace_back( this, ArgType::STRING, getCurrentOffset(), size );
	ByteBuffer b( v, v + len );
	b.resize( size, 0 );
	mDataBuffer.insert( mDataBuffer.end(), b.begin(), b.end() );
}

void Message::appendBlob( void* blob, uint32_t size )
{
	mIsCached = false;
	auto totalBufferSize = 4 + size + getTrailingZeros( size );
	mDataViews.emplace_back( this, ArgType::BLOB, getCurrentOffset(), totalBufferSize, true );
	ByteBuffer b( totalBufferSize, 0 );
	memcpy( b.data(), &size, 4 );
	memcpy( b.data() + 4, blob, size );
	mDataBuffer.insert( mDataBuffer.end(), b.begin(), b.end() );
}

void Message::appendBlob( const ci::Buffer &buffer )
{
	appendBlob( (void*)buffer.getData(), buffer.getSize() );
}

void Message::appendTimeTag( uint64_t v )
{
	mIsCached = false;
	mDataViews.emplace_back( this, ArgType::TIME_TAG, getCurrentOffset(), 8, true );
	ByteArray<8> b;
	memcpy( b.data(), &v, 8 );
	mDataBuffer.insert( mDataBuffer.end(), b.begin(), b.end() );
}
	
void Message::appendCurrentTime()
{
	appendTimeTag( time::get_current_ntp_time() );
}

void Message::append( bool v )
{
	mIsCached = false;
	if( v )
		mDataViews.emplace_back( this, ArgType::BOOL_T, -1, 0 );
	else
		mDataViews.emplace_back( this, ArgType::BOOL_F, -1, 0 );
}

void Message::append( int64_t v )
{
	mIsCached = false;
	mDataViews.emplace_back( this, ArgType::INTEGER_64, getCurrentOffset(), 8, true );
	ByteArray<8> b;
	memcpy( b.data(), &v, 8 );
	mDataBuffer.insert( mDataBuffer.end(), b.begin(), b.end() );
}

void Message::append( double v )
{
	mIsCached = false;
	mDataViews.emplace_back( this, ArgType::DOUBLE, getCurrentOffset(), 8, true );
	ByteArray<8> b;
	memcpy( b.data(), &v, 8 );
	mDataBuffer.insert( mDataBuffer.end(), b.begin(), b.end() );
}

void Message::append( char v )
{
	mIsCached = false;
	mDataViews.emplace_back( this, ArgType::CHAR, getCurrentOffset(), 4, true );
	ByteArray<4> b;
	b.fill( 0 );
	b[0] = v;
	mDataBuffer.insert( mDataBuffer.end(), b.begin(), b.end() );
}

void Message::appendMidi( uint8_t port, uint8_t status, uint8_t data1, uint8_t data2 )
{
	mIsCached = false;
	mDataViews.emplace_back( this, ArgType::MIDI, getCurrentOffset(), 4 );
	ByteArray<4> b;
	b[0] = port;
	b[1] = status;
	b[2] = data1;
	b[3] = data2;
	mDataBuffer.insert( mDataBuffer.end(), b.begin(), b.end() );
}

//void Message::appendArray( void* array, size_t size )
//{
//	if( !array || size == 0 ) return;
//	mIsCached = false;
//	mTypesArray.push_back( '[' );
//	mTypesArray.insert( mTypesArray.end(), (uint8_t*) &array, (uint8_t*) &array + size );
//	mTypesArray.push_back( ']' );
//}

void Message::createCache() const
{
	std::string address( mAddress );
	size_t addressLen = address.size() + getTrailingZeros( address.size() );
	std::vector<uint8_t> dataArray( mDataBuffer );
	std::vector<char> typesArray( mDataViews.size() + getTrailingZeros( mDataViews.size() ), 0 );
	
	typesArray[0] = ',';
	int i = 1;
	for( auto & dataView : mDataViews ) {
		typesArray[i++] = Argument::translateArgTypeToChar( dataView.getArgType() );
		if( dataView.needsEndianSwapForTransmit() )
			dataView.swapEndianForTransmit( reinterpret_cast<uint8_t*>( dataArray.data() ) );
	}
	
	if( ! mCache )
		mCache = ByteBufferRef( new ByteBuffer() );
	
	size_t typesArrayLen = typesArray.size();
	ByteArray<4> sizeArray;
	int32_t messageSize = addressLen + typesArrayLen + mDataBuffer.size();
	auto endianSize = htonl( messageSize );
	memcpy( sizeArray.data(), reinterpret_cast<uint8_t*>( &endianSize ), 4 );
	
	mCache->resize( 4 + messageSize );
	
	std::copy( sizeArray.begin(),	sizeArray.end(),	mCache->begin() );
	std::copy( mAddress.begin(),	mAddress.end(),		mCache->begin() + 4 );
	std::copy( typesArray.begin(),	typesArray.end(),	mCache->begin() + 4 + addressLen );
	std::copy( dataArray.begin(),	dataArray.end(),	mCache->begin() + 4 + addressLen + typesArrayLen );
	mIsCached = true;
}

ByteBufferRef Message::getSharedBuffer() const
{
	if( ! mIsCached )
		createCache();
	return mCache;
}

template<typename T>
const Argument& Message::getDataView( uint32_t index ) const
{
	if( index >= mDataViews.size() )
		throw ExcIndexOutOfBounds( mAddress, index );
	
	return mDataViews[index];
}
	
const Argument& Message::operator[]( uint32_t index ) const
{
	if( index >= mDataViews.size() )
		throw ExcIndexOutOfBounds( mAddress, index );
	
	return mDataViews[index];
}

int32_t	Argument::int32() const
{
	if( ! convertible<int32_t>() ) // TODO: change the type to typeid print out.
		throw ExcNonConvertible( mOwner->getAddress(), ArgType::INTEGER_32, getArgType() );
	
	return *reinterpret_cast<const int32_t*>(&mOwner->mDataBuffer[getOffset()]);;
}
	
int64_t	Argument::int64() const
{
	if( ! convertible<int64_t>() )
		throw ExcNonConvertible( mOwner->getAddress(), ArgType::INTEGER_64, getArgType() );
	
	return *reinterpret_cast<const int64_t*>(&mOwner->mDataBuffer[getOffset()]);;
}
	
float Argument::flt() const
{
	if( ! convertible<float>() )
		throw ExcNonConvertible( mOwner->getAddress(), ArgType::FLOAT, getArgType() );
	
	return *reinterpret_cast<const float*>(&mOwner->mDataBuffer[getOffset()]);;
}
	
double Argument::dbl() const
{
	if( ! convertible<double>() )
		throw ExcNonConvertible( mOwner->getAddress(), ArgType::DOUBLE, getArgType() );
	
	return *reinterpret_cast<const double*>(&mOwner->mDataBuffer[getOffset()]);;
}
	
bool Argument::boolean() const
{
	if( ! convertible<bool>() )
		throw ExcNonConvertible( mOwner->getAddress(), ArgType::BOOL_T, getArgType() );
	
	return getArgType() == ArgType::BOOL_T;
}
	
void Argument::midi( uint8_t *port, uint8_t *status, uint8_t *data1, uint8_t *data2 ) const
{
	if( ! convertible<int32_t>() )
		throw ExcNonConvertible( mOwner->getAddress(), ArgType::MIDI, getArgType() );
	
	int32_t midiVal = *reinterpret_cast<const int32_t*>(&mOwner->mDataBuffer[getOffset()]);
	*port = midiVal;
	*status = midiVal >> 8;
	*data1 = midiVal >> 16;
	*data2 = midiVal >> 24;
}
	
ci::Buffer Argument::blob( bool deepCopy ) const
{
	if( ! convertible<ci::Buffer>() )
		throw ExcNonConvertible( mOwner->getAddress(), ArgType::BLOB, getArgType() );
	
	// skip the first 4 bytes, as they are the size
	const uint8_t* data = reinterpret_cast<const uint8_t*>( &mOwner->mDataBuffer[getOffset()+4] );
	if( deepCopy ) {
		ci::Buffer ret( getArgSize() );
		memcpy( ret.getData(), data, getArgSize() );
		return ret;
	}
	else {
		return ci::Buffer( reinterpret_cast<void*>(  const_cast<uint8_t*>( data ) ), getArgSize() );
	}
}
	
char Argument::character() const
{
	if( ! convertible<int32_t>() )
		throw ExcNonConvertible( mOwner->getAddress(), ArgType::CHAR, getArgType() );
	
	return mOwner->mDataBuffer[getOffset()];
}
	
std::string Argument::string() const
{
	if( ! convertible<std::string>() )
		throw ExcNonConvertible( mOwner->getAddress(), ArgType::STRING, getArgType() );
	
	const char* head = reinterpret_cast<const char*>(&mOwner->mDataBuffer[getOffset()]);
	return std::string( head );
}

template<typename T>
bool Argument::convertible() const
{
	switch ( mType ) {
		case ArgType::INTEGER_32: return std::is_same<T, int32_t>::value;
		case ArgType::FLOAT: return std::is_same<T, float>::value;
		case ArgType::STRING: return std::is_same<T, std::string>::value;
		case ArgType::BLOB: return std::is_same<T, ci::Buffer>::value;
		case ArgType::INTEGER_64: return std::is_same<T, int64_t>::value;
		case ArgType::TIME_TAG: return std::is_same<T, int64_t>::value;
		case ArgType::DOUBLE: return std::is_same<T, double>::value;
		case ArgType::CHAR: return std::is_same<T, int32_t>::value;
		case ArgType::MIDI: return std::is_same<T, int32_t>::value;
		case ArgType::BOOL_T: return std::is_same<T, bool>::value;
		case ArgType::BOOL_F: return std::is_same<T, bool>::value;
		case ArgType::NULL_T: return false;
		case ArgType::INFINITUM: return false;
		case ArgType::NONE: return false;
		default: return false;
	}
}

ArgType Message::getArgType( uint32_t index ) const
{
	if( index >= mDataViews.size() )
		throw ExcIndexOutOfBounds( mAddress, index );
	
	auto &dataView = mDataViews[index];
	return dataView.getArgType();
}

int32_t Message::getInt( uint32_t index ) const
{
	auto &dataView = getDataView<int32_t>( index );
	return dataView.int32();
}

float Message::getFloat( uint32_t index ) const
{
	auto &dataView = getDataView<float>( index );
	return dataView.flt();
}

std::string Message::getString( uint32_t index ) const
{
	auto &dataView = getDataView<std::string>( index );
	return dataView.string();
}

int64_t Message::getTime( uint32_t index ) const
{
	auto &dataView = getDataView<int64_t>( index );
	return dataView.int64();
}

int64_t Message::getInt64( uint32_t index ) const
{
	auto &dataView = getDataView<int64_t>( index );
	return dataView.int64();
}

double Message::getDouble( uint32_t index ) const
{
	auto &dataView = getDataView<double>( index );
	return dataView.dbl();
}

bool Message::getBool( uint32_t index ) const
{
	auto &dataView = getDataView<bool>( index );
	return dataView.boolean();
}

char Message::getChar( uint32_t index ) const
{
	auto &dataView = getDataView<int32_t>( index );
	return dataView.character();
}

void Message::getMidi( uint32_t index, uint8_t *port, uint8_t *status, uint8_t *data1, uint8_t *data2 ) const
{
	auto &dataView = getDataView<int32_t>( index );
	dataView.midi( port, status, data1, data2 );
}

ci::Buffer Message::getBlob( uint32_t index, bool deepCopy ) const
{
	auto &dataView = getDataView<ci::Buffer>( index );
	return dataView.blob( deepCopy );
}

bool Message::bufferCache( uint8_t *data, size_t size )
{
	uint8_t *head, *tail;
	uint32_t i = 0;
	size_t remain = size;
	
	// extract address
	head = tail = data;
	while( tail[i] != '\0' && ++i < remain );
	if( i == remain ) return false;
	
	mAddress.insert( 0, (char*)head, i );
	
	head += i + getTrailingZeros( i );
	remain = size - ( head - data );
	
	i = 0;
	tail = head;
	if( head[i++] != ',' ) return false;
	
	// extract types
	while( tail[i] != '\0' && ++i < remain );
	if( i == remain ) return false;
	
	std::vector<char> types( i - 1 );
	std::copy( head + 1, head + i, types.begin() );
	head += i + getTrailingZeros( i );
	remain = size - ( head - data );
	
	// extract data
	uint32_t int32;
	uint64_t int64;
	
	mDataViews.resize( types.size() );
	int j = 0;
	for( auto & dataView : mDataViews ) {
		dataView.mOwner = this;
		dataView.mType = Argument::translateCharToArgType( types[j] );
		switch( types[j] ) {
			case 'i':
			case 'f':
			case 'r': {
				dataView.mSize = 4;
				dataView.mOffset = getCurrentOffset();
				memcpy( &int32, head, 4 );
				int32 = htonl( int32 );
				ByteArray<4> v;
				memcpy( v.data(), &int32, 4 );
				mDataBuffer.insert( mDataBuffer.end(), v.begin(), v.end() );
				head += 4;
				remain -= 4;
			}
				break;
			case 'b': {
				memcpy( &int32, head, 4 );
				head += 4;
				remain -= 4;
				int32 = htonl( int32 );
				if( int32 > remain ) return false;
				dataView.mSize = int32;
				dataView.mOffset = getCurrentOffset();
				mDataBuffer.resize( mDataBuffer.size() + 4 + int32 );
				memcpy( &mDataBuffer[dataView.mOffset], &int32, 4 );
				memcpy( &mDataBuffer[dataView.mOffset + 4], head, int32 );
				head += int32 + 4;
				remain -= int32;
			}
				break;
			case 's':
			case 'S': {
				tail = head;
				i = 0;
				while( tail[i] != '\0' && ++i < remain );
				dataView.mSize = i;
				dataView.mOffset = getCurrentOffset();
				mDataBuffer.resize( mDataBuffer.size() + i + 1 );
				memcpy( &mDataBuffer[dataView.mOffset], head, i + 1 );
				mDataBuffer[mDataBuffer.size() - 1] = '\0';
				i += getTrailingZeros( i );
				head += i;
				remain -= i;
			}
				break;
			case 'h':
			case 'd':
			case 't': {
				memcpy( &int64, head, 8 );
				int64 = htonll( int64 );
				dataView.mSize = i;
				dataView.mOffset = getCurrentOffset();
				ByteArray<8> v;
				memcpy( v.data(), &int64, 8 );
				mDataBuffer.insert( mDataBuffer.end(), v.begin(), v.end() );
				head += 8;
				remain -= 8;
			}
				break;
			case 'c': {
				dataView.mSize = 4;
				dataView.mOffset = getCurrentOffset();
				memcpy( &int32, head, 4 );
				mDataBuffer.push_back( (char) htonl( int32 ) );
				head += 4;
				remain -= 8;
			}
				break;
			case 'm': {
				dataView.mSize = 4;
				dataView.mOffset = getCurrentOffset();
				std::array<uint8_t, 4> v;
				memcpy( v.data(), head, 4 );
				mDataBuffer.insert( mDataBuffer.end(), v.begin(), v.end() );
				head += 4;
				remain -= 4;
			}
				break;
		}
		j++;
	}
	
	return true;
}

bool Message::compareTypes( const std::string &types )
{
	int i = 0;
	for( auto & dataView : mDataViews ) {
		if( Argument::translateArgTypeToChar( dataView.getArgType() ) != types[i++] )
			return false;
	}
	return true;
}

void Message::setAddress( const std::string& address )
{
	mIsCached = false;
	mAddress = address;
}

size_t Message::size() const
{
	if( ! mIsCached )
		createCache();
	return mCache->size();
}

void Message::clear()
{
	mIsCached = false;
	mAddress.clear();
	mDataViews.clear();
	mDataBuffer.clear();
	mCache.reset();
}

std::ostream& operator<<( std::ostream &os, const Message &rhs )
{
	os << "Address: " << rhs.getAddress() << std::endl;
	for( auto &dataView : rhs.mDataViews ) {
		os << "\t<" << argTypeToString( dataView.getArgType() ) << ">: ";
		dataView.outputValueToStream( os );
		os << std::endl;
	}
	return os;
}
	
////////////////////////////////////////////////////////////////////////////////////////
//// Bundle

Bundle::Bundle()
: mDataBuffer( new std::vector<uint8_t>( 20 ) )
{
	static std::string id = "#bundle";
	std::copy( id.begin(), id.end(), mDataBuffer->begin() + 4 );
	(*mDataBuffer)[19] = 1;
}

void Bundle::setTimetag( uint64_t ntp_time )
{
	uint64_t a = htonll( ntp_time );
	ByteArray<8> b;
	memcpy( b.data(), reinterpret_cast<uint8_t*>( &a ), 8 );
	mDataBuffer->insert( mDataBuffer->begin() + 12, b.begin(), b.end() );
}

void Bundle::appendData( const ByteBufferRef& data )
{
	int32_t a = htonl( data->size() );
	ByteArray<4> b;
	memcpy( b.data(), reinterpret_cast<uint8_t*>(&a), 4 );
	mDataBuffer->insert( mDataBuffer->end(), b.begin(), b.end() );
	mDataBuffer->insert( mDataBuffer->end(), data->begin(), data->end() );
}

ByteBufferRef Bundle::getSharedBuffer() const
{
	int32_t a = htonl( size() );
	memcpy( mDataBuffer->data(), reinterpret_cast<uint8_t*>( &a ), 4 );
	return mDataBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////
//// SenderUdp

SenderUdp::SenderUdp( uint16_t localPort, const std::string &destinationHost, uint16_t destinationPort, const protocol &protocol, asio::io_service &service )
: mSocket( new udp::socket( service, udp::endpoint( protocol, localPort ) ) ),
	mRemoteEndpoint( udp::endpoint( address::from_string( destinationHost ), destinationPort ) )
{
}
	
SenderUdp::SenderUdp( uint16_t localPort, const protocol::endpoint &destination, const protocol &protocol, asio::io_service &service )
: mSocket( new udp::socket( service, udp::endpoint( protocol, localPort ) ) ),
	mRemoteEndpoint( destination )
{
}
	
SenderUdp::SenderUdp( const UdpSocketRef &socket, const protocol::endpoint &destination )
: mSocket( socket ), mRemoteEndpoint( destination )
{
}
	
void SenderUdp::sendImpl( const ByteBufferRef &data )
{
	// TODO: make sure this math works. we're getting rid of the first integer which is size.
	mSocket->async_send_to( asio::buffer( data->data() + 4, data->size() - 4 ), mRemoteEndpoint,
	// copy data pointer to persist the asynchronous send
	[&, data]( const asio::error_code& error, size_t bytesTransferred ) {
		// TODO: Figure out what to do with errors
		if( error ) {
			// derive oscAddress
			std::string oscAddress;
			if( ! data->empty() )
				oscAddress = ( (const char*)(data.get() + 4) );
			
//			if( mErrorHandler ) {
//				mErrorHandler( error.message(), mRemoteEndpoint.address().to_string(), oscAddress );
//			}
//			else
			// TODO: Figure out what to do with errors
				CI_LOG_E( error.message() << ", didn't send message [" << oscAddress << "] to " << mRemoteEndpoint.address().to_string() );
		}
		else {
			//		cout << "SENDER: bytestransferred: " << bytesTransferred << " | bytesToTransfer: " << byte_buffer->size() << endl;
		}
	});
}
	
////////////////////////////////////////////////////////////////////////////////////////
//// SenderTcp

SenderTcp::SenderTcp( uint16_t localPort, const string &destinationHost, uint16_t destinationPort, const protocol &protocol, io_service &service )
: mSocket( new tcp::socket( service, tcp::endpoint( protocol, localPort ) ) ),
	mRemoteEndpoint( tcp::endpoint( address::from_string( destinationHost ), destinationPort ) )
{
	
}
	
SenderTcp::SenderTcp( uint16_t localPort, const protocol::endpoint &destination, const protocol &protocol, io_service &service )
: mSocket( new tcp::socket( service, tcp::endpoint( protocol, localPort ) ) ),
	mRemoteEndpoint( destination )

{
	
}

SenderTcp::SenderTcp( const TcpSocketRef &socket, const protocol::endpoint &destination )
: mSocket( socket ), mRemoteEndpoint( destination )
{
	
}
	
void SenderTcp::connect()
{
	mSocket->async_connect( mRemoteEndpoint,
	[&]( const error_code &error ){
		if( error )
			CI_LOG_E( error.message() );
	});
}

void SenderTcp::sendImpl( const ByteBufferRef &data )
{
	mSocket->async_send( asio::buffer( *data ),
	[&, data]( const asio::error_code& error, size_t bytesTransferred ) {
		// TODO: Figure out what to do with errors
		if( error ) {
			// derive oscAddress
			std::string oscAddress;
			if( ! data->empty() )
				oscAddress = ( (const char*)(data.get() + 4) );
			
			//			if( mErrorHandler ) {
			//				mErrorHandler( error.message(), mRemoteEndpoint.address().to_string(), oscAddress );
			//			}
			//			else
			// TODO: Figure out what to do with errors
			CI_LOG_E( error.message() << ", didn't send message [" << oscAddress << "] to " << mRemoteEndpoint.address().to_string() );
		}
		else {
			//		cout << "SENDER: bytestransferred: " << bytesTransferred << " | bytesToTransfer: " << byte_buffer->size() << endl;
		}
	});
}
	
/////////////////////////////////////////////////////////////////////////////////////////
//// Decode
	
namespace decode {

// TODO: Better error handling here.
bool decodeData( uint8_t *data, uint32_t size, std::vector<Message> &messages, uint64_t timetag )
{
	if( ! memcmp( data, "#bundle\0", 8 ) ) {
		data += 8; size -= 8;
		
		uint64_t timestamp;
		memcpy( &timestamp, data, 8 ); data += 8; size -= 8;
		
		while( size != 0 ) {
			uint32_t seg_size;
			memcpy( &seg_size, data, 4 ); data += 4; size -= 4;
			seg_size = ntohl( seg_size );
			if( seg_size > size ) return false;
			// Need to move the data pointer for bundles up 4 because when attaching data to a bundle, I don't know what protocol is being used.
			if( !decodeData( data + 4, seg_size - 4, messages, ntohll( timestamp ) ) ) return false;
			data += seg_size; size -= seg_size;
		}
	}
	else {
		if( ! decodeMessage( data, size, messages, timetag ) ) return false;
	}
	
	return true;
}

bool decodeMessage( uint8_t *data, uint32_t size, std::vector<Message> &messages, uint64_t timetag )
{
	//	ParsedMessage m{ timetag, Message( std::string("") ) };
	Message message;
	if( ! message.bufferCache( data, size ) )
		return false;
	
	messages.push_back( std::move( message ) );
	return true;
}


bool patternMatch( const std::string& lhs, const std::string& rhs )
{
	bool negate = false;
	bool mismatched = false;
	std::string::const_iterator seq_tmp;
	std::string::const_iterator seq = lhs.begin();
	std::string::const_iterator seq_end = lhs.end();
	std::string::const_iterator pattern = rhs.begin();
	std::string::const_iterator pattern_end = rhs.end();
	while( seq != seq_end && pattern != pattern_end ) {
		switch( *pattern ) {
			case '?':
				break;
			case '*': {
				// if * is the last pattern, return true
				if( ++pattern == pattern_end ) return true;
				while( *seq != *pattern && seq != seq_end ) ++seq;
				// if seq reaches to the end without matching pattern
				if( seq == seq_end ) return false;
			}
				break;
			case '[': {
				negate = false;
				mismatched = false;
				if( *( ++pattern ) == '!' ) {
					negate = true;
					++pattern;
				}
				if( *( pattern + 1 ) == '-' ) {
					// range matching
					char c_start = *pattern; ++pattern;
					//assert(*pattern == '-');
					char c_end = *( ++pattern ); ++pattern;
					//assert(*pattern == ']');
					// swap c_start and c_end if c_start is larger
					if( c_start > c_end ) {
						char tmp = c_start;
						c_end = c_start;
						c_start = tmp;
					}
					mismatched = ( c_start <= *seq && *seq <= c_end ) ? negate : !negate;
					if( mismatched ) return false;
				}
				else {
					// literal matching
					while( *pattern != ']' ) {
						if( *seq == *pattern ) {
							mismatched = negate;
							break;
						}
						++pattern;
					}
					if( mismatched ) return false;
					while( *pattern != ']' ) ++pattern;
				}
			}
				break;
			case '{': {
				seq_tmp = seq;
				mismatched = true;
				while( *( ++pattern ) != '}' ) {
					// this assumes that there's no sequence like "{,a}" where ',' is
					// follows immediately after '{', which is illegal.
					if( *pattern == ',' ) {
						mismatched = false;
						break;
					}
					else if( *seq != *pattern ) {
						// fast forward to the next ',' or '}'
						while( *( ++pattern ) != ',' && *pattern != '}' );
						if( *pattern == '}' ) return false;
						// redo seq matching
						seq = seq_tmp;
						mismatched = true;
					}
					else {
						// matched
						++seq;
						mismatched = false;
					}
				}
				if( mismatched ) return false;
				while( *pattern != '}' ) ++pattern;
				--seq;
			}
				break;
			default: // non-special character
				if( *seq != *pattern ) return false;
				break;
		}
		++seq; ++pattern;
	}
	if( seq == seq_end && pattern == pattern_end ) return true;
	else return false;
}
	
} // decode
	
/////////////////////////////////////////////////////////////////////////////////////////
//// ReceiverBase
	
void ReceiverBase::setListener( const std::string &address, Listener listener )
{
	std::lock_guard<std::mutex> lock( mListenerMutex );
	auto foundListener = std::find_if( mListeners.begin(), mListeners.end(),
									  [address]( const std::pair<std::string, Listener> &listener ) {
										  return address == listener.first;
									  });
	if( foundListener != mListeners.end() ) {
		foundListener->second = listener;
	}
	else {
		mListeners.push_back( { address, listener } );
	}
}

void ReceiverBase::removeListener( const std::string &address )
{
	std::lock_guard<std::mutex> lock( mListenerMutex );
	auto foundListener = std::find_if( mListeners.begin(), mListeners.end(),
									  [address]( const std::pair<std::string, Listener> &listener ) {
										  return address == listener.first;
									  });
	if( foundListener != mListeners.end() ) {
		mListeners.erase( foundListener );
	}
}
	
uint64_t ReceiverBase::getClock()
{
	uint64_t ntp_time = time::get_current_ntp_time();
	
	uint64_t secs = ( ntp_time >> 32 ) + sTimeOffsetSecs;
	int64_t usecs = ( ntp_time & uint32_t( ~0 ) ) + sTimeOffsetUsecs;
	
	if( usecs < 0 ) {
		secs += usecs / 1000000;
		usecs += ( usecs / 1000000 ) * 1000000;
	}
	else {
		secs += usecs / 1000000;
		usecs -= ( usecs / 1000000 ) * 1000000;
	}
	
	return ( secs << 32 ) + usecs;
}

uint64_t ReceiverBase::getClock( uint32_t *year, uint32_t *month, uint32_t *day, uint32_t *hours, uint32_t *minutes, uint32_t *seconds )
{
	uint64_t ntp_time = getClock();
	
	// Convert to unix timestamp.
	std::time_t sec_since_epoch = ( ntp_time - ( uint64_t( 0x83AA7E80 ) << 32 ) ) >> 32;
	
	auto tm = std::localtime( &sec_since_epoch );
	if( year ) *year = tm->tm_year + 1900;
	if( month ) *month = tm->tm_mon + 1;
	if( day ) *day = tm->tm_mday;
	if( hours ) *hours = tm->tm_hour;
	if( minutes ) *minutes = tm->tm_min;
	if( seconds )*seconds = tm->tm_sec;
	
	return ntp_time;
}

std::string ReceiverBase::getClockString( bool includeDate )
{
	uint32_t year, month, day, hours, minutes, seconds;
	getClock( &year, &month, &day, &hours, &minutes, &seconds );
	
	char buffer[128];
	
	if( includeDate )
		sprintf( buffer, "%d/%d/%d %02d:%02d:%02d", month, day, year, hours, minutes, seconds );
	else
		sprintf( buffer, "%02d:%02d:%02d", hours, minutes, seconds );
	
	return std::string( buffer );
}

void ReceiverBase::setClock( uint64_t ntp_time )
{
	uint64_t current_ntp_time = time::get_current_ntp_time();
	
	sTimeOffsetSecs = ( ntp_time >> 32 ) - ( current_ntp_time >> 32 );
	sTimeOffsetUsecs = ( ntp_time & uint32_t( ~0 ) ) - ( current_ntp_time & uint32_t( ~0 ) );
}

void ReceiverBase::dispatchMethods( uint8_t *data, uint32_t size )
{
	using namespace decode;
	std::vector<Message> messages;
	
	if( ! decodeData( data, size, messages ) ) return;
	CI_ASSERT( messages.size() > 0 );
	
	std::lock_guard<std::mutex> lock( mListenerMutex );
	// iterate through all the messages and find matches with registered methods
	//! TODO: figure out what to do here. for messages that don't match and also messages that aren't yet ready to
	//! be sent.
	for( auto & message : messages ) {
		bool dispatchedOnce = false;
		for( auto & listener : mListeners ) {
			if( patternMatch( message.getAddress(), listener.first ) ) {
				listener.second( message );
				dispatchedOnce = true;
			}
		}
		if( ! dispatchedOnce ) {
			
		}
	}
}
	
/////////////////////////////////////////////////////////////////////////////////////////
//// ReceiverUdp
	
ReceiverUdp::ReceiverUdp( uint16_t port, const asio::ip::udp &protocol, asio::io_service &io )
: mSocket( new udp::socket( io, udp::endpoint( protocol, port ) ) ), mAmountToReceive( 4096 )
{
	asio::socket_base::reuse_address reuse( true );
	mSocket->set_option( reuse );
}

ReceiverUdp::ReceiverUdp( const asio::ip::udp::endpoint &localEndpoint, asio::io_service &io )
: mSocket( new udp::socket( io, localEndpoint ) ), mAmountToReceive( 4096 )
{
	asio::socket_base::reuse_address reuse( true );
	mSocket->set_option( reuse );
}

ReceiverUdp::ReceiverUdp( UdpSocketRef socket )
: mSocket( socket ), mAmountToReceive( 4096 )
{
}

void ReceiverUdp::listenImpl()
{
	auto tempBuffer = mBuffer.prepare( mAmountToReceive );
	
	mSocket->async_receive( tempBuffer,
	[&]( const error_code &error, size_t bytesTransferred ) {
		//! TODO: insert error handling.
		mBuffer.commit( bytesTransferred );
		auto data = std::unique_ptr<uint8_t[]>( new uint8_t[ bytesTransferred + 1 ] );
		data[ bytesTransferred ] = 0;
		istream stream( &mBuffer );
		stream.read( reinterpret_cast<char*>( data.get() ), bytesTransferred );
		// TODO: figure out whether we need this + 4, maybe there's someway to remove the size earlier from the streambuf
		// right now it represents skipping the size.
		dispatchMethods( data.get(), bytesTransferred );
		if( ! error )
			listen();
		else
			close();
	});
}
	
/////////////////////////////////////////////////////////////////////////////////////////
//// ReceiverTcp

ReceiverTcp::Connection::Connection( TcpSocketRef socket, ReceiverTcp *transport )
: mSocket( socket ), mTransport( transport ), mDataBuffer( 4096 )
{
}

ReceiverTcp::Connection::Connection( Connection && other ) noexcept
: mSocket( move( other.mSocket ) ), mTransport( other.mTransport ), mDataBuffer( move( other.mDataBuffer ) )
{
}

ReceiverTcp::Connection& ReceiverTcp::Connection::operator=( Connection && other ) noexcept
{
	if( this != &other ) {
		mSocket = move( other.mSocket );
		mTransport = other.mTransport;
		mDataBuffer = move( other.mDataBuffer );
	}
	return *this;
}

ReceiverTcp::Connection::~Connection()
{
	close();
	mTransport = nullptr;
}
	
using iterator = asio::buffers_iterator<asio::streambuf::const_buffers_type>;
	
std::pair<iterator, bool> ReceiverTcp::Connection::readMatchCondition( iterator begin, iterator end )
{
	iterator i = begin;
	ByteArray<4> data;
	int inc = 0;
	while ( i != end && inc < 4 )
		data[inc++] = *i++;
	
	int numBytes = *reinterpret_cast<int*>( data.data() );
	// swap for big endian from the other side
	numBytes = ntohl( numBytes );
	
	if( inc == 4 && numBytes > 0 && numBytes + 4 <= std::distance( begin, end ) ) {
		return { begin + numBytes + 4, true };
	}
	else {
		return { begin, false };
	}
}

void ReceiverTcp::Connection::read()
{
	asio::async_read_until( *mSocket, mBuffer, &readMatchCondition,
	[&]( const asio::error_code &error, size_t bytesTransferred ) {
		auto data = std::unique_ptr<uint8_t[]>( new uint8_t[ bytesTransferred + 1 ] );
		data[ bytesTransferred ] = 0;
		istream stream( &mBuffer );
		stream.read( reinterpret_cast<char*>( data.get() ), bytesTransferred );
		{
			std::lock_guard<std::mutex> lock( mTransport->mDataHandlerMutex );
			mTransport->dispatchMethods( (data.get() + 4), bytesTransferred );
		}
		// TODO: Figure these errors out.
		read();
	});
}

ReceiverTcp::ReceiverTcp( uint16_t port, const protocol &protocol, asio::io_service &service )
: mIoService( service ), mAcceptor( mIoService, asio::ip::tcp::endpoint( protocol, port ) )
{
	
}

ReceiverTcp::ReceiverTcp( const protocol::endpoint &localEndpoint, asio::io_service &service )
: mIoService( service ), mAcceptor( mIoService, localEndpoint )
{
	
}

void ReceiverTcp::listenImpl()
{
	auto socket = TcpSocketRef( new tcp::socket( mIoService ) );
	
	if( ! mAcceptor.is_open() );
	//		mAcceptor.open();
	
	mAcceptor.async_accept( *socket, std::bind(
	[&]( TcpSocketRef socket, const asio::error_code &error ) {
		   CI_LOG_V("Accepted a connection");
		   if( ! error ) {
			   std::lock_guard<std::mutex> lock( mConnectionMutex );
			   mConnections.emplace_back( socket, this );
			   mConnections.back().read();
		   }
		   else {
			   // TODO: Figure out error interface.
			   CI_LOG_E(error.message());
		   }
		   listen();
	}, socket, _1 ) );
}

void ReceiverTcp::closeImpl()
{
	mAcceptor.close();
	std::lock_guard<std::mutex> lock( mConnectionMutex );
	for( auto & connection : mConnections ) {
		connection.close();
	}
	mConnections.clear();
}
	
const char* argTypeToString( ArgType type )
{
	switch ( type ) {
		case ArgType::INTEGER_32: return "INTEGER_32"; break;
		case ArgType::FLOAT: return "FLOAT"; break;
		case ArgType::DOUBLE: return "DOUBLE"; break;
		case ArgType::STRING: return "STRING"; break;
		case ArgType::BLOB: return "BLOB"; break;
		case ArgType::MIDI: return "MIDI"; break;
		case ArgType::TIME_TAG: return "TIME_TAG"; break;
		case ArgType::INTEGER_64: return "INTEGER_64"; break;
		case ArgType::BOOL_T: return "BOOL_T"; break;
		case ArgType::BOOL_F: return "BOOL_F"; break;
		case ArgType::CHAR: return "CHAR"; break;
		case ArgType::NULL_T: return "NULL_T"; break;
		case ArgType::INFINITUM: return "INFINITUM"; break;
		case ArgType::NONE: return "NONE"; break;
		default: return "Unknown ArgType"; break;
	}
}

namespace time {

uint64_t get_current_ntp_time()
{
	auto now = std::chrono::system_clock::now();
	auto sec = std::chrono::duration_cast<std::chrono::seconds>( now.time_since_epoch() ).count() + 0x83AA7E80;
	auto usec = std::chrono::duration_cast<std::chrono::microseconds>( now.time_since_epoch() ).count() + 0x7D91048BCA000;
	
	return ( sec << 32 ) + ( usec % 1000000L );
}

} // namespace time

} // namespace osc