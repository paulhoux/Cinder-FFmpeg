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

	void playVideo( const fs::path &path );
	void stopVideo();
private:
	bool			mIsPlaying;

	MovieDecoder	mMovieDecoder;

	int				mFrameWidth;
	int				mFrameHeight;
	Area			mFrameArea;

	double			mAudioClockOffset;
	double			mAudioClock;
	double			mVideoClock;

	gl::Texture		mYPlane;
	gl::Texture		mUPlane;
	gl::Texture		mVPlane;

	gl::GlslProg	mShader;

	Timer			mTimer;
};

void _TBOX_PREFIX_App::setup()
{
	// load and compile YUV-decoding shader
	try { mShader = gl::GlslProg( loadAsset("framerender_vert.glsl"), loadAsset("framerender_frag.glsl") ); }
	catch( const std::exception &e ) { console() << e.what() << std::endl; }
}

void _TBOX_PREFIX_App::update()
{
	// video does not run without decoding the audio, for some reason
	while(true)
	{
		AudioFrame audioFrame;
		if( mMovieDecoder.decodeAudioFrame(audioFrame) )
		{
		}
		else
			break;
	}
	
	mAudioClock = mTimer.getSeconds() + mAudioClockOffset;

	bool decoded = false;
	int count = 0;

	VideoFrame videoFrame;
	while( mVideoClock < mAudioClock && count < 10 )
	{
		++count;

		if( mMovieDecoder.decodeVideoFrame(videoFrame) )
		{
			mVideoClock = mMovieDecoder.getVideoClock();
			if( (mVideoClock - mAudioClock) > 1.0f )
				mAudioClockOffset = mVideoClock - mAudioClock;

			// resize
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

			decoded = true;
		}
		else
		{
			break;
		}
	}

	if( decoded ) 
	{
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

	for( size_t i=0; i<event.getNumFiles() && !mIsPlaying; ++i )
		playVideo( event.getFile(i) );
}

void _TBOX_PREFIX_App::playVideo( const fs::path &path )
{
	stopVideo();

	if( mMovieDecoder.initialize( path.generic_string() ) )
	{
		console() << "Playing " << path.generic_string() << std::endl;

		mMovieDecoder.start();
		mIsPlaying = true;		

		// initialize time
		mAudioClockOffset = 0.0;
		mAudioClock = 0.0;
		mVideoClock = 0.0;

		mTimer.start();
	}
	else
	{
        console() << "Failed to open " << path.generic_string() << std::endl;

		stopVideo();
	}
}

void _TBOX_PREFIX_App::stopVideo()
{
	if( mIsPlaying )
	{
		mIsPlaying = false;
		mMovieDecoder.destroy();
	}
}

CINDER_APP_NATIVE( _TBOX_PREFIX_App, RendererGl )
