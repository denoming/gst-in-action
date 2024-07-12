#include <gst/gst.h>

#include <iostream>

using namespace std;

/**
 * Media formats and pad capabilities
 **/

/* Functions below print the capabilities in a human-friendly format */
static gboolean
printField(GQuark field, const GValue* value, gpointer pfx)
{
    gchar* str = gst_value_serialize(value);
    g_print("%s  %15s: %s\n", (gchar*) pfx, g_quark_to_string(field), str);
    g_free(str);
    return TRUE;
}

static void
printCaps(const GstCaps* caps, const gchar* pfx)
{
    g_return_if_fail(caps != NULL);

    if (gst_caps_is_any(caps)) {
        g_print("%sANY\n", pfx);
        return;
    }
    if (gst_caps_is_empty(caps)) {
        g_print("%sEMPTY\n", pfx);
        return;
    }

    for (guint i = 0; i < gst_caps_get_size(caps); i++) {
        GstStructure* structure = gst_caps_get_structure(caps, i);
        g_print("%s%s\n", pfx, gst_structure_get_name(structure));
        gst_structure_foreach(structure, printField, (gpointer) pfx);
    }
}

/* Prints information about a pad template, including its capabilities */
static void
printPadTemplatesInfo(GstElementFactory* factory)
{
    g_print("Pad Templates for %s:\n", gst_element_factory_get_longname(factory));
    if (!gst_element_factory_get_num_pad_templates(factory)) {
        g_print("  none\n");
        return;
    }

    const GList* pads = gst_element_factory_get_static_pad_templates(factory);
    while (pads) {
        auto* padTemplate = static_cast<GstStaticPadTemplate*>(pads->data);
        pads = g_list_next(pads);

        if (padTemplate->direction == GST_PAD_SRC)
            g_print("  SRC template: '%s'\n", padTemplate->name_template);
        else if (padTemplate->direction == GST_PAD_SINK)
            g_print("  SINK template: '%s'\n", padTemplate->name_template);
        else
            g_print("  Unknown template: '%s'\n", padTemplate->name_template);

        if (padTemplate->presence == GST_PAD_ALWAYS)
            g_print("    Availability: Always\n");
        else if (padTemplate->presence == GST_PAD_SOMETIMES)
            g_print("    Availability: Sometimes\n");
        else if (padTemplate->presence == GST_PAD_REQUEST)
            g_print("    Availability: On request\n");
        else
            g_print("    Availability: Unknown\n");

        if (padTemplate->static_caps.string) {
            GstCaps* caps;
            g_print("    Capabilities:\n");
            caps = gst_static_caps_get(&padTemplate->static_caps);
            printCaps(caps, "      ");
            gst_caps_unref(caps);
        }

        g_print("\n");
    }
}

/* Shows the CURRENT capabilities of the requested pad in the given element */
static void
printPadCaps(GstElement* element, const gchar* padName)
{
    /* Retrieve pad */
    GstPad* pad = gst_element_get_static_pad(element, padName);
    if (not pad) {
        g_printerr("Could not retrieve pad '%s'\n", padName);
        return;
    }

    /* Retrieve negotiated caps (or acceptable caps if negotiation is not finished yet) */
    GstCaps* caps = gst_pad_get_current_caps(pad);
    if (not caps) {
        caps = gst_pad_query_caps(pad, nullptr);
    }

    /* Print and free */
    g_print("Caps for the %s pad:\n", padName);
    printCaps(caps, "      ");
    gst_caps_unref(caps);
    gst_object_unref(pad);
}

int
main(int argc, char* argv[])
{
    gboolean terminate = FALSE;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the element factories */
    GstElementFactory* sourceFactory = gst_element_factory_find("audiotestsrc");
    GstElementFactory* sinkFactory = gst_element_factory_find("autoaudiosink");
    if (!sourceFactory || !sinkFactory) {
        g_printerr("Not all element factories could be created.\n");
        return -1;
    }

    /* Print information about the pad templates of these factories */
    printPadTemplatesInfo(sourceFactory);
    printPadTemplatesInfo(sinkFactory);

    /* Ask the factories to instantiate actual elements */
    GstElement* source = gst_element_factory_create(sourceFactory, "source");
    GstElement* sink = gst_element_factory_create(sinkFactory, "sink");

    /* Create the empty pipeline */
    GstElement* pipeline = gst_pipeline_new("test-pipeline");

    if (!pipeline || !source || !sink) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    /* Build the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), source, sink, NULL);
    if (gst_element_link(source, sink) != TRUE) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    /* Print initial negotiated caps (in NULL state) */
    g_print("In NULL state:\n");
    printPadCaps(sink, "sink");

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
                    printPadCaps(sink, "sink");
                }
                break;
            default:
                /* We should not reach here because we only asked for ERRORs, EOS and STATE_CHANGED
                 */
                g_printerr("Unexpected message received.\n");
                break;
            }
            gst_message_unref(msg);
        }
    }
    while (!terminate);

    /* Free resources */
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    gst_object_unref(sourceFactory);
    gst_object_unref(sinkFactory);
    return 0;
}
