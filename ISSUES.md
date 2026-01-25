Issues
==================

Ongoing issues and development notes for testing and verification.

Ongoing changes (USA trip)
------------
  - Restructure project code base, make it easier to navigate
  - Change to OIIO convert type in APIs
  - Add OIIO plugins
  - In the example start to add Resolve like images to fill the timeline
  - Start to work on some sort of documentation
  - Implement basic rendereffect, audiofilter for processing on GPUs
  - Get a styled App up and running ASAP
    - Costa Rica look, see iPhots

  - Check all classes and update
    - order of the headers (sdk first)
    - inherits from Object, Container or Plugin

  - Add in macros for shared_data_ptr<> and scoped_data_ptr<>

  - objects => stored data (is_valid, reset)
  - containers => collect potenial errors from models? (error, reset)
  - plugins => generates data from io using plugins (error)

  - You should always be able to propagate errors up the stream, if you ask a clip for it's error it should scan all contained objects for errors.

  - Investigate scoped and shared ptrs => lägg till macro/ types.h eller liknande med macros
  

  Fixes
  * if we reset a track, will we also release it's clips? Pointers and parent ownership
  * changed name from container to processor?
  * add origin to clip, add more tests to it.
  * renderlayer will deliver per frame audio and image?
  * remove all empty constructors in privates?
  * change clang for "    : p(other.p)" <= indentation not need for init vars?
  * error on the audio filter can be when returning the code, it does not exist or it can't be compiled etc
  * we init all structs directly so that we can call either init()
  * define a macro in core::object object_share_ptr<class> => ...? same for container_ptr<class> => ?

  * create a LUT from CIEXYZ => ARRI => Applied D50 ...
  * time detach before 

  * uses QSG_RHI_DEBUG for debugging of Metal code


Upcoming
------------

Urgent
  - Rhi renderer engine to render without widget
  - Consistent use of error() + reset() across classes
    - reset next to close()
    - reset will p.reset()
    - see timeline design

  - OIIO resize and type conversion functions instead of copy of code
  - Parent core::Object for future use
  - Fix data/*.mov files, timecode export 29 97 04-05-03_10.mov is not correct

  - Platform init is initializing power API functions on mac several times
  - core::File for output files, if an image already exists it will create a new .#### pattern

Medium, needs to be fixed:
  - Timeline, media and layers test with filter
  - Evaluate av::Media design, timeline will handle multi-threading
  - "force16bit" in QtWriter test
  - Add in icon

- Slow cooking
  - Check all utility classes for
    - Valid/ minium set of operators =/!= and possibly </> and +/-
    - Have reset and is_valid()
    - Move all insert/setters/getters to above operators

  - Refactor QuicktimeReader, read native Apple buffer type (not CPU processing)
  - Image and Audio Buffers in av::*
  - New test with reading Quicktime and writing OIIO image buffers
  - New application up and running again with new classes
  - Add application image and functionality to README.md
  - MediaReader/ MediaWriter overrides, check inheritance
  - Second check on av::Time and timescale conversion, preserve original values or recreate on write?
  - Push to master!

Experimental (out-dated)
------------

- Test line drawings, grids, borders and aspect ratios
- Test frame scrubbing with custom paint event for the timeline
- Test double buffer reading and multi threading
- Test fps other than media fps
- Test texture readback from render target for status bar stats
- Test in and out markers
- Test hud elements and drawing text
- Test recompilation of shaders during playback
- Test filter and kernel based shaders like gaussian
- Test shuffle and soft scrolling in fullscreen and normal mode
- Test drag window for zoom-in
- Test stylesheet
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

In-development (out-dated)
------------

- Separate UI and loading to *Changed methods if more than one widget (start, end etc), possible use a timeline struct
- End of reader generate invalid read output
- /Qt6.8/Docs/Qt-6.8.1/qtgui/qrhi.html
  - see offline example of rendering
- Test sidecar metadata
  - top and bottom metadata selection, write in order of appearance
    - like camera data in top and show info in bottom
- audio video icons
  https://www.flaticon.com/packs/audio-video-2


Goot to know
------------

- qshareddata.h:149:50 Member access into incomplete type 'classname'
  Destructor missing for classname
  Must have at least operator=
  Must have a copy constructor
  All member variables most also be copyable (have copy constructors if shared objects etc)
