//
//  TransportTCP.cpp
//  Test
//
//  Created by ryan bartley on 9/19/15.
//
//

#include "TransportTCP.h"
#include "Utils.h"
#include "cinder/Log.h"

using namespace std;
using namespace asio;
using namespace asio::ip;
using namespace std::placeholders;

namespace osc {
	
TransportSenderTCP::TransportSenderTCP( WriteHandler handler, const tcp::endpoint &localEndpoint, const tcp::endpoint &remoteEndpoint, asio::io_service &io )
: mSocket( new tcp::socket( io, localEndpoint ) ), mRemoteEndpoint( remoteEndpoint )
{
	socket_base::reuse_address reuse( true );
	mSocket->set_option( reuse );
	mWriteHandler = handler;
}
	
TransportSenderTCP::TransportSenderTCP( WriteHandler handler, const TCPSocketRef &socket, const tcp::endpoint &remoteEndpoint )
: mSocket( socket ), mRemoteEndpoint( remoteEndpoint )
{
	mWriteHandler = handler;
}
	
void TransportSenderTCP::connect()
{
	mSocket->async_connect( mRemoteEndpoint, std::bind( &TransportSenderTCP::onConnect, this, std::placeholders::_1 ) );
}
	
void TransportSenderTCP::send( std::shared_ptr<std::vector<uint8_t> > data )
{
	data->push_back( (uint8_t)'\n' );
	mSocket->async_send( asio::buffer( *data ),
	[&, data]( const asio::error_code &error, size_t bytesTransferred ){
		mWriteHandler( error, bytesTransferred, data );
	});
}
	
void TransportSenderTCP::onConnect( const asio::error_code &error )
{
	if( error )
		CI_LOG_E( error.message() );
}

using Connection = TransportReceiverTCP::Connection;

Connection::Connection( TCPSocketRef socket, TransportReceiverTCP *transport )
: mSocket( socket ), mTransport( transport ), mDataBuffer( 4096 )
{
}
	
Connection::Connection( Connection && other ) noexcept
: mSocket( move( other.mSocket ) ), mTransport( other.mTransport )
{
	//TODO: Decide what to do about buffer copy
}
	
Connection& Connection::operator=( Connection && other ) noexcept
{
	if( this != &other ) {
		mSocket = move( other.mSocket );
		mTransport = other.mTransport;
	}
	return *this;
}
	
Connection::~Connection()
{
	close();
	mTransport = nullptr;
}
	
typedef asio::buffers_iterator<
asio::streambuf::const_buffers_type> iterator;

std::pair<iterator, bool> match_size(iterator begin, iterator end)
{
	iterator i = begin;
	osc::ByteArray<4> data;
	int inc = 0;
	while ( i != end && inc < 4 ) {
		data[inc++] = *i++;
	}
	int* numBytes = reinterpret_cast<int*>( data.data() );
	if( inc == 4 && *numBytes > 0 && *numBytes + 4 + 1 <= std::distance( begin, end ) ) {
		return { begin + *numBytes + 4 + 1, true };
	}
	else {
		return { begin, false };
	}
}
	
void Connection::read()
{
	asio::async_read_until( *mSocket, mBuffer, &match_size, std::bind( &Connection::onRead, this, _1, _2 ) );
}
	
void Connection::onRead( const asio::error_code &error, size_t bytesTransferred )
{
	{
		std::lock_guard<std::mutex> lock( mTransport->mDataHandlerMutex );
		mTransport->mDataHandler( error, bytesTransferred, mBuffer );
	}
	// TODO: Figure these errors out.
	//		if( ! error )
	read();
}
	
void Connection::close()
{
	mSocket->close();
}
	
TransportReceiverTCP::TransportReceiverTCP( DataHandler dataHandler, const asio::ip::tcp::endpoint &localEndpoint, asio::io_service &service )
: TransportReceiverBase( 4096 ), mIoService( service ), mAcceptor( mIoService, localEndpoint, true )
{
	mDataHandler = dataHandler;
}
	
void TransportReceiverTCP::listen()
{
	auto socket = TCPSocketRef( new tcp::socket( mIoService ) );
	
	if( ! mAcceptor.is_open() );
//		mAcceptor.open();
	
	mAcceptor.async_accept( *socket, std::bind( &TransportReceiverTCP::onAccept, this, socket, _1 ) );
}
	
void TransportReceiverTCP::onAccept( TCPSocketRef socket, const asio::error_code &error )
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
	
void TransportReceiverTCP::close()
{
	mAcceptor.close();
	std::lock_guard<std::mutex> lock( mConnectionMutex );
	for( auto & connection : mConnections ) {
		connection.close();
	}
	mConnections.clear();
}

}