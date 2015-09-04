//
//  Server.cpp
//  Test
//
//  Created by ryan bartley on 9/3/15.
//
//

#include "Receiver.h"

namespace osc {
	
Receiver::Receiver( uint16_t port, asio::io_service &io )
: mPort( port ), mSocket( io, asio::ip::udp::endpoint( asio::ip::udp::v4(), mPort ) ),
	 mBuffer( 4096 )
{
	asio::socket_base::reuse_address reuse( true );
	mSocket.set_option( reuse );
}
	
Receiver::Receiver( Receiver &&other )
: mPort( other.mPort ), mSocket( std::move( other.mSocket ) )
{
	other.mPort = 0;
}
	
void Receiver::setListener( const std::string &address, Listener listener, void* userData )
{
	auto foundListener = std::find_if( mListeners.begin(), mListeners.end(),
							  [address]( const std::pair<std::string, ListenerInfo> &listener ) {
								  return address == listener.first;
							  });
	if( foundListener != mListeners.end() ) {
		foundListener->second = ListenerInfo{ listener, userData };
	}
	else {
		mListeners.push_back( { address, ListenerInfo{ listener, userData } } );
	}
}
	
void Receiver::removeListener( const std::string &address )
{
	auto foundListener = std::find_if( mListeners.begin(), mListeners.end(),
									  [address]( const std::pair<std::string, ListenerInfo> &listener ) {
										  
									  });
}
	
void Receiver::open()
{
	listen();
}
	
void Receiver::close()
{
	mSocket.close();
}
	
void Receiver::setPort( uint16_t port )
{
	mSocket.close();
	mPort = port;
	mSocket.bind( asio::ip::udp::endpoint( asio::ip::udp::v4(), mPort ) );
}
	
void Receiver::setBufferSize( size_t bufferSize )
{
	mBuffer.resize( bufferSize );
}
	
void Receiver::listen()
{
	asio::async_read( mSocket, asio::buffer( mBuffer ),
					 std::bind( &Receiver::receiveHandler,
							   this,
							   std::placeholders::_1,
							   std::placeholders::_2 ) );
}
	
void Receiver::receiveHandler( const asio::error_code &error, std::size_t bytesTransferred )
{
	if( error ) {
		if( mErrorHandler ) {
			mErrorHandler( error.message() );
		}
	}
	else {
		dispatchMethods( mBuffer.data(), bytesTransferred );
		listen();
	}
}
	
void Receiver::dispatchMethods( uint8_t *data, uint32_t size )
{
	std::vector<Message> messages;
	
	if( ! decodeData( data, size, messages ) ) return;
	CI_ASSERT( messages.size() > 0 );
	
	// iterate through all the messages and find matches with registered methods
	for( auto & message : messages ) {
		for( auto & listener : mListeners ) {
			if( patternMatch( message.getAddress(), listener.first ) ) {
				
			}
		}
	}
}
	
bool Receiver::decodeData( uint8_t *data, uint32_t size, std::vector<Message> &messages, uint64_t timetag )
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
			if( !decodeData( data, seg_size, messages, ntohll( timestamp ) ) ) return false;
			data += seg_size; size -= seg_size;
		}
	}
	else {
		if( ! decodeMessage( data, size, messages, timetag ) ) return false;
	}
	
	return true;
}
	
bool Receiver::decodeMessage( uint8_t *data, uint32_t size, std::vector<Message> &messages, uint64_t timetag )
{
	ParsedMessage m{ timetag, Message( std::string("") ) };
	
	if( ! m.message.bufferCache( (uint8_t*)data, size ) )
		return false;
	
	messages.push_back( m );
	return true;
}
	
	
bool Receiver::patternMatch( const std::string& lhs, const std::string& rhs )
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
	
}