#include <gst/gst.h>

/**
 * Example 13: Identifying media type using typefind gstreamer element
 **/

static void
onPipelineError(GstBus* /*bus*/, GstMessage* message, gpointer data)
{
    auto* mainLoop = static_cast<GMainLoop*>(data);
    g_assert(mainLoop);

    GError* error{};
    gchar* debugInfo;
    gst_message_parse_error(message, &error, &debugInfo);
    g_message("Received error: %s, %s", GST_OBJECT_NAME(message->src), error->message);
    g_message("Debugging information: %s", debugInfo ? debugInfo : "none");
    g_clear_error(&error);
    g_free(debugInfo);

    // Quit from main loop
    g_main_loop_quit(mainLoop);
}

static gboolean
idle_exit_loop(gpointer data)
{
    g_main_loop_quit((GMainLoop*) data);

    /* once */
    return FALSE;
}

static void
onTypeFound(GstElement* /*typefind*/, const guint probability, GstCaps* caps, gpointer data)
{
    auto* mainLoop = static_cast<GMainLoop*>(data);
    g_assert(mainLoop);

    gchar* type = gst_caps_to_string(caps);
    g_print("Media type %s found, probability %d%%\n", type, probability);
    g_free(type);

    /* since we connect to a signal in the pipeline thread context, we need
     * to set an idle handler to exit the main loop in the mainloop context.
     * Normally, your app should not need to worry about such things. */
    g_idle_add(idle_exit_loop, mainLoop);
}

int
main(int argc, char* argv[])
{
    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Check args
    if (argc != 2) {
        g_print("Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    GMainLoop* mainLoop = g_main_loop_new(nullptr, FALSE);
    g_assert(mainLoop);

    // Create a new pipeline to hold the elements
    GstElement* pipeline = gst_pipeline_new("pipe");

    // Configure message handlers
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    g_assert(bus);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message::error", GCallback(onPipelineError), mainLoop);
    gst_object_unref(bus);

    // Create file source and typefind element
    GstElement* src = gst_element_factory_make("filesrc", "src");
    g_assert(src);
    g_object_set(G_OBJECT(src), "location", argv[1], NULL);
    GstElement* typefind = gst_element_factory_make("typefind", "typefinder");
    g_assert(typefind);
    g_signal_connect(typefind, "have-type", G_CALLBACK(onTypeFound), mainLoop);
    GstElement* sink = gst_element_factory_make("fakesink", "sink");
    g_assert(sink);

    // Setup
    gst_bin_add_many(GST_BIN(pipeline), src, typefind, sink, NULL);
    gst_element_link_many(src, typefind, sink, NULL);
    gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);

    g_main_loop_run(mainLoop);

    // Unset
    gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));

    return EXIT_SUCCESS;
}
