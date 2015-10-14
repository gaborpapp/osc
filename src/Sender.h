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

class SenderBase {
public:
	using ErrorHandler = std::function<void( const std::string & /*oscAddress*/,
											 const std::string & /*socketAddress*/,
											 const std::string & /*error*/)>;
	
	SenderBase( const SenderBase &other ) = delete;
	SenderBase& operator=( const SenderBase &other ) = delete;
	SenderBase( SenderBase &&other ) = default;
	SenderBase& operator=( SenderBase &&other ) = default;
	~SenderBase() = default;
	
	//! Sends \a message to the destination endpoint.
	void send( const Message &message );
	void send( const Bundle &bundle );
	
	//! Adds a handler to be called if there are errors with the asynchronous receive.
	void		setErrorHandler( ErrorHandler errorHandler ) { mErrorHandler = errorHandler; }
	//! Returns the const TransportSenderBase pointer
	const TransportSenderBase* getTransport() const { return mTransportSender.get(); }
	template<typename T>
	const T* getTransport() const { return dynamic_cast<T*>( mTransportSender.get() ); }
	
protected:
	SenderBase( std::unique_ptr<TransportSenderBase> transport );
	
	//!
	void writeHandler( const asio::error_code &error, size_t bytesTransferred, std::shared_ptr<std::vector<uint8_t>> byte_buffer );
	
	std::unique_ptr<TransportSenderBase>	mTransportSender;
	ErrorHandler							mErrorHandler;
};
	
class SenderUdp : public SenderBase {
public:
	SenderUdp( uint16_t localPort, const std::string &destinationHost, uint16_t destinationPort, const asio::ip::udp &protocol = asio::ip::udp::v4(), asio::io_service &io = ci::app::App::get()->io_service() );
	SenderUdp( uint16_t localPort,  const asio::ip::udp::endpoint &destination, const asio::ip::udp &protocol = asio::ip::udp::v4(), asio::io_service &io = ci::app::App::get()->io_service() );
	SenderUdp( const UdpSocketRef &socket, const asio::ip::udp::endpoint &destination );
};
	
class SenderTcp : public SenderBase {
public:
	SenderTcp( uint16_t localPort, const std::string &destinationHost, uint16_t destinationPort, const asio::ip::tcp &protocol = asio::ip::tcp::v4(), asio::io_service &io = ci::app::App::get()->io_service() );
	SenderTcp( uint16_t localPort,  const asio::ip::tcp::endpoint &destination, const asio::ip::tcp &protocol = asio::ip::tcp::v4(), asio::io_service &io = ci::app::App::get()->io_service() );
	SenderTcp( const TcpSocketRef &socket, const asio::ip::tcp::endpoint &destination );
	
	void connect();
};

}