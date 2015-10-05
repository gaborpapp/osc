//
//  ClientUDP.cpp
//  Test
//
//  Created by ryan bartley on 9/11/15.
//
//

#include "TransportUDP.h"

using namespace std;
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
: TransportReceiverBase( 4096 ), mSocket( new udp::socket( service, localEndpoint ) )
{
	asio::socket_base::reuse_address reuse( true );
	mSocket->set_option( reuse );
	mDataHandler = dataHandler;
}
	
TransportReceiverUDP::TransportReceiverUDP( DataHandler dataHandler, const UDPSocketRef &socket )
: TransportReceiverBase( 4096 ), mSocket( socket )
{
	mDataHandler = dataHandler;
}
	
void TransportReceiverUDP::listen()
{
	auto tempBuffer = mBuffer.prepare( mAmountToReceive );
	
	mSocket->async_receive( tempBuffer,
	[&]( const error_code &error, size_t bytesTransferred ){
		std::string str( asio::buffers_begin( mBuffer.data() ), asio::buffers_end( mBuffer.data() ) );
		mBuffer.commit( bytesTransferred );
		mDataHandler( error, bytesTransferred, mBuffer );
		if( ! error )
			listen();
		else
			close();
	});
}
	
void TransportReceiverUDP::close()
{
	mSocket->close();
}

	
}