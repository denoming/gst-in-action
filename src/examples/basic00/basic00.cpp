#include "common/Utils.hpp"

#include <gst/gst.h>

using namespace std;

int
main(int argc, char* argv[])
{
    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Print GStreamer version */
    printVersion();

    /* Print a list with all available factories */
    printAllFactories();
}
