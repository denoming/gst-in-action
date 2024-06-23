#include <gst/gst.h>

#include <iostream>

#include <gst/gst.h>

using namespace std;

/**
 * - Query pipeline position and duration using `GstQuery` mechanism
 * - Seeking (jumping) to a different position (time) inside the stream
 **/

static const char* kUri{"https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm"};

static GstElement* playbin;  /* Our one and only element */
static gboolean playing;     /* Are we in the PLAYING state? */
static gboolean shutdown;    /* Should we terminate execution? */
static gboolean seekEnabled; /* Is seeking enabled for this media? */
static gboolean seekDone;    /* Have we performed the seek already? */
static gint64 duration;      /* How long does this media last, in nanoseconds */

/* Forward definition of the message processing function */
static void
handleMessage(GstMessage* msg);

int
main(int argc, char* argv[])
{
    playing = FALSE;
    shutdown = FALSE;
    seekEnabled = FALSE;
    seekDone = FALSE;
    duration = GST_CLOCK_TIME_NONE;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    if (playbin = gst_element_factory_make("playbin", "playbin"); not playbin) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    /* Set the URI to play */
    g_object_set(playbin, "uri", kUri, NULL);

    /* Start playing */
    if (gst_element_set_state(playbin, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(playbin);
        return -1;
    }

    /* Listen to the bus */
    GstBus* bus = gst_element_get_bus(playbin);
    do {
        GstMessage* msg = gst_bus_timed_pop_filtered(
            bus,
            100 * GST_MSECOND,
            static_cast<GstMessageType>(GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR
                                        | GST_MESSAGE_EOS | GST_MESSAGE_DURATION));
        if (msg != nullptr) {
            handleMessage(msg);
        } else {
            /* We got no message, this means the timeout expired */
            if (playing) {
                gint64 current = -1;

                /* Query the current position of the stream */
                if (not gst_element_query_position(playbin, GST_FORMAT_TIME, &current)) {
                    g_printerr("Could not query current position.\n");
                }

                /* If we didn't know it yet, query the stream duration */
                if (not GST_CLOCK_TIME_IS_VALID(duration)) {
                    if (!gst_element_query_duration(playbin, GST_FORMAT_TIME, &duration)) {
                        g_printerr("Could not query current duration.\n");
                    }
                }

                /* Print current position and total duration */
                g_print("Position %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
                        GST_TIME_ARGS(current),
                        GST_TIME_ARGS(duration));

                /* If seeking is enabled, we have not done it yet, and the time is right, seek */
                if (seekEnabled && not seekDone && (current > 10 * GST_SECOND)) {
                    g_print("\nReached 10s, performing seek...\n");
                    gst_element_seek_simple(
                        playbin,
                        GST_FORMAT_TIME,
                        /* Use 'GST_SEEK_FLAG_ACCURATE' for some media clips to get more accuracy */
                        static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
                        30 * GST_SECOND);
                    seekDone = TRUE;
                }
            }
        }
    }
    while (!shutdown);

    /* Free resources */
    gst_object_unref(bus);
    gst_element_set_state(playbin, GST_STATE_NULL);
    gst_object_unref(playbin);
    return 0;
}

static void
handleMessage(GstMessage* msg)
{
    GError* err;
    gchar* debugInfo;

    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR:
        gst_message_parse_error(msg, &err, &debugInfo);
        g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
        g_printerr("Debugging information: %s\n", debugInfo ? debugInfo : "none");
        g_clear_error(&err);
        g_free(debugInfo);
        shutdown = TRUE;
        break;
    case GST_MESSAGE_EOS:
        g_print("\nEnd-Of-Stream reached.\n");
        shutdown = TRUE;
        break;
    case GST_MESSAGE_DURATION:
        /* The duration has changed, mark the current one as invalid */
        duration = GST_CLOCK_TIME_NONE;
        break;
    case GST_MESSAGE_STATE_CHANGED: {
        GstState oldState, newState, pendingState;
        gst_message_parse_state_changed(msg, &oldState, &newState, &pendingState);
        if (GST_MESSAGE_SRC(msg) == GST_OBJECT(playbin)) {
            g_print("Pipeline state changed from %s to %s:\n",
                    gst_element_state_get_name(oldState),
                    gst_element_state_get_name(newState));

            /* Remember whether we are in the PLAYING state or not */
            if (playing = (newState == GST_STATE_PLAYING) ? TRUE : FALSE; playing) {
                /* We just moved to PLAYING. Check if seeking is possible */
                gint64 start{}, end{};
                GstQuery* query = gst_query_new_seeking(GST_FORMAT_TIME);
                if (gst_element_query(playbin, query)) {
                    gst_query_parse_seeking(query, NULL, &seekEnabled, &start, &end);
                    if (seekEnabled) {
                        g_print("Seeking is ENABLED from %" GST_TIME_FORMAT " to %" GST_TIME_FORMAT
                                "\n",
                                GST_TIME_ARGS(start),
                                GST_TIME_ARGS(end));
                    } else {
                        g_print("Seeking is DISABLED for this stream.\n");
                    }
                } else {
                    g_printerr("Seeking query failed.");
                }
                gst_query_unref(query);
            }
        }
    } break;
    default:
        /* We should not reach here */
        g_printerr("Unexpected message received.\n");
        break;
    }
    gst_message_unref(msg);
}
