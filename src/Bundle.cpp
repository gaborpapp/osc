//
//  Bundle.c
//  Test
//
//  Created by ryan bartley on 10/2/15.
//
//

#include "Bundle.h"

namespace osc {
	
Bundle::Bundle()
: mDataBuffer( new std::vector<uint8_t>( 20 ) )
{
	static std::string id = "#bundle";
	std::copy( id.begin(), id.end(), mDataBuffer->begin() + 4 );
	(*mDataBuffer)[19] = 1;
}
	
void Bundle::set_timetag( uint64_t ntp_time )
{
	uint64_t a = htonll( ntp_time );
	ByteArray<8> b;
	memcpy( b.data(), reinterpret_cast<uint8_t*>( a ), 8 );
	mDataBuffer->insert( mDataBuffer->begin() + 12, b.begin(), b.end() );
}
	
void Bundle::append_data( const ByteBuffer& data )
{
	int32_t a = htonl( data.size() );
	ByteBuffer b( 4 + data.size() );
	memcpy( b.data(), reinterpret_cast<uint8_t*>(a), 4 );
	std::copy( data.begin(), data.end(), b.begin() + 4 );
	mDataBuffer->insert( mDataBuffer->end(), b.begin(), b.end() );
}
	
ByteBufferRef Bundle::getSharedBuffer() const
{
	int32_t a = htonl( size() );
	memcpy( mDataBuffer->data(), reinterpret_cast<uint8_t*>( a ), 4 );
	return mDataBuffer;
}
	
}