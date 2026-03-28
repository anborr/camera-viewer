#include "CameraWidget.h"
#include "CameraWorker.h"

#include <QPainter>
#include <QMessageBox>

CameraWidget::CameraWidget(int deviceIndex, const CameraSettings &settings, QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("Camera Viewer");

    // Parse resolution for initial window size
    QStringList res = settings.resolution.split('x');
    if (res.size() == 2)
        resize(res[0].toInt(), res[1].toInt());
    else
        resize(1280, 720);

    m_worker = new CameraWorker(deviceIndex, settings, this);
    connect(m_worker, &CameraWorker::frameReady, this, &CameraWidget::updateFrame);
    connect(m_worker, &CameraWorker::errorOccurred, this, &CameraWidget::handleError);
    m_worker->open();
}

CameraWidget::~CameraWidget()
{
    if (m_worker) {
        m_worker->close();
    }
}

void CameraWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if (!m_currentFrame.isNull()) {
        QImage scaled = m_currentFrame.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        int x = (width() - scaled.width()) / 2;
        int y = (height() - scaled.height()) / 2;
        painter.fillRect(rect(), Qt::black);
        painter.drawImage(x, y, scaled);
    } else {
        painter.fillRect(rect(), Qt::black);
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "Waiting for camera...");
    }
}

void CameraWidget::updateFrame(const QImage &image)
{
    m_currentFrame = image;
    update();
}

void CameraWidget::handleError(const QString &message)
{
    QMessageBox::critical(this, "Camera Error", message);
}
