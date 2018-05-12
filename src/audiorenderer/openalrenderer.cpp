#include "cinder/app/App.h"

#include "audiorenderer/audioformat.h"
#include "audiorenderer/audioframe.h"
#include "audiorenderer/openalrenderer.h"
#include "common/numericoperations.h"

using namespace std;

ALCdevice * OpenAlRenderer::mPAudioDevice = nullptr;
ALCcontext *OpenAlRenderer::mPAlcContext = nullptr;
int         OpenAlRenderer::mRefCount = 0;

OpenAlRenderer::OpenAlRenderer()
    : AudioRenderer()
    , mAudioSource( 0 )
    , mCurrentBuffer( 0 )
    , mVolume( 1.f )
    , mAudioFormat( AL_FORMAT_STEREO16 )
    , mNumChannels( 0 )
    , mFrequency( 0 )
{
	if( !mPAudioDevice )
		mPAudioDevice = alcOpenDevice( NULL );

	if( mPAudioDevice && !mPAlcContext ) {
		mPAlcContext = alcCreateContext( mPAudioDevice, NULL );
		alcMakeContextCurrent( mPAlcContext );
	}

	mRefCount++;

	assert( alGetError() == AL_NO_ERROR );
	alGenBuffers( NUM_BUFFERS, mAudioBuffers );
	alGenSources( 1, &mAudioSource );
}

OpenAlRenderer::~OpenAlRenderer()
{
	alSourceStop( mAudioSource );
	alDeleteSources( 1, &mAudioSource );
	alDeleteBuffers( NUM_BUFFERS, mAudioBuffers );

	if( --mRefCount <= 0 ) {
		if( mPAlcContext ) {
			alcMakeContextCurrent( NULL );
			alcDestroyContext( mPAlcContext );
			mPAlcContext = nullptr;
		}

		if( mPAudioDevice ) {
			alcCloseDevice( mPAudioDevice );
			mPAudioDevice = nullptr;
		}
	}
}

void OpenAlRenderer::setFormat( const AudioFormat &format )
{
	switch( format.bits ) {
	case 8:
		switch( format.numChannels ) {
		case 1:
			mAudioFormat = alGetEnumValue( "AL_FORMAT_MONO8" );
			break;
		case 2:
			mAudioFormat = alGetEnumValue( "AL_FORMAT_STEREO8" );
			break;
		case 4:
			mAudioFormat = alGetEnumValue( "AL_FORMAT_QUAD8" );
			break;
		case 6:
			mAudioFormat = alGetEnumValue( "AL_FORMAT_51CHN8" );
			break;
		case 7:
			mAudioFormat = alGetEnumValue( "AL_FORMAT_61CHN8" );
			break;
		case 8:
			mAudioFormat = alGetEnumValue( "AL_FORMAT_71CHN8" );
			break;
		}

		if( alGetError() != AL_NO_ERROR )
			throw logic_error( "OpenAlRenderer: unsupported format" );
		break;
	case 16:
		switch( format.numChannels ) {
		case 1:
			mAudioFormat = alGetEnumValue( "AL_FORMAT_MONO16" );
			break;
		case 2:
			mAudioFormat = alGetEnumValue( "AL_FORMAT_STEREO16" );
			break;
		case 4:
			mAudioFormat = alGetEnumValue( "AL_FORMAT_QUAD16" );
			break;
		case 6:
			mAudioFormat = alGetEnumValue( "AL_FORMAT_51CHN16" );
			break;
		case 7:
			mAudioFormat = alGetEnumValue( "AL_FORMAT_61CHN16" );
			break;
		case 8:
			mAudioFormat = alGetEnumValue( "AL_FORMAT_71CHN16" );
			break;
		}

		if( alGetError() != AL_NO_ERROR )
			throw logic_error( "OpenAlRenderer: unsupported format" );
		break;
	default:
		throw logic_error( "OpenAlRenderer: unsupported format" );
	}

	mNumChannels = format.numChannels;
	mFrequency = format.rate;
}

bool OpenAlRenderer::hasQueuedFrames()
{
	int queued = 0;
	alGetSourcei( mAudioSource, AL_BUFFERS_QUEUED, &queued );
	return queued > 0;
}

bool OpenAlRenderer::hasBufferSpace()
{
	int queued = 0;
	alGetSourcei( mAudioSource, AL_BUFFERS_QUEUED, &queued );
	return queued < NUM_BUFFERS;
}

void OpenAlRenderer::queueFrame( const AudioFrame &frame )
{
	assert( frame.getFrameData() );
	alBufferData( mAudioBuffers[mCurrentBuffer], mAudioFormat, frame.getFrameData(), frame.getDataSize(), mFrequency );
	alSourceQueueBuffers( mAudioSource, 1, &mAudioBuffers[mCurrentBuffer] );
	mPtsQueue.push_back( frame.getPts() );

	play();

	++mCurrentBuffer;
	mCurrentBuffer %= NUM_BUFFERS;

	assert( alGetError() == AL_NO_ERROR );
}

void OpenAlRenderer::clearBuffers()
{
	stop();

	int queued = 0;
	alGetSourcei( mAudioSource, AL_BUFFERS_QUEUED, &queued );

	if( queued > 0 ) {
		ALuint *buffers = new ALuint[queued];
		alSourceUnqueueBuffers( mAudioSource, queued, buffers );
		delete[] buffers;
	}
	mPtsQueue.clear();
}

void OpenAlRenderer::flushBuffers()
{
	int processed = 0;
	alGetSourcei( mAudioSource, AL_BUFFERS_PROCESSED, &processed );

	while( processed-- ) {
		ALuint buffer;
		alSourceUnqueueBuffers( mAudioSource, 1, &buffer );
		assert( alGetError() == AL_NO_ERROR );
		mPtsQueue.pop_front();
	}
}

bool OpenAlRenderer::isPlaying()
{
	ALenum state;
	alGetSourcei( mAudioSource, AL_SOURCE_STATE, &state );

	return ( state == AL_PLAYING );
}

void OpenAlRenderer::play()
{
	if( !isPlaying() && !mPtsQueue.empty() ) {
		alSourcePlay( mAudioSource );
	}
}

void OpenAlRenderer::pause()
{
	if( isPlaying() ) {
		alSourcePause( mAudioSource );
	}
}

void OpenAlRenderer::stop()
{
	alSourceStop( mAudioSource );
	flushBuffers();
}

int OpenAlRenderer::getBufferSize()
{
	return NUM_BUFFERS;
}

void OpenAlRenderer::adjustVolume( float offset )
{
	mVolume += offset;
	NumericOperations::clip( mVolume, 0.f, 1.f );
	alSourcef( mAudioSource, AL_GAIN, mVolume );
}

double OpenAlRenderer::getCurrentPts()
{
	float offsetInSeconds = 0.f;
	alGetSourcef( mAudioSource, AL_SEC_OFFSET, &offsetInSeconds );

	return mPtsQueue.empty() ? 0 : mPtsQueue.front() + double( offsetInSeconds );
}
