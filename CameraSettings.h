#pragma once

#include <QString>
#include <QStringList>

struct CameraMode {
    int width;
    int height;
    double minFps;
    double maxFps;

    QString resolutionString() const {
        return QString("%1x%2").arg(width).arg(height);
    }
    QString fpsString() const {
        if (minFps == maxFps)
            return QString::number(static_cast<int>(maxFps));
        return QString("%1-%2").arg(static_cast<int>(minFps)).arg(static_cast<int>(maxFps));
    }
};

struct CameraSettings {
    QString resolution = "1280x720";
    QString framerate = "30";
    QString pixelFormat = "uyvy422";
};
