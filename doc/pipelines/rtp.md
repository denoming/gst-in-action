# RTP

## Pipelines

Send AV over RTP:
```shell
$ gst-launch-1.0 -v videotestsrc ! "video/x-raw,framerate=20/1" ! \
   videoscale ! videoconvert ! x264enc tune=zerolatency bitrate=500 speed-preset=superfast ! \
   rtph264pay ! udpsink host=127.0.0.1 port=5000
```

Receive AV via RTP:
```shell
$ gst-launch-1.0 -v udpsrc port=5000 caps="application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96" ! \
  rtph264depay ! decodebin ! videoconvert ! autovideosink
```
