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
	
protected:
	TransportSenderTCP(	WriteHandler handler,
						const asio::ip::tcp::endpoint &localEndpoint,
						const asio::ip::tcp::endpoint &remoteEndpoint,
						asio::io_service &io );
	TransportSenderTCP( WriteHandler writeHandler,
						const TCPSocketRef &socket,
						const asio::ip::tcp::endpoint &remoteEndpoint );
	
	TCPSocketRef			mSocket;
	asio::ip::tcp::endpoint mRemoteEndpoint;
};

class TransportReceiverTCP : public TransportReceiverBase {
public:
	virtual ~TransportReceiverTCP() = default;
	
	void listen() override;
	void close() override;
	
	virtual asio::ip::address getLocalAddress() const;
	
	struct Connection {
		Connection( TCPSocketRef socket, TransportReceiverTCP* transport );
		Connection( const Connection &other ) = default;
		Connection( Connection &&other ) = default;
		Connection& operator=( const Connection &other ) = default;
		Connection& operator=( Connection &&other ) = default;
		
		~Connection() { mTransport = nullptr; }
		
		asio::ip::tcp::endpoint getRemoteEndpoint() { return mSocket->remote_endpoint(); }
		asio::ip::tcp::endpoint getLocalEndpoint() { return  mSocket->local_endpoint(); }
		
		void read();
		void close();
		
		TCPSocketRef			mSocket;
		TransportReceiverTCP*	mTransport;
		asio::streambuf			mBuffer;
	};
	
protected:
	TransportReceiverTCP( DataHandler dataHandler,
						  const asio::ip::udp::endpoint &localEndpoint,
						  asio::io_service &service );
	TransportReceiverTCP( DataHandler dataHandler, const UDPSocketRef &socket );
	
	void onAccept( TCPSocketRef socket, const asio::error_code &error );
	
	asio::io_service			&mIoService;
	std::mutex					mDataHandlerMutex;
	asio::ip::tcp::acceptor		mAcceptor;
	std::vector<Connection>		mConnections;
	uint16_t					mPort;
	
	friend class Connection;
};

}
