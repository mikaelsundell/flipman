Ongoing:

- Improve concurrent reader reading
- Improve signals from reader for set_* methods
- Improve coding format and .clang-formats
- Improve set_rate
- Fix issues with AV* API's when scubbing quickly
- Test iv viewer, check ImageBuf use and support

Experimental:

- Improve frame seeking in Quicktime qt_reader class
- Test line drawings, grids, borders and aspect ratios
- Test frame scrubbing with custom paint event for the timeline
- Test double buffer reading and multi threading
- Test fps check and drop frames
- Test texture readback from render target for status bar stats
- Test in and out markers
- Test hud elements and drawing text
- Test recompilation of shaders during playback
- Test filter and kernel based shaders like gaussian
- Test shuffle and soft scrolling in fullscreen and normal mode
- Test drag window for zoom-in
- Test stylesheet
- Test playback loop modes
- Test: sidecar json for metadata, filename + json
- Test: Mouse cursor in white/ red and shuffle wheel cursor
- Test: Optimize type conversions, Qt, AVFoundation and Rhi, speed up
- Test: Modify cursor, white/ red-like when using app
- Test: Test audio device output, Qt multimedia classes?
- Test: Store bookmarks and cut-n-paste timeline positions
- Test: Color space of video tracks, ocio conversion
- Test: Add in braw, ffmpeg AVReader interfaces
- Test: Get actual film name not just filename

- Mac: Test device aspect ratio for displays
- Mac: Publish as Github developer signed app in releases
- Mac: Metal profiler in xcode

In-development:

- Separate UI and loading to *Changed methods if more than one widget (start, end etc), pissible use a timeline struct
- End of reader generate invalid read output
- /Qt6.8/Docs/Qt-6.8.1/qtgui/qrhi.html
  - see offline example of rendering
- Test sidecar metadata
  - top and bottom metadata selection, write in order of appearance
    - like camera data in top and show info in bottom
- audio video icons
  https://www.flaticon.com/packs/audio-video-2
