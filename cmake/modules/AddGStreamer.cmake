find_package(PkgConfig)

pkg_check_modules(GStreamer REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_check_modules(GStreamerBase REQUIRED IMPORTED_TARGET gstreamer-base-1.0)
pkg_check_modules(GStreamerPluginsBase REQUIRED IMPORTED_TARGET gstreamer-plugins-base-1.0)
pkg_check_modules(GStreamerPluginsBad REQUIRED IMPORTED_TARGET gstreamer-plugins-bad-1.0)
