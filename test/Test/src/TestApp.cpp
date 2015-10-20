#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"

#include "Osc.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#define TEST_UDP 0

struct TestStruct {
	int myInt;
	float myFloat;
	double myDouble;
};

class TestApp : public App {
  public:
	TestApp();
	void update() override;
	
#if defined TEST_UDP
	void sendMessageUdp( const osc::Message &message );
	osc::ReceiverUdp	mReceiver;
	osc::SenderUdp		mSender;
#else
	void sendMessageTcp( const osc::Message &message );
	osc::ReceiverTcp	mReceiver;
	osc::SenderTcp		mSender;
#endif
	osc::Message		mMessage;
	
	bool mIsConnected = false;
};

TestApp::TestApp()
: App(), mReceiver( 10000 ), mSender( 12345, "127.0.0.1", 10000 )
{	
	mReceiver.bind();
	mReceiver.listen();
	mReceiver.setListener( "/app/?",
	[]( const osc::Message &message ){
		cout << "Integer: " << message[0].int32() << endl;
	} );
	mReceiver.setListener( "/app/?",
	[]( const osc::Message &message ) {
		cout << "String: " << message[1].string() << endl;
	});
	mReceiver.setListener( "/app/*",
	[]( const osc::Message &message ) {
		auto blob0 = message[2].blob();
		TestStruct myStruct0( *reinterpret_cast<TestStruct*>( blob0.getData() ) );
		cout << "Test Struct 0: " <<  endl <<
		"\tint: " << myStruct0.myInt << endl <<
		"\tfloat: " << myStruct0.myFloat << endl <<
		"\tdouble: " << myStruct0.myDouble << endl;
		auto blob1 = message.getArgBlob( 3 );
		TestStruct myStruct1( *reinterpret_cast<TestStruct*>( blob1.getData() ) );
		cout << "Test Struct 1: " <<  endl <<
		"\tint: " << myStruct1.myInt << endl <<
		"\tfloat: " << myStruct1.myFloat << endl <<
		"\tdouble: " << myStruct1.myDouble << endl;
	});
	mReceiver.setListener( "/app/?",
	[]( const osc::Message &message ) {
		cout << "Character: " << message[4].character() << endl;
	});
	mReceiver.setListener( "/app/?",
	[]( const osc::Message &message ) {
		cout << "Midi: " << message << endl;
	});
	mReceiver.setListener( "/app/?",
	[]( const osc::Message &message ) {
		cout << "Int64: " << message[6].int64() << endl;
	});
	mReceiver.setListener( "/app/?",
	[]( const osc::Message &message ) {
		cout << "Float: " << message[7].flt() << endl;
	});
	mReceiver.setListener( "/app/?",
	[]( const osc::Message &message ) {
		cout << "Double: " << message[8].dbl() << endl;
	});
	mReceiver.setListener( "/app/?",
	[]( const osc::Message &message ) {
		cout << "Boolean: " << message[9].boolean() << endl;
	});
	mReceiver.setListener( "/app/10",
	[&]( const osc::Message &message ) {
		cout << "Address: " << message.getAddress() << endl;
		cout << "Integer: " << message[0].int32() << endl;
		cout << "String: " << message[1].string() << endl;
		auto blob0 = message[2].blob();
		TestStruct myStruct0( *reinterpret_cast<TestStruct*>( blob0.getData() ) );
		cout << "Test Struct 0: " <<  endl <<
		"\tint: " << myStruct0.myInt << endl <<
		"\tfloat: " << myStruct0.myFloat << endl <<
		"\tdouble: " << myStruct0.myDouble << endl;
		auto blob1 = message.getArgBlob( 3 );
		TestStruct myStruct1( *reinterpret_cast<TestStruct*>( blob1.getData() ) );
		cout << "Test Struct 1: " <<  endl <<
		"\tint: " << myStruct1.myInt << endl <<
		"\tfloat: " << myStruct1.myFloat << endl <<
		"\tdouble: " << myStruct1.myDouble << endl;
		cout << "Character: " << message[4].character() << endl;
		osc::ByteArray<4> midi;
		message[5].midi( &midi[0], &midi[1], &midi[2], &midi[3] );
		cout << "Midi: " <<  (int)midi[0] << " " <<  (int)midi[1] << " " <<  (int)midi[2] << " " <<  (int)midi[3] << endl;
		cout << "Int64: " << message[6].int64() << endl;
		cout << "Float: " << message[7].flt() << endl;
		cout << "Double: " << message[8].dbl() << endl;
		cout << "Boolean: " << message[9].boolean() << endl;
		auto messagesTheSame = (mMessage == message);
		cout << "Messages are the same: " << (messagesTheSame ? "true" : "false") << endl;
	});
}

void TestApp::update()
{
	if( ! mIsConnected ) {
		mSender.bind();
#if ! defined TEST_UDP
		mSender.connect();
#endif
		mIsConnected = true;
	}
	else {
		
		static int i = 245;
		i++;
		static std::string test("testing");
		test += ".";
		static TestStruct mTransmitStruct{ 0, 0, 0 };
		mTransmitStruct.myInt += 45;
		mTransmitStruct.myFloat += 32.4f;
		mTransmitStruct.myDouble += 128.09;
		
		osc::Message message( "/app/10" );
		message.append( i );
		message.append( test );
		
		auto buffer = ci::Buffer::create( &mTransmitStruct, sizeof(TestStruct) );
		message.appendBlob( *buffer );
		mTransmitStruct.myInt += 33;
		mTransmitStruct.myFloat += 45.4f;
		mTransmitStruct.myDouble += 1.01;
		message.appendBlob( &mTransmitStruct, sizeof(TestStruct) );
		
		message.append( 'X' );
		message.appendMidi( 10, 20, 30, 40 );
		message.append( int64_t( 1000000000000 ) );
		message.append( 1.4f );
		message.append( 28.4 );
		message.append( true );
		
#if defined TEST_UDP
		sendMessageUdp( message );
#else
		sendMessageTcp( message );
#endif
		mMessage = std::move( message );
//		cout << mMessage << endl;
	}
}

#if defined TEST_UDP
void TestApp::sendMessageUdp( const osc::Message &message )
{
	if( getElapsedFrames() % 2 == 0 ) {
		cout << "broadcasting" << endl;
		mSender.broadcast( message );
	}
	else {
		cout << "sending regularly" << endl;
		mSender.send( message );
	}
}
#else
void TestApp::sendMessageTcp( const osc::Message &message )
{
	mSender.send( message );
}
#endif

CINDER_APP( TestApp, RendererGl, []( App::Settings *settings ) {
	settings->setConsoleWindowEnabled();
})
