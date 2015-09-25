//
//  Server.cpp
//  Test
//
//  Created by ryan bartley on 9/3/15.
//
//

#include "Receiver.h"
#include "TransportUDP.h"

using namespace std;
using namespace ci;
using namespace ci::app;
using namespace asio;
using namespace asio::ip;
using namespace std::placeholders;

namespace osc {
	
Receiver::Receiver( uint16_t port, const asio::ip::udp &protocol, asio::io_service &io )
: mTransportReceiver( new TransportReceiverUDP( std::bind( &Receiver::receiveHandler, this, _1, _2, _3 ),
												   asio::ip::udp::endpoint( protocol, port ),
												   io ) )
{
}
	
Receiver::Receiver( const asio::ip::udp::endpoint &localEndpoint, asio::io_service &io )
: mTransportReceiver( new TransportReceiverUDP( std::bind( &Receiver::receiveHandler, this, _1, _2, _3 ),
												localEndpoint,
												io ) )
{
	
}
	
Receiver::Receiver( UDPSocketRef socket )
	: mTransportReceiver( new TransportReceiverUDP( std::bind( &Receiver::receiveHandler, this, _1, _2, _3 ),
												   socket ) )
{
}
	
void Receiver::setListener( const std::string &address, Listener listener )
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
	
void Receiver::removeListener( const std::string &address )
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
	
void Receiver::listen()
{
	mTransportReceiver->listen();
}
	
void Receiver::receiveHandler( const asio::error_code &error, std::size_t bytesTransferred, asio::streambuf &streamBuf )
{
	if( error ) {
		if( mErrorHandler ) {
			mErrorHandler( error.message() );
		}
	}
	else {
		char* data = new char[ bytesTransferred + 1 ]();
		data[ bytesTransferred ] = 0;
		streamBuf.commit( bytesTransferred );
		istream stream( &streamBuf );
		stream.read( data, bytesTransferred );
		size_t remainingBytes;
		if( checkValidity( data, bytesTransferred, &remainingBytes ) ) {
			dispatchMethods( (uint8_t*)data, bytesTransferred );
			streamBuf.consume( bytesTransferred );
		}
	}
}
	
void Receiver::dispatchMethods( uint8_t *data, uint32_t size )
{
	std::vector<Message> messages;
	
	if( ! decodeData( data, size, messages ) ) return;
	CI_ASSERT( messages.size() > 0 );
	
	std::lock_guard<std::mutex> lock( mListenerMutex );
	// iterate through all the messages and find matches with registered methods
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
//	ParsedMessage m{ timetag, Message( std::string("") ) };
	Message message;
	if( ! message.bufferCache( (uint8_t*)data, size ) )
		return false;
	
	messages.push_back( std::move( message ) );
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