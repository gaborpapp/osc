// Copyright (c) 2011 Toshiro Yamada
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/// @file tnyosc.hpp
/// @brief tnyosc main (and only) header file
/// @author Toshiro Yamada
///
/// tnyosc is a header-only Open Sound Control library written in C++ for
/// creating OSC-compliant messages. tnyosc supports Open Sound Control 1.0 and
/// 1.1 types and other nonstandard types, and bundles. Note that tnyosc does not
/// include code to actually send or receive OSC messages.

#pragma once

#include "utils.h"

namespace osc {
	
/// This class represents an Open Sound Control message. It supports Open Sound
/// Control 1.0 and 1.1 specifications and extra non-standard arguments listed
/// in http://opensoundcontrol.org/spec-1_0.
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
	friend bool decodeMessage( uint8_t *data, uint32_t size, std::vector<Message> &messages, uint64_t timetag );
};

//! Convenient stream operator for Message
std::ostream& operator<<( std::ostream &os, const Message &rhs );
	
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
	// TODO: reimplement this so that it takes the shared buffer and get rid of bytebuffer.
	void append( const Message &message ) { append_data( message.byteArray() ); }
	void append( const Bundle &bundle ) { append_data( bundle.byteBuffer() ); }
	
	/// Sets timestamp of the bundle.
	void setTimetag( uint64_t ntp_time );
	
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

//! SenderBase represents an OSC Sender(client in OSC terms) and implements a unified
//! interface without implementing any of the networking layer.
class SenderBase {
public:
	// TODO: Figure out error handling
	using ErrorHandler = std::function<void( const std::string & /*oscAddress*/,
										 const std::string & /*socketAddress*/,
										 const std::string & /*error*/)>;
	
	//! Sends \a message to the destination endpoint.
	void send( const Message &message ) { sendImpl( message.getSharedBuffer() ); }
	//! Sends \a bundle to the destination endpoint.
	void send( const Bundle &bundle ) { sendImpl( bundle.getSharedBuffer() ); }
	//! Closes the underlying connection to the socket.
	void close() { closeImpl(); }
	
protected:
	SenderBase() = default;
	virtual ~SenderBase() = default;
	SenderBase( const SenderBase &other ) = delete;
	SenderBase& operator=( const SenderBase &other ) = delete;
	SenderBase( SenderBase &&other ) = default;
	SenderBase& operator=( SenderBase &&other ) = default;
	
	//! Abstract send function implemented by the network layer.
	virtual void sendImpl( const ByteBufferRef &byteBuffer ) = 0;
	//! Abstract close function implemented by the network layer
	virtual void closeImpl() = 0;
};
	
class SenderUdp : public SenderBase {
public:
	using protocol = asio::ip::udp;
	SenderUdp( uint16_t localPort,
			   const std::string &destinationHost,
			   uint16_t destinationPort,
			   const protocol &protocol = protocol::v4(),
			   asio::io_service &service = ci::app::App::get()->io_service() );
	SenderUdp( uint16_t localPort,
			   const protocol::endpoint &destination,
			   const protocol &protocol = protocol::v4(),
			   asio::io_service &service = ci::app::App::get()->io_service() );
	SenderUdp( const UdpSocketRef &socket, const protocol::endpoint &destination );
	
	virtual ~SenderUdp() = default;
	
	//! Returns the local address of the endpoint associated with this socket.
	protocol::endpoint getLocalAddress() const { return mSocket->local_endpoint(); }
	//! Returns the remote address of the endpoint associated with this transport.
	const protocol::endpoint& getRemoteAddress() const { return mRemoteEndpoint; }
	
protected:
	//! Sends the byte buffer /a data to the remote endpoint using the udp socket, asynchronously.
	void sendImpl( const ByteBufferRef &data ) override;
	//! Closes the underlying udp socket.
	void closeImpl() override { mSocket->close(); }
	
	UdpSocketRef			mSocket;
	asio::ip::udp::endpoint mRemoteEndpoint;
	
public:
	SenderUdp( const SenderUdp &other ) = delete;
	SenderUdp& operator=( const SenderUdp &other ) = delete;
	SenderUdp( SenderUdp &&other ) = default;
	SenderUdp& operator=( SenderUdp &&other ) = default;
};

class SenderTcp : public SenderBase {
public:
	using protocol = asio::ip::tcp;
	SenderTcp( uint16_t localPort,
			   const std::string &destinationHost,
			   uint16_t destinationPort,
			   const protocol &protocol = protocol::v4(),
			   asio::io_service &service = ci::app::App::get()->io_service() );
	SenderTcp( uint16_t localPort,
			   const protocol::endpoint &destination,
			   const protocol &protocol = protocol::v4(),
			   asio::io_service &service = ci::app::App::get()->io_service() );
	SenderTcp( const TcpSocketRef &socket, const asio::ip::tcp::endpoint &destination );
	
	virtual ~SenderTcp() = default;
	
	//! Connects to the remote endpoint using the underlying socket. Has to be called before attempting to send anything.
	void connect();
	
	//! Returns the local address of the endpoint associated with this socket.
	protocol::endpoint getLocalEndpoint() const { return mSocket->local_endpoint(); }
	//! Returns the remote address of the endpoint associated with this transport.
	const protocol::endpoint& getRemoteEndpoint() const { return mRemoteEndpoint; }
	
protected:
	//! Sends the byte buffer /a data to the remote endpoint using the tcp socket, asynchronously.
	void sendImpl( const ByteBufferRef &data ) override;
	//! Closes the underlying tcp socket.
	void closeImpl() override { mSocket->close(); }
	
	TcpSocketRef			mSocket;
	asio::ip::tcp::endpoint mRemoteEndpoint;
	
public:
	SenderTcp( const SenderTcp &other ) = delete;
	SenderTcp& operator=( const SenderTcp &other ) = delete;
	SenderTcp( SenderTcp &&other ) = default;
	SenderTcp& operator=( SenderTcp &&other ) = default;
};
	
namespace decode {
	//! Free function that decodes a complete OSC Packet into it's individual parts.
	bool decodeData( uint8_t *data, uint32_t size, std::vector<Message> &messages, uint64_t timetag = 0 );
	//! Free function that decodes an individual message.
	bool decodeMessage( uint8_t *data, uint32_t size, std::vector<Message> &messages, uint64_t timetag = 0 );
	//! Free function used to match the addresses of messages based on the OSC spec.
	bool patternMatch( const std::string &lhs, const std::string &rhs );
}

//! ReceiverBase represents an OSC Receiver(server in OSC terms) and implements a unified
//! interface without implementing any of the networking layer.
class ReceiverBase {
public:
	//! TODO: Figure out what to do with Error Handler
	using ErrorHandler = std::function<void( const std::string & )>;
	using Listener = std::function<void( const Message &message )>;
	using Listeners = std::vector<std::pair<std::string, Listener>>;
	
	//! Commits the socket to asynchronously listen and begin to receive from outside connections. Uses receiveHandler, below, as the completion handler.
	void		listen() { listenImpl(); }
	void		close() { closeImpl(); }
	
	//! Sets a callback, \a listener, to be called when receiving a message with \a address. If a listener exists for this address, \a listener will replace it.
	void		setListener( const std::string &address, Listener listener );
	//! Removes the listener associated with \a address.
	void		removeListener( const std::string &address );
	
protected:
	ReceiverBase() = default;
	virtual ~ReceiverBase() = default;
	ReceiverBase( const ReceiverBase &other ) = delete;
	ReceiverBase& operator=( const ReceiverBase &other ) = delete;
	ReceiverBase( ReceiverBase &&other ) = default;
	ReceiverBase& operator=( ReceiverBase &&other ) = default;
	
	//! Handles the async receive completion operations. Errors are handled by the specific network layer. Therefore, when this is called we know that we have a complete message.
	void receiveHandler( std::size_t bytesTransferred, asio::streambuf &stream );
	void dispatchMethods( uint8_t *data, uint32_t size );
	
	//! Implemented
	virtual void listenImpl() = 0;
	virtual void closeImpl() = 0;
	
	Listeners								mListeners;
	std::mutex								mListenerMutex;
};
	
//! ReceiverUdp represents an OSC Receiver(server in OSC terms) and implements the udp transport
//!	networking layer.
class ReceiverUdp : public ReceiverBase {
public:
	using protocol = asio::ip::udp;
	ReceiverUdp( uint16_t port,
				 const protocol &protocol = protocol::v4(),
				 asio::io_service &io = ci::app::App::get()->io_service()  );
	ReceiverUdp( const protocol::endpoint &localEndpoint,
				 asio::io_service &io = ci::app::App::get()->io_service() );
	ReceiverUdp( UdpSocketRef socket );
	
	virtual ~ReceiverUdp() = default;
	
	// TODO: Check to see that this is needed, see if we can't auto accept a size of datagram.
	void setAmountToReceive( uint32_t amountToReceive ) { mAmountToReceive = amountToReceive; }
	//! Returns the local udp::endpoint of the underlying socket.
	asio::ip::udp::endpoint getLocalEndpoint() { return mSocket->local_endpoint(); }
	
protected:
	void listenImpl() override;
	void closeImpl() override { mSocket->close(); }
	
	UdpSocketRef			mSocket;
	asio::streambuf			mBuffer;
	uint32_t				mAmountToReceive;
	
public:
	ReceiverUdp( const ReceiverUdp &other ) = delete;
	ReceiverUdp& operator=( const ReceiverUdp &other ) = delete;
	ReceiverUdp( ReceiverUdp &&other ) = default;
	ReceiverUdp& operator=( ReceiverUdp &&other ) = default;
};

//! ReceiverTcp represents an OSC Receiver(server in OSC terms) and implements the tcp transport
//!	networking layer.
class ReceiverTcp : public ReceiverBase {
public:
	using protocol = asio::ip::tcp;
	ReceiverTcp( uint16_t port,
			 const protocol &protocol = protocol::v4(),
			 asio::io_service &service = ci::app::App::get()->io_service()  );
	ReceiverTcp( const protocol::endpoint &localEndpoint,
			 asio::io_service &service = ci::app::App::get()->io_service() );
	// TODO: Decide on maybe allowing a constructor for an already constructed acceptor
	virtual ~ReceiverTcp() = default;
	
protected:
	struct Connection {
		Connection( TcpSocketRef socket, ReceiverTcp* transport );
		
		~Connection();
		
		asio::ip::tcp::endpoint getRemoteEndpoint() { return mSocket->remote_endpoint(); }
		asio::ip::tcp::endpoint getLocalEndpoint() { return  mSocket->local_endpoint(); }
		
		//! Implements the read on the underlying socket.
		void read();
		//! Simple alias for asio buffer iterator type.
		using iterator = asio::buffers_iterator<asio::streambuf::const_buffers_type>;
		//! Static method which is used to read the stream as it's coming in and notate each packet.
		static std::pair<iterator, bool> readMatchCondition( iterator begin, iterator end );
		//! Implements the close of this socket
		void close() { mSocket->close(); }
		
		TcpSocketRef			mSocket;
		ReceiverTcp*			mTransport;
		asio::streambuf			mBuffer;
		std::vector<uint8_t>	mDataBuffer;
		
		
		Connection( const Connection &other ) = delete;
		Connection& operator=( const Connection &other ) = delete;
		Connection( Connection &&other ) noexcept;
		Connection& operator=( Connection &&other ) noexcept;
	};
	
	void listenImpl() override;
	void closeImpl() override;
	
	asio::io_service			&mIoService;
	asio::ip::tcp::acceptor		mAcceptor;
	
	std::mutex					mDataHandlerMutex, mConnectionMutex;
	std::vector<Connection>		mConnections;

	friend class Connection;
public:
	ReceiverTcp( const ReceiverTcp &other ) = delete;
	ReceiverTcp& operator=( const ReceiverTcp &other ) = delete;
	ReceiverTcp( ReceiverTcp &&other ) = default;
	ReceiverTcp& operator=( ReceiverTcp &&other ) = default;
};
	
}