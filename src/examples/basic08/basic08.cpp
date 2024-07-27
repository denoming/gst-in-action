#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

#include <iostream>

using namespace std;

static GstDiscoverer* discoverer{};
static GMainLoop* loop{};

/* Print a tag in a human-readable format (name: value) */
static void
printTagForeach(const GstTagList* tags, const gchar* tag, gpointer user_data)
{
    GValue val{};
    gst_tag_list_copy_value(&val, tags, tag);

    gchar* str{};
    if (G_VALUE_HOLDS_STRING(&val)) {
        str = g_value_dup_string(&val);
    } else {
        str = gst_value_serialize(&val);
    }

    const gint depth = GPOINTER_TO_INT(user_data);
    g_print("%*s%s: %s\n", 2 * depth, " ", gst_tag_get_nick(tag), str);
    g_free(str);

    g_value_unset(&val);
}

/* Print information regarding a stream */
static void
printStreamInfo(GstDiscovererStreamInfo* info, const gint depth)
{
    gchar* desc = nullptr;
    if (GstCaps* caps = gst_discoverer_stream_info_get_caps(info); caps) {
        if (gst_caps_is_fixed(caps)) {
            desc = gst_pb_utils_get_codec_description(caps);
        } else {
            desc = gst_caps_to_string(caps);
        }
        gst_caps_unref(caps);
    }

    g_print("%*s%s: %s\n",
            2 * depth,
            " ",
            gst_discoverer_stream_info_get_stream_type_nick(info),
            (desc ? desc : ""));
    if (desc) {
        g_free(desc);
        desc = nullptr;
    }

    if (const GstTagList* tags = gst_discoverer_stream_info_get_tags(info); tags) {
        g_print("%*sTags:\n", 2 * (depth + 1), " ");
        gst_tag_list_foreach(tags, printTagForeach, GINT_TO_POINTER(depth + 2));
    }
}

/* Print information regarding a stream and its substreams, if any */
static void
printTopology(GstDiscovererStreamInfo* info, gint depth)
{
    GstDiscovererStreamInfo* next;

    if (!info)
        return;

    printStreamInfo(info, depth);

    next = gst_discoverer_stream_info_get_next(info);
    if (next) {
        printTopology(next, depth + 1);
        gst_discoverer_stream_info_unref(next);
    } else if (GST_IS_DISCOVERER_CONTAINER_INFO(info)) {
        GList *tmp, *streams;

        streams = gst_discoverer_container_info_get_streams(GST_DISCOVERER_CONTAINER_INFO(info));
        for (tmp = streams; tmp; tmp = tmp->next) {
            GstDiscovererStreamInfo* tmpinf = (GstDiscovererStreamInfo*) tmp->data;
            printTopology(tmpinf, depth + 1);
        }
        gst_discoverer_stream_info_list_free(streams);
    }
}

/* This function is called every time the discoverer has information regarding
 * one of the URIs we provided.*/
static void
onDiscovered(GstDiscoverer* discoverer, GstDiscovererInfo* info, GError* err)
{
    const gchar* uri = gst_discoverer_info_get_uri(info);
    GstDiscovererResult result = gst_discoverer_info_get_result(info);
    switch (result) {
    case GST_DISCOVERER_URI_INVALID:
        g_print("Invalid URI '%s'\n", uri);
        break;
    case GST_DISCOVERER_ERROR:
        g_print("Discoverer error: %s\n", err->message);
        break;
    case GST_DISCOVERER_TIMEOUT:
        g_print("Timeout\n");
        break;
    case GST_DISCOVERER_BUSY:
        g_print("Busy\n");
        break;
    case GST_DISCOVERER_MISSING_PLUGINS: {
        const GstStructure* s;
        gchar* str;

        s = gst_discoverer_info_get_misc(info);
        str = gst_structure_to_string(s);

        g_print("Missing plugins: %s\n", str);
        g_free(str);
        break;
    }
    case GST_DISCOVERER_OK:
        g_print("Discovered '%s'\n", uri);
        break;
    }

    if (result != GST_DISCOVERER_OK) {
        g_printerr("This URI cannot be played\n");
        return;
    }

    /* If we got no error, show the retrieved information */

    g_print("\nDuration: %" GST_TIME_FORMAT "\n",
            GST_TIME_ARGS(gst_discoverer_info_get_duration(info)));

    const GstTagList* tags = gst_discoverer_info_get_tags(info);
    if (tags) {
        g_print("Tags:\n");
        gst_tag_list_foreach(tags, printTagForeach, GINT_TO_POINTER(1));
    }
    g_print("Seekable: %s\n", (gst_discoverer_info_get_seekable(info) ? "yes" : "no"));
    g_print("\n");

    GstDiscovererStreamInfo* sinfo = gst_discoverer_info_get_stream_info(info);
    if (!sinfo) {
        return;
    }
    g_print("Stream information:\n");
    printTopology(sinfo, 1);
    g_print("\n");

    gst_discoverer_stream_info_unref(sinfo);
}

/* This function is called when the discoverer has finished examining
 * all the URIs we provided.*/
static void
on_finished_cb(GstDiscoverer* /*discoverer*/)
{
    g_print("Finished discovering\n");
    g_main_loop_quit(loop);
}

int
main(int argc, char** argv)
{
    /* if a URI was provided, use it instead of the default one */
    const gchar* uri = "https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm";
    if (argc > 1) {
        uri = argv[1];
    }
    g_print("Discovering '%s'\n", uri);

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Instantiate the Discoverer */
    GError* err = NULL;
    discoverer = gst_discoverer_new(5 * GST_SECOND, &err);
    if (not discoverer) {
        g_print("Error creating discoverer instance: %s\n", err->message);
        g_clear_error(&err);
        return EXIT_FAILURE;
    }

    /* Connect to the interesting signals */
    g_signal_connect(discoverer, "discovered", G_CALLBACK(onDiscovered), nullptr);
    g_signal_connect(discoverer, "finished", G_CALLBACK(on_finished_cb), nullptr);

    /* Start the discoverer process (nothing to do yet) */
    gst_discoverer_start(discoverer);

    /* Add a request to process asynchronously the URI passed through the command line */
    if (!gst_discoverer_discover_uri_async(discoverer, uri)) {
        g_print("Unable to start discovering URI '%s'\n", uri);
        g_object_unref(discoverer);
        return EXIT_FAILURE;
    }

    /* Create a GLib Main Loop and set it to run, so we can wait for the signals */
    loop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(loop);

    /* Stop the discoverer process */
    gst_discoverer_stop(discoverer);

    /* Free resources */
    g_object_unref(discoverer);
    g_main_loop_unref(loop);

    return 0;
}
