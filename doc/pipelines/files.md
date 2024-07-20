# Files

When using the command-line, the `-e` parameter ensures the output file is correctly completed on exit.

## Pipelines

Write to an MP4 file:
```shell
$ gst-launch-1.0 -e \
    videotestsrc pattern=ball ! "video/x-raw, width=1280, height=720" ! timeoverlay font-desc="Sans, 48" ! x264enc ! mux. \
    audiotestsrc is-live=true wave=ticks ! audioconvert ! audioresample ! faac bitrate=32000 ! mux. \
    mp4mux name=mux ! filesink location=file.mp4
```
