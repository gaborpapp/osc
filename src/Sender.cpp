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

namespace osc {
	
shared_ptr<vector<uint8_t>> createSharedByteBuffer( const Message &message )
{
	return shared_ptr<vector<uint8_t>>( new vector<uint8_t>( std::move( message.byteArray() ) ) );
}

Sender::Sender( asio::io_service &io )
: mSocket( io,  asio::ip::udp::endpoint( asio::ip::udp::v4(), 8081 ) )
{
}
	
void Sender::send( const osc::Message &message, const asio::ip::udp::endpoint &recipient )
{
	auto cache = createSharedByteBuffer( message );
	writeAsync( cache, message.getAddress(), recipient );
}
	
void Sender::send( const osc::Message &message, const std::string &host, uint16_t port )
{
	auto recipient = asio::ip::udp::endpoint( asio::ip::address::from_string( host ), port );
	auto cache = createSharedByteBuffer( message );
	writeAsync( cache, message.getAddress(), recipient );
}
	
void Sender::send( const osc::Message &message, const std::vector<asio::ip::udp::endpoint> &recipients )
{
	auto cache = createSharedByteBuffer( message );
	auto address = message.getAddress();
	for( auto & recipient : recipients ) {
		writeAsync( cache, address, recipient );
	}
}
	
void Sender::send( const osc::Bundle &bundle, const std::string &host, uint16_t port )
{
	
}

void Sender::send( const osc::Bundle &bundle, const asio::ip::udp::endpoint &recipient )
{
	
}

void Sender::send( const osc::Bundle &bundle, const std::vector<asio::ip::udp::endpoint> &recipients )
{
	
}
	
void Sender::writeAsync( std::shared_ptr<std::vector<uint8_t> > cache, const std::string &address, const asio::ip::udp::endpoint &recipient )
{
	mSocket.async_send_to( asio::buffer( *cache ), recipient,
						  std::bind( &Sender::writeHandler, this,
									std::placeholders::_1, std::placeholders::_2,
									cache, address, recipient ) );
}
	
void Sender::writeHandler( const asio::error_code &error, size_t bytesTransferred, std::shared_ptr<std::vector<uint8_t>> &byte_buffer, std::string address, asio::ip::udp::endpoint recipient )
{
	if( error ) {
		if( mErrorHandler )
			mErrorHandler( address, recipient, error.message() );
		else
			CI_LOG_E( error.message() << ", didn't send message [" << address << "] to " << recipient.address() << ":" << recipient.port() );
	}
	else {
		byte_buffer.reset();
	}
}

}