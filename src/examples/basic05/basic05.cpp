#include <gst/gst.h>

#include <iostream>

#include "common/Utils.hpp"

using namespace std;

/**
 * Media formats and pad capabilities
 **/

int
main(int argc, char* argv[])
{
    gboolean terminate = FALSE;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the element factories */
    GstElementFactory* sourceFactory = gst_element_factory_find("audiotestsrc");
    GstElementFactory* sinkFactory = gst_element_factory_find("autoaudiosink");
    if (!sourceFactory or not sinkFactory) {
        g_printerr("Not all element factories could be created.\n");
        return EXIT_FAILURE;
    }

    /* Print information about the pad templates of these factories */
    printPadTemplatesInfo(sourceFactory);
    printPadTemplatesInfo(sinkFactory);

    /* Ask the factories to instantiate actual elements */
    GstElement* source = gst_element_factory_create(sourceFactory, "source");
    GstElement* sink = gst_element_factory_create(sinkFactory, "sink");

    /* Create the empty pipeline */
    GstElement* pipeline = gst_pipeline_new("test-pipeline");

    if (not pipeline or not source or not sink) {
        g_printerr("Not all elements could be created.\n");
        return EXIT_FAILURE;
    }

    /* Build the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), source, sink, NULL);
    if (gst_element_link(source, sink) != TRUE) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return EXIT_FAILURE;
    }

    /* Print initial negotiated caps (in NULL state) */
    g_print("In NULL state:\n");
    printAllPadCaps(sink, "sink");

    /* Start playing */
    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state (check the bus for error "
                   "messages).\n");
    }

    /* Wait until error, EOS or State Change */
    GstBus* bus = gst_element_get_bus(pipeline);
    do {
        GstMessage* msg = gst_bus_timed_pop_filtered(
            bus,
            GST_CLOCK_TIME_NONE,
            static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS
                                        | GST_MESSAGE_STATE_CHANGED));

        /* Parse message */
        if (msg != nullptr) {
            GError* err{};
            gchar* debugInfo{};

            switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debugInfo);
                g_printerr("Error received from element %s: %s\n",
                           GST_OBJECT_NAME(msg->src),
                           err->message);
                g_printerr("Debugging information: %s\n", debugInfo ? debugInfo : "none");
                g_clear_error(&err);
                g_free(debugInfo);
                terminate = TRUE;
                break;
            case GST_MESSAGE_EOS:
                g_print("End-Of-Stream reached.\n");
                terminate = TRUE;
                break;
            case GST_MESSAGE_STATE_CHANGED:
                /* We are only interested in state-changed messages from the pipeline */
                if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
                    GstState oldState, newState, pendingState;
                    gst_message_parse_state_changed(msg, &oldState, &newState, &pendingState);
                    g_print("\nPipeline state changed from %s to %s:\n",
                            gst_element_state_get_name(oldState),
                            gst_element_state_get_name(newState));
                    /* Print the current capabilities of the sink element */
                    printAllPadCaps(sink, "sink");
                }
                break;
            default:
                /* We should not reach here */
                g_printerr("Unexpected message received.\n");
                break;
            }
            gst_message_unref(msg);
        }
    }
    while (not terminate);

    /* Free resources */
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    gst_object_unref(sourceFactory);
    gst_object_unref(sinkFactory);

    return EXIT_SUCCESS;
}
