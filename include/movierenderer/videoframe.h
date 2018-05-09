#ifndef VIDEO_FRAME_H
#define VIDEO_FRAME_H

#include "common/commontypes.h"

class VideoFrame {
  public:
	VideoFrame();
	virtual ~VideoFrame();

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
	byte * m_YPlane;
	byte * m_UPlane;
	byte * m_VPlane;
	double m_Pts;
	int    m_Width;
	int    m_Height;
	int    m_YLineSize;
	int    m_ULineSize;
	int    m_VLineSize;
};

#endif
