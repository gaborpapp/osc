//
//  ClientUdp.h
//  Test
//
//  Created by ryan bartley on 9/11/15.
//
//

#pragma once

#include "TransportBase.h"

namespace osc {
	
class TransportSenderUdp : public TransportSenderBase {
public:
	using protocol = asio::ip::udp;
	
	virtual ~TransportSenderUdp() = default;
	
	//! Sends the data array /a data to the remote endpoint using the Udp socket, asynchronously.
	void send( std::shared_ptr<std::vector<uint8_t>> data ) override;
	//!
	void close();
	
	//! Returns the local address of the endpoint associated with this socket.
	asio::ip::address getLocalAddress() const override { return mSocket->local_endpoint().address(); }
	//! Returns the remote address of the endpoint associated with this transport.
	asio::ip::address getRemoteAddress() const override { return mRemoteEndpoint.address(); }
	
protected:
	TransportSenderUdp( WriteHandler writeHandler,
					   const protocol::endpoint &localEndpoint,
					   const protocol::endpoint &remoteEndpoint,
					   asio::io_service &service );
	TransportSenderUdp( WriteHandler writeHandler,
					   const UdpSocketRef &socket,
					   const protocol::endpoint &remoteEndpoint );
	
	UdpSocketRef			mSocket;
	asio::ip::udp::endpoint mRemoteEndpoint;
	
	friend class SenderUdp;
};
	
class TransportReceiverUdp : public TransportReceiverBase {
public:
	using protocol = asio::ip::udp;
	
	virtual ~TransportReceiverUdp() = default;
	
	//!
	void listen() override;
	//!
	void close() override;
	
	//! Returns the underlying Udp socket associated with this transport.
	const UdpSocketRef& getSocket() { return mSocket; }
	//! Returns the local address of the endpoint associated with this socket.
	asio::ip::address getLocalAddress() const { return mSocket->local_endpoint().address(); }
	
protected:
	TransportReceiverUdp( DataHandler dataHandler, const protocol::endpoint &localEndpoint, asio::io_service &service );
	TransportReceiverUdp( DataHandler dataHandler, const UdpSocketRef &socket );
	
	UdpSocketRef			mSocket;
	asio::streambuf			mBuffer;
	uint32_t				mAmountToReceive;
	
	friend class ReceiverUdp;
};
	
}
