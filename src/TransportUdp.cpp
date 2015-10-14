//
//  ClientUdp.cpp
//  Test
//
//  Created by ryan bartley on 9/11/15.
//
//

#include "TransportUdp.h"

using namespace std;
using namespace asio;
using namespace asio::ip;

namespace osc {
	
TransportSenderUdp::TransportSenderUdp( WriteHandler writeHandler, const udp::endpoint &localEndpoint, const udp::endpoint &remoteEndpoint, io_service &service )
: TransportSenderBase( writeHandler ), mSocket( new udp::socket( service, localEndpoint ) ), mRemoteEndpoint( remoteEndpoint )
{
	socket_base::reuse_address reuse( true );
	mSocket->set_option( reuse );
}
	
TransportSenderUdp::TransportSenderUdp( WriteHandler writeHandler, const UdpSocketRef &socket, const udp::endpoint &remoteEndpoint )
: TransportSenderBase( writeHandler ), mSocket( socket ), mRemoteEndpoint( remoteEndpoint )
{
}
	
void TransportSenderUdp::send( std::shared_ptr<std::vector<uint8_t>> data )
{
	mSocket->async_send_to( asio::buffer( data->data() + 4, data->size() - 4 ), mRemoteEndpoint,
	[&, data]( const asio::error_code& error, size_t bytesTransferred ) {
		   mWriteHandler( error, bytesTransferred, data );
	});
}
	
void TransportSenderUdp::close()
{
	mSocket->close();
}
	
TransportReceiverUdp::TransportReceiverUdp( DataHandler dataHandler, const udp::endpoint &localEndpoint, asio::io_service &service )
: TransportReceiverBase( dataHandler ), mSocket( new udp::socket( service, localEndpoint ) ), mAmountToReceive( 4096 )
{
	asio::socket_base::reuse_address reuse( true );
	mSocket->set_option( reuse );
}
	
TransportReceiverUdp::TransportReceiverUdp( DataHandler dataHandler, const UdpSocketRef &socket )
: TransportReceiverBase( dataHandler ), mSocket( socket ), mAmountToReceive( 4096 )
{
}
	
void TransportReceiverUdp::listen()
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
	
void TransportReceiverUdp::close()
{
	mSocket->close();
}

	
}