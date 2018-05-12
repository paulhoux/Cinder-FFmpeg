#ifndef VIDEO_FRAME_H
#define VIDEO_FRAME_H

#include "common/commontypes.h"

class VideoFrame {
  public:
	VideoFrame();

	bool isValid() const
	{
		return ( m_YPlane && m_UPlane && m_VPlane && m_Width > 0 && m_Height > 0 );
	}

	size_t getYDataSize() const;
	size_t getUDataSize() const;
	size_t getVDataSize() const;
	byte * getYPlane() const;
	byte * getUPlane() const;
	byte * getVPlane() const;
	double getPts() const;
	int    getWidth() const;
	int    getHeight() const;
	int    getYLineSize() const;
	int    getULineSize() const;
	int    getVLineSize() const;

	void storeYPlane( byte *data, int lineSize );
	void storeUPlane( byte *data, int lineSize );
	void storeVPlane( byte *data, int lineSize );
	void setPts( double pts );
	void setWidth( int width );
	void setHeight( int height );

  private:
	byte * m_YPlane = nullptr;
	byte * m_UPlane = nullptr;
	byte * m_VPlane = nullptr;
	double m_Pts = 0.0;
	int    m_Width = 0;
	int    m_Height = 0;
	int    m_YLineSize = 0;
	int    m_ULineSize = 0;
	int    m_VLineSize = 0;
};

#endif
