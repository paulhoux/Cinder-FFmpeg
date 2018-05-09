#include "CinderFFmpeg.h"
#include "cinder/app/App.h"
#include "cinder/gl/draw.h"
#include "cinder/gl/scoped.h"

using namespace ci;

namespace ph {
namespace ffmpeg {

MovieGl::MovieGl( const fs::path &path )
    : mWidth( 0 )
    , mHeight( 0 )
    , mDuration( 0.0f )
    , mAudioRenderer( nullptr )
    , mMovieDecoder( nullptr )
    , mVideoClock( 0.0 )
{
	mMovieDecoder = std::make_unique<MovieDecoder>( path.generic_string() );
	if( !mMovieDecoder->isInitialized() )
		throw std::logic_error( "MovieDecoder: Failed to initialize" );

	// initialize OpenAL audio renderer
	if( !mAudioRenderer ) {
		mAudioRenderer = std::unique_ptr<AudioRenderer>( AudioRendererFactory::create( AudioRendererFactory::OPENAL_OUTPUT ) );
		mAudioRenderer->setFormat( mMovieDecoder->getAudioFormat() );
	}

	//
	initializeShader();
}

MovieGl::~MovieGl()
{
	stop();
}

const gl::Texture2dRef &MovieGl::getTexture()
{
	if( !mMovieDecoder->isInitialized() )
		return mTexture;

	// decode audio
	while( mAudioRenderer->hasBufferSpace() ) {
		AudioFrame audioFrame;
		if( mMovieDecoder->decodeAudioFrame( audioFrame ) )
			mAudioRenderer->queueFrame( audioFrame );
		else
			break;
	}

	//
	mAudioRenderer->flushBuffers();

	// decode video
	bool hasVideo = false;
	int  count = 0;

	VideoFrame videoFrame;
	while( mVideoClock < mAudioRenderer->getCurrentPts() && count++ < 10 ) {
		if( mMovieDecoder->decodeVideoFrame( videoFrame ) ) {
			mVideoClock = videoFrame.getPts();
			hasVideo = true;
		}
		else
			break;
	}

	if( hasVideo ) {
		// resize textures if needed
		if( !mYPlane || !mUPlane || !mVPlane || !mFbo || videoFrame.getHeight() != mHeight || videoFrame.getWidth() != mWidth ) {
			mWidth = videoFrame.getWidth();
			mHeight = videoFrame.getHeight();

			{
				const auto fmt = gl::Texture2d::Format().internalFormat( GL_RED ).swizzleMask( GL_RED, GL_RED, GL_RED, GL_ONE );

				mYPlane = gl::Texture2d::create( videoFrame.getYLineSize(), mHeight, fmt );
				mUPlane = gl::Texture2d::create( videoFrame.getULineSize(), mHeight / 2, fmt );
				mVPlane = gl::Texture2d::create( videoFrame.getVLineSize(), mHeight / 2, fmt );
			}

			{
				const auto tfmt = gl::Texture2d::Format().target( GL_TEXTURE_RECTANGLE_ARB ); // .internalFormat( GL_RGB );
				const auto fmt = gl::Fbo::Format().colorTexture( tfmt );

				mFbo = gl::Fbo::create( mWidth, mHeight, fmt );
			}
		}

		// upload texture data
		if( mYPlane ) {
			gl::ScopedTextureBind scpTex0( mYPlane, 0 );
			glTexSubImage2D( mYPlane->getTarget(), 0, 0, 0, mYPlane->getWidth(), mYPlane->getHeight(), mYPlane->getInternalFormat(), GL_UNSIGNED_BYTE, videoFrame.getYPlane() );
		}

		if( mUPlane ) {
			gl::ScopedTextureBind scpTex0( mUPlane, 0 );
			glTexSubImage2D( mUPlane->getTarget(), 0, 0, 0, mUPlane->getWidth(), mUPlane->getHeight(), mUPlane->getInternalFormat(), GL_UNSIGNED_BYTE, videoFrame.getUPlane() );
		}

		if( mVPlane ) {
			gl::ScopedTextureBind scpTex0( mVPlane, 0 );
			glTexSubImage2D( mVPlane->getTarget(), 0, 0, 0, mVPlane->getWidth(), mVPlane->getHeight(), mVPlane->getInternalFormat(), GL_UNSIGNED_BYTE, videoFrame.getVPlane() );
		}

		// render to FBO
		{
			gl::ScopedFramebuffer scpFbo( mFbo );

			// set viewport and matrices
			gl::ScopedViewport scpViewport( getSize() );
			gl::ScopedMatrices scpMatrices;
			gl::setMatricesWindow( getSize(), false );

			// bind and initialize shader
			gl::ScopedGlslProg scpGlsl( mShader );
			mShader->uniform( "texUnit1", 0 );
			mShader->uniform( "texUnit2", 1 );
			mShader->uniform( "texUnit3", 2 );
			mShader->uniform( "brightness", 0.0f );
			mShader->uniform( "gamma", vec3( 1.0f ) );
			mShader->uniform( "contrast", 1.0f );

			// render video
			gl::ScopedTextureBind scpTex0( mYPlane, 0 );
			gl::ScopedTextureBind scpTex1( mUPlane, 1 );
			gl::ScopedTextureBind scpTex2( mVPlane, 2 );
			gl::drawSolidRect( mFbo->getBounds() );
		}

		mTexture = mFbo->getColorTexture();
	}

	return mTexture;
}

bool MovieGl::checkNewFrame() const
{
	if( !mAudioRenderer )
		return false;

	if( !mMovieDecoder->isInitialized() )
		return false;

	//
	return ( mMovieDecoder->getVideoClock() < mAudioRenderer->getCurrentPts() );
}

float MovieGl::getCurrentTime() const
{
	return static_cast<float>( mMovieDecoder->getVideoClock() );
}

float MovieGl::getFramerate() const
{
	return static_cast<float>( mMovieDecoder->getFramesPerSecond() );
}

uint64_t MovieGl::getNumFrames() const
{
	return mMovieDecoder->getNumberOfFrames();
}

bool MovieGl::isPlaying() const
{
	return mMovieDecoder->isPlaying();
}

bool MovieGl::isDone() const
{
	return mMovieDecoder->isDone();
}

void MovieGl::play()
{
	if( !mMovieDecoder->isInitialized() )
		return;

	mMovieDecoder->start();

	mWidth = static_cast<int32_t>( mMovieDecoder->getFrameWidth() );
	mHeight = static_cast<int32_t>( mMovieDecoder->getFrameHeight() );
	mDuration = mMovieDecoder->getDuration();
}

void MovieGl::stop() const
{
	if( !mAudioRenderer )
		return;

	if( !mMovieDecoder->isInitialized() )
		return;

	mMovieDecoder->stop();
	mAudioRenderer->stop();
}

void MovieGl::seekToTime( float seconds )
{
	if( !mAudioRenderer )
		return;

	if( !mMovieDecoder->isInitialized() )
		return;

	mAudioRenderer->clearBuffers();
	mMovieDecoder->seekToTime( double( seconds ) );
	mAudioRenderer->play();

	mTexture.reset();
	mVideoClock = 0.0;
}

void MovieGl::seekToFrame( int frame )
{
	if( !mAudioRenderer )
		return;

	if( !mMovieDecoder->isInitialized() )
		return;

	mAudioRenderer->clearBuffers();
	mMovieDecoder->seekToFrame( uint32_t( frame ) );
	mAudioRenderer->play();

	mTexture.reset();
	mVideoClock = 0.0;
}

void MovieGl::seekToStart()
{
	if( !mAudioRenderer )
		return;

	if( !mMovieDecoder->isInitialized() )
		return;

	mAudioRenderer->clearBuffers();
	mMovieDecoder->seekToTime( 0.0 );
	mAudioRenderer->play();

	mTexture.reset();
	mVideoClock = 0.0;
}

void MovieGl::seekToEnd()
{
	if( !mAudioRenderer )
		return;

	if( !mMovieDecoder->isInitialized() )
		return;

	mAudioRenderer->clearBuffers();
	mMovieDecoder->seekToTime( double( mMovieDecoder->getDuration() ) );
	mAudioRenderer->play();

	mTexture.reset();
	mVideoClock = 0.0;
}

void MovieGl::initializeShader()
{
	// compile YUV-decoding shader
	const char *vs =
	    R"(#version 150
		
		uniform mat4 ciModelViewProjection;
		
		in vec4 ciPosition;
		in vec2 ciTexCoord0;

		out vec2 vertTexCoord0;

		void main(void)
		{
			vertTexCoord0 = ciTexCoord0;
			gl_Position = ciModelViewProjection * ciPosition;
		})";

	const char *fs =
	    R"(#version 150

		uniform sampler2D texUnit1, texUnit2, texUnit3;
		uniform float brightness;
		uniform float contrast;
		uniform vec3  gamma;

		in vec2 vertTexCoord0;

		out vec4 fragColor;

		void main(void)
		{
			vec3 yuv;
			yuv.x = texture(texUnit1, vertTexCoord0.st).x - 16.0/256.0 + brightness;
			yuv.y = texture(texUnit2, vertTexCoord0.st).x - 128.0/256.0;
			yuv.z = texture(texUnit3, vertTexCoord0.st).x - 128.0/256.0;

			fragColor.r = dot(yuv, vec3(1.164,  0.000,  1.596)) - 0.5;
			fragColor.g = dot(yuv, vec3(1.164, -0.391, -0.813)) - 0.5;
			fragColor.b = dot(yuv, vec3(1.164,  2.018,  0.000)) - 0.5;
			fragColor.a = 1.0;

			fragColor.rgb = fragColor.rgb * contrast + vec3(0.5);
			fragColor.rgb = pow(fragColor.rgb, gamma);
		})";

	try {
		mShader = gl::GlslProg::create( vs, fs );
	}
	catch( const std::exception &e ) {
		app::console() << e.what() << std::endl;
	}
}

} // namespace ffmpeg
} // namespace ph
