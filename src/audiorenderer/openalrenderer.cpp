#include "audiorenderer/openalrenderer.h"

#include "audiorenderer/audioframe.h"
#include "audiorenderer/audioformat.h"
#include "common/numericoperations.h"

#include <iostream>
#include <assert.h>
#include <stdexcept>

#include "cinder/app/AppBasic.h"

using namespace std;

OpenALRenderer::OpenALRenderer()
: AudioRenderer()
, m_pAudioDevice(NULL)
, m_pAlcContext(NULL)
, m_AudioSource(0)
, m_CurrentBuffer(0)
, m_Volume(1.f)
, m_AudioFormat(AL_FORMAT_STEREO16)
, m_NumChannels(0)
, m_Frequency(0)
{
    m_pAudioDevice = alcOpenDevice(NULL);

    if (m_pAudioDevice)
    {
        m_pAlcContext = alcCreateContext(m_pAudioDevice, NULL);
        alcMakeContextCurrent(m_pAlcContext);
    }

    assert(alGetError() == AL_NO_ERROR);
    alGenBuffers(NUM_BUFFERS, m_AudioBuffers);
    alGenSources(1, &m_AudioSource);
}

OpenALRenderer::~OpenALRenderer()
{
    alSourceStop(m_AudioSource);
    alDeleteSources(1, &m_AudioSource);
    alDeleteBuffers(NUM_BUFFERS, m_AudioBuffers);

    if (m_pAudioDevice)
    {
        alcMakeContextCurrent(NULL);
        alcDestroyContext(m_pAlcContext);
        alcCloseDevice(m_pAudioDevice);
    }
}

void OpenALRenderer::setFormat(const AudioFormat& format)
{
	switch (format.bits)
	{
	case 8:
		switch(format.numChannels)
		{
		case 1:
			m_AudioFormat = alGetEnumValue("AL_FORMAT_MONO8");
			break;
		case 2:
			m_AudioFormat = alGetEnumValue("AL_FORMAT_STEREO8");
			break;
		case 4:
			m_AudioFormat = alGetEnumValue("AL_FORMAT_QUAD8");
			break;
		case 6:
			m_AudioFormat = alGetEnumValue("AL_FORMAT_51CHN8");
			break;
		case 7:
			m_AudioFormat = alGetEnumValue("AL_FORMAT_61CHN8");
			break;
		case 8:
			m_AudioFormat = alGetEnumValue("AL_FORMAT_71CHN8");
			break;
		}

		if(alGetError() != AL_NO_ERROR)
			throw logic_error("OpenAlRenderer: unsupported format");
		break;
	case 16:
		switch(format.numChannels)
		{
		case 1:
			m_AudioFormat = alGetEnumValue("AL_FORMAT_MONO16");
			break;
		case 2:
			m_AudioFormat = alGetEnumValue("AL_FORMAT_STEREO16");
			break;
		case 4:
			m_AudioFormat = alGetEnumValue("AL_FORMAT_QUAD16");
			break;
		case 6:
			m_AudioFormat = alGetEnumValue("AL_FORMAT_51CHN16");
			break;
		case 7:
			m_AudioFormat = alGetEnumValue("AL_FORMAT_61CHN16");
			break;
		case 8:
			m_AudioFormat = alGetEnumValue("AL_FORMAT_71CHN16");
			break;
		}

		if(alGetError() != AL_NO_ERROR)
			throw logic_error("OpenAlRenderer: unsupported format");
		break;
	default:
		throw logic_error("OpenAlRenderer: unsupported format");
	}

	m_NumChannels = format.numChannels;
    m_Frequency = format.rate;
}

bool OpenALRenderer::hasQueuedFrames()
{
	int queued = 0;
    alGetSourcei(m_AudioSource, AL_BUFFERS_QUEUED, &queued);
    return queued > 0;
}

bool OpenALRenderer::hasBufferSpace()
{
    int queued = 0;
    alGetSourcei(m_AudioSource, AL_BUFFERS_QUEUED, &queued);
    return queued < NUM_BUFFERS;
}

void OpenALRenderer::queueFrame(const AudioFrame& frame)
{
	assert(frame.getFrameData());
    alBufferData(m_AudioBuffers[m_CurrentBuffer], m_AudioFormat, frame.getFrameData(), frame.getDataSize(), m_Frequency);
    alSourceQueueBuffers(m_AudioSource, 1, &m_AudioBuffers[m_CurrentBuffer]);
    m_PtsQueue.push_back(frame.getPts());

    play();

    ++m_CurrentBuffer;
    m_CurrentBuffer %= NUM_BUFFERS;

	assert(alGetError() == AL_NO_ERROR);
}

void OpenALRenderer::clearBuffers()
{
    stop();

    int queued = 0;
    alGetSourcei(m_AudioSource, AL_BUFFERS_QUEUED, &queued);

    if (queued > 0)
    {
        ALuint* buffers = new ALuint[queued];
        alSourceUnqueueBuffers(m_AudioSource, queued, buffers);
        delete[] buffers;
    }
    m_PtsQueue.clear();
}

void OpenALRenderer::flushBuffers()
{
    int processed = 0;
    alGetSourcei(m_AudioSource, AL_BUFFERS_PROCESSED, &processed);

    while(processed--)
    {
        ALuint buffer;
        alSourceUnqueueBuffers(m_AudioSource, 1, &buffer);
        assert(alGetError() == AL_NO_ERROR);
        m_PtsQueue.pop_front();
    }
}

bool OpenALRenderer::isPlaying()
{
    ALenum state;
    alGetSourcei(m_AudioSource, AL_SOURCE_STATE, &state);

    return (state == AL_PLAYING);
}

void OpenALRenderer::play()
{
    if (!isPlaying() && !m_PtsQueue.empty())
    {
        alSourcePlay(m_AudioSource);
    }
}

void OpenALRenderer::pause()
{
    if (isPlaying())
    {
        alSourcePause(m_AudioSource);
    }
}

void OpenALRenderer::stop()
{
    alSourceStop(m_AudioSource);
    flushBuffers();
}

int OpenALRenderer::getBufferSize()
{
    return NUM_BUFFERS;
}

void OpenALRenderer::adjustVolume(float offset)
{
    m_Volume += offset;
    NumericOperations::clip(m_Volume, 0.f, 1.f);
    alSourcef(m_AudioSource, AL_GAIN, m_Volume);
}

double OpenALRenderer::getCurrentPts()
{
	float offsetInSeconds = 0.f;
	alGetSourcef( m_AudioSource, AL_SEC_OFFSET, &offsetInSeconds );

    return m_PtsQueue.empty() ? 0 : m_PtsQueue.front() + double(offsetInSeconds);
}

