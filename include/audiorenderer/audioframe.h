#ifndef AUDIO_FRAME_H
#define AUDIO_FRAME_H

#include "common/commontypes.h"

class AudioFrame {
  public:
	AudioFrame();
	virtual ~AudioFrame();

	byte * getFrameData() const;
	uint32 getDataSize() const;
	double getPts() const;

	void setFrameData( byte *data );
	void setDataSize( uint32 size );
	void setPts( double pts );

  private:
	byte * mFrameData;
	uint32 mDataSize;
	double mPts;
};

#endif
