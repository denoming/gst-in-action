# Tools

## [gst-launch-1.0](https://gstreamer.freedesktop.org/documentation/tools/gst-launch.html)

Options:
```text
-v, --verbose
Output status information and property notifications
-m, --messages
Output messages posted on the pipeline's bus
-t, --tags
Output tags (also known as metadata)
-T, --trace
Print memory allocation traces (feature must be enabled at compile time to work)
```

Pipeline description:
```text
> Elements
ELEMENTTYPE [PROPERTY1 ...]

> Element Properties
PROPERTY=VALUE ...

> Bins
[BINTYPE.] ([PROPERTY1 ...] PIPELINE-DESCRIPTION)
e.g. gst-launch-1.0 pipeline. \( latency=2000000000 videotestsrc ! jpegenc ! jpegdec ! fakevideosink \)

> Links
[[SRCELEMENT].[PAD1,...]] ! [[SINKELEMENT].[PAD1,...]]
e.g. gst-launch-1.0 textoverlay name=overlay ! videoconvert ! videoscale ! autovideosink
        filesrc location=movie.avi ! decodebin2 !  videoconvert ! overlay.video_sink
        filesrc location=movie.srt ! subparse ! overlay.text_sink

> Links (with caps)
[[SRCELEMENT].[PAD1,...]] ! CAPS ! [[SINKELEMENT].[PAD1,...]]

> Caps
MIMETYPE [, PROPERTY[, PROPERTY ...]]] [; CAPS[; CAPS ...]]

> Caps Properties
NAME=[(TYPE)] VALUE in lists and ranges: [(TYPE)] VALUE
e.g. gst-launch-1.0 videotestsrc ! \
     "video/x-raw,format=(string)YUY2;video/x-raw,format=(string)YV12" ! \
     xvimagesink
Types:
  * i or int for integer values or ranges;
  * f or float for float values or ranges;
  * 4 or fourcc for FOURCC values;
  * b, bool, or boolean for boolean values;
  * s, str, or string for strings;
  * fraction for fractions (framerate, pixel-aspect-ratio);
  * l or list for lists.
```
where `SRCELEMENT` is a particular element name.

## [gst-inspect-1.0](https://gstreamer.freedesktop.org/documentation/tools/gst-inspect.html)

`gst-inspect-1.0` is a tool that prints out information on available GStreamer plugins, information about a particular plugin,
or information about a particular element.

List information about `libgstvpx.so` plugin:
```shell
$ gst-inspect-1.0 /usr/lib/gstreamer-1.0/libgstvpx.so
Plugin Details:
  Name                     vpx
  Description              VP8/VP9 video encoding and decoding based on libvpx
  Filename                 /usr/lib/gstreamer-1.0/libgstvpx.so
  Version                  1.24.3
  License                  LGPL
  Source module            gst-plugins-good
  Documentation            https://gstreamer.freedesktop.org/documentation/vpx/
  Source release date      2024-04-30
  Binary package           Arch Linux GStreamer 1.24.3-1
  Origin URL               https://www.archlinux.org/
...
```

List information about `vp8dec` element:
```shell
$ gst-inspect-1.0 vp8dec
```

## gst-discoverer-1.0

Print all information regarding the media:
```shell
Properties:
  Duration: 0:00:52.250000000
  Seekable: yes
  Live: no
...
  container #0: video/webm
...
    video #1: video/x-vp8, width=(int)854, height=(int)480, framerate=(fraction)0/1
...
      Codec:
        video/x-vp8, width=(int)854, height=(int)480, framerate=(fraction)0/1
      Stream ID: 02e452af592733468311797f6c2e3066ce570853053b968b90fdec7efc380ec4/001:3465118714092722455
      Width: 854
      Height: 480
      Depth: 24
      Frame rate: 0/1
      Pixel aspect ratio: 1/1
      Interlaced: false
      Bitrate: 0
      Max bitrate: 0
...
    audio #2: audio/x-vorbis, channels=(int)2, rate=(int)48000,
...
```
