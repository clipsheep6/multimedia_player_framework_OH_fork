/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <cmath>
#include <cstdlib>
#include <iostream>
#include "aw_common.h"
#include "string_ex.h"
#include "media_log.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "screen_capture.h"
#include "screencaptureaudiocodecformatfile_fuzzer.h"

using namespace std;
using namespace OHOS;
using namespace Media;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ScreenCaptureAudioCodecFormatFileFuzzer"};
}

namespace OHOS {
namespace Media {
ScreenCaptureAudioCodecFormatFileFuzzer::ScreenCaptureAudioCodecFormatFileFuzzer()
{
}

ScreenCaptureAudioCodecFormatFileFuzzer::~ScreenCaptureAudioCodecFormatFileFuzzer()
{
}

void SetConfig(AVScreenCaptureConfig &config)
{
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 48000,
        .audioChannels = 2,
        .audioSource = ALL_PLAYBACK
    };

    AudioEncInfo audioEncInfo = {
        .audioBitrate = 48000,
        .audioCodecformat = AudioCodecFormat::AAC_LC
    };

    VideoCaptureInfo videoCapInfo = {
        .videoFrameWidth = 720,
        .videoFrameHeight = 1080,
        .videoSource = VIDEO_SOURCE_SURFACE_RGBA
    };

    VideoEncInfo videoEncInfo = {
        .videoCodec = VideoCodecFormat::MPEG4,
        .videoBitrate = 2000000,
        .videoFrameRate = 30
    };

    AudioInfo audioInfo = {
        .innerCapInfo = innerCapInfo,
        .audioEncInfo = audioEncInfo
    };

    VideoInfo videoInfo = {
        .videoCapInfo = videoCapInfo,
        .videoEncInfo = videoEncInfo
    };

    config = {
        .captureMode = CAPTURE_HOME_SCREEN,
        .dataType = CAPTURE_FILE,
        .audioInfo = audioInfo,
        .videoInfo = videoInfo,
    };
}

bool ScreenCaptureAudioCodecFormatFileFuzzer::FuzzScreenCaptureAudioCodecFormatFile(uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(int32_t)) {
        return false;
    }
    bool retFlags = TestScreenCapture::CreateScreenCapture();
    RETURN_IF(retFlags, false);

    AVScreenCaptureConfig config;
    SetConfig(config);
    constexpr int32_t audioCodecformatList = 3;
    constexpr uint32_t recorderTime = 3;
    const AudioCodecFormat audioCodecformat[audioCodecformatList] {
        AUDIO_DEFAULT,
        AAC_LC,
        AUDIO_CODEC_FORMAT_BUTT
    };
    int32_t randomNum = abs((*reinterpret_cast<int32_t *>(data)) % (audioCodecformatList));
    MEDIA_LOGI("FuzzTest ScreenCaptureAudioBitrateFileNdkFuzzer randomNum: %{public}d ", randomNum);

    config.audioInfo.audioEncInfo.audioCodecformat = audioCodecformat[randomNum];

    RecorderInfo recorderInfo;
    const std::string screenCaptureRoot = "/data/test/media/";
    int32_t outputFd = open((screenCaptureRoot + "screen_capture_fuzz_audiocodecformat_file_01.mp4").c_str(),
        O_RDWR | O_CREAT, 0777);
    recorderInfo.url = "fd://" + to_string(outputFd);
    recorderInfo.fileFormat = "mp4";
    config.recorderInfo = recorderInfo;

    TestScreenCapture::Init(config);
    TestScreenCapture::StartScreenCapture();
    sleep(recorderTime);
    TestScreenCapture::StopScreenCapture();
    TestScreenCapture::Release();
    return true;
}
} // namespace Media

bool FuzzTestScreenCaptureAudioCodecFormatFile(uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return true;
    }

    if (size < sizeof(int32_t)) {
        return true;
    }
    ScreenCaptureAudioCodecFormatFileFuzzer testScreenCapture;
    return testScreenCapture.FuzzScreenCaptureAudioCodecFormatFile(data, size);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    MEDIA_LOGI("FuzzTest ScreenCaptureAudioCodecFormatFileFuzzer start");
    MEDIA_LOGI("FuzzTest ScreenCaptureAudioCodecFormatFileFuzzer data: %{public}d ", *data);
    /* Run your code on data */
    OHOS::FuzzTestScreenCaptureAudioCodecFormatFile(data, size);
    MEDIA_LOGI("FuzzTest ScreenCaptureAudioCodecFormatFileFuzzer end");
    return 0;
}