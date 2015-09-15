//
//  Sender.h
//  Test
//
//  Created by ryan bartley on 9/4/15.
//
//

#pragma once

#include "asio/asio.hpp"
#include "Message.h"
#include "Bundle.hpp"

namespace osc {

class Sender {
public:
	using ErrorHandler = std::function<void( const std::string & /*address*/,
											const asio::ip::udp::endpoint &/*endpoint*/,
											const std::string &/*error*/)>;
	
	Sender( uint16_t localPort, const std::string &destinationHost, uint16_t destinationPort, const asio::ip::udp &protocol = asio::ip::udp::v4(), asio::io_service &io = ci::app::App::get()->io_service() );
	Sender( uint16_t localPort,  const asio::ip::udp::endpoint &destination, const asio::ip::udp &protocol = asio::ip::udp::v4(), asio::io_service &io = ci::app::App::get()->io_service() );
	Sender( const UDPSocketRef &socket, const asio::ip::udp::endpoint &destination );
	
	Sender( const Sender &other ) = delete;
	Sender& operator=( const Sender &other ) = delete;
	Sender( Sender &&other ) = default;
	Sender& operator=( Sender &&other ) = default;
	
	//! Sends \a message to the destination endpoint.
	void send( const Message &message );
	//! Sends \a bundle to the destination endpoint.
	void send( const Bundle &bundle );
	
	//! Adds a handler to be called if there are errors with the asynchronous receive.
	void		setErrorHandler( ErrorHandler errorHandler ) { mErrorHandler = errorHandler; }
	//! Returns a const reference to the underlying SocketRef.
	const UDPSocketRef&			getSocket() { return mSocket; }
	//! Returns the local endpoint associated with this socket.
	asio::ip::udp::endpoint		getLocalEndpoint() { return mSocket->local_endpoint(); }
	//! Returns the remote endpoint associated with this sender.
	const asio::ip::udp::endpoint& getRemoteEndpoint() { return mDestinationEndpoint; }
	
private:
	//!
	void writeHandler( const asio::error_code &error, size_t bytesTransferred, std::shared_ptr<std::vector<uint8_t>> &byte_buffer, std::string address );
	//!
	void writeAsync( std::shared_ptr<std::vector<uint8_t>> cache, const std::string &address );
	
	UDPSocketRef			mSocket;
	asio::ip::udp::endpoint mDestinationEndpoint;
	ErrorHandler			mErrorHandler;
};

}