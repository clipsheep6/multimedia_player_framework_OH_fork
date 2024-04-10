/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#ifndef HI_TRANSCODER_IMPL_H
#define HI_TRANSCODER_IMPL_H

#include "i_transcoder_engine.h"
#include "transcoder_param.h"
#include "common/log.h"
#include "filter/filter_factory.h"
#include "osal/task/condition_variable.h"
#include "filter/filter.h"
#include "media_errors.h"
#include "osal/task/task.h"
#include "pipeline/pipeline.h"

namespace OHOS {
namespace Media {

class HiTransCoderImpl : public ITransCoderEngine {
public:
    HiTransCoderImpl(int32_t appUid, int32_t appPid, uint32_t appTokenId, uint64_t appFullTokenId);
    ~HiTransCoderImpl();
    int32_t Init();
    int32_t SetOutputFormat(OutputFormatType format);
    int32_t SetObs(const std::weak_ptr<ITransCoderEngineObs> &obs);
    int32_t Configure(const TransCoderParam &recParam);
    int32_t Prepare();
    int32_t Start();
    int32_t Pause();
    int32_t Resume();
    int32_t Cancel();
    void OnEvent(const Event &event);
    void OnCallback(std::shared_ptr<Pipeline::Filter> filter, const Pipeline::FilterCallBackCommand cmd,
        Pipeline::StreamType outType);

private:
    int32_t appUid_{0};
    int32_t appPid_{0};
    int32_t appTokenId_{0};
    int64_t appFullTokenId_{0};

    std::shared_ptr<Pipeline::Pipeline> pipeline_;
};
} // namespace MEDIA
} // namespace OHOS
#endif // HI_RECORDER_IMPL_H