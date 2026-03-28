cmake_minimum_required(VERSION 3.16)
project(camera-viewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
enable_language(OBJCXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core)
find_package(PkgConfig REQUIRED)

pkg_check_modules(AVFORMAT REQUIRED IMPORTED_TARGET libavformat)
pkg_check_modules(AVCODEC  REQUIRED IMPORTED_TARGET libavcodec)
pkg_check_modules(AVUTIL   REQUIRED IMPORTED_TARGET libavutil)
pkg_check_modules(SWSCALE  REQUIRED IMPORTED_TARGET libswscale)
pkg_check_modules(AVDEVICE REQUIRED IMPORTED_TARGET libavdevice)

add_executable(camera-viewer MACOSX_BUNDLE
    main.cpp
    CameraWorker.cpp
    CameraWidget.cpp
    CameraSettingsDialog.cpp
    CameraPermission.mm
)

set_target_properties(camera-viewer PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist"
)

target_link_libraries(camera-viewer PRIVATE
    Qt6::Widgets
    Qt6::Gui
    Qt6::Core
    PkgConfig::AVFORMAT
    PkgConfig::AVCODEC
    PkgConfig::AVUTIL
    PkgConfig::SWSCALE
    PkgConfig::AVDEVICE
    "-framework AVFoundation"
)
