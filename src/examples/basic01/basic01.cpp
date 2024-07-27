#include "common/Utils.hpp"

#include <gst/gst.h>

#include <iostream>

using namespace std;

static const char* kDescription = R"(
    playbin uri=https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm
)";

int
main(int argc, char* argv[])
{
    GstElement* pipeline{};
    GstBus* bus{};
    GstMessage* msg{};

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Print GStreamer version */
    printVersion();

    /* Build the pipeline */
    GError* err{};
    pipeline = gst_parse_launch(kDescription, &err);
    if (err != nullptr) {
        gchar* debugInfo{};
        gst_message_parse_error(msg, &err, &debugInfo);
        g_printerr("Error on parsing pipeline %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
        g_printerr("Debugging information: %s\n", debugInfo ? debugInfo : "none");
        g_clear_error(&err);
        g_free(debugInfo);
        return EXIT_FAILURE;
    }

    /* Start playing */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /* Wait until error or EOS */
    const GstMessageType types = static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, types);

    /* See next tutorial for proper error message handling/parsing */
    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        g_printerr("An error occurred! Set GST_DEBUG=*:WARN env to get more details\n");
    }

    /* Free resources */
    gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return EXIT_SUCCESS;
}
