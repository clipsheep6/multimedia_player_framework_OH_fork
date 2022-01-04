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

#include "media_surface_impl.h"
#include "media_log.h"
#include "media_errors.h"
#include "display_type.h"
#include "surface_utils.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaSurface"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<MediaSurface> MediaSurfaceFactory::CreateMediaSurface()
{
    static std::shared_ptr<MediaSurfaceImpl> instance = std::make_shared<MediaSurfaceImpl>();
    return instance;
}

MediaSurfaceImpl::MediaSurfaceImpl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaSurfaceImpl::~MediaSurfaceImpl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

std::string MediaSurfaceImpl::GetSurfaceId(const sptr<Surface> &surface)
{
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t uniqueId = surface->GetUniqueId;
    std::string uniqueIdStr = std::to_string(uniqueId);
    MEDIA_LOGE("Get surface id, surfaceId:%{public}s", uniqueIdStr.c_str());
    return uniqueIdStr;
}

bool MediaSurfaceImpl::StrToUint64(const std::string &str, int64_t &value)
{
    if (str.empty() || (!isdigit(str.front()))) {
        return false;
    }

    char *end = nullptr;
    errno = 0;
    auto addr = str.data();
    auto result = std::strtoull(addr, &end, 10); /* 10 means decimal */
    if ((end == addr) || (end[0] != '\0') || (errno == ERANGE)) {
        MEDIA_LOGE("failed convert string to uint64");
        return false;
    }

    value = result;
    return true;
}

sptr<Surface> MediaSurfaceImpl::GetSurface(const std::string &id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t surfaceId = 0;
    if (!StrToUint64(id, surfaceId)) {
        MEDIA_LOGE("failed to get surface id");
        return nullptr;
    }

    MEDIA_LOGD("get surface, surfaceId:%{public}s, id = (%{public}" PRIu64 ")", id.c_str(), surfaceId);
    sptr<Surface> surface = SurfaceUtils::GetInstance()->GetSurface(surfaceId);
    return nullptr;
}

sptr<Surface> MediaSurfaceImpl::GetSurface()
{
    std::lock_guard<std::mutex> lock(mutex_);
    sptr<WindowManager> wmi = WindowManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(wmi != nullptr, nullptr, "WindowManager is nullptr!");

    wmi->Init();
    sptr<WindowOption> option = WindowOption::Get();
    CHECK_AND_RETURN_RET_LOG(option != nullptr, nullptr, "WindowOption is nullptr!");
    const int32_t height = 360;
    const int32_t width = 640;
    (void)option->SetWidth(width);
    (void)option->SetHeight(height);
    (void)option->SetX(0);
    (void)option->SetY(0);
    (void)option->SetWindowType(WINDOW_TYPE_NORMAL);
    (void)wmi->CreateWindow(mwindow_, option);
    CHECK_AND_RETURN_RET_LOG(mwindow_ != nullptr, nullptr, "mwindow_ is nullptr!");

    sptr<Surface> producerSurface = mwindow_->GetSurface();
    CHECK_AND_RETURN_RET_LOG(producerSurface != nullptr, nullptr, "producerSurface is nullptr!");

    SurfaceError error = SurfaceUtils::GetInstance()->Add(producerSurface->GetUniqueId(), producerSurface);
    if (error != SURFACE_ERROR_OK) {
        MEDIA_LOGE("add producerSurface error");
        return nullptr;
    }

    const std::string format = "SURFACE_FORMAT";
    (void)producerSurface->SetUserData(format, std::to_string(static_cast<int>(PIXEL_FMT_RGBA_8888)));
    return producerSurface;
}
}
}