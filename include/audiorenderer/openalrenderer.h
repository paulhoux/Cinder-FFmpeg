#ifndef OPENAL_RENDERER_H
#define OPENAL_RENDERER_H

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#include <deque>
#include "audiorenderer.h"

#define NUM_BUFFERS 10

class AudioFrame;
struct AudioFormat;

class OpenALRenderer : public AudioRenderer
{
public:
    OpenALRenderer();
    virtual ~OpenALRenderer();

    void setFormat(const AudioFormat& format);
	bool hasQueuedFrames();
    bool hasBufferSpace();
    void queueFrame(const AudioFrame& frame);
    void clearBuffers();
    void flushBuffers();
    int getBufferSize();
    double getCurrentPts();
    void play();
    void pause();
    void stop();
    void adjustVolume(float offset);

private:
    bool isPlaying();

    static ALCdevice*   m_pAudioDevice;
    static ALCcontext*  m_pAlcContext;
	static int          m_RefCount;

    ALuint              m_AudioSource;
    ALuint              m_AudioBuffers[NUM_BUFFERS];
    int                 m_CurrentBuffer;
    float               m_Volume;
    ALenum              m_AudioFormat;
	ALsizei				m_NumChannels;
    ALsizei             m_Frequency;

    std::deque<double>  m_PtsQueue;
};

#endif
