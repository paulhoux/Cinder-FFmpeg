#include "cinder/app/AppNative.h"
#include "cinder/audio/Output.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/Timer.h"

#include "CinderFFmpeg.h"

using namespace ci;
using namespace ci::app;
using namespace ph;
using namespace std;

class _TBOX_PREFIX_App : public AppNative {
public:
	void setup();
	
	void keyDown( KeyEvent event );
	void fileDrop( FileDropEvent event );

	void update();
	void draw();

	void loadMovieFile( const fs::path &path );
private:
	gl::Texture			mFrameTexture;
	ffmpeg::MovieGlRef	mMovie;
};

void _TBOX_PREFIX_App::setup()
{
	fs::path moviePath = getOpenFilePath();
	if( ! moviePath.empty() )
		loadMovieFile( moviePath );
}

void _TBOX_PREFIX_App::keyDown( KeyEvent event )
{
	if( event.getChar() == 'f' ) {
		setFullScreen( ! isFullScreen() );
	}
	else if( event.getChar() == 'o' ) {
		fs::path moviePath = getOpenFilePath();
		if( ! moviePath.empty() )
			loadMovieFile( moviePath );
	}
	//else if( event.getChar() == '1' )
	//	mMovie->setRate( 0.5f );
	//else if( event.getChar() == '2' )
	//	mMovie->setRate( 2 );
}

void _TBOX_PREFIX_App::loadMovieFile( const fs::path &moviePath )
{
	try {
		// load up the movie, set it to loop, and begin playing
		mMovie.reset();
		mMovie = ffmpeg::MovieGl::create( moviePath );
		//mMovie->setLoop();
		mMovie->play();
		
		/*// create a texture for showing some info about the movie
		TextLayout infoText;
		infoText.clear( ColorA( 0.2f, 0.2f, 0.2f, 0.5f ) );
		infoText.setColor( Color::white() );
		infoText.addCenteredLine( moviePath.filename().string() );
		infoText.addLine( toString( mMovie->getWidth() ) + " x " + toString( mMovie->getHeight() ) + " pixels" );
		infoText.addLine( toString( mMovie->getDuration() ) + " seconds" );
		infoText.addLine( toString( mMovie->getNumFrames() ) + " frames" );
		infoText.addLine( toString( mMovie->getFramerate() ) + " fps" );
		infoText.setBorder( 4, 2 );
		mInfoTexture = gl::Texture( infoText.render( true ) );*/
	}
	catch( ... ) {
		console() << "Unable to load the movie." << std::endl;
		mMovie.reset();
		//mInfoTexture.reset();
	}

	mFrameTexture.reset();
}

void _TBOX_PREFIX_App::fileDrop( FileDropEvent event )
{
	loadMovieFile( event.getFile( 0 ) );
}

void _TBOX_PREFIX_App::update()
{
	if( mMovie )
		mFrameTexture = mMovie->getTexture();
}

void _TBOX_PREFIX_App::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	//gl::enableAlphaBlending();
	gl::color( Color::white() );

	if( mFrameTexture ) {
		Rectf centeredRect = Rectf( mFrameTexture.getBounds() ).getCenteredFit( getWindowBounds(), true );
		gl::draw( mFrameTexture, centeredRect  );
	}

	/*if( mInfoTexture ) {
		glDisable( GL_TEXTURE_RECTANGLE_ARB );
		gl::draw( mInfoTexture, Vec2f( 20, getWindowHeight() - 20 - mInfoTexture.getHeight() ) );
	}*/
}

CINDER_APP_NATIVE( _TBOX_PREFIX_App, RendererGl )
