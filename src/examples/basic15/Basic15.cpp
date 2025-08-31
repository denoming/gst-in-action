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

/**
 * Example 15: Seek video with blocking SRC pads
 **/

static GMainLoop* loop{};
static gint counter{};
static GstBus* bus{};
static gboolean prerolled{FALSE};
static GstPad* sinkpad{};

static void
decrementCounter(GstElement* pipeline)
{
    if (prerolled) {
        return;
    }

    if (g_atomic_int_dec_and_test(&counter)) {
        /**
         * All probes blocked and no-more-pads signaled, post
         * message on the bus.
         */
        prerolled = TRUE;

        gst_bus_post(bus,
                     gst_message_new_application(GST_OBJECT_CAST(pipeline),
                                                 gst_structure_new_empty("ExPrerolled")));
    }
}

/* Called when a source pad of "uridecodebin" is blocked */
static GstPadProbeReturn
onBlocked(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    GstElement* pipeline = GST_ELEMENT(user_data);

    if (prerolled) {
        return GST_PAD_PROBE_REMOVE;
    }

    decrementCounter(pipeline);

    // Keep blocking until "onNoMorePads" is called and counter is reduced up to 0
    return GST_PAD_PROBE_OK;
}

/* Called when "uridecodebin" has a new pad */
static void
onPadAdded(GstElement* /*element*/, GstPad* pad, gpointer user_data)
{
    GstElement* pipeline = GST_ELEMENT(user_data);

    if (prerolled) {
        return;
    }

    g_atomic_int_inc(&counter);

    // Block every added pads and set callback
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, onBlocked, pipeline, nullptr);

    // Try to link to the video pad
    gst_pad_link(pad, sinkpad);
}

/* Called when "uridecodebin" has created all pads */
static void
onNoMorePads(GstElement* /*element*/, gpointer user_data)
{
    GstElement* pipeline = GST_ELEMENT(user_data);

    if (prerolled) {
        return;
    }

    decrementCounter(pipeline);
}

/* Called when a new message is posted on the bus */
static void
onPipelineMessage(GstBus* bus, GstMessage* message, gpointer user_data)
{
    GstElement* pipeline = GST_ELEMENT(user_data);

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR: {
        GError* error{};
        gchar* debugInfo;
        gst_message_parse_error(message, &error, &debugInfo);
        g_message("Received error: %s, %s", GST_OBJECT_NAME(message->src), error->message);
        g_message("Debugging information: %s", debugInfo ? debugInfo : "none");
        g_clear_error(&error);
        g_free(debugInfo);
        g_main_loop_quit(loop);
        break;
    }
    case GST_MESSAGE_EOS: {
        g_print("Reached EOS\n");
        g_main_loop_quit(loop);
        break;
    }
    case GST_MESSAGE_APPLICATION: {
        if (gst_message_has_name(message, "ExPrerolled")) {
            /* it's our message */
            g_print("We are all pre-rolled, do seek\n");
            gst_element_seek(
                pipeline,
                1.0,
                GST_FORMAT_TIME,
                static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
                GST_SEEK_TYPE_SET,
                2 * GST_SECOND,
                GST_SEEK_TYPE_SET,
                15 * GST_SECOND);
            gst_element_set_state(pipeline, GST_STATE_PLAYING);
        }
        break;
    }
    default:
        break;
    }
}

gint
main(gint argc, gchar* argv[])
{
    // Initialize GStreamer
    gst_init(&argc, &argv);

    if (argc < 2) {
        g_print("usage: %s <uri>", argv[0]);
        return EXIT_FAILURE;
    }

    // Build
    GstElement* pipeline = gst_pipeline_new("my-pipeline");

    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message", (GCallback) onPipelineMessage, pipeline);

    GstElement* src = gst_element_factory_make("uridecodebin", "src");
    g_assert(src != nullptr);
    g_object_set(src, "uri", argv[1], NULL);
    GstElement* csp = gst_element_factory_make("videoconvert", "csp");
    g_assert(csp != nullptr);
    GstElement* vs = gst_element_factory_make("videoscale", "vs");
    g_assert(vs != nullptr);
    GstElement* sink = gst_element_factory_make("autovideosink", "sink");
    g_assert(sink != nullptr);

    gst_bin_add_many(GST_BIN(pipeline), src, csp, vs, sink, NULL);

    // Link all elements except "uridecodebin" (there is no pads yet)
    gst_element_link_many(csp, vs, sink, NULL);

    sinkpad = gst_element_get_static_pad(csp, "sink");

    /**
     * For each pad block that is installed, we will increment
     * the counter. For each pad block that is signaled, we
     * decrement the counter. When the counter is 0 we post
     * an app message to tell the app that all pads are
     * blocked. Start with 1 that is decremented when no-more-pads
     * is signaled to make sure that we only post the message
     * after no-more-pads */
    g_atomic_int_set(&counter, 1);

    g_signal_connect(src, "pad-added", GCallback(onPadAdded), pipeline);
    g_signal_connect(src, "no-more-pads", GCallback(onNoMorePads), pipeline);

    gst_element_set_state(pipeline, GST_STATE_PAUSED);

    loop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);

    gst_object_unref(sinkpad);
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);

    return EXIT_SUCCESS;
}
