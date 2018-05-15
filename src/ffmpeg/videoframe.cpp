#include "ffmpeg/videoframe.h"

#include <cassert>
#include <algorithm>

namespace ffmpeg {

VideoFrame::VideoFrame()
    : m_Pts( 0.0 )
    , m_Width( 0 )
    , m_Height( 0 )
{
	memset( &m_pData, 0, sizeof( m_pData ) );
	memset( &m_LineSize, 0, sizeof( m_LineSize ) );
}

size_t VideoFrame::getDataSize( size_t index ) const
{
	assert( index >= 0 && index < m_Count );
	return getLineSize( index ) * getHeight();
}

uint8_t *VideoFrame::getPlane( size_t index ) const
{
	assert( index >= 0 && index < m_Count );
	return m_pData[index];
}

double VideoFrame::getPts() const
{
	return m_Pts;
}

int VideoFrame::getWidth() const
{
	return m_Width;
}

int VideoFrame::getHeight() const
{
	return m_Height;
}

int VideoFrame::getLineSize( size_t index ) const
{
	assert( index >= 0 && index < m_Count );
	return m_LineSize[index];
}

void VideoFrame::storePlane( size_t index, uint8_t *data, int lineSize )
{
	assert( index >= 0 && index < AV_NUM_DATA_POINTERS );
	m_Count = std::max( m_Count, index + 1 );
	m_pData[index] = data;
	m_LineSize[index] = lineSize;
}

void VideoFrame::setPts( double pts )
{
	m_Pts = pts;
}

void VideoFrame::setWidth( int width )
{
	m_Width = width;
}

void VideoFrame::setHeight( int height )
{
	m_Height = height;
}

} // namespace ffmpeg
