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

#ifndef GST_TRACER_MGR_H
#define GST_TRACER_MGR_H

#include <cstdint>
#include <mutex>
#include <gst/gst.h>
#include "nocopyable.h"

namespace OHOS {
namespace Media {
/**
 * @brief Manage the gst tracer instance's create and destory.
 * The multi-instance for media subservice is unsupported.
 */
class GstTracerMgr {
public:
    static GstTracerMgr &Instance()
    {
        static GstTracerMgr instance;
        return instance;
    }

    void SetUp();

private:
    GstTracerMgr() = default;
    ~GstTracerMgr() = default;

    std::mutex mutex_;
    GstTracer *perfTracer_ = nullptr;
};
}
}

#endif