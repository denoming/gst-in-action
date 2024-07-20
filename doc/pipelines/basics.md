# Basics

## Introduction

These examples assume that bash variable `SRC` to be set to a video file (e.g. an mp4 file). You can do this by, e.g.
```shell
export SRC=/home/me/videos/test.mp4
```

## Pipelines

Duplicate video playing:
```shell
gst-launch-1.0 filesrc location=$SRC ! qtdemux name=demux demux.video_0 ! queue ! decodebin ! \
  tee name=t ! queue ! videoconvert ! videoscale ! autovideosink \
  t.         ! queue ! videoconvert ! videoscale ! autovideosink
```

Play a video (with audio):
```shell
GST_DEBUG=4 GST_DEBUG_NO_COLOR=1 GST_DEBUG_DUMP_DOT_DIR=$HOME/temp/pipeline.dot \
 gst-launch-1.0 playbin uri=file://$SRC
```
where `playbin` element is a collection of elements.
It can be split into the individual components:
```shell
gst-launch-1.0 filesrc location=$SRC ! \
  qtdemux name=d \
  d.audio_0 ! queue ! decodebin ! audioconvert ! audioresample ! autoaudiosink \
  d.video_0 ! queue ! decodebin ! videoconvert ! videoscale ! autovideosink
```
or (using `matroskademux`)
```shell
$ gst-launch-1.0 souphttpsrc location=https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm ! \
  matroskademux name=d \
  d.video_0 ! queue ! vp8dec ! videoconvert ! autovideosink \
  d.audio_0 ! queue ! vorbisdec ! audioconvert ! audioresample ! autoaudiosink
```

Play a video (without audio):
```shell
gst-launch-1.0 -v uridecodebin uri="file://$SRC" ! autovideosink
```
which could also have been done as:
```shell
gst-launch-1.0 -v filesrc location="$SRC" ! decodebin ! autovideosink
```

Play just the audio from a video:
```shell
gst-launch-1.0 -v uridecodebin uri="file://$SRC" ! autoaudiosink
```

Re-mux without decoding (stripping out audio):
```shell
$ gst-launch-1.0 souphttpsrc location=https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm ! \
  matroskademux name=d d.video_0 ! matroskamux ! filesink location=sintel_video.mkv
```
the same but stripping out video:
```shell
$ gst-launch-1.0 souphttpsrc location=https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm ! \
 matroskademux name=d d.audio_0 ! vorbisparse ! matroskamux ! filesink location=sintel_audio.mka
```
or using caps filter:
```shell
$ gst-launch-1.0 souphttpsrc location=https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm ! \
 matroskademux ! video/x-vp8 ! matroskamux ! filesink location=sintel_video.mkv
```

Add a clock:
```shell
$ gst-launch-1.0 -v filesrc location="$SRC" ! decodebin ! clockoverlay font-desc="Sans, 48" ! videoconvert ! autovideosink
```

Resize video:
```shell
$ gst-launch-1.0 uridecodebin uri=https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm ! \
 queue ! videoscale ! video/x-raw,width=320,height=200 ! videoconvert ! autovideosink
```

Change framerate:
```shell
gst-launch-1.0 -v filesrc location="$SRC" ! decodebin ! videoconvert ! videorate ! video/x-raw,framerate=5/1 ! autovideosink
```

Change the size and framerate (with audio):
```shell
gst-launch-1.0 filesrc location="$SRC" ! \
    qtdemux name=demux \
    demux.audio_0 ! queue ! decodebin ! audioconvert ! audioresample ! autoaudiosink \
    demux.video_0 ! queue ! decodebin ! videoconvert ! videoscale ! videorate ! \
        "video/x-raw, width=(int)320, height=(int)240, framerate=(fraction)30/1" ! autovideosink
```

Play an MP3 audio file:
```shell
# All three of these do the same thing:
gst-launch-1.0 -v playbin uri=file://$SRC
gst-launch-1.0 -v uridecodebin uri="file://$SRC" ! autoaudiosink
gst-launch-1.0 -v filesrc location=$SRC ! mpegaudioparse ! decodebin ! autoaudiosink
```

Concatenate multiple streams gaplessly:
See (https://coaxion.net/blog/2014/08/concatenate-multiple-streams-gaplessly-with-gstreamer/)[https://coaxion.net/blog/2014/08/concatenate-multiple-streams-gaplessly-with-gstreamer/]
