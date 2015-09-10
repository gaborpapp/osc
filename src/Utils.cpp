//
//  Utils.cpp
//  Test
//
//  Created by ryan bartley on 9/10/15.
//
//

#include "Utils.h"

namespace osc {
	
const char* argTypeToString( ArgType type )
{
	switch ( type ) {
		case ArgType::INTEGER_32: return "INTEGER_32"; break;
		case ArgType::FLOAT: return "FLOAT"; break;
		case ArgType::DOUBLE: return "DOUBLE"; break;
		case ArgType::STRING: return "STRING"; break;
		case ArgType::BLOB: return "BLOB"; break;
		case ArgType::MIDI: return "MIDI"; break;
		case ArgType::TIME_TAG: return "TIME_TAG"; break;
		case ArgType::INTEGER_64: return "INTEGER_64"; break;
		case ArgType::BOOL_T: return "BOOL_T"; break;
		case ArgType::BOOL_F: return "BOOL_F"; break;
		case ArgType::CHAR: return "CHAR"; break;
		case ArgType::NULL_T: return "NULL_T"; break;
		case ArgType::INFINITUM: return "INFINITUM"; break;
		case ArgType::NONE: return "NONE"; break;
		default: return "Unknown ArgType"; break;
	}
}
	
namespace time {
	
static int64_t sTimeOffsetSecs = 0;
static int64_t sTimeOffsetUsecs = 0;
	
uint64_t get_current_ntp_time()
{
	auto now = std::chrono::system_clock::now();
	auto sec = std::chrono::duration_cast<std::chrono::seconds>( now.time_since_epoch() ).count() + 0x83AA7E80;
	auto usec = std::chrono::duration_cast<std::chrono::microseconds>( now.time_since_epoch() ).count() + 0x7D91048BCA000;
	
	return ( sec << 32 ) + ( usec % 1000000L );
}

uint64_t getClock()
{
	uint64_t ntp_time = get_current_ntp_time();

	uint64_t secs = ( ntp_time >> 32 ) + sTimeOffsetSecs;
	int64_t usecs = ( ntp_time & uint32_t( ~0 ) ) + sTimeOffsetUsecs;

	if( usecs < 0 ) {
		secs += usecs / 1000000;
		usecs += ( usecs / 1000000 ) * 1000000;
	}
	else {
		secs += usecs / 1000000;
		usecs -= ( usecs / 1000000 ) * 1000000;
	}

	return ( secs << 32 ) + usecs;
}

uint64_t getClock( uint32_t *year, uint32_t *month, uint32_t *day, uint32_t *hours, uint32_t *minutes, uint32_t *seconds )
{
	uint64_t ntp_time = getClock();

	// Convert to unix timestamp.
	std::time_t sec_since_epoch = ( ntp_time - ( uint64_t( 0x83AA7E80 ) << 32 ) ) >> 32;

	auto tm = std::localtime( &sec_since_epoch );
	if( year ) *year = tm->tm_year + 1900;
	if( month ) *month = tm->tm_mon + 1;
	if( day ) *day = tm->tm_mday;
	if( hours ) *hours = tm->tm_hour;
	if( minutes ) *minutes = tm->tm_min;
	if( seconds )*seconds = tm->tm_sec;

	return ntp_time;
}

std::string getClockString( bool includeDate )
{
	uint32_t year, month, day, hours, minutes, seconds;
	getClock( &year, &month, &day, &hours, &minutes, &seconds );

	char buffer[128];

	if( includeDate )
		sprintf( buffer, "%d/%d/%d %02d:%02d:%02d", month, day, year, hours, minutes, seconds );
	else
		sprintf( buffer, "%02d:%02d:%02d", hours, minutes, seconds );

	return std::string( buffer );
}

void setClock( uint64_t ntp_time )
{
	uint64_t current_ntp_time = get_current_ntp_time();

	sTimeOffsetSecs = ( ntp_time >> 32 ) - ( current_ntp_time >> 32 );
	sTimeOffsetUsecs = ( ntp_time & uint32_t( ~0 ) ) - ( current_ntp_time & uint32_t( ~0 ) );
}

}} // time // osc