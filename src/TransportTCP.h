#pragma once

#include "TransportBase.h"

namespace osc {
	
class TransportSenderTCP : public TransportSenderBase {
public:
	virtual ~TransportSenderTCP() = default;
	
	void connect();
	//! Sends \a message to the destination endpoint.
	void send( std::shared_ptr<std::vector<uint8_t>> data ) override;
	void close() override;
	
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
	
	void onConnect( const asio::error_code &error );
	
	TCPSocketRef			mSocket;
	asio::ip::tcp::endpoint mRemoteEndpoint;
	
	friend class SenderTCP;
};
	
using iterator = asio::buffers_iterator<asio::streambuf::const_buffers_type>;

class TransportReceiverTCP : public TransportReceiverBase {
public:
	virtual ~TransportReceiverTCP() = default;
	
	void listen() override;
	void close() override;
	
	asio::ip::address getLocalAddress() const override { return mAcceptor.local_endpoint().address(); }
	
protected:
	TransportReceiverTCP( DataHandler dataHandler,
						  const asio::ip::tcp::endpoint &localEndpoint,
						  asio::io_service &service );
	TransportReceiverTCP( DataHandler dataHandler, const TCPSocketRef &socket );
	
	void onAccept( TCPSocketRef socket, const asio::error_code &error );
	void onRead( const asio::error_code &error, size_t bytesTransferred );
	bool readComplete( const asio::error_code &error, size_t bytesTransferred );
	
	struct Connection {
		Connection( TCPSocketRef socket, TransportReceiverTCP* transport );
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
		
		TCPSocketRef			mSocket;
		TransportReceiverTCP*	mTransport;
		asio::streambuf			mBuffer;
		std::vector<uint8_t>	mDataBuffer;
	};
	
	asio::io_service			&mIoService;
	std::mutex					mDataHandlerMutex, mConnectionMutex;
	asio::ip::tcp::acceptor		mAcceptor;
	std::vector<Connection>		mConnections;
	
	friend class ReceiverTCP;
	friend class Connection;
};

}
