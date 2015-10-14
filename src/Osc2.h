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
	
	//! Sends \a message to the destination endpoint.
	void send( const Message &message ) { sendImpl( message.getSharedBuffer() ); }
	void send( const Bundle &bundle ) { sendImpl( bundle.getSharedBuffer() );}
	
	void close() { closeImpl(); }
	
	//! Adds a handler to be called if there are errors with the asynchronous receive.
	void		setErrorHandler( ErrorHandler errorHandler ) { mErrorHandler = errorHandler; }
	
protected:
	//!
	void writeHandler( const asio::error_code &error, size_t bytesTransferred, std::shared_ptr<std::vector<uint8_t>> byte_buffer );
	
	virtual void sendImpl( std::shared_ptr<std::vector<uint8_t>> data ) = 0;
	virtual void closeImpl() = 0;
	
	ErrorHandler							mErrorHandler;
	
public:
	SenderBase( const SenderBase &other ) = delete;
	SenderBase& operator=( const SenderBase &other ) = delete;
	SenderBase( SenderBase &&other ) = default;
	SenderBase& operator=( SenderBase &&other ) = default;
	
	virtual ~SenderBase() = default;
};
	
class ReceiverBase {
public:
	using ErrorHandler = std::function<void( const std::string & )>;
	using Listener = std::function<void( const Message &message )>;
	using Listeners = std::vector<std::pair<std::string, Listener>>;
	
	//! Commits the socket to asynchronously receive into the internal buffer. Uses receiveHandler, below, as the completion handler.
	void		listen() { listenImpl(); }
	void		close() { closeImpl(); }
	
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
	
	virtual void listenImpl() = 0;
	virtual void closeImpl() = 0;
	
	Listeners								mListeners;
	std::mutex								mListenerMutex;
	ErrorHandler							mErrorHandler;
	
public:
	ReceiverBase( const ReceiverBase &other ) = delete;
	ReceiverBase& operator=( const ReceiverBase &other ) = delete;
	ReceiverBase( ReceiverBase &&other ) = default;
	ReceiverBase& operator=( ReceiverBase &&other ) = default;
	
	virtual ~ReceiverBase() = default;
};
	
class SenderUdp : public SenderBase {
public:
	using protocol = asio::ip::udp;
	
	SenderUdp( uint16_t localPort,
			   const std::string &destinationHost,
			   uint16_t destinationPort,
			   const protocol &protocol = protocol::v4(),
			   asio::io_service &io = ci::app::App::get()->io_service() );
	SenderUdp( uint16_t localPort,
			   const protocol::endpoint &destination,
			   const protocol &protocol = protocol::v4(),
			   asio::io_service &io = ci::app::App::get()->io_service() );
	SenderUdp( const UdpSocketRef &socket, const protocol::endpoint &destination );
	
	virtual ~SenderUdp() = default;
	
protected:
	
	//! Sends the data array /a data to the remote endpoint using the Udp socket, asynchronously.
	void sendImpl( std::shared_ptr<std::vector<uint8_t>> data ) override;
	//!
	void closeImpl() override;
	
	UdpSocketRef			mSocket;
	protocol::endpoint		mRemoteEndpoint;
	
	friend class SenderUdp;
};

class SenderTcp : public SenderBase {
public:
	using protocol = asio::ip::udp;
	
	SenderTcp( uint16_t localPort,
			   const std::string &destinationHost,
			   uint16_t destinationPort,
			   const protocol &protocol = protocol::v4(),
			   asio::io_service &io = ci::app::App::get()->io_service() );
	SenderTcp( uint16_t localPort,
			   const protocol::endpoint &destination,
			   const protocol &protocol = protocol::v4(),
			   asio::io_service &io = ci::app::App::get()->io_service() );
	SenderTcp( const TcpSocketRef &socket, const asio::ip::tcp::endpoint &destination );
	
	void connect();
	
	virtual ~SenderTcp() = default;
	
protected:
	
	//! Sends \a message to the destination endpoint.
	void sendImpl( std::shared_ptr<std::vector<uint8_t>> data ) override;
	void closeImpl() override;
	
	void onConnect( const asio::error_code &error );
	
	TcpSocketRef			mSocket;
	asio::ip::tcp::endpoint mRemoteEndpoint;
	
	friend class SenderTcp;
};
	
class ReceiverUdp : public ReceiverBase {
public:
	ReceiverUdp( uint16_t port,
			 const asio::ip::udp &protocol = asio::ip::udp::v4(),
			 asio::io_service &io = ci::app::App::get()->io_service()  );
	ReceiverUdp( const asio::ip::udp::endpoint &localEndpoint,
			 asio::io_service &io = ci::app::App::get()->io_service() );
	ReceiverUdp( UdpSocketRef socket );
	
	using protocol = asio::ip::udp;
	
	virtual ~ReceiverUdp() = default;
	
	//! Returns the underlying Udp socket associated with this transport.
	const UdpSocketRef& getSocket() { return mSocket; }
	//! Returns the local address of the endpoint associated with this socket.
	asio::ip::address getLocalAddress() const { return mSocket->local_endpoint().address(); }
	
protected:
	
	//!
	void listenImpl() override;
	//!
	void closeImpl() override;
	
	UdpSocketRef			mSocket;
	asio::streambuf			mBuffer;
	uint32_t				mAmountToReceive;
	
	friend class ReceiverUdp;
	
};

class ReceiverTcp : public ReceiverBase {
public:
	ReceiverTcp( uint16_t port,
			 const asio::ip::tcp &protocol = asio::ip::tcp::v4(),
			 asio::io_service &io = ci::app::App::get()->io_service()  );
	ReceiverTcp( const asio::ip::tcp::endpoint &localEndpoint,
			 asio::io_service &io = ci::app::App::get()->io_service() );
	
	virtual ~ReceiverTcp() = default;
	
protected:
	
	void onAccept( TcpSocketRef socket, const asio::error_code &error );
	void onRead( const asio::error_code &error, size_t bytesTransferred );
	bool readComplete( const asio::error_code &error, size_t bytesTransferred );
	
	struct Connection {
		Connection( TcpSocketRef socket, ReceiverTcp* transport );
		Connection( const Connection &other ) = delete;
		Connection& operator=( const Connection &other ) = delete;
		Connection( Connection &&other ) noexcept;
		Connection& operator=( Connection &&other ) noexcept;
		
		~Connection();
		
		asio::ip::tcp::endpoint getRemoteEndpoint() { return mSocket->remote_endpoint(); }
		asio::ip::tcp::endpoint getLocalEndpoint() { return  mSocket->local_endpoint(); }
		
		void read();
		static std::pair<iterator, bool> readMatchCondition( iterator begin, iterator end );
		void onReadComplete( const asio::error_code &error, size_t bytesTransferred );
		void close();
		
		TcpSocketRef			mSocket;
		ReceiverTcp*			mTransport;
		asio::streambuf			mBuffer;
		std::vector<uint8_t>	mDataBuffer;
	};
	
	void listenImpl() override;
	void closeImpl() override;
	
	asio::io_service			&mIoService;
	std::mutex					mDataHandlerMutex, mConnectionMutex;
	asio::ip::tcp::acceptor		mAcceptor;
	std::vector<Connection>		mConnections;
	
	friend class ReceiverTcp;
	friend class Connection;
};

}