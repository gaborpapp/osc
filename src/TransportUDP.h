//
//  ClientUDP.h
//  Test
//
//  Created by ryan bartley on 9/11/15.
//
//

#pragma once

#include "TransportBase.h"

namespace osc {
	
class TransportSenderUDP : public TransportSenderBase {
public:
	virtual ~TransportSenderUDP() = default;
	
	//! Sends the data array /a data to the remote endpoint using the udp socket, asynchronously.
	void send( std::shared_ptr<std::vector<uint8_t>> data ) override;
	//!
	void close();
	
	//! Returns the local address of the endpoint associated with this socket.
	asio::ip::address getLocalAddress() const override { return mSocket->local_endpoint().address(); }
	//! Returns the remote address of the endpoint associated with this transport.
	asio::ip::address getRemoteAddress() const override { return mRemoteEndpoint.address(); }
	
protected:
	TransportSenderUDP( WriteHandler writeHandler,
					   const asio::ip::udp::endpoint &localEndpoint,
					   const asio::ip::udp::endpoint &remoteEndpoint,
					   asio::io_service &service );
	TransportSenderUDP( WriteHandler writeHandler,
					   const UDPSocketRef &socket,
					   const asio::ip::udp::endpoint &remoteEndpoint );
	
	UDPSocketRef			mSocket;
	asio::ip::udp::endpoint mRemoteEndpoint;
	
	friend class SenderUDP;
};
	
class TransportReceiverUDP : public TransportReceiverBase {
public:
	virtual ~TransportReceiverUDP() = default;
	
	//!
	void listen() override;
	//!
	void close() override;
	
	//! Returns the underlying udp socket associated with this transport.
	const UDPSocketRef& getSocket() { return mSocket; }
	//! Returns the local address of the endpoint associated with this socket.
	asio::ip::address getLocalAddress() const { return mSocket->local_endpoint().address(); }
	
protected:
	TransportReceiverUDP( DataHandler dataHandler, const asio::ip::udp::endpoint &localEndpoint, asio::io_service &service );
	TransportReceiverUDP( DataHandler dataHandler, const UDPSocketRef &socket );
	
	UDPSocketRef			mSocket;
	asio::streambuf			mBuffer;
	uint32_t				mAmountToReceive;
	
	friend class ReceiverUDP;
};
	
}
