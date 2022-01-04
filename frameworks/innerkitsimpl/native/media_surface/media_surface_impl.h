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

#ifndef MEDIA_SURFACE_IMPL_H
#define MEDIA_SURFACE_IMPL_H

#include <map>
#include <string>
#include "media_surface.h"
#include "window_manager.h"

namespace OHOS {
namespace Media {
class MediaSurfaceImpl : public MediaSurface {
public:
    MediaSurfaceImpl();
    ~MediaSurfaceImpl();
    DISALLOW_COPY_AND_MOVE(MediaSurfaceImpl);
    virtual std::string GetSurfaceId(const sptr<Surface> &surface) override;
    virtual sptr<Surface> GetSurface() override;
    virtual sptr<Surface> GetSurface(const std::string &id) override;

private:
    bool StrToUint64(const std::string &str, uint64_t &value);
    std::mutex mutex_;
    sptr<Window> mwindow_ = nullptr;
    std::map<std::string, wptr<Surface>> surfaceMap_;
};
}
}
#endif // MEDIA_SURFACE_IMPL_H