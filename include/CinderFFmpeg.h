#pragma once

#include "cinder/Area.h"
#include "cinder/Cinder.h"
#include "cinder/Filesystem.h"
#include "cinder/Vector.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"

#include "audiorenderer/audioframe.h"
#include "audiorenderer/audiorenderer.h"
#include "audiorenderer/audiorendererfactory.h"

#include "movierenderer/moviedecoder.h"

//

namespace ph {
namespace ffmpeg {

typedef std::shared_ptr<class MovieGl> MovieGlRef;

class MovieGl {
  public:
	MovieGl() = default;
	explicit MovieGl( const ci::fs::path &path );
	//MovieGl( const class MovieLoader &loader );
	//MovieGl( const void *data, size_t dataSize, const std::string &fileNameHint, const std::string &mimeTypeHint = "" );
	//MovieGl( DataSourceRef dataSource, const std::string mimeTypeHint = "" );

	~MovieGl();

	static MovieGlRef create( const ci::fs::path &path ) { return std::make_shared<MovieGl>( path ); }
	//static MovieGlRef create( const MovieLoaderRef &loader );
	//static MovieGlRef create( const void *data, size_t dataSize, const std::string &fileNameHint, const std::string &mimeTypeHint = "" )
	//	 { return std::shared_ptr<MovieGl>( new MovieGl( data, dataSize, fileNameHint, mimeTypeHint ) ); }
	//static MovieGlRef create( DataSourceRef dataSource, const std::string mimeTypeHint = "" )
	//	 { return std::shared_ptr<MovieGl>( new MovieGl( dataSource, mimeTypeHint ) ); }

	//! Returns the gl::Texture representing the Movie's current frame, bound to the \c GL_TEXTURE_RECTANGLE_ARB target
	const ci::gl::Texture2dRef &getTexture();

	//! Returns whether the movie has loaded and buffered enough to playback without interruption
	// bool		checkPlayable();

	//! Returns the width of the movie in pixels
	int32_t getWidth() const { return mWidth; }
	//! Returns the height of the movie in pixels
	int32_t getHeight() const { return mHeight; }
	//! Returns the size of the movie in pixels
	ci::ivec2 getSize() const { return ci::ivec2( getWidth(), getHeight() ); }
	//! Returns the movie's aspect ratio, the ratio of its width to its height
	float getAspectRatio() const { return mWidth / static_cast<float>( mHeight ); }
	//! the Area defining the Movie's bounds in pixels: [0,0]-[width,height]
	ci::Area getBounds() const { return ci::Area( 0, 0, getWidth(), getHeight() ); }
	//! Returns the movie's pixel aspect ratio. Returns 1.0 if the movie does not contain an explicit pixel aspect ratio.
	///float		getPixelAspectRatio() const;
	//! Returns the movie's length measured in seconds
	float getDuration() const { return mDuration; }
	//! Returns the movie's framerate measured as frames per second
	float getFramerate() const;
	//! Returns the total number of frames (video samples) in the movie
	uint64_t getNumFrames() const;
	//! Returns whether the first video track in the movie contains an alpha channel. Returns false in the absence of visual media.
	///bool		hasAlpha() const;

	//! Returns whether a movie contains at least one visual track, defined as Video, MPEG, Sprite, QuickDraw3D, Text, or TimeCode tracks
	///bool		hasVisuals() const;
	//! Returns whether a movie contains at least one audio track, defined as Sound, Music, or MPEG tracks
	///bool		hasAudio() const;

	//! Returns whether a movie has a new frame available
	bool checkNewFrame() const;

	//! Returns the current time of a movie in seconds
	float getCurrentTime() const;
	//! Sets the movie to the time \a seconds
	void seekToTime( float seconds );
	//! Sets the movie time to the start time of frame \a frame
	void seekToFrame( int frame );
	//! Sets the movie time to its beginning
	void seekToStart();
	//! Sets the movie time to its end
	void seekToEnd();
	//! Limits the active portion of a movie to a subset beginning at \a startTime seconds and lasting for \a duration seconds. QuickTime will not process the movie outside the active segment.
	///void		setActiveSegment( float startTime, float duration );
	//! Resets the active segment to be the entire movie
	///void		resetActiveSegment();

	//! Sets whether the movie is set to loop during playback. If \a palindrome is true, the movie will "ping-pong" back and forth
	///void		setLoop( bool loop = true, bool palindrome = false );
	//! Advances the movie by one frame (a single video sample). Ignores looping settings.
	///void		stepForward();
	//! Steps backward by one frame (a single video sample). Ignores looping settings.
	///void		stepBackward();
	//! Sets the playback rate, which begins playback immediately for nonzero values. 1.0 represents normal speed. Negative values indicate reverse playback and \c 0 stops.
	///void		setRate( float rate );

	//! Sets the audio playback volume ranging from [0 - 1.0]
	///void		setVolume( float volume );
	//! Gets the audio playback volume ranging from [0 - 1.0]
	///float		getVolume() const;

	//! Sets up Fourier analysis on the movie using \a numBands distinct bands in a single (mono) channel. This data is only available during playback.
	///void		setupMonoFft( uint32_t numBands );
	//! Sets up Fourier analysis on the movie using \a numBands distinct bands in a two (stereo) channels. This data is only available during playback.
	///void		setupStereoFft( uint32_t numBands );
	//! Sets up Fourier analysis on the movie using \a numBands distinct bands in as many channels as the audo track contains. To determine the number of channels, use getNumFftChannels(). This data is only available during playback.
	///void		setupMultiChannelFft( uint32_t numBands );

	///float*		getFftData() const;
	///uint32_t	getNumFftBands() const;
	///uint32_t	getNumFftChannels() const;

	//! Returns whether the movie is currently playing or is paused/stopped.
	bool isPlaying() const;
	//! Returns whether the movie has completely finished playing
	bool isDone() const;
	//! Begins movie playback.
	void play();
	//! Stops movie playback.
	void stop() const;

	//! Sets a function which is called whenever the movie has rendered a new frame during playback. Generally only necessary for advanced users.
	void setNewFrameCallback( void ( *aNewFrameCallback )( long, void * ), void *aNewFrameCallbackRefcon )
	{
	}

  private:
	void initializeShader();

  private:
	// copy ops are private to prevent copying
	MovieGl( const MovieGl & ) = delete;
	MovieGl &operator=( const MovieGl & ) = delete;

	int32_t mWidth;
	int32_t mHeight;

	float mDuration;

	//
	std::unique_ptr<AudioRenderer> mAudioRenderer;
	std::unique_ptr<MovieDecoder>  mMovieDecoder;

	double mVideoClock;

	ci::gl::Texture2dRef mYPlane;
	ci::gl::Texture2dRef mUPlane;
	ci::gl::Texture2dRef mVPlane;
	ci::gl::Texture2dRef mTexture;

	ci::gl::GlslProgRef mShader;

	ci::gl::FboRef mFbo;
};

} // namespace ffmpeg
} // namespace ph
