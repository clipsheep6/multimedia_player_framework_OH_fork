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

#ifndef PLAYER_CODEC_CTRL_H
#define PLAYER_CODEC_CTRL_H

#include <mutex>
#include <string>
#include <list>
#include <unordered_map>
#include <gst/gst.h>
#include <gst/player/player.h>

namespace OHOS {
namespace Media {
class PlayerCodecCtrl {
public:
    using CapsFixErrorNotifier = std::function<void()>;
    PlayerCodecCtrl();
    ~PlayerCodecCtrl();
    void DetectCodecSetup(const std::string &metaStr, GstElement *src, GstElement *videoSink,
        CapsFixErrorNotifier notifier);
    void DetectCodecUnSetup(GstElement *src, GstElement *videoSink);
    void EnhanceSeekPerformance(bool enable);
    int32_t GetHEBCMode() const;
    int32_t HandleCodecBuffers(bool enable);
    void StopFormatChange();

private:
    void SetupCodecCb(const std::string &metaStr, GstElement *src, GstElement *videoSink,
        CapsFixErrorNotifier notifier);
    void HlsSwichSoftAndHardCodec(GstElement *videoSink);
    void SetupCodecBufferNum(const std::string &metaStr, GstElement *src) const;
    static void CapsFixErrorCb(const GstElement *decoder, gpointer userData);
    bool IsFirstCodecSetup() const;
    void DisablePerformanceBySysParam();

    bool isHardwareDec_ = false;
    bool isHEBCMode_ = false;
    struct DecoderElement {
        bool isHardware = false;
        gulong signalId = 0;
    };
    std::unordered_map<GstElement *, DecoderElement> elementMap_;
    std::list<bool> codecTypeList_;
    std::mutex mutex_;
    CapsFixErrorNotifier notifier_;
    bool isEnablePerformanceMode_ = true;
};
} // Media
} // OHOS
#endif // PLAYER_CODEC_CTRL_H