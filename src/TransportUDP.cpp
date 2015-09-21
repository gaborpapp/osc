//
//  ClientUDP.cpp
//  Test
//
//  Created by ryan bartley on 9/11/15.
//
//

#include "TransportUDP.h"

using namespace asio;
using namespace asio::ip;

namespace osc {
	
TransportSenderUDP::TransportSenderUDP( WriteHandler writeHandler, const udp::endpoint &localEndpoint, const udp::endpoint &remoteEndpoint, asio::io_service &service )
: mSocket( new udp::socket( service, localEndpoint ) ), mRemoteEndpoint( remoteEndpoint )
{
	socket_base::reuse_address reuse( true );
	mSocket->set_option( reuse );
	mWriteHandler = writeHandler;
}
	
TransportSenderUDP::TransportSenderUDP( WriteHandler writeHandler, const UDPSocketRef &socket, const asio::ip::udp::endpoint &remoteEndpoint )
: mSocket( socket ), mRemoteEndpoint( remoteEndpoint )
{
	mWriteHandler = writeHandler;
}
	
void TransportSenderUDP::send( std::shared_ptr<std::vector<uint8_t>> data )
{
	mSocket->async_send_to( asio::buffer( *data ), mRemoteEndpoint,
						   [&, data]( const asio::error_code& error, size_t bytesTransferred ) {
							   mWriteHandler( error, bytesTransferred, data );
						   });
}
	
TransportReceiverUDP::TransportReceiverUDP( DataHandler dataHandler, const asio::ip::udp::endpoint &localEndpoint, asio::io_service &service )
: mSocket( new udp::socket( service, localEndpoint ) ), mAmountToReceive( 4096 )
{
	asio::socket_base::reuse_address reuse( true );
	mSocket->set_option( reuse );
	mDataHandler = dataHandler;
}
	
TransportReceiverUDP::TransportReceiverUDP( DataHandler dataHandler, const UDPSocketRef &socket )
: mSocket( socket ), mAmountToReceive( 4096 )
{
	mDataHandler = dataHandler;
}
	
void TransportReceiverUDP::listen()
{
	mSocket->async_receive( mBuffer.prepare( mAmountToReceive ),
						   [&]( const error_code &error, size_t bytesTransferred ){
							   mDataHandler( error, bytesTransferred, mBuffer );
						   });
}
	
void TransportReceiverUDP::close()
{
	mSocket->close();
}

	
}