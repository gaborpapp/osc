#pragma once

#include "TransportBase.h"

namespace osc {
	
class TransportSenderTCP : public TransportSenderBase {
public:
	virtual ~TransportSenderTCP() = default;
	//! Sends \a message to the destination endpoint.
	virtual void send( std::shared_ptr<std::vector<uint8_t>> data );
	
	//! Returns the underlying tcp socket associated with this transport.
	const TCPSocketRef& getSocket() const { return mSocket; }
	//! Returns the local tcp endpoint associated with the socket.
	asio::ip::tcp::endpoint getLocalEndpoint() const { return mSocket->local_endpoint(); }
	//! Returns a const reference to the remote tcp endpoint this transport is sending to.
	const asio::ip::tcp::endpoint& getRemoteEndpoint() const { return mRemoteEndpoint; }
	
	//! Returns the local address of the endpoint associated with this socket.
	asio::ip::address getLocalAddress() const override { return mSocket->local_endpoint().address(); }
	//! Returns the remote address of the endpoint associated with this transport.
	asio::ip::address getRemoteAddress() const override { return mRemoteEndpoint.address(); }
	
	void connect();
	
protected:
	TransportSenderTCP(	WriteHandler handler,
						const asio::ip::tcp::endpoint &localEndpoint,
						const asio::ip::tcp::endpoint &remoteEndpoint,
						asio::io_service &io );
	TransportSenderTCP( WriteHandler writeHandler,
						const TCPSocketRef &socket,
						const asio::ip::tcp::endpoint &remoteEndpoint );
	
	void onConnect( const asio::error_code &error );
	
	TCPSocketRef			mSocket;
	asio::ip::tcp::endpoint mRemoteEndpoint;
	
	friend class SenderTCP;
};

class TransportReceiverTCP : public TransportReceiverBase {
public:
	virtual ~TransportReceiverTCP() = default;
	
	void listen() override;
	void close() override;
	
	asio::ip::address getLocalAddress() const override { return mAcceptor.local_endpoint().address(); }
	
	struct Connection {
		Connection( TCPSocketRef socket, TransportReceiverTCP* transport );
		Connection( const Connection &other ) = delete;
		Connection( Connection &&other ) noexcept;
		Connection& operator=( const Connection &other ) = delete;
		Connection& operator=( Connection &&other ) noexcept;
		
		~Connection();
		
		asio::ip::tcp::endpoint getRemoteEndpoint() { return mSocket->remote_endpoint(); }
		asio::ip::tcp::endpoint getLocalEndpoint() { return  mSocket->local_endpoint(); }
		
		void read();
		bool readComplete( const asio::error_code &error, size_t bytesTransferred );
		void onRead( const asio::error_code &error, size_t bytesTransferred );
		void close();
		
		TCPSocketRef			mSocket;
		TransportReceiverTCP*	mTransport;
		asio::streambuf			mBuffer;
		std::vector<uint8_t>	mDataBuffer;
	};
	
protected:
	TransportReceiverTCP( DataHandler dataHandler,
						  const asio::ip::tcp::endpoint &localEndpoint,
						  asio::io_service &service );
	TransportReceiverTCP( DataHandler dataHandler, const TCPSocketRef &socket );
	
	void onAccept( TCPSocketRef socket, const asio::error_code &error );
	void onRead( const asio::error_code &error, size_t bytesTransferred );
	bool readComplete( const asio::error_code &error, size_t bytesTransferred );
	
	asio::io_service			&mIoService;
	std::mutex					mDataHandlerMutex, mConnectionMutex;
	asio::ip::tcp::acceptor		mAcceptor;
	std::vector<Connection>		mConnections;
	
	friend class ReceiverTCP;
	friend class Connection;
};

}
