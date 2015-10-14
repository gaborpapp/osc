#pragma once

#include "TransportBase.h"

namespace osc {
	
class TransportSenderTcp : public TransportSenderBase {
public:
	virtual ~TransportSenderTcp() = default;
	
	void connect();
	//! Sends \a message to the destination endpoint.
	void send( std::shared_ptr<std::vector<uint8_t>> data ) override;
	void close() override;
	
	//! Returns the local address of the endpoint associated with this socket.
	asio::ip::address getLocalAddress() const override { return mSocket->local_endpoint().address(); }
	//! Returns the remote address of the endpoint associated with this transport.
	asio::ip::address getRemoteAddress() const override { return mRemoteEndpoint.address(); }
	
	
protected:
	TransportSenderTcp(	WriteHandler handler,
						const asio::ip::tcp::endpoint &localEndpoint,
						const asio::ip::tcp::endpoint &remoteEndpoint,
						asio::io_service &io );
	TransportSenderTcp( WriteHandler writeHandler,
						const TcpSocketRef &socket,
						const asio::ip::tcp::endpoint &remoteEndpoint );
	
	void onConnect( const asio::error_code &error );
	
	TcpSocketRef			mSocket;
	asio::ip::tcp::endpoint mRemoteEndpoint;
	
	friend class SenderTcp;
};
	
using iterator = asio::buffers_iterator<asio::streambuf::const_buffers_type>;

class TransportReceiverTcp : public TransportReceiverBase {
public:
	virtual ~TransportReceiverTcp() = default;
	
	void listen() override;
	void close() override;
	
	asio::ip::address getLocalAddress() const override { return mAcceptor.local_endpoint().address(); }
	
protected:
	TransportReceiverTcp( DataHandler dataHandler,
						  const asio::ip::tcp::endpoint &localEndpoint,
						  asio::io_service &service );
	TransportReceiverTcp( DataHandler dataHandler, const TcpSocketRef &socket );
	
	void onAccept( TcpSocketRef socket, const asio::error_code &error );
	void onRead( const asio::error_code &error, size_t bytesTransferred );
	bool readComplete( const asio::error_code &error, size_t bytesTransferred );
	
	struct Connection {
		Connection( TcpSocketRef socket, TransportReceiverTcp* transport );
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
		TransportReceiverTcp*	mTransport;
		asio::streambuf			mBuffer;
		std::vector<uint8_t>	mDataBuffer;
	};
	
	asio::io_service			&mIoService;
	std::mutex					mDataHandlerMutex, mConnectionMutex;
	asio::ip::tcp::acceptor		mAcceptor;
	std::vector<Connection>		mConnections;
	
	friend class ReceiverTcp;
	friend class Connection;
};

}
