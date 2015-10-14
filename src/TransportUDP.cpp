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
: TransportSenderBase( writeHandler ), mSocket( new udp::socket( service, localEndpoint ) ), mRemoteEndpoint( remoteEndpoint )
{
	socket_base::reuse_address reuse( true );
	mSocket->set_option( reuse );
}
	
TransportSenderUDP::TransportSenderUDP( WriteHandler writeHandler, const UDPSocketRef &socket, const asio::ip::udp::endpoint &remoteEndpoint )
: TransportSenderBase( writeHandler ), mSocket( socket ), mRemoteEndpoint( remoteEndpoint )
{
}
	
void TransportSenderUDP::send( std::shared_ptr<std::vector<uint8_t>> data )
{
	mSocket->async_send_to( asio::buffer( data->data() + 4, data->size() - 4 ), mRemoteEndpoint,
	[&, data]( const asio::error_code& error, size_t bytesTransferred ) {
		   mWriteHandler( error, bytesTransferred, data );
	});
}
	
void TransportSenderUDP::close()
{
	mSocket->close();
}
	
TransportReceiverUDP::TransportReceiverUDP( DataHandler dataHandler, const asio::ip::udp::endpoint &localEndpoint, asio::io_service &service )
: TransportReceiverBase( dataHandler ), mSocket( new udp::socket( service, localEndpoint ) ), mAmountToReceive( 4096 )
{
	asio::socket_base::reuse_address reuse( true );
	mSocket->set_option( reuse );
}
	
TransportReceiverUDP::TransportReceiverUDP( DataHandler dataHandler, const UDPSocketRef &socket )
: TransportReceiverBase( dataHandler ), mSocket( socket ), mAmountToReceive( 4096 )
{
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