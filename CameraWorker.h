#pragma once

#include <QObject>
#include <QImage>
#include <QStringList>
#include <QTimer>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class CameraWorker : public QObject {
    Q_OBJECT
public:
    explicit CameraWorker(int deviceIndex, QObject *parent = nullptr);
    ~CameraWorker();

    static QStringList listDevices();

    bool open();
    void close();

signals:
    void frameReady(const QImage &image);
    void errorOccurred(const QString &message);

private slots:
    void grabFrame();

private:
    void cleanup();

    int m_deviceIndex;
    QTimer m_timer;
    AVFormatContext *m_formatCtx = nullptr;
    AVCodecContext *m_codecCtx = nullptr;
    SwsContext *m_swsCtx = nullptr;
    AVPacket *m_packet = nullptr;
    AVFrame *m_frame = nullptr;
    AVFrame *m_rgbFrame = nullptr;
    uint8_t *m_buffer = nullptr;
    int m_videoStreamIndex = -1;
    int m_width = 0;
    int m_height = 0;
};
