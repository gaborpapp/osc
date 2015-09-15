//
//  Transport.h
//  Test
//
//  Created by ryan bartley on 9/11/15.
//
//

#pragma once

#include "Message.h"
#include "Bundle.hpp"

namespace osc {
	
class TransportSenderBase {
public:
	
	virtual ~TransportSenderBase() = default;
	//! Sends \a message to the destination endpoint.
	virtual void send( const std::shared_ptr<std::vector<uint8_t>> &data ) = 0;
	
protected:
	virtual void writeHandler( const asio::error_code &error, size_t bytesTransferred, std::shared_ptr<std::vector<uint8_t>> &byte_buffer, std::string address ) = 0;
	
	asio::streambuf mBuffer;
};
	
class TransportReceiverBase {
public:
	
	virtual ~TransportReceiverBase() = default;
	//! 
	virtual void listen() = 0;
	
protected:
	virtual void receiveHandler( const asio::error_code &error,
								std::size_t bytesTransferred ) = 0;
	
	asio::streambuf mBuffer;
};
	
}
