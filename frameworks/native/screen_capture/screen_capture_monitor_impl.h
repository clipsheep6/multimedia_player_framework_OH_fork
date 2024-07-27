/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#ifndef SCREEN_CAPTURE_MONITOR_IMPL_H
#define SCREEN_CAPTURE_MONITOR_IMPL_H

#include <media_dfx.h>
#include "screen_capture_monitor.h"
#include "nocopyable.h"
#include "i_screen_capture_monitor_service.h"

namespace OHOS {
namespace Media {
class ScreenCaptureMonitorImpl : public ScreenCaptureMonitor, public NoCopyable {
public:
    ScreenCaptureMonitorImpl();
    ~ScreenCaptureMonitorImpl();

    int32_t IsScreenCaptureWorking();
    void RegisterScreenCaptureMonitorListener(sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> listener);
    void UnregisterScreenCaptureMonitorListener(sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> listener);
    int32_t Init();
private:
    std::shared_ptr<IScreenCaptureMonitorService> screenCaptureMonitorService_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // SCREEN_CAPTURE_MONITOR_IMPL_H