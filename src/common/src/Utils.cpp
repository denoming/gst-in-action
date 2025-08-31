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

#include "common/Utils.hpp"

#include <gst/gst.h>

namespace {

void
printRegistryFeature(GstPluginFeature* feature, void* /*data*/)
{
    const gchar* name = gst_plugin_feature_get_name(feature);
    if (name != nullptr) {
        g_print("Plugin Name: '%s'\n", name);
    }
}

void
onGettingStreamTagsPadAdded(GstElement* dec, GstPad* pad, GstElement* sink)
{
    GstPad* sinkpad = gst_element_get_static_pad(sink, "sink");
    if (not gst_pad_is_linked(sinkpad)) {
        if (gst_pad_link(pad, sinkpad) != GST_PAD_LINK_OK) {
            g_error("Failed to link pads!");
        }
    }
    gst_object_unref(sinkpad);
}

void
printOneStreamTag(const GstTagList* list, const gchar* tag, gpointer data)
{
    const guint tagSize = gst_tag_list_get_tag_size(list, tag);
    for (int i = 0; i < tagSize; ++i) {
        const GValue* val{};
        /* Note: when looking for specific tags, use the gst_tag_list_get_xyz() API,
         * we only use the GValue approach here because it is more generic */
        val = gst_tag_list_get_value_index(list, tag, i);
        if (G_VALUE_HOLDS_STRING(val)) {
            g_print("\t%20s : %s\n", tag, g_value_get_string(val));
        } else if (G_VALUE_HOLDS_UINT(val)) {
            g_print("\t%20s : %u\n", tag, g_value_get_uint(val));
        } else if (G_VALUE_HOLDS_DOUBLE(val)) {
            g_print("\t%20s : %g\n", tag, g_value_get_double(val));
        } else if (G_VALUE_HOLDS_BOOLEAN(val)) {
            g_print("\t%20s : %s\n", tag, (g_value_get_boolean(val)) ? "true" : "false");
        } else if (GST_VALUE_HOLDS_BUFFER(val)) {
            GstBuffer* buffer = gst_value_get_buffer(val);
            guint bufferSize = gst_buffer_get_size(buffer);
            g_print("\t%20s : buffer of size %u\n", tag, bufferSize);
        } else if (GST_VALUE_HOLDS_DATE_TIME(val)) {
            auto* dt = static_cast<GstDateTime*>(g_value_get_boxed(val));
            gchar* dtStr = gst_date_time_to_iso8601_string(dt);
            g_print("\t%20s : %s\n", tag, dtStr);
            g_free(dtStr);
        } else {
            g_print("\t%20s : tag of type '%s'\n", tag, G_VALUE_TYPE_NAME(val));
        }
    }
}

gboolean
printCapsField(const GQuark field, const GValue* value, gpointer prefix)
{
    gchar* str = gst_value_serialize(value);
    g_print("%s  %15s: %s\n", static_cast<gchar*>(prefix), g_quark_to_string(field), str);
    g_free(str);
    return TRUE;
}

void
printCaps(const GstCaps* caps, const gchar* prefix)
{
    g_return_if_fail(caps != nullptr);

    if (gst_caps_is_any(caps)) {
        g_print("%sANY\n", prefix);
        return;
    }
    if (gst_caps_is_empty(caps)) {
        g_print("%sEMPTY\n", prefix);
        return;
    }

    for (guint i = 0; i < gst_caps_get_size(caps); i++) {
        GstStructure* structure = gst_caps_get_structure(caps, i);
        g_print("%s%s\n", prefix, gst_structure_get_name(structure));
        gst_structure_foreach(structure, printCapsField, gpointer(prefix));
    }
}

} // namespace

void
printVersion()
{
    guint major, minor, micro, nano;
    gst_version(&major, &minor, &micro, &nano);
    const gchar* nanoStr = (nano == 1) ? "(CVS)" : (nano == 2) ? "(Prerelease)" : "";
    g_print("GStreamer %d.%d.%d %s\n", major, minor, micro, nanoStr);
}

void
printAllFactories()
{
    GstRegistry* registry = gst_registry_get();
    g_assert_nonnull(registry);
    GList* list = gst_registry_get_feature_list(registry, GST_TYPE_ELEMENT_FACTORY);
    g_assert_nonnull(list);
    g_list_foreach(list, GFunc(&printRegistryFeature), nullptr);
    gst_plugin_feature_list_free(list);
}

void
printAllStreamTags(const std::filesystem::path& path)
{
    gchar* uri{};
    if (gst_uri_is_valid(path.c_str())) {
        uri = g_strdup(path.c_str());
    } else {
        uri = gst_filename_to_uri(path.c_str(), nullptr);
    }

    GstElement* pipe = gst_pipeline_new("pipeline");
    g_assert_nonnull(pipe);

    GstElement* dec = gst_element_factory_make("uridecodebin", nullptr);
    g_assert_nonnull(pipe);
    g_object_set(dec, "uri", uri, nullptr);
    gst_bin_add(GST_BIN(pipe), dec);

    GstElement* sink = gst_element_factory_make("fakesink", nullptr);
    g_assert_nonnull(pipe);
    gst_bin_add(GST_BIN(pipe), sink);

    g_signal_connect(dec, "pad-added", G_CALLBACK(onGettingStreamTagsPadAdded), sink);
    gst_element_set_state(pipe, GST_STATE_PAUSED);

    GstMessage* msg{};
    while (TRUE) {
        msg = gst_bus_timed_pop_filtered(
            GST_ELEMENT_BUS(pipe),
            GST_CLOCK_TIME_NONE,
            GstMessageType(GST_MESSAGE_ASYNC_DONE | GST_MESSAGE_TAG | GST_MESSAGE_ERROR));

        if (GST_MESSAGE_TYPE(msg) != GST_MESSAGE_TAG) /* error or async_done */ {
            break;
        }

        GstTagList* tags = nullptr;
        gst_message_parse_tag(msg, &tags);

        g_print("Got tags from element %s:\n", GST_OBJECT_NAME(msg->src));
        gst_tag_list_foreach(tags, printOneStreamTag, nullptr);
        g_print("\n");
        gst_tag_list_unref(tags);

        gst_message_unref(msg);
    }

    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        GError* err{};
        gst_message_parse_error(msg, &err, nullptr);
        g_printerr("Got error: %s\n", err->message);
        g_error_free(err);
    }

    gst_message_unref(msg);
    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
    g_free(uri);
}

void
printAllPadCaps(GstElement* element, const gchar* padName)
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
    printCaps(caps, "   ");
    gst_caps_unref(caps);
    gst_object_unref(pad);
}

void
printPadTemplatesInfo(GstElementFactory* factory)
{
    g_print("Pad Templates for %s:\n", gst_element_factory_get_longname(factory));
    if (not gst_element_factory_get_num_pad_templates(factory)) {
        g_print("  none\n");
        return;
    }

    const GList* pads = gst_element_factory_get_static_pad_templates(factory);
    while (pads) {
        auto* padTemplate = static_cast<GstStaticPadTemplate*>(pads->data);
        pads = g_list_next(pads);

        if (padTemplate->direction == GST_PAD_SRC) {
            g_print("  SRC template: '%s'\n", padTemplate->name_template);
        } else if (padTemplate->direction == GST_PAD_SINK) {
            g_print("  SINK template: '%s'\n", padTemplate->name_template);
        } else {
            g_print("  Unknown template: '%s'\n", padTemplate->name_template);
        }

        if (padTemplate->presence == GST_PAD_ALWAYS) {
            g_print("    Availability: Always\n");
        } else if (padTemplate->presence == GST_PAD_SOMETIMES) {
            g_print("    Availability: Always\n");
        } else if (padTemplate->presence == GST_PAD_REQUEST) {
            g_print("    Availability: On request\n");
        } else {
            g_print("    Availability: Unknown\n");
        }

        if (padTemplate->static_caps.string) {
            g_print("    Capabilities:\n");
            GstCaps* caps = gst_static_caps_get(&padTemplate->static_caps);
            printCaps(caps, "      ");
            gst_caps_unref(caps);
        }

        g_print("\n");
    }
}
