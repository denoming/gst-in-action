# Images

## Introduction

Images can be added to video.
In addition, video can be converted into images.

## Pipelines

Create a video from an image:
```shell
export PIC="https://www.google.com.ua/images/branding/googlelogo/1x/googlelogo_color_272x92dp.png"
gst-launch-1.0 \
    uridecodebin uri="$PIC" ! \
    videoconvert ! \
    imagefreeze ! \
    autovideosink
```

Create a video from multiple images:
```shell
gst-launch-1.0 \
    compositor name=m sink_1::xpos=263 sink_2::ypos=240 sink_3::xpos=263 sink_3::ypos=240 !  autovideosink \
    uridecodebin uri=$PIC ! videoconvert ! videoscale ! video/x-raw, width=263, height=240 ! imagefreeze ! m. \
    uridecodebin uri=$PIC ! videoconvert ! videoscale ! video/x-raw, width=263, height=240 ! imagefreeze ! m. \
    uridecodebin uri=$PIC ! videoconvert ! videoscale ! video/x-raw, width=263, height=240 ! imagefreeze ! m. \
    uridecodebin uri=$PIC ! videoconvert ! videoscale ! video/x-raw, width=263, height=240 ! imagefreeze ! m.
```

Capture an image as PNG:
```shell
gst-launch-1.0 videotestsrc ! pngenc ! filesink location=foo.png
```

Capture an image as JPEG:
```shell
gst-launch-1.0 videotestsrc ! jpegenc ! filesink location=foo.jpg
```

Capturing images every X seconds:
```shell
gst-launch-1.0 -v videotestsrc is-live=true ! clockoverlay font-desc="Sans, 48" ! \
  videoconvert ! videorate ! "video/x-raw, framerate=1/3" ! \
  jpegenc ! multifilesink location=file-%02d.jpg
```
