//
//  TransportTCP.h
//  Test
//
//  Created by ryan bartley on 9/19/15.
//
//

#pragma once

#include "TransportBase.h"

namespace osc {
	
class TransportSenderTCP : public TransportSenderBase {
public:
	virtual ~TransportSenderTCP() = default;
	//! Sends \a message to the destination endpoint.
	virtual void send( std::shared_ptr<std::vector<uint8_t>> data );
	
	virtual asio::ip::address getRemoteAddress() const;
	virtual asio::ip::address getLocalAddress() const;
	
protected:
	
	TCPSocketRef			mSocket;
	asio::ip::tcp::endpoint mDestinationEndpoint;
};

class TransportReceiverTCP : public TransportReceiverBase {
public:
	virtual ~TransportReceiverTCP() = default;
	//!
	virtual void listen();
	virtual void close();
	
	virtual asio::ip::address getLocalAddress() const;
	
	struct Connection {
		
		Connection( TCPSocketRef socket, TransportReceiverTCP* transport );
		Connection( const Connection &other ) = default;
		Connection( Connection &&other ) = default;
		Connection& operator=( const Connection &other ) = default;
		Connection& operator=( Connection &&other ) = default;
		
		~Connection() { mTransport = nullptr; }
		
		TCPSocketRef			mSocket;
		TransportReceiverTCP*	mTransport;
	};
	
protected:
	TransportReceiverTCP( );
	
	void onAccept( TCPSocketRef socket, const asio::error_code &error );
	
	asio::io_service			&mIoService;
	asio::ip::tcp::acceptor		mAcceptor;
	std::vector<Connection>		mConnections;
};

}
