#pragma once

#include <QDialog>
#include <QComboBox>
#include "CameraSettings.h"

class CameraSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit CameraSettingsDialog(int deviceIndex, const QString &deviceName, QWidget *parent = nullptr);

    CameraSettings settings() const;

private:
    void queryCapabilities(int deviceIndex);

    QComboBox *m_resolutionCombo;
    QComboBox *m_framerateCombo;
    QComboBox *m_pixelFormatCombo;
    QList<CameraMode> m_modes;
    QStringList m_pixelFormats;

private slots:
    void onResolutionChanged();
};
