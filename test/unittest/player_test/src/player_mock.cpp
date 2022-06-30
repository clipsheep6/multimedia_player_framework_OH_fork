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

#include "player_mock.h"
#include "media_errors.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"

using namespace OHOS::Media::PlayerTestParam;

namespace OHOS {
namespace Media {
void PlayerSignal::SetState(PlayerStates state)
{
    state_ = state;
}

void PlayerSignal::SetSeekResult(bool seekDoneFlag)
{
    seekDoneFlag_ = seekDoneFlag;
}

void PlayerCallbackTest::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    switch (type) {
        case INFO_TYPE_SEEKDONE:
            seekDoneFlag_ = true;
            test_->SetSeekResult(true);
            SeekNotify(extra, infoBody);
            break;
        case INFO_TYPE_STATE_CHANGE:
            state_ = static_cast<PlayerStates>(extra);
            test_->SetState(state_);
            Notify(state_);
            break;
        case INFO_TYPE_POSITION_UPDATE:
            position_ = extra;
            break;
        default:
            break;
    }
}

void PlayerCallbackTest::Notify(PlayerStates currentState)
{
    if (currentState == PLAYER_PREPARED) {
        test_->condVarPrepare_.notify_all();
    } else if (currentState == PLAYER_STARTED) {
        test_->condVarPlay_.notify_all();
    } else if (currentState == PLAYER_PAUSED) {
        test_->condVarPause_.notify_all();
    } else if (currentState == PLAYER_STOPPED) {
        test_->condVarStop_.notify_all();
    } else if (currentState == PLAYER_IDLE) {
        test_->condVarReset_.notify_all();
    }
}

void PlayerCallbackTest::SeekNotify(int32_t extra, const Format &infoBody)
{
    if (test_->seekMode_ == PlayerSeekMode::SEEK_CLOSEST) {
        if (test_->seekPosition_ == extra) {
            test_->condVarSeek_.notify_all();
        }
    } else if (test_->seekMode_ == PlayerSeekMode::SEEK_PREVIOUS_SYNC) {
        if (test_->seekPosition_ - extra < DELTA_TIME && extra - test_->seekPosition_ >= 0) {
            test_->condVarSeek_.notify_all();
        }
    } else if (test_->seekMode_ == PlayerSeekMode::SEEK_NEXT_SYNC) {
        if (extra - test_->seekPosition_ < DELTA_TIME && test_->seekPosition_ - extra >= 0) {
            test_->condVarSeek_.notify_all();
        }
    } else if (abs(test_->seekPosition_ - extra) <= DELTA_TIME) {
        test_->condVarSeek_.notify_all();
    } else {
        test_->SetSeekResult(false);
    }
}

Player_mock::Player_mock(std::shared_ptr<PlayerSignal> test) : test_(test) {}

Player_mock::~Player_mock()
{
    if (previewWindow_ != nullptr) {
        previewWindow_->Destroy();
        previewWindow_ = nullptr;
    }
}

bool Player_mock::CreatePlayer()
{
    player_ = PlayerFactory::CreatePlayer();
    return player_ != nullptr;
}

int32_t Player_mock::SetSource(const std::string url)
{
    int32_t ret = player_->SetSource(url);
    return ret;
}

int32_t Player_mock::Reset()
{
    int32_t ret = player_->Reset();
    if (ret == MSERR_OK && test_->state_ != PLAYER_IDLE) {
        std::unique_lock<std::mutex> lockReset(test_->mutexReset_);
        test_->condVarReset_.wait_for(lockReset, std::chrono::seconds(WAITSECOND));
        if (test_->state_ != PLAYER_IDLE) {
            return -1;
        }
    }
    return ret;
}

int32_t Player_mock::Release()
{
    if (previewWindow_ != nullptr) {
        previewWindow_->Destroy();
        previewWindow_ = nullptr;
    }
    return player_->Release();
}

int32_t Player_mock::SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback)
{
    return player_->SetPlayerCallback(callback);
}

PlayerCallbackTest::PlayerCallbackTest(std::shared_ptr<PlayerSignal> test)
    : test_(test) {}
} // namespace Media
} // namespace OHOS
