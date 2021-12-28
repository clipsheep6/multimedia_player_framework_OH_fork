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

#include "gst_tracer_mgr.h"
#include <string>
#include "media_log.h"
#include "media_errors.h"

#define ENABLE_GST_TRACE

namespace {
    [[maybe_unused]] constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstTracerMgr"};
}

namespace OHOS {
namespace Media {
#ifdef ENABLE_GST_TRACE
static GstTracer *MakeTracer(const std::string_view &factoryName, const std::string_view &name)
{
    CHECK_AND_RETURN_RET(!factoryName.empty(), nullptr);

    MEDIA_LOGD("gsttracerfactory: make \"%{public}s\" \"%{public}s\"",
        factoryName.data(), GST_STR_NULL(name.data()));

    GstPluginFeature *feature = gst_registry_find_feature(
        gst_registry_get(), factoryName.data(), GST_TYPE_TRACER_FACTORY);
    if (feature == nullptr) {
        MEDIA_LOGW("no such tracer factory \"%{public}s\"", factoryName.data());
        return nullptr;
    }

    GstTracerFactory *factory = GST_TRACER_FACTORY(gst_plugin_feature_load(feature));
    if (factory == nullptr) {
        MEDIA_LOGW("loading plugin containing feature %{public}s returned NULL!", factoryName.data());
        gst_object_unref(feature);
        return nullptr;
    }

    gst_object_unref(feature);

    GType type = gst_tracer_factory_get_tracer_type(factory);
    if (type == 0) {
        MEDIA_LOGW("factory has no type");
        gst_object_unref(factory);
        return nullptr;
    }

    GstTracer *tracer = GST_TRACER_CAST(g_object_new(type, "name", name.data(), nullptr));
    if (tracer == nullptr) {
        MEDIA_LOGW("couldn't create instance !");
        gst_object_unref(factory);
        return nullptr;
    }

    gst_object_unref(factory);
    return tracer;
}

void GstTracerMgr::SetUp()
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (perfTracer_ != nullptr) { // will not released
        perfTracer_ = MakeTracer("bytrace_tracer", "perf_treacer");
    }
}
#else
void GstTracerMgr::SetUpPerfTracer()
{
}

void GstTracerMgr::SetUp()
{
}

void GstTracerMgr::TearDown()
{
}
#endif
}
}
