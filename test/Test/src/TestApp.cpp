#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"

#include "Sender.h"
#include "Receiver.h"
#include "Osc.h"

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
	
	osc::ReceiverTcp	mReceiver;
	osc::SenderTcp		mSender;
	
	bool mIsConnected = false;
};

TestApp::TestApp()
: App(), mReceiver( 10000 ), mSender( 12345, "127.0.0.1", 10000 )
{
	osc::SenderUdpTemp mSenderUdp( 10000, "", 124 );
	
	mReceiver.listen();
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
	
}

void TestApp::update()
{
	if( ! mIsConnected ) {
		mSender.connect();
		mIsConnected = true;
	}
	else {
		static int i = 245;
		i++;
		osc::Message message( "/app/1" );
		message.append( i );
		mSender.send( message );
		osc::Message message2( "/app/2" );
		static std::string test("testing");
		test += ".";
		message2.append( test );
		mSender.send( message2 );
		osc::Message message3( "/app/3" );
		static TestStruct mTransmitStruct{ 0, 0, 0 };
		mTransmitStruct.myInt += 45;
		mTransmitStruct.myFloat += 32.4f;
		mTransmitStruct.myDouble += 128.09;
		auto buffer = ci::Buffer::create( &mTransmitStruct, sizeof(TestStruct) );
		message3.appendBlob( *buffer );
		auto bufferFrom = message3.getBlob( 0 );
		mTransmitStruct.myInt += 33;
		mTransmitStruct.myFloat += 45.4f;
		mTransmitStruct.myDouble += 1.01;
		message3.appendBlob( &mTransmitStruct, sizeof(TestStruct) );
		//		cout << message3 << endl;
		mSender.send( message3 );
	}
}

void TestApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( TestApp, RendererGl )
