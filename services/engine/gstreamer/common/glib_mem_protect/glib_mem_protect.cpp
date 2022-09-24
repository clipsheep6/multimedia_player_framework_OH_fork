/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "glib_mem_protect.h"
#include <unistd.h>
#include <thread>
#include <vector>
#include <glib.h>

namespace {
constexpr int32_t DEFAULT_MEM_NUM = 6;
constexpr int32_t DEFAULT_MEM_SIZE = 160;
}

namespace OHOS {
namespace Media {
GlibMemProtect &GlibMemProtect::GetInstance()
{
    static GlibMemProtect glibMemProtect;
    return glibMemProtect;
}

void GlibMemProtect::EngineExit()
{
    std::unique_ptr<std::thread> thread = std::make_unique<std::thread>(&GlibMemProtect::ExitProcess, this);
    thread->detach();
}

void GlibMemProtect::ExitProcess()
{
    std::vector<gpointer> vec;
    for (int i = 0; i < DEFAULT_MEM_NUM; ++i) {
        vec.push_back(g_slice_alloc(DEFAULT_MEM_SIZE));
    }
    for (int i = 0; i < DEFAULT_MEM_NUM; ++i) {
        g_slice_free1(DEFAULT_MEM_SIZE, vec[i]);
    }
    sleep(30);
}
} // namespace Media
} // namespace OHOS