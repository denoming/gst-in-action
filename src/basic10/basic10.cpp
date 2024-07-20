#include <gst/gst.h>

#include <cstdio>

static GstElement* pipeline{};
static GstElement* sink{};
static GMainLoop* loop{};
static gboolean playing{}; /* Playing or Paused */
static gdouble rate{};     /* Current playback rate (can be negative) */

/* Send seek event to change rate */
static void
sendSeekEvent()
{
    /* Obtain the current position, needed for the seek event */
    gint64 position;
    if (!gst_element_query_position(pipeline, GST_FORMAT_TIME, &position)) {
        g_printerr("Unable to retrieve current position.\n");
        return;
    }

    /* Create the seek event */
    GstEvent* seekEvent;
    if (rate > 0) {
        seekEvent = gst_event_new_seek(
            rate,
            GST_FORMAT_TIME,
            static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
            GST_SEEK_TYPE_SET,
            position,
            GST_SEEK_TYPE_END,
            0);
    } else {
        seekEvent = gst_event_new_seek(
            rate,
            GST_FORMAT_TIME,
            static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
            GST_SEEK_TYPE_SET,
            0,
            GST_SEEK_TYPE_SET,
            position);
    }

    if (sink == NULL) {
        /* If we have not done so, obtain the sink through which we will send the seek events */
        g_object_get(pipeline, "video-sink", &sink, NULL);
    }

    /* Send the event */
    gst_element_send_event(sink, seekEvent);

    g_print("Current rate: %g\n", rate);
}

/* Process keyboard input */
static gboolean
handleKeyboard(GIOChannel* source, GIOCondition cond)
{
    gchar* str{};
    if (g_io_channel_read_line(source, &str, nullptr, nullptr, nullptr) != G_IO_STATUS_NORMAL) {
        return TRUE;
    }

    switch (g_ascii_tolower(str[0])) {
    case 'p':
        playing = !playing;
        gst_element_set_state(pipeline, playing ? GST_STATE_PLAYING : GST_STATE_PAUSED);
        g_print("Setting state to %s\n", playing ? "PLAYING" : "PAUSE");
        break;
    case 's':
        if (g_ascii_isupper(str[0])) {
            rate *= 2.0;
        } else {
            rate /= 2.0;
        }
        sendSeekEvent();
        break;
    case 'd':
        rate *= -1.0;
        sendSeekEvent();
        break;
    case 'n':
        if (sink == nullptr) {
            /* If we have not done so, obtain the sink through which we will send the step events */
            g_object_get(pipeline, "video-sink", &sink, NULL);
        }
        gst_element_send_event(sink,
                               gst_event_new_step(GST_FORMAT_BUFFERS, 1, ABS(rate), TRUE, FALSE));
        g_print("Stepping one frame\n");
        break;
    case 'q':
        g_main_loop_quit(loop);
        break;
    default:
        break;
    }

    g_free(str);
    return TRUE;
}

int
main(int argc, char* argv[])
{
    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Print usage map */
    g_print("USAGE: Choose one of the following options, then press enter:\n"
            " 'P' to toggle between PAUSE and PLAY\n"
            " 'S' to increase playback speed, 's' to decrease playback speed\n"
            " 'D' to toggle playback direction\n"
            " 'N' to move to next frame (in the current direction, better in PAUSE)\n"
            " 'Q' to quit\n");

    /* Build the pipeline */
    pipeline = gst_parse_launch(
        "playbin uri=https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm", NULL);

    /* Add a keyboard watch so we get notified of keystrokes */
#ifdef G_OS_WIN32
    GIOChannel* ioStdin = g_io_channel_win32_new_fd(fileno(stdin));
#else
    GIOChannel* ioStdin = g_io_channel_unix_new(fileno(stdin));
#endif
    g_io_add_watch(ioStdin, G_IO_IN, reinterpret_cast<GIOFunc>(handleKeyboard), nullptr);

    /* Start playing */
    if (const GstStateChangeReturn rv = gst_element_set_state(pipeline, GST_STATE_PLAYING);
        rv == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }
    playing = TRUE;
    rate = 1.0;

    /* Create a GLib Main Loop and set it to run */
    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    /* Free resources */
    g_main_loop_unref(loop);
    g_io_channel_unref(ioStdin);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    if (sink != nullptr) {
        gst_object_unref(sink);
    }
    gst_object_unref(pipeline);

    return EXIT_SUCCESS;
}
