#include "audiorenderer/audioframe.h"

#include <memory>

#define USE_MEMCPY 1

AudioFrame::AudioFrame()
: mFrameData(nullptr)
, mDataSize(0)
, mPts(0.0)
{
}

AudioFrame::~AudioFrame()
{
#if USE_MEMCPY
	if(mFrameData)
		delete[] mFrameData;
#endif

	mFrameData = nullptr;
}

byte* AudioFrame::getFrameData() const
{
    return mFrameData;
}

uint32 AudioFrame::getDataSize() const
{
    return mDataSize;
}

double AudioFrame::getPts() const
{
    return mPts;
}

void AudioFrame::setFrameData(byte* data)
{
#if USE_MEMCPY
	if(!mFrameData)
		mFrameData = new byte[mDataSize];

	memcpy(mFrameData, data, mDataSize);
#else
	m_FrameData = data;
#endif
}

void AudioFrame::setDataSize(uint32 size)
{
#if USE_MEMCPY
	if(size > mDataSize && mFrameData)
	{
		delete[] mFrameData;
		mFrameData = nullptr;
	}
#endif
    mDataSize = size;
}

void AudioFrame::setPts(double pts)
{
    mPts = pts;
}
