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

class Receiver {
public:
	using ErrorHandler = std::function<void( const std::string & )>;
	using Listeners = std::vector<std::pair<std::string, Listener>>;
	
	Receiver( uint16_t port, const asio::ip::udp &protocol = asio::ip::udp::v4(), asio::io_service &io = ci::app::App::get()->io_service()  );
	Receiver( const asio::ip::udp::endpoint &localEndpoint, asio::io_service &io = ci::app::App::get()->io_service() );
	Receiver( UDPSocketRef socket );
	
	Receiver( const Receiver &other ) = delete;
	Receiver& operator=( const Receiver &other ) = delete;
	
	Receiver( Receiver &&other ) = default;
	Receiver& operator=( Receiver &&other ) = default;
	
	~Receiver() = default;
	
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

}