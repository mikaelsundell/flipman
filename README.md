Flipman
==================

[![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg?style=flat-square)](https://github.com/mikaelsundell/brawtool/blob/master/README.md)

Introduction
------------

This project is a purely experimental exploration of Qt's accelerated 3D API and its shader abstraction layer, applied to QuickTime video playback and macOS Mach timing APIs. It serves as a sandbox for testing concepts, including real-time rendering, integration of QuickTime with Qt frameworks, and precise time synchronization using macOS Mach timers and SMPTE timecode generation.


Application
------------

todo: Insert application image here

### Functionality

todo: Insert application functionality here


API Reference
------------

### Audio Video

### Utility Classes
| Class | Description |
|-------|------------|
| [`av::Fps`](core/fps.h) | Manages frame rate (FPS) calculations. |
| `av::Parameters` | Stores and handles AV parameters. |
| `av::SmpteTime` | Represents SMPTE timecode format. |
| `av::Time` | General-purpose time management. |
| `av::TimeRange` | Defines a range of time intervals. |

### Media Classes
| Class | Description |
|-------|------------|
| `av::Media` | Base class for media handling. |
| `av::MediaReader` | Reads media files. |
| `av::MediaWriter` | Writes media files. |
| `av::MetaData` | Stores and manages metadata. |
| `av::Sidecar` | Handles sidecar files for metadata. |

### Render Classes
| Class | Description |
|-------|------------|
| `av::RenderFilter` | Applies filters to rendered media. |
| `av::RenderLayer` | Manages render layers. |

### Timeline Classes
| Class | Description |
|-------|------------|
| `av::Clip` | Represents a timeline clip. |
| `av::Timeline` | Manages timeline structure. |
| `av::Track` | Defines media tracks. |
| `av::Timer` | Handles time-based operations. |

### Core

### Audio Classes
| Class | Description |
|-------|------------|
| `core::AudioBuffer` | Stores audio data in a buffer. |
| `core::AudioFormat` | Defines audio format specifications. |

### Error Classes
_(Descriptions needed)_

### File Classes
| Class | Description |
|-------|------------|
| `core::File` | Handles file operations. |
| `core::FileRange` | Manages file ranges. |

### Image Classes
| Class | Description |
|-------|------------|
| `core::ImageBuffer` | Stores image data in a buffer. |
| `core::ImageFormat` | Defines image format specifications. |

### Platform
| Class | Description |
|-------|------------|
| `platform::Platform` | Provides platform-specific utilities. |

### Plugins
| Class | Description |
|-------|------------|
| `plugin::BrawReader` | Reads Blackmagic RAW files. |
| `plugin::OiioReader` | Reads images via OpenImageIO. |
| `plugin::OiioWriter` | Writes images via OpenImageIO. |
| `plugin::QtReader` | Reads Qt-supported formats. |
| `plugin::QuicktimeReader` | Reads QuickTime media. |
| `plugin::QuicktimeWriter` | Writes QuickTime media. |

### Widgets
| Class | Description |
|-------|------------|
| `widgets::RenderWidget` | Widget for rendering media. |
| `widgets::TimeEdit` | Time editing UI component. |
| `widgets::Timeline` | Timeline UI widget. |


References
-------------

* AVFoundation   
https://developer.apple.com/documentation/avfoundation/

* Apple Technotes SMPTE   
https://developer.apple.com/library/archive/technotes/tn2310/_index.html

* Apple Mach Absolute Time Units   
https://developer.apple.com/library/archive/qa/qa1398/_index.html

* BlackmagicRAW SDK    
https://documents.blackmagicdesign.com/DeveloperManuals/BlackmagicRAW-SDK.pdf

* OpenImageIO Github    
https://github.com/AcademySoftwareFoundation/OpenImageIO

* QRhiWidget   
https://doc.qt.io/qt-6/qrhiwidget.html

* Qt GUI Private C++ Classes   
https://doc.qt.io/qt-6/qtguiprivate-module.html


Project
-------

* GitHub page   
https://github.com/mikaelsundell/flipman

* Issues   
https://github.com/mikaelsundell/flipman/issues

