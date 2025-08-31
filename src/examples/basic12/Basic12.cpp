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

#include "CustomRtPool.hpp"

/**
 * Example 12: Manual threads configuration
 *  + creating custom threads pool
 *  + creating threads with custom policy and priority
 */

static GMainLoop* mainLoop{};

static void
onPipelineStreamStatus(GstBus* /*bus*/, GstMessage* message, gpointer /*userData*/)
{
    g_message("Received <STREAM_STATUS>");

    GstElement* owner{};
    GstStreamStatusType type{};
    gst_message_parse_stream_status(message, &type, &owner);

    g_message("...type:   %d", type);
    gchar* path = gst_object_get_path_string(GST_MESSAGE_SRC(message));
    g_message("...source: %s", path);
    g_free(path);
    path = gst_object_get_path_string(GST_OBJECT(owner));
    g_message("...owner:  %s", path);
    g_free(path);

    GstTask* task{};
    if (const GValue* val = gst_message_get_stream_status_object(message);
        G_VALUE_TYPE(val) == GST_TYPE_TASK) {
        task = static_cast<GstTask*>(g_value_get_object(val));
    }

    switch (type) {
    case GST_STREAM_STATUS_TYPE_CREATE:
        g_message("...status: create");
        if (task) {
            GstTaskPool* pool = custom_rt_pool_new();
            g_assert(pool);
            gst_task_set_pool(task, pool);
        }
        break;
    case GST_STREAM_STATUS_TYPE_ENTER:
        g_message("...status: enter");
        break;
    case GST_STREAM_STATUS_TYPE_LEAVE:
        g_message("...status: leave");
        break;
    default:
        break;
    }
}

static void
onPipelineError(GstBus* /*bus*/, GstMessage* message, gpointer /*userData*/)
{
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

static void
onPipelineEos(GstBus* bus, GstMessage* message, gpointer user_data)
{
    g_message("Received EoS");
    g_main_loop_quit(mainLoop);
}

int
main(int argc, char* argv[])
{
    // Initialize gstreamer
    gst_init(&argc, &argv);

    // Create a new bin to hold the elements
    GstElement* bin = gst_pipeline_new("pipeline");
    g_assert(bin);

    // Create source element
    GstElement* src = gst_element_factory_make("fakesrc", "src");
    g_assert(src);
    g_object_set(src, "num-buffers", 50, NULL);

    // Create sink element
    GstElement* sink = gst_element_factory_make("fakesink", "sink");
    g_assert(sink);

    // Add elements to the main pipeline
    gst_bin_add_many(GST_BIN(bin), src, sink, NULL);

    // Link the elements
    gst_element_link(src, sink);

    // Configure message handlers
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(bin));
    g_assert(bus);
    gst_bus_enable_sync_message_emission(bus);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "sync-message::stream-status", GCallback(onPipelineStreamStatus), NULL);
    g_signal_connect(bus, "message::error", GCallback(onPipelineError), NULL);
    g_signal_connect(bus, "message::eos", GCallback(onPipelineEos), NULL);
    gst_object_unref(bus);

    // Start playing
    std::ignore = gst_element_set_state(bin, GST_STATE_PLAYING);

    // Run pipeline
    mainLoop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(mainLoop);

    // Tear-down pipeline and loop
    gst_element_set_state(bin, GST_STATE_NULL);
    g_main_loop_unref(mainLoop);

    return 0;
}
