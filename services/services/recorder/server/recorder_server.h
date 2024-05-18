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

#ifndef RECORDER_SERVICE_SERVER_H
#define RECORDER_SERVICE_SERVER_H

#include <chrono>

#include "i_recorder_service.h"
#include "i_recorder_engine.h"
#include "nocopyable.h"
#include "task_queue.h"
#include "watchdog.h"
#include "meta/meta.h"
#ifdef SUPPORT_POWER_MANAGER
#include "shutdown/sync_shutdown_callback_stub.h"
#include "shutdown/shutdown_client.h"
#endif

namespace OHOS {
namespace Media {
enum class RecorderWatchDogStatus : int32_t {
    WATCHDOG_WATCHING = 0,
    WATCHDOG_PAUSE,
};
#ifdef SUPPORT_POWER_MANAGER
class SaveDocumentSyncCallback : public PowerMgr::SyncShutdownCallbackStub {
public:
    SaveDocumentSyncCallback() {};
    virtual ~SaveDocumentSyncCallback() {};
    void OnSyncShutdown() override;
    bool isShutdown = false;

private:
    const int32_t intervalTime = 500000; // 500 ms
};
#endif
class RecorderServer : public IRecorderService, public IRecorderEngineObs, public NoCopyable {
public:
    static std::shared_ptr<IRecorderService> Create();
    RecorderServer();
    ~RecorderServer();

    enum RecStatus {
        REC_INITIALIZED = 0,
        REC_CONFIGURED,
        REC_PREPARED,
        REC_RECORDING,
        REC_PAUSED,
        REC_ERROR,
    };

    enum HdrType : int8_t {
        HDR_TYPE_NONE,
        HDR_TYPE_VIVID,
    };

    // IRecorderService override
    int32_t SetVideoSource(VideoSourceType source, int32_t &sourceId) override;
    int32_t SetVideoEncoder(int32_t sourceId, VideoCodecFormat encoder) override;
    int32_t SetVideoSize(int32_t sourceId, int32_t width, int32_t height) override;
    int32_t SetVideoFrameRate(int32_t sourceId, int32_t frameRate) override;
    int32_t SetVideoEncodingBitRate(int32_t sourceId, int32_t rate) override;
    int32_t SetVideoIsHdr(int32_t sourceId, bool isHdr) override;
    int32_t SetVideoEnableTemporalScale(int32_t sourceId, bool enableTemporalScale) override;
    int32_t SetCaptureRate(int32_t sourceId, double fps) override;
    sptr<OHOS::Surface> GetSurface(int32_t sourceId) override;
    int32_t SetAudioSource(AudioSourceType source, int32_t &sourceId) override;
    int32_t SetAudioDataSource(const std::shared_ptr<IAudioDataSource>& audioSource, int32_t& sourceId) override;
    int32_t SetAudioEncoder(int32_t sourceId, AudioCodecFormat encoder) override;
    int32_t SetAudioSampleRate(int32_t sourceId, int32_t rate) override;
    int32_t SetAudioChannels(int32_t sourceId, int32_t num) override;
    int32_t SetAudioEncodingBitRate(int32_t sourceId, int32_t bitRate) override;
    int32_t SetDataSource(DataSourceType dataType, int32_t &sourceId) override;
    int32_t SetUserCustomInfo(Meta &userCustomInfo) override;
    int32_t SetGenre(std::string &genre) override;
    int32_t SetMaxDuration(int32_t duration) override;
    int32_t SetOutputFormat(OutputFormatType format) override;
    int32_t SetOutputFile(int32_t fd) override;
    int32_t SetNextOutputFile(int32_t fd) override;
    int32_t SetMaxFileSize(int64_t size) override;
    void SetLocation(float latitude, float longitude) override;
    void SetOrientationHint(int32_t rotation) override;
    int32_t SetRecorderCallback(const std::shared_ptr<RecorderCallback> &callback) override;
    int32_t Prepare() override;
    int32_t Start() override;
    int32_t Pause() override;
    int32_t Resume() override;
    int32_t Stop(bool block) override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t SetFileSplitDuration(FileSplitType type, int64_t timestamp, uint32_t duration) override;
    int32_t SetParameter(int32_t sourceId, const Format &format) override;
    int32_t DumpInfo(int32_t fd);
    int32_t GetAVRecorderConfig(ConfigMap &configMap) override;
    int32_t GetLocation(Location &location) override;
    int32_t GetCurrentCapturerChangeInfo(AudioRecorderChangeInfo &changeInfo) override;
    int32_t GetAvailableEncoder(std::vector<EncoderCapabilityData> &encoderInfo) override;
    int32_t GetMaxAmplitude() override;

    // IRecorderEngineObs override
    void OnError(ErrorType errorType, int32_t errorCode) override;
    void OnInfo(InfoType type, int32_t extra) override;
    void OnAudioCaptureChange(const AudioRecorderChangeInfo &audioRecorderChangeInfo) override;

    void SetMetaDataReport();
    int64_t GetCurrentMillisecond();
    void SetErrorInfo(int32_t errCode, std::string &errMsg);
    const std::string& GetVideoMime(VideoCodecFormat encoder);
    const std::string& GetAudioMime(AudioCodecFormat encoder);

    /* used for DFX events */
    uint64_t instanceId_ = 0;
    std::string bundleName_;
private:
    int32_t Init();
    const std::string &GetStatusDescription(OHOS::Media::RecorderServer::RecStatus status);

    std::unique_ptr<IRecorderEngine> recorderEngine_ = nullptr;
    std::shared_ptr<RecorderCallback> recorderCb_ = nullptr;
    RecStatus status_ = REC_INITIALIZED;
    std::mutex mutex_;
    std::mutex cbMutex_;
    TaskQueue taskQue_;
    struct ConfigInfo {
        VideoSourceType videoSource;
        AudioSourceType audioSource;
        VideoCodecFormat videoCodec;
        AudioCodecFormat audioCodec;
        int32_t width;
        int32_t height;
        int32_t frameRate;
        int32_t bitRate;
        bool isHdr = false;
        bool enableTemporalScale;
        double captureRate;
        int32_t audioSampleRate;
        int32_t audioChannel;
        int32_t audioBitRate;
        int32_t maxDuration;
        OutputFormatType format;
        int64_t maxFileSize;
        float latitude;
        float longitude;
        int32_t rotation;
        int32_t url;
        Meta customInfo;
        std::string genre;
        bool withVideo = false;
        bool withAudio = false;
        bool withLocation = false;
    } config_;
    std::string lastErrMsg_;

    std::atomic<bool> watchdogPause_ = false;
    struct StatisticalEventInfo {
        int32_t errCode;
        std::string errMsg;
        int32_t recordDuration = -1;
        std::string containerMime;
        std::string videoResolution;
        int8_t hdrType = HdrType::HDR_TYPE_NONE;
        int32_t startLatency = -1;
    } statisticalEventInfo_;
    int64_t startTime_ = 0;
#ifdef SUPPORT_POWER_MANAGER
    sptr<SaveDocumentSyncCallback> syncCallback_ = nullptr;
    PowerMgr::ShutdownClient &shutdownClient_ = PowerMgr::ShutdownClient::GetInstance();
#endif
};
} // namespace Media
} // namespace OHOS
#endif // RECORDER_SERVICE_SERVER_H
