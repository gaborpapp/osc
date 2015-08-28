//
//  Argument.cpp
//  Test
//
//  Created by ryan bartley on 8/28/15.
//
//

#include "Argument.h"

namespace osc {

Argument::Argument( const Argument &arg )
: mType( arg.mType ), mSize( arg.mSize ), mData( new uint8_t[mSize] )
{
	memcpy( mData.get(), mData.get(), mSize );
}

Argument::Argument( Argument &&arg )
: mType( arg.mType ), mSize( arg.mSize ), mData( std::move( arg.mData ) )
{
	mType = ArgType::NONE;
	mSize = 0;
}

Argument& Argument::operator=( const Argument &arg )
{
	mType = arg.mType;
	mSize = arg.mSize;
	mData.reset( new uint8_t[mSize] );
	memcpy( mData.get(), arg.mData.get(), mSize );
	return *this;
}

Argument& Argument::operator=( Argument &&arg )
{
	mType = arg.mType;
	mSize = arg.mSize;
	mData = std::move( arg.mData );
	arg.mType = ArgType::NONE;
	arg.mSize = 0;
	return *this;
}
	
float Argument::getArgAsFloat() const
{
	if( mType == ArgType::FLOAT )
		return *(reinterpret_cast<float*>(mData.get()));
	else
		return 0.0f;
}
	
int32_t Argument::getArgAsInt() const
{
	if( mType == ArgType::INTEGER )
		return *(reinterpret_cast<int32_t*>(mData.get()));
	else
		return 0;
}
	
std::string Argument::getArgAsString() const
{
	std::string ret;
	if( mType == ArgType::STRING ) {
		ret.resize( mSize );
		memcpy( &ret[0], mData.get(), mSize );
		ret[mSize] = '\0';
	}
	return ret;
}
	
}