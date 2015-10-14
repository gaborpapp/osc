//
//  TransportTcp.cpp
//  Test
//
//  Created by ryan bartley on 9/19/15.
//
//

#include "TransportTcp.h"
#include "Utils.h"
#include "cinder/Log.h"

using namespace std;
using namespace asio;
using namespace asio::ip;
using namespace std::placeholders;

namespace osc {
	
TransportSenderTcp::TransportSenderTcp( WriteHandler writeHandler, const tcp::endpoint &localEndpoint, const tcp::endpoint &remoteEndpoint, asio::io_service &io )
: TransportSenderBase( writeHandler ), mSocket( new tcp::socket( io, localEndpoint ) ), mRemoteEndpoint( remoteEndpoint )
{
	socket_base::reuse_address reuse( true );
	mSocket->set_option( reuse );
}
	
TransportSenderTcp::TransportSenderTcp( WriteHandler writeHandler, const TcpSocketRef &socket, const tcp::endpoint &remoteEndpoint )
: TransportSenderBase( writeHandler ), mSocket( socket ), mRemoteEndpoint( remoteEndpoint )
{
}
	
void TransportSenderTcp::connect()
{
	mSocket->async_connect( mRemoteEndpoint, std::bind( &TransportSenderTcp::onConnect, this, std::placeholders::_1 ) );
}
	
void TransportSenderTcp::send( std::shared_ptr<std::vector<uint8_t> > data )
{
//	data->push_back( (uint8_t)'\n' );
	mSocket->async_send( asio::buffer( *data ),
	[&, data]( const asio::error_code &error, size_t bytesTransferred ){
		mWriteHandler( error, bytesTransferred, data );
	});
}
	
void TransportSenderTcp::onConnect( const asio::error_code &error )
{
	if( error )
		CI_LOG_E( error.message() );
}
	
void TransportSenderTcp::close()
{
	mSocket->close();
}

TransportReceiverTcp::Connection::Connection( TcpSocketRef socket, TransportReceiverTcp *transport )
: mSocket( socket ), mTransport( transport ), mDataBuffer( 4096 )
{
}
	
TransportReceiverTcp::Connection::Connection( Connection && other ) noexcept
: mSocket( move( other.mSocket ) ), mTransport( other.mTransport )
{
	//TODO: Decide what to do about buffer copy
}
	
TransportReceiverTcp::Connection& TransportReceiverTcp::Connection::operator=( Connection && other ) noexcept
{
	if( this != &other ) {
		mSocket = move( other.mSocket );
		mTransport = other.mTransport;
	}
	return *this;
}
	
TransportReceiverTcp::Connection::~Connection()
{
	close();
	mTransport = nullptr;
}

std::pair<iterator, bool> TransportReceiverTcp::Connection::readMatchCondition(iterator begin, iterator end)
{
	iterator i = begin;
	ByteArray<4> data;
	int inc = 0;
	while ( i != end && inc < 4 )
		data[inc++] = *i++;
	
	int numBytes = *reinterpret_cast<int*>( data.data() );
	// swap for big endian from the other side
	numBytes = ntohl( numBytes );
	
	if( inc == 4 && numBytes > 0 && numBytes + 4 <= std::distance( begin, end ) ) {
		return { begin + numBytes + 4, true };
	}
	else {
		return { begin, false };
	}
}
	
void TransportReceiverTcp::Connection::read()
{
	asio::async_read_until( *mSocket, mBuffer, &readMatchCondition,
						   std::bind( &Connection::onReadComplete, this, _1, _2 ) );
}
	
void TransportReceiverTcp::Connection::onReadComplete( const asio::error_code &error, size_t bytesTransferred )
{
	{
		std::lock_guard<std::mutex> lock( mTransport->mDataHandlerMutex );
		mTransport->mDataHandler( error, bytesTransferred, mBuffer );
	}
	// TODO: Figure these errors out.
	read();
}
	
void TransportReceiverTcp::Connection::close()
{
	mSocket->close();
}
	
TransportReceiverTcp::TransportReceiverTcp( DataHandler dataHandler, const tcp::endpoint &localEndpoint, io_service &service )
: TransportReceiverBase( dataHandler ), mIoService( service ), mAcceptor( mIoService, localEndpoint, true )
{
}
	
void TransportReceiverTcp::listen()
{
	auto socket = TcpSocketRef( new tcp::socket( mIoService ) );
	
	if( ! mAcceptor.is_open() );
//		mAcceptor.open();
	
	mAcceptor.async_accept( *socket, std::bind( &TransportReceiverTcp::onAccept, this, socket, _1 ) );
}
	
void TransportReceiverTcp::onAccept( TcpSocketRef socket, const asio::error_code &error )
{
	CI_LOG_V("Accepted a connection");
	if( ! error ) {
		std::lock_guard<std::mutex> lock( mConnectionMutex );
		mConnections.emplace_back( socket, this );
		mConnections.back().read();
	}
	else {
		// TODO: Figure out error interface.
		CI_LOG_E(error.message());
	}
	listen();
}
	
void TransportReceiverTcp::close()
{
	mAcceptor.close();
	std::lock_guard<std::mutex> lock( mConnectionMutex );
	for( auto & connection : mConnections ) {
		connection.close();
	}
	mConnections.clear();
}

}