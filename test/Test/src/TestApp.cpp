#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class TestApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void TestApp::setup()
{
}

void TestApp::mouseDown( MouseEvent event )
{
}

void TestApp::update()
{
}

void TestApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( TestApp, RendererGl )
