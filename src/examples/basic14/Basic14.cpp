#include <gst/gst.h>

/**
 * Example 14: Data buffer modification using data probes
 **/

static const gint kWidht = 384;
static const gint kHeight = 288;

/**
 * This method is runnig under stream thread.
 **/
static GstPadProbeReturn
onHaveData(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    buffer = gst_buffer_make_writable(buffer);

    /**
     * Making a buffer writable can fail (for example if it
     * cannot be copied and is used more than once)
     */
    if (buffer == nullptr) {
        return GST_PAD_PROBE_OK;
    }

    /* Mapping a buffer can fail (non-writable) */
    GstMapInfo map;
    guint16 *ptr{}, t{};
    if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
        ptr = reinterpret_cast<guint16*>(map.data);
        // Invert data
        for (gint y = 0; y < kHeight; y++) {
            for (gint x = 0; x < kWidht / 2; x++) {
                t = ptr[kWidht - 1 - x];
                ptr[kWidht - 1 - x] = ptr[x];
                ptr[x] = t;
            }
            ptr += kWidht;
        }
        gst_buffer_unmap(buffer, &map);
    }

    GST_PAD_PROBE_INFO_DATA(info) = buffer;

    return GST_PAD_PROBE_OK;
}

int
main(int argc, char* argv[])
{
    // Initialize GStreamer
    gst_init(&argc, &argv);
    GMainLoop* loop = g_main_loop_new(nullptr, FALSE);

    // Build
    GstElement* pipeline = gst_pipeline_new("my-pipeline");
    GstElement* src = gst_element_factory_make("videotestsrc", "src");
    g_assert(src != nullptr);
    GstElement* filter = gst_element_factory_make("capsfilter", "filter");
    g_assert(filter != nullptr);
    GstElement* convert = gst_element_factory_make("videoconvert", "csp");
    g_assert(convert != nullptr);
    GstElement* sink = gst_element_factory_make("xvimagesink", "sink");
    if (sink == nullptr) {
        sink = gst_element_factory_make("ximagesink", "sink");
        g_assert(sink != nullptr);
    }

    gst_bin_add_many(GST_BIN(pipeline), src, filter, convert, sink, NULL);
    gst_element_link_many(src, filter, convert, sink, NULL);
    GstCaps* filterCaps = gst_caps_new_simple("video/x-raw",
                                              "format",
                                              G_TYPE_STRING,
                                              "RGB16",
                                              "width",
                                              G_TYPE_INT,
                                              kWidht,
                                              "height",
                                              G_TYPE_INT,
                                              kHeight,
                                              "framerate",
                                              GST_TYPE_FRACTION,
                                              25,
                                              1,
                                              NULL);
    g_object_set(G_OBJECT(filter), "caps", filterCaps, NULL);
    gst_caps_unref(filterCaps);

    GstPad* pad = gst_element_get_static_pad(src, "src");
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, onHaveData, nullptr, nullptr);
    gst_object_unref(pad);

    // Run
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Wait until it's up and running or failed
    if (gst_element_get_state(pipeline, nullptr, nullptr, -1) == GST_STATE_CHANGE_FAILURE) {
        g_error("Failed to go into PLAYING state");
    }

    g_print("Running ...\n");
    g_main_loop_run(loop);

    // Exit
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}
