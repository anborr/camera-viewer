#include "CameraWorker.h"
#include <QDebug>
#include <QRegularExpression>

CameraWorker::CameraWorker(int deviceIndex, QObject *parent)
    : QObject(parent), m_deviceIndex(deviceIndex)
{
    connect(&m_timer, &QTimer::timeout, this, &CameraWorker::grabFrame);
}

CameraWorker::~CameraWorker()
{
    close();
}

QStringList CameraWorker::listDevices()
{
    avdevice_register_all();

    const AVInputFormat *inputFormat = av_find_input_format("avfoundation");
    if (!inputFormat)
        return {};

    AVFormatContext *tmpCtx = avformat_alloc_context();
    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "list_devices", "true", 0);

    QStringList devices;

    struct LogCapture {
        QStringList *devices;
        bool inVideoSection;
    };
    static LogCapture capture;
    capture.devices = &devices;
    capture.inVideoSection = false;

    av_log_set_callback([](void *, int, const char *fmt, va_list vl) {
        char buf[1024];
        vsnprintf(buf, sizeof(buf), fmt, vl);
        QString line = QString::fromUtf8(buf).trimmed();

        if (line.contains("AVFoundation video devices:")) {
            capture.inVideoSection = true;
            return;
        }
        if (line.contains("AVFoundation audio devices:")) {
            capture.inVideoSection = false;
            return;
        }
        if (capture.inVideoSection) {
            QRegularExpression re(R"(\[(\d+)\]\s+(.+))");
            auto match = re.match(line);
            if (match.hasMatch()) {
                capture.devices->append(match.captured(2));
            }
        }
    });

    avformat_open_input(&tmpCtx, "", inputFormat, &opts);
    av_dict_free(&opts);
    if (tmpCtx)
        avformat_close_input(&tmpCtx);

    av_log_set_callback(av_log_default_callback);

    return devices;
}

bool CameraWorker::open()
{
    avdevice_register_all();

    const AVInputFormat *inputFormat = av_find_input_format("avfoundation");
    if (!inputFormat) {
        emit errorOccurred("Could not find avfoundation input format");
        return false;
    }

    AVDictionary *options = nullptr;
    av_dict_set(&options, "framerate", "30", 0);
    av_dict_set(&options, "pixel_format", "uyvy422", 0);

    m_formatCtx = avformat_alloc_context();
    QString deviceStr = QString::number(m_deviceIndex);
    int ret = avformat_open_input(&m_formatCtx, deviceStr.toUtf8().constData(), inputFormat, &options);
    av_dict_free(&options);

    if (ret < 0) {
        char errBuf[256];
        av_strerror(ret, errBuf, sizeof(errBuf));
        emit errorOccurred(QString("Could not open camera: %1").arg(errBuf));
        return false;
    }

    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) {
        emit errorOccurred("Could not find stream info");
        cleanup();
        return false;
    }

    m_videoStreamIndex = -1;
    for (unsigned i = 0; i < m_formatCtx->nb_streams; i++) {
        if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoStreamIndex = static_cast<int>(i);
            break;
        }
    }

    if (m_videoStreamIndex < 0) {
        emit errorOccurred("No video stream found");
        cleanup();
        return false;
    }

    AVCodecParameters *codecPar = m_formatCtx->streams[m_videoStreamIndex]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec) {
        emit errorOccurred("Could not find decoder");
        cleanup();
        return false;
    }

    m_codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_codecCtx, codecPar);

    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        emit errorOccurred("Could not open codec");
        cleanup();
        return false;
    }

    m_width = m_codecCtx->width;
    m_height = m_codecCtx->height;

    m_swsCtx = sws_getContext(
        m_width, m_height, m_codecCtx->pix_fmt,
        m_width, m_height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    if (!m_swsCtx) {
        emit errorOccurred("Could not create SwsContext");
        cleanup();
        return false;
    }

    m_packet = av_packet_alloc();
    m_frame = av_frame_alloc();
    m_rgbFrame = av_frame_alloc();

    int bufSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, m_width, m_height, 1);
    m_buffer = static_cast<uint8_t *>(av_malloc(bufSize));
    av_image_fill_arrays(m_rgbFrame->data, m_rgbFrame->linesize, m_buffer,
                         AV_PIX_FMT_RGB24, m_width, m_height, 1);

    // ~33ms interval for 30fps, non-blocking via event loop
    m_timer.start(1);
    return true;
}

void CameraWorker::grabFrame()
{
    int ret = av_read_frame(m_formatCtx, m_packet);
    if (ret < 0)
        return;

    if (m_packet->stream_index == m_videoStreamIndex) {
        ret = avcodec_send_packet(m_codecCtx, m_packet);
        if (ret >= 0) {
            while (avcodec_receive_frame(m_codecCtx, m_frame) == 0) {
                sws_scale(m_swsCtx,
                          m_frame->data, m_frame->linesize, 0, m_height,
                          m_rgbFrame->data, m_rgbFrame->linesize);

                QImage image(m_rgbFrame->data[0], m_width, m_height,
                             m_rgbFrame->linesize[0], QImage::Format_RGB888);
                emit frameReady(image.copy());
            }
        }
    }

    av_packet_unref(m_packet);
}

void CameraWorker::close()
{
    m_timer.stop();
    cleanup();
}

void CameraWorker::cleanup()
{
    if (m_buffer) {
        av_free(m_buffer);
        m_buffer = nullptr;
    }
    if (m_rgbFrame) {
        av_frame_free(&m_rgbFrame);
        m_rgbFrame = nullptr;
    }
    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }
    if (m_packet) {
        av_packet_free(&m_packet);
        m_packet = nullptr;
    }
    if (m_swsCtx) {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
    if (m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
        m_codecCtx = nullptr;
    }
    if (m_formatCtx) {
        avformat_close_input(&m_formatCtx);
        m_formatCtx = nullptr;
    }
}
