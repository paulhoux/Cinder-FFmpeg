#include "CinderFFmpeg.h"
#include "cinder/app/AppBasic.h"

using namespace ci;

namespace ph { namespace ffmpeg {

MovieGl::MovieGl(void) :
	mWidth(0),
	mHeight(0),
	mDuration(0.0f)
{
	app::console() << "Constructor called" << std::endl;
	// initialize OpenAL audio renderer
	mAudioRenderer = AudioRendererFactory::create(AudioRendererFactory::OPENAL_OUTPUT);
}

MovieGl::MovieGl( const fs::path &path ) :
	mWidth(0),
	mHeight(0),
	mDuration(0.0f)
{
	app::console() << "Constructor called" << std::endl;
	//stop();

	mMovieDecoder = new MovieDecoder( path.generic_string() );
	if(!mMovieDecoder->isInitialized())
		throw std::logic_error("MovieDecoder: Failed to initialize");

	//
	initializeShader();
	
	// initialize OpenAL audio renderer
	mAudioRenderer = AudioRendererFactory::create(AudioRendererFactory::OPENAL_OUTPUT);
	mAudioRenderer->setFormat( mMovieDecoder->getAudioFormat() );
}

MovieGl::~MovieGl()
{
	app::console() << "Deconstructor called" << std::endl;
	stop();

	if(mMovieDecoder)
	{
		delete mMovieDecoder;
		mMovieDecoder = nullptr;
	}

	if(mAudioRenderer) 
		delete mAudioRenderer;

	mAudioRenderer = NULL;
}

const gl::Texture MovieGl::getTexture() 
{
	if( !mMovieDecoder->isInitialized() )
		return mTexture;
	
	// decode audio 
	while( mAudioRenderer->hasBufferSpace() )
	{
		AudioFrame audioFrame;
		if( mMovieDecoder->decodeAudioFrame(audioFrame) )
		{
			mAudioRenderer->queueFrame(audioFrame);
		}
		else
			break;
	}
	
	// 
	mAudioRenderer->flushBuffers();
	mAudioClock = mAudioRenderer->getCurrentPts();

	// decode video
	bool hasVideo = false;
	int count = 0;

	VideoFrame videoFrame;
	while( mVideoClock < mAudioClock && count < 10 )
	{
		++count;

		if( mMovieDecoder->decodeVideoFrame(videoFrame) )
		{
			mVideoClock = videoFrame.getPts();

			hasVideo = true;
		}
		else
			break;
	}

	if(count > 1)
		app::console() << "Skipped " << (count-1) << " frames!" << std::endl;

	if( hasVideo ) 
	{
		// resize textures if needed
		if( !mYPlane || !mUPlane || !mVPlane || !mFbo || videoFrame.getHeight() != mHeight || videoFrame.getWidth() != mWidth )
		{
			mWidth = videoFrame.getWidth();
			mHeight = videoFrame.getHeight();

			{
				gl::Texture::Format fmt;
				fmt.setInternalFormat( GL_LUMINANCE );	

				mYPlane = gl::Texture( videoFrame.getYLineSize(), mHeight, fmt );
				mUPlane = gl::Texture( videoFrame.getULineSize(), mHeight / 2, fmt );
				mVPlane = gl::Texture( videoFrame.getVLineSize(), mHeight / 2, fmt );
			}

			{
				gl::Fbo::Format fmt;
				fmt.enableColorBuffer(true);
				fmt.enableDepthBuffer(false);
				fmt.setTarget( GL_TEXTURE_RECTANGLE_ARB );
				fmt.setColorInternalFormat( GL_RGB );

				mFbo = gl::Fbo( mWidth, mHeight, fmt );
			}
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

		// render to FBO
		mFbo.bindFramebuffer();
		{
			// set viewport and matrices
			glPushAttrib( GL_VIEWPORT_BIT );
			gl::setViewport( getBounds() );
			gl::pushMatrices();
			gl::setMatricesWindow( getSize(), false );

			// bind and initialize shader
			mShader.bind();
			mShader.uniform("texUnit1", 0);
			mShader.uniform("texUnit2", 1);
			mShader.uniform("texUnit3", 2);
			mShader.uniform("brightness", 0.0f);
			mShader.uniform("gamma", 1.0f);
			mShader.uniform("contrast", 1.0f);

			// render video
			mUPlane.bind(1);
			mVPlane.bind(2);
			gl::draw( mYPlane );

			// unbind shader
			mShader.unbind();

			// restore viewport and matrices
			gl::popMatrices();
			glPopAttrib();
		}
		mFbo.unbindFramebuffer();

		mTexture = gl::Texture( mFbo.getTexture() );
	}	

	return mTexture;
}

bool MovieGl::checkNewFrame()
{
	if( !mAudioRenderer )
		return false;

	if( ! mMovieDecoder->isInitialized() )
		return false;

	//
	return ( mMovieDecoder->getVideoClock() < mAudioRenderer->getCurrentPts() );	
}

float MovieGl::getCurrentTime() const
{
	return static_cast<float>( mMovieDecoder->getVideoClock() );
}

bool MovieGl::isPlaying() const
{
	return true; // TODO
}

bool MovieGl::isDone() const
{
	return false; // TODO
}

void MovieGl::play()
{
	if( mMovieDecoder->isInitialized() ) 
	{
		mMovieDecoder->start();

		mWidth = static_cast<int32_t>( mMovieDecoder->getFrameWidth() );
		mHeight = static_cast<int32_t>( mMovieDecoder->getFrameHeight() );
		mDuration = mMovieDecoder->getDuration();

		mVideoClock = mMovieDecoder->getVideoClock();
		mAudioClock = mMovieDecoder->getAudioClock();
	}
}

void MovieGl::stop()
{	
	if( ! mAudioRenderer )
		return;

	if( ! mMovieDecoder->isInitialized() )
		return;

	mMovieDecoder->stop();
	mAudioRenderer->stop();

	mVideoClock = 0.0;
	mAudioClock = 0.0;

	//mMovieDecoder->destroy();
}

void MovieGl::initializeShader()
{
	// compile YUV-decoding shader
	const char *vs = 
		"#version 120\n"
		"\n"
		"void main(void)\n"
		"{\n"
		"	gl_Position = ftransform();\n"
		"	gl_TexCoord[0] = gl_MultiTexCoord0;\n"
		"	gl_TexCoord[1] = gl_MultiTexCoord0;\n"
		"}";

	const char *fs = 
		"#version 120\n"
		"uniform sampler2D texUnit1, texUnit2, texUnit3;\n"
		"uniform float brightness;\n"
		"uniform float gamma;\n"
		"uniform float contrast;\n"
		"\n"
		"void main(void)\n"
		"{\n"
		"	vec3 yuv;\n"
		"	yuv.x = (texture2D(texUnit1, gl_TexCoord[0].st).x + brightness);\n"
		"	yuv.y = texture2D(texUnit2, gl_TexCoord[1].st).x;\n"
		"	yuv.z = texture2D(texUnit3, gl_TexCoord[1].st).x;\n"
		"\n"
		"	gl_FragColor.x = 1.164 * (yuv.x - 16.0/256.0) + 1.596 * (yuv.z - 128.0/256.0);\n"
		"	gl_FragColor.y = 1.164 * (yuv.x - 16.0/256.0) - 0.813 * (yuv.z - 128.0/256.0) - 0.391 * (yuv.y - 128.0/256.0);\n"
		"	gl_FragColor.z = 1.164 * (yuv.x - 16.0/256.0)                                 + 2.018 * (yuv.y - 128.0/256.0);\n"
		"\n"    
		"	gl_FragColor.xyz = (gl_FragColor.xyz - vec3(0.5, 0.5, 0.5)) * contrast + vec3(0.5, 0.5, 0.5);\n"
		"\n"
		"	gl_FragColor.x = pow(gl_FragColor.x, gamma);\n"
		"	gl_FragColor.y = pow(gl_FragColor.y, gamma);\n"
		"	gl_FragColor.z = pow(gl_FragColor.z, gamma);\n"
		"}";

	mShader = gl::GlslProg( vs, fs );
}

} } // namespace ph::ffmpeg