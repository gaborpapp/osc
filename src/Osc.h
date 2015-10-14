//
//  Sender.h
//  Test
//
//  Created by ryan bartley on 9/4/15.
//
//

#pragma once

#include "Utils.h"

namespace osc {
	
//! This class represents an Open Sound Control message. It supports Open Sound
//! Control 1.0 and 1.1 specifications and extra non-standard arguments listed
//! in http://opensoundcontrol.org/spec-1_0.
class Message {
public:
	
	//! Create an OSC message.
	Message() = default;
	explicit Message( const std::string& address );
	Message( const Message & ) = delete;
	Message& operator=( const Message & ) = delete;
	Message( Message && ) = default;
	Message& operator=( Message && ) = default;
	~Message() = default;
	
	// Functions for appending OSC 1.0 types
	
	//! Appends an int32 to the back of the message.
	void append( int32_t v );
	//! Appends a float to the back of the message.
	void append( float v );
	//! Appends a string to the back of the message.
	void append( const std::string& v );
	//! Appends a string to the back of the message.
	void append( const char* v, size_t len );
	void append( const char* v ) = delete;
	//! Appends an osc blob to the back of the message.
	void appendBlob( void* blob, uint32_t size );
	//! Appends an osc blob to the back of the message.
	void appendBlob( const ci::Buffer &buffer );
	
	// Functions for appending OSC 1.1 types
	//! Appends an OSC-timetag (NTP format) to the back of the message.
	void appendTimeTag( uint64_t v );
	//! Appends the current UTP timestamp to the back of the message.
	void appendCurrentTime() { appendTimeTag( time::get_current_ntp_time() ); }
	//! Appends a 'T'(True) or 'F'(False) to the back of the message.
	void append( bool v );
	//! Appends a Null (or nil) to the back of the message.
	void appendNull() { mIsCached = false; mDataViews.emplace_back( ArgType::NULL_T, -1, 0 ); }
	//! Appends an Impulse (or Infinitum) to the back of the message
	void appendImpulse() { mIsCached = false; mDataViews.emplace_back( ArgType::INFINITUM, -1, 0 ); }
	
	// Functions for appending nonstandard types
	//! Appends an int64_t to the back of the message.
	void append( int64_t v );
	//! Appends a float64 (or double) to the back of the message.
	void append( double v );
	//! Appends an ascii character to the back of the message.
	void append( char v );
	//! Appends a midi value to the back of the message.
	void appendMidi( uint8_t port, uint8_t status, uint8_t data1, uint8_t data2 );
	// TODO: figure out if array is useful.
	// void appendArray( void* array, size_t size );
	
	//! Returns the int32_t located at \a index.
	int32_t		getInt( uint32_t index ) const;
	//! Returns the float located at \a index.
	float		getFloat( uint32_t index ) const;
	//! Returns the string located at \a index.
	std::string getString( uint32_t index ) const;
	//! Returns the time_tag located at \a index.
	int64_t		getTime( uint32_t index ) const;
	//! Returns the time_tag located at \a index.
	int64_t		getInt64( uint32_t index ) const;
	//! Returns the double located at \a index.
	double		getDouble( uint32_t index ) const;
	//! Returns the bool located at \a index.
	bool		getBool( uint32_t index ) const;
	//! Returns the char located at \a index.
	char		getChar( uint32_t index ) const;
	
	void		getMidi( uint32_t index, uint8_t *port, uint8_t *status, uint8_t *data1, uint8_t *data2 ) const;
	//! Returns the blob, as a ci::Buffer, located at \a index.
	ci::Buffer	getBlob( uint32_t index ) const;
	
	//! Returns the argument type located at \a index.
	ArgType		getArgType( uint32_t index ) const;
	
	bool compareTypes( const std::string &types );
	
	/// Sets the OSC address of this message.
	/// @param[in] address The new OSC address.
	void setAddress( const std::string& address );
	
	/// Returns the OSC address of this message.
	const std::string& getAddress() const { return mAddress; }
	
	/// Returns a complete byte array of this OSC message as a ByteArray type.
	/// The byte array is constructed lazily and is cached until the cache is
	/// obsolete. Call to |data| and |size| perform the same caching.
	///
	/// @return The OSC message as a ByteArray.
	/// @see data
	/// @see size
	const ByteBuffer& byteArray() const;
	
	/// Returns the size of this OSC message.
	///
	/// @return Size of the OSC message in bytes.
	/// @see byte_array
	/// @see data
	size_t size() const;
	
	/// Clears the message.
	void clear();
	
	class Argument {
	public:
		Argument();
		Argument( ArgType type, int32_t offset, uint32_t size, bool needsSwap = false );
		Argument( const Argument &arg ) = default;
		Argument( Argument &&arg ) = default;
		Argument& operator=( const Argument &arg ) = default;
		Argument& operator=( Argument &&arg ) = default;
		
		~Argument() = default;
		
		ArgType		getArgType() const { return mType; }
		uint32_t	getArgSize() const { return mSize; }
		int32_t		getOffset() const { return mOffset; }
		bool		needsEndianSwapForTransmit() const { return mNeedsEndianSwapForTransmit; }
		void		outputValueToStream( uint8_t *buffer, std::ostream &ostream ) const;
		
		template<typename T>
		bool convertible() const;
		
		static char translateArgTypeToChar( ArgType type );
		static ArgType translateCharToArgType( char type );
		
		void swapEndianForTransmit( uint8_t* buffer ) const;
		
	private:
		
		ArgType			mType;
		int32_t			mOffset;
		uint32_t		mSize;
		bool			mNeedsEndianSwapForTransmit;
		
		friend class Message;
	};
	
private:
	static uint8_t getTrailingZeros( size_t bufferSize ) { return 4 - ( bufferSize % 4 ); }
	size_t getCurrentOffset() { return mDataBuffer.size(); }
	
	template<typename T>
	const Argument& getDataView( uint32_t index ) const;
	
	ByteBufferRef getSharedBuffer() const;
	
	std::string				mAddress;
	ByteBuffer				mDataBuffer;
	std::vector<Argument>	mDataViews;
	mutable bool			mIsCached;
	mutable ByteBufferRef	mCache;
	
	/// Create the OSC message and store it in cache.
	void createCache() const;
	bool bufferCache( uint8_t *data, size_t size );
	
	friend class Bundle;
	friend class SenderBase;
	friend class ReceiverBase;
	friend std::ostream& operator<<( std::ostream &os, const Message &rhs );
};
	
//! This class represents an Open Sound Control bundle message. A bundle can
//! contain any number of Message and Bundle.
class Bundle {
public:
	
	//! Creates a OSC bundle with timestamp set to immediate. Call set_timetag to
	//! set a custom timestamp.
	Bundle();
	~Bundle() = default;
	
	//! Appends an OSC message to this bundle. The message is immediately copied
	//! into this bundle and any changes to the message after the call to this
	//! function does not affect this bundle.
	void append( const Message &message ) { append_data( message.byteArray() ); }
	void append( const Bundle &bundle ) { append_data( bundle.byteBuffer() ); }
	
	/// Sets timestamp of the bundle.
	void set_timetag( uint64_t ntp_time );
	
	//! Returns a complete byte array of this OSC bundle type.
	const ByteBuffer& byteBuffer() const { return *getSharedBuffer(); }
	
	//! Returns the size of this OSC bundle.
	size_t size() const { return mDataBuffer->size(); }
	
	//! Clears the bundle.
	void clear() { mDataBuffer->clear(); }
	
private:
	ByteBufferRef mDataBuffer;
	
	/// Returns a pointer to the byte array of this OSC bundle. This call is
	/// convenient for actually sending this OSC bundle.
	ByteBufferRef getSharedBuffer() const;
	
	void append_data( const ByteBuffer& data );
	
	friend class SenderBase;
};
	
namespace detail { namespace decode {
	bool decodeData( uint8_t *data, uint32_t size, std::vector<Message> &messages, uint64_t timetag = 0 );
	bool decodeMessage( uint8_t *data, uint32_t size, std::vector<Message> &messages, uint64_t timetag = 0 );
	bool patternMatch( const std::string &lhs, const std::string &rhs );
}}

template<typename TransportSender>
class Sender : public TransportSender {
public:
	using protocol = typename TransportSender::protocol;
	using endpoint = typename protocol::endpoint;
	using SocketRef = std::shared_ptr<typename protocol::socket>;
	
	Sender( uint16_t localPort,
			const std::string &destinationHost,
			uint16_t destinationPort,
			const protocol &protocol = protocol::v4(),
			asio::io_service &service = ci::app::App::get()->io_service() )
	: TransportSender( endpoint( protocol, localPort ),
					   endpoint( asio::ip::address::from_string( destinationHost ), destinationPort ),
					   service ) {}
	
	Sender( uint16_t localPort,
			const endpoint &destination,
			const protocol &protocol = typename protocol::v4(),
		   asio::io_service &service = ci::app::App::get()->io_service() )
	: TransportSender( endpoint( protocol, localPort ), destination, service ) {}
	
	Sender( const SocketRef &socket, const endpoint &destination )
	: TransportSender( socket, destination ) {}
	
	Sender( const Sender &other ) = delete;
	Sender& operator=( const Sender &other ) = delete;
	Sender( Sender &&other ) = default;
	Sender& operator=( Sender &&other ) = default;
	~Sender() = default;
	
	//! Sends \a message to the destination endpoint.
	void send( const Message &message ) { this->sendImpl( message.getSharedBuffer() ); }
	void send( const Bundle &bundle ) { this->sendImpl( bundle.getSharedBuffer() ); }
	
	void close() { this->closeImpl(); }
	
};

template<typename TransportReceiver>
class Receiver : public TransportReceiver {
public:
	using Listener = std::function<void( const Message &message )>;
	using Listeners = std::vector<std::pair<std::string, Listener>>;
	
	using protocol = typename TransportReceiver::protocol;
	using endpoint = typename protocol::endpoint;
	using SocketRef = std::shared_ptr<typename protocol::socket>;
	
	Receiver( uint16_t port,
			  const protocol &protocol = protocol::v4(),
			  asio::io_service &service = ci::app::App::get()->io_service()  )
	: TransportReceiver( endpoint( protocol, port ), service ) { this->setMessageCompleteHandler( std::bind( &Receiver::messageCompleteHandler, this, std::placeholders::_1, std::placeholders::_2 ) ); }
	Receiver( const endpoint &localEndpoint,
			  asio::io_service &service = ci::app::App::get()->io_service() )
	: TransportReceiver( localEndpoint, service ) { this->setMessageCompleteHandler( std::bind( &Receiver::messageCompleteHandler, this, std::placeholders::_1, std::placeholders::_2 ) ); }
	Receiver( UdpSocketRef socket ) : TransportReceiver( socket ) { this->setMessageCompleteHandler( std::bind( &Receiver::messageCompleteHandler, this, std::placeholders::_1, std::placeholders::_2 ) ); }
	
	Receiver( const Receiver &other ) = delete;
	Receiver& operator=( const Receiver &other ) = delete;
	Receiver( Receiver &&other ) = default;
	Receiver& operator=( Receiver &&other ) = default;
	
	virtual ~Receiver() = default;
	
	//! Commits the socket to asynchronously receive into the internal buffer. Uses receiveHandler, below, as the completion handler.
	void		listen() { this->listenImpl(); }
	void		close() { this->closeImpl(); }
	
	//! Sets a callback, \a listener, to be called when receiving a message with \a address. If a listener exists for this address, \a listener will replace it.
	void		setListener( const std::string &address, Listener listener );
	//! Removes the listener associated with \a address.
	void		removeListener( const std::string &address );
	
protected:
	//! Handles the async receive completion operations
	void messageCompleteHandler( std::size_t bytesTransferred, asio::streambuf &streamBuf );
	void dispatchMethods( uint8_t *data, uint32_t size );
	
	Listeners								mListeners;
	std::mutex								mListenerMutex;
};
	
namespace detail {
	
class TransportSenderUdp {
public:
	using protocol = asio::ip::udp;
	using ErrorHandler = std::function<void( const std::string & /*oscAddress*/,
											const std::string & /*socketAddress*/,
											const std::string & /*error*/)>;
	
	//! Returns the local address of the endpoint associated with this socket.
	const asio::ip::udp::endpoint& getRemoteEndpoint() const { return mRemoteEndpoint; }
	//! Returns the remote address of the endpoint associated with this transport.
	asio::ip::udp::endpoint getLocalEndpoint() const { return mSocket->local_endpoint(); }
	
	void setErrorHandler( ErrorHandler errorHandler ) { mErrorHandler = errorHandler; }
	void setWriteHandler( WriteHandler writeHandler ) { mWriteHandler = writeHandler; }
	
protected:
	TransportSenderUdp( const protocol::endpoint &localEndpoint,
				    const protocol::endpoint &remoteEndpoint,
				    asio::io_service &service );
	TransportSenderUdp( const UdpSocketRef &socket,
				    const protocol::endpoint &remoteEndpoint );
	
	~TransportSenderUdp() = default;
	
	//! Sends the data array /a data to the remote endpoint using the Udp socket, asynchronously.
	void sendImpl( std::shared_ptr<std::vector<uint8_t>> data );
	//!
	void closeImpl();
	
	UdpSocketRef			mSocket;
	asio::ip::udp::endpoint mRemoteEndpoint;
	
	WriteHandler			mWriteHandler;
	ErrorHandler			mErrorHandler;
	
	friend class SenderUdp;
};

class TransportReceiverUdp {
public:
	using protocol = asio::ip::udp;
	using ErrorHandler = std::function<void( const std::string & )>;
	
	//! Returns the local endpoint of the endpoint associated with this socket.
	asio::ip::udp::endpoint getLocalAddress() const { return mSocket->local_endpoint(); }
	//! Adds a handler to be called if there are errors with the asynchronous receive.
	void		setErrorHandler( ErrorHandler errorHandler ) { mErrorHandler = errorHandler; }
	
protected:
	TransportReceiverUdp( const protocol::endpoint &localEndpoint, asio::io_service &service );
	TransportReceiverUdp( const UdpSocketRef &socket );
	~TransportReceiverUdp() = default;
	
	//! Implements listening on the socket
	void listenImpl();
	//! Implements close of the socket
	void closeImpl();
	//!
	void setMessageCompleteHandler( MessageCompleteHandler messageCompleteHandler ) { mMessageCompleteHandler = messageCompleteHandler; }
	
	UdpSocketRef			mSocket;
	asio::streambuf			mBuffer;
	
	MessageCompleteHandler	mMessageCompleteHandler;
	ErrorHandler			mErrorHandler;
	
	friend class ReceiverUdp;
};
	
class TransportSenderTcp {
public:
	
	void connect();
	//! Returns the local address of the endpoint associated with this socket.
	const asio::ip::tcp::endpoint& getRemoteEndpoint() const { return mRemoteEndpoint; }
	//! Returns the remote address of the endpoint associated with this transport.
	asio::ip::tcp::endpoint getLocalEndpoint() const { return mSocket->local_endpoint(); }
	
//	void setErrorHandler( ErrorHandler errorHandler ) { mErrorHandler = errorHandler; }
	void setWriteHandler( WriteHandler writeHandler ) { mWriteHandler = writeHandler; }
	
	//! Returns the local address of the endpoint associated with this socket.
	asio::ip::address getLocalAddress() const  { return mSocket->local_endpoint().address(); }
	//! Returns the remote address of the endpoint associated with this transport.
	asio::ip::address getRemoteAddress() const { return mRemoteEndpoint.address(); }
	
	
protected:
	TransportSenderTcp(	const asio::ip::tcp::endpoint &localEndpoint,
					   const asio::ip::tcp::endpoint &remoteEndpoint,
					   asio::io_service &io );
	TransportSenderTcp( const TcpSocketRef &socket,
					   const asio::ip::tcp::endpoint &remoteEndpoint );
	~TransportSenderTcp() = default;
	
	//! Sends \a message to the destination endpoint.
	void sendImpl( std::shared_ptr<std::vector<uint8_t>> data );
	void closeImpl();
	
	void onConnect( const asio::error_code &error );
	
	TcpSocketRef			mSocket;
	asio::ip::tcp::endpoint mRemoteEndpoint;
	
	WriteHandler			mWriteHandler;

	friend class SenderTcp;
};

using iterator = asio::buffers_iterator<asio::streambuf::const_buffers_type>;

class TransportReceiverTcp {
public:
	virtual ~TransportReceiverTcp() = default;

	asio::ip::address getLocalAddress() const { return mAcceptor.local_endpoint().address(); }
	
protected:
	TransportReceiverTcp( DataHandler dataHandler,
					  const asio::ip::tcp::endpoint &localEndpoint,
					  asio::io_service &service );
	TransportReceiverTcp( DataHandler dataHandler, const TcpSocketRef &socket );
	
	void listenImpl();
	void closeImpl();
	
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
	
template<typename TransportReceiver>
void Receiver<TransportReceiver>::setListener( const std::string &address, Listener listener )
{
	std::lock_guard<std::mutex> lock( mListenerMutex );
	auto foundListener = std::find_if( mListeners.begin(), mListeners.end(),
									  [address]( const std::pair<std::string, Listener> &listener ) {
										  return address == listener.first;
									  });
	if( foundListener != mListeners.end() ) {
		foundListener->second = listener;
	}
	else {
		mListeners.push_back( { address, listener } );
	}
}

template<typename TransportReceiver>
void Receiver<TransportReceiver>::removeListener( const std::string &address )
{
	std::lock_guard<std::mutex> lock( mListenerMutex );
	auto foundListener = std::find_if( mListeners.begin(), mListeners.end(),
									  [address]( const std::pair<std::string, Listener> &listener ) {
										  return address == listener.first;
									  });
	if( foundListener != mListeners.end() ) {
		mListeners.erase( foundListener );
	}
}
	
template<typename TransportReceiver>
void Receiver<TransportReceiver>::messageCompleteHandler( std::size_t bytesTransferred, asio::streambuf &streamBuf )
{
	auto data = new std::unique_ptr<uint8_t[]>( new uint8_t[bytesTransferred + 1] );
	data[ bytesTransferred ] = 0;
	std::istream stream( &streamBuf );
	stream.read( (char*)data, bytesTransferred );
	dispatchMethods( (uint8_t*)(data + 4), bytesTransferred );
}

template<typename TransportReceiver>
void Receiver<TransportReceiver>::dispatchMethods( uint8_t *data, uint32_t size )
{
	using namespace detail::decode;
	std::vector<Message> messages;
	
	if( ! decodeData( data, size, messages ) ) return;
	
	std::lock_guard<std::mutex> lock( mListenerMutex );
	// iterate through all the messages and find matches with registered methods
	for( auto & message : messages ) {
		bool dispatchedOnce = false;
		for( auto & listener : mListeners ) {
			if( patternMatch( message.getAddress(), listener.first ) ) {
				listener.second( message );
				dispatchedOnce = true;
			}
		}
		if( ! dispatchedOnce ) {
			
		}
	}
}
	
using SenderUdpTemp = Sender<detail::TransportSenderUdp>;
using ReceiverUdpTemp = Receiver<detail::TransportReceiverUdp>;
	
}