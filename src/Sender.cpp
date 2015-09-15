//
//  Sender.cpp
//  Test
//
//  Created by ryan bartley on 9/4/15.
//
//

#include "Sender.h"

#include "cinder/Log.h"

using namespace std;
using namespace ci;
using namespace ci::app;
using namespace asio;
using namespace asio::ip;

namespace osc {
	
shared_ptr<vector<uint8_t>> createSharedByteBuffer( const Message &message )
{
	return shared_ptr<vector<uint8_t>>( new vector<uint8_t>( std::move( message.byteArray() ) ) );
}

Sender::Sender( uint16_t localPort, const std::string &host, uint16_t port, const udp &protocol, io_service &io )
: mSocket( new udp::socket( io, udp::endpoint( protocol, localPort ) ) ),
	mDestinationEndpoint( asio::ip::address::from_string( host ), port )
{
	socket_base::reuse_address reuse( true );
	mSocket->set_option( reuse );
}
	
Sender::Sender( uint16_t localPort, const udp::endpoint &destination, const udp &protocol, io_service &io )
: mSocket( new udp::socket( io, udp::endpoint( protocol, localPort ) ) ),
	mDestinationEndpoint( destination )
{
	socket_base::reuse_address reuse( true );
	mSocket->set_option( reuse );
}
	
Sender::Sender( const SocketRef &socket, const udp::endpoint &destination )
: mSocket( socket ), mDestinationEndpoint( destination )
{
}
	
void Sender::send( const osc::Message &message )
{
	auto cache = createSharedByteBuffer( message );
	writeAsync( cache, message.getAddress() );
}
	
void Sender::send( const osc::Bundle &bundle )
{
	
}
	
void Sender::writeAsync( std::shared_ptr<std::vector<uint8_t> > cache, const std::string &address )
{
	mSocket->async_send_to( asio::buffer( *cache ), mDestinationEndpoint,
						   std::bind( &Sender::writeHandler, this,
									 std::placeholders::_1, std::placeholders::_2,
									 cache, address ) );
}
	
void Sender::writeHandler( const error_code &error, size_t bytesTransferred, shared_ptr<vector<uint8_t>> &byte_buffer, string address )
{
	if( error ) {
		if( mErrorHandler )
			mErrorHandler( address, mDestinationEndpoint, error.message() );
		else
			CI_LOG_E( error.message() << ", didn't send message [" << address << "] to " <<
					 mDestinationEndpoint.address() << ":" << mDestinationEndpoint.port() );
	}
	else {
		byte_buffer.reset();
	}
}

}