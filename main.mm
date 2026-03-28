#include <QApplication>
#include <QInputDialog>
#include <QMessageBox>
#include "CameraWorker.h"
#include "CameraWidget.h"

#import <AVFoundation/AVFoundation.h>

static bool requestCameraPermission()
{
    __block bool granted = false;
    AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];

    if (status == AVAuthorizationStatusAuthorized) {
        return true;
    }

    if (status == AVAuthorizationStatusDenied || status == AVAuthorizationStatusRestricted) {
        return false;
    }

    // Not determined - request permission
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL g) {
        granted = g;
        dispatch_semaphore_signal(sem);
    }];
    // Process events while waiting so the permission dialog can show
    while (dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC)) != 0) {
        QApplication::processEvents();
    }
    return granted;
}

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
