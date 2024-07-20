# Network

## Introduction

GStreamer can send and receive audio and video via a network socket, using either UDP or TCP:
* `UDP` is faster but lossy - there is no attempt to resend lost network packets
* `TCP` acknowledges every network packet so is slower, but more reliable.

See also [SRT](srt.md) as an alternative format.

## Pipelines

### UDP

Send an audio test source:
```shell
$ gst-launch-1.0 audiotestsrc ! avenc_ac3 ! mpegtsmux ! rtpmp2tpay ! udpsink host=127.0.0.1 port=7001
```

Send an audio file:
```shell
#
$ gst-launch-1.0 -v filesrc location=$AUDIO_SRC ! mpegaudioparse ! udpsink port=7001
```

Receive audio:
```shell
$ gst-launch-1.0 udpsrc port=7001 ! decodebin ! autoaudiosink
```

Send a test video stream:
```shell
$ gst-launch-1.0 videotestsrc ! decodebin ! x264enc ! rtph264pay ! udpsink port=7001
```

Send a file (video or audio only, not both):
```shell
# The $SRC points to a video file (e.g. a MP4 file)
$ gst-launch-1.0  filesrc location=$SRC ! decodebin ! x264enc ! rtph264pay ! udpsink port=7001
```

Receive a video stream:
```shell
$ gst-launch-1.0 \
    udpsrc port=7001 caps = "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96" ! \
    rtph264depay ! decodebin ! videoconvert ! autovideosink
```

Send a video (UDP stream, as MPEG-2) test stream:
```shell
$ gst-launch-1.0 videotestsrc ! x264enc ! mpegtsmux ! rtpmp2tpay ! udpsink host=127.0.0.1 port=7001
```

Receive a video stream:
```shell
$ gst-launch-1.0 udpsrc port=7001 ! decodebin ! autovideosink
```

Send both a video and audio test source (mixed together):
```shell
$ gst-launch-1.0 \
    videotestsrc ! x264enc ! muxer. \
    audiotestsrc ! avenc_ac3 ! muxer. \
    mpegtsmux name=muxer ! rtpmp2tpay ! udpsink host=127.0.0.1 port=7001
```

Receive both video and audio together:
```shell
$ gst-launch-1.0 \
    udpsrc port=7001 caps="application/x-rtp" ! \
    rtpmp2tdepay ! decodebin name=decoder ! autoaudiosink  decoder. ! autovideosink
```

Example of `SDP` file to receive stream (using VLC):
```shell
$ cat test.sdp
v=0
m=video 5000 RTP/AVP 96
c=IN IP4 127.0.0.1
a=rtpmap:96 H264/90000
$ vlc test.sdp
```

### TCP

Send a test stream:
```shell
$ gst-launch-1.0 \
    audiotestsrc ! \
    avenc_ac3 ! mpegtsmux ! \
    tcpserversink port=7001 host=0.0.0.0
```

Send a file:
```shell
# Make sure $SRC is set to an audio file (e.g. an MP3 file)
gst-launch-1.0 \
    filesrc location=$AUDIO_SRC ! \
    mpegaudioparse ! \
    tcpserversink port=7001 host=0.0.0.0
```

Receive audio stream:
```shell
$ gst-launch-1.0 tcpclientsrc port=7001 host=0.0.0.0 ! decodebin ! autoaudiosink
```

Send video stream:
```shell
$ gst-launch-1.0 videotestsrc ! \
    decodebin ! x264enc ! mpegtsmux ! queue ! \
    tcpserversink port=7001 host=127.0.0.1 recover-policy=keyframe sync-method=latest-keyframe sync=false
```

Send MP4 file (video only):
```shell
$ gst-launch-1.0 \
    filesrc location=$SRC ! decodebin ! x264enc ! mpegtsmux ! queue ! \
    tcpserversink host=127.0.0.1 port=7001 recover-policy=keyframe sync-method=latest-keyframe sync=false
```

Receive, either use VLC (`tcp://localhost:7001`) or this command:
```shell
$ gst-launch-1.0 \
    tcpclientsrc host=127.0.0.1 port=7001 ! \
    decodebin ! videoconvert ! autovideosink sync=false
```

Send a test stream (using `Matroska`):
```shell
$ gst-launch-1.0 \
    videotestsrc is-live=true ! \
    queue ! videoconvert ! x264enc byte-stream=true ! \
    h264parse config-interval=1 ! queue ! matroskamux ! queue leaky=2 ! \
    tcpserversink port=7001 host=0.0.0.0 recover-policy=keyframe sync-method=latest-keyframe sync=false
```

Send a file (video only, using `Matroska`):
```shell
# Make sure $SRC is set to an video file (e.g. an MP4 file)
$ gst-launch-1.0 \
    filesrc location=$SRC ! decodebin ! \
    queue ! videoconvert ! x264enc byte-stream=true ! \
    h264parse config-interval=1 ! queue ! matroskamux ! queue leaky=2 ! \
    tcpserversink port=7001 host=0.0.0.0 recover-policy=keyframe sync-method=latest-keyframe sync=false
```

Receive a video stream (using `Matroska`):
```shell
$ gst-launch-1.0 \
    tcpclientsrc host=0.0.0.0 port=7001 typefind=true do-timestamp=false ! \
    matroskademux ! typefind ! avdec_h264 ! autovideosink
```

Send both audio and video test (using `Matroska`):
```shell
$ gst-launch-1.0 \
    videotestsrc ! decodebin ! x264enc ! muxer. \
    audiotestsrc ! avenc_ac3 ! muxer. \
    mpegtsmux name=muxer ! \
    tcpserversink port=7001 host=0.0.0.0 recover-policy=keyframe sync-method=latest-keyframe sync=false
```

Send audio and video of an MP4 file:
(Note, quite a few `queue2` elements are required)
```shell
$ gst-launch-1.0 \
    filesrc location=$SRC ! \
    qtdemux name=demux \
    demux.video_0 ! queue2 ! decodebin ! x264enc ! queue2 ! muxer. \
    demux.audio_0 ! queue2 ! decodebin ! audioconvert ! audioresample ! avenc_ac3 ! queue2 ! muxer. \
    mpegtsmux name=muxer ! \
    tcpserversink port=7001 host=0.0.0.0 recover-policy=keyframe sync-method=latest-keyframe sync=false
```

Receive, either use VLC (`tcp://localhost:7001`) or this command:
```shell
$ gst-launch-1.0 \
    tcpclientsrc host=127.0.0.1 port=7001 ! \
    decodebin name=decoder ! autoaudiosink  decoder. ! autovideosink
```

Send a video stream to browser (`tcp-receive.html` HTML page is needed):
```shell
gst-launch-1.0 \
        videotestsrc is-live=true ! queue ! \
        videoconvert ! videoscale ! video/x-raw,width=320,height=180 ! \
        clockoverlay shaded-background=true font-desc="Sans 38" ! \
        theoraenc ! oggmux ! tcpserversink host=127.0.0.1 port=9090
```

Send a video and an audio stream together to browser (`tcp-receive.html` HTML page is needed):
```shell
gst-launch-1.0 \
        videotestsrc is-live=true ! queue ! \
        videoconvert ! videoscale ! video/x-raw,width=320,height=180 ! \
        clockoverlay shaded-background=true font-desc="Sans 38" ! \
        theoraenc ! queue2 ! mux. \
        audiotestsrc ! audioconvert ! vorbisenc ! mux. \
        oggmux name=mux ! tcpserversink host=127.0.0.1 port=9090
```

Play a source rather than test (`tcp-receive.html` HTML page is needed):
```shell
$ gst-launch-1.0 \
    filesrc location=$SRC ! \
    qtdemux name=demux \
    demux.audio_0 ! queue ! decodebin ! vorbisenc ! muxer. \
    demux.video_0 ! queue ! decodebin ! \
    videoconvert ! videoscale ! video/x-raw,width=320,height=180 ! \
    theoraenc ! muxer. \
    oggmux name=muxer ! \
    tcpserversink host=127.0.0.1 port=9090 recover-policy=keyframe sync-method=latest-keyframe
```
