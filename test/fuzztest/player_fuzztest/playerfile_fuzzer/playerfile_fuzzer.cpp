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

#include "playerfile_fuzzer.h"
#include <iostream>
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;

namespace OHOS {
namespace Media {
PlayerFileFuzzer::PlayerFileFuzzer()
{
}

PlayerFileFuzzer::~PlayerFileFuzzer()
{
}

bool PlayerFileFuzzer::FuzzFile(const uint8_t* data, size_t size)
{
    player_ = OHOS::Media::PlayerFactory::CreatePlayer();
    if (player_ == nullptr) {
        return false;
    }
    std::shared_ptr<TestPlayerCallback> cb = std::make_shared<TestPlayerCallback>();
    player_->SetPlayerCallback(cb);
    const string path = "/data/test/media/fuzztest.mp4";
    int32_t retWrite = WriteDataToFile(path, data, size);
    if (retWrite != 0) {
        return true;
    }
    int32_t retValue = SetFdSource(path);
    if (retValue != 0) {
        return true;
    }
    sptr<Surface> producerSurface = nullptr;
    producerSurface = GetVideoSurface();
    player_->SetVideoSurface(producerSurface);
    player_->PrepareAsync();
    sleep(3); // wait PrepareAsync success, sleep 3 s
    player_->Play();
    sleep(1);
    player_->Pause();
    player_->Seek(3000, SEEK_NEXT_SYNC); // seek 3000 ms
    player_->SetVolume(0.5, 0.5); // leftVolume is  0.5, rightVolume is 0.5
    int32_t time;
    player_->GetCurrentTime(time);
    std::vector<Format> videoTrack;
    player_->GetVideoTrackInfo(videoTrack);
    std::vector<Format> audioTrack;
    player_->GetAudioTrackInfo(audioTrack);
    player_->GetVideoWidth();
    player_->GetVideoHeight();
    int32_t duration = 0;
    player_->GetDuration(duration);
    player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X);
    PlaybackRateMode mode;
    player_->GetPlaybackSpeed(mode);
    player_->SelectBitRate(0);
    player_->Reset();
    player_->Stop();
    player_->Release();
    return true;
}
}

int32_t WriteDataToFile(const string &path, const uint8_t* data, size_t size)
{
    FILE *file = nullptr;
    file = fopen(path.c_str(), "w+");
    if (file == nullptr) {
        return -1;
    }
    if (fwrite(data, 1, size, file) != size) {
        (void)fclose(file);
        return -1;
    }
    (void)fclose(file);
    return 0;
}

bool FuzzPlayerFile(const uint8_t* data, size_t size)
{
    auto player = std::make_unique<PlayerFileFuzzer>();
    if (player == nullptr) {
        return true;
    }
    return player->FuzzFile(data, size);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    FuzzPlayerFile(data, size);
    return 0;
}
