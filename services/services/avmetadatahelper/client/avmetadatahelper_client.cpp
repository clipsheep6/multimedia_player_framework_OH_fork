/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "avmetadatahelper_client.h"
#include "helper_listener_stub.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetadataHelperClient"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<AVMetadataHelperClient> AVMetadataHelperClient::Create(
    const sptr<IStandardAVMetadataHelperService> &ipcProxy)
{
    std::shared_ptr<AVMetadataHelperClient> AVMetadataHelper = std::make_shared<AVMetadataHelperClient>(ipcProxy);
    int32_t ret = AVMetadataHelper->CreateListenerObject();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to create listener object..");
    return AVMetadataHelper;
}

AVMetadataHelperClient::AVMetadataHelperClient(const sptr<IStandardAVMetadataHelperService> &ipcProxy)
    : avMetadataHelperProxy_(ipcProxy)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVMetadataHelperClient::~AVMetadataHelperClient()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (avMetadataHelperProxy_ != nullptr) {
        (void)avMetadataHelperProxy_->DestroyStub();
    }
    callback_ = nullptr;
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVMetadataHelperClient::SetHelperCallback(const std::shared_ptr<HelperCallback> &callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_NO_MEMORY, "input param callback is nullptr..");
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, MSERR_NO_MEMORY, "listenerStub_ is nullptr..");

    callback_ = callback;
    listenerStub_->SetHelperCallback(callback);

    CHECK_AND_RETURN_RET_LOG(avMetadataHelperProxy_ != nullptr, MSERR_SERVICE_DIED,
        "metadata service does not exist..");
    return avMetadataHelperProxy_->SetHelperCallback();
}

void AVMetadataHelperClient::MediaServerDied()
{
    std::lock_guard<std::mutex> lock(mutex_);
    avMetadataHelperProxy_ = nullptr;
    if (callback_ != nullptr) {
        callback_->OnError(MSERR_SERVICE_DIED,
            "mediaserver is died, please create a new meta data helper instance again");
    }
}

int32_t AVMetadataHelperClient::SetSource(const std::string &uri, int32_t usage)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperProxy_ != nullptr, MSERR_NO_MEMORY,
        "avmetadatahelper service does not exist.");
    return avMetadataHelperProxy_->SetSource(uri, usage);
}

int32_t AVMetadataHelperClient::SetSource(int32_t fd, int64_t offset, int64_t size, int32_t usage)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperProxy_ != nullptr, MSERR_NO_MEMORY,
        "avmetadatahelper service does not exist.");
    return avMetadataHelperProxy_->SetSource(fd, offset, size, usage);
}

int32_t AVMetadataHelperClient::SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperProxy_ != nullptr, MSERR_SERVICE_DIED,
        "Meta data service does not exist..");
    CHECK_AND_RETURN_RET_LOG(dataSrc != nullptr, MSERR_NO_MEMORY, "data source is nullptr");

    dataSrcStub_ = new(std::nothrow) MediaDataSourceStub(dataSrc);
    CHECK_AND_RETURN_RET_LOG(dataSrcStub_ != nullptr, MSERR_NO_MEMORY, "failed to new dataSrcStub object");

    sptr<IRemoteObject> object = dataSrcStub_->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "listener object is nullptr..");
    return avMetadataHelperProxy_->SetSource(object);
}

std::string AVMetadataHelperClient::ResolveMetadata(int32_t key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperProxy_ != nullptr, "", "avmetadatahelper service does not exist.");
    return avMetadataHelperProxy_->ResolveMetadata(key);
}

std::unordered_map<int32_t, std::string> AVMetadataHelperClient::ResolveMetadata()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperProxy_ != nullptr, {}, "avmetadatahelper service does not exist.");
    return avMetadataHelperProxy_->ResolveMetadataMap();
}

std::shared_ptr<Meta> AVMetadataHelperClient::GetAVMetadata()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperProxy_ != nullptr, {}, "avmetadatahelper service does not exist.");
    return avMetadataHelperProxy_->GetAVMetadata();
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperClient::FetchArtPicture()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperProxy_ != nullptr, {}, "avmetadatahelper service does not exist.");
    return avMetadataHelperProxy_->FetchArtPicture();
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperClient::FetchFrameAtTime(int64_t timeUs, int32_t option,
    const OutputConfiguration &param)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperProxy_ != nullptr, nullptr, "avmetadatahelper service does not exist.");
    return avMetadataHelperProxy_->FetchFrameAtTime(timeUs, option, param);
}

void AVMetadataHelperClient::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(avMetadataHelperProxy_ != nullptr, "avmetadatahelper service does not exist.");
    avMetadataHelperProxy_->Release();
    callback_ = nullptr;
}

int32_t AVMetadataHelperClient::CreateListenerObject()
{
    std::lock_guard<std::mutex> lock(mutex_);
    listenerStub_ = new(std::nothrow) HelperListenerStub();
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, MSERR_NO_MEMORY, "failed to new HelperListenerStub object");
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperProxy_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");

    sptr<IRemoteObject> object = listenerStub_->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "listener object is nullptr..");

    MEDIA_LOGD("SetListenerObject");
    return avMetadataHelperProxy_->SetListenerObject(object);
}
} // namespace Media
} // namespace OHOS