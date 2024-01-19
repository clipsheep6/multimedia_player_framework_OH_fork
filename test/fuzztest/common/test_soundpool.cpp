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

#include "test_soundpool.h"
#include <sys/stat.h>

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;

void TestSoundPoolCallback::OnError(int32_t errorCode)
{
    cout << "Error received, errorCode:" << errorCode << endl;
}

void TestSoundPoolCallback::OnLoadCompleted(int32_t soundId)
{
    cout << "OnLoadCompleted soundId:" << soundId << endl;
}

void TestSoundPoolCallback::OnPlayFinished()
{
}

TestSoundPool::TestSoundPool()
{
}

TestSoundPool::~TestSoundPool()
{
}

bool TestSoundPool::CreateSoundPool(int maxStreams, AudioStandard::AudioRendererInfo audioRenderInfo)
{
    soundPool = SoundPoolFactory::CreateSoundPool(maxStreams, audioRenderInfo);
    if (soundPool == nullptr) {
        return false;
    }
    return true;
}

int32_t TestSoundPool::Load(std::string url)
{
    if (soundPool == nullptr) {
        cout << "TestSoundPool::Load url: soundPool is null" << endl;
        return -1;
    }
    return soundPool->Load(url);
}

int32_t TestSoundPool::Load(int32_t fd, int64_t offset, int64_t length)
{
    if (soundPool == nullptr) {
        cout << "TestSoundPool::Load Fd: soundPool is null" << endl;
        return -1;
    }
    return soundPool->Load(fd, offset, length);
}

int32_t TestSoundPool::Play(int32_t soundID, PlayParams playParameters)
{
    if (soundPool == nullptr) {
        cout << "TestSoundPool::Play: soundPool is null" << endl;
        return -1;
    }
    return soundPool->Play(soundID, playParameters);
}

int32_t TestSoundPool::Stop(int32_t streamID)
{
    if (soundPool == nullptr) {
        cout << "TestSoundPool::Stop: soundPool is null" << endl;
        return -1;
    }
    return soundPool->Stop(streamID);
}

int32_t TestSoundPool::SetLoop(int32_t streamID, int32_t loop)
{
    if (soundPool == nullptr) {
        cout << "TestSoundPool::SetLoop: soundPool is null" << endl;
        return -1;
    }
    return soundPool->SetLoop(streamID, loop);
}

int32_t TestSoundPool::SetPriority(int32_t streamID, int32_t priority)
{
    if (soundPool == nullptr) {
        cout << "TestSoundPool::SetPriority: soundPool is null" << endl;
        return -1;
    }
    return soundPool->SetPriority(streamID, priority);
}

int32_t TestSoundPool::SetRate(int32_t streamID, AudioStandard::AudioRendererRate renderRate)
{
    if (soundPool == nullptr) {
        cout << "TestSoundPool::SetRate: soundPool is null" << endl;
        return -1;
    }
    return soundPool->SetRate(streamID, renderRate);
}

int32_t TestSoundPool::SetVolume(int32_t streamID, float leftVolume, float rigthVolume)
{
    if (soundPool == nullptr) {
        cout << "TestSoundPool::SetVolume: soundPool is null" << endl;
        return -1;
    }
    return soundPool->SetVolume(streamID, leftVolume, rigthVolume);
}

int32_t TestSoundPool::Unload(int32_t soundID)
{
    if (soundPool == nullptr) {
        cout << "TestSoundPool::Unload: soundPool is null" << endl;
        return -1;
    }
    return soundPool->Unload(soundID);
}

int32_t TestSoundPool::Release()
{
    if (soundPool == nullptr) {
        return -1;
    }
    return soundPool->Release();
}

int32_t TestSoundPool::SetSoundPoolCallback(const std::shared_ptr<ISoundPoolCallback> &soundPoolCallback)
{
    if (soundPool == nullptr) {
        cout << "TestSoundPool::SetSoundPoolCallback: soundPool is null" << endl;
        return -1;
    }
    return soundPool->SetSoundPoolCallback(soundPoolCallback);
}

size_t TestSoundPool::GetFileSize(const std::string& fileName)
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