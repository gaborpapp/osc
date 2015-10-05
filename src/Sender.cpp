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
#include "TransportTCP.h"

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

SenderUDP::SenderUDP( uint16_t localPort, const std::string &host, uint16_t port, const udp &protocol, io_service &io )
: SenderBase( std::unique_ptr<TransportSenderBase>( new TransportSenderUDP(
	std::bind( &SenderUDP::writeHandler, this, _1, _2, _3 ),
	ip::udp::endpoint( protocol, localPort ),
	ip::udp::endpoint( address::from_string( host ), port ),
	io ) ) )
{
}
	
SenderUDP::SenderUDP( uint16_t localPort, const udp::endpoint &destination, const udp &protocol, io_service &io )
: SenderBase( std::unique_ptr<TransportSenderBase>( new TransportSenderUDP(
	std::bind( &SenderUDP::writeHandler, this, _1, _2, _3 ),
	ip::udp::endpoint( protocol, localPort ),
	destination,
	io ) ) )
{
}
	
SenderUDP::SenderUDP( const UDPSocketRef &socket, const udp::endpoint &destination )
: SenderBase( std::unique_ptr<TransportSenderBase>( new TransportSenderUDP(
	std::bind( &SenderUDP::writeHandler, this, _1, _2, _3 ),
	socket,
	destination ) ) )
{
}
	
SenderTCP::SenderTCP( uint16_t localPort, const string &destinationHost, uint16_t destinationPort, const tcp &protocol , io_service &io )
: SenderBase( std::unique_ptr<TransportSenderBase>( new TransportSenderTCP(
	std::bind( &SenderTCP::writeHandler, this, _1, _2, _3 ),
	ip::tcp::endpoint( protocol, localPort ),
	ip::tcp::endpoint( address::from_string( destinationHost ), destinationPort ),
	io ) )  )
{
}
	
SenderTCP::SenderTCP( uint16_t localPort,  const tcp::endpoint &destination, const tcp &protocol, io_service &io )
: SenderBase( std::unique_ptr<TransportSenderBase>( new TransportSenderTCP(
	std::bind( &SenderTCP::writeHandler, this, _1, _2, _3 ),
	ip::tcp::endpoint( protocol, localPort ),
	destination,
	io ) )  )
{
	
}

SenderTCP::SenderTCP( const TCPSocketRef &socket, const tcp::endpoint &destination )
: SenderBase( std::unique_ptr<TransportSenderBase>( new TransportSenderTCP(
	std::bind( &SenderTCP::writeHandler, this, _1, _2, _3 ),
	socket,
	destination ) )  )
{
	
}
	
SenderBase::SenderBase( std::unique_ptr<TransportSenderBase> transport )
: mTransportSender( std::move( transport ) )
{
}
	
void SenderTCP::connect()
{
	auto transportTCP = dynamic_cast<TransportSenderTCP*>(mTransportSender.get());
	transportTCP->connect();
}
	
void SenderBase::send( const TransportData &message )
{
	mTransportSender->send( message.getSharedBuffer() );
}
	
void SenderBase::writeHandler( const error_code &error, size_t bytesTransferred, shared_ptr<vector<uint8_t>> byte_buffer )
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
	else {
//		cout << "SENDER: bytestransferred: " << bytesTransferred << " | bytesToTransfer: " << byte_buffer->size() << endl;
	}
}

}