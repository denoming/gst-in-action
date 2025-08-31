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

#include <iostream>

using namespace std;

#include <gst/gst.h>

int
main(int argc, char* argv[])
{
    GstBus* bus{};
    GstMessage* msg{};
    GstStateChangeReturn ret{};

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    GstElement* source = gst_element_factory_make("videotestsrc", "source");
    GstElement* sink = gst_element_factory_make("autovideosink", "sink");

    /* Create the empty pipeline */
    GstElement* pipeline = gst_pipeline_new("basic02");
    if (not pipeline or not source or not sink) {
        g_printerr("Not all elements could be created.\n");
        return EXIT_FAILURE;
    }

    /* Build the pipeline (must be done before linking) */
    gst_bin_add_many(GST_BIN(pipeline), source, sink, nullptr);
    if (gst_element_link(source, sink) != TRUE) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }

    /* Modify the source's properties */
    g_object_set(source, "pattern", 0, nullptr);

    /* Start playing */
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }

    /* Wait until error or EOS */
    bus = gst_element_get_bus(pipeline);
    const GstMessageType types = static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, types);

    /* Parse message */
    if (msg != nullptr) {
        GError* err{};
        gchar* debugInfo{};

        switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(msg, &err, &debugInfo);
            g_printerr(
                "Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
            g_printerr("Debugging information: %s\n", debugInfo ? debugInfo : "none");
            g_clear_error(&err);
            g_free(debugInfo);
            break;
        case GST_MESSAGE_EOS:
            g_print("End-Of-Stream reached.\n");
            break;
        default:
            /* We should not reach here because we only asked for ERRORs and EOS */
            g_printerr("Unexpected message received.\n");
            break;
        }
        gst_message_unref(msg);
    }

    /* Free resources */
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}
