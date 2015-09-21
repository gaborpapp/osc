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
	
	//! Returns the underlying udp socket associated with this transport.
	const UDPSocketRef& getSocket() const { return mSocket; }
	//! Returns the local udp endpoint associated with the socket.
	asio::ip::udp::endpoint getLocalEndpoint() const { return mSocket->local_endpoint(); }
	//! Returns a const reference to the remote udp endpoint this transport is sending to.
	const asio::ip::udp::endpoint& getRemoteEndpoint() const { return mRemoteEndpoint; }
	
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
	
	friend class Sender;
};
	
class TransportReceiverUDP : public TransportReceiverBase {
public:
	virtual ~TransportReceiverUDP() = default;
	
	void listen() override;
	void close() override;
	
	//! Returns the underlying udp socket associated with this transport.
	const UDPSocketRef& getSocket() { return mSocket; }
	//! Returns the local address of the endpoint associated with this socket.
	asio::ip::address getLocalAddress() const { return mSocket->local_endpoint().address(); }
	
	void setAmountToReceive( size_t amountToReceive ) { mAmountToReceive = amountToReceive; }
	size_t getAmountToReceive() const { return mAmountToReceive; }
	
protected:
	TransportReceiverUDP( DataHandler dataHandler, const asio::ip::udp::endpoint &localEndpoint, asio::io_service &service );
	TransportReceiverUDP( DataHandler dataHandler, const UDPSocketRef &socket );
	
	UDPSocketRef			mSocket;
	size_t					mAmountToReceive;
	friend class Receiver;
};
	
}
