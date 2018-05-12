#ifndef MOVIEDECODER_H
#define MOVIEDECODER_H

#pragma comment( lib, "avcodec.lib" )
#pragma comment( lib, "avformat.lib" )
#pragma comment( lib, "avutil.lib" )
#pragma comment( lib, "swresample.lib" )
#pragma comment( lib, "swscale.lib" )

#include <mutex>
#include <queue>
#include <string>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

#include "audiorenderer/audioformat.h"
#include "movierenderer/videoframe.h"

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
	void loop( bool enabled = true ) { m_bLoop = enabled; }

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
	double getProgress() const;

	double   getDuration() const;
	double   getFramesPerSecond() const;
	uint64_t getNumberOfFrames() const;

	bool isInitialized() const { return m_bInitialized; }
	bool isPlaying() const { return m_bPlaying; }
	bool isLoop() const { return m_bLoop; }
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
	int                  m_VideoStream;
	int                  m_AudioStream;
	AVFormatContext *    m_pFormatContext;
	AVCodecContext *     m_pVideoCodecContext;
	AVCodecContext *     m_pAudioCodecContext;
	AVCodec *            m_pVideoCodec;
	AVCodec *            m_pAudioCodec;
	AVStream *           m_pVideoStream;
	AVStream *           m_pAudioStream;
	AVSampleFormat       m_SourceFormat;
	AVSampleFormat       m_TargetFormat;
	uint8_t              m_AudioBuffer[MAX_AUDIO_FRAME_SIZE * 4];
	AVFrame *            m_pFrame;
	AVFrame *            m_pConvertedFrame;
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
	bool                 m_bLoop;
	bool                 m_bDone;
	bool                 m_bSeeking;
	int                  m_SeekFlags;
	int64_t              m_SeekTimestamp;
	double               m_AudioClock;
	double               m_VideoClock;
};

#endif
