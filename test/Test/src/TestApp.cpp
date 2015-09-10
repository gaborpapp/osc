#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"

#include "Sender.h"
#include "Receiver.h"

using namespace ci;
using namespace ci::app;
using namespace std;

struct TestStruct {
	int myInt;
	float myFloat;
	double myDouble;
};

class TestApp : public App {
  public:
	TestApp();
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
	
	osc::Sender		mSender;
	osc::Receiver	mReceiver;
};

TestApp::TestApp()
: App(), mSender(), mReceiver( 8080 )
{
	mReceiver.open(); 
	mReceiver.setListener( "/app/1",
						  []( const osc::Message &message ){
							  cout << "Integer: " << message.getInt( 0 ) << endl;
						  } );
	mReceiver.setListener( "/app/2",
						  []( const osc::Message &message ) {
							  cout << "String: " << message.getString( 0 ) << endl;
						  });
	mReceiver.setListener( "/app/3",
						  []( const osc::Message &message ) {
							  auto blob0 = message.getBlob( 0 );
							  TestStruct myStruct0( *reinterpret_cast<TestStruct*>( blob0.getData() ) );
							  cout << "Test Struct 0: " <<  endl <<
								"\tint: " << myStruct0.myInt << endl <<
								"\tfloat: " << myStruct0.myFloat << endl <<
								"\tdouble: " << myStruct0.myDouble << endl;
							  auto blob1 = message.getBlob( 1 );
							  TestStruct myStruct1( *reinterpret_cast<TestStruct*>( blob1.getData() ) );
							  cout << "Test Struct 1: " <<  endl <<
							  "\tint: " << myStruct1.myInt << endl <<
							  "\tfloat: " << myStruct1.myFloat << endl <<
							  "\tdouble: " << myStruct1.myDouble << endl;
						  });
}

void TestApp::setup()
{
	
}

void TestApp::mouseDown( MouseEvent event )
{
	osc::Message message( "/app/1" );
	message.append( 245 );
	mSender.send( message, asio::ip::udp::endpoint( asio::ip::address::from_string( "127.0.0.1" ), 8080 ) );
	osc::Message message2( "/app/2" );
	message2.append( std::string("testing") );
	mSender.send( message2, asio::ip::udp::endpoint( asio::ip::address::from_string( "127.0.0.1" ), 8080 ) );
	osc::Message message3( "/app/3" );
	TestStruct mTransmitStruct;
	mTransmitStruct.myInt = 45;
	mTransmitStruct.myFloat = 32.4f;
	mTransmitStruct.myDouble = 128.09;
	auto buffer = ci::Buffer::create( &mTransmitStruct, sizeof(TestStruct) );
	message3.appendBlob( *buffer );
	auto bufferFrom = message3.getBlob( 0 );
	mTransmitStruct.myInt = 33;
	mTransmitStruct.myFloat = 45.4f;
	mTransmitStruct.myDouble = 1.01;
	message3.appendBlob( &mTransmitStruct, sizeof(TestStruct) );
	mSender.send( message3, asio::ip::udp::endpoint( asio::ip::address::from_string( "127.0.0.1" ), 8080 ) );
}

void TestApp::update()
{
}

void TestApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( TestApp, RendererGl )
