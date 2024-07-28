#include "common/Utils.hpp"

#include <gst/gst.h>

namespace {

void
printFeature(GstPluginFeature* feature, void* data)
{
    const gchar* name = gst_plugin_feature_get_name(feature);
    if (name != nullptr) {
        g_print("Plugin Name: '%s'\n", name);
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
    g_list_foreach(list, GFunc(&printFeature), nullptr);
    gst_plugin_feature_list_free(list);
}
