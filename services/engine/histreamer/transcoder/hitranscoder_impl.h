/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "demuxer_filter.h"
#include "audio_decoder_filter.h"
#include "surface_decoder_filter.h"
#include "audio_encoder_filter.h"
#include "surface_encoder_filter.h"
#include "muxer_filter.h"
#include "hitranscoder_callback_looper.h"

namespace OHOS {
namespace Media {

class HiTransCoderImpl : public ITransCoderEngine {
public:
    HiTransCoderImpl(int32_t appUid, int32_t appPid, uint32_t appTokenId, uint64_t appFullTokenId);
    ~HiTransCoderImpl();
    int32_t Init();
    int32_t SetInputFile(const std::string &url);
    int32_t SetOutputFile(const int32_t fd);
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
    int32_t GetCurrentTime(int32_t& currentPositionMs);
    int32_t GetDuration(int32_t& durationMs);

private:
    int32_t GetRealPath(const std::string &url, std::string &realUrlPath) const;
    void ConfigureVideoEncoderFormat(const TransCoderParam &transCoderParam);
    Status LinkAudioDecoderFilter(const std::shared_ptr<Pipeline::Filter>& preFilter, Pipeline::StreamType type);
    Status LinkAudioEncoderFilter(const std::shared_ptr<Pipeline::Filter>& preFilter, Pipeline::StreamType type);
    Status LinkVideoDecoderFilter(const std::shared_ptr<Pipeline::Filter>& preFilter, Pipeline::StreamType type);
    Status LinkVideoEncoderFilter(const std::shared_ptr<Pipeline::Filter>& preFilter, Pipeline::StreamType type);
    Status LinkMuxerFilter(const std::shared_ptr<Pipeline::Filter>& preFilter, Pipeline::StreamType type);
    void CancelTransCoder();

    int32_t appUid_{0};
    int32_t appPid_{0};
    int32_t appTokenId_{0};
    int64_t appFullTokenId_{0};

    std::shared_ptr<Pipeline::Pipeline> pipeline_;
    std::shared_ptr<Pipeline::DemuxerFilter> demuxerFilter_;
    std::shared_ptr<Pipeline::AudioDecoderFilter> audioDecoderFilter_;
    std::shared_ptr<Pipeline::SurfaceDecoderFilter> videoDecoderFilter_;
    std::shared_ptr<Pipeline::AudioEncoderFilter> audioEncoderFilter_;
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> videoEncoderFilter_;
    std::shared_ptr<Pipeline::MuxerFilter> muxerFilter_;

    std::shared_ptr<Pipeline::EventReceiver> transCoderEventReceiver_;
    std::shared_ptr<Pipeline::FilterCallback> transCoderFilterCallback_;

    std::shared_ptr<Task> cancelTask_{nullptr};

    std::shared_ptr<Meta> audioEncFormat_ = std::make_shared<Meta>();
    std::shared_ptr<Meta> videoEncFormat_ = std::make_shared<Meta>();
    std::shared_ptr<Meta> muxerFormat_ = std::make_shared<Meta>();

    std::weak_ptr<ITransCoderEngineObs> obs_{};
    std::shared_ptr<HiTransCoderCallbackLooper> callbackLooper_;
    OutputFormatType outputFormatType_{OutputFormatType::FORMAT_BUTT};
    int32_t fd_ = -1;
    std::string inputFile_;

    std::string transCoderId_;
    int32_t inputVideoWidth_ = 0;
    int32_t inputVideoHeight_ = 0;
    bool isNeedVideoResizeFilter_ = false;

    std::atomic<int32_t> durationMs_{-1};
};
} // namespace MEDIA
} // namespace OHOS
#endif // HI_RECORDER_IMPL_H