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

#include "CustomRtPool.hpp"

#include <pthread.h>

static void
custom_rt_pool_finalize(GObject* object);

typedef struct {
    pthread_t thread{};
} CustomRtId;

G_DEFINE_TYPE(CustomRtPool, custom_rt_pool, GST_TYPE_TASK_POOL);

static void
defaultPrepare(GstTaskPool* pool, GError** /*error*/)
{
    /**
     * We don't do anything here.
     * We could construct a pool of threads here to reuse it later.
     */
    g_message("Pool %p: prepare", pool);
}

static void
defaultCleanup(GstTaskPool* pool)
{
    g_message("Pool %p: cleanup", pool);
}

static gpointer
defaultPush(GstTaskPool* pool, GstTaskPoolFunction func, gpointer data, GError** error)
{
    g_message("Pool %p: pushing %p", pool, func);
    CustomRtId* tid = g_slice_new0(CustomRtId);

    g_message("Pool %p: set policy", pool);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (const gint rv = pthread_attr_setschedpolicy(&attr, SCHED_RR); rv != 0) {
        g_warning("Pool class %p: set policy has failed, %s", pool, g_strerror(rv));
    }

    g_message("Pool %p: set priority", pool);
    sched_param param{};
    param.sched_priority = 50;
    if (const gint rv = pthread_attr_setschedparam(&attr, &param); rv != 0) {
        g_warning("Pool class %p: set priority has failed, %s", pool, g_strerror(rv));
    }

    g_message("Pool %p: set inherit", pool);
    if (const gint rv = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED); rv != 0) {
        g_warning("Pool class %p: set inherit has failed, %s", pool, g_strerror(rv));
    }

    g_message("Pool %p: create thread", pool);
    if (const gint rv = pthread_create(&tid->thread, &attr, (void* (*) (void*) ) func, data);
        rv != 0) {
        g_set_error(error,
                    G_THREAD_ERROR,
                    G_THREAD_ERROR_AGAIN,
                    "Error creating thread: %s",
                    g_strerror(rv));
        g_slice_free(CustomRtId, tid);
        tid = nullptr;
    }
    return tid;
}

static void
defaultJoin(GstTaskPool* pool, gpointer id)
{
    if (auto* tid = static_cast<CustomRtId*>(id); tid != nullptr) {
        g_message("Pool %p: joining", pool);
        pthread_join(tid->thread, nullptr);
        g_slice_free(CustomRtId, tid);
    }
}

static void
custom_rt_pool_class_init(CustomRtPoolClass* klass)
{
    auto* customClass = reinterpret_cast<GObjectClass*>(klass);
    customClass->finalize = GST_DEBUG_FUNCPTR(custom_rt_pool_finalize);

    auto* customPoolClass = reinterpret_cast<GstTaskPoolClass*>(klass);
    customPoolClass->prepare = defaultPrepare;
    customPoolClass->cleanup = defaultCleanup;
    customPoolClass->push = defaultPush;
    customPoolClass->join = defaultJoin;
}

static void
custom_rt_pool_init(CustomRtPool* pool)
{
    g_message("Pool %p: init", pool);
}

static void
custom_rt_pool_finalize(GObject* object)
{
    g_message("Pool %p: finalize", object);
    G_OBJECT_CLASS(custom_rt_pool_parent_class)->finalize(object);
}

GstTaskPool*
custom_rt_pool_new()
{
    auto* pool = static_cast<GstTaskPool*>(g_object_new(CUSTOM_TYPE_RT_POOL, nullptr));
    g_message("Pool %p: new", pool);
    return pool;
}
