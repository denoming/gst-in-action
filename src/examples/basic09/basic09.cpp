#include <gst/gst.h>

using namespace std;

static gboolean isLive{};
static GstElement* pipeline{};
static GMainLoop* loop{};

static void
onMessage(GstBus* /*bus*/, GstMessage* msg)
{
    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR: {
        GError* err{};
        gchar* debug{};

        gst_message_parse_error(msg, &err, &debug);
        g_print("Error: %s\n", err->message);
        g_error_free(err);
        g_free(debug);

        gst_element_set_state(pipeline, GST_STATE_READY);
        g_main_loop_quit(loop);
        break;
    }
    case GST_MESSAGE_EOS:
        /* Handle EoS */
        gst_element_set_state(pipeline, GST_STATE_READY);
        g_main_loop_quit(loop);
        break;
    case GST_MESSAGE_BUFFERING: {
        /* Handle buffering */
        if (isLive) {
            /* If the stream is live, we do not care about buffering. */
            break;
        }

        gint percent{};
        gst_message_parse_buffering(msg, &percent);
        g_print("Buffering (%3d%%)\r", percent);

        /* Wait until buffering is complete before start/resume playing */
        if (percent < 100) {
            gst_element_set_state(pipeline, GST_STATE_PAUSED);
        } else {
            gst_element_set_state(pipeline, GST_STATE_PLAYING);
        }
    }
    case GST_MESSAGE_CLOCK_LOST:
        /* Get a new clock (we need to pause stream and resume) */
        gst_element_set_state(pipeline, GST_STATE_PAUSED);
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
        break;
    default:
        /* Unhandled message */
        break;
    }
}

int
main(int argc, char* argv[])
{
    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Build the pipeline */
    pipeline = gst_parse_launch(
        "playbin uri=https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm",
        nullptr);

    /* Start playing */
    if (const GstStateChangeReturn rv = gst_element_set_state(pipeline, GST_STATE_PLAYING);
        rv == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    } else if (rv == GST_STATE_CHANGE_NO_PREROLL) {
        isLive = TRUE;
    }

    loop = g_main_loop_new(nullptr, FALSE);

    GstBus* bus = gst_element_get_bus(pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message", G_CALLBACK(onMessage), nullptr);

    g_main_loop_run(loop);

    /* Free resources */
    g_main_loop_unref(loop);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return EXIT_SUCCESS;
}
