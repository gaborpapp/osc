//
//  TransportTCP.cpp
//  Test
//
//  Created by ryan bartley on 9/19/15.
//
//

#include "TransportTCP.h"

using namespace asio;
using namespace asio::ip;
using namespace std::placeholders;

namespace osc {
	
TransportSenderTCP::TransportSenderTCP( WriteHandler handler, const tcp::endpoint &localEndpoint, const tcp::endpoint &remoteEndpoint, asio::io_service &io )
: mSocket( new tcp::socket( io, localEndpoint ) ), mRemoteEndpoint( remoteEndpoint )
{
	mWriteHandler = handler;
}
	
TransportSenderTCP::TransportSenderTCP( WriteHandler handler, const TCPSocketRef &socket, const tcp::endpoint &remoteEndpoint )
	: mSocket( socket ), mRemoteEndpoint( remoteEndpoint )
{
	mWriteHandler = handler;
}
	
void TransportSenderTCP::send( std::shared_ptr<std::vector<uint8_t> > data )
{
	mSocket->async_send( asio::buffer( *data ),
	[&, data]( const asio::error_code &error, size_t bytesTransferred ){
		mWriteHandler( error, bytesTransferred, data );
	});
}

using Connection = TransportReceiverTCP::Connection;

Connection::Connection( TCPSocketRef socket, TransportReceiverTCP *transport )
: mSocket( socket ), mTransport( transport )
{
	
}
	
void Connection::read()
{
	asio::async_read( *mSocket, mBuffer,
	[&]( const asio::error_code &error, size_t bytesTransferred ){
		{
			std::lock_guard<std::mutex> lock( mTransport->mDataHandlerMutex );
			mTransport->mDataHandler( error, bytesTransferred, mBuffer );
		}
		if( ! error )
			read();
		else
			close();
	});
}
	
void TransportReceiverTCP::listen()
{
	TCPSocketRef socket = TCPSocketRef( new tcp::socket( mIoService, tcp::endpoint( tcp::v4(), mPort ) ) );
	socket_base::reuse_address reuse( true );
	socket->set_option( reuse );
	mAcceptor.async_accept( *socket, std::bind( &TransportReceiverTCP::onAccept, this, socket, _1 ) );
}
	
void TransportReceiverTCP::onAccept( TCPSocketRef socket, const asio::error_code &error )
{
	Connection connection( socket, this );
	
	mConnections.push_back( connection );
}
	


}