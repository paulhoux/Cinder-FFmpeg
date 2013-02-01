#include "cinder/app/AppNative.h"
#include "cinder/audio/Output.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"

#include "CinderVideo.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class _TBOX_PREFIX_App : public AppNative {
public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
private:
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
};

void _TBOX_PREFIX_App::setup()
{
	//
	try {
	mShader = gl::GlslProg( loadAsset("framerender_vert.glsl"), loadAsset("framerender_frag.glsl") );
	}
	catch( const std::exception &e ) {
		console() << e.what() << std::endl;
	}

	std::string movieFilename = "V:/Animation/(2012) Kuifje en het Geheim van de Eenhoorn (HD)(NL).m2ts";

	if( mMovieDecoder.initialize( movieFilename ) )
	{
		mMovieDecoder.start();
		console() << "Playing " << movieFilename << std::endl;
	}
	else
	{
		mMovieDecoder.destroy();
        console() << "Failed to open " << movieFilename << std::endl;
	}

	mAudioClockOffset = 0.0;
	mAudioClock = 0.0;
	mVideoClock = 0.0;
}

void _TBOX_PREFIX_App::update()
{
	//while( mAudioRenderer->hasBufferSpace() )
	while(true)
	{
		AudioFrame audioFrame;
		if( mMovieDecoder.decodeAudioFrame(audioFrame) )
		{
			//mAudioRenderer->queueFrame(audioFrame);
		}
		else
			break;
	}
	
	mAudioClock = getElapsedSeconds() + mAudioClockOffset;

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
			if( videoFrame.getHeight() != mFrameHeight ||
				videoFrame.getWidth() != mFrameWidth )
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

void _TBOX_PREFIX_App::mouseDown( MouseEvent event )
{
}

CINDER_APP_NATIVE( _TBOX_PREFIX_App, RendererGl )
