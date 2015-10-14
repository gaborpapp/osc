//
//  Server.h
//  Test
//
//  Created by ryan bartley on 9/3/15.
//
//

#pragma once

#include "TransportBase.h"

namespace osc {
	
using Listener = std::function<void( const Message &message )>;

class ReceiverBase {
public:
	using ErrorHandler = std::function<void( const std::string & )>;
	using Listeners = std::vector<std::pair<std::string, Listener>>;
	
	ReceiverBase( const ReceiverBase &other ) = delete;
	ReceiverBase& operator=( const ReceiverBase &other ) = delete;
	ReceiverBase( ReceiverBase &&other ) = default;
	ReceiverBase& operator=( ReceiverBase &&other ) = default;
	
	virtual ~ReceiverBase() = default;
	
	//! Commits the socket to asynchronously receive into the internal buffer. Uses receiveHandler, below, as the completion handler.
	void		listen();
	void		close();
	
	//! Sets a callback, \a listener, to be called when receiving a message with \a address. If a listener exists for this address, \a listener will replace it.
	void		setListener( const std::string &address, Listener listener );
	//! Removes the listener associated with \a address.
	void		removeListener( const std::string &address );
	//! Adds a handler to be called if there are errors with the asynchronous receive.
	void		setErrorHandler( ErrorHandler errorHandler ) { mErrorHandler = errorHandler; }
	
protected:
	ReceiverBase( std::unique_ptr<TransportReceiverBase> transport );
	//! Handles the async receive completion operations
	void receiveHandler( const asio::error_code &error, std::size_t bytesTransferred, asio::streambuf &stream );
	
	bool checkValidity( char* &messageHeading, size_t numBytes, size_t *remainingBytes ) { return true; }
	void dispatchMethods( uint8_t *data, uint32_t size );
	bool decodeData( uint8_t *data, uint32_t size, std::vector<Message> &messages, uint64_t timetag = 0 );
	bool decodeMessage( uint8_t *data, uint32_t size, std::vector<Message> &messages, uint64_t timetag = 0 );
	
	bool patternMatch( const std::string &lhs, const std::string &rhs );
	
	std::unique_ptr<TransportReceiverBase>	mTransportReceiver;
	Listeners								mListeners;
	std::mutex								mListenerMutex;
	ErrorHandler							mErrorHandler;
};
	
class ReceiverUdp : public ReceiverBase {
public:
	ReceiverUdp( uint16_t port,
				 const asio::ip::udp &protocol = asio::ip::udp::v4(),
				 asio::io_service &io = ci::app::App::get()->io_service()  );
	ReceiverUdp( const asio::ip::udp::endpoint &localEndpoint,
				 asio::io_service &io = ci::app::App::get()->io_service() );
	ReceiverUdp( UdpSocketRef socket );
	
	virtual ~ReceiverUdp() = default;
	
protected:
	
};
	
class ReceiverTcp : public ReceiverBase {
public:
	ReceiverTcp( uint16_t port,
				 const asio::ip::tcp &protocol = asio::ip::tcp::v4(),
				 asio::io_service &io = ci::app::App::get()->io_service()  );
	ReceiverTcp( const asio::ip::tcp::endpoint &localEndpoint,
				 asio::io_service &io = ci::app::App::get()->io_service() );
	// TODO: Decide on maybe allowing a constructor for an already constructed acceptor
	
	~ReceiverTcp() = default;
};

}