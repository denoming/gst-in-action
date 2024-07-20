# Sharing

## Introduction

Reason to decouple pipeline:

* To enter a separate application
* To use multiple processes (perhaps for security reasons)
* To split into multiple pipelines, so that a failure in one part does not alter another
* To split into multiple pipelines, so that you can 'seek' (jump to a certain point) in one without affecting another

Available methods of sharing:

| Name                                | Description                                                                                 | 
|-------------------------------------|---------------------------------------------------------------------------------------------|
| *shmsink and shmsrc*                | Allows video to be read/written from shared memory                                          |
| *appsrc/appsink*                    | Allows video data to leave/enter the pipeline from your own application                     |
| *fdsrc/fdsink*                      | Allows communication via a file descriptor                                                  |
| *interpipe*                         | Allows simple communication between two or more independent pipelines. Very powerful.       |
| *inter* (intervideosink, etc)       | Send/receive AV between two pipelines in the same process (only raw audio/video, no events) |
| *ipcpipeline*                       | Allows communication between pipelines *in different processes*.                            |
| *gstproxy (proxysink and proxysrc)* | Send/receive AV between two pipelines in the same process.                                  |

Additional resources:

* `ipcpipeline`:
  [Collabora post](https://www.collabora.com/news-and-blog/blog/2017/11/17/ipcpipeline-splitting-a-gstreamer-pipeline-into-multiple-processes/)
* `interpipe`:
  [Great RidgeRun docs](https://developer.ridgerun.com/wiki/index.php?title=GstInterpipe)
  [Presentation](https://gstreamer.freedesktop.org/data/events/gstreamer-conference/2015/Melissa%20Montero%20-%20GST%20Daemon%20and%20Interpipes:%20A%20simpler%20way%20to%20get%20your%20applications%20done%20.pdf)
* `inter`: [Nirbheek's blog](http://blog.nirbheek.in/2018/02/decoupling-gstreamer-pipelines.html)

## Pipelines

### Using `shmsink`+`shmsrc` elements

Writing stream into memory (shmsink):
```shell
gst-launch-1.0 -v videotestsrc ! \
    "video/x-raw, format=(string)I420,  width=(int)320, height=(int)240, framerate=(fraction)30/1" ! \
    shmsink socket-path=/tmp/tmpsock shm-size=2000000
```
or from a file rather than test source, and keeping the audio local:
```shell
gst-launch-1.0 filesrc location=$SRC ! qtdemux name=demux \
    demux.audio_0 ! queue ! decodebin ! audioconvert ! audioresample ! autoaudiosink \
    demux.video_0 ! queue ! decodebin ! videoconvert ! videoscale ! videorate ! \
    "video/x-raw, format=(string)I420, width=(int)320, height=(int)240, framerate=(fraction)30/1" ! \
    queue !  identity ! \
    shmsink wait-for-connection=0 socket-path=/tmp/tmpsock shm-size=20000000 sync=true  
```

Reading stream from memory (shmsrc):
```shell
gst-launch-1.0 shmsrc socket-path=/tmp/tmpsock ! \
    "video/x-raw, format=(string)I420, width=(int)320, height=(int)240, framerate=(fraction)30/1" ! \
    queue ! videoconvert ! autovideosink
````

### Using `proxysink` and `proxysrc` elements

The elements `proxysink` and `proxysrc` can be used to split larger pipelines into smaller ones.
Unlike `inter`, `proxy` will keep timing in sync. If you want pipelines to have own timing you should
use other elements.

The `proxy` documentations:
* {Introduction](http://blog.nirbheek.in/2018/02/decoupling-gstreamer-pipelines.html)
* [proxysrc](https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-bad-plugins/html/gst-plugins-bad-plugins-proxysrc.html):
* [proxysink](https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-bad-plugins/html/gst-plugins-bad-plugins-proxysink.html)

### Using `inter` (`intervideosink`/`intervideosrc` elements and their audio&subtitle counterparts)

The `inter` versions of `proxy` doesn't sync timings. But this can be useful if you want
pipelines to be more independent. (Pros and cons on this discussed [here](http://gstreamer-devel.966125.n4.nabble.com/How-to-connect-intervideosink-and-intervideosrc-for-IPC-pipelines-td4684567.html).)

* `interaudiosink` and `intervideosink` allow a pipeline to send audio/video to another pipeline.
* `interaudiosrc` and `intervideosrc` are the corresponding elements for receiving the audio/video.
* subtitle versions are available too.
