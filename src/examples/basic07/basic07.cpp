#include <gst/gst.h>
#include <gst/audio/audio.h>

#include <iostream>

using namespace std;

/**
 * Short-cutting the pipeline
 *
 * Using `appsrc` and `appsink` components to push and pull data from pipeline.
 **/

#define CHUNK_SIZE 1024   /* Amount of bytes we are sending in each buffer */
#define SAMPLE_RATE 44100 /* Samples per second we are sending */

static GstElement* pipeline{};
static GstElement* appSource{};
static GstElement* tee{};
static GstElement* audioQueue{};
static GstElement* audioConvert1{};
static GstElement* audioResample{};
static GstElement* audioSink{};
static GstElement* videoQueue{};
static GstElement* audioConvert2{};
static GstElement* visual{};
static GstElement* videoConvert{};
static GstElement* videoSink{};
static GstElement* appQueue{};
static GstElement* appSink{};
static guint64 samplesCounter{}; /* Number of samples generated so far (for timestamp generation) */
static gfloat a{}, b{}, c{}, d{}; /* For waveform generation */
static guint idleSourceId{};      /* To control the GSource */
static GMainLoop* mainLoop{};     /* GLib's Main Loop */

/**
 * This method is called by the idle GSource in the mainloop, to feed CHUNK_SIZE bytes into appsrc.
 * The idle handler is added to the mainloop when appsrc requests us to start sending data
 * (need-data signal) and is removed when appsrc has enough data (enough-data signal).
 */
static gboolean
push_data()
{
    /* Create a new empty buffer */
    GstBuffer* buffer = gst_buffer_new_and_alloc(CHUNK_SIZE);

    /* Set its timestamp and duration */
    constexpr gint samplesCount = CHUNK_SIZE / 2; /* Because each sample is 16 bits */
    GST_BUFFER_TIMESTAMP(buffer) = gst_util_uint64_scale(samplesCounter, GST_SECOND, SAMPLE_RATE);
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(samplesCount, GST_SECOND, SAMPLE_RATE);

    /* Generate some psychodelic waveforms */
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    gint16* raw = reinterpret_cast<gint16*>(map.data);
    c += d;
    d -= c / 1000;
    const gfloat freq = 1100 + 1000 * d;
    for (int i = 0; i < samplesCount; i++) {
        a += b;
        b -= a / freq;
        raw[i] = static_cast<gint16>(500 * a);
    }
    gst_buffer_unmap(buffer, &map);
    samplesCounter += samplesCount;

    /* Push the buffer into the appsrc */
    GstFlowReturn ret;
    g_signal_emit_by_name(appSource, "push-buffer", buffer, &ret);

    /* Free the buffer now that we are done with it */
    gst_buffer_unref(buffer);

    if (ret != GST_FLOW_OK) {
        /* We got some error, stop sending data */
        return FALSE;
    }

    return TRUE;
}

/* This signal callback triggers when appsrc needs  Here, we add an idle handler
 * to the mainloop to start pushing data into the appsrc */
static void
startFeed(GstElement* /*source*/, guint /*size*/)
{
    if (idleSourceId == 0) {
        g_print("Start feeding\n");
        idleSourceId = g_idle_add(reinterpret_cast<GSourceFunc>(push_data), nullptr);
    }
}

/* This callback triggers when appsrc has enough data and we can stop sending.
 * We remove the idle handler from the mainloop */
static void
stopFeed(GstElement* source)
{
    if (idleSourceId != 0) {
        g_print("Stop feeding\n");
        g_source_remove(idleSourceId);
        idleSourceId = 0;
    }
}

/* The appsink has received a buffer */
static GstFlowReturn
onNewSample(GstElement* sink)
{
    GstSample* sample;

    /* Retrieve the buffer */
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    if (sample) {
        /* The only thing we do in this example is print a * to indicate a received buffer */
        g_print("*");
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    return GST_FLOW_ERROR;
}

/* This function is called when an error message is posted on the bus */
static void
error_cb(GstBus* bus, GstMessage* msg)
{
    GError* err;
    gchar* debug_info;

    /* Print error details on the screen */
    gst_message_parse_error(msg, &err, &debug_info);
    g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
    g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
    g_clear_error(&err);
    g_free(debug_info);

    g_main_loop_quit(mainLoop);
}

int
main(int argc, char* argv[])
{
    /* Initialize custom data structure */
    b = 1; /* For waveform generation */
    d = 1;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    appSource = gst_element_factory_make("appsrc", "audio_source");
    tee = gst_element_factory_make("tee", "tee");
    if (not appSource or not tee) {
        g_printerr("Unable to create audio source elements\n");
        return EXIT_FAILURE;
    }

    audioQueue = gst_element_factory_make("queue", "audioQueue");
    audioConvert1 = gst_element_factory_make("audioconvert", "audioConvert1");
    audioResample = gst_element_factory_make("audioresample", "audioResample");
    audioSink = gst_element_factory_make("autoaudiosink", "audioSink");
    if (not audioQueue or not audioConvert1 or not audioResample or not audioSink) {
        g_printerr("Unable to create audio branch elements\n");
        return EXIT_FAILURE;
    }

    videoQueue = gst_element_factory_make("queue", "videoQueue");
    audioConvert2 = gst_element_factory_make("audioconvert", "audioConvert2");
    visual = gst_element_factory_make("wavescope", "visual");
    videoConvert = gst_element_factory_make("videoconvert", "videoConvert");
    videoSink = gst_element_factory_make("autovideosink", "videoSink");
    if (not videoQueue or not visual or not videoConvert or not videoSink) {
        g_printerr("Unable to create vidoe branch elements\n");
        return EXIT_FAILURE;
    }

    appQueue = gst_element_factory_make("queue", "appQueue");
    appSink = gst_element_factory_make("appsink", "appSink");
    if (not appQueue or not appSink) {
        g_printerr("Unable to create sink elements\n");
        return EXIT_FAILURE;
    }

    /* Create the empty pipeline */
    pipeline = gst_pipeline_new("basic07");
    if (not pipeline) {
        g_printerr("Unable to create pipeline\n");
        return EXIT_FAILURE;
    }

    /* Configure wavescope */
    g_object_set(visual, "shader", 0, "style", 0, NULL);

    /* Configure appsrc */
    GstAudioInfo info;
    gst_audio_info_set_format(&info, GST_AUDIO_FORMAT_S16, SAMPLE_RATE, 1, NULL);
    GstCaps* audioCaps = gst_audio_info_to_caps(&info);
    g_object_set(appSource, "caps", audioCaps, "format", GST_FORMAT_TIME, NULL);
    g_signal_connect(appSource, "need-data", G_CALLBACK(startFeed), nullptr);
    g_signal_connect(appSource, "enough-data", G_CALLBACK(stopFeed), nullptr);

    /* Configure appsink */
    g_object_set(appSink, "emit-signals", TRUE, "caps", audioCaps, NULL);
    g_signal_connect(appSink, "new-sample", G_CALLBACK(onNewSample), nullptr);
    gst_caps_unref(audioCaps);

    /* Link all elements that can be automatically linked because they have "Always" pads */
    gst_bin_add_many(GST_BIN(pipeline),
                     appSource,
                     tee,
                     audioQueue,
                     audioConvert1,
                     audioResample,
                     audioSink,
                     videoQueue,
                     audioConvert2,
                     visual,
                     videoConvert,
                     videoSink,
                     appQueue,
                     appSink,
                     NULL);
    if (gst_element_link_many(appSource, tee, NULL) != TRUE) {
        g_printerr("Unable to link source elements\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }
    if (gst_element_link_many(audioQueue, audioConvert1, audioResample, audioSink, NULL) != TRUE) {
        g_printerr("Unable to link audio branch elements\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }
    if (gst_element_link_many(videoQueue, audioConvert2, visual, videoConvert, videoSink, NULL)
        != TRUE) {
        g_printerr("Unable to link video elements\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }
    if (gst_element_link_many(appQueue, appSink, NULL) != TRUE) {
        g_printerr("Unable to link sink elements\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }

    /* Manually link the tee with audio branch */
    GstPad* teeAudioPad = gst_element_request_pad_simple(tee, "src_%u");
    g_print("Obtained request pad %s for audio branch\n", gst_pad_get_name(teeAudioPad));
    GstPad* queueAudioPad = gst_element_get_static_pad(audioQueue, "sink");
    if (gst_pad_link(teeAudioPad, queueAudioPad) != GST_PAD_LINK_OK) {
        g_printerr("Audio branch could not be linked\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }

    /* Manually link the tee with video branch */
    GstPad* teeVideoPad = gst_element_request_pad_simple(tee, "src_%u");
    g_print("Obtained request pad %s for video branch\n", gst_pad_get_name(teeVideoPad));
    GstPad* queueVideoPad = gst_element_get_static_pad(videoQueue, "sink");
    if (gst_pad_link(teeVideoPad, queueVideoPad) != GST_PAD_LINK_OK) {
        g_printerr("Video branch could not be linked\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }
    gst_object_unref(queueVideoPad);

    /* Manually link the tee with sink branch */
    GstPad* teeAppPad = gst_element_request_pad_simple(tee, "src_%u");
    g_print("Obtained request pad %s for sink branch\n", gst_pad_get_name(teeAppPad));
    GstPad* queueAppPad = gst_element_get_static_pad(appQueue, "sink");
    if (gst_pad_link(teeAppPad, queueAppPad) != GST_PAD_LINK_OK) {
        g_printerr("Sink branch could not be linked\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }
    gst_object_unref(queueAppPad);

    /* Instruct the bus to emit signals for each received message, and connect to the interesting
     * signals */
    GstBus* bus = gst_element_get_bus(pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message::error", (GCallback) error_cb, nullptr);
    gst_object_unref(bus);

    /* Start playing the pipeline */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /* Create a GLib Main Loop and set it to run */
    mainLoop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(mainLoop);

    /* Release the request pads from the Tee, and unref them */
    gst_element_release_request_pad(tee, teeAudioPad);
    gst_element_release_request_pad(tee, teeVideoPad);
    gst_element_release_request_pad(tee, teeAppPad);
    gst_object_unref(teeAudioPad);
    gst_object_unref(teeVideoPad);
    gst_object_unref(teeAppPad);

    /* Free resources */
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}
