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

#include "acodecfile_fuzzer.h"
#include <iostream>
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"
#include "aw_common.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::Media::ACodecTestParam;
using namespace OHOS::Media::PlayerTestParam;

ACodecFileFuzzer::ACodecFileFuzzer()
{
}

ACodecFileFuzzer::~ACodecFileFuzzer()
{
}

bool ACodecFileFuzzer::FuzzAudioFile(uint8_t *data, size_t size)
{
    constexpr int32_t WAITTING_TIME = 2;
    std::shared_ptr<ACodecSignal> acodecSignal = std::make_shared<ACodecSignal>();
    adecCallback_ = std::make_shared<ADecCallbackTest>(acodecSignal);
    CHECK_INSTANCE_AND_RETURN_RET(adecCallback_, false);
    aencCallback_ = std::make_shared<AEncCallbackTest>(acodecSignal);
    CHECK_INSTANCE_AND_RETURN_RET(aencCallback_, false);
    audioCodec_ = std::make_shared<ACodecMock>(acodecSignal);
    CHECK_INSTANCE_AND_RETURN_RET(audioCodec_, false);
    CHECK_BOOL_AND_RETURN_RET(audioCodec_->CreateAudioEncMockByMime("audio/mp4a-latm"), false);
    CHECK_STATE_AND_RETURN_RET(audioCodec_->SetCallbackEnc(aencCallback_), false);
    CHECK_BOOL_AND_RETURN_RET(audioCodec_->CreateAudioDecMockByMime("audio/mp4a-latm"), false);
    CHECK_STATE_AND_RETURN_RET(audioCodec_->SetCallbackDec(adecCallback_), false);

    defaultFormat_ = AVCodecMockFactory::CreateFormat();
    CHECK_INSTANCE_AND_RETURN_RET(defaultFormat_, false);
    (void)defaultFormat_->PutIntValue("channel_count", 2); // 2 common channel count
    (void)defaultFormat_->PutIntValue("sample_rate", 44100); // 44100 common sample rate
    (void)defaultFormat_->PutIntValue("audio_sample_format", 1); // 1 AudioStandard::SAMPLE_S16LE

    audioCodec_->SetOutPath("/data/test/media/aac_filefuzz_out.es");
    const string path = "/data/test/media/AAC_48000_32_1.aac";
    CHECK_STATE_AND_RETURN_RET(WriteDataToFile(path, data, size), false);
    
    audioCodec_->ConfigureEnc(defaultFormat_);
    audioCodec_->ConfigureDec(defaultFormat_);
    audioCodec_->PrepareDec();
    audioCodec_->PrepareEnc();
    audioCodec_->StartDec();
    audioCodec_->StartEnc();
    sleep(WAITTING_TIME);
    if (audioCodec_ != nullptr) {
        CHECK_STATE_AND_RETURN_RET(audioCodec_->ReleaseDec(), false);
        CHECK_STATE_AND_RETURN_RET(audioCodec_->ReleaseEnc(), false);
    }
    return true;
}

bool OHOS::Media::FuzzACodecFile(uint8_t *data, size_t size)
{
    auto codecfuzzer = std::make_unique<ACodecFileFuzzer>();
    if (codecfuzzer == nullptr) {
        cout << "codecfuzzer is null" << endl;
        return 0;
    }
    if (size < sizeof(int32_t)) {
        return 0;
    }
    return codecfuzzer->FuzzAudioFile(data, size);
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::FuzzACodecFile(data, size);
    return 0;
}

