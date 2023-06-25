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

#include "playersetvolume_fuzzer.h"
#include <iostream>
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;

PlayerSetVolumeFuzzer::PlayerSetVolumeFuzzer()
{
}

PlayerSetVolumeFuzzer::~PlayerSetVolumeFuzzer()
{
}

namespace OHOS {
namespace Media {
bool PlayerSetVolumeFuzzer::FuzzSetVolume(uint8_t* data, size_t size)
{
    player_ = OHOS::Media::PlayerFactory::CreatePlayer();
    if (player_ == nullptr) {
        cout << "player_ is null" << endl;
        return false;
    }
    std::shared_ptr<TestPlayerCallback> cb = std::make_shared<TestPlayerCallback>();
    player_->SetPlayerCallback(cb);
    const string path = "/data/test/media/H264_AAC.mp4";
    SetFdSource(path);
    sptr<Surface> producerSurface = nullptr;
    producerSurface = GetVideoSurface();
    player_->SetVideoSurface(producerSurface);
    player_->PrepareAsync();
    sleep(1);
    player_->Play();
    if (size >= sizeof(float)) {
        player_->SetVolume(*reinterpret_cast<float *>(data), *reinterpret_cast<float *>(data));
        sleep(1);
    }
    player_->Release();
    return true;
}
}

bool FuzzPlayerSetVolume(uint8_t* data, size_t size)
{
    auto player = std::make_unique<PlayerSetVolumeFuzzer>();
    if (player == nullptr) {
        cout << "player is null" << endl;
        return true;
    }
    return player->FuzzSetVolume(data, size);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t* data, size_t size)
{
    /* Run your code on data */
    FuzzPlayerSetVolume(data, size);
    return 0;
}

