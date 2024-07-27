#include "common/Utils.hpp"

#include <gst/gst.h>

void
printVersion()
{
    guint major, minor, micro, nano;
    gst_version(&major, &minor, &micro, &nano);
    const gchar* nanoStr = (nano == 1) ? "(CVS)" : (nano == 2) ? "(Prerelease)" : "";
    g_print("GStreamer %d.%d.%d %s\n", major, minor, micro, nanoStr);
}
