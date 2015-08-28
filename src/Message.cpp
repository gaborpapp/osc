//
//  Message.cpp
//  Test
//
//  Created by ryan bartley on 8/28/15.
//
//

#include "Message.h"

namespace osc {
	
void Message::addFloat( float arg )
{
	mArguments.emplace_back( arg );
}

void Message::addInt( int32_t arg )
{
	mArguments.emplace_back( arg );
}

void Message::addString( const std::string &arg )
{
	mArguments.emplace_back( arg );
}
	
float Message::getFloat( uint32_t index ) const
{
	return mArguments[index].getArgAsFloat();
}

int32_t	Message::getInt( uint32_t index ) const
{
	return mArguments[index].getArgAsInt();
}

std::string	Message::getString( uint32_t index ) const
{
	return mArguments[index].getArgAsString();
}

const Argument& Message::getArgument( uint32_t index ) const
{
	return mArguments[index];
}
	

}