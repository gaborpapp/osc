//
//  Sender.h
//  Test
//
//  Created by ryan bartley on 9/4/15.
//
//

#pragma once

#include "Message.h"
#include "Bundle.h"
#include "TransportUdpPolicy.h"

namespace osc {

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
	
using SenderUdpTemp = Sender<TransportSenderUdp>;
	
}