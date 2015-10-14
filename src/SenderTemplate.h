//
//  Sender.h
//  Test
//
//  Created by ryan bartley on 9/4/15.
//
//

#pragma once

#include "TransportBase.h"
#include "TransportUdp.h"
#include "TransportTcp.h"

namespace osc {

template<typename Transport>
class Sender {
public:
	using SocketRef = std::shared_ptr<asio::basic_datagram_socket<typename Transport::protocol>>;
	using ErrorHandler = std::function<void( const std::string & /*oscAddress*/,
										 const std::string & /*socketAddress*/,
										 const std::string & /*error*/)>;
	Sender( uint16_t localPort,
		   const std::string &destinationHost,
		   uint16_t destinationPort,
		   const typename Transport::protocol &protocol = typename Transport::protocol::v4(),
		   asio::io_service &io = ci::app::App::get()->io_service() );
	
	Sender( uint16_t localPort,
		   const typename Transport::protocol::endpoint &destination,
		   const typename Transport::protocol &protocol = typename Transport::protocol::v4(),
		   asio::io_service &io = ci::app::App::get()->io_service() );
	
	Sender( const UdpSocketRef &socket, const asio::ip::udp::endpoint &destination );
	
	Sender( const Sender &other ) = delete;
	Sender& operator=( const Sender &other ) = delete;
	Sender( Sender &&other ) = default;
	Sender& operator=( Sender &&other ) = default;
	~Sender() = default;
	
	void connect();
	
	//! Sends \a message to the destination endpoint.
	void send( const Message &message );
	void send( const Bundle &bundle );
	
	//! Adds a handler to be called if there are errors with the asynchronous receive.
	void		setErrorHandler( ErrorHandler errorHandler ) { mErrorHandler = errorHandler; }
	
protected:
	//!
	void writeHandler( const asio::error_code &error, size_t bytesTransferred, std::shared_ptr<std::vector<uint8_t>> byte_buffer );
	
	Transport		mTransportSender;
	ErrorHandler	mErrorHandler;
};
	
using SenderUdpTemp = Sender<TransportSenderUdp>;
using SenderTcpTemp = Sender<TransportSenderTcp>;
	
}