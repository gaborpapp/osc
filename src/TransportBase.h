//
//  Transport.h
//  Test
//
//  Created by ryan bartley on 9/11/15.
//
//

#pragma once

#include "Message.h"
#include "Bundle.h"

namespace osc {
	
class TransportSenderBase {
public:
	using WriteHandler = std::function<void(const asio::error_code &/*error*/,
											size_t/*bytesTransferred*/,
											std::shared_ptr<std::vector<uint8_t>> /*data*/)>;
	
	virtual ~TransportSenderBase() = default;
	//! Sends \a message to the destination endpoint.
	virtual void send( std::shared_ptr<std::vector<uint8_t>> data ) = 0;
	virtual void close() = 0;
	
	virtual asio::ip::address getRemoteAddress() const = 0;
	virtual asio::ip::address getLocalAddress() const = 0;
	
protected:
	TransportSenderBase( WriteHandler writeHandler )
	: mWriteHandler( writeHandler ) {}
	
	WriteHandler mWriteHandler;
};
	
class TransportReceiverBase {
public:
	using DataHandler = std::function<void ( const asio::error_code &/*error*/,
											 size_t/*bytesTransferred*/,
											 asio::streambuf &/*data*/ )>;
	
	virtual ~TransportReceiverBase() = default;
	
	virtual void listen() = 0;
	virtual void close() = 0;
	
	virtual asio::ip::address getLocalAddress() const = 0;
	
protected:
	TransportReceiverBase( DataHandler dataHandler )
	: mDataHandler( dataHandler ) {}
	
	DataHandler		mDataHandler;
};
	
}
