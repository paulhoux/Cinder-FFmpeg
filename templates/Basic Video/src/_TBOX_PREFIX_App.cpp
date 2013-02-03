#include "cinder/app/AppNative.h"
#include "cinder/audio/Output.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/Timer.h"

#include "CinderVideo.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class _TBOX_PREFIX_App : public AppNative {
public:
	void setup();
	void update();
	void draw();

	void fileDrop( FileDropEvent event );

	bool playVideo( const fs::path &path );
	void stopVideo();
private:
	AudioRenderer*	mAudioRenderer;
	MovieDecoder	mMovieDecoder;

	int				mFrameWidth;
	int				mFrameHeight;
	Area			mFrameArea;

	double			mAudioClock;
	double			mVideoClock;

	gl::Texture		mYPlane;
	gl::Texture		mUPlane;
	gl::Texture		mVPlane;

	gl::GlslProg	mShader;
};

void _TBOX_PREFIX_App::setup()
{
	// initialize OpenAL audio renderer
	mAudioRenderer = AudioRendererFactory::create(AudioRendererFactory::OPENAL_OUTPUT);
	
	// load and compile YUV-decoding shader
	try { mShader = gl::GlslProg( loadAsset("framerender_vert.glsl"), loadAsset("framerender_frag.glsl") ); }
	catch( const std::exception &e ) { console() << e.what() << std::endl; }
}

void _TBOX_PREFIX_App::update()
{
	// decode audio
	while( mAudioRenderer->hasBufferSpace() )
	{
		AudioFrame audioFrame;
		if( mMovieDecoder.decodeAudioFrame(audioFrame) )
		{
			mAudioRenderer->queueFrame(audioFrame);
		}
		else
			break;
	}
	
	mAudioRenderer->flushBuffers();
	mAudioClock = mAudioRenderer->getCurrentPts();

	// decode video
	bool hasVideo = false;
	int count = 0;

	VideoFrame videoFrame;
	while( mVideoClock < mAudioClock && count < 10 )
	{
		++count;

		if( mMovieDecoder.decodeVideoFrame(videoFrame) )
		{
			mVideoClock = videoFrame.getPts();

			hasVideo = true;
		}
		else
			break;
	}

	if( hasVideo ) 
	{
		// resize textures if needed
		if( videoFrame.getHeight() != mFrameHeight || videoFrame.getWidth() != mFrameWidth )
		{
			mFrameWidth = videoFrame.getWidth();
			mFrameHeight = videoFrame.getHeight();
			mFrameArea = Area(0, 0, mFrameWidth, mFrameHeight);

			gl::Texture::Format fmt;
			fmt.setInternalFormat( GL_LUMINANCE );	

			mYPlane = gl::Texture( videoFrame.getYLineSize(), mFrameHeight, fmt );
			mUPlane = gl::Texture( videoFrame.getULineSize(), mFrameHeight / 2, fmt );
			mVPlane = gl::Texture( videoFrame.getVLineSize(), mFrameHeight / 2, fmt );
		}

		// upload texture data
		if( mYPlane ) {
			glBindTexture( mYPlane.getTarget(), mYPlane.getId() );
			glTexSubImage2D( mYPlane.getTarget(), 0, 
				0, 0, mYPlane.getWidth(), mYPlane.getHeight(), 
				GL_LUMINANCE, GL_UNSIGNED_BYTE, videoFrame.getYPlane() );
		}
		
		if( mUPlane ) {
			glBindTexture( mUPlane.getTarget(), mUPlane.getId() );
			glTexSubImage2D( mUPlane.getTarget(), 0, 
				0, 0, mUPlane.getWidth(), mUPlane.getHeight(), 
				GL_LUMINANCE, GL_UNSIGNED_BYTE, videoFrame.getUPlane() );
		}
		
		if( mVPlane ) {
			glBindTexture( mVPlane.getTarget(), mVPlane.getId() );
			glTexSubImage2D( mVPlane.getTarget(), 0, 
				0, 0, mVPlane.getWidth(), mVPlane.getHeight(), 
				GL_LUMINANCE, GL_UNSIGNED_BYTE, videoFrame.getVPlane() );
		}

		glBindTexture( GL_TEXTURE_2D, 0 );
	}
}

void _TBOX_PREFIX_App::draw()
{
	// 
	gl::clear();

	//
	if( ! mShader ) return;
	if( ! mYPlane ) return;
	if( ! mUPlane ) return;
	if( ! mVPlane ) return;

	//
	mShader.bind();
	mShader.uniform("texUnit1", 0);
	mShader.uniform("texUnit2", 1);
	mShader.uniform("texUnit3", 2);
	mShader.uniform("brightness", 0.0f);
	mShader.uniform("gamma", 1.0f);
	mShader.uniform("contrast", 1.0f);

	//
	Area area = Area::proportionalFit( mFrameArea, getWindowBounds(), true, true );

	//
	mUPlane.bind(1);
	mVPlane.bind(2);
	gl::draw( mYPlane, mFrameArea, area );

	//
	mShader.unbind();
}

void _TBOX_PREFIX_App::fileDrop( FileDropEvent event )
{
	stopVideo();

	// play the first video file found
	bool isPlaying = false;
	for( size_t i=0; i<event.getNumFiles() && !isPlaying; ++i )
		isPlaying = playVideo( event.getFile(i) );
}

bool _TBOX_PREFIX_App::playVideo( const fs::path &path )
{
	stopVideo();

	if( mMovieDecoder.initialize( path.generic_string() ) )
	{
		mAudioRenderer->setFormat( mMovieDecoder.getAudioFormat() );

		mMovieDecoder.start();

		mVideoClock = mMovieDecoder.getVideoClock();
		mAudioClock = mMovieDecoder.getAudioClock();

		return true;
	}
	else
	{
        console() << "Failed to open " << path.generic_string() << std::endl;
	}

	return false;
}

void _TBOX_PREFIX_App::stopVideo()
{
	mMovieDecoder.stop();
	mAudioRenderer->stop();

	mVideoClock = 0.0;
	mAudioClock = 0.0;

	mMovieDecoder.destroy();
}

CINDER_APP_NATIVE( _TBOX_PREFIX_App, RendererGl )
