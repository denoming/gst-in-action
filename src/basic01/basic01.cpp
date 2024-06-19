#include <iostream>

using namespace std;

#include <gst/gst.h>

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

    /* Build the pipeline */
    pipeline = gst_parse_launch(kDescription, nullptr);

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
