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
#include <iostream>
#include "aw_common.h"
#include "string_ex.h"
#include "media_log.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "screencapturedatatype_ndk_fuzzer.h"

using namespace std;
using namespace OHOS;
using namespace Media;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ScreenCaptureDataTypeNdkFuzzer"};
}

namespace OHOS {
namespace Media {
ScreenCaptureDataTypeNdkFuzzer::ScreenCaptureDataTypeNdkFuzzer()
{
}

ScreenCaptureDataTypeNdkFuzzer::~ScreenCaptureDataTypeNdkFuzzer()
{
}

void SetConfig(OH_AVScreenCaptureConfig &config)
{
    OH_AudioCaptureInfo miccapinfo = {
        .audioSampleRate = 48000,
        .audioChannels = 2,
        .audioSource = OH_SOURCE_DEFAULT
    };

    OH_AudioEncInfo audioEncInfo = {
        .audioBitrate = 48000,
        .audioCodecformat = OH_AudioCodecFormat::OH_AAC_LC
    };

    OH_VideoCaptureInfo videocapinfo = {
        .videoFrameWidth = 720,
        .videoFrameHeight = 1080,
        .videoSource = OH_VIDEO_SOURCE_SURFACE_RGBA
    };

    OH_VideoEncInfo videoEncInfo = {
        .videoCodec = OH_VideoCodecFormat::OH_MPEG4,
        .videoBitrate = 2000000,
        .videoFrameRate = 30
    };

    OH_AudioInfo audioinfo = {
        .micCapInfo = miccapinfo,
        .audioEncInfo = audioEncInfo
    };

    OH_VideoInfo videoinfo = {
        .videoCapInfo = videocapinfo,
        .videoEncInfo = videoEncInfo
    };

    config = {
        .captureMode = OH_CAPTURE_HOME_SCREEN,
        .dataType = OH_ORIGINAL_STREAM,
        .audioInfo = audioinfo,
        .videoInfo = videoinfo,
    };
}

bool ScreenCaptureDataTypeNdkFuzzer::FuzzScreenCaptureDataTypeNdk(uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(OH_DataType)) {
        return false;
    }
    screenCapture = OH_AVScreenCapture_Create();

    OH_AVScreenCaptureConfig config;
    SetConfig(config);
    constexpr int32_t dataTypeList = 4;
    constexpr int32_t dataTypeCaptureFile = 2;
    constexpr uint32_t recorderTime = 3;
    const OH_DataType dataType_[dataTypeList] {
        OH_ORIGINAL_STREAM,
        OH_ENCODED_STREAM,
        OH_CAPTURE_FILE,
        OH_INVAILD
    };
    int32_t datatypesubscript = (static_cast<int32_t>(*data)) % (dataTypeList);
    MEDIA_LOGI("FuzzTest ScreenCaptureDataTypeNdkFuzzer datatypesubscript: %{public}d ", datatypesubscript);

    OH_RecorderInfo recorderInfo;
    const std::string screenCaptureRoot = "/data/test/media/";
    int32_t outputFd = open((screenCaptureRoot + "screen_capture_fuzz_ndk_datatype_file_01.mp4").c_str(),
        O_RDWR | O_CREAT, 0777);
    std::string fileUrl = "fd://" + to_string(outputFd);
    recorderInfo.url = const_cast<char *>(fileUrl.c_str());
    recorderInfo.fileFormat = OH_ContainerFormatType::CFT_MPEG_4;
    
    if (datatypesubscript == dataTypeCaptureFile) {
        config.dataType = dataType_[datatypesubscript];
        config.recorderInfo = recorderInfo;
    } else {
        config.dataType = dataType_[datatypesubscript];
    }

    OH_AVScreenCapture_SetMicrophoneEnabled(screenCapture, true);
    OH_AVScreenCapture_SetCanvasRotation(screenCapture, true);
    OH_AVScreenCaptureCallback callback;
    callback.onError = TestScreenCaptureNdkCallback::OnError;
    callback.onAudioBufferAvailable = TestScreenCaptureNdkCallback::OnAudioBufferAvailable;
    callback.onVideoBufferAvailable = TestScreenCaptureNdkCallback::OnVideoBufferAvailable;
    OH_AVScreenCapture_SetCallback(screenCapture, callback);
    OH_AVScreenCapture_Init(screenCapture, config);
    OH_AVScreenCapture_StartScreenCapture(screenCapture);
    sleep(recorderTime);
    OH_AVScreenCapture_StopScreenCapture(screenCapture);
    OH_AVScreenCapture_Release(screenCapture);
    return true;
}
} // namespace Media

bool FuzzTestScreenCaptureDataTypeNdk(uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return true;
    }

    if (size < sizeof(OH_DataType)) {
        return true;
    }
    ScreenCaptureDataTypeNdkFuzzer testScreenCapture;
    return testScreenCapture.FuzzScreenCaptureDataTypeNdk(data, size);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    MEDIA_LOGI("FuzzTest ScreenCaptureDataTypeNdkFuzzer start");
    MEDIA_LOGI("FuzzTest ScreenCaptureDataTypeNdkFuzzer data: %{public}d ", static_cast<int32_t>(*data));
    /* Run your code on data */
    OHOS::FuzzTestScreenCaptureDataTypeNdk(data, size);
    MEDIA_LOGI("FuzzTest ScreenCaptureDataTypeNdkFuzzer end");
    return 0;
}