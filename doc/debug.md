# Debug

## Logging

Use `GST_DEBUG` environment variable to set log level. This environment variable is a comma-separated
list of `category:level` pairs, with an optional _level_ at the beginning, representing the default
level for all categories.

The wildcard `*` is available to select a bunch of categories and set particular debug level.
For example: `GST_DEBUG=2,audio*:6` will use `Debug` level for all categories starting with
the word `audio` and `Warning` for other categories.

The command `gst-launch-1.0 --gst-debug-help` lists all registered categories.

### Adding own debug information

To log use following macros: `GST_ERROR()`, `GST_WARNING()`, `GST_INFO()`, `GST_LOG()`, `GST_DEBUG()`.

To change the category from default to custom one in compilation unit:
```cpp
GST_DEBUG_CATEGORY_STATIC (my_category);
#define GST_CAT_DEFAULT my_category
```
To register custom category:
```cpp
GST_DEBUG_CATEGORY_INIT (my_category, "my category", 0, "This is my very own");
```

## Graph

Set `GST_DEBUG_DUMP_DOT_DIR` environment variable to point to exist folde where `.dot` file will be placed.

Use following macros to generate `.dot` file in code:
* `GST_DEBUG_BIN_TO_DOT_FILE()`
* `GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS()`


