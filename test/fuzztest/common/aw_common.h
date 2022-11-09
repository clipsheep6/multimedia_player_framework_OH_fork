/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef AWCOMMON_H
#define AWCOMMON_H

#include <string>
#include "window.h"
#include "recorder.h"
namespace OHOS {
namespace Media {
#define CHECK_INSTANCE_AND_RETURN_RET(cond, ret, ...)           \
    do {                                                        \
        if (cond == nullptr) {                                  \
            cout << cond << "is nullptr" << endl;               \
            return ret;                                         \
        }                                                       \
    } while (0)

#define CHECK_BOOL_AND_RETURN_RET(cond, ret, ...)               \
    do {                                                        \
        if (!(cond)) {                                          \
            return ret;                                         \
        }                                                       \
    } while (0)

#define CHECK_STATE_AND_RETURN_RET(cond, ret, ...)              \
    do {                                                        \
        if (cond != 0) {                                        \
            return ret;                                         \
        }                                                       \
    } while (0)
namespace PlayerTestParam {
    int32_t WriteDataToFile(const std::string &path, const std::uint8_t *data, std::size_t size);
    int32_t ProduceRandomNumberCrypt(void);
} // namespace PlayerTestParam
namespace RecorderTestParam {
    struct VideoRecorderConfig_ {
        int32_t audioSourceId = 0;
        int32_t videoSourceId = 0;
        int32_t dataSourceId = 0;
        int32_t audioEncodingBitRate = 48000;
        int32_t channelCount = 2;
        int32_t duration = 60;
        int32_t width = 1280;
        int32_t height = 720;
        int32_t frameRate = 30;
        int32_t videoEncodingBitRate = 48000;
        int32_t sampleRate = 48000;
        double captureFps = 30;
        int32_t outputFd = 0;
        AudioCodecFormat audioFormat = AAC_LC;
        AudioSourceType aSource = AUDIO_MIC;
        DataSourceType dataType = METADATA;
        OutputFormatType outPutFormat = FORMAT_MPEG_4;
        VideoSourceType vSource = VIDEO_SOURCE_SURFACE_YUV;
        VideoCodecFormat videoFormat = MPEG4;
    };
    struct AudioRecorderConfig_ {
        int32_t outputFd = 0;
        int32_t audioSourceId = 0;
        int32_t audioEncodingBitRate = 48000;
        int32_t channelCount = 2;
        int32_t duration = 60;
        int32_t sampleRate = 48000;
        AudioCodecFormat audioFormat = AAC_LC;
        AudioSourceType inputSource = AUDIO_MIC;
        OutputFormatType outPutFormat = FORMAT_M4A;
    };
} // namespace RecorderTestParam
} // namespace Media
} // namespace OHOS

#endif