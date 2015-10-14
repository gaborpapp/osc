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

#include "Message.h"

namespace osc {
	
Message::Message( const std::string& address )
: mAddress( address ), mIsCached( false )
{
}
	
Message::Argument::Argument()
: mType( ArgType::NULL_T ), mSize( 0 ), mOffset( -1 )
{
	
}

Message::Argument::Argument( ArgType type, int32_t offset, uint32_t size, bool needsSwap )
: mType( type ), mOffset( offset ), mSize( size ), mNeedsEndianSwapForTransmit( needsSwap )
{
	
}
	
ArgType Message::Argument::translateCharToArgType( char type )
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

char Message::Argument::translateArgTypeToChar( ArgType type )
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
	
void Message::Argument::swapEndianForTransmit( uint8_t *buffer ) const
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
	
void Message::Argument::outputValueToStream( uint8_t *buffer, std::ostream &ostream ) const
{
	
	auto ptr = &buffer[mOffset];
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
	mDataViews.emplace_back( ArgType::INTEGER_32, getCurrentOffset(), 4, true );
	ByteArray<4> b;
	memcpy( b.data(), &v, 4 );
	mDataBuffer.insert( mDataBuffer.end(), b.begin(), b.end() );
}

void Message::append( float v )
{
	mIsCached = false;
	mDataViews.emplace_back( ArgType::FLOAT, getCurrentOffset(), 4, true );
	ByteArray<4> b;
	memcpy( b.data(), &v, 4 );
	mDataBuffer.insert( mDataBuffer.end(), b.begin(), b.end() );
}

void Message::append( const std::string& v )
{
	mIsCached = false;
	auto size = v.size() + getTrailingZeros( v.size() );
	mDataViews.emplace_back( ArgType::STRING, getCurrentOffset(), size );
	mDataBuffer.insert( mDataBuffer.end(), v.begin(), v.end() );
	mDataBuffer.resize( mDataBuffer.size() + getTrailingZeros( v.size() ), 0 );
}

void Message::append( const char* v, size_t len )
{
	if( ! v || len == 0 ) return;
	mIsCached = false;
	auto size = len + getTrailingZeros( len );
	mDataViews.emplace_back( ArgType::STRING, getCurrentOffset(), size );
	ByteBuffer b( v, v + len );
	b.resize( size, 0 );
	mDataBuffer.insert( mDataBuffer.end(), b.begin(), b.end() );
}

void Message::appendBlob( void* blob, uint32_t size )
{
	mIsCached = false;
	auto totalBufferSize = 4 + size + getTrailingZeros( size );
	mDataViews.emplace_back( ArgType::BLOB, getCurrentOffset(), totalBufferSize, true );
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
	mDataViews.emplace_back( ArgType::TIME_TAG, getCurrentOffset(), 8, true );
	ByteArray<8> b;
	memcpy( b.data(), &v, 8 );
	mDataBuffer.insert( mDataBuffer.end(), b.begin(), b.end() );
}

void Message::append( bool v )
{
	mIsCached = false;
	if( v )
		mDataViews.emplace_back( ArgType::BOOL_T, -1, 0 );
	else
		mDataViews.emplace_back( ArgType::BOOL_F, -1, 0 );
}

void Message::append( int64_t v )
{
	mIsCached = false;
	mDataViews.emplace_back( ArgType::INTEGER_64, getCurrentOffset(), 8, true );
	ByteArray<8> b;
	memcpy( b.data(), &v, 8 );
	mDataBuffer.insert( mDataBuffer.end(), b.begin(), b.end() );
}

void Message::append( double v )
{
	mIsCached = false;
	mDataViews.emplace_back( ArgType::DOUBLE, getCurrentOffset(), 8, true );
	ByteArray<8> b;
	memcpy( b.data(), &v, 8 );
	mDataBuffer.insert( mDataBuffer.end(), b.begin(), b.end() );
}

void Message::append( char v )
{
	mIsCached = false;
	mDataViews.emplace_back( ArgType::CHAR, getCurrentOffset(), 4, true );
	ByteArray<4> b;
	b.fill( 0 );
	b[0] = v;
	mDataBuffer.insert( mDataBuffer.end(), b.begin(), b.end() );
}

void Message::appendMidi( uint8_t port, uint8_t status, uint8_t data1, uint8_t data2 )
{
	mIsCached = false;
	mDataViews.emplace_back( ArgType::MIDI, getCurrentOffset(), 4 );
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
const Message::Argument& Message::getDataView( uint32_t index ) const
{
	if( index >= mDataViews.size() )
		throw ExcIndexOutOfBounds( mAddress, index );
	
	auto &dataView = mDataViews[index];
	if( ! dataView.convertible<T>() ) // TODO: change the type to typeid print out.
		throw ExcNonConvertible( mAddress, index, ArgType::INTEGER_32, dataView.getArgType() );
	
	return dataView;
}

template<typename T>
bool Message::Argument::convertible() const
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
	auto &dataView = mDataViews[index];
	return dataView.getArgType();
}

int32_t Message::getInt( uint32_t index ) const
{
	auto &dataView = getDataView<int32_t>( index );
	return *reinterpret_cast<const int32_t*>(&mDataBuffer[dataView.getOffset()]);
}

float Message::getFloat( uint32_t index ) const
{
	auto &dataView = getDataView<float>( index );
	return *reinterpret_cast<const float*>(&mDataBuffer[dataView.getOffset()]);
}

std::string Message::getString( uint32_t index ) const
{
	auto &dataView = getDataView<std::string>( index );
	const char* head = reinterpret_cast<const char*>(&mDataBuffer[dataView.getOffset()]);
	return std::string( head );
}

int64_t Message::getTime( uint32_t index ) const
{
	auto &dataView = getDataView<int64_t>( index );
	return *reinterpret_cast<int64_t*>(mDataBuffer[dataView.getOffset()]);
}

int64_t Message::getInt64( uint32_t index ) const
{
	auto &dataView = getDataView<int64_t>( index );
	return *reinterpret_cast<int64_t*>(mDataBuffer[dataView.getOffset()]);
}

double Message::getDouble( uint32_t index ) const
{
	auto &dataView = getDataView<double>( index );
	return *reinterpret_cast<double*>(mDataBuffer[dataView.getOffset()]);
}

bool Message::getBool( uint32_t index ) const
{
	auto &dataView = getDataView<bool>( index );
	return dataView.getArgType() == ArgType::BOOL_T;
}

char Message::getChar( uint32_t index ) const
{
	auto &dataView = getDataView<int32_t>( index );
	return mDataBuffer[dataView.getOffset()];
}

void Message::getMidi( uint32_t index, uint8_t *port, uint8_t *status, uint8_t *data1, uint8_t *data2 ) const
{
	auto &dataView = getDataView<int32_t>( index );
	int32_t midiVal = *reinterpret_cast<const int32_t*>(&mDataBuffer[dataView.getOffset()]);
	*port = midiVal;
	*status = midiVal >> 8;
	*data1 = midiVal >> 16;
	*data2 = midiVal >> 24;
}

ci::Buffer Message::getBlob( uint32_t index ) const
{
	auto &dataView = getDataView<ci::Buffer>( index );
	ci::Buffer ret( dataView.getArgSize() );
	const uint8_t* data = reinterpret_cast<const uint8_t*>( &mDataBuffer[dataView.getOffset()+4] );
	memcpy( ret.getData(), data, dataView.getArgSize() );
	return ret;
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
	
const ByteBuffer& Message::byteArray() const
{
	if( ! mIsCached )
		createCache();
	return *mCache;
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
}
	
std::ostream& operator<<( std::ostream &os, const Message &rhs )
{
	os << "Address: " << rhs.getAddress() << std::endl;
	for( auto &dataView : rhs.mDataViews ) {
		os << "\t<" << argTypeToString( dataView.getArgType() ) << ">: ";
		dataView.outputValueToStream( const_cast<uint8_t*>(rhs.mDataBuffer.data()), os );
		os << std::endl;
	}
	return os;
}
	
}