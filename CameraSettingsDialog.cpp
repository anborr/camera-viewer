#include "CameraSettingsDialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QRegularExpression>
#include <QSet>

extern "C" {
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
}

CameraSettingsDialog::CameraSettingsDialog(int deviceIndex, const QString &deviceName, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Camera Settings");
    setMinimumWidth(350);

    auto *layout = new QVBoxLayout(this);

    auto *header = new QLabel(QString("<b>%1</b>").arg(deviceName));
    layout->addWidget(header);

    m_resolutionCombo = new QComboBox;
    m_framerateCombo = new QComboBox;
    m_pixelFormatCombo = new QComboBox;

    auto *form = new QFormLayout;
    form->addRow("Resolution:", m_resolutionCombo);
    form->addRow("Framerate:", m_framerateCombo);
    form->addRow("Pixel Format:", m_pixelFormatCombo);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    connect(m_resolutionCombo, &QComboBox::currentIndexChanged,
            this, &CameraSettingsDialog::onResolutionChanged);

    queryCapabilities(deviceIndex);

    // Populate resolution combo with unique resolutions
    QStringList seenResolutions;
    for (const auto &mode : m_modes) {
        QString res = mode.resolutionString();
        if (!seenResolutions.contains(res)) {
            seenResolutions.append(res);
            m_resolutionCombo->addItem(res);
        }
    }

    // Select highest resolution by default
    if (m_resolutionCombo->count() > 0)
        m_resolutionCombo->setCurrentIndex(0);

    // Populate pixel formats
    for (const auto &fmt : m_pixelFormats)
        m_pixelFormatCombo->addItem(fmt);

    // Select uyvy422 if available
    int uyvy = m_pixelFormatCombo->findText("uyvy422");
    if (uyvy >= 0)
        m_pixelFormatCombo->setCurrentIndex(uyvy);
}

void CameraSettingsDialog::queryCapabilities(int deviceIndex)
{
    avdevice_register_all();

    const AVInputFormat *inputFormat = av_find_input_format("avfoundation");
    if (!inputFormat)
        return;

    // We trigger an error by opening with an unsupported pixel format.
    // AVFoundation then logs the supported modes and pixel formats.
    struct LogCapture {
        QList<CameraMode> *modes;
        QStringList *pixelFormats;
        bool inModes;
        bool inPixFmts;
    };
    static LogCapture capture;
    capture.modes = &m_modes;
    capture.pixelFormats = &m_pixelFormats;
    capture.inModes = false;
    capture.inPixFmts = false;

    av_log_set_callback([](void *, int, const char *fmt, va_list vl) {
        char buf[1024];
        vsnprintf(buf, sizeof(buf), fmt, vl);
        QString line = QString::fromUtf8(buf).trimmed();

        if (line.contains("Supported modes:")) {
            capture.inModes = true;
            capture.inPixFmts = false;
            return;
        }
        if (line.contains("Supported pixel formats:")) {
            capture.inPixFmts = true;
            capture.inModes = false;
            return;
        }
        if (line.contains("Overriding") || line.contains("selected pixel format")) {
            capture.inModes = false;
            capture.inPixFmts = false;
            return;
        }

        if (capture.inModes) {
            // Parse: "1280x720@[30.000000 30.000000]fps"
            QRegularExpression re(R"((\d+)x(\d+)@\[(\d+\.?\d*)\s+(\d+\.?\d*)\]fps)");
            auto match = re.match(line);
            if (match.hasMatch()) {
                CameraMode mode;
                mode.width = match.captured(1).toInt();
                mode.height = match.captured(2).toInt();
                mode.minFps = match.captured(3).toDouble();
                mode.maxFps = match.captured(4).toDouble();
                capture.modes->append(mode);
            }
        }

        if (capture.inPixFmts) {
            QString fmt = line.trimmed();
            if (!fmt.isEmpty() && !fmt.contains("[") && !capture.pixelFormats->contains(fmt))
                capture.pixelFormats->append(fmt);
        }
    });

    // Open with deliberately wrong pixel format to trigger the log output
    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "pixel_format", "yuv420p", 0);
    AVFormatContext *ctx = nullptr;
    QString deviceStr = QString::number(deviceIndex);
    avformat_open_input(&ctx, deviceStr.toUtf8().constData(), inputFormat, &opts);
    av_dict_free(&opts);
    if (ctx)
        avformat_close_input(&ctx);

    av_log_set_callback(av_log_default_callback);

    // If no modes were found, add a default
    if (m_modes.isEmpty()) {
        m_modes.append({1280, 720, 30, 30});
        m_modes.append({640, 480, 30, 30});
    }
    if (m_pixelFormats.isEmpty()) {
        m_pixelFormats = {"uyvy422", "yuyv422", "nv12", "0rgb", "bgr0"};
    }
}

void CameraSettingsDialog::onResolutionChanged()
{
    m_framerateCombo->clear();
    QString currentRes = m_resolutionCombo->currentText();

    QSet<int> seenFps;
    for (const auto &mode : m_modes) {
        if (mode.resolutionString() == currentRes) {
            int fps = static_cast<int>(mode.maxFps);
            if (!seenFps.contains(fps)) {
                seenFps.insert(fps);
                m_framerateCombo->addItem(QString::number(fps));
            }
        }
    }
}

CameraSettings CameraSettingsDialog::settings() const
{
    CameraSettings s;
    s.resolution = m_resolutionCombo->currentText();
    s.framerate = m_framerateCombo->currentText();
    s.pixelFormat = m_pixelFormatCombo->currentText();
    return s;
}
