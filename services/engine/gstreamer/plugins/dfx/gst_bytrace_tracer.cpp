/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gst_bytrace_tracer.h"
#include <string>
#include <unordered_map>
#include "bytrace.h"

namespace OHOS {
namespace Media {
class BytraceTracer {
public:
    static BytraceTracer &Instance()
    {
        static BytraceTracer instance;
        return instance;
    }

    inline void StartTrace(const std::string &name)
    {
        ::StartTrace(BYTRACE_TAG_ZMEDIA, name);
    }

    inline void FinishTrace()
    {
        ::FinishTrace(BYTRACE_TAG_ZMEDIA);
    }

    inline void StartTraceAsync(gpointer key, const std::string &name)
    {
        ::StartAsyncTrace(BYTRACE_TAG_ZMEDIA, name, asyncTaskId_);
        asyncTask_.emplace(key, asyncTaskId_);
        asyncTaskId_ += 1; // allow flip
    }

    inline void FinishTraceAsync(gpointer key, const std::string &name)
    {
        auto iter = asyncTask_.find(key);
        if (iter == asyncTask_.end()) {
            return;
        }
        ::FinishAsyncTrace(BYTRACE_TAG_ZMEDIA, name, iter->second);
    }

private:
    BytraceTracer() = default;
    ~BytraceTracer() = default;

    int32_t asyncTaskId_ = 0;
    std::unordered_map<void *, int32_t> asyncTask_;
};
}
}

GST_DEBUG_CATEGORY_STATIC (gst_bytrace_debug);
#define GST_CAT_DEFAULT gst_bytrace_debug

#define _do_init \
    GST_DEBUG_CATEGORY_INIT (gst_bytrace_debug, "bytracetracer", 0, "bytrace tracer");
#define gst_bytrace_tracer_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstBytraceTracer, gst_bytrace_tracer, GST_TYPE_TRACER,
    _do_init);

static constexpr std::string::size_type DEFAULT_TITLE_LEN = 50;

static void do_post_message_pre(GstTracer *self, guint64 ts, GstElement *elem, GstMessage *msg)
{
    (void)self;
    (void)ts;

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ASYNC_START:
            OHOS::Media::BytraceTracer::Instance().StartTraceAsync(elem, "async state change");
            break;
        case GST_MESSAGE_ASYNC_DONE:
            OHOS::Media::BytraceTracer::Instance().FinishTraceAsync(elem, "async state change");
            break;
        default:
            break;
    }
}

static void do_element_change_state_pre(GstTracer *self, guint64 ts, GstElement *elem, GstStateChange change)
{
    (void)self;
    (void)ts;

    if (elem == nullptr || GST_ELEMENT_NAME(elem) == nullptr) {
        return;
    }

    GstState curr = GST_STATE_TRANSITION_CURRENT(change);
    GstState next = GST_STATE_TRANSITION_NEXT(change);

    std::string title;
    title.reserve(DEFAULT_TITLE_LEN);
    title = GST_ELEMENT_NAME(elem);

    title += "[";
    title += gst_element_state_get_name(curr);
    title += "->";
    title += gst_element_state_get_name(next);
    title += "]";

    OHOS::Media::BytraceTracer::Instance().StartTrace(title);
}

static void do_element_change_state_post(GstTracer *self, guint64 ts, GstElement *elem,
    GstStateChange change, GstStateChangeReturn res)
{
    (void)self;
    (void)ts;
    (void)elem;
    (void)change;
    (void)res;

    OHOS::Media::BytraceTracer::Instance().FinishTrace();
}

static void gst_bytrace_tracer_class_init(GstBytraceTracerClass *klass)
{
    (void)klass;
}

static void gst_bytrace_tracer_init(GstBytraceTracer *self)
{
    GstTracer *tracer = GST_TRACER(self);

    gst_tracing_register_hook (tracer, "element-post-message-pre",
        G_CALLBACK(do_post_message_pre));
    gst_tracing_register_hook (tracer, "element-change-state-pre",
        G_CALLBACK(do_element_change_state_pre));
    gst_tracing_register_hook (tracer, "element-change-state-post",
        G_CALLBACK(do_element_change_state_post));
}
