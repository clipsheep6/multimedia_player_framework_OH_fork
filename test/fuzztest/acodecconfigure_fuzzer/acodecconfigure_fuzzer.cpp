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

#include "acodecconfigure_fuzzer.h"
#include <iostream>
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::Media::ACodecTestParam;

ACodecConfigureFuzzer::ACodecConfigureFuzzer()
{
}

ACodecConfigureFuzzer::~ACodecConfigureFuzzer()
{
}

bool ACodecConfigureFuzzer::FuzzAudioConfigure(uint8_t* data, size_t size)
{
    while(true) {
        std::shared_ptr<ACodecSignal> acodecSignal = std::make_shared<ACodecSignal>();
        adecCallback_ = std::make_shared<ADecCallbackTest>(acodecSignal);
        if (adecCallback_ == nullptr) {
            cout << "create adecCallback_ fail" << endl;
            return false;
        } else {
            cout << "create adecCallback_ success" << endl;
        }
        aencCallback_ = std::make_shared<AEncCallbackTest>(acodecSignal);
        if (aencCallback_ == nullptr) {
            cout << "create aencCallback_ fail" << endl;
            return false;
        } else {
            cout << "create aencCallback_ success" << endl;            
        }
        audioCodec_ = std::make_shared<ACodecMock>(acodecSignal);
        if (audioCodec_ == nullptr) {
            cout << "create audioCodec_ fail" << endl;
            return false;
        } else {
            cout << "create audioCodec_ success" << endl;
        }
        cout << "enter CreateAudioEncMockByMine" << endl;
        bool retcreate = audioCodec_->CreateAudioEncMockByMine("audio/mp4a-latm");
        if (!retcreate) {
            cout << "enc CreateAudioEncMockByMine fail" << endl;
        } else {
            cout << "enc CreateAudioEncMockByMine success" << endl;  
        }
        cout << "enter SetCallbackEnc" << endl;
        int32_t ret = audioCodec_->SetCallbackEnc(aencCallback_);
        if (ret != 0) {
            cout << "enc SetCallbackEnc fail" << endl;
        } else {
            cout << "enc SetCallbackEnc success" << endl;
        }
        cout << "enter CreateAudioDecMockByMine" << endl;
        retcreate = audioCodec_->CreateAudioDecMockByMine("audio/mp4a-latm");
        if (!retcreate) {
            cout << "DEC CreateAudioDecMockByMine fail" << endl;
        } else {
            cout << "DEC CreateAudioDecMockByMine success" << endl;
        }
        ret = audioCodec_->SetCallbackDec(adecCallback_);
        if (ret != 0) {
            cout << "DEC SetCallbackDec fail" << endl;
        } else {
            cout << "DEC SetCallbackDec success" << endl;     
        }

        defaultFormat_ = AVCodecMockFactory::CreateFormat();
        if (defaultFormat_ == nullptr) {
            cout << "create defaultFormat_ fail" << endl;
        }
        int32_t data_ = *reinterpret_cast<int32_t *>(data);
        cout << "configure data " << data_ << endl;
        (void)defaultFormat_->PutIntValue("channel_count", 1); // 2 common channel count
        (void)defaultFormat_->PutIntValue("sample_rate", data_); // 44100 common sample rate
        (void)defaultFormat_->PutIntValue("audio_sample_format", 1); // 1 AudioStandard::SAMPLE_S16LE

        string prefix = "/data/test/media/";
        string fileName = "aac_out";
        string suffix = ".es";
        audioCodec_->SetOutPath(prefix + fileName + suffix);
        audioCodec_->SetReadPath("/data/test/media/LC_AAC_1_8000_1s.aac");

        ret = audioCodec_->ConfigureEnc(defaultFormat_);
        if (ret == 0) {
            cout << "enc configure success" << endl;
        } else {
            cout << "enc configure fail" << endl;
            break;
        }
        ret = audioCodec_->ConfigureDec(defaultFormat_);
        if (ret == 0) {
            cout << "dec configure success" << endl;
        } else {
            cout << "dec configure fail" << endl;
            break;
        }
        ret = audioCodec_->PrepareDec();
        if (ret == 0) {
            cout << "dec Prepare success" << endl;
        } else {
            cout << "dec Prepare fail" << endl;
            break;
        }
        ret = audioCodec_->PrepareEnc();
        if (ret == 0) {
            cout << "enc Prepare success" << endl;
        } else {
            cout << "enc Prepare fail" << endl;
            break;
        }
        ret = audioCodec_->StartDec();
        if (ret == 0) {
            cout << "dec start success" << endl;
        } else {
            cout << "dec start fail" << endl;
            break;
        }
        ret = audioCodec_->StartEnc();
        if (ret == 0) {
            cout << "enc start success" << endl;
        } else {
            cout << "enc start fail" << endl;
            break;
        }
        sleep(3);
        break;
    }
    if (audioCodec_ != nullptr) {
        int32_t ret = audioCodec_->ReleaseDec();
        if (ret == 0) {
            cout << "dec release success" << endl;
        } else {
            cout << "dec release fail" << endl;
            return false;
        }
        ret = audioCodec_->ReleaseEnc();
        if (ret == 0) {
            cout << "enc release success" << endl;
        } else {
            cout << "enc release fail" << endl;
            return false;
        }
    }
    cout << "next return true" << endl;
    return true;
}

bool OHOS::Media::FuzzACodecConfigure(uint8_t* data, size_t size)
{
    auto codecfuzzer = std::make_unique<ACodecConfigureFuzzer>();
    if (codecfuzzer == nullptr) {
        cout << "codecfuzzer is null" << endl;
        return 0;
    }
    return codecfuzzer->FuzzAudioConfigure(data, size);
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::FuzzACodecConfigure(data, size);
    return 0;
}

