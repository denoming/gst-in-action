// Copyright 2025 Denys Asauliak
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gst/gst.h>

#include <iostream>

using namespace std;

/**
 *  Dynamic pipeline example. Use `uridecodebin` source element it's `pad-added` signal to
 *  attach audio and video branches of pipeline when particular source pad become availalbe
 *  on source element.
 */

static const char* kUri{"https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm"};

static GstElement* pipeline{};
static GstElement* source{};
static GstElement* audioConvert{};
static GstElement* audioResample{};
static GstElement* audioSink{};
static GstElement* videoConvert{};
static GstElement* videoSink{};

/* Handler for the pad-added signal */
static void
handlePadAdded(GstElement* src, GstPad* pad, void* data);

static void
handleMessages();

int
main(int argc, char* argv[])
{
    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    source = gst_element_factory_make("uridecodebin", "source");
    if (not source) {
        g_printerr("Unable to create source element.\n");
        return EXIT_FAILURE;
    }

    audioConvert = gst_element_factory_make("audioconvert", "audioConvert");
    audioResample = gst_element_factory_make("audioresample", "audioResample");
    audioSink = gst_element_factory_make("autoaudiosink", "audioSink");
    if (not audioConvert or not audioResample or not audioSink) {
        g_printerr("Not all audio elements could be created.\n");
        return EXIT_FAILURE;
    }

    videoConvert = gst_element_factory_make("videoconvert", "videoConvert");
    videoSink = gst_element_factory_make("autovideosink", "videoSink");
    if (not videoConvert or not videoSink) {
        g_printerr("Not all video elements could be created.\n");
        return EXIT_FAILURE;
    }

    /* Create the empty pipeline */
    if (pipeline = gst_pipeline_new("basic03"); not pipeline) {
        g_printerr("Not all elements could be created.\n");
        return EXIT_FAILURE;
    }

    /* Add all elemetns but link without source element (src pads are not present yet) */
    gst_bin_add_many(GST_BIN(pipeline),
                     source,
                     audioConvert,
                     audioResample,
                     audioSink,
                     videoConvert,
                     videoSink,
                     nullptr);

    /* Link audio pipeline branch */
    if (not gst_element_link_many(audioConvert, audioResample, audioSink, nullptr)) {
        g_printerr("Audio elements could not be linked.\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }

    /* Link audio pipeline branch */
    if (not gst_element_link_many(videoConvert, videoSink, nullptr)) {
        g_printerr("Video elements could not be linked.\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }

    /* Set the URI to play */
    g_object_set(source, "uri", kUri, nullptr);

    /* Connect to the pad-added signal (pospone linking source element with the rest of pipeline) */
    g_signal_connect(source, "pad-added", G_CALLBACK(handlePadAdded), nullptr);

    /* Start playing */
    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }

    handleMessages();

    /* Free resources */
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return EXIT_SUCCESS;
}

static void
handlePadAdded(GstElement* src, GstPad* srcPad, void* /*data*/)
{
    g_print("Received new pad '%s' from '%s'.\n", GST_PAD_NAME(srcPad), GST_ELEMENT_NAME(src));

    GstCaps* newPadCaps = gst_pad_get_current_caps(srcPad);
    const gchar* newPadType = gst_structure_get_name(gst_caps_get_structure(newPadCaps, 0));
    if (g_str_has_prefix(newPadType, "audio/x-raw")) {
        GstPad* sinkPad = gst_element_get_static_pad(audioConvert, "sink");
        if (gst_pad_is_linked(sinkPad)) {
            g_print("We are already linked audio branch. Ignoring.\n");
            gst_object_unref(sinkPad);
        } else {
            if (GST_PAD_LINK_FAILED(gst_pad_link(srcPad, sinkPad))) {
                g_print("Type is '%s' but audio branch link failed.\n", newPadType);
            } else {
                g_print("Link audio branch succeeded (type '%s').\n", newPadType);
            }
        }
    } else if (g_str_has_prefix(newPadType, "video/x-raw")) {
        GstPad* sinkPad = gst_element_get_static_pad(videoConvert, "sink");
        if (gst_pad_is_linked(sinkPad)) {
            g_print("We are already linked video branch. Ignoring.\n");
            gst_object_unref(sinkPad);
        } else {
            if (GST_PAD_LINK_FAILED(gst_pad_link(srcPad, sinkPad))) {
                g_print("Type is '%s' but video branch link failed.\n", newPadType);
            } else {
                g_print("Link video branch succeeded (type '%s').\n", newPadType);
            }
        }
    } else {
        g_print("Ignore pad with '%s' type. Ignoring.\n", newPadType);
    }
}

static void
handleMessages()
{
    bool terminate{};
    GstBus* bus = gst_element_get_bus(pipeline);
    do {
        if (GstMessage* msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ANY);
            msg != nullptr) {
            GError* err{};
            gchar* debugInfo{};
            switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debugInfo);
                g_printerr("Error received from element %s: %s\n",
                           GST_OBJECT_NAME(msg->src),
                           err->message);
                g_printerr("Debugging information: %s\n", debugInfo ? debugInfo : "none");
                g_clear_error(&err);
                g_free(debugInfo);
                terminate = true;
                break;
            case GST_MESSAGE_EOS:
                g_print("EoS reached.\n");
                terminate = true;
                break;
            case GST_MESSAGE_NEW_CLOCK: {
                GstClock* clock{};
                gst_message_parse_new_clock(msg, &clock);
                if (clock != nullptr) {
                    GstClockTime time = gst_clock_get_time(clock);
                    g_print("New Clock received: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS(time));
                }
                break;
            }
            case GST_MESSAGE_LATENCY:
                g_print("Latency is changed.\n");
                break;
            case GST_MESSAGE_CLOCK_LOST:
                g_print("Clock was lost.\n");
                break;
            case GST_MESSAGE_STATE_CHANGED:
                /* We are only interested in state-changed messages from the pipeline */
                if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
                    GstState oldState, newState, pendingState;
                    gst_message_parse_state_changed(msg, &oldState, &newState, &pendingState);
                    g_print("Pipeline state changed from %s to %s:\n",
                            gst_element_state_get_name(oldState),
                            gst_element_state_get_name(newState));
                }
                break;
            default:
                break;
            }
            gst_message_unref(msg);
        }
    }
    while (not terminate);
    gst_object_unref(bus);
}
