# Mixing

## Introduction

Possible scenario:
* Mixing video (i.e. replacing or overlaying)
* Mixing audio (i.e. replacing or merging audio tracks)
* Mixing video & audio together

## Pipelines

### Mixing video

Picture in picture:
```shell
$ gst-launch-1.0 \
    filesrc location="$SRC2" ! decodebin ! \
      videoconvert ! videoscale ! "video/x-raw, width=640, height=360" ! \
      compositor name=mix sink_0::alpha=1 sink_1::alpha=1 ! videoconvert ! autovideosink \
    filesrc location="$SRC1" ! decodebin ! \
      videoconvert ! videoscale ! "video/x-raw, width=320, height=180" ! mix.
```
or (put a box around using `videobox`)
```shell
$ gst-launch-1.0   \
    filesrc location="$SRC2" ! decodebin ! \
      videoconvert ! videoscale ! "video/x-raw, width=640, height=360" ! \
      compositor name=mix sink_0::alpha=1 sink_1::alpha=1 ! videoconvert ! autovideosink \
    filesrc location="$SRC1" ! decodebin ! videoconvert ! videoscale ! "video/x-raw, width=320, height=180" ! \
      videobox border-alpha=0 top=-10 bottom=-10 right=-10 left=-10 ! \
      mix.
```
or (choosing position)
```shell
$ gst-launch-1.0   \
    filesrc location="$SRC2" ! decodebin ! videoconvert ! videoscale ! "video/x-raw, width=640, height=360" ! \
      compositor name=mix sink_0::alpha=1 sink_1::alpha=1 sink_1::xpos=50 sink_1::ypos=50 ! videoconvert ! autovideosink \
    filesrc location="$SRC1" ! decodebin ! videoconvert ! videoscale ! "video/x-raw, width=320, height=180" ! \
      mix.
```
or (with audio)
```shell
$ gst-launch-1.0 \
    filesrc location="$SRC1" ! qtdemux name=d \
      d.audio_0 ! queue ! decodebin ! audioconvert ! audioresample ! autoaudiosink \
      d.video_0 ! queue ! decodebin ! videoconvert ! videoscale ! "video/x-raw, width=640, height=360" ! \
      compositor name=mix sink_0::alpha=1 sink_1::alpha=1 sink_1::xpos=50 sink_1::ypos=50 ! \
      videoconvert ! autovideosink \
    filesrc location="$SRC2" ! decodebin ! videoconvert ! videoscale ! "video/x-raw, width=320, height=180" ! mix.
```

Compositor with just one source:
```shell
$ gst-launch-1.0   \
    videotestsrc pattern=ball ! decodebin ! compositor name=mix sink_0::alpha=1 ! x264enc ! mux. \
    audiotestsrc ! avenc_ac3 ! mux. \
    mpegtsmux name=mux ! queue ! \
    tcpserversink host=127.0.0.1 port=7001 recover-policy=keyframe sync-method=latest-keyframe sync=false
```

### Mixing audio

Mix two (or more) test audio streams:
```shell
$ gst-launch-1.0 \
    audiomixer name=mix !  audioconvert ! autoaudiosink \
    audiotestsrc freq=400 ! mix. \
    audiotestsrc freq=600 ! mix.
```

Mix two (or more) MP3 files:
```shell
$ gst-launch-1.0 \
    audiomixer name=mix !  audioconvert ! autoaudiosink \
    filesrc location=$AUDIO_SRC1  ! mpegaudioparse ! decodebin ! mix. \
    filesrc location=$AUDIO_SRC2 ! mpegaudioparse ! decodebin ! mix.
```


Mix a test stream with an MP3 file:
```shell
$ gst-launch-1.0 \
    audiomixer name=mix ! audioconvert ! autoaudiosink \
    audiotestsrc is-live=true freq=400 ! audioconvert ! mix. \
    filesrc location=$AUDIO_SRC ! mpegaudioparse ! decodebin ! audioconvert ! mix.
```

### Mixing video & audio

Mix two fake video and audio sources:
```shell
$ gst-launch-1.0 \
    compositor name=videomix ! videoconvert ! autovideosink \
    audiomixer name=audiomix ! audioconvert ! autoaudiosink \
    videotestsrc pattern=ball ! videomix. \
    videotestsrc pattern=pinwheel ! videoscale ! video/x-raw,width=100 ! videomix. \
    audiotestsrc freq=400 ! audiomix. \
    audiotestsrc freq=600 ! audiomix.
```

Mix and stream to TCP:
```shell
# To view: vlc tcp://localhost:7001
$ gst-launch-1.0 \
    mpegtsmux name=mux ! \
    tcpserversink port=7001 host=0.0.0.0 recover-policy=keyframe sync-method=latest-keyframe sync=false \
    compositor name=videomix ! x264enc ! queue2 ! mux. \
    audiomixer name=audiomix ! audioconvert ! audioconvert ! audioresample ! avenc_ac3 ! queue2 ! mux. \
    videotestsrc pattern=ball ! videomix. \
    videotestsrc pattern=pinwheel ! videoscale ! "video/x-raw, width=100" ! videomix. \
    audiotestsrc freq=400 ! audiomix. \
    audiotestsrc freq=600 ! audiomix.
```

Mix AV file with fake video and audio:
```shell
$ gst-launch-1.0 \
    compositor name=videomix ! autovideosink \
    audiomixer name=audiomix !  audioconvert ! autoaudiosink \
    filesrc location=$SRC ! qtdemux name=demux \
    demux.video_0 ! queue2 ! decodebin ! videoconvert ! videoscale ! "video/x-raw, width=640, height=360" ! videomix. \
    demux.audio_0 ! queue2 ! decodebin ! audioconvert ! audioresample ! audiomix. \
    videotestsrc pattern=ball ! videoscale ! "video/x-raw, width=100, height=100" ! videomix. \
    audiotestsrc freq=400 volume=0.1 ! audiomix.
```
or (with sending over TCP)
```shell
# To view: vlc tcp://localhost:7001
gst-launch-1.0 \
    mpegtsmux name=mux ! \
    tcpserversink port=7001 host=0.0.0.0 recover-policy=keyframe sync-method=latest-keyframe sync=false \
    compositor name=videomix ! x264enc ! queue2 ! mux. \
    audiomixer name=audiomix !  audioconvert ! audioconvert ! audioresample ! avenc_ac3 ! queue2 ! mux. \
    filesrc location=$SRC ! qtdemux name=demux \
    demux.video_0 ! queue2 ! decodebin ! videoconvert ! videoscale ! "video/x-raw, width=640, height=360" ! videomix. \
    demux.audio_0 ! queue2 ! decodebin ! audioconvert ! audioresample ! audiomix. \
    videotestsrc pattern=ball ! videoscale ! "video/x-raw, width=100, height=100" ! videomix. \
    audiotestsrc freq=400 volume=0.1 ! audiomix.
```
or (with `uridecodebin`)
```shell
$ gst-launch-1.0 \
    compositor name=videomix ! autovideosink \
    audiomixer name=audiomix !  audioconvert ! autoaudiosink \
    uridecodebin uri=file://$SRC name=demux ! \
    queue2 ! audioconvert ! audioresample ! audiomix. \
    demux. ! queue2 ! decodebin ! videoconvert ! videoscale ! "video/x-raw, width=640, height=360" ! videomix. \
    videotestsrc pattern=ball ! videoscale ! "video/x-raw, width=100, height=100" ! videomix. \
    audiotestsrc freq=400 volume=0.1 ! audiomix.
```
or (with `uridecodebin` and sending over TCP)
```shell
# To view: vlc tcp://localhost:7001
$ gst-launch-1.0 \
    mpegtsmux name=mux ! \
    tcpserversink port=7001 host=0.0.0.0 recover-policy=keyframe sync-method=latest-keyframe sync=false \
    compositor name=videomix ! x264enc ! queue2 ! mux. \
    audiomixer name=audiomix ! audioconvert ! audioresample ! avenc_ac3 ! queue2 ! mux. \
    uridecodebin uri=file://$SRC name=demux ! \
    queue2 ! audioconvert ! audioresample ! audiomix. \
    demux. ! queue2 ! decodebin ! videoconvert ! videoscale ! "video/x-raw, width=640, height=360" ! videomix. \
    videotestsrc pattern=ball ! videoscale ! "video/x-raw, width=100, height=100" ! videomix. \
    audiotestsrc freq=400 volume=0.2 ! audiomix.
```

Mix two AV files:
```shell
$ gst-launch-1.0 \
    compositor name=videomix ! autovideosink \
    audiomixer name=audiomix !  audioconvert ! autoaudiosink \
    uridecodebin uri=file://$SRC1 name=demux1 ! \
    queue2 ! audioconvert ! audioresample ! audiomix. \
    demux1. ! queue2 ! decodebin ! videoconvert ! videoscale ! "video/x-raw, width=640, height=360" ! videomix. \
    uridecodebin uri=file://$SRC2 name=demux2 ! \
    queue2 ! audioconvert ! audioresample ! audiomix. \
    demux2. ! queue2 ! decodebin ! videoconvert ! videoscale ! "video/x-raw, width=320, height=180" ! videomix.
```

### Fading

Fading between sources instead of cutting beetwen source is possible using [dynamically controlled parameters](https://gstreamer.freedesktop.org/documentation/application-development/advanced/dparams.html).
Using these parameters you can tell gStreamer the `from` and `to` values (e.g. 0 and 1 to increase volume), and the time period to do it.
This can be done both with video (temporarily blending using alpha channel) or audio (lowering the volume of one whilst raising it on another).
