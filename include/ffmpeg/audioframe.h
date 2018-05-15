#pragma once

#include <cstdint>

namespace ffmpeg {

class AudioFrame {
  public:
	AudioFrame();
	virtual ~AudioFrame();

	uint8_t *getFrameData() const;
	uint32_t getDataSize() const;
	double   getPts() const;

	void setFrameData( uint8_t *data );
	void setDataSize( uint32_t size );
	void setPts( double pts );

  private:
	uint8_t *mFrameData;
	uint32_t mDataSize;
	double   mPts;
};

} // namespace ffmpeg
