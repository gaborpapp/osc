//
//  ClientUDP.h
//  Test
//
//  Created by ryan bartley on 9/11/15.
//
//

#pragma once

#include "TransportBase.h"

namespace osc { namespace udp {
	
class TransportSenderUDP : public TransportSenderBase {
public:
	virtual ~TransportSenderUDP() = default;
	
	void send( const std::shared_ptr<std::vector<uint8_t>> &data ) override;
	
	const UDPSocketRef& getSocket() { return mSocket; }
	
protected:
	TransportSenderUDP( const asio::ip::udp::endpoint &localEndpoint, const asio::ip::udp::endpoint &remoteEndpoint, asio::io_service &service );
	TransportSenderUDP( const UDPSocketRef &socket, const asio::ip::udp::endpoint &remoteEndpoint );
	
	void writeHandler( const asio::error_code &error, size_t bytesTransferred, std::shared_ptr<std::vector<uint8_t>> &byte_buffer, std::string address ) override;
	
	UDPSocketRef			mSocket;
	asio::ip::udp::endpoint mRemoteEndpoint;
	
	friend class Sender;
};
	
class TransportReceiverUDP : public TransportReceiverBase {
public:
	
	virtual ~TransportReceiverUDP() = default;
	
	void listen() override;
	
	const UDPSocketRef& getSocket() { return mSocket; }

protected:
	TransportReceiverUDP( const asio::ip::udp::endpoint &localEndpoint, asio::io_service &io );
	TransportReceiverUDP( const UDPSocketRef &socket );
	
	void receiveHandler( const asio::error_code &error,
								std::size_t bytesTransferred ) override;
	
	UDPSocketRef			mSocket;
	asio::ip::udp::endpoint mLocalEndpoint;
	
	friend class Receiver;
};
	
}}
