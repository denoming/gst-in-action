#pragma once

#include <filesystem>

#include <gst/gstelement.h>

void
printVersion();

void
printAllFactories();

void
printAllStreamTags(const std::filesystem::path& path);

void
printAllPadCaps(GstElement* element, const gchar* padName);

void
printPadTemplatesInfo(GstElementFactory* factory);
