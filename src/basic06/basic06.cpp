#include <gst/gst.h>

#include <iostream>

using namespace std;

/**
 * Multithreading and Pad Availability
 *
 * Using `tee` element to duplicate audio stream and `queue` to handle each copy in separate thread.
 **/

int
main(int argc, char* argv[])
{
    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create source elements */
    GstElement* audioSource = gst_element_factory_make("audiotestsrc", "audio_source");
    GstElement* tee = gst_element_factory_make("tee", "tee");
    if (not audioSource or not tee) {
        g_printerr("Unable to create audio source elements\n");
        return EXIT_FAILURE;
    }

    /* Create audio branch elements */
    GstElement* audioQueue = gst_element_factory_make("queue", "audioQueue");
    GstElement* audioConvert = gst_element_factory_make("audioconvert", "audioConvert");
    GstElement* audioResample = gst_element_factory_make("audioresample", "audioResample");
    GstElement* audioSink = gst_element_factory_make("autoaudiosink", "audioSink");
    if (not audioQueue or not audioConvert or not audioResample or not audioSink) {
        g_printerr("Unable to create audio branch elements\n");
        return EXIT_FAILURE;
    }

    /* Create video branch elements */
    GstElement* videoQueue = gst_element_factory_make("queue", "videoQueue");
    GstElement* visual = gst_element_factory_make("wavescope", "visual");
    GstElement* videoConvert = gst_element_factory_make("videoconvert", "videoConvert");
    GstElement* videoSink = gst_element_factory_make("autovideosink", "videoSink");
    if (not videoQueue or not visual or not videoConvert or not videoSink) {
        g_printerr("Unable to create video branch elements\n");
        return EXIT_FAILURE;
    }

    /* Create the empty pipeline */
    GstElement* pipeline = gst_pipeline_new("basic06");
    if (not pipeline) {
        g_printerr("Unable to create pipeline\n");
        return EXIT_FAILURE;
    }

    /* Configure elements */
    g_object_set(audioSource, "freq", 215.0f, NULL);
    g_object_set(visual, "shader", 0, "style", 1, NULL);

    /* Link all elements that can be automatically linked because they have "Always" pads */
    gst_bin_add_many(GST_BIN(pipeline),
                     audioSource,
                     tee,
                     audioQueue,
                     audioConvert,
                     audioResample,
                     audioSink,
                     videoQueue,
                     visual,
                     videoConvert,
                     videoSink,
                     NULL);
    if (gst_element_link_many(audioSource, tee, NULL) != TRUE) {
        g_printerr("Unable to link source elements\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }
    if (gst_element_link_many(audioQueue, audioConvert, audioResample, audioSink, NULL) != TRUE) {
        g_printerr("Unable to link audio branch elements\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }
    if (gst_element_link_many(videoQueue, visual, videoConvert, videoSink, NULL) != TRUE) {
        g_printerr("Unable to link video elements\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }

    /* Manually link the Tee with audio branch */
    GstPad* teeAudioPad = gst_element_request_pad_simple(tee, "src_%u");
    g_print("Obtained request pad %s for audio branch\n", gst_pad_get_name(teeAudioPad));
    GstPad* queueAudioPad = gst_element_get_static_pad(audioQueue, "sink");
    if (gst_pad_link(teeAudioPad, queueAudioPad) != GST_PAD_LINK_OK) {
        g_printerr("Audio branch could not be linked\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }
    gst_object_unref(queueAudioPad);

    /* Manually link the Tee with video branch */
    GstPad* teeVideoPad = gst_element_request_pad_simple(tee, "src_%u");
    g_print("Obtained request pad %s for video branch\n", gst_pad_get_name(teeVideoPad));
    GstPad* queueVideoPad = gst_element_get_static_pad(videoQueue, "sink");
    if (gst_pad_link(teeVideoPad, queueVideoPad) != GST_PAD_LINK_OK) {
        g_printerr("Video branch could not be linked\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }
    gst_object_unref(queueVideoPad);

    /* Start playing the pipeline */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /* Wait until error or EOS */
    GstBus* bus = gst_element_get_bus(pipeline);
    static auto types = static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    GstMessage* msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, types);

    /* Release the request pads from the Tee, and unref them */
    gst_element_release_request_pad(tee, teeAudioPad);
    gst_element_release_request_pad(tee, teeVideoPad);
    gst_object_unref(teeAudioPad);
    gst_object_unref(teeVideoPad);

    /* Free resources */
    if (msg != nullptr) {
        gst_message_unref(msg);
    }
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);

    gst_object_unref(pipeline);
    return EXIT_FAILURE;
}
