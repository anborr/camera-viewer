#pragma once

#include <QWidget>
#include <QImage>

class CameraWorker;

class CameraWidget : public QWidget {
    Q_OBJECT
public:
    explicit CameraWidget(int deviceIndex, QWidget *parent = nullptr);
    ~CameraWidget();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateFrame(const QImage &image);
    void handleError(const QString &message);

private:
    QImage m_currentFrame;
    CameraWorker *m_worker = nullptr;
};
