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
	
	Sender( asio::io_service &io = ci::app::App::get()->io_service() );
	
	Sender( const Sender &other ) = delete;
	Sender& operator=( const Sender &other ) = delete;
	
	Sender( Sender &&other );
	Sender& operator=( Sender &&other );
	
	void send( const Message &message, const asio::ip::udp::endpoint &recipient );
	void send( const Message &message, const std::string &host, uint16_t port );
	void send( const Message &message, const std::vector<asio::ip::udp::endpoint> &recipients );
	void broadcast( const Message &message, uint16_t port );
	
	void send( const Bundle &bundle, const asio::ip::udp::endpoint &recipient );
	void send( const Bundle &bundle, const std::string &host, uint16_t port );
	void send( const Bundle &bundle, const std::vector<asio::ip::udp::endpoint> &recipients );
	void broadcast( const Bundle &bundle, uint16_t port );
	
	void setErrorHandler( ErrorHandler errorHandler ) { mErrorHandler = errorHandler; }
	
	const asio::ip::udp::socket& getSocket() { return mSocket; }
	asio::ip::udp::endpoint getLocalEndpoint() { return mSocket.local_endpoint(); }
	
private:
	void writeHandler( const asio::error_code &error, size_t bytesTransferred, std::shared_ptr<std::vector<uint8_t>> &byte_buffer, std::string address, asio::ip::udp::endpoint recipient );
	
	void writeAsync( std::shared_ptr<std::vector<uint8_t>> cache, const std::string &address, const asio::ip::udp::endpoint &recipient );
	
	asio::ip::udp::socket mSocket;
	
	ErrorHandler mErrorHandler;
};

}