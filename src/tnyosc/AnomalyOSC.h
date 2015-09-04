#pragma once

#define ASIO_STANDALONE 1
#include "asio/asio.hpp"

#include "Bundle.hpp"
#include "tnyosc-dispatch.h"

namespace osc {

	//----------------------------------------------------------------------------------------------------

	typedef std::shared_ptr<class OSCRequest> OSCRequestRef;

	class OSCRequest {
	public:
		OSCRequest( class OSCServer &server, const tnyosc::Message &message );
		~OSCRequest();

		static std::shared_ptr<class OSCRequest> create( class OSCServer &server, const tnyosc::Message &message )
		{
			return std::make_shared<OSCRequest>( server, message );
		}

		//OSCServerRef getServer() { return mServer.lock(); }

		const tnyosc::Message& getMessage() const { return mMessage; }
		void setMessage( const tnyosc::Message &message ) { mMessage = message; }

		// SEND message to one or more endpoints. (not implemented)
		bool send( const asio::ip::udp::endpoint &recepient );
		bool send( const std::vector<asio::ip::udp::endpoint> &recepients );

		// BROADCAST message to network.
		void broadcast();

	private:
		//OSCServerWeakRef	mServer;
		tnyosc::Message		mMessage;

		const int					mPort;
		asio::ip::udp::socket		mSocket;
		asio::ip::udp::endpoint		mEndpoint;
	};

	//----------------------------------------------------------------------------------------------------

	//typedef std::shared_ptr<class OSCServer> OSCServerRef;
	//typedef std::weak_ptr<class OSCServer> OSCServerWeakRef;

	class OSCServer /* : public std::enable_shared_from_this < OSCServer > */ {
	public:
		OSCServer( int port );
		virtual ~OSCServer();

		static std::shared_ptr<OSCServer> create( int port ) { return std::make_shared<OSCServer>( port ); }

		//! Returns the port.
		int const getPort() const { return mPort; }
		//! Sets the port.
		void setPort( int port ) { mPort = port; }

		//! Returns the socket.
		//const asio::ip::udp::socket& getSocket() const { return mSocket; }
		//! Returns the dispatcher.
		//const tnyosc::Dispatcher& getDispatcher() const { return mDispatcher; }
		//! Returns the dispatcher.
		//tnyosc::Dispatcher& getDispatcher() { return mDispatcher; }
		//! Returns the buffer.
		//const std::vector<char>& getBuffer() const { return mBuffer; }

		//! Adds and endpoint to the dispatcher. Will not check if endpoint has already been added.
		virtual void registerHandler( const std::string &address, OSCCallbackType callback );
		//! Adds and endpoint to the dispatcher. Will not check if endpoint has already been added.
		template<typename T, typename Y>
		void registerHandler( const std::string &address, T fn, Y *inst ) { registerHandler( address, std::bind( fn, inst, std::placeholders::_1 ) ); }

		//! Starts listening for packets.
		virtual void start();
		//! Stops listening for packets.
		virtual void stop();

		//! Poll for packets. Should be called as often as possible.
		void poll();

		//! Constructs a request object.
		virtual OSCRequestRef request( const tnyosc::Message &message );

		//! Returns the I/O object.
		const asio::io_service& getIo() const { return mIo; }
		//! Returns the I/O object.
		asio::io_service& getIo() { return mIo; }

		//! Returns the current presentation time as NTP time, which may include an offset to the system clock.
		static uint64_t getClock();
		//! Returns the current presentation time as NTP time, which may include an offset to the system clock.
		static uint64_t getClock( int *year, int *month, int *day, int *hours, int *minutes, int *seconds );
		//! Returns the system clock as NTP time.
		static uint64_t getSystemClock() { return tnyosc::get_current_ntp_time(); }
		//! Returns the current presentation time as a string.
		static std::string getClockString( bool includeDate = true );
		//! Sets the current presentation time as NTP time, from which an offset to the system clock is calculated.
		static void setClock( uint64_t ntp_time );

	protected:
		virtual void listen();
		virtual void listenHandler( const asio::error_code &error, std::size_t bytesTransferred );

	protected:
		std::string					mHost;
		int							mPort;

		tnyosc::Dispatcher			mDispatcher;

		asio::io_service			mIo;
		asio::ip::udp::socket		mSocket;
		asio::ip::udp::endpoint		mEndpoint;
		std::vector<char>			mBuffer;

		static int64_t              sTimeOffsetSecs;
		static int64_t              sTimeOffsetUsecs;
	};

	//----------------------------------------------------------------------------------------------------

	namespace anomaly {

		typedef std::shared_ptr<class AnomalyOSC> AnomalyOSCRef;

		class AnomalyOSC : public osc::OSCServer {
		public:
			static AnomalyOSC& instance()
			{
				static AnomalyOSC sInstance;
				return sInstance;
			}

			//! Returns the name of this client.
			const std::string& getIdentifier() const { return mId; }
			//! Sets the name of this client.
			void setIdentifier( const std::string &id ) { mId = id; }

			// ADD endpoints to dispatcher.
			void registerHandler( const std::string &address, OSCCallbackType callback ) override;
			//! Adds and endpoint to the dispatcher. Will not check if endpoint has already been added.
			template<typename T, typename Y>
			void registerHandler( const std::string &address, T fn, Y *inst ) { registerHandler( address, std::bind( fn, inst, std::placeholders::_1 ) ); }

		private:
			AnomalyOSC() : AnomalyOSC( 13540 ) {}
			AnomalyOSC( int port );

			// CUSTOM receive handler that ensures recipient matches <identifier>.
			virtual void listenHandler( const asio::error_code &error, std::size_t byteTransferred ) override;

		private:
			std::string  mId;
		};

		static inline AnomalyOSC& osc() { return AnomalyOSC::instance(); }


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - Endpoints

		const std::string PROJECTION_WALL = "projection_wall";
		const std::string MEDIA_WALL = "media_wall";
		const std::string CONTROLLER = "controller";
		const std::string AMX = "amx";
		const std::string ANY = "*";


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - Scenes
		const std::string SCENE_ALL = "all";
		const std::string SCENE_NONE = "none";
		const std::string SCENE_NOBEL_GHOSTS = "nobel_ghosts";
		const std::string SCENE_BELL_LABS_HEROES = "bell_labs_heroes";
		const std::string SCENE_NETWORK = "network";
		const std::string SCENE_PHYSICS = "physics";
		const std::string SCENE_SOFTWARE = "software";
		const std::string SCENE_STARFIELD = "starfield";
		const std::string SCENE_MEDIA_LIBRARY = "media_library";
        const std::string SCENE_ACTIVATION = "activation";


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - Scene Status
		const std::string SCENE_STATUS_UNKNOWN = "unknown";
		const std::string SCENE_STATUS_LOADING = "loading";
		const std::string SCENE_STATUS_OUT_OF_MEMORY = "oom";
		const std::string SCENE_STATUS_INCOMPLETE = "incomplete";
		const std::string SCENE_STATUS_READY = "ready";
		const std::string SCENE_STATUS_STARTING = "starting";
		const std::string SCENE_STATUS_CURRENT = "current";
		const std::string SCENE_STATUS_STOPPING = "stopping";

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - Video Status
		const std::string VIDEO_STATUS_UNKNOWN = "unknown";
		const std::string VIDEO_STATUS_HIDDEN = "hidden";
		const std::string VIDEO_STATUS_PLAYING = "playing";
		const std::string VIDEO_STATUS_STOPPED = "stopped";


		// - - - - - - - - - - - - - - - - - - - - - - - - - - -  Miscellaneous Commands

		const std::string TEST = "/BELL/test";
		const std::string TEST_RESPONSE = "/BELL/test/response";

		OSCRequestRef test( const std::string &recipient = ANY );
		OSCRequestRef testResponse( bool response, const std::string &recipient = ANY );


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Application Commands

		const std::string MINIMIZE = "/BELL/app/minimize";
		const std::string MINIMIZE_QUERY = "/BELL/app/minimize/query";
		const std::string MINIMIZE_RESPONSE = "/BELL/app/minimize/response";

		OSCRequestRef minimize( const std::string &recipient = ANY );
		OSCRequestRef minimizeQuery( const std::string &recipient = ANY );
		OSCRequestRef minimizeResponse( bool response, const std::string &recipient = ANY );


		const std::string MAXIMIZE = "/BELL/app/maximize";
		const std::string MAXIMIZE_QUERY = "/BELL/app/maximize/query";
		const std::string MAXIMIZE_RESPONSE = "/BELL/app/maximize/response";

		OSCRequestRef maximize( const std::string &recipient = ANY );
		OSCRequestRef maximizeQuery( const std::string &recipient = ANY );
		OSCRequestRef maximizeResponse( bool response, const std::string &recipient = ANY );


		const std::string FULLSCREEN = "/BELL/app/fullscreen";
		const std::string FULLSCREEN_QUERY = "/BELL/app/fullscreen/query";
		const std::string FULLSCREEN_RESPONSE = "/BELL/app/fullscreen/response";

		OSCRequestRef fullscreen( const std::string &recipient = ANY );
		OSCRequestRef fullscreenQuery( const std::string &recipient = ANY );
		OSCRequestRef fullscreenResponse( bool response, const std::string &recipient = ANY );


		const std::string WINDOWED = "/BELL/app/windowed";
		const std::string WINDOWED_QUERY = "/BELL/app/windowed/query";
		const std::string WINDOWED_RESPONSE = "/BELL/app/windowed/response";

		OSCRequestRef windowed( const std::string &recipient = ANY );
		OSCRequestRef windowedQuery( const std::string &recipient = ANY );
		OSCRequestRef windowedResponse( bool response, const std::string &recipient = ANY );


		const std::string WHOIS = "/BELL/app/whois";
		const std::string WHOIS_RESPONSE = "/BELL/app/whois/response";

		OSCRequestRef whois( const std::string &recipient = ANY );
		OSCRequestRef whoisResponse( const std::string &recipient = ANY );


		const std::string HEARTBEAT = "/BELL/app/heartbeat";

		OSCRequestRef heartbeat( const std::string &recipient = ANY );


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - System Commands

		const std::string SHUTDOWN = "/BELL/system/shutdown";
		const std::string SHUTDOWN_QUERY = "/BELL/system/shutdown/query";
		const std::string SHUTDOWN_CANCEL = "/BELL/system/shutdown/cancel";
		const std::string SHUTDOWN_RESPONSE = "/BELL/system/shutdown/response";

		OSCRequestRef shutdown( const std::string &recipient = ANY );
		OSCRequestRef shutdownCancel( const std::string &recipient = ANY );
		OSCRequestRef shutdownQuery( const std::string &recipient = ANY );
		OSCRequestRef shutdownResponse( bool response, const std::string &recipient = ANY );


		const std::string TIME = "/BELL/system/time";
		const std::string TIME_QUERY = "/BELL/system/time/query";
		const std::string TIME_RESPONSE = "/BELL/system/time/response";

		OSCRequestRef timeChange( const uint64_t &value, const std::string &recipient = ANY );
		OSCRequestRef timeQuery( const std::string &recipient = ANY );
		OSCRequestRef timeResponse( const uint64_t &value, const std::string &recipient = ANY );


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Scene Commands

		const std::string SCENE_START = "/BELL/scene/start";
		const std::string SCENE_START_RESPONSE = "/BELL/scene/start/response";

		OSCRequestRef sceneStart( const std::string &scene, const std::string &recipient = ANY );
		OSCRequestRef sceneStartResponse( const std::string &scene, bool success, const std::string &recipient = ANY );


		const std::string SCENE_STATUS = "/BELL/scene/status";
		const std::string SCENE_STATUS_RESPONSE = "/BELL/scene/status/response";

		OSCRequestRef sceneStatus( const std::string &scene, const std::string &recipient = ANY );
		OSCRequestRef sceneStatusResponse( const std::string &scene, const std::string &state, const std::string &recipient = ANY );


		const std::string SCENE_CURRENT_QUERY = "/BELL/scene/current/query";
		const std::string SCENE_CURRENT_RESPONSE = "/BELL/scene/current/response";

		OSCRequestRef sceneCurrentQuery( const std::string &recipient = ANY );
		OSCRequestRef sceneCurrentResponse( const std::string &scene, const std::string &recipient = ANY );


		const std::string SCENE_LIST = "/BELL/scene/list";
		const std::string SCENE_LIST_RESPONSE = "/BELL/scene/list/response";

		OSCRequestRef sceneList( const std::string &recipient = ANY );
		OSCRequestRef sceneListResponse( const std::vector<std::string> &scenes, const std::string &recipient = ANY );


		const std::string SCENE_PRELOAD = "/BELL/scene/preload";
		const std::string SCENE_PRELOAD_RESPONSE = "/BELL/scene/preload/response";

		OSCRequestRef scenePreload( const std::string &scene, const std::string &recipient = ANY );
		OSCRequestRef scenePreloadResponse( const std::string &scene, bool success, const std::string &recipient = ANY );


		const std::string SCENE_UNLOAD = "/BELL/scene/unload";
		const std::string SCENE_UNLOAD_RESPONSE = "/BELL/scene/unload/response";

		OSCRequestRef sceneUnload( const std::string &scene, const std::string &recipient = ANY );
		OSCRequestRef sceneUnloadResponse( const std::string &scene, bool success, const std::string &recipient = ANY );


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Touch Commands

		const std::string TOUCH_BEGIN = "/BELL/input/touch/begin";
		const std::string TOUCH_MOVE = "/BELL/input/touch/move";
		const std::string TOUCH_END = "/BELL/input/touch/end";

		OSCRequestRef touchBegin( int touchID, float normX, float normY, float force = 0.0f, const std::string &recipient = ANY );
		OSCRequestRef touchMove( int touchID, float normX, float normY, float force = 0.0f, const std::string &recipient = ANY );
		OSCRequestRef touchEnd( int touchID, float normX, float normY, float force = 0.0f, const std::string &recipient = ANY );


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Blob Commands

		const std::string BLOB_BEGIN = "/BELL/blob/begin";
		const std::string BLOB_MOVE = "/BELL/blob/move";
		const std::string BLOB_END = "/BELL/blob/end";

		OSCRequestRef blobBegin( int blobID, float normX, float normY, float width, float height, const std::string &recipient = ANY );
		OSCRequestRef blobMove( int blobID, float normX, float normY, float width, float height, const std::string &recipient = ANY );
		OSCRequestRef blobEnd( int blobID, float normX, float normY, float width, float height, const std::string &recipient = ANY );

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Media Library Commands

		const std::string MEDIA_CATEGORY_ACTIVATE = "/BELL/medialibrary/category/activate";
		const std::string MEDIA_CATEGORY_DEACTIVATE = "/BELL/medialibrary/category/deactivate";

		OSCRequestRef mediaCategoryActivate( const std::string &categoryName, const std::string &recipient = ANY );
		OSCRequestRef mediaCategoryActivateResponse( const std::string &categoryName, bool success, const std::string &recipient = ANY );
		OSCRequestRef mediaCategoryDeactivate( const std::string &categoryName, const std::string &recipient = ANY );
		OSCRequestRef mediaCategoryDeactivateResponse( const std::string &categoryName, bool success, const std::string &recipient = ANY );

		const std::string MEDIA_SINGLEVIEW_ACTIVATE = "/BELL/medialibrary/singleview/activate";
		const std::string MEDIA_SINGLEVIEW_DEACTIVATE = "/BELL/medialibrary/singleview/deactivate";

		OSCRequestRef mediaSingleviewActivate( const std::string &recipient = ANY );
		OSCRequestRef mediaSingleviewActivateResponse( bool success, const std::string &recipient = ANY );
		OSCRequestRef mediaSingleviewDeactivate( const std::string &recipient = ANY );
		OSCRequestRef mediaSingleviewDeactivateResponse( bool success, const std::string &recipient = ANY );

		const std::string MEDIA_GENERAL_RESET = "/BELL/medialibrary/reset";

		OSCRequestRef mediaGeneralReset( const std::string &recipient = ANY );
		OSCRequestRef mediaGeneralResetResponse( bool success, const std::string &recipient = ANY );

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - Calendar Commands

		const std::string CALENDAR_SHOW = "/BELL/calendar/show";
		const std::string CALENDAR_SHOW_RESPONSE = "/BELL/calendar/show/response";

		OSCRequestRef calendarShow( const std::string &recipient = ANY );
		OSCRequestRef calendarShowResponse( bool success, const std::string &recipient = ANY );

		const std::string CALENDAR_HIDE = "/BELL/calendar/hide";
		const std::string CALENDAR_HIDE_RESPONSE = "/BELL/calendar/hide/response";

		OSCRequestRef calendarHide( const std::string &recipient = ANY );
		OSCRequestRef calendarHideResponse( bool success, const std::string &recipient = ANY );


		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Video Commands

		const std::string VIDEO_STATUS = "/BELL/video/status";
		const std::string VIDEO_STATUS_RESPONSE = "/BELL/video/status/response";

		OSCRequestRef videoStatus( const std::string &recipient = ANY );
		OSCRequestRef videoStatusResponse( const std::string &video, const std::string &state, const std::string &recipient = ANY );
		
		const std::string VIDEO_CURRENT_QUERY = "/BELL/video/current/query";
		const std::string VIDEO_CURRENT_RESPONSE = "/BELL/video/current/response";

		OSCRequestRef videoCurrentQuery( const std::string &recipient = ANY );
		OSCRequestRef videoCurrentResponse( const std::string &video, const std::string &recipient = ANY );

		const std::string VIDEO_LOAD = "/BELL/video/load";
		const std::string VIDEO_LOAD_RESPONSE = "/BELL/video/load/response";

		OSCRequestRef videoLoad( const std::string &file, const std::string &recipient = ANY );
		OSCRequestRef videoLoadResponse( bool success, const std::string &recipient );

		//const std::string VIDEO_UNLOAD = "/BELL/video/unload";
		//const std::string VIDEO_UNLOAD_RESPONSE = "/BELL/video/unload/response";

		//OSCRequestRef videoUnload( const std::string &recipient = ANY );
		//OSCRequestRef videoUnloadResponse( bool success, const std::string &recipient );

		const std::string VIDEO_PLAY = "/BELL/video/play";
		const std::string VIDEO_PLAY_RESPONSE = "/BELL/video/play/response";

		OSCRequestRef videoPlay( const std::string &recipient = ANY );
		OSCRequestRef videoPlayResponse( bool success, const std::string &recipient );

		const std::string VIDEO_PAUSE = "/BELL/video/pause";
		const std::string VIDEO_PAUSE_RESPONSE = "/BELL/video/pause/response";

		OSCRequestRef videoPause( const std::string &recipient = ANY );
		OSCRequestRef videoPauseResponse( bool success, const std::string &recipient );

		const std::string VIDEO_LIST = "/BELL/video/list";
		const std::string VIDEO_LIST_RESPONSE = "/BELL/video/list/response";

		OSCRequestRef videoList( const std::string &recipient = ANY );
		OSCRequestRef videoListResponse( const std::vector<std::string> &videos, const std::string &recipient = ANY );

		const std::string VIDEO_STOPPED = "/BELL/video/stopped";
		
		OSCRequestRef videoStopped( const std::string &file, const std::string &recipient = ANY );

	}	// end namespace: anomaly
}	// end namespace: osc