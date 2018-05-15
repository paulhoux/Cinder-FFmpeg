#pragma once

#include <cstdint>
#include <libavutil/frame.h>

namespace ffmpeg {

class VideoFrame {
  public:
	VideoFrame();

	bool isValid() const
	{
		return ( m_pData && m_Width > 0 && m_Height > 0 );
	}

	bool     empty() const { return m_Count == 0; }
	size_t   size() const { return m_Count; }

	size_t   getDataSize( size_t index = 0 ) const;
	uint8_t *getPlane( size_t index = 0 ) const;
	double   getPts() const;
	int      getWidth() const;
	int      getHeight() const;
	int      getLineSize( size_t index = 0 ) const;

	void storePlane( size_t index, uint8_t *data, int lineSize );
	void setPts( double pts );
	void setWidth( int width );
	void setHeight( int height );

  private:
	size_t   m_Count = 0;
	uint8_t *m_pData[AV_NUM_DATA_POINTERS];
	int      m_LineSize[AV_NUM_DATA_POINTERS];
	double   m_Pts = 0.0;
	int      m_Width = 0;
	int      m_Height = 0;
};

} // namespace ffmpeg
