#pragma once

#include <gst/gst.h>

G_BEGIN_DECLS

// clang-format off
#define CUSTOM_TYPE_RT_POOL             (custom_rt_pool_get_type())
#define CUSTOM_RT_POOL(pool)            (G_TYPE_CHECK_INSTANCE_CAST((pool), CUSTOM_TYPE_RT_POOL, CustomRtPool))
#define CUSTOM_IS_RT_POOL(pool)         (G_TYPE_CHECK_INSTANCE_TYPE((pool), CUSTOM_TYPE_RT_POOL))
#define CUSTOM_RT_POOL_CLASS(pclass)    (G_TYPE_CHECK_CLASS_CAST((pclass), CUSTOM_TYPE_RT_POOL, CustomRtPoolClass))
#define CUSTOM_IS_RT_POOL_CLASS(pclass) (G_TYPE_CHECK_CLASS_TYPE((pclass), CUSTOM_TYPE_RT_POOL))
#define CUSTOM_RT_POOL_GET_CLASS(pool)  (G_TYPE_INSTANCE_GET_CLASS((pool), CUSTOM_TYPE_RT_POOL, CustomRtPoolClass))
#define CUSTOM_RT_POOL_CAST(pool)       ((CustomRtPool*)(pool))
// clang-format on

struct CustomRtPool {
    GstTaskPool object;
};

struct CustomRtPoolClass {
    GstTaskPoolClass parent_class;
};

GType
custom_rt_pool_get_type(void);
GstTaskPool*
custom_rt_pool_new(void);

G_END_DECLS
