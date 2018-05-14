#include "cinder/App/App.h"

#include "audiorenderer/audioframe.h"
#include "movierenderer/moviedecoder.h"
#include "movierenderer/videoframe.h"

#include <cassert>

extern "C" {
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

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
    : m_VideoStream( -1 )
    , m_AudioStream( -1 )
    , m_pFormatContext( NULL )
    , m_pVideoCodecContext( NULL )
    , m_pAudioCodecContext( NULL )
    , m_pVideoCodec( NULL )
    , m_pAudioCodec( NULL )
    , m_pVideoStream( NULL )
    , m_pAudioStream( NULL )
    , m_pFrame( NULL )
    , m_pConvertedFrame( NULL )
    , m_pSwrContext( NULL )
    , m_MaxVideoQueueSize( VIDEO_QUEUESIZE )
    , m_MaxAudioQueueSize( AUDIO_QUEUESIZE )
    , m_pPacketReaderThread( NULL )
    , m_bInitialized( false )
    , m_bPlaying( false )
    , m_bPaused( false )
    , m_bSingleFrame( false )
    , m_bLoop( false )
    , m_bDone( false )
    , m_bSeeking( false )
    , m_AudioClock( 0.0 )
    , m_VideoClock( 0.0 )
{
	m_bInitialized = false;

	startFFmpeg();

	av_init_packet( &m_FlushPacket );
	m_FlushPacket.data = reinterpret_cast<uint8_t *>( "FLUSH" );
	m_FlushPacket.size = strlen( reinterpret_cast<const char *>( m_FlushPacket.data ) );

#if LIBAVCODEC_VERSION_MAJOR < 53
	if( av_open_input_file( &m_pFormatContext, filename.c_str(), NULL, 0, NULL ) != 0 )
#else
	if( avformat_open_input( &m_pFormatContext, filename.c_str(), NULL, NULL ) != 0 )
#endif
	{
		throw logic_error( "MovieDecoder: Could not open input file" );
	}

	try {
#if LIBAVCODEC_VERSION_MAJOR < 53
		if( av_find_stream_info( m_pFormatContext ) < 0 )
#else
		if( avformat_find_stream_info( m_pFormatContext, NULL ) < 0 )
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

	m_bHasVideo = initializeVideo();
	m_bHasAudio = initializeAudio();
	m_bInitialized = ( m_bHasVideo || m_bHasAudio );
}

MovieDecoder::~MovieDecoder()
{
	stop();

	m_bInitialized = false;

	if( m_pConvertedFrame ) {
		av_frame_free( &m_pConvertedFrame );
		m_pConvertedFrame = NULL;
	}

	if( m_pFrame ) {
		av_frame_free( &m_pFrame );
		m_pFrame = NULL;
	}

	if( m_pVideoCodecContext ) {
		avcodec_close( m_pVideoCodecContext );
		m_pVideoCodecContext = NULL;
	}

	if( m_pAudioCodecContext ) {
		avcodec_close( m_pAudioCodecContext );
		m_pAudioCodecContext = NULL;
	}

	if( m_pFormatContext ) {
#if LIBAVCODEC_VERSION_MAJOR < 53
		av_close_input_file( m_pFormatContext );
		m_pFormatContext = NULL;
#else
		avformat_close_input( &m_pFormatContext );
#endif
	}

	if( m_pSwrContext )
		swr_free( &m_pSwrContext );
}

bool MovieDecoder::initializeVideo()
{
	for( unsigned int i = 0; i < m_pFormatContext->nb_streams; i++ ) {
#if LIBAVCODEC_VERSION_MAJOR < 53
		if( m_pFormatContext->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO )
#else
		if( m_pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
#endif
		{
			m_pVideoStream = m_pFormatContext->streams[i];
			m_VideoStream = i;
			break;
		}
	}

	if( m_VideoStream == -1 ) {
		throw logic_error( "MovieDecoder: Could not find video stream" );
		return false;
	}

	m_pVideoCodecContext = m_pFormatContext->streams[m_VideoStream]->codec;
	m_pVideoCodec = avcodec_find_decoder( m_pVideoCodecContext->codec_id );

	if( m_pVideoCodec == NULL ) {
		// set to NULL, otherwise avcodec_close(m_pVideoCodecContext) crashes
		m_pVideoCodecContext = NULL;
		throw logic_error( "MovieDecoder: Video Codec not found" );
		return false;
	}

	m_pVideoCodecContext->workaround_bugs = 1;
	m_pFormatContext->flags |= AVFMT_FLAG_GENPTS;

#if LIBAVCODEC_VERSION_MAJOR < 53
	if( avcodec_open( m_pVideoCodecContext, m_pVideoCodec ) < 0 )
#else
	if( avcodec_open2( m_pVideoCodecContext, m_pVideoCodec, NULL ) < 0 )
#endif
	{
		throw logic_error( "MovieDecoder: Could not open video codec" );
		return false;
	}

	m_pFrame = av_frame_alloc();

	return true;
}

bool MovieDecoder::initializeAudio()
{
	for( unsigned int i = 0; i < m_pFormatContext->nb_streams; i++ ) {
#if LIBAVCODEC_VERSION_MAJOR < 53
		if( m_pFormatContext->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO )
#else
		if( m_pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
#endif
		{
			m_pAudioStream = m_pFormatContext->streams[i];
			m_AudioStream = i;
			break;
		}
	}

	if( m_AudioStream == -1 ) {
		return false;
	}

	m_pAudioCodecContext = m_pFormatContext->streams[m_AudioStream]->codec;
	if( m_pAudioCodecContext->channel_layout == 0 || m_pAudioCodecContext->channels != av_get_channel_layout_nb_channels( m_pAudioCodecContext->channel_layout ) )
		m_pAudioCodecContext->channel_layout = av_get_default_channel_layout( m_pAudioCodecContext->channels );
	m_pAudioCodec = avcodec_find_decoder( m_pAudioCodecContext->codec_id );

	if( m_pAudioCodec == NULL ) {
		// set to NULL, otherwise avcodec_close(m_pAudioCodecContext) crashes
		m_pAudioCodecContext = NULL;
		throw logic_error( "MovieDecoder: Audio Codec not found" );
		return false;
	}

	m_pAudioCodecContext->workaround_bugs = 1;

#if LIBAVCODEC_VERSION_MAJOR < 53
	if( avcodec_open( m_pAudioCodecContext, m_pAudioCodec ) < 0 )
#else
	if( avcodec_open2( m_pAudioCodecContext, m_pAudioCodec, NULL ) < 0 )
#endif
	{
		throw logic_error( "MovieDecoder: Could not open audio codec" );
		return false;
	}

	return true;
}

int MovieDecoder::getFrameHeight() const
{
	return m_pVideoCodecContext ? m_pVideoCodecContext->height : -1;
}

int MovieDecoder::getFrameWidth() const
{
	return m_pVideoCodecContext ? m_pVideoCodecContext->width : -1;
}

double MovieDecoder::getVideoClock() const
{
	return m_VideoClock;
}

double MovieDecoder::getAudioClock() const
{
	return m_AudioClock;
}

double MovieDecoder::getProgress() const
{
	return m_pFormatContext ? m_bHasAudio ? m_AudioClock / getDuration() : m_VideoClock / getDuration() : 0.0;
}

double MovieDecoder::getDuration() const
{
	return m_pFormatContext ? m_pFormatContext->duration / double( AV_TIME_BASE ) : 0.0;
}

double MovieDecoder::getFramesPerSecond() const
{
	return m_pVideoStream ? m_pVideoStream->avg_frame_rate.num / double( m_pVideoStream->avg_frame_rate.den ) : 0.0;
}

uint64_t MovieDecoder::getNumberOfFrames() const
{
	return m_pVideoStream ? m_pVideoStream->nb_frames : 0;
}

void MovieDecoder::seekToTime( double seconds )
{
	m_SeekTimestamp = ::int64_t( AV_TIME_BASE * seconds );
	m_SeekFlags = ( seconds < m_AudioClock ) ? AVSEEK_FLAG_BACKWARD : 0;

	if( m_SeekTimestamp < 0 )
		m_SeekTimestamp = 0;
	else if( m_SeekTimestamp > m_pFormatContext->duration )
		m_SeekTimestamp = m_pFormatContext->duration;

	m_AudioClock = double( m_SeekTimestamp ) / AV_TIME_BASE;
	m_VideoClock = m_AudioClock;

	m_bSingleFrame = !m_bPlaying;
	m_bSeeking = true;
}

void MovieDecoder::seekToFrame( uint32_t frame )
{
	if( !m_pVideoStream )
		return;

	const double fps = getFramesPerSecond();
	const double seconds = frame / fps;
	seekToTime( seconds );
}

bool MovieDecoder::decodeVideoFrame( VideoFrame &frame )
{
	if( !m_bHasVideo )
		return false;

	AVPacket packet;
	bool     frameDecoded = false;

	do {
		if( !popVideoPacket( &packet ) )
			return false;

		// handle flush packets
		if( packet.data == m_FlushPacket.data ) {
			avcodec_flush_buffers( m_pFormatContext->streams[m_VideoStream]->codec );
			continue;
		}

		frameDecoded = decodeVideoPacket( packet );
	} while( !frameDecoded );

	if( m_bSingleFrame ) {
		m_bSingleFrame = false;
		m_bPlaying = false;
	}

	if( m_pFrame->interlaced_frame ) {
		// See: https://stackoverflow.com/a/40018558/858219
		throw logic_error( "MovieDecoder: Interlaced video is not supported yet." );
	}

	m_VideoClock = packet.dts * av_q2d( m_pVideoStream->time_base );

	frame.setPts( m_VideoClock );
	frame.setWidth( getFrameWidth() );
	frame.setHeight( getFrameHeight() );

	try {
		if( m_pVideoCodecContext->pix_fmt != AV_PIX_FMT_YUV420P && m_pVideoCodecContext->pix_fmt != AV_PIX_FMT_YUV420P ) {
			if( !m_pConvertedFrame )
				createAVFrame( &m_pConvertedFrame, frame.getWidth(), frame.getHeight(), AV_PIX_FMT_YUV420P );

			convertVideoFrame( AV_PIX_FMT_YUV420P );

			frame.storeYPlane( m_pConvertedFrame->data[0], m_pConvertedFrame->linesize[0] );
			frame.storeUPlane( m_pConvertedFrame->data[1], m_pConvertedFrame->linesize[1] );
			frame.storeVPlane( m_pConvertedFrame->data[2], m_pConvertedFrame->linesize[2] );
		}
		else {
			frame.storeYPlane( m_pFrame->data[0], m_pFrame->linesize[0] );
			frame.storeUPlane( m_pFrame->data[1], m_pFrame->linesize[1] );
			frame.storeVPlane( m_pFrame->data[2], m_pFrame->linesize[2] );
		}
	}
	catch( const std::exception & ) {
		return false;
	}

	return frameDecoded;
}

void MovieDecoder::convertVideoFrame( AVPixelFormat format )
{
	SwsContext *scaleContext = sws_getContext( m_pVideoCodecContext->width, m_pVideoCodecContext->height, m_pVideoCodecContext->pix_fmt, m_pVideoCodecContext->width, m_pVideoCodecContext->height, format, 0, NULL, NULL, NULL );
	if( NULL == scaleContext )
		throw logic_error( "MovieDecoder: Failed to create resize context" );

	sws_scale( scaleContext, m_pFrame->data, m_pFrame->linesize, 0, m_pVideoCodecContext->height, m_pConvertedFrame->data, m_pConvertedFrame->linesize );
	sws_freeContext( scaleContext );
}

void MovieDecoder::createAVFrame( AVFrame **avFrame, int width, int height, AVPixelFormat format ) const
{
	*avFrame = av_frame_alloc();

	const int numBytes = avpicture_get_size( format, width, height );
	uint8_t * pBuffer = new uint8_t[numBytes];
	avpicture_fill( reinterpret_cast<AVPicture *>( *avFrame ), pBuffer, format, width, height );

	//const int numBytes = av_image_get_buffer_size( format, width, height, 1 );
	//uint8_t * pBuffer = static_cast<uint8_t *>( av_malloc( numBytes ) );
	//av_image_fill_arrays( ( *avFrame )->data, ( *avFrame )->linesize, pBuffer, format, ( *avFrame )->width, ( *avFrame )->height, 1 );
}

bool MovieDecoder::decodeVideoPacket( AVPacket &packet )
{
	std::lock_guard<std::mutex> lock( m_DecodeVideoMutex );

	int       frameFinished;
	const int bytesDecoded = avcodec_decode_video2( m_pVideoCodecContext, m_pFrame, &frameFinished, &packet );
	av_free_packet( &packet );

	if( bytesDecoded < 0 ) {
		ci::app::console() << "Failed to decode video frame: bytesDecoded < 0" << endl;
	}

	return frameFinished > 0;
}

bool MovieDecoder::decodeAudioFrame( AudioFrame &frame )
{
	if( !m_bHasAudio )
		return false;

	bool frameDecoded = false;

	AVPacket packet;
	if( !popAudioPacket( &packet ) )
		return false;

	// handle flush packets
	if( packet.data == m_FlushPacket.data ) {
		avcodec_flush_buffers( m_pFormatContext->streams[m_AudioStream]->codec );
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
		av_frame_free( &decodedFrame );

	return frameDecoded;
}

void MovieDecoder::readPackets()
{
	AVPacket packet;

	while( !m_bDone || m_bSeeking ) {
		if( m_bSeeking ) {
			m_bSeeking = false;

			const int ret = av_seek_frame( m_pFormatContext, -1, m_SeekTimestamp, m_SeekFlags );
			if( ret >= 0 ) {
				{
					std::lock_guard<std::mutex> audioLock( m_AudioQueueMutex );
					std::lock_guard<std::mutex> videoLock( m_VideoQueueMutex );

					clearQueue( m_AudioQueue );
					clearQueue( m_VideoQueue );
				}

				if( m_AudioStream >= 0 )
					queueAudioPacket( &m_FlushPacket );

				if( m_VideoStream >= 0 )
					queueVideoPacket( &m_FlushPacket );
			}
		}
		else if( int( m_VideoQueue.size() ) >= m_MaxVideoQueueSize || int( m_AudioQueue.size() ) >= m_MaxAudioQueueSize ) {
			this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
		}
		else if( m_bPlaying && av_read_frame( m_pFormatContext, &packet ) >= 0 ) {
			if( packet.stream_index == m_VideoStream ) {
				queueVideoPacket( &packet );
			}
			else if( packet.stream_index == m_AudioStream ) {
				queueAudioPacket( &packet );
			}
			else {
				av_free_packet( &packet );
			}
		}
		else if( m_bLoop && !m_bPaused ) {
			const auto stream = m_pFormatContext->streams[m_VideoStream];
			avio_seek( m_pFormatContext->pb, 0, SEEK_SET );
			avformat_seek_file( m_pFormatContext, m_VideoStream, 0, 0, stream->duration, 0 );
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
	m_bPaused = false;
	m_bDone = false;
	if( !m_pPacketReaderThread ) {
		m_pPacketReaderThread = new std::thread( std::bind( &MovieDecoder::readPackets, this ) );
	}
}

void MovieDecoder::pause()
{
	if( m_bPlaying && !m_bPaused ) {
		m_bPlaying = false;
		m_bPaused = true;
	}
}

void MovieDecoder::resume()
{
	if( !m_bPlaying && m_bPaused ) {
		m_bPlaying = true;
		m_bPaused = false;
	}
}

void MovieDecoder::stop()
{
	m_VideoClock = 0;
	m_AudioClock = 0;

	m_bPlaying = false;
	m_bPaused = false;
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

	if( m_pAudioCodecContext ) {
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
	}

	return format;
}
