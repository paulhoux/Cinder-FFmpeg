#ifndef AUDIO_FRAME_H
#define AUDIO_FRAME_H

#include "Common/commontypes.h"

class AudioFrame
{
public:
    AudioFrame();
    virtual ~AudioFrame();

    byte*   getFrameData() const;
    uint32  getDataSize() const;
    double  getPts() const;

    void setFrameData(byte* data);
    void setDataSize(uint32 size);
    void setPts(double pts);

private:
    byte*   m_FrameData;
    uint32  m_DataSize;
    double  m_Pts;
};

#endif
