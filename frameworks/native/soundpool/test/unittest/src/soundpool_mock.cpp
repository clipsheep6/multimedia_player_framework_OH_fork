/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "soundpool_mock.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

void SoundPoolCallbackTest::OnError(int32_t errorCode)
{
    cout << "Error received, errorCode:" << errorCode << endl;
}

void SoundPoolCallbackTest::OnLoadCompleted(int32_t soundId)
{
    cout << "OnLoadCompleted soundId:" << soundId << ", haveLoadedSoundNumInner_: "<< haveLoadedSoundNumInner_ << endl;
    haveLoadedSoundNumInner_++;
}

void SoundPoolCallbackTest::OnPlayFinished()
{
    cout << "OnPlayFinished haveLoadedSoundNumInner_: "<< havePlayedSoundNumInner_ << endl;
    havePlayedSoundNumInner_++;
}

bool SoundPoolMock::CreateSoundPool(int maxStreams, AudioStandard::AudioRendererInfo audioRenderInfo)
{
    soundPool_ = SoundPoolFactory::CreateSoundPool(maxStreams, audioRenderInfo);
    return soundPool_ != nullptr;
}

int32_t SoundPoolMock::Load(std::string url)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(soundPool_ != nullptr, MSERR_INVALID_OPERATION, "soundPool_ == nullptr");
    return soundPool_->Load(url);
}

int32_t SoundPoolMock::Load(int32_t fd, int64_t offset, int64_t length)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(soundPool_ != nullptr, MSERR_INVALID_OPERATION, "soundPool_ == nullptr");
    return soundPool_->Load(fd, offset, length);
}

int32_t SoundPoolMock::Play(int32_t soundID, PlayParams playParameters)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(soundPool_ != nullptr, MSERR_INVALID_OPERATION, "soundPool_ == nullptr");
    return soundPool_->Play(soundID, playParameters);
}

int32_t SoundPoolMock::Stop(int32_t streamID)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(soundPool_ != nullptr, MSERR_INVALID_OPERATION, "soundPool_ == nullptr");
    if (isExit_.load()) {
        isExit_.store(true);
    }
    return soundPool_->Stop(streamID);
}

int32_t SoundPoolMock::SetLoop(int32_t streamID, int32_t loop)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(soundPool_ != nullptr, MSERR_INVALID_OPERATION, "soundPool_ == nullptr");
    return soundPool_->SetLoop(streamID, loop);
}

int32_t SoundPoolMock::SetPriority(int32_t streamID, int32_t priority)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(soundPool_ != nullptr, MSERR_INVALID_OPERATION, "soundPool_ == nullptr");
    return soundPool_->SetPriority(streamID, priority);
}

int32_t SoundPoolMock::SetRate(int32_t streamID, AudioStandard::AudioRendererRate renderRate)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(soundPool_ != nullptr, MSERR_INVALID_OPERATION, "soundPool_ == nullptr");
    return soundPool_->SetRate(streamID, renderRate);
}

int32_t SoundPoolMock::SetVolume(int32_t streamID, float leftVolume, float rigthVolume)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(soundPool_ != nullptr, MSERR_INVALID_OPERATION, "soundPool_ == nullptr");
    return soundPool_->SetVolume(streamID, leftVolume, rigthVolume);
}

int32_t SoundPoolMock::Unload(int32_t soundID)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(soundPool_ != nullptr, MSERR_INVALID_OPERATION, "soundPool_ == nullptr");
    return soundPool_->Unload(soundID);
}

int32_t SoundPoolMock::Release()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(soundPool_ != nullptr, MSERR_INVALID_OPERATION, "soundPool_ == nullptr");
    if (isExit_.load()) {
        isExit_.store(true);
    }
    return soundPool_->Release();
}

int32_t SoundPoolMock::SetSoundPoolCallback(const std::shared_ptr<ISoundPoolCallback> &soundPoolCallback)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(soundPool_ != nullptr, MSERR_INVALID_OPERATION, "soundPool_ == nullptr");
    return soundPool_->SetSoundPoolCallback(soundPoolCallback);
}

size_t SoundPoolMock::GetFileSize(const std::string& fileName)
{
    size_t fileSize = 0;
    if (!fileName.empty()) {
        struct stat fileStatus {};
        if (stat(fileName.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<size_t>(fileStatus.st_size);
        }
    }
    return fileSize;
}