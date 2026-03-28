#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#include <cstdio>

int main() {
    @autoreleasepool {
        AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
        switch (status) {
            case AVAuthorizationStatusAuthorized:
                printf("Camera: AUTHORIZED\n");
                break;
            case AVAuthorizationStatusDenied:
                printf("Camera: DENIED\n");
                break;
            case AVAuthorizationStatusRestricted:
                printf("Camera: RESTRICTED\n");
                break;
            case AVAuthorizationStatusNotDetermined:
                printf("Camera: NOT DETERMINED - requesting...\n");
                dispatch_semaphore_t sem = dispatch_semaphore_create(0);
                [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
                    printf("Granted: %d\n", granted);
                    dispatch_semaphore_signal(sem);
                }];
                dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, 30 * NSEC_PER_SEC));
                break;
        }
    }
    return 0;
}
