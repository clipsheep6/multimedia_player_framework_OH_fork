/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "transcoder_client.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "TransCoderClient"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<TransCoderClient> TransCoderClient::Create(const sptr<IStandardTransCoderService> &ipcProxy)
{
    CHECK_AND_RETURN_RET_LOG(ipcProxy != nullptr, nullptr, "ipcProxy is nullptr..");

    std::shared_ptr<TransCoderClient> transCoder = std::make_shared<TransCoderClient>(ipcProxy);
    CHECK_AND_RETURN_RET_LOG(transCoder != nullptr, nullptr, "failed to new TransCoderClient..");

    int32_t ret = transCoder->CreateListenerObject();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to create listener object..");

    return transCoder;
}

TransCoderClient::TransCoderClient(const sptr<IStandardTransCoderService> &ipcProxy)
    : transCoderProxy_(ipcProxy)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

TransCoderClient::~TransCoderClient()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        (void)DisableMonitor();
        if (transCoderProxy_ != nullptr) {
            (void)transCoderProxy_->DestroyStub();
            transCoderProxy_ = nullptr;
        }
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void TransCoderClient::MediaServerDied()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        transCoderProxy_ = nullptr;
        listenerStub_ = nullptr;
        if (callback_ != nullptr) {
            callback_->OnError(MSERR_SERVICE_DIED,
                "mediaserver is died, please create a new transcodercallback instance again");
        }
    }
}

int32_t TransCoderClient::CreateListenerObject()
{
    std::lock_guard<std::mutex> lock(mutex_);
    listenerStub_ = new(std::nothrow) TransCoderListenerStub();
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, MSERR_NO_MEMORY, "failed to new TransCoderListenerStub object");
    CHECK_AND_RETURN_RET_LOG(transCoderProxy_ != nullptr, MSERR_NO_MEMORY, "transcoder service does not exist.");

    (void)listenerStub_->SetMonitor(weak_from_this());
    sptr<IRemoteObject> object = listenerStub_->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "listener object is nullptr..");

    MEDIA_LOGD("SetListenerObject");
    return transCoderProxy_->SetListenerObject(object);
}

int32_t TransCoderClient::SetVideoEncoder(VideoCodecFormat encoder)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(transCoderProxy_ != nullptr, MSERR_NO_MEMORY, "transcoder service does not exist.");

    MEDIA_LOGD("SetVideoSource encoder(%{public}d)", encoder);
    return transCoderProxy_->SetVideoEncoder(encoder);
}

int32_t TransCoderClient::SetVideoSize(int32_t width, int32_t height)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(transCoderProxy_ != nullptr, MSERR_NO_MEMORY, "transcoder service does not exist.");

    MEDIA_LOGD("SetVideoSize width(%{public}d), height(%{public}d)", width, height);
    return transCoderProxy_->SetVideoSize(width, height);
}

int32_t TransCoderClient::SetVideoEncodingBitRate(int32_t rate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(transCoderProxy_ != nullptr, MSERR_NO_MEMORY, "transcoder service does not exist.");

    MEDIA_LOGD("SetVideoEncodingBitRate rate(%{public}d)", rate);
    return transCoderProxy_->SetVideoEncodingBitRate(rate);
}

int32_t TransCoderClient::SetAudioEncoder(AudioCodecFormat encoder)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(transCoderProxy_ != nullptr, MSERR_NO_MEMORY, "transcoder service does not exist.");

    MEDIA_LOGD("SetAudioEncoder encoder(%{public}d)", encoder);
    return transCoderProxy_->SetAudioEncoder(encoder);
}

int32_t TransCoderClient::SetAudioEncodingBitRate(int32_t bitRate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(transCoderProxy_ != nullptr, MSERR_NO_MEMORY, "transcoder service does not exist.");

    MEDIA_LOGD("SetAudioEncodingBitRate bitRate(%{public}d)", bitRate);
    return transCoderProxy_->SetAudioEncodingBitRate(bitRate);
}

int32_t TransCoderClient::SetOutputFormat(OutputFormatType format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(transCoderProxy_ != nullptr, MSERR_NO_MEMORY, "transcoder service does not exist.");

    MEDIA_LOGD("SetOutputFormat format(%{public}d)", format);
    return transCoderProxy_->SetOutputFormat(format);
}

int32_t TransCoderClient::SetInputFile(std::string url)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(transCoderProxy_ != nullptr, MSERR_NO_MEMORY, "transcoder service does not exist.");

    MEDIA_LOGD("SetInputFile url(%{public}s)", url.c_str());
    return transCoderProxy_->SetInputFile(url);
}

int32_t TransCoderClient::SetInputFile(int32_t fd, int64_t offset, int64_t size)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(transCoderProxy_ != nullptr, MSERR_NO_MEMORY, "transcoder service does not exist.");

    return transCoderProxy_->SetInputFile(fd, offset, size);
}

int32_t TransCoderClient::SetOutputFile(int32_t fd)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(transCoderProxy_ != nullptr, MSERR_NO_MEMORY, "transcoder service does not exist.");

    MEDIA_LOGD("SetOutputFile fd(%{public}d)", fd);
    return transCoderProxy_->SetOutputFile(fd);
}

int32_t TransCoderClient::SetTransCoderCallback(const std::shared_ptr<TransCoderCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_NO_MEMORY, "input param callback is nullptr.");
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, MSERR_NO_MEMORY, "listenerStub_ is nullptr.");

    callback_ = callback;
    MEDIA_LOGD("SetTransCoderCallback");
    listenerStub_->SetTransCoderCallback(callback);
    return MSERR_OK;
}

int32_t TransCoderClient::Prepare()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(transCoderProxy_ != nullptr, MSERR_NO_MEMORY, "transcoder service does not exist.");

    MEDIA_LOGD("Prepare");
    return transCoderProxy_->Prepare();
}

int32_t TransCoderClient::ExecuteWhen(int32_t ret, bool ok)
{
    if ((ok && (ret == MSERR_OK)) || ((!ok) && (ret != MSERR_OK))) {
        (void)DisableMonitor();
    }
    return ret;
}

int32_t TransCoderClient::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(transCoderProxy_ != nullptr, MSERR_NO_MEMORY, "transcoder service does not exist.");

    MEDIA_LOGD("TransCoderClient::Start");
    (void)EnableMonitor();
    return ExecuteWhen(transCoderProxy_->Start(), false);
}

int32_t TransCoderClient::Pause()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(transCoderProxy_ != nullptr, MSERR_NO_MEMORY, "transcoder service does not exist.");

    MEDIA_LOGD("Pause");
    return ExecuteWhen(transCoderProxy_->Pause(), true);
}

int32_t TransCoderClient::Resume()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(transCoderProxy_ != nullptr, MSERR_NO_MEMORY, "transcoder service does not exist.");

    MEDIA_LOGD("Resume");
    (void)EnableMonitor();
    return ExecuteWhen(transCoderProxy_->Resume(), false);
}

int32_t TransCoderClient::Cancel()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(transCoderProxy_ != nullptr, MSERR_NO_MEMORY, "transcoder service does not exist.");

    MEDIA_LOGD("Cancel");
    return ExecuteWhen(transCoderProxy_->Cancel(), true);
}

int32_t TransCoderClient::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(transCoderProxy_ != nullptr, MSERR_NO_MEMORY, "transcoder service does not exist.");

    MEDIA_LOGD("Release");
    return ExecuteWhen(transCoderProxy_->Release(), true);
}
} // namespace Media
} // namespace OHOS
