#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/gl.h"

#include "CinderFFmpeg.h"

using namespace ci;
using namespace ci::app;
using namespace ph;
using namespace std;

class _TBOX_PREFIX_App : public App {
  public:
	void setup() override;

	void keyDown( KeyEvent event ) override;
	void fileDrop( FileDropEvent event ) override;

	void update() override;
	void draw() override;

	void loadMovieFile( const fs::path &path );

  private:
	gl::Texture2dRef   mFrameTexture;
	ffmpeg::MovieGlRef mMovie;
};

void _TBOX_PREFIX_App::setup()
{
	disableFrameRate();

	fs::path moviePath = getOpenFilePath();
	if( !moviePath.empty() )
		loadMovieFile( moviePath );
}

void _TBOX_PREFIX_App::keyDown( KeyEvent event )
{
	if( event.getChar() == 'f' ) {
		setFullScreen( !isFullScreen() );
	}
	else if( event.getChar() == 'o' ) {
		fs::path moviePath = getOpenFilePath();
		if( !moviePath.empty() )
			loadMovieFile( moviePath );
	}
}

void _TBOX_PREFIX_App::loadMovieFile( const fs::path &moviePath )
{
	try {
		// Load the movie and begin playing.
		mMovie.reset();
		mMovie = ffmpeg::MovieGl::create( moviePath );
		mMovie->play();
	}
	catch( const std::exception &exc ) {
		console() << "Unable to load the movie:" << exc.what() << std::endl;
		mMovie.reset();
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
	gl::clear();
	gl::color( Color::white() );

	if( mFrameTexture ) {
		const auto centeredRect = Rectf( mFrameTexture->getBounds() ).getCenteredFit( getWindowBounds(), true );
		gl::draw( mFrameTexture, centeredRect );
	}
}

CINDER_APP( _TBOX_PREFIX_App, RendererGl )
