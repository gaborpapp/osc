//
//  ClientUdp.h
//  Test
//
//  Created by ryan bartley on 9/11/15.
//
//

#pragma once

#include "Utils.h"

namespace osc {
	
class TransportSenderUdp {
public:
	using protocol = asio::ip::udp;
	using ErrorHandler = std::function<void( const std::string & /*oscAddress*/,
											const std::string & /*socketAddress*/,
											const std::string & /*error*/)>;
	
	//! Returns the local address of the endpoint associated with this socket.
	const asio::ip::udp::endpoint& getRemoteEndpoint() const { return mRemoteEndpoint; }
	//! Returns the remote address of the endpoint associated with this transport.
	asio::ip::udp::endpoint getLocalEndpoint() const { return mSocket->local_endpoint(); }
	
	void setErrorHandler( ErrorHandler errorHandler ) { mErrorHandler = errorHandler; }
	void setWriteHandler( WriteHandler writeHandler ) { mWritehandler = writeHandler; }
	
protected:
	TransportSenderUdp( const protocol::endpoint &localEndpoint,
					    const protocol::endpoint &remoteEndpoint,
					    asio::io_service &service );
	TransportSenderUdp( const UdpSocketRef &socket,
					    const protocol::endpoint &remoteEndpoint );
	
	~TransportSenderUdp() = default;
	
	//! Sends the data array /a data to the remote endpoint using the Udp socket, asynchronously.
	void sendImpl( std::shared_ptr<std::vector<uint8_t>> data );
	//!
	void closeImpl();
	
	UdpSocketRef			mSocket;
	asio::ip::udp::endpoint mRemoteEndpoint;
	WriteHandler			mWritehandler;
	ErrorHandler			mErrorHandler;
	
	friend class SenderUdp;
};

class TransportReceiverUdp {
public:
	using protocol = asio::ip::udp;
	
	//!
	void listen();
	//!
	void close();
	
	//! Returns the underlying Udp socket associated with this transport.
	const UdpSocketRef& getSocket() { return mSocket; }
	//! Returns the local address of the endpoint associated with this socket.
	asio::ip::address getLocalAddress() const { return mSocket->local_endpoint().address(); }
	
protected:
	TransportReceiverUdp( DataHandler dataHandler,
						  const protocol::endpoint &localEndpoint,
						  asio::io_service &service );
	TransportReceiverUdp( DataHandler dataHandler,
						  const UdpSocketRef &socket );
	
	~TransportReceiverUdp() = default;
	
	UdpSocketRef			mSocket;
	asio::streambuf			mBuffer;
	uint32_t				mAmountToReceive;
	
	friend class ReceiverUdp;
};

}
