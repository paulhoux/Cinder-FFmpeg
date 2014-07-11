#ifndef MOVIEDECODER_H
#define MOVIEDECODER_H

/*#ifdef WIN32
    typedef signed char  int8_t;
    typedef signed short int16_t;
    typedef signed int   int32_t;
    typedef unsigned char  uint8_t;
    typedef unsigned short uint16_t;
    typedef unsigned int   uint32_t;
    typedef signed long long   int64_t;
    typedef unsigned long long uint64_t;
    typedef signed char int_fast8_t;
    typedef signed int  int_fast16_t;
    typedef signed int  int_fast32_t;
    typedef unsigned char uint_fast8_t;
    typedef unsigned int  uint_fast16_t;
    typedef unsigned int  uint_fast32_t;
    typedef uint64_t      uint_fast64_t;
#else
    #include <inttypes.h>
#endif*/

#include <string>
#include <queue>
#include <boost/cstdint.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "common/commontypes.h"
#include "videoframe.h"
#include "audiorenderer/audioformat.h"

#define MAX_AUDIO_FRAME_SIZE 192000

class AudioFrame;

class MovieDecoder
{
public:
    MovieDecoder(const std::string& filename);
    ~MovieDecoder();

    bool decodeVideoFrame(VideoFrame& videoFrame);
    bool decodeAudioFrame(AudioFrame& audioFrame);
    void seek(int offset);
    void start();
    void stop();

    int     getFrameWidth() const;
    int     getFrameHeight() const;
    int     getLineSize(int planeNr) const;
    double  getAudioTimeBase() const;
    AudioFormat getAudioFormat();

    double  getVideoClock() const;
    double  getAudioClock() const;
    float   getProgress() const;
    float   getDuration() const;

	bool	isInitialized() const { return m_bInitialized; }

private:
	// copy ops are private to prevent copying 
    MovieDecoder(const MovieDecoder&); // no implementation 
    MovieDecoder& operator=(const MovieDecoder&); // no implementation 

    void readPackets();
    bool queuePacket(std::queue<AVPacket>& packetQueue, AVPacket* packet);
    bool queueVideoPacket(AVPacket* packet);
    bool queueAudioPacket(AVPacket* packet);
    bool popPacket(std::queue<AVPacket>& packetQueue, AVPacket* packet);
    bool popVideoPacket(AVPacket* packet);
    bool popAudioPacket(AVPacket* packet);
    void clearQueue(std::queue<AVPacket>& packetQueue);
    void createAVFrame(AVFrame** avFrame, int width, int height, PixelFormat format);

    bool initializeVideo();
    bool initializeAudio();

    bool decodeVideoPacket(AVPacket& packet);
    void convertVideoFrame(PixelFormat target);

private:
    int                     m_VideoStream;
    int                     m_AudioStream;
    AVFormatContext*        m_pFormatContext;
    AVCodecContext*         m_pVideoCodecContext;
    AVCodecContext*         m_pAudioCodecContext;
    AVCodec*                m_pVideoCodec;
    AVCodec*                m_pAudioCodec;
    AVStream*               m_pVideoStream;
    AVStream*               m_pAudioStream;
	AVSampleFormat          m_SourceFormat;
	AVSampleFormat          m_TargetFormat;
    uint8_t                 m_AudioBuffer[MAX_AUDIO_FRAME_SIZE * 4];
    AVFrame*                m_pFrame;

	SwrContext*             m_pSwrContext;

    int                     m_MaxVideoQueueSize;
    int                     m_MaxAudioQueueSize;
    std::queue<AVPacket>    m_VideoQueue;
    std::queue<AVPacket>    m_AudioQueue;
    boost::mutex            m_VideoQueueMutex;
    boost::mutex            m_AudioQueueMutex;
    boost::mutex            m_DecodeVideoMutex;
    boost::mutex            m_DecodeAudioMutex;
    boost::thread*          m_pPacketReaderThread;

	bool					m_bInitialized;
    bool                    m_Stop;

    double                  m_AudioClock;
    double                  m_VideoClock;
};

#endif
