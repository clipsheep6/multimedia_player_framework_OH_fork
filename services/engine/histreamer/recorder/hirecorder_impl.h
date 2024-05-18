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

#ifndef HI_RECORDER_IMPL_H
#define HI_RECORDER_IMPL_H

#include "i_recorder_engine.h"
#include "recorder_utils.h"
#include "recorder_param.h"
#include "common/log.h"
#include "filter/filter_factory.h"
#include "osal/task/condition_variable.h"
#include "filter/filter.h"
#include "audio_capture_filter.h"
#include "audio_data_source_filter.h"
#include "audio_encoder_filter.h"
#include "media_errors.h"
#include "muxer_filter.h"
#include "osal/task/task.h"
#include "pipeline/pipeline.h"
#include "surface_encoder_filter.h"
#include "video_capture_filter.h"
#include "codec_capability_adapter.h"

namespace OHOS {
namespace Media {

enum class StateId {
    INIT,
    RECORDING_SETTING,
    READY,
    PAUSE,
    RECORDING,
    ERROR,
    BUTT,
};

const std::string AUIDO_TYPE = "audio";
const std::string VIDEO_TYPE = "video";

class HiRecorderImpl : public IRecorderEngine {
public:
    HiRecorderImpl(int32_t appUid, int32_t appPid, uint32_t appTokenId, uint64_t appFullTokenId);
    ~HiRecorderImpl();
    int32_t Init();
    int32_t SetVideoSource(VideoSourceType source, int32_t &sourceId);
    int32_t SetAudioSource(AudioSourceType source, int32_t &sourceId);
    int32_t SetAudioDataSource(const std::shared_ptr<IAudioDataSource>& audioSource, int32_t& sourceId);
    int32_t SetOutputFormat(OutputFormatType format);
    int32_t SetObs(const std::weak_ptr<IRecorderEngineObs> &obs);
    int32_t Configure(int32_t sourceId, const RecorderParam &recParam);
    sptr<Surface> GetSurface(int32_t sourceId);
    int32_t Prepare();
    int32_t Start();
    int32_t Pause();
    int32_t Resume();
    int32_t Stop(bool isDrainAll);
    int32_t Reset();
    int32_t SetParameter(int32_t sourceId, const RecorderParam &recParam);
    void OnEvent(const Event &event);
    Status OnCallback(std::shared_ptr<Pipeline::Filter> filter, const Pipeline::FilterCallBackCommand cmd,
        Pipeline::StreamType outType);
    void OnAudioCaptureChange(const AudioStandard::AudioCapturerChangeInfo &capturerChangeInfo);
    int32_t GetCurrentCapturerChangeInfo(AudioRecorderChangeInfo &changeInfo);
    int32_t GetAvailableEncoder(std::vector<EncoderCapabilityData> &encoderInfo);
    int32_t GetMaxAmplitude();
    void SetCallingInfo(const std::string &bundleName, uint64_t instanceId);

private:
    void ConfigureAudioCapture();
    void ConfigureAudio(const RecorderParam &recParam);
    void ConfigureVideo(const RecorderParam &recParam);
    void ConfigureMuxer(const RecorderParam &recParam);
    bool CheckParamType(int32_t sourceId, const RecorderParam &recParam);
    void OnStateChanged(StateId state);
    void ConfigureVideoEncoderFormat(const RecorderParam &recParam);
    void ConfigureVideoEnableTemporalScale(const RecorderParam &recParam);
    bool CheckAudioSourceType(AudioSourceType sourceType);
    void ConfigureRotation(const RecorderParam &recParam);
    EncoderCapabilityData ConvertAudioEncoderInfo(MediaAVCodec::CapabilityData *capabilityData);
    EncoderCapabilityData ConvertVideoEncoderInfo(MediaAVCodec::CapabilityData *capabilityData);
    std::vector<EncoderCapabilityData> ConvertEncoderInfo(std::vector<MediaAVCodec::CapabilityData*> &capData);

    AudioRecorderChangeInfo ConvertCapturerChangeInfo(const AudioStandard::AudioCapturerChangeInfo &capturerChangeInfo);
    std::atomic<uint32_t> audioCount_{0};
    std::atomic<uint32_t> videoCount_{0};
    std::atomic<uint32_t> audioSourceId_{0};
    std::atomic<uint32_t> videoSourceId_{0};
    int32_t appUid_{0};
    int32_t appPid_{0};
    int32_t appTokenId_{0};
    int64_t appFullTokenId_{0};

    std::shared_ptr<Pipeline::Pipeline> pipeline_;
    std::shared_ptr<Pipeline::AudioCaptureFilter> audioCaptureFilter_;
    std::shared_ptr<Pipeline::AudioDataSourceFilter> audioDataSourceFilter_;
    std::shared_ptr<Pipeline::AudioEncoderFilter> audioEncoderFilter_;
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> videoEncoderFilter_;
    std::shared_ptr<Pipeline::VideoCaptureFilter> videoCaptureFilter_;
    std::shared_ptr<Pipeline::MuxerFilter> muxerFilter_;
    std::shared_ptr<Pipeline::CodecCapabilityAdapter> codecCapabilityAdapter_;

    std::shared_ptr<Pipeline::EventReceiver> recorderEventReceiver_;
    std::shared_ptr<Pipeline::FilterCallback> recorderCallback_;

    std::shared_ptr<Meta> audioEncFormat_ = std::make_shared<Meta>();
    std::shared_ptr<Meta> videoEncFormat_ = std::make_shared<Meta>();
    std::shared_ptr<Meta> muxerFormat_ = std::make_shared<Meta>();
    std::shared_ptr<Meta> userMeta_ = std::make_shared<Meta>();
    std::atomic<StateId> curState_;

    std::shared_ptr<AudioStandard::AudioCapturerInfoChangeCallback> CapturerInfoChangeCallback_;
    std::weak_ptr<IRecorderEngineObs> obs_{};
    OutputFormatType outputFormatType_{OutputFormatType::FORMAT_BUTT};
    int32_t fd_ = -1;
    int64_t maxDuration_ = 0;
    int64_t maxSize_ = 0;

    bool videoSourceIsYuv_ = false;
    bool videoSourceIsRGBA_ = false;

    Mutex stateMutex_ {};
    ConditionVariable cond_ {};

    sptr<Surface> producerSurface_{nullptr};
    sptr<Surface> consumerSurface_{nullptr};

    static constexpr uint32_t ENCODE_USAGE = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE |
        BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_VIDEO_ENCODER;
    std::string recorderId_;
    std::string bundleName_;
    uint64_t instanceId_ = 0;
};
} // namespace MEDIA
} // namespace OHOS
#endif // HI_RECORDER_IMPL_H