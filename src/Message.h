//
//  Message.h
//  Test
//
//  Created by ryan bartley on 8/28/15.
//
//

#pragma once

#include "Argument.h"
#include "tinyosc/tinyosc.h"

namespace osc {
	
class Message {
public:
	Message();
	Message( std::string address );
	
	Message( const Message &other ) = default;
	Message( Message &&other ) = default;
	Message& operator=( const Message &other ) = default;
	Message& operator=( Message &&other ) = default;
	
	~Message() = default;
	
	void addFloat( float arg );
	void addInt( int32_t arg );
	void addString( const std::string &arg );
	
	float			getFloat( uint32_t index ) const;
	int32_t			getInt( uint32_t index ) const;
	std::string		getString( uint32_t index ) const;
	const Argument& getArgument( uint32_t index ) const;
	
	void				setAddress( const std::string &address );
	const std::string&	getAddress() const { return mAddress; }
	
private:
	
	
	
	std::string				mAddress;
	std::vector<Argument>	mArguments;
};
	
}
