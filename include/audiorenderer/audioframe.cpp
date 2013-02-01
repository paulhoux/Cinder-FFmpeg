#include "audioframe.h"

AudioFrame::AudioFrame()
: m_FrameData(0)
, m_DataSize(0)
, m_Pts(0.0)
{
}

AudioFrame::~AudioFrame()
{

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
    m_FrameData = data;
}

void AudioFrame::setDataSize(uint32 size)
{
    m_DataSize = size;
}

void AudioFrame::setPts(double pts)
{
    m_Pts = pts;
}
