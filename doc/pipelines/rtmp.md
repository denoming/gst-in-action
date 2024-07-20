# RTMP

## Introduction

GStreamer can:
* retrieve an RTMP stream from an RTMP server, and
* send an RTMP stream to an RTMP server (including YouTube).

If you need your own RTMP server, [the Nginx RTMP extension](https://github.com/arut/nginx-rtmp-module) works quite well.
[Linode has a good NGINX RTMP installation guide.](https://www.linode.com/docs/guides/set-up-a-streaming-rtmp-server/)

RTMP can be live streams, or on-demand streams - playback is the same in both cases.

## Pipelines

### Receiving a live stream from RTMP server

Play an RTMP stream:
```shell
$ export RTMP_SRC="rtmp://matthewc.co.uk/vod/scooter.flv"
$ gst-launch-1.0 playbin uri=$RTMP_SRC
```
or
```shell
$ gst-launch-1.0 uridecodebin uri=$RTMP_SRC ! autovideosink
```
or
```shell
$ gst-launch-1.0 rtmpsrc location=$RTMP_SRC ! decodebin ! autovideosink
```
or (only audio)
```shell
$ gst-launch-1.0 rtmpsrc name=rtmpsrc location=$RTMP_SRC ! decodebin ! \
  queue ! audioconvert ! autoaudiosink
```
or (using `flvdemux`)
```shell
$ gst-launch-1.0 rtmpsrc location="$RTMP_SRC" ! flvdemux name=d \
    d.audio ! queue ! decodebin ! fakesink \
    d.video ! queue ! decodebin ! autovideosink
```
or (using `flvdemux` with saving to MP4 container)
```shell
$ gst-launch-1.0 -e rtmpsrc location="$RTMP_SRC" ! \
    flvdemux name=d \
    d.audio ! queue ! decodebin ! audioconvert ! faac bitrate=32000 ! mux. \
    d.video ! queue ! decodebin ! videoconvert ! "video/x-raw, format=I420" ! x264enc speed-preset=superfast tune=zerolatency psy-tune=grain sync-lookahead=5 bitrate=480 key-int-max=50 ref=2 ! mux. \
    mp4mux name=mux ! filesink location="out.mp4"
```

Picture-in-picture on top of playing local file:
```shell
$ gst-launch-1.0 \
    filesrc location="$SRC" ! decodebin ! videoconvert ! \
    videoscale ! "video/x-raw, width=640, height=360" ! \
    compositor name=mix sink_0::alpha=1 sink_1::alpha=1 sink_1::xpos=50 sink_1::ypos=50 ! \
    videoconvert ! xvimagesink \
    rtmpsrc location="$RTMP_SRC" ! \
    flvdemux name=d \
      d.audio ! queue ! decodebin ! autoaudiosink \
      d.video ! queue ! decodebin ! videoconvert ! videoscale ! "video/x-raw, width=320, height=180" ! mix.
```

### Sending a live stream to an RTMP server

If you're using [Nginx RTMP](https://github.com/arut/nginx-rtmp-module),
the name configured for your application needs to be the first part of the URL path.

For example, if your Nginx configuration is:
```text
rtmp {
    server {
        listen 1935;
        hunk_size 4096;
        notify_method get;
        application livestream {
            live on;
        }
    }
}
```
then the application name is `livestream`, and so your URL will be `rtmp://<your-domain>/livestream/<stream-name>`
(where `<stream-name> can be anything`).

To send a video test source:
```shell
$ export RTMP_DEST="rtmp://example.com/live/test"
$ gst-launch-1.0 videotestsrc  is-live=true ! \
    queue ! x264enc ! flvmux name=muxer ! rtmpsink location="$RTMP_DEST live=1"
```

To send an audio test source (note: `flvmux` is still required even though there is no muxing of audio & video):
```shell
$ export RTMP_DEST="rtmp://example.com/live/test"
$ gst-launch-1.0 audiotestsrc is-live=true ! \
    audioconvert ! audioresample ! audio/x-raw,rate=48000 ! \
    voaacenc bitrate=96000 ! audio/mpeg ! aacparse ! audio/mpeg, mpegversion=4 ! \
    flvmux name=mux ! \
    rtmpsink location=$RTMP_DEST
```

This sends both video and audio as a test source:
```shell
$ gst-launch-1.0 videotestsrc is-live=true ! \
    videoconvert ! x264enc bitrate=1000 tune=zerolatency ! video/x-h264 ! h264parse ! \
    video/x-h264 ! queue ! flvmux name=mux ! \
    rtmpsink location="$RTMP_DEST" audiotestsrc is-live=true ! \
    audioconvert ! audioresample ! "audio/x-raw, rate=48000" ! \
    voaacenc bitrate=96000 ! "audio/mpeg" ! aacparse ! "audio/mpeg, mpegversion=4" ! mux.
```

### Sending a live stream to YouTube:

YouTube accepts live RTMP streams. They must have both audio and video.

Set up a stream by visiting [YouTube.com](https://www.youtube.com/) on desktop, and selecting 'Create' from the top-right.
YouTube will provide a 'Stream URL' and a 'Stream key'. Combine these to create the full URL.

For example, if the URL is `rtmp://a.rtmp.youtube.com/live2` and the key is `abcd-1234-5678`, then:
```shell
$ export RTMP_DEST="rtmp://a.rtmp.youtube.com/live2/abcd-1234-5678"
```

Streaming pipeline:
```shell
$ gst-launch-1.0 \
    videotestsrc is-live=1 \
    ! videoconvert \
    ! "video/x-raw, width=1280, height=720, framerate=30/1" \
    ! queue \
    ! x264enc cabac=1 bframes=2 ref=1 \
    ! "video/x-h264,profile=main" \
    ! flvmux streamable=true name=mux \
    ! rtmpsink location="${RTMP_DEST} live=1"
    audiotestsrc is-live=1 wave=ticks \
    ! voaacenc bitrate=128000 \
    ! mux.
```

### Sending a file

Audio and video:
```shell
$ gst-launch-1.0 filesrc location=$SRC ! \
    qtdemux name=demux \
    demux.video_0 ! queue ! \
    decodebin ! videoconvert ! x264enc bitrate=1000 tune=zerolatency ! video/x-h264 ! h264parse ! \
    video/x-h264 ! queue ! flvmux name=mux ! \
    rtmpsink location=$RTMP_DEST \
    demux.audio_0 ! queue ! decodebin ! audioconvert ! audioresample ! \
    audio/x-raw,rate=48000 ! \
    voaacenc bitrate=96000 ! audio/mpeg ! aacparse ! audio/mpeg, mpegversion=4 ! mux.
```

Only video:
```shell
$ gst-launch-1.0 filesrc location=$SRC ! \
    qtdemux name=demux \
    demux.video_0 ! queue ! \
    decodebin ! videoconvert ! x264enc bitrate=1000 tune=zerolatency ! video/x-h264 ! h264parse ! \
    video/x-h264 ! queue ! flvmux name=mux ! \
    rtmpsink location=$RTMP_DEST
```

Note: To reduce latency `sync` option must be set to `false`
