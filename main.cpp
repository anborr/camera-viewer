#include <QApplication>
#include <QInputDialog>
#include <QMessageBox>
#include "CameraPermission.h"
#include "CameraWorker.h"
#include "CameraWidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    if (!requestCameraPermission()) {
        QMessageBox::critical(nullptr, "Camera Viewer",
            "Camera access denied. Please enable camera access in\n"
            "System Settings > Privacy & Security > Camera.");
        return 1;
    }

    QStringList devices = CameraWorker::listDevices();
    if (devices.isEmpty()) {
        QMessageBox::critical(nullptr, "Camera Viewer", "No cameras found.");
        return 1;
    }

    bool ok = false;
    QString selected = QInputDialog::getItem(
        nullptr, "Camera Selection", "Select a camera:",
        devices, 0, false, &ok
    );

    if (!ok)
        return 0;

    int deviceIndex = devices.indexOf(selected);

    CameraWidget widget(deviceIndex);
    widget.show();

    return app.exec();
}
