#include "audiorenderer/audioframe.h"
#include <memory>

#define USE_MEMCPY 1

AudioFrame::AudioFrame()
: m_FrameData(nullptr)
, m_DataSize(0)
, m_Pts(0.0)
{
}

AudioFrame::~AudioFrame()
{
#if USE_MEMCPY
	if(m_FrameData)
		delete[] m_FrameData;
#endif

	m_FrameData = nullptr;
}

byte* AudioFrame::getFrameData() const
{
    return m_FrameData;
}

uint32 AudioFrame::getDataSize() const
{
    return m_DataSize;
}

double AudioFrame::getPts() const
{
    return m_Pts;
}

void AudioFrame::setFrameData(byte* data)
{
#if USE_MEMCPY
	if(!m_FrameData)
		m_FrameData = new byte[m_DataSize];

	memcpy(m_FrameData, data, m_DataSize);
#else
	m_FrameData = data;
#endif
}

void AudioFrame::setDataSize(uint32 size)
{
#if USE_MEMCPY
	if(size > m_DataSize && m_FrameData)
	{
		delete[] m_FrameData;
		m_FrameData = nullptr;
	}
#endif
    m_DataSize = size;
}

void AudioFrame::setPts(double pts)
{
    m_Pts = pts;
}
