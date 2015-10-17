#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "cinder/Log.h"
#include "Osc.h"

#define USE_UDP 1

using namespace ci;
using namespace ci::app;
using namespace std;

class SimpleReceiverApp : public App {
  public:
	SimpleReceiverApp();
	void setup() override;
	void update() override;
	void draw() override;
	
	Color	mCircleColor = Color( 1, 1, 1 );
	ivec2	mCurrentMousePositon;
	bool	mMouseDown = false;
	
#if USE_UDP
	osc::ReceiverUdp mReceiver;
#else
	osc::ReceiverTcp mReceiver;
#endif
};

SimpleReceiverApp::SimpleReceiverApp()
: mReceiver( 10001 )
{
}

void SimpleReceiverApp::setup()
{
	mReceiver.setListener( "/mousemove/1",
	[&]( const osc::Message &msg ){
		mCurrentMousePositon.x = msg[0].int32();
		mCurrentMousePositon.y = msg[1].int32();
	});
	mReceiver.setListener( "/mouseclick/1",
	[&]( const osc::Message &msg ){
		if( msg[0].boolean() ) {
			gl::clearColor( Color( 1, 0, 0 ) );
			mCircleColor = Color( 0, 0, 0 );
		}
		else {
			gl::clearColor( Color( 0, 0, 0 ) );
			mCircleColor = Color( .5, .5, .5 );
		}
	});
	mReceiver.setListener( "/windowPos/1",
	[&]( const osc::Message &msg ){
		setWindowPos( ivec2( msg[0].int32(), msg[1].int32() ) );
		setWindowSize( ivec2( msg[2].int32(), msg[3].int32() ) );
	});
	mReceiver.bind();
	mReceiver.listen();
}

void SimpleReceiverApp::update()
{
}

void SimpleReceiverApp::draw()
{
	gl::clear( GL_COLOR_BUFFER_BIT );
	gl::setMatricesWindow( getWindowSize() );
	gl::color( mCircleColor );
	gl::drawSolidCircle( mCurrentMousePositon, 100 );
}

CINDER_APP( SimpleReceiverApp, RendererGl )
