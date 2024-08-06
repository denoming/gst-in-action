#include "common/Utils.hpp"

#include <gst/gst.h>

#include <gst/video/colorbalance.h>

static GstElement* pipeline{};

static void
listBalanceChannels(GstColorBalance* balance)
{
    const GList* ptr = gst_color_balance_list_channels(balance);
    while (ptr != nullptr) {
        auto* ch = static_cast<GstColorBalanceChannel*>(ptr->data);
        g_print("Channel: label<%s>, min<%d>, max<%d>, cur<%d>\n",
                ch->label,
                ch->min_value,
                ch->max_value,
                gst_color_balance_get_value(balance, ch));
        ptr = ptr->next;
    }
}

static void
setChannelValue(GstColorBalance* balance, const gchar* label, gint newValue)
{
    const GList* ptr = gst_color_balance_list_channels(balance);
    while (ptr != nullptr) {
        if (auto* ch = static_cast<GstColorBalanceChannel*>(ptr->data);
            g_str_equal(ch->label, label)) {
            gst_color_balance_set_value(balance, ch, newValue);
        }
        ptr = ptr->next;
    }
}

static void
handleMessages()
{
    bool terminate{};
    constexpr auto targetTypes = static_cast<GstMessageType>(
        GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR // List of target types
    );

    GstBus* bus = gst_element_get_bus(pipeline);
    g_assert_nonnull(bus);

    do {
        if (GstMessage* msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, targetTypes);
            msg != nullptr) {
            switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR: {
                GError* err{};
                gchar* debugInfo{};
                gst_message_parse_error(msg, &err, &debugInfo);
                g_printerr("Error received from element %s: %s\n",
                           GST_OBJECT_NAME(msg->src),
                           err->message);
                g_printerr("Debugging information: %s\n", debugInfo ? debugInfo : "none");
                g_clear_error(&err);
                g_free(debugInfo);
                terminate = true;
                break;
            }
            case GST_MESSAGE_STATE_CHANGED: {
                if (g_str_equal(GST_OBJECT_NAME(msg->src), "source")) {
                    GstState oldState, newState;
                    gst_message_parse_state_changed(msg, &oldState, &newState, nullptr);
                    if (newState == GST_STATE_PLAYING) {
                        printAllPadCaps(GST_ELEMENT(msg->src), "src");
                    }
                }
                break;
            }
            default:
                g_printerr("Unexpected message\n");
                break;
            }
            gst_message_unref(msg);
        }
    }
    while (not terminate);
    gst_object_unref(bus);
}

int
main(int argc, char* argv[])
{
    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    GstElement* const source = gst_element_factory_make("v4l2src", "source");
    g_assert_nonnull(source);
    GstElement* const rate = gst_element_factory_make("videorate", "rate");
    g_assert_nonnull(rate);
    GstElement* const convert = gst_element_factory_make("videoconvert", "convert");
    g_assert_nonnull(convert);
    GstElement* const sink = gst_element_factory_make("autovideosink", "sink");
    g_assert_nonnull(sink);

    /* Create the empty pipeline */
    pipeline = gst_pipeline_new("basic11");
    g_assert_nonnull(pipeline);

    /* Build the pipeline (must be done before linking) */
    gst_bin_add_many(GST_BIN(pipeline), source, rate, convert, sink, nullptr);
    g_assert_true(gst_element_link(source, rate));
    GstCaps* caps = gst_caps_from_string("video/x-raw,framerate=30/1");
    g_assert_nonnull(caps);
    g_assert_true(gst_element_link_filtered(rate, convert, caps));
    g_assert_true(gst_element_link(convert, sink));

    /* Start playing */
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }

    // {{

    g_assert_true(GST_IS_COLOR_BALANCE(source));
    GstColorBalance* balance = GST_COLOR_BALANCE(source);
    g_assert_nonnull(balance);

    /* Get type */
    const auto type = gst_color_balance_get_balance_type(balance);
    g_print("Color balance: ptr<%p>, type<%s>\n",
            balance,
            (type == GST_COLOR_BALANCE_HARDWARE) ? "Hardware" : "Software");

    /* Change available channels */
    listBalanceChannels(balance);

    /* Change values  of specifuc channels */
    setChannelValue(balance, "brightness", 50);
    setChannelValue(balance, "saturation", 200);

    // }}

    /* Parse messages */
    handleMessages();

    /* Free resources */
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return EXIT_SUCCESS;
}
