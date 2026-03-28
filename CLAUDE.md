# Camera Viewer

## Overview
macOS camera viewer built with C++17, Qt6 (GUI) and FFmpeg (capture). Displays a live video feed from a user-selected camera with configurable resolution, framerate and pixel format.

## Build

```bash
cd build
cmake .. -DCMAKE_PREFIX_PATH="$(brew --prefix qt);$(brew --prefix ffmpeg)"
make -j$(sysctl -n hw.ncpu)
open camera-viewer.app
```

Dependencies: `brew install ffmpeg qt`

## Architecture

### Main Thread Only
All camera capture runs on the main thread via `QTimer` (1ms interval). This is required because AVFoundation needs an active CFRunLoop to deliver frames - a blocking loop on a background thread will hang.

### App Bundle
The app must be built as a macOS bundle (`MACOSX_BUNDLE`) with an `Info.plist` containing `NSCameraUsageDescription`. Without this, macOS silently blocks camera access and `avformat_open_input` hangs indefinitely.

### AVFoundation/FFmpeg Name Conflict
Apple's `AVMediaType` (NSString typedef) conflicts with FFmpeg's `AVMediaType` (enum). These headers must never be included in the same translation unit. Camera permission code lives in `CameraPermission.mm` (ObjC++ only, no FFmpeg headers).

### Device and Capability Discovery
Device enumeration and capability queries work by capturing FFmpeg's AVLog output via `av_log_set_callback`. The capability query deliberately opens a device with an unsupported pixel format to trigger AVFoundation's error log listing supported modes.

## File Map

| File | Role |
|------|------|
| `main.cpp` | Entry point: permission check, camera selection, settings dialog, launch |
| `CameraWorker.h/cpp` | FFmpeg capture: device listing, open/close, timer-based frame grabbing |
| `CameraWidget.h/cpp` | Qt widget: renders frames scaled with aspect ratio on black background |
| `CameraSettingsDialog.h/cpp` | Settings UI: queries and presents available resolutions, FPS, pixel formats |
| `CameraSettings.h` | Data structs: `CameraMode`, `CameraSettings` |
| `CameraPermission.h/mm` | macOS camera permission request (ObjC++) |
| `Info.plist` | App bundle metadata with `NSCameraUsageDescription` |
| `CMakeLists.txt` | Build config: C++17, ObjC++, Qt6, FFmpeg (pkg-config), AVFoundation |

## Key Constraints
- Pixel format `uyvy422` is the preferred default (natively supported by most Mac cameras)
- The FaceTime HD Camera typically supports 1280x720 and 640x480 at 30fps
- `av_read_frame` is called per timer tick - the 1ms interval lets the event loop dispatch frames as fast as the camera provides them
