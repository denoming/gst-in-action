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

static gchar* selectedEffects{};

#define DEFAULT_EFFECTS                                                                            \
    "identity,exclusion,navigationtest,"                                                           \
    "agingtv,videoflip,vertigotv,gaussianblur,shagadelictv,edgetv"

static GstPad* blockpad{};
static GstElement* convBefore{};
static GstElement* convAfter{};
static GstElement* curEffect{};
static GstElement* pipeline{};

static GQueue effects = G_QUEUE_INIT;

/* Called then EoS event leaves SRC pad of current effect element */
static GstPadProbeReturn
onProbeEvent(GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    auto* loop = static_cast<GMainLoop*>(data);
    g_assert(loop != nullptr);

    if (GST_EVENT_TYPE(GST_PAD_PROBE_INFO_DATA(info)) != GST_EVENT_EOS) {
        return GST_PAD_PROBE_OK;
    }

    /* Removed event probe */
    gst_pad_remove_probe(pad, GST_PAD_PROBE_INFO_ID(info));

    /* Take next effect from the queue */
    auto* next = static_cast<GstElement*>(g_queue_pop_head(&effects));
    if (next == nullptr) {
        GST_DEBUG_OBJECT(pad, "no more effects");
        g_main_loop_quit(loop);
        return GST_PAD_PROBE_DROP;
    }

    g_print("Switching from '%s' to '%s'..\n", GST_OBJECT_NAME(curEffect), GST_OBJECT_NAME(next));

    /* Nullify current element */
    gst_element_set_state(curEffect, GST_STATE_NULL);

    /* Remove unlinks automatically */
    GST_DEBUG_OBJECT(pipeline, "removing %" GST_PTR_FORMAT, curEffect);
    gst_bin_remove(GST_BIN(pipeline), curEffect);

    /* Push current effect back into the queue */
    g_queue_push_tail(&effects, g_steal_pointer(&curEffect));

    /* Add and link new effect */
    GST_DEBUG_OBJECT(pipeline, "adding %" GST_PTR_FORMAT, next);
    gst_bin_add(GST_BIN(pipeline), next);
    GST_DEBUG_OBJECT(pipeline, "linking...");
    gst_element_link_many(convBefore, next, convAfter, NULL);

    /* Start new effect */
    gst_element_set_state(next, GST_STATE_PLAYING);

    curEffect = next;
    GST_DEBUG_OBJECT(pipeline, "done");

    return GST_PAD_PROBE_DROP;
}

/* Called when SRC pad of queue (q1) is blocked successfully */
static GstPadProbeReturn
onPadProbe(GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    GST_DEBUG_OBJECT(pad, "Pad is blocked now");

    /* Remove the probe first */
    gst_pad_remove_probe(pad, GST_PAD_PROBE_INFO_ID(info));

    /* Install new probe for EoS (we need to flush removing element internal data) */
    GstPad* srcPad = gst_element_get_static_pad(curEffect, "src");
    gst_pad_add_probe(
        srcPad,
        GstPadProbeType(GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM),
        onProbeEvent,
        data,
        nullptr);
    gst_object_unref(srcPad);

    /**
     * Push EoS into the element, the probe will be fired when the
     * EoS leaves the effect (the element is "clear" and can be removed from pipeline)
     */
    GstPad* sinkPad = gst_element_get_static_pad(curEffect, "sink");
    gst_pad_send_event(sinkPad, gst_event_new_eos());
    gst_object_unref(sinkPad);

    return GST_PAD_PROBE_OK;
}

/* Called after 1 second timeout */
static gboolean
onTimeout(gpointer data)
{
    /* Block SRC pad on queue (q1) */
    gst_pad_add_probe(blockpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, onPadProbe, data, NULL);
    return TRUE;
}

static gboolean
onBusMessage(GstBus* /*bus*/, GstMessage* msg, gpointer data)
{
    auto* loop = static_cast<GMainLoop*>(data);
    g_assert(loop != nullptr);

    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR: {
        GError* err{};
        gchar* dbg;
        gst_message_parse_error(msg, &err, &dbg);
        gst_object_default_error(msg->src, err, dbg);
        g_clear_error(&err);
        g_free(dbg);
        g_main_loop_quit(loop);
        break;
    }
    default:
        break;
    }
    return TRUE;
}

int
main(int argc, char** argv)
{
    GOptionEntry options[] = {{"effects",
                               'e',
                               0,
                               G_OPTION_ARG_STRING,
                               &selectedEffects,
                               "Effects to use (comma-separated list of element names)",
                               nullptr},
                              {nullptr}};

    GOptionContext* ctx = g_option_context_new("");
    g_option_context_add_main_entries(ctx, options, nullptr);
    g_option_context_add_group(ctx, gst_init_get_option_group());

    GError* err{};
    if (!g_option_context_parse(ctx, &argc, &argv, &err)) {
        g_error("Error initializing: %s\n", err->message);
        return EXIT_FAILURE;
    }
    g_option_context_free(ctx);

    gchar** effectNames{};
    if (selectedEffects != nullptr) {
        effectNames = g_strsplit(selectedEffects, ",", -1);
    } else {
        effectNames = g_strsplit(DEFAULT_EFFECTS, ",", -1);
    }

    gchar** e{};
    for (e = effectNames; e != nullptr && *e != nullptr; ++e) {
        if (GstElement* el = gst_element_factory_make(*e, nullptr); el) {
            g_print("Adding effect '%s'\n", *e);
            g_queue_push_tail(&effects, gst_object_ref_sink(el));
        }
    }

    pipeline = gst_pipeline_new("pipeline");

    GstElement* src = gst_element_factory_make("videotestsrc", nullptr);
    g_assert(src != nullptr);
    g_object_set(src, "is-live", TRUE, NULL);
    GstElement* filter = gst_element_factory_make("capsfilter", nullptr);
    g_assert(filter != nullptr);
    gst_util_set_object_arg(G_OBJECT(filter),
                            "caps",
                            "video/x-raw, width=320, height=240, "
                            "format={ I420, YV12, YUY2, UYVY, AYUV, Y41B, Y42B, "
                            "YVYU, Y444, v210, v216, NV12, NV21, UYVP, A420, YUV9, YVU9, IYU1 }");

    GstElement* q1 = gst_element_factory_make("queue", nullptr);
    g_assert(q1 != nullptr);
    blockpad = gst_element_get_static_pad(q1, "src");
    convBefore = gst_element_factory_make("videoconvert", nullptr);
    g_assert(q1 != nullptr);
    convAfter = gst_element_factory_make("videoconvert", nullptr);
    g_assert(q1 != nullptr);
    GstElement* q2 = gst_element_factory_make("queue", nullptr);
    g_assert(q1 != nullptr);
    GstElement* sink = gst_element_factory_make("ximagesink", nullptr);
    g_assert(q1 != nullptr);

    auto* effect = static_cast<GstElement*>(g_queue_pop_head(&effects));
    curEffect = effect;

    gst_bin_add_many(
        GST_BIN(pipeline), src, filter, q1, convBefore, effect, convAfter, q2, sink, nullptr);
    gst_element_link_many(src, filter, q1, convBefore, effect, convAfter, q2, sink, nullptr);

    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        g_error("Error starting pipeline");
        return EXIT_FAILURE;
    }

    GMainLoop* loop = g_main_loop_new(nullptr, FALSE);
    g_assert(loop != nullptr);
    gst_bus_add_watch(GST_ELEMENT_BUS(pipeline), onBusMessage, loop);
    g_timeout_add_seconds(1, onTimeout, loop);
    g_main_loop_run(loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);

    gst_object_unref(blockpad);
    gst_bus_remove_watch(GST_ELEMENT_BUS(pipeline));
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);

    g_queue_clear_full(&effects, gst_object_unref);
    gst_object_unref(curEffect);
    g_strfreev(effectNames);

    return EXIT_SUCCESS;
}
