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

/// @file tnyosc-dispatch.hpp
/// @brief tnyosc dispatch header file
/// @author Toshiro Yamada
#pragma once

#include "Message.h"

#include <string>
#include <vector>
#include <list>

#include <time.h>

namespace tnyosc {

/*
typedef void (*osc_method)(const std::string& address,
const std::vector<Argument>& argv, void* user_data);
*/
using ReceivedMessageCallback = std::function<void( const std::string &, const Message &, void * )>;

// structure to hold method handles
struct MethodTemplate {
	std::string address; // OSC-Address
	std::string types; // OSC-types as a string
	void* user_data; // user data
	ReceivedMessageCallback method; // OSC-Methods to call
};

struct ParsedMessage {
	uint64_t	timetag;
	Message		message;
};

// structure to hold callback function for a given OSC packet
struct Callback {
	uint64_t timetag; // OSC-timetag to determine when to call the method
	Message meesage;
	void* user_data; // user data
	ReceivedMessageCallback method; // matched method to call
};

using CallbackRef = std::shared_ptr<struct Callback> ;
// use to sort list<Callback> according to their timetag

class Dispatcher {
public:
	Dispatcher() = default;
	~Dispatcher() = default;

	/// Add a method template that may respond to an incoming Open Sound Control 
	/// message. Use match_methods to deserialize a raw OSC message and match 
	/// with the added methods.
	void addMethod( const char* address, ReceivedMessageCallback method, void* user_data );

	/// Deserializes a raw Open Sound Control message (as coming from a network)
	/// and returns a list of CallbackRef that matches with the registered method
	/// tempaltes.
	std::list<CallbackRef> matchMethods( const char* data, size_t size );

	/// decode_data is called inside match_methods to extract the OSC data from
	/// a raw data.
	static bool decode_data( const char* data, size_t size,
							 std::list<ParsedMessage>& messages, uint64_t timetag = 0 );

private:
	static bool decode_osc( const char* data, size_t size,
							std::list<ParsedMessage>& messages, uint64_t timetag );
	static bool pattern_match( const std::string& lhs, const std::string& rhs );

	std::list<MethodTemplate> mMethods;
};

} // namespace tnyosc