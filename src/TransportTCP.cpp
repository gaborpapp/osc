//
//  TransportTCP.cpp
//  Test
//
//  Created by ryan bartley on 9/19/15.
//
//

#include "TransportTCP.h"

using namespace asio;
using namespace asio::ip;

namespace osc {
	
void TransportReceiverTCP::listen()
{
	TCPSocketRef socket = TCPSocketRef( new tcp::socket( mIoService ) );
	mAcceptor.async_accept( *socket, [](){});
}

}