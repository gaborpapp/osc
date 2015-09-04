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
	
using Argument = Message::Argument;
	
	Argument::Argument()
	: mType( ArgType::NULL_T ), mSize( 0 ), mOffset( -1 )
	{
		
	}
	
	Argument::Argument( ArgType type, int32_t offset, uint32_t size, bool needsSwap )
	: mType( type ), mOffset( offset ), mSize( size ), mNeedsEndianSwapForTransmit( needsSwap )
	{
		
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
		case ArgType::INFINITUM: return 'T'; break;
		case ArgType::NONE: return 'N'; break;
	}
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
	
void Argument::swapEndianForTransmit( uint8_t *buffer ) const
{
	auto ptr = &buffer[mOffset];
	switch ( mType ) {
		case ArgType::INTEGER_32: {
			int32_t a = htonl( *reinterpret_cast<int32_t*>(ptr) );
			memcpy( ptr, &a, 4 );
		}
		break;
		case ArgType::FLOAT: {
			int32_t a = htonf( *reinterpret_cast<float*>(ptr) );
			memcpy( ptr, &a, 4 );
		}
		break;
		case ArgType::BLOB: {
			int32_t a = htonl( *reinterpret_cast<int32_t*>(ptr) );
			memcpy( ptr, &a, 4 );
		}
		break;
		case ArgType::TIME_TAG: {
			uint64_t a = htonll( *reinterpret_cast<uint64_t*>(ptr) );
			memcpy( ptr, &a, 8 );
		}
		break;
		case ArgType::INTEGER_64: {
			int64_t a = htonll( *reinterpret_cast<int64_t*>(ptr) );
			memcpy( ptr, &a, 8 );
		}
		break;
		case ArgType::DOUBLE: {
			int64_t a = htond( *reinterpret_cast<double*>(ptr) );
			memcpy( ptr, &a, 8 );
		}
		break;
		case ArgType::CHAR: {
			int32_t a = htonl( *reinterpret_cast<int32_t*>(ptr) );
			memcpy( ptr, &a, 4 );
		}
		default:
		break;
	}
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
	
}