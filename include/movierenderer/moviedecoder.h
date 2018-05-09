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

//#define __STDC_LIMIT_MACROS 1
//#define __STDC_CONSTANT_MACROS 1

#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

//#include <boost/thread/thread.hpp>
//#include <boost/thread/mutex.hpp>

#include "audiorenderer/audioformat.h"
#include "videoframe.h"

#define MAX_AUDIO_FRAME_SIZE 192000

class AudioFrame;

class MovieDecoder {
  public:
	explicit MovieDecoder( const std::string &filename );
	~MovieDecoder();

	bool decodeVideoFrame( VideoFrame &videoFrame );
	bool decodeAudioFrame( AudioFrame &audioFrame );
	void seekToTime( double seconds );
	void seekToFrame( uint32_t frame );
	void start();
	void stop();

	int getFrameWidth() const;
	int getFrameHeight() const;

	static int getLineSize( int planeNr )
	{
		return 0;
	}

	double      getAudioTimeBase() const;
	AudioFormat getAudioFormat();

	double getVideoClock() const;
	double getAudioClock() const;
	float  getProgress() const;

	float    getDuration() const;
	float    getFramesPerSecond() const;
	uint64_t getNumberOfFrames() const;

	bool isInitialized() const { return m_bInitialized; }
	bool isPlaying() const { return m_bPlaying; }
	bool isDone() const { return m_bDone; }

  private:
	// copy ops are private to prevent copying
	MovieDecoder( const MovieDecoder & ) = delete;            // no implementation
	MovieDecoder &operator=( const MovieDecoder & ) = delete; // no implementation

	void readPackets();
	bool queuePacket( std::queue<AVPacket> &packetQueue, AVPacket *packet ) const;
	bool queueVideoPacket( AVPacket *packet );
	bool queueAudioPacket( AVPacket *packet );
	bool popPacket( std::queue<AVPacket> &packetQueue, AVPacket *packet ) const;
	bool popVideoPacket( AVPacket *packet );
	bool popAudioPacket( AVPacket *packet );
	void clearQueue( std::queue<AVPacket> &packetQueue ) const;
	void createAVFrame( AVFrame **avFrame, int width, int height, AVPixelFormat format ) const;

	bool initializeVideo();
	bool initializeAudio();

	bool decodeVideoPacket( AVPacket &packet );
	void convertVideoFrame( AVPixelFormat target );

	//! Initializes FFmpeg
	static void startFFmpeg();

  private:
	int                  mVideoStream;
	int                  mAudioStream;
	AVFormatContext *    mPFormatContext;
	AVCodecContext *     mPVideoCodecContext;
	AVCodecContext *     m_pAudioCodecContext;
	AVCodec *            m_pVideoCodec;
	AVCodec *            m_pAudioCodec;
	AVStream *           m_pVideoStream;
	AVStream *           m_pAudioStream;
	AVSampleFormat       m_SourceFormat;
	AVSampleFormat       m_TargetFormat;
	uint8_t              m_AudioBuffer[MAX_AUDIO_FRAME_SIZE * 4];
	AVFrame *            m_pFrame;
	AVPacket             m_FlushPacket;
	SwrContext *         m_pSwrContext;
	int                  m_MaxVideoQueueSize;
	int                  m_MaxAudioQueueSize;
	std::queue<AVPacket> m_VideoQueue;
	std::queue<AVPacket> m_AudioQueue;
	std::mutex           m_VideoQueueMutex;
	std::mutex           m_AudioQueueMutex;
	std::mutex           m_DecodeVideoMutex;
	std::mutex           m_DecodeAudioMutex;
	std::thread *        m_pPacketReaderThread;
	bool                 m_bInitialized;
	bool                 m_bPlaying;
	bool                 m_bSingleFrame;
	bool                 m_bDone;
	bool                 m_bSeeking;
	int                  m_SeekFlags;
	int64_t              m_SeekTimestamp;
	double               m_AudioClock;
	double               m_VideoClock;
};

#endif
