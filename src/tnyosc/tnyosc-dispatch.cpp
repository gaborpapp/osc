#include "tnyosc-dispatch.h"
#include "tnyosc.h"

#include <algorithm>
#include <iostream>

#include <assert.h>
#include <stdio.h>

#include "cinder/Log.h"

#if defined(_WIN32)
#else
#include <arpa/inet.h>
#endif

namespace tnyosc {

void print_bytes( const char* bytes, size_t size )
{
	size_t i;
	for( i = 0; i < size; ++i ) {
		printf( "%02x ", bytes[i] );
		if( i % 16 == 15 ) {
			printf( "\n" );
		}
		else if( i % 4 == 3 ) {
			printf( " " );
		}
	}
	if( i % 16 != 0 ) {
		printf( "\n" );
	}
	printf( "\n" );
}

bool compare_callback_timetag( const CallbackRef first, const CallbackRef second )
{
	return first->timetag < second->timetag;
}

void Dispatcher::addMethod( const char* address, ReceivedMessageCallback method, void* user_data )
{
	MethodTemplate m;
	m.address = address == NULL ? "" : address;
	m.user_data = user_data;
	m.method = method;
	mMethods.push_back( m );
}

std::list<CallbackRef> Dispatcher::matchMethods( const char* data, size_t size )
{
	std::list<ParsedMessage> parsedMessages;
	std::list<CallbackRef> callback_list;
	
	if( !decode_data( data, size, parsedMessages ) ) return callback_list;
	CI_ASSERT( parsedMessages.size() > 0 );

	// iterate through all the messages and find matches with registered methods
	for( auto parsedMessage : parsedMessages ) {
		for( auto method : mMethods ) {
			if( pattern_match( parsedMessage.message.getAddress(), method.address ) ) {
				// if a method specifies a type, make sure it matches
				if( method.types.empty() || parsedMessage.message.compareTypes( method.types ) ) {
					CallbackRef callback = CallbackRef( new Callback{
						parsedMessage.timetag,
						parsedMessage.message,
						method.user_data,
						method.method
					});
					callback_list.push_back( callback );
				}
			}
		}
	}

	callback_list.sort( compare_callback_timetag );

	return callback_list;
}

bool Dispatcher::decode_data( const char* data, size_t size,
							 std::list<ParsedMessage>& messages, uint64_t timetag )
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
			if( !decode_data( data, seg_size, messages, ntohll( timestamp ) ) ) return false;
			data += seg_size; size -= seg_size;
		}
	}
	else {
		if( ! decode_osc( data, size, messages, timetag ) ) return false;
	}
	
	return true;
}

bool Dispatcher::decode_osc( const char* data, size_t size,
							 std::list<ParsedMessage>& messages, uint64_t timetag )
{
	ParsedMessage m{ timetag, Message( std::string("") ) };
	
	if( ! m.message.bufferCache( (uint8_t*)data, size ) )
		return false;
	
	messages.push_back( m );
	return true;
}

// pattern_match compares two strings and returns true or false depending on if
// they match according to OSC's pattern matching guideline
//
// OSC Pattern Matching Guideline:
// 
//   1. '?' in the OSC Address Pattern matches any single character.
//   2. '*' in the OSC Address Pattern matches any sequence of zero or more 
//      characters.
//   3. A string of characters in square brackets (e.g., "[string]") in the 
//      OSC Address Pattern matches any character in the string. Inside square
//      brackets, the minus sign (-) and exclamation point (!) have special 
//      meanings:
//        o two characters separated by a minus sign indicate the range of 
//          characters between the given two in ASCII collating sequence. (A 
//          minus sign at the end of the string has no special meaning.)
//        o An exclamation point at the beginning of a bracketed string 
//          negates the sense of the list, meaning that the list matches any 
//          character not in the list. (An exclamation point anywhere besides 
//          the first character after the open bracket has no special meaning.)
//   4. A comma-separated list of strings enclosed in curly braces 
//      (e.g., "{foo,bar}") in the OSC Address Pattern matches any of the 
//      strings in the list.
//   5. Any other character in an OSC Address Pattern can match only the same 
//      character.
//
// @param lhs incoming OSC address pattern to match it with rhs's pattern
// @param rhs method address pattern that may contain special characters
bool Dispatcher::pattern_match( const std::string& lhs, const std::string& rhs )
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

} // namespace tnyosc