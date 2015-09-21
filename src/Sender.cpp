//
//  Sender.cpp
//  Test
//
//  Created by ryan bartley on 9/4/15.
//
//

#include "Sender.h"

#include "cinder/Log.h"
#include "TransportUDP.h"

using namespace std;
using namespace ci;
using namespace ci::app;
using namespace asio;
using namespace asio::ip;
using namespace std::placeholders;

namespace osc {
	
shared_ptr<vector<uint8_t>> createSharedByteBuffer( const Message &message )
{
	return shared_ptr<vector<uint8_t>>( new vector<uint8_t>( std::move( message.byteArray() ) ) );
}

Sender::Sender( uint16_t localPort, const std::string &host, uint16_t port, const udp &protocol, io_service &io )
: mTransportSender( new TransportSenderUDP( std::bind( &Sender::writeHandler, this, _1, _2, _3 ),
											ip::udp::endpoint( protocol, localPort ),
											ip::udp::endpoint( address::from_string( host ), port ), io ) )
{
}
	
Sender::Sender( uint16_t localPort, const udp::endpoint &destination, const udp &protocol, io_service &io )
: mTransportSender( new TransportSenderUDP( std::bind( &Sender::writeHandler, this, _1, _2, _3 ),
										   ip::udp::endpoint( protocol, localPort ),
										   destination, io ) )
{
}
	
Sender::Sender( const UDPSocketRef &socket, const udp::endpoint &destination )
: mTransportSender( new TransportSenderUDP( std::bind( &Sender::writeHandler, this, _1, _2, _3 ),
										   socket, destination ) )
{
}
	
void Sender::send( const osc::Message &message )
{
	auto cache = createSharedByteBuffer( message );
	mTransportSender->send( cache );
}
	
void Sender::send( const osc::Bundle &bundle )
{
	
}
	
void Sender::writeHandler( const error_code &error, size_t bytesTransferred, shared_ptr<vector<uint8_t>> byte_buffer )
{
	if( error ) {
		// get socket address
		auto socketAddress = mTransportSender->getRemoteAddress();
		// derive oscAddress
		std::string oscAddress;
		if( ! byte_buffer->empty() )
			oscAddress = ( (const char*)byte_buffer.get() );
	
		if( mErrorHandler ) {
			mErrorHandler( error.message(), socketAddress.to_string(), oscAddress );
		}
		else
			CI_LOG_E( error.message() << ", didn't send message [" << oscAddress << "] to " << socketAddress.to_string() );
	}
}

}