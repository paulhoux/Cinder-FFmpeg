#include "movierenderer/moviedecoder.h"
#include "audiorenderer/audioframe.h"
#include "movierenderer/videoframe.h"

#include <cassert>

//#include <boost/bind.hpp>
//#include <boost/thread/xtime.hpp>

extern "C" {
#include <libswscale/swscale.h>
}

#include "cinder/App/App.h"

#define VIDEO_QUEUESIZE 200
#define AUDIO_QUEUESIZE 50
#define VIDEO_FRAMES_BUFFERSIZE 5

using namespace std;
//using namespace boost;

void MovieDecoder::startFFmpeg()
{
	static bool libavcodec_initialized = false;
	if( !libavcodec_initialized ) {
		libavcodec_initialized = true;
		av_register_all();
		avcodec_register_all();
	}
}

MovieDecoder::MovieDecoder( const string &filename )
    : mVideoStream( -1 )
    , mAudioStream( -1 )
    , mPFormatContext( NULL )
    , mPVideoCodecContext( NULL )
    , m_pAudioCodecContext( NULL )
    , m_pVideoCodec( NULL )
    , m_pAudioCodec( NULL )
    , m_pVideoStream( NULL )
    , m_pAudioStream( NULL )
    , m_pFrame( NULL )
    , m_pSwrContext( NULL )
    , m_MaxVideoQueueSize( VIDEO_QUEUESIZE )
    , m_MaxAudioQueueSize( AUDIO_QUEUESIZE )
    , m_pPacketReaderThread( NULL )
    , m_bInitialized( false )
    , m_bPlaying( false )
    , m_bSingleFrame( false )
    , m_bDone( false )
    , m_bSeeking( false )
    , m_AudioClock( 0.0 )
    , m_VideoClock( 0.0 )
{
	bool ok = true;
	m_bInitialized = false;

	startFFmpeg();

	av_init_packet( &m_FlushPacket );
	m_FlushPacket.data = reinterpret_cast<uint8_t *>( "FLUSH" );
	m_FlushPacket.size = strlen( reinterpret_cast<const char *>( m_FlushPacket.data ) );

#if LIBAVCODEC_VERSION_MAJOR < 53
	if( av_open_input_file( &m_pFormatContext, filename.c_str(), NULL, 0, NULL ) != 0 )
#else
	if( avformat_open_input( &mPFormatContext, filename.c_str(), NULL, NULL ) != 0 )
#endif
	{
		throw logic_error( "MovieDecoder: Could not open input file" );
	}

	try {
#if LIBAVCODEC_VERSION_MAJOR < 53
		if( av_find_stream_info( m_pFormatContext ) < 0 )
#else
		if( avformat_find_stream_info( mPFormatContext, NULL ) < 0 )
#endif
			throw;
	}
	catch( ... ) {
		throw logic_error( "MovieDecoder: Could not find stream information" );
	}

#ifdef _DEBUG
	av_log_set_level( AV_LOG_DEBUG );
	//av_dump_format(m_pFormatContext, 0, filename.c_str(), false);
#endif

	ok = ok && initializeVideo();
	m_bInitialized = ok && initializeAudio();
}

MovieDecoder::~MovieDecoder()
{
	stop();

	m_bInitialized = false;

	if( m_pFrame ) {
		av_free( m_pFrame );
		m_pFrame = NULL;
	}

	if( mPVideoCodecContext ) {
		avcodec_close( mPVideoCodecContext );
		mPVideoCodecContext = NULL;
	}

	if( m_pAudioCodecContext ) {
		avcodec_close( m_pAudioCodecContext );
		m_pAudioCodecContext = NULL;
	}

	if( mPFormatContext ) {
#if LIBAVCODEC_VERSION_MAJOR < 53
		av_close_input_file( m_pFormatContext );
		m_pFormatContext = NULL;
#else
		avformat_close_input( &mPFormatContext );
#endif
	}

	if( m_pSwrContext )
		swr_free( &m_pSwrContext );
}

bool MovieDecoder::initializeVideo()
{
	for( unsigned int i = 0; i < mPFormatContext->nb_streams; i++ ) {
#if LIBAVCODEC_VERSION_MAJOR < 53
		if( m_pFormatContext->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO )
#else
		if( mPFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
#endif
		{
			m_pVideoStream = mPFormatContext->streams[i];
			mVideoStream = i;
			break;
		}
	}

	if( mVideoStream == -1 ) {
		throw logic_error( "MovieDecoder: Could not find video stream" );
	}

	mPVideoCodecContext = mPFormatContext->streams[mVideoStream]->codec;
	m_pVideoCodec = avcodec_find_decoder( mPVideoCodecContext->codec_id );

	if( m_pVideoCodec == NULL ) {
		// set to NULL, otherwise avcodec_close(m_pVideoCodecContext) crashes
		mPVideoCodecContext = NULL;
		throw logic_error( "MovieDecoder: Video Codec not found" );
	}

	mPVideoCodecContext->workaround_bugs = 1;
	mPFormatContext->flags |= AVFMT_FLAG_GENPTS;

#if LIBAVCODEC_VERSION_MAJOR < 53
	if( avcodec_open( m_pVideoCodecContext, m_pVideoCodec ) < 0 )
#else
	if( avcodec_open2( mPVideoCodecContext, m_pVideoCodec, NULL ) < 0 )
#endif
	{
		throw logic_error( "MovieDecoder: Could not open video codec" );
	}

	m_pFrame = av_frame_alloc();

	return true;
}

bool MovieDecoder::initializeAudio()
{
	for( unsigned int i = 0; i < mPFormatContext->nb_streams; i++ ) {
#if LIBAVCODEC_VERSION_MAJOR < 53
		if( m_pFormatContext->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO )
#else
		if( mPFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
#endif
		{
			m_pAudioStream = mPFormatContext->streams[i];
			mAudioStream = i;
			break;
		}
	}

	if( mAudioStream == -1 ) {
		throw logic_error( "MovieDecoder: No audiostream found" );
	}

	m_pAudioCodecContext = mPFormatContext->streams[mAudioStream]->codec;
	if( m_pAudioCodecContext->channel_layout == 0 || m_pAudioCodecContext->channels != av_get_channel_layout_nb_channels( m_pAudioCodecContext->channel_layout ) )
		m_pAudioCodecContext->channel_layout = av_get_default_channel_layout( m_pAudioCodecContext->channels );
	m_pAudioCodec = avcodec_find_decoder( m_pAudioCodecContext->codec_id );

	if( m_pAudioCodec == NULL ) {
		// set to NULL, otherwise avcodec_close(m_pAudioCodecContext) crashes
		m_pAudioCodecContext = NULL;
		throw logic_error( "MovieDecoder: Audio Codec not found" );
	}

	m_pAudioCodecContext->workaround_bugs = 1;

#if LIBAVCODEC_VERSION_MAJOR < 53
	if( avcodec_open( m_pAudioCodecContext, m_pAudioCodec ) < 0 )
#else
	if( avcodec_open2( m_pAudioCodecContext, m_pAudioCodec, NULL ) < 0 )
#endif
	{
		throw logic_error( "MovieDecoder: Could not open audio codec" );
	}

	return true;
}

int MovieDecoder::getFrameHeight() const
{
	return mPVideoCodecContext ? mPVideoCodecContext->height : -1;
}

int MovieDecoder::getFrameWidth() const
{
	return mPVideoCodecContext ? mPVideoCodecContext->width : -1;
}

double MovieDecoder::getVideoClock() const
{
	return m_VideoClock;
}

double MovieDecoder::getAudioClock() const
{
	return m_AudioClock;
}

float MovieDecoder::getProgress() const
{
	if( mPFormatContext ) {
		return static_cast<float>( m_AudioClock / ( mPFormatContext->duration / AV_TIME_BASE ) );
	}

	return 0.f;
}

float MovieDecoder::getDuration() const
{
	return static_cast<float>( mPFormatContext->duration / AV_TIME_BASE );
}

float MovieDecoder::getFramesPerSecond() const
{
	return static_cast<float>( m_pVideoStream->avg_frame_rate.num / double( m_pVideoStream->avg_frame_rate.den ) );
}

uint64_t MovieDecoder::getNumberOfFrames() const
{
	return m_pVideoStream->nb_frames;
}

void MovieDecoder::seekToTime( double seconds )
{
	m_SeekTimestamp = ::int64_t( AV_TIME_BASE * seconds );
	m_SeekFlags = ( seconds < m_AudioClock ) ? AVSEEK_FLAG_BACKWARD : 0;

	if( m_SeekTimestamp < 0 )
		m_SeekTimestamp = 0;
	else if( m_SeekTimestamp > mPFormatContext->duration )
		m_SeekTimestamp = mPFormatContext->duration;

	m_AudioClock = double( m_SeekTimestamp ) / AV_TIME_BASE;
	m_VideoClock = m_AudioClock;

	m_bSingleFrame = !m_bPlaying;
	m_bSeeking = true;
}

void MovieDecoder::seekToFrame( uint32_t frame )
{
	const double fps = m_pVideoStream->avg_frame_rate.num / double( m_pVideoStream->avg_frame_rate.den );
	const double seconds = frame / fps;
	seekToTime( seconds );
}

bool MovieDecoder::decodeVideoFrame( VideoFrame &frame )
{
	AVPacket packet;
	bool     frameDecoded = false;

	do {
		if( !popVideoPacket( &packet ) )
			return false;

		// handle flush packets
		if( packet.data == m_FlushPacket.data ) {
			avcodec_flush_buffers( mPFormatContext->streams[mVideoStream]->codec );
			continue;
		}

		frameDecoded = decodeVideoPacket( packet );
	} while( !frameDecoded );

	if( m_bSingleFrame ) {
		m_bSingleFrame = false;
		m_bPlaying = false;
	}

	//if( m_pFrame->interlaced_frame ) {
	//	avpicture_deinterlace( reinterpret_cast<AVPicture *>( m_pFrame ), reinterpret_cast<AVPicture *>( m_pFrame ), mPVideoCodecContext->pix_fmt, mPVideoCodecContext->width, mPVideoCodecContext->height );
	//}

	try {
		if( mPVideoCodecContext->pix_fmt != AV_PIX_FMT_YUV420P && mPVideoCodecContext->pix_fmt != AV_PIX_FMT_YUV420P )
			convertVideoFrame( AV_PIX_FMT_YUV420P );
	}
	catch( const std::exception & ) {
		return false;
	}

	m_VideoClock = packet.dts * av_q2d( m_pVideoStream->time_base );

	frame.setWidth( getFrameWidth() );
	frame.setHeight( getFrameHeight() );

	frame.storeYPlane( m_pFrame->data[0], m_pFrame->linesize[0] );
	frame.storeUPlane( m_pFrame->data[1], m_pFrame->linesize[1] );
	frame.storeVPlane( m_pFrame->data[2], m_pFrame->linesize[2] );
	frame.setPts( m_VideoClock );

	return frameDecoded;
}

void MovieDecoder::convertVideoFrame( AVPixelFormat format )
{
	SwsContext *scaleContext = sws_getContext( mPVideoCodecContext->width, mPVideoCodecContext->height, mPVideoCodecContext->pix_fmt, mPVideoCodecContext->width, mPVideoCodecContext->height, format, 0, NULL, NULL, NULL );

	if( NULL == scaleContext )
		throw logic_error( "MovieDecoder: Failed to create resize context" );

	AVFrame *convertedFrame = NULL;
	createAVFrame( &convertedFrame, mPVideoCodecContext->width, mPVideoCodecContext->height, format );

	sws_scale( scaleContext, m_pFrame->data, m_pFrame->linesize, 0, mPVideoCodecContext->height, convertedFrame->data, convertedFrame->linesize );
	sws_freeContext( scaleContext );

	av_free( m_pFrame );
	m_pFrame = convertedFrame;
}

void MovieDecoder::createAVFrame( AVFrame **avFrame, int width, int height, AVPixelFormat format ) const
{
	*avFrame = av_frame_alloc();

	const int numBytes = avpicture_get_size( format, width, height );
	uint8_t * pBuffer = new uint8_t[numBytes];

	avpicture_fill( reinterpret_cast<AVPicture *>( *avFrame ), pBuffer, format, width, height );
}

bool MovieDecoder::decodeVideoPacket( AVPacket &packet )
{
	std::lock_guard<std::mutex> lock( m_DecodeVideoMutex );

	int       frameFinished;
	const int bytesDecoded = avcodec_decode_video2( mPVideoCodecContext, m_pFrame, &frameFinished, &packet );
	av_free_packet( &packet );

	if( bytesDecoded < 0 ) {
		ci::app::console() << "Failed to decode video frame: bytesDecoded < 0" << endl;
	}

	return frameFinished > 0;
}

bool MovieDecoder::decodeAudioFrame( AudioFrame &frame )
{
	bool frameDecoded = false;

	AVPacket packet;
	if( !popAudioPacket( &packet ) )
		return false;

	// handle flush packets
	if( packet.data == m_FlushPacket.data ) {
		avcodec_flush_buffers( mPFormatContext->streams[mAudioStream]->codec );
		return false;
	}

	AVFrame *decodedFrame = nullptr;

	int gotFrame = 0;
	int bytesRemaining = packet.size;
	while( bytesRemaining > 0 ) {
		int bytesDecoded;
		{
			if( !decodedFrame ) {
				if( decodedFrame = av_frame_alloc() ) {
					av_frame_unref( decodedFrame );
				}
				else {
					// out of memory
					break;
				}
			}

			std::lock_guard<std::mutex> lock( m_DecodeAudioMutex );
			bytesDecoded = avcodec_decode_audio4( m_pAudioStream->codec, decodedFrame, &gotFrame, &packet );
		}

		if( bytesDecoded < 0 ) {
			// error while decoding
			break;
		}

		bytesRemaining -= bytesDecoded;

		int dataSize = 0;
		if( gotFrame ) {
			dataSize = av_samples_get_buffer_size( nullptr, m_pAudioCodecContext->channels, decodedFrame->nb_samples, m_pAudioCodecContext->sample_fmt, 1 );

			//
			if( m_pAudioCodecContext->sample_fmt != m_TargetFormat || !m_pSwrContext ) {
				if( m_pSwrContext ) {
					swr_free( &m_pSwrContext );
					m_pSwrContext = nullptr;
				}

				m_pSwrContext = swr_alloc_set_opts( m_pSwrContext,
				    m_pAudioCodecContext->channel_layout,
				    m_TargetFormat,
				    m_pAudioCodecContext->sample_rate,
				    m_pAudioCodecContext->channel_layout,
				    m_pAudioCodecContext->sample_fmt,
				    m_pAudioCodecContext->sample_rate,
				    0,
				    NULL );

				if( !m_pSwrContext ) {
					break;
				}
				else if( swr_init( m_pSwrContext ) < 0 ) {
					break;
				}

				m_SourceFormat = m_pAudioCodecContext->sample_fmt;
			}

			if( m_pSwrContext ) {
				const uint8_t **in = const_cast<const uint8_t **>( decodedFrame->extended_data );
				uint8_t *       out = &m_AudioBuffer[0];

				const int bytesPerSample = m_pAudioCodecContext->channels * av_get_bytes_per_sample( m_TargetFormat );
				const int samplesOut = swr_convert( m_pSwrContext, &out, sizeof( m_AudioBuffer ) / bytesPerSample, in, decodedFrame->nb_samples );

				dataSize = samplesOut * bytesPerSample;
			}
		}

		if( dataSize > 0 ) {
			frameDecoded = true;
			frame.setDataSize( dataSize );
			frame.setFrameData( m_AudioBuffer );
			frame.setPts( packet.pts * av_q2d( m_pAudioStream->time_base ) );
		}
	}

	av_free_packet( &packet );

	if( decodedFrame )
		av_free( decodedFrame );

	return frameDecoded;
}

void MovieDecoder::readPackets()
{
	AVPacket packet;

	while( !m_bDone || m_bSeeking ) {
		if( m_bSeeking ) {
			m_bSeeking = false;

			const int ret = av_seek_frame( mPFormatContext, -1, m_SeekTimestamp, m_SeekFlags );
			if( ret >= 0 ) {
				{
					std::lock_guard<std::mutex> audioLock( m_AudioQueueMutex );
					std::lock_guard<std::mutex> videoLock( m_VideoQueueMutex );

					clearQueue( m_AudioQueue );
					clearQueue( m_VideoQueue );
				}

				if( mAudioStream >= 0 )
					queueAudioPacket( &m_FlushPacket );

				if( mVideoStream >= 0 )
					queueVideoPacket( &m_FlushPacket );
			}
		}
		else if( int( m_VideoQueue.size() ) >= m_MaxVideoQueueSize || int( m_AudioQueue.size() ) >= m_MaxAudioQueueSize ) {
			//boost::xtime xt;
			//boost::xtime_get( &xt, TIME_UTC_ );
			//xt.nsec += 25000000; // 25 msec
			//m_pPacketReaderThread->sleep( xt );
		}
		else if( m_bPlaying && av_read_frame( mPFormatContext, &packet ) >= 0 ) {
			if( packet.stream_index == mVideoStream ) {
				queueVideoPacket( &packet );
			}
			else if( packet.stream_index == mAudioStream ) {
				queueAudioPacket( &packet );
			}
			else {
				av_free_packet( &packet );
			}
		}
		else {
			m_bPlaying = false;
		}
	}
}

void MovieDecoder::start()
{
	stop();

	m_bPlaying = true;
	m_bDone = false;
	if( !m_pPacketReaderThread ) {
		m_pPacketReaderThread = new std::thread( std::bind( &MovieDecoder::readPackets, this ) );
	}
}

void MovieDecoder::stop()
{
	m_VideoClock = 0;
	m_AudioClock = 0;

	m_bPlaying = false;
	m_bDone = true;
	if( m_pPacketReaderThread ) {
		m_pPacketReaderThread->join();
		delete m_pPacketReaderThread;
		m_pPacketReaderThread = NULL;
	}

	clearQueue( m_AudioQueue );
	clearQueue( m_VideoQueue );
}

bool MovieDecoder::queueVideoPacket( AVPacket *packet )
{
	std::lock_guard<std::mutex> lock( m_VideoQueueMutex );
	return queuePacket( m_VideoQueue, packet );
}

bool MovieDecoder::queueAudioPacket( AVPacket *packet )
{
	std::lock_guard<std::mutex> lock( m_AudioQueueMutex );
	return queuePacket( m_AudioQueue, packet );
}

bool MovieDecoder::queuePacket( queue<AVPacket> &packetQueue, AVPacket *packet ) const
{
	if( av_dup_packet( packet ) < 0 ) {
		return false;
	}
	packetQueue.push( *packet );

	return true;
}

bool MovieDecoder::popPacket( queue<AVPacket> &packetQueue, AVPacket *packet ) const
{
	if( packetQueue.empty() ) {
		return false;
	}

	*packet = packetQueue.front();
	packetQueue.pop();

	return true;
}

void MovieDecoder::clearQueue( std::queue<AVPacket> &packetQueue ) const
{
	while( !packetQueue.empty() ) {
		packetQueue.pop();
	}
}

bool MovieDecoder::popAudioPacket( AVPacket *packet )
{
	std::lock_guard<std::mutex> lock( m_AudioQueueMutex );
	return popPacket( m_AudioQueue, packet );
}

bool MovieDecoder::popVideoPacket( AVPacket *packet )
{
	std::lock_guard<std::mutex> lock( m_VideoQueueMutex );
	return popPacket( m_VideoQueue, packet );
}

double MovieDecoder::getAudioTimeBase() const
{
	return m_pAudioStream ? av_q2d( m_pAudioStream->time_base ) : 0.0;
}

AudioFormat MovieDecoder::getAudioFormat()
{
	AudioFormat format;

	switch( m_pAudioCodecContext->sample_fmt ) {
	case AV_SAMPLE_FMT_U8:
	case AV_SAMPLE_FMT_U8P:
		format.bits = 8;
		m_TargetFormat = AV_SAMPLE_FMT_U8;
		break;
	case AV_SAMPLE_FMT_S16:
	case AV_SAMPLE_FMT_S16P:
		format.bits = 16;
		m_TargetFormat = AV_SAMPLE_FMT_S16;
		break;
	case AV_SAMPLE_FMT_S32:
	case AV_SAMPLE_FMT_S32P:
		format.bits = 32;
		m_TargetFormat = AV_SAMPLE_FMT_S16;
		break;
	case AV_SAMPLE_FMT_FLTP:
		format.bits = 16;
		m_TargetFormat = AV_SAMPLE_FMT_S16;
		break;
	default:
		// try to resample the audio to 16-but signed integers
		format.bits = 16;
		m_TargetFormat = AV_SAMPLE_FMT_S16;
	}

	format.rate = m_pAudioCodecContext->sample_rate;
	format.numChannels = m_pAudioCodecContext->channels;
	format.framesPerPacket = m_pAudioCodecContext->frame_size;

	return format;
}
