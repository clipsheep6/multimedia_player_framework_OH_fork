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

#include "playerseek_fuzzer.h"
#include <iostream>
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;

PlayerSeekFuzzer::PlayerSeekFuzzer()
{
}

PlayerSeekFuzzer::~PlayerSeekFuzzer()
{
}

bool PlayerSeekFuzzer::FuzzSeek(uint8_t* data, size_t size)
{
    player_ = OHOS::Media::PlayerFactory::CreatePlayer();
    if (player_ == nullptr) {
        return false;
    }
    std::shared_ptr<TestPlayerCallback> cb = std::make_shared<TestPlayerCallback>();
    int32_t ret = player_->SetPlayerCallback(cb);
    if (ret != 0) {
    }
    const string path = "/data/test/media/H264_AAC.mp4";
    if ((SetFdSource(path)) != 0) {
        return false;
    }
    sptr<Surface> producerSurface = nullptr;
    producerSurface = GetVideoSurface();
    if ((player_->SetVideoSurface(producerSurface)) != 0) {
    }

    if ((player_->PrepareAsync()) != 0) {
        return false;
    }
    sleep(1);
    ret = player_->Play();
    if (ret != 0) {
        return false;
    }
    if (size >= sizeof(int32_t)) {
        int32_t data_ = *reinterpret_cast<int32_t *>(data);
        cout << "seek to " << data_ << endl;
        ret = player_->Seek(data_, SEEK_NEXT_SYNC);
        if (ret != 0) {
            return false;
        } else {
            sleep(1);
        }
    }
        
    ret = player_->Release();
    if (ret != 0) {
        return false;
    }
    return true;
}

bool OHOS::Media::FuzzPlayerSeek(uint8_t* data, size_t size)
{
    auto player = std::make_unique<PlayerSeekFuzzer>();
    if (player == nullptr) {
        return 0;
    }
    return player->FuzzSeek(data, size);
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::FuzzPlayerSeek(data, size);
    return 0;
}

