#include "movierenderer/videoframe.h"

VideoFrame::VideoFrame()
    : m_YPlane( 0 )
    , m_UPlane( 0 )
    , m_VPlane( 0 )
    , m_Pts( 0.0 )
    , m_Width( 0 )
    , m_Height( 0 )
    , m_YLineSize( 0 )
    , m_ULineSize( 0 )
    , m_VLineSize( 0 )
{
}

size_t VideoFrame::getYDataSize() const
{
	return getYLineSize() * getHeight();
}

size_t VideoFrame::getUDataSize() const
{
	return getULineSize() * getHeight() / 2;
}

size_t VideoFrame::getVDataSize() const
{
	return getVLineSize() * getHeight() / 2;
}

byte *VideoFrame::getYPlane() const
{
	return m_YPlane;
}

byte *VideoFrame::getUPlane() const
{
	return m_UPlane;
}

byte *VideoFrame::getVPlane() const
{
	return m_VPlane;
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

int VideoFrame::getYLineSize() const
{
	return m_YLineSize;
}

int VideoFrame::getULineSize() const
{
	return m_ULineSize;
}

int VideoFrame::getVLineSize() const
{
	return m_VLineSize;
}

void VideoFrame::storeYPlane( byte *data, int lineSize )
{
	m_YPlane = data;
	m_YLineSize = lineSize;
}

void VideoFrame::storeUPlane( byte *data, int lineSize )
{
	m_UPlane = data;
	m_ULineSize = lineSize;
}

void VideoFrame::storeVPlane( byte *data, int lineSize )
{
	m_VPlane = data;
	m_VLineSize = lineSize;
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
