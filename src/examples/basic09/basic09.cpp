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

using namespace std;

static gboolean isLive{};
static gboolean isBuffering{};
static GstElement* pipeline{};
static GMainLoop* loop{};

static gboolean
onBufferTimeout(gpointer data)
{
    auto* const pipeline = static_cast<GstElement*>(data);

    GstQuery* query = gst_query_new_buffering(GST_FORMAT_TIME);
    if (not gst_element_query(pipeline, query)) {
        return TRUE;
    }

    gboolean busy{};
    gint percent{};
    gint64 downloadTimeRemaining{};
    gst_query_parse_buffering_percent(query, &busy, &percent);
    gst_query_parse_buffering_range(query, nullptr, nullptr, nullptr, &downloadTimeRemaining);
    if (downloadTimeRemaining == -1) {
        downloadTimeRemaining = 0;
    }

    /* Calculate the remaining playback time */
    gint64 position{}, duration{};
    if (not gst_element_query_position(pipeline, GST_FORMAT_TIME, &position)) {
        position = TRUE;
    }
    if (not gst_element_query_duration(pipeline, GST_FORMAT_TIME, &duration)) {
        duration = TRUE;
    }

    guint64 playLeft{};
    if (duration != -1 and position != -1) {
        playLeft = GST_TIME_AS_MSECONDS(duration - position);
    } else {
        playLeft = 0;
    }

    g_message("Left %" G_GUINT64_FORMAT ", Remaining (download) %" G_GUINT64_FORMAT ", Percent %d",
              playLeft,
              downloadTimeRemaining,
              percent);

    /**
     * We are buffering or the download time remaining is bigger than the
     * remaining playback time. We keep buffering.
     */
    isBuffering = (busy or gdouble(downloadTimeRemaining) * 1.1 > gdouble(playLeft));
    if (not isBuffering) {
        g_print("Set pipeline state: (1) PLAYING\n");
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
    }

    return isBuffering;
}

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
    case GST_MESSAGE_BUFFERING: /* Handle buffering */ {
        if (isLive) {
            /* If the stream is live, we do not care about buffering. */
            break;
        }

        gint percent{};
        gst_message_parse_buffering(msg, &percent);
        g_print("Buffering (%3d%%)\n", percent);

        /* Wait until buffering is complete before start/resume playing */
        if (percent < 100) {
            if (isBuffering == FALSE) {
                isBuffering = TRUE;
                g_print("Set pipeline state: PAUSED\n");
                gst_element_set_state(pipeline, GST_STATE_PAUSED);
            }
        }
        break;
    }
    case GST_MESSAGE_ASYNC_DONE:
        if (isBuffering == FALSE) {
            g_print("Set pipeline state: (2) PLAYING\n");
            gst_element_set_state(pipeline, GST_STATE_PLAYING);
        } else {
            g_timeout_add(500, onBufferTimeout, pipeline);
        }
        break;
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

    isLive = FALSE;
    isBuffering = FALSE;

    /* Start playing */
    const GstStateChangeReturn rv = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    switch (rv) {
    case GST_STATE_CHANGE_FAILURE:
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    case GST_STATE_CHANGE_SUCCESS:
        isLive = FALSE;
        break;
    case GST_STATE_CHANGE_NO_PREROLL:
        isLive = TRUE;
        break;
    default:
        break;
    };

    loop = g_main_loop_new(nullptr, FALSE);

    GstBus* bus = gst_element_get_bus(pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message", G_CALLBACK(onMessage), nullptr);
    gst_object_unref(bus);

    g_main_loop_run(loop);

    /* Free resources */
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);

    return EXIT_SUCCESS;
}
