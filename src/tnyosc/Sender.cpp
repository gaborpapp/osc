//
//  Sender.cpp
//  Test
//
//  Created by ryan bartley on 9/4/15.
//
//

#include "Sender.h"

namespace osc {

Sender::Sender( asio::io_service &io )
: mSocket( io )
{
	
}
	
void Sender::send( const osc::Message &message, const asio::ip::udp::endpoint &recipient )
{
	auto cache = message.createCache();
	mSocket.async_send_to( asio::buffer( cache ), recipient,
						  std::bind( &Sender::writeHandler, this,
									message.getAddress(), recipient,
									std::placeholders::_1, std::placeholders::_2 ) );
}
	
void Sender::send( const osc::Message &message, const std::string &host, uint16_t port )
{
	auto cache = message.createCache();
	auto recipient = asio::ip::udp::endpoint( asio::ip::address::from_string( host ), port );
	mSocket.async_send_to( asio::buffer( cache ), recipient,
						  std::bind( &Sender::writeHandler, this,
									message.getAddress(), recipient,
									std::placeholders::_1, std::placeholders::_2 ) );
}
	
void Sender::writeHandler( std::string address, asio::ip::udp::endpoint recipient, const asio::error_code &error, size_t bytesTransferred )
{
	if( error ) {
		
	}
	else {
		
	}
}

}