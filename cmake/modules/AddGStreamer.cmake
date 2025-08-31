# Copyright 2025 Denys Asauliak
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

find_package(PkgConfig)

pkg_check_modules(GStreamer REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_check_modules(GStreamerBase REQUIRED IMPORTED_TARGET gstreamer-base-1.0)
pkg_check_modules(GStreamerAudio REQUIRED IMPORTED_TARGET gstreamer-audio-1.0)
pkg_check_modules(GStreamerPbUtils REQUIRED IMPORTED_TARGET gstreamer-pbutils-1.0)
pkg_check_modules(GStreamerPluginsBase REQUIRED IMPORTED_TARGET gstreamer-plugins-base-1.0)
pkg_check_modules(GStreamerPluginsBad REQUIRED IMPORTED_TARGET gstreamer-plugins-bad-1.0)
