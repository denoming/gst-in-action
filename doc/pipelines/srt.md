# SRT

## Introduction

SRT is a means of sending AV between to servers.
SRT has a client-server relationship. One side must be the server, the other the client.
The server can either be the side that is sending the audio/video (pushing) or the side that is
receiving (pulling). The server must have started exist before the client.

## Pipelines

### Client receiving from server

Server part:
```shell
$ gst-launch-1.0 -v videotestsrc ! "video/x-raw, height=360, width=640" ! \
  videoconvert ! x264enc tune=zerolatency ! "video/x-h264, profile=high" ! mpegtsmux ! \
  srtsink uri="srt://:8888"
```

Client part:
```shell
$ gst-launch-1.0 -v srtsrc uri="srt://127.0.0.1:8888" ! decodebin ! videoconvert ! ximagesink
```
or using `playbin`:
```shell
$ gst-launch-1.0 playbin uri=srt://127.0.0.1:8888
```

### Client sending to server

Server part:
```shell
$ gst-launch-1.0 -v srtserversrc uri="srt://127.0.0.1:8889" ! decodebin ! videoconvert ! autovideosink
```

Client part:
```shell
$ gst-launch-1.0 -v videotestsrc ! "video/x-raw, height=360, width=640" ! \
  videoconvert ! clockoverlay font-desc="Sans, 48"  ! x264enc tune=zerolatency ! \
  "video/x-h264, profile=high" ! mpegtsmux ! srtclientsink uri="srt://:8889"
```

