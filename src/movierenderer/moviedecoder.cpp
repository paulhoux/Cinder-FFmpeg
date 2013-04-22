#include "movierenderer/moviedecoder.h"
#include "movierenderer/videoframe.h"
#include "audiorenderer/audioframe.h"

#include <cassert>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/thread/xtime.hpp>

extern "C" {
#include <libswscale/swscale.h>
}

#include "cinder/App/AppBasic.h"

#define VIDEO_QUEUESIZE 20
#define AUDIO_QUEUESIZE 50
#define VIDEO_FRAMES_BUFFERSIZE 5

using namespace std;
using namespace boost;

MovieDecoder::MovieDecoder()
: m_VideoStream(-1)
, m_AudioStream(-1)
, m_pFormatContext(NULL)
, m_pVideoCodecContext(NULL)
, m_pAudioCodecContext(NULL)
, m_pVideoCodec(NULL)
, m_pAudioCodec(NULL)
, m_pVideoStream(NULL)
, m_pAudioStream(NULL)
, m_pAudioBuffer(NULL)
, m_pAudioBufferTemp(NULL)
, m_pFrame(NULL)
, m_pSwrContext(NULL)
, m_MaxVideoQueueSize(VIDEO_QUEUESIZE)
, m_MaxAudioQueueSize(AUDIO_QUEUESIZE)
, m_pPacketReaderThread(NULL)
, m_Stop(false)
, m_AudioClock(0.0)
, m_VideoClock(0.0)
{
}

MovieDecoder::~MovieDecoder()
{
    destroy();
}

void MovieDecoder::destroy()
{
    stop();

    if (m_pFrame)
    {
        av_free(m_pFrame);
        m_pFrame = NULL;
    }

    if (m_pVideoCodecContext)
    {
        avcodec_close(m_pVideoCodecContext);
        m_pVideoCodecContext = NULL;
    }

    if (m_pAudioCodecContext)
    {
        avcodec_close(m_pAudioCodecContext);
        m_pAudioCodecContext = NULL;
    }

    if (m_pFormatContext)
    {
#if LIBAVCODEC_VERSION_MAJOR < 53
        av_close_input_file(m_pFormatContext);
        m_pFormatContext = NULL;
#else
        avformat_close_input(&m_pFormatContext);
#endif
    }

    if (m_pAudioBufferTemp)
    {
        delete[] m_pAudioBufferTemp;
        m_pAudioBufferTemp = NULL;
    }

	if (m_pAudioBuffer)
    {
        delete[] m_pAudioBuffer;
        m_pAudioBuffer = NULL;
    }

	if( m_pSwrContext ) 
		swr_free(&m_pSwrContext);
}

bool MovieDecoder::initialize(const string& filename)
{
    bool ok = true;

    av_register_all();
    avcodec_register_all();
    
#if LIBAVCODEC_VERSION_MAJOR < 53
    if (av_open_input_file(&m_pFormatContext, filename.c_str(), NULL, 0, NULL) != 0)
#else
    if (avformat_open_input(&m_pFormatContext, filename.c_str(), NULL, NULL) != 0)
#endif
    {
        throw logic_error("MovieDecoder: Could not open input file");
    }

	try {
#if LIBAVCODEC_VERSION_MAJOR < 53
        if (av_find_stream_info(m_pFormatContext) < 0)
#else
        if (avformat_find_stream_info(m_pFormatContext, NULL) < 0)
#endif
            throw;
    }
    catch(...)
    {
        throw logic_error("MovieDecoder: Could not find stream information");
    }

#ifdef _DEBUG
    //dump_format(m_pFormatContext, 0, filename.c_str(), false);
    av_log_set_level(AV_LOG_DEBUG);
#endif

    ok = ok && initializeVideo();
    ok = ok && initializeAudio();

    return ok;
}

bool MovieDecoder::initializeVideo()
{
    for(unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
    {
#if LIBAVCODEC_VERSION_MAJOR < 53
        if (m_pFormatContext->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
#else
        if (m_pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
#endif
        {
            m_pVideoStream = m_pFormatContext->streams[i];
            m_VideoStream = i;
            break;
        }
    }

    if (m_VideoStream == -1)
    {
        throw logic_error("MovieDecoder: Could not find video stream");
    }

    m_pVideoCodecContext = m_pFormatContext->streams[m_VideoStream]->codec;
    m_pVideoCodec = avcodec_find_decoder(m_pVideoCodecContext->codec_id);

    if (m_pVideoCodec == NULL)
    {
        // set to NULL, otherwise avcodec_close(m_pVideoCodecContext) crashes
        m_pVideoCodecContext = NULL;
        throw logic_error("MovieDecoder: Video Codec not found");
    }

    m_pVideoCodecContext->workaround_bugs = 1;
    m_pFormatContext->flags |= AVFMT_FLAG_GENPTS;

#if LIBAVCODEC_VERSION_MAJOR < 53
    if (avcodec_open(m_pVideoCodecContext, m_pVideoCodec) < 0)
#else
    if (avcodec_open2(m_pVideoCodecContext, m_pVideoCodec, NULL) < 0)
#endif
    {
        throw logic_error("MovieDecoder: Could not open video codec");
    }

    m_pFrame = avcodec_alloc_frame();

    return true;
}

bool MovieDecoder::initializeAudio()
{
    for(unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
    {
#if LIBAVCODEC_VERSION_MAJOR < 53
        if (m_pFormatContext->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO)
#else
        if (m_pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
#endif
        {
            m_pAudioStream = m_pFormatContext->streams[i];
            m_AudioStream = i;
            break;
        }
    }

    if (m_AudioStream == -1)
    {
        throw logic_error("MovieDecoder: No audiostream found");
    }

    m_pAudioCodecContext = m_pFormatContext->streams[m_AudioStream]->codec;
    m_pAudioCodec = avcodec_find_decoder(m_pAudioCodecContext->codec_id);

    if (m_pAudioCodec == NULL)
    {
        // set to NULL, otherwise avcodec_close(m_pAudioCodecContext) crashes
        m_pAudioCodecContext = NULL;
        throw logic_error("MovieDecoder: Audio Codec not found");
    }

    m_pAudioCodecContext->workaround_bugs = 1;

#if LIBAVCODEC_VERSION_MAJOR < 53
    if (avcodec_open(m_pAudioCodecContext, m_pAudioCodec) < 0)
#else
    if (avcodec_open2(m_pAudioCodecContext, m_pAudioCodec, NULL) < 0)
#endif
    {
        throw logic_error("MovieDecoder: Could not open audio codec");
    }

    m_pAudioBuffer = new ::uint8_t[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
	m_pAudioBufferTemp = new ::uint8_t[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];

    return true;
}

int MovieDecoder::getFrameHeight()
{
    return m_pVideoCodecContext ? m_pVideoCodecContext->height : -1;
}

int MovieDecoder::getFrameWidth()
{
    return m_pVideoCodecContext ? m_pVideoCodecContext->width : -1;
}

double MovieDecoder::getVideoClock()
{
    return m_VideoClock;
}

double MovieDecoder::getAudioClock()
{
    return m_AudioClock;
}

float MovieDecoder::getProgress()
{
    if (m_pFormatContext)
    {
        return static_cast<float>(m_AudioClock / (m_pFormatContext->duration / AV_TIME_BASE));
    }

    return 0.f;
}

float MovieDecoder::getDuration()
{
    return static_cast<float>(m_pFormatContext->duration / AV_TIME_BASE);
}

void MovieDecoder::seek(int offset)
{
    ::int64_t timestamp = (::int64_t)((m_AudioClock * AV_TIME_BASE) + (AV_TIME_BASE * offset));
    int flags = offset < 0 ? AVSEEK_FLAG_BACKWARD : 0;

    if (timestamp < 0)
    {
        timestamp = 0;
    }

    ci::app::console() << "av_gettime: " << timestamp << " timebase " << AV_TIME_BASE << " " << flags << endl;
    int ret = av_seek_frame(m_pFormatContext, -1, timestamp, flags);

    if (ret >= 0)
    {
        m_AudioClock = (double) timestamp / AV_TIME_BASE;
        m_VideoClock = m_AudioClock;
        mutex::scoped_lock videoLock(m_VideoQueueMutex);
        mutex::scoped_lock audioLock(m_AudioQueueMutex);

        clearQueue(m_AudioQueue);
        clearQueue(m_VideoQueue);

        avcodec_flush_buffers(m_pFormatContext->streams[m_VideoStream]->codec);
        avcodec_flush_buffers(m_pFormatContext->streams[m_AudioStream]->codec);
    }
    else
    {
        ci::app::console() << "Error seeking to position: " << timestamp << endl;
    }
}

bool MovieDecoder::decodeVideoFrame(VideoFrame& frame)
{
    AVPacket    packet;
    bool        frameDecoded = false;

    do
    {
        if (!popVideoPacket(&packet))
        {
            return false;
        }
        frameDecoded = decodeVideoPacket(packet);
    } while (!frameDecoded);

    if (m_pFrame->interlaced_frame)
    {
        avpicture_deinterlace((AVPicture*) m_pFrame, (AVPicture*) m_pFrame,
                              m_pVideoCodecContext->pix_fmt,
                              m_pVideoCodecContext->width,
                              m_pVideoCodecContext->height);
    }

	try {
		if (m_pVideoCodecContext->pix_fmt != PIX_FMT_YUV420P && m_pVideoCodecContext->pix_fmt != PIX_FMT_YUVJ420P)
			convertVideoFrame(PIX_FMT_YUV420P);
	}
	catch(std::exception &e) {
		return false;
	}

    m_VideoClock = packet.dts * av_q2d(m_pVideoStream->time_base);

    frame.setWidth(getFrameWidth());
    frame.setHeight(getFrameHeight());

    frame.storeYPlane(m_pFrame->data[0], m_pFrame->linesize[0]);
    frame.storeUPlane(m_pFrame->data[1], m_pFrame->linesize[1]);
    frame.storeVPlane(m_pFrame->data[2], m_pFrame->linesize[2]);
    frame.setPts(m_VideoClock);

    return frameDecoded;
}

void MovieDecoder::convertVideoFrame(PixelFormat format)
{
    SwsContext* scaleContext = sws_getContext(m_pVideoCodecContext->width, m_pVideoCodecContext->height,
                                              m_pVideoCodecContext->pix_fmt, m_pVideoCodecContext->width, m_pVideoCodecContext->height,
                                              format, 0, NULL, NULL, NULL);

    if (NULL == scaleContext)
        throw logic_error("MovieDecoder: Failed to create resize context");

    AVFrame* convertedFrame = NULL;
    createAVFrame(&convertedFrame, m_pVideoCodecContext->width, m_pVideoCodecContext->height, format);

    sws_scale(scaleContext, m_pFrame->data, m_pFrame->linesize, 0, m_pVideoCodecContext->height,
              convertedFrame->data, convertedFrame->linesize);
    sws_freeContext(scaleContext);

    av_free(m_pFrame);
    m_pFrame = convertedFrame;
}

void MovieDecoder::createAVFrame(AVFrame** avFrame, int width, int height, PixelFormat format)
{
    *avFrame = avcodec_alloc_frame();

    int numBytes = avpicture_get_size(format, width, height);
    uint8_t* pBuffer = new uint8_t[numBytes];

    avpicture_fill((AVPicture*) *avFrame, pBuffer, format, width, height);
}

bool MovieDecoder::decodeVideoPacket(AVPacket& packet)
{
    int frameFinished;
	mutex::scoped_lock lock(m_DecodeVideoMutex);
    int bytesDecoded = avcodec_decode_video2(m_pVideoCodecContext, m_pFrame, &frameFinished, &packet);
    av_free_packet(&packet);
    if (bytesDecoded < 0)
    {
        ci::app::console() << "Failed to decode video frame: bytesDecoded < 0" << endl;
    }

    return frameFinished > 0;
}


bool MovieDecoder::decodeAudioFrame(AudioFrame& frame)
{
    AVPacket    packet;
    int         bytesRemaining;
    //::uint8_t*  rawData;
    int         dataSize = (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2;
    bool        frameDecoded = false;

    if (!popAudioPacket(&packet))
    {
        return false;
    }

    bytesRemaining = packet.size;
    //rawData = packet.data;
    
    int bufferSize = (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2;

    while(bytesRemaining > 0)
    {
        int bytesDecoded;
		
		if(m_pSwrContext)
		{
			mutex::scoped_lock lock(m_DecodeAudioMutex);
            bytesDecoded = avcodec_decode_audio3(m_pAudioStream->codec,
												(::int16_t *)m_pAudioBufferTemp,
                                                &bufferSize, &packet);
		}
		else
        {
            mutex::scoped_lock lock(m_DecodeAudioMutex);
            bytesDecoded = avcodec_decode_audio3(m_pAudioStream->codec,
												(::int16_t *)m_pAudioBuffer,
                                                &bufferSize, &packet);
        }

        if (bytesDecoded < 0)
        {
            ci::app::console() << "Error decoding audio frame" << endl;
            break;
        }

        //rawData += bytesDecoded;
        bytesRemaining -= bytesDecoded;
        if (dataSize <= 0)
        {
            continue;
        }

		if(m_pSwrContext)
		{
			swr_convert(m_pSwrContext, &m_pAudioBuffer, bufferSize, (const uint8_t**) &m_pAudioBufferTemp, bufferSize); 
		}

		frameDecoded = true;
        m_AudioClock = packet.pts * av_q2d(m_pAudioStream->time_base);

        frame.setDataSize(bufferSize);
        frame.setFrameData(m_pAudioBuffer);
        frame.setPts(m_AudioClock);		
    }

    av_free_packet(&packet);

    return frameDecoded;
}

void MovieDecoder::readPackets()
{
    AVPacket    packet;
    bool        framesAvailable = true;

    while(framesAvailable && !m_Stop)
    {
        if ((int) m_VideoQueue.size() >= m_MaxVideoQueueSize ||
            (int) m_AudioQueue.size() >= m_MaxAudioQueueSize)
        {
            //std::this_thread::sleep_for(std::chrono::milliseconds(25));
			xtime xt;
			xtime_get(&xt, TIME_UTC_);
			xt.nsec += 25000000; // 25 msec
			m_pPacketReaderThread->sleep(xt);
            
        }
        else if (av_read_frame(m_pFormatContext, &packet) >= 0)
        {
            if (packet.stream_index == m_VideoStream)
            {
                queueVideoPacket(&packet);
            }
            else if (packet.stream_index == m_AudioStream)
            {
                queueAudioPacket(&packet);
            }
            else
            {
                av_free_packet(&packet);
            }
        }
        else
        {
            framesAvailable = false;
        }
    }
}

void MovieDecoder::start()
{
    m_Stop = false;
    if (!m_pPacketReaderThread)
    {
        m_pPacketReaderThread = new thread(boost::bind(&MovieDecoder::readPackets, this));
    }
}

void MovieDecoder::stop()
{
    m_VideoClock = 0;
    m_AudioClock = 0;
    m_Stop = true;
    if (m_pPacketReaderThread)
    {
        m_pPacketReaderThread->join();
        delete m_pPacketReaderThread;
        m_pPacketReaderThread = NULL;
    }

    clearQueue(m_AudioQueue);
    clearQueue(m_VideoQueue);
}

bool MovieDecoder::queueVideoPacket(AVPacket* packet)
{
    mutex::scoped_lock lock(m_VideoQueueMutex);
    return queuePacket(m_VideoQueue, packet);
}

bool MovieDecoder::queueAudioPacket(AVPacket* packet)
{
    mutex::scoped_lock lock(m_AudioQueueMutex);
    return queuePacket(m_AudioQueue, packet);
}

bool MovieDecoder::queuePacket(queue<AVPacket>& packetQueue, AVPacket* packet)
{
    if (av_dup_packet(packet) < 0)
    {
        return false;
    }
    packetQueue.push(*packet);

    return true;
}

bool MovieDecoder::popPacket(queue<AVPacket>& packetQueue, AVPacket* packet)
{
    if (packetQueue.empty())
    {
        return false;
    }

    *packet = packetQueue.front();
    packetQueue.pop();

    return true;
}

void MovieDecoder::clearQueue(std::queue<AVPacket>& packetQueue)
{
    while(!packetQueue.empty())
    {
        packetQueue.pop();
    }
}

bool MovieDecoder::popAudioPacket(AVPacket* packet)
{
    mutex::scoped_lock lock(m_AudioQueueMutex);
    return popPacket(m_AudioQueue, packet);
}

bool MovieDecoder::popVideoPacket(AVPacket* packet)
{
    mutex::scoped_lock lock(m_VideoQueueMutex);
    return popPacket(m_VideoQueue, packet);
}

double MovieDecoder::getAudioTimeBase()
{
    return m_pAudioStream ? av_q2d(m_pAudioStream->time_base) : 0.0;
}

AudioFormat MovieDecoder::getAudioFormat()
{
    AudioFormat format;

    switch(m_pAudioCodecContext->sample_fmt)
    {
    case AV_SAMPLE_FMT_U8:
        format.bits = 8;
        break;
    case AV_SAMPLE_FMT_S16:
        format.bits = 16;
        break;
    case AV_SAMPLE_FMT_S32:
        format.bits = 32;
        break;
	/*case AV_SAMPLE_FMT_FLTP:
		format.bits = 16;

		// we have to resample the audio, so let's setup libswresample now
		m_pSwrContext = swr_alloc_set_opts(m_pSwrContext, 
			m_pAudioCodecContext->channel_layout,
			AV_SAMPLE_FMT_S16,
			m_pAudioCodecContext->sample_rate,
			m_pAudioCodecContext->channel_layout,
			AV_SAMPLE_FMT_FLTP,
			m_pAudioCodecContext->sample_rate,
			0,
			NULL);

		swr_init(m_pSwrContext);

		break;*/
    default:
        format.bits = 0;
    }

    format.rate             = m_pAudioCodecContext->sample_rate;
    format.numChannels      = m_pAudioCodecContext->channels;
    format.framesPerPacket  = m_pAudioCodecContext->frame_size;

    ci::app::console() << "Audio format: rate (" <<  format.rate << ") numChannels (" << format.numChannels << ")" << endl;

    return format;
}

