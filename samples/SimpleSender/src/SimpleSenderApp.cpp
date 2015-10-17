#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "Osc.h"

#define USE_UDP 1

using namespace ci;
using namespace ci::app;
using namespace std;

const std::string destinationHost = "127.0.0.1";
const uint16_t destinationPort = 10001;


class SimpleSenderApp : public App {
  public:
	SimpleSenderApp();
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void mouseUp( MouseEvent event ) override;
	void mouseMove( MouseEvent event ) override;
	void update() override;
	void draw() override;
	
	ivec2 mCurrentMousePositon;
	
#if USE_UDP
	osc::SenderUdp mSender;
#else
	osc::SenderTcp mSender;
#endif
};

SimpleSenderApp::SimpleSenderApp()
: mSender( 10000, destinationHost, destinationPort )
{
}

void SimpleSenderApp::setup()
{
	mSender.bind();
#if ! USE_UDP
	mSender.connect();
#endif
	auto displaySize = getDisplay()->getSize();
	ivec2 otherScreenPos = ivec2( displaySize.x / 2, 0 );
	ivec2 halfScreenSize = ivec2( displaySize.x / 2, displaySize.y );
	
	setWindowPos( 0, 0 );
	setWindowSize( halfScreenSize );
	
	osc::Message msg( "/windowPos/1" );
	msg.append( otherScreenPos.x );
	msg.append( otherScreenPos.y );
	msg.append( halfScreenSize.x );
	msg.append( halfScreenSize.y );
	mSender.send( msg );
}

void SimpleSenderApp::mouseMove( cinder::app::MouseEvent event )
{
	mCurrentMousePositon = event.getPos();
	osc::Message msg( "/mousemove/1" );
	msg.append( mCurrentMousePositon.x );
	msg.append( mCurrentMousePositon.y );
	
	mSender.send( msg );
}

void SimpleSenderApp::mouseDown( MouseEvent event )
{
	osc::Message msg( "/mouseclick/1" );
	msg.append( true );
	
	mSender.send( msg );
}

void SimpleSenderApp::mouseUp( MouseEvent event )
{
	osc::Message msg( "/mouseclick/1" );
	msg.append( false );
	
	mSender.send( msg );
}

void SimpleSenderApp::update()
{
}

void SimpleSenderApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	gl::setMatricesWindow( getWindowSize() );
	gl::drawSolidCircle( mCurrentMousePositon, 100 );
}

CINDER_APP( SimpleSenderApp, RendererGl)
