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

#ifndef I_PLAYBIN_CTRLER_H
#define I_PLAYBIN_CTRLER_H

#include <memory>
#include <string>
#include <unordered_map>
#include "nocopyable.h"
#include "playbin_msg_define.h"
#include "playbin_sink_provider.h"
#include "gst_appsrc_engine.h"
#include "meta/format.h"
#include "foundation/multimedia/drm_framework/services/drm_service/ipc/i_keysession_service.h"

namespace OHOS {
namespace Media {
class IPlayBinCtrler {
public:
    enum class PlayBinKind : uint8_t {
        PLAYBIN2,
    };

    enum PlayBinRenderMode : uint8_t {
        DEFAULT_RENDER = 0,
        NATIVE_STREAM = 1 << 0,
        DISABLE_TEXT = 1 << 1,
    };

    enum PlayBinSeekMode : uint8_t {
        NEXT_SYNC,
        PREV_SYNC,
        CLOSET_SYNC,
        CLOSET,
    };

    struct PlayBinCreateParam {
        PlayBinRenderMode renderMode;
        PlayBinMsgNotifier notifier;
        std::shared_ptr<PlayBinSinkProvider> sinkProvider;
    };

    virtual ~IPlayBinCtrler() = default;

    static std::shared_ptr<IPlayBinCtrler> Create(PlayBinKind kind, const PlayBinCreateParam &createParam);

    virtual int32_t SetSource(const std::string &url) = 0;
    virtual int32_t SetSource(const std::shared_ptr<GstAppsrcEngine> &appsrcWrap) = 0;
    virtual int32_t AddSubSource(const std::string &url) = 0;
    virtual int32_t PrepareAsync() = 0; // async
    virtual int32_t Play() = 0; // async
    virtual int32_t Pause() = 0; // async
    virtual int32_t Seek(int64_t timeUs, int32_t seekOption) = 0; // async
    virtual int32_t Stop(bool needWait) = 0; // async
    virtual int32_t SetRate(double rate) = 0;
    virtual int32_t SetLoop(bool loop) = 0;
    virtual void SetVolume(const float &leftVolume, const float &rightVolume) = 0;
    virtual int32_t SelectBitRate(uint32_t bitRate) = 0;
    virtual int32_t SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySessionProxy,
        bool svp) = 0;
    virtual void SetAudioInterruptMode(const int32_t interruptMode) = 0;
    virtual int32_t SetAudioRendererInfo(const uint32_t rendererInfo, const int32_t rendererFlag) = 0;
    virtual int32_t SetAudioEffectMode(const int32_t effectMode) = 0;
    using ElemSetupListener = std::function<void(GstElement &elem)>;
    using AutoPlugSortListener = std::function<GValueArray *(GValueArray &factories)>;
    virtual void SetElemSetupListener(ElemSetupListener listener) = 0;
    virtual void SetElemUnSetupListener(ElemSetupListener listener) = 0;
    virtual void SetAutoPlugSortListener(AutoPlugSortListener listener) = 0;
    virtual void RemoveGstPlaySinkVideoConvertPlugin() = 0;
    virtual int64_t QueryPosition() = 0;
    virtual void SetNotifier(PlayBinMsgNotifier notifier) = 0;
    virtual void SetAutoSelectBitrate(bool enable) = 0;
    virtual int32_t SelectTrack(int32_t index) = 0;
    virtual int32_t DeselectTrack(int32_t index) = 0;
    virtual int32_t GetCurrentTrack(int32_t trackType, int32_t &index) = 0;
    virtual int32_t GetVideoTrackInfo(std::vector<Format> &videoTrack) = 0;
    virtual int32_t GetAudioTrackInfo(std::vector<Format> &audioTrack) = 0;
    virtual int32_t GetSubtitleTrackInfo(std::vector<Format> &subtitleTrack) = 0;
};
} // namespace Media
} // namespace OHOS
#endif // I_PLAYBIN_CTRLER_H