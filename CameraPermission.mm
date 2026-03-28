#import <AVFoundation/AVFoundation.h>
#include "CameraPermission.h"

bool requestCameraPermission()
{
    __block bool granted = false;
    AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];

    if (status == AVAuthorizationStatusAuthorized) {
        return true;
    }

    if (status == AVAuthorizationStatusDenied || status == AVAuthorizationStatusRestricted) {
        return false;
    }

    // Not determined - request permission synchronously
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL g) {
        granted = g;
        dispatch_semaphore_signal(sem);
    }];
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
    return granted;
}
