#pragma once

#pragma comment( lib, "OpenAL32.lib" )

#include <al/al.h>
#include <al/alc.h>
#include <deque>

#include "ffmpeg/audiorenderer.h"

#define NUM_BUFFERS 10

namespace ffmpeg {

class AudioFrame;
struct AudioFormat;

class OpenAlRenderer : public AudioRenderer {
  public:
	OpenAlRenderer();
	virtual ~OpenAlRenderer();

	void       setFormat( const AudioFormat &format ) override;
	bool       hasQueuedFrames() override;
	bool       hasBufferSpace() override;
	void       queueFrame( const AudioFrame &frame ) override;
	void       clearBuffers() override;
	void       flushBuffers() override;
	static int getBufferSize();
	double     getCurrentPts() override;
	void       play() override;
	void       pause() override;
	void       stop() override;
	void       adjustVolume( float offset ) override;

  private:
	bool isPlaying() const;

	static ALCdevice * s_pAudioDevice;
	static ALCcontext *s_pAlcContext;
	static int         s_RefCount;

	ALuint             mAudioSource;
	ALuint             mAudioBuffers[NUM_BUFFERS];
	int                mCurrentBuffer;
	float              mVolume;
	ALenum             mAudioFormat;
	ALsizei            mNumChannels;
	ALsizei            mFrequency;
	std::deque<double> mPtsQueue;
};

} // namespace ffmpeg
