# Splitting

## Introduction

This page describes the `tee` element, which allows audio and video streams to be sent to more
than one place.

## Pipelines

Duplicate one video stream into two separate video outputs (windows):
```shell
$ gst-launch-1.0 \
    videotestsrc ! tee name=t \
    t. ! queue ! videoconvert ! autovideosink \
    t. ! queue ! videoconvert ! autovideosink
```

Duplicate one video stream into window and TCP streaming:
(`async=false` is required on both sinks, because the encoding step on the TCP branch takes longer, and so the timing will be different)
```shell
$ gst-launch-1.0 videotestsrc ! \
    decodebin ! tee name=t \
    t. ! queue ! videoconvert ! autovideosink \
    t. ! queue ! videoconvert ! x264enc tune=zerolatency ! mpegtsmux ! tcpserversink port=7001 host=127.0.0.1 recover-policy=keyframe sync-method=latest-keyframe async=false
```
or without reducing quality of x264 encoding specifying queue size:
```
gst-launch-1.0 videotestsrc ! \
    decodebin ! tee name=t \
    t. ! queue max-size-time=3000000000 ! videoconvert ! autovideosink \
    t. ! queue ! videoconvert ! x264enc ! mpegtsmux ! tcpserversink port=7001 host=127.0.0.1 recover-policy=keyframe sync-method=latest-keyframe async=false
```

Combines two audio visualisations (+output sound to default sink):
```shell
gst-launch-1.0 filesrc location=$SRC ! decodebin ! tee name=t ! \
    queue ! audioconvert ! wavescope style=color-lines shade-amount=0x00080402 ! alpha alpha=0.5 ! \
    videomixer name=m background=black ! videoconvert ! autovideosink \
    t. ! queue ! audioconvert ! spacescope style=color-lines shade-amount=0x00080402 ! alpha alpha=0.5 ! m. \
    t. ! queue ! autoaudiosink
```
