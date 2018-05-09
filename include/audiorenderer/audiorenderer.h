#ifndef AUDIORENDERER_H
#define AUDIORENDERER_H

class AudioFrame;
struct AudioFormat;

class AudioRenderer {
  public:
	AudioRenderer();
	virtual ~AudioRenderer();

	virtual void setFormat( const AudioFormat &format ) = 0;

	virtual void play() = 0;
	virtual void pause() = 0;
	virtual void stop() = 0;
	virtual void adjustVolume( float offset ) = 0;

	virtual bool hasQueuedFrames() = 0;
	virtual bool hasBufferSpace() = 0;
	virtual void clearBuffers() = 0;
	virtual void flushBuffers() = 0;
	virtual void queueFrame( const AudioFrame &frame ) = 0;

	virtual double getCurrentPts() = 0;
};

#endif
