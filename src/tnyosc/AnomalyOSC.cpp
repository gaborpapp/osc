#include "AnomalyOSC.h"

#include "cinder/app/App.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace osc {

	// ================================================================= OSC Request

	OSCRequest::OSCRequest( OSCServer &server, const tnyosc::Message &message )
		: mMessage( message ), mSocket( server.getIo() ), mPort( server.getPort() )
	{
		//CI_LOG_V( "Created OSCRequest:" << message << "@" << mPort );
	}

	OSCRequest::~OSCRequest()
	{}

	bool OSCRequest::send( const asio::ip::udp::endpoint &recepient )
	{
		CI_LOG_W( "The send() method has not been implemented." );
		/*
		try {
		OSCServerRef server = getServer();
		if( server ) {
		osc().getSocket().send_to( asio::buffer( mMessage.data(), mMessage.size() ), recepient );
		return true;
		}
		}
		catch( std::exception &e ) {
		std::cout << e.what() << "\n";
		}
		*/
		return false;
	}

	bool OSCRequest::send( const std::vector<asio::ip::udp::endpoint> &recepients )
	{
		CI_LOG_W( "The send() method has not been implemented." );
		/*
		bool status = true;

		for( auto &recepient : recepients ) {
		bool success = send( recepient );
		if( ! success  ) {
		status = false;
		}
		}

		return status;
		*/
		return false;
	}

	void OSCRequest::broadcast()
	{
		try {
			mSocket.open( asio::ip::udp::v4() );
			mSocket.set_option( asio::socket_base::broadcast( true ) );

			asio::ip::udp::endpoint broadcastEndpoint( asio::ip::address_v4::broadcast(), mPort );

			mSocket.send_to( asio::buffer( mMessage.data(), mMessage.size() ), broadcastEndpoint );
			mSocket.close();
		}
		catch( std::exception &e ) {
			CI_LOG_E( "Failed to broadcast: " << e.what() );
		}
	}


	// ================================================================== OSC Server

	int64_t OSCServer::sTimeOffsetSecs = 0;
	int64_t OSCServer::sTimeOffsetUsecs = 0;

	OSCServer::OSCServer( int port )
		: mSocket( mIo, asio::ip::udp::endpoint( asio::ip::udp::v4(), port ) ), mPort( port )
	{
		asio::socket_base::reuse_address reuse( true );
		mSocket.set_option( reuse );
		
		mBuffer = std::vector<char>( 512 );
	}

	OSCServer::~OSCServer()
	{
		stop();
	}

	void OSCServer::registerHandler( const std::string &address, OSCCallbackType callback )
	{
		// To avoid larger refactoring of tnyosc at this time, 
		// wrapping <callback> in a tnyosc required signature.
		tnyosc::osc_method wrappedCallback = [callback]( const std::string &address, const std::vector<tnyosc::Argument> &argv, void *userData ) {
			OSCMessageRef message = OSCMessage::create( address, argv );

			// Perform callback.
			callback( message );
		};

		mDispatcher.add_method( address.c_str(), nullptr, wrappedCallback, nullptr );
	}

	void OSCServer::start()
	{
		listen();
	}

	void OSCServer::stop()
	{
		mSocket.close();
	}

	void OSCServer::poll()
	{
		mIo.poll();
	}

	void OSCServer::listen()
	{
		mSocket.async_receive(
			asio::buffer( mBuffer ),
			[&]( const asio::error_code &error, std::size_t bytesTransferred ) {
			listenHandler( error, bytesTransferred );
		} );
	}

	void OSCServer::listenHandler( const asio::error_code &error, std::size_t bytesTransferred )
	{
		if( !error || error == asio::error::message_size ) {
			std::list<tnyosc::CallbackRef> callbacks = mDispatcher.match_methods( mBuffer.data(), bytesTransferred );
			for( auto &callback : callbacks ) {
				callback->method( callback->address, callback->argv, callback->user_data );
			}
		}

		listen();
	}

	OSCRequestRef OSCServer::request( const tnyosc::Message &message )
	{
		return OSCRequest::create( *this, message );
	}


	uint64_t OSCServer::getClock()
	{
		uint64_t ntp_time = tnyosc::get_current_ntp_time();

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

	uint64_t OSCServer::getClock( int *year, int *month, int *day, int *hours, int *minutes, int *seconds )
	{
		assert( year != nullptr );
		assert( month != nullptr );
		assert( day != nullptr );
		assert( hours != nullptr );
		assert( minutes != nullptr );
		assert( seconds != nullptr );

		uint64_t ntp_time = getClock();

		// Convert to unix timestamp.
		std::time_t sec_since_epoch = ( ntp_time - ( uint64_t( 0x83AA7E80 ) << 32 ) ) >> 32;

		auto tm = std::localtime( &sec_since_epoch );
		*year = tm->tm_year + 1900;
		*month = tm->tm_mon + 1;
		*day = tm->tm_mday;
		*hours = tm->tm_hour;
		*minutes = tm->tm_min;
		*seconds = tm->tm_sec;

		return ntp_time;
	}

	std::string OSCServer::getClockString( bool includeDate )
	{
		int year, month, day, hours, minutes, seconds;
		getClock( &year, &month, &day, &hours, &minutes, &seconds );

		char buffer[128];

		if( includeDate )
			sprintf( buffer, "%d/%d/%d %02d:%02d:%02d", month, day, year, hours, minutes, seconds );
		else
			sprintf( buffer, "%02d:%02d:%02d", hours, minutes, seconds );

		return std::string( buffer );
	}

	void OSCServer::setClock( uint64_t ntp_time )
	{
		uint64_t current_ntp_time = tnyosc::get_current_ntp_time();

		sTimeOffsetSecs = ( ntp_time >> 32 ) - ( current_ntp_time >> 32 );
		sTimeOffsetUsecs = ( ntp_time & uint32_t( ~0 ) ) - ( current_ntp_time & uint32_t( ~0 ) );
	}

	// ================================================================= Anomaly OSC

	namespace anomaly {

		AnomalyOSC::AnomalyOSC( int port )
			: OSCServer( port ), mId( "*" )
		{
		}

		void AnomalyOSC::registerHandler( const std::string &address, OSCCallbackType callback )
		{
			auto id = mId;

			// To avoid larger refactoring of tnyosc at this time, 
			// wrapping <callback> in a tnyosc required signature.
			tnyosc::osc_method wrappedCallback = [id, callback]( const std::string &address, const std::vector<tnyosc::Argument> &argv, void *userData ) {
				OSCMessageRef message = OSCMessage::create( address, argv );

				// Check if message has sender and recipient arguments.
				const auto& args = message->getArgs();
				if( args.size() < 2 ) // invalid param count
				{
					CI_LOG_W( "Invalid number of params: " << args.size() );
					return;
				}
				if( args[0].type != 's' || args[1].type != 's' ) // invalid param types
				{
					CI_LOG_W( "Invalid param types: " << args[0].type << args[1].type );
					return;
				}

				std::string sender = args[0].data.s;
				if( sender.empty() || sender == "*" ) // invalid sender
				{
					CI_LOG_W( "Invalid sender ID:" << sender );
					return;
				}

				// Filter on recipient.
				std::string recipient = args[1].data.s;
				if( recipient == "*" || recipient == id ) {
					// Perform callback.
					callback( message );
				}
			};

			mDispatcher.add_method( address.c_str(), nullptr, wrappedCallback, nullptr );
		}

		void AnomalyOSC::listenHandler( const asio::error_code &error, std::size_t bytesTransferred )
		{
			if( !error || error == asio::error::message_size ) {
				std::list<tnyosc::CallbackRef> callbacks = mDispatcher.match_methods( mBuffer.data(), bytesTransferred );
				for( auto &callback : callbacks ) {

					std::string recepientId = callback->argv[1].data.s;
					if( recepientId == getIdentifier() || recepientId == ANY || recepientId == "all" ) {
						callback->method( callback->address, callback->argv, callback->user_data );
					}
				}
			}
			else {
				CI_LOG_E( "Error " << error.value() << ": " << error.message() );
			}

			listen();
		}

		// ================================================================= Anomaly API

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Test

		OSCRequestRef test( const std::string &recipient )
		{
			tnyosc::Message msg( TEST );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( "Test osc message." );

			return osc().request( msg );
		}

		OSCRequestRef testResponse( bool response, const std::string &recipient )
		{
			tnyosc::Message msg( TEST_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( response );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Minimize

		OSCRequestRef minimize( const std::string &recipient )
		{
			tnyosc::Message msg( MINIMIZE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef minimizeQuery( const std::string &recipient )
		{
			tnyosc::Message msg( MINIMIZE_QUERY );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef minimizeResponse( bool response, const std::string &recipient )
		{
			tnyosc::Message msg( MINIMIZE_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( response );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Maximize

		OSCRequestRef maximize( const std::string &recipient )
		{
			tnyosc::Message msg( MAXIMIZE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef maximizeQuery( const std::string &recipient )
		{
			tnyosc::Message msg( MAXIMIZE_QUERY );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef maximizeResponse( bool response, const std::string &recipient )
		{
			tnyosc::Message msg( MAXIMIZE_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( response );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Fullscreen

		OSCRequestRef fullscreen( const std::string &recipient )
		{
			tnyosc::Message msg( FULLSCREEN );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef fullscreenQuery( const std::string &recipient )
		{
			tnyosc::Message msg( FULLSCREEN_QUERY );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef fullscreenResponse( bool response, const std::string &recipient )
		{
			tnyosc::Message msg( FULLSCREEN_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( response );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Windowed

		OSCRequestRef windowed( const std::string &recipient )
		{
			tnyosc::Message msg( WINDOWED );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef windowedQuery( const std::string &recipient )
		{
			tnyosc::Message msg( WINDOWED_QUERY );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef windowedResponse( bool response, const std::string &recipient )
		{
			tnyosc::Message msg( WINDOWED_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( response );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - Whois

		OSCRequestRef whois( const std::string &recipient )
		{
			tnyosc::Message msg( WHOIS );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef whoisResponse( const std::string &recipient )
		{
			tnyosc::Message msg( WHOIS_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - Heartbeat

		OSCRequestRef heartbeat( const std::string &recipient )
		{
			tnyosc::Message msg( HEARTBEAT );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Shutdown

		OSCRequestRef shutdown( const std::string &recipient )
		{
			tnyosc::Message msg( SHUTDOWN );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef shutdownCancel( const std::string &recipient )
		{
			tnyosc::Message msg( SHUTDOWN_CANCEL );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef shutdownQuery( const std::string &recipient )
		{
			tnyosc::Message msg( SHUTDOWN_QUERY );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef shutdownResponse( bool response, const std::string &recipient )
		{
			tnyosc::Message msg( SHUTDOWN_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( response );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Time

		OSCRequestRef timeChange( const uint64_t &value, const std::string &recipient )
		{
			tnyosc::Message msg( TIME );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.appendTimeTag( value );

			return osc().request( msg );
		}

		OSCRequestRef timeQuery( const std::string &recipient )
		{
			tnyosc::Message msg( TIME_QUERY );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef timeResponse( const uint64_t &value, const std::string &recipient )
		{
			tnyosc::Message msg( TIME_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.appendTimeTag( value );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - Scene Start

		OSCRequestRef sceneStart( const std::string &scene, const std::string &recipient )
		{
			tnyosc::Message msg( SCENE_START );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( scene );

			return osc().request( msg );
		}

		OSCRequestRef sceneStartResponse( const std::string &scene, bool success, const std::string &recipient )
		{
			tnyosc::Message msg( SCENE_START_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( scene );
			msg.append( success );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Scene Current

		OSCRequestRef sceneCurrentQuery( const std::string &recipient )
		{
			tnyosc::Message msg( SCENE_CURRENT_QUERY );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef sceneCurrentResponse( const std::string &scene, const std::string &recipient )
		{
			tnyosc::Message msg( SCENE_CURRENT_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( scene );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Scene Status

		OSCRequestRef sceneStatus( const std::string &scene, const std::string &recipient )
		{
			tnyosc::Message msg( SCENE_STATUS );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( scene );

			return osc().request( msg );
		}

		OSCRequestRef sceneStatusResponse( const std::string &scene, const std::string &state, const std::string &recipient )
		{
			tnyosc::Message msg( SCENE_STATUS_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( scene );
			msg.append( state );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Scene List

		OSCRequestRef sceneList( const std::string &recipient )
		{
			tnyosc::Message msg( SCENE_LIST );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef sceneListResponse( const vector<std::string> &scenes, const std::string &recipient )
		{
			tnyosc::Message msg( SCENE_LIST_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			for( auto &scene : scenes ) {
				msg.append( scene );
			}

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Scene Preload

		OSCRequestRef scenePreload( const std::string &scene, const std::string &recipient )
		{
			tnyosc::Message msg( SCENE_PRELOAD );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( scene );

			return osc().request( msg );
		}

		OSCRequestRef scenePreloadResponse( const std::string &scene, bool success, const std::string &recipient )
		{
			tnyosc::Message msg( SCENE_PRELOAD_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( scene );
			msg.append( success );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Scene Unload

		OSCRequestRef sceneUnload( const std::string &scene, const std::string &recipient )
		{
			tnyosc::Message msg( SCENE_UNLOAD );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( scene );

			return osc().request( msg );
		}

		OSCRequestRef sceneUnloadResponse( const std::string &scene, bool success, const std::string &recipient )
		{
			tnyosc::Message msg( SCENE_UNLOAD_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( scene );
			msg.append( success );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Touch Begin

		OSCRequestRef touchBegin( int touchID, float normX, float normY, float force, const std::string &recipient )
		{
			tnyosc::Message msg( TOUCH_BEGIN );
			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( normX );
			msg.append( normY );
			msg.append( force );
			msg.append( touchID );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Touch Move

		OSCRequestRef touchMove( int touchID, float normX, float normY, float force, const std::string &recipient )
		{
			tnyosc::Message msg( TOUCH_MOVE );
			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( normX );
			msg.append( normY );
			msg.append( force );
			msg.append( touchID );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Touch End

		OSCRequestRef touchEnd( int touchID, float normX, float normY, float force, const std::string &recipient )
		{
			tnyosc::Message msg( TOUCH_END );
			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( normX );
			msg.append( normY );
			msg.append( force );
			msg.append( touchID );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Blob Begin

		OSCRequestRef blobBegin( int blobID, float normX, float normY, float width, float height, const std::string &recipient )
		{
			tnyosc::Message msg( BLOB_BEGIN );
			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( normX );
			msg.append( normY );
			msg.append( width );
			msg.append( height );
			msg.append( blobID );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Blob Move

		OSCRequestRef blobMove( int blobID, float normX, float normY, float width, float height, const std::string &recipient )
		{
			tnyosc::Message msg( BLOB_MOVE );
			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( normX );
			msg.append( normY );
			msg.append( width );
			msg.append( height );
			msg.append( blobID );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Blob End

		OSCRequestRef blobEnd( int blobID, float normX, float normY, float width, float height, const std::string &recipient )
		{
			tnyosc::Message msg( BLOB_END );
			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( normX );
			msg.append( normY );
			msg.append( width );
			msg.append( height );
			msg.append( blobID );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Media Category Activate

		OSCRequestRef mediaCategoryActivate( const std::string &categoryName, const std::string &recipient )
		{
			tnyosc::Message msg( MEDIA_CATEGORY_ACTIVATE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( categoryName );

			return osc().request( msg );
		}

		OSCRequestRef mediaCategoryActivateResponse( const std::string &categoryName, bool success, const std::string &recipient )
		{
			tnyosc::Message msg( MEDIA_SINGLEVIEW_ACTIVATE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( categoryName );
			msg.append( success );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Media Category Deactivate

		OSCRequestRef mediaCategoryDeactivate( const std::string &categoryName, const std::string &recipient )
		{
			tnyosc::Message msg( MEDIA_CATEGORY_DEACTIVATE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( categoryName );

			return osc().request( msg );
		}

		OSCRequestRef mediaCategoryDeactivateResponse( const std::string &categoryName, bool success, const std::string &recipient )
		{
			tnyosc::Message msg( MEDIA_SINGLEVIEW_DEACTIVATE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( categoryName );
			msg.append( success );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Media Singleview Activate

		OSCRequestRef mediaSingleviewActivate( const std::string &recipient )
		{
			tnyosc::Message msg( MEDIA_SINGLEVIEW_ACTIVATE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef mediaSingleviewActivateResponse( bool success, const std::string &recipient )
		{
			tnyosc::Message msg( MEDIA_SINGLEVIEW_ACTIVATE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( success );

			return osc().request( msg );
		}


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Media Singleview Deactivate

		OSCRequestRef mediaSingleviewDeactivate( const std::string &recipient )
		{
			tnyosc::Message msg( MEDIA_SINGLEVIEW_DEACTIVATE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef mediaSingleviewDeactivateResponse( bool success, const std::string &recipient )
		{
			tnyosc::Message msg( MEDIA_SINGLEVIEW_DEACTIVATE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( success );

			return osc().request( msg );
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Media General Reset

		OSCRequestRef mediaGeneralReset( const std::string &recipient )
		{
			tnyosc::Message msg( MEDIA_GENERAL_RESET );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef mediaGeneralResetResponse( bool success, const std::string &recipient )
		{
			tnyosc::Message msg( MEDIA_GENERAL_RESET );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( success );

			return osc().request( msg );
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Calendar Show

		OSCRequestRef calendarShow( const std::string &recipient )
		{
			tnyosc::Message msg( CALENDAR_SHOW );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef calendarShowResponse( bool success, const std::string &recipient )
		{
			tnyosc::Message msg( CALENDAR_SHOW_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( success );

			return osc().request( msg );
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Calendar Hide

		OSCRequestRef calendarHide( const std::string &recipient )
		{
			tnyosc::Message msg( CALENDAR_HIDE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef calendarHideResponse( bool success, const std::string &recipient )
		{
			tnyosc::Message msg( CALENDAR_HIDE_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( success );

			return osc().request( msg );
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Video Status

		OSCRequestRef videoStatus( const std::string &recipient )
		{
			tnyosc::Message msg( VIDEO_STATUS );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef videoStatusResponse( const std::string &video, const std::string &status, const std::string &recipient )
		{
			tnyosc::Message msg( VIDEO_STATUS_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( video );
			msg.append( status );

			return osc().request( msg );
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Video Current

		OSCRequestRef videoCurrentQuery( const std::string &recipient )
		{
			tnyosc::Message msg( VIDEO_CURRENT_QUERY );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef videoCurrentResponse( const std::string &video, const std::string &recipient )
		{
			tnyosc::Message msg( VIDEO_CURRENT_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( video );

			return osc().request( msg );
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Video Load

		OSCRequestRef videoLoad( const std::string &file, const std::string &recipient )
		{
			tnyosc::Message msg( VIDEO_LOAD );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( file );

			return osc().request( msg );
		}

		OSCRequestRef videoLoadResponse( bool success, const std::string &recipient )
		{
			tnyosc::Message msg( VIDEO_LOAD_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( success );

			return osc().request( msg );
		}

		/*// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Video Unload

		OSCRequestRef videoUnload( const std::string &recipient )
		{
			tnyosc::Message msg( VIDEO_UNLOAD );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef videoUnloadResponse( bool success, const std::string &recipient )
		{
			tnyosc::Message msg( VIDEO_UNLOAD_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( success );

			return osc().request( msg );
		}	//*/

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Video Play

		OSCRequestRef videoPlay( const std::string &recipient )
		{
			tnyosc::Message msg( VIDEO_PLAY );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef videoPlayResponse( bool success, const std::string &recipient )
		{
			tnyosc::Message msg( VIDEO_PLAY_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( success );

			return osc().request( msg );
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Video Pause

		OSCRequestRef videoPause( const std::string &recipient )
		{
			tnyosc::Message msg( VIDEO_PAUSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef videoPauseResponse( bool success, const std::string &recipient )
		{
			tnyosc::Message msg( VIDEO_PAUSE_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( success );

			return osc().request( msg );
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Video List

		OSCRequestRef videoList( const std::string &recipient )
		{
			tnyosc::Message msg( VIDEO_LIST );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			return osc().request( msg );
		}

		OSCRequestRef videoListResponse( const vector<std::string> &videos, const std::string &recipient )
		{
			tnyosc::Message msg( VIDEO_LIST_RESPONSE );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );

			for( auto &video : videos ) {
				msg.append( video );
			}

			return osc().request( msg );
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Video Stopped

		OSCRequestRef videoStopped( const std::string &file, const std::string &recipient )
		{
			tnyosc::Message msg( VIDEO_STOPPED );

			msg.append( osc().getIdentifier() );
			msg.append( recipient );
			msg.append( file );

			return osc().request( msg );
		}

	} // namespace anomaly
} // namespace osc
