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
