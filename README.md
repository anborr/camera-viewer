# Camera Viewer

A native macOS camera application built with **C++17**, **Qt6** and **FFmpeg**. Captures video from any connected camera (built-in, external, or Continuity Camera) and displays it in a resizable window.

## Features

- **Camera selection** - choose from all available video devices at startup
- **Configurable settings** - resolution, framerate and pixel format are queried from the hardware and presented in a settings dialog
- **Live preview** - real-time video feed with aspect-ratio-preserving scaling
- **macOS integration** - proper app bundle with camera permission handling

## Prerequisites

- macOS 12 or later
- [Homebrew](https://brew.sh)
- Xcode Command Line Tools (`xcode-select --install`)

## Installation

Install the dependencies:

```bash
brew install ffmpeg qt
```

## Build

```bash
git clone https://github.com/anborr/camera-viewer.git
cd camera-viewer
mkdir -p build && cd build
cmake .. -DCMAKE_PREFIX_PATH="$(brew --prefix qt);$(brew --prefix ffmpeg)"
make -j$(sysctl -n hw.ncpu)
```

## Run

```bash
open camera-viewer.app
```

On first launch, macOS will prompt for camera access. The app then presents two dialogs in sequence:

1. **Camera Selection** - pick from the list of detected video devices
2. **Camera Settings** - configure resolution, framerate and pixel format

After confirming, the live camera feed is displayed.

## Project Structure

```
camera-viewer/
├── CMakeLists.txt              # Build configuration
├── Info.plist                   # macOS app bundle metadata
├── main.cpp                     # Entry point and startup flow
├── CameraWorker.h/cpp           # FFmpeg capture and frame decoding
├── CameraWidget.h/cpp           # Qt display widget
├── CameraSettingsDialog.h/cpp   # Settings UI with capability query
├── CameraSettings.h             # Settings data structures
├── CameraPermission.h/mm        # macOS camera permission (Obj-C++)
└── build/                       # Build output (gitignored)
```

## Technical Notes

- **Frame capture** runs on the main thread via `QTimer` because AVFoundation requires an active CFRunLoop to deliver frames.
- **Capability discovery** queries the camera's supported modes by parsing AVFoundation's log output through FFmpeg's `av_log_set_callback`.
- The app is built as a **macOS bundle** with `NSCameraUsageDescription` in `Info.plist` to enable the system permission dialog.

## License

MIT
