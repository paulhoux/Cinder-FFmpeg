#include "ffmpeg/audioframe.h"

#include <memory>

#define USE_MEMCPY 1

namespace ffmpeg {

AudioFrame::AudioFrame()
    : mFrameData( nullptr )
    , mDataSize( 0 )
    , mPts( 0.0 )
{
}

AudioFrame::~AudioFrame()
{
#if USE_MEMCPY
	if( mFrameData )
		delete[] mFrameData;
#endif

	mFrameData = nullptr;
}

uint8_t *AudioFrame::getFrameData() const
{
	return mFrameData;
}

uint32_t AudioFrame::getDataSize() const
{
	return mDataSize;
}

double AudioFrame::getPts() const
{
	return mPts;
}

void AudioFrame::setFrameData( uint8_t *data )
{
#if USE_MEMCPY
	if( !mFrameData )
		mFrameData = new uint8_t[mDataSize];

	memcpy( mFrameData, data, mDataSize );
#else
	m_FrameData = data;
#endif
}

void AudioFrame::setDataSize( uint32_t size )
{
#if USE_MEMCPY
	if( size > mDataSize && mFrameData ) {
		delete[] mFrameData;
		mFrameData = nullptr;
	}
#endif
	mDataSize = size;
}

void AudioFrame::setPts( double pts )
{
	mPts = pts;
}

} // namespace ffmpeg
