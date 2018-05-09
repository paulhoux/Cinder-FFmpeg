#ifndef OPENAL_RENDERER_H
#define OPENAL_RENDERER_H

#pragma comment( lib, "OpenAL32.lib" )

#include <al/al.h>
#include <al/alc.h>
#include <deque>

#include "audiorenderer/audiorenderer.h"

#define NUM_BUFFERS 10

class AudioFrame;
struct AudioFormat;

class OpenAlRenderer : public AudioRenderer {
  public:
	OpenAlRenderer();
	virtual ~OpenAlRenderer();

	void   setFormat( const AudioFormat &format ) override;
	bool   hasQueuedFrames() override;
	bool   hasBufferSpace() override;
	void   queueFrame( const AudioFrame &frame ) override;
	void   clearBuffers() override;
	void   flushBuffers() override;
	int    getBufferSize();
	double getCurrentPts() override;
	void   play() override;
	void   pause() override;
	void   stop() override;
	void   adjustVolume( float offset ) override;

  private:
	bool isPlaying();

	static ALCdevice * mPAudioDevice;
	static ALCcontext *mPAlcContext;
	static int         mRefCount;

	ALuint             mAudioSource;
	ALuint             mAudioBuffers[NUM_BUFFERS];
	int                mCurrentBuffer;
	float              mVolume;
	ALenum             mAudioFormat;
	ALsizei            mNumChannels;
	ALsizei            mFrequency;
	std::deque<double> mPtsQueue;
};

#endif
