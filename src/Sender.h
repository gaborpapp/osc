//
//  Sender.h
//  Test
//
//  Created by ryan bartley on 9/4/15.
//
//

#pragma once

#include "TransportBase.h"

namespace osc {

class Sender {
public:
	using ErrorHandler = std::function<void( const std::string & /*oscAddress*/,
											const std::string &/*socketAddress*/,
											const std::string &/*error*/)>;
	//! UDP SENDER CONSTRUCTORS
	
	Sender( uint16_t localPort, const std::string &destinationHost, uint16_t destinationPort, const asio::ip::udp &protocol = asio::ip::udp::v4(), asio::io_service &io = ci::app::App::get()->io_service() );
	Sender( uint16_t localPort,  const asio::ip::udp::endpoint &destination, const asio::ip::udp &protocol = asio::ip::udp::v4(), asio::io_service &io = ci::app::App::get()->io_service() );
	Sender( const UDPSocketRef &socket, const asio::ip::udp::endpoint &destination );
	
	//! TCP SENDER CONSTRUCTORS
	
	Sender( const Sender &other ) = delete;
	Sender& operator=( const Sender &other ) = delete;
	Sender( Sender &&other ) = default;
	Sender& operator=( Sender &&other ) = default;
	~Sender() = default;
	
	//! Sends \a message to the destination endpoint.
	void send( const Message &message );
	//! Sends \a bundle to the destination endpoint.
	void send( const Bundle &bundle );
	
	//! Adds a handler to be called if there are errors with the asynchronous receive.
	void		setErrorHandler( ErrorHandler errorHandler ) { mErrorHandler = errorHandler; }
	//! Returns the const TransportSenderBase pointer
	const TransportSenderBase* getTransport() const { return mTransportSender.get(); }
	template<typename T>
	const T* getTransport() const { return dynamic_cast<T*>( mTransportSender.get() ); }
	
private:
	//!
	void writeHandler( const asio::error_code &error, size_t bytesTransferred, std::shared_ptr<std::vector<uint8_t>> byte_buffer );
	
	std::unique_ptr<TransportSenderBase>	mTransportSender;
	ErrorHandler							mErrorHandler;
};

}