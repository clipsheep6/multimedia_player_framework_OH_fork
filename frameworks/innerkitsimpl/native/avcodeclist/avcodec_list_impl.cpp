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

#include "avcodec_list_impl.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecListImpl"};
}
namespace OHOS {
namespace Media {
std::shared_ptr<AVCodecList> AVCodecListFactory::CreateAVCodecList()
{
    std::shared_ptr<AVCodecListImpl> impl = std::make_shared<AVCodecListImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "failed to new AVCodecListImpl");

    int32_t ret = impl->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to init AVCodecListImpl");

    return impl;
}

int32_t AVCodecListImpl::Init()
{
    return MSERR_OK;
}

AVCodecListImpl::AVCodecListImpl()
{
    MEDIA_LOGD("AVCodecListImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecListImpl::~AVCodecListImpl()
{
    MEDIA_LOGD("AVCodecListImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

std::string AVCodecListImpl::FindVideoDecoder(const Format &format)
{
    return "a";
}

std::string AVCodecListImpl::FindVideoEncoder(const Format &format)
{
    return "a";
}

std::string AVCodecListImpl::FindAudioDecoder(const Format &format)
{
    return "a";
}

std::string AVCodecListImpl::FindAudioEncoder(const Format &format)
{
    return "a";
}

std::vector<CapabilityData> AVCodecListImpl::GetCodecCapabilityInfos()
{
    std::vector<CapabilityData> vec;
    return vec;
}

std::vector<std::shared_ptr<VideoCaps>> AVCodecListImpl::GetVideoDecoderCaps()
{
    std::vector<CapabilityData> capabilityArray = GetCodecCapabilityInfos();

    SelectTargetCapabilityDataArray(capabilityArray, AVCODEC_TYPE_VIDEO_DECODER);
    std::vector<std::shared_ptr<VideoCaps>> videoCapsArray;
    for (auto iter = capabilityArray.begin(); iter != capabilityArray.end(); iter++) {
        std::shared_ptr<VideoCaps> videoCaps = std::make_shared<VideoCaps>(*iter);
        CHECK_AND_RETURN_RET_LOG(videoCaps != nullptr, videoCapsArray, "Is null mem");
        videoCapsArray.push_back(videoCaps);
    }
    return videoCapsArray;
}

std::vector<std::shared_ptr<VideoCaps>> AVCodecListImpl::GetVideoEncoderCaps()
{
    std::vector<CapabilityData> capabilityArray = GetCodecCapabilityInfos();
    SelectTargetCapabilityDataArray(capabilityArray, AVCODEC_TYPE_VIDEO_ENCODER);
    std::vector<std::shared_ptr<VideoCaps>> videoCapsArray;
    for (auto iter = capabilityArray.begin(); iter != capabilityArray.end(); iter++) {
        std::shared_ptr<VideoCaps> videoCaps = std::make_shared<VideoCaps>(*iter);
        CHECK_AND_RETURN_RET_LOG(videoCaps != nullptr, videoCapsArray, "Is null mem");
        videoCapsArray.push_back(videoCaps);
    }
    return videoCapsArray;
}

std::vector<std::shared_ptr<AudioCaps>> AVCodecListImpl::GetAudioDecoderCaps()
{
    std::vector<CapabilityData> capabilityArray = GetCodecCapabilityInfos();
    SelectTargetCapabilityDataArray(capabilityArray, AVCODEC_TYPE_AUDIO_DECODER);
    std::vector<std::shared_ptr<AudioCaps>> audioCapsArray;
    for (auto iter = capabilityArray.begin(); iter != capabilityArray.end(); iter++) {
        std::shared_ptr<AudioCaps> audioCaps = std::make_shared<AudioCaps>(*iter);
        CHECK_AND_RETURN_RET_LOG(audioCaps != nullptr, audioCapsArray, "Is null mem");
        audioCapsArray.push_back(audioCaps);
    }
    return audioCapsArray;
}

std::vector<std::shared_ptr<AudioCaps>> AVCodecListImpl::GetAudioEncoderCaps()
{
    std::vector<CapabilityData> capabilityArray = GetCodecCapabilityInfos();
    SelectTargetCapabilityDataArray(capabilityArray, AVCODEC_TYPE_AUDIO_ENCODER);
    std::vector<std::shared_ptr<AudioCaps>> audioCapsArray;
    for (auto iter = capabilityArray.begin(); iter != capabilityArray.end(); iter++) {
        std::shared_ptr<AudioCaps> audioCaps = std::make_shared<AudioCaps>(*iter);
        CHECK_AND_RETURN_RET_LOG(audioCaps != nullptr, audioCapsArray, "Is null mem");
        audioCapsArray.push_back(audioCaps);
    }
    return audioCapsArray;
}

void AVCodecListImpl::SelectTargetCapabilityDataArray(std::vector<CapabilityData> &capabilityArray,
                                                    const AVCodecType &codecType)
{
    for (auto iter = capabilityArray.begin(); iter != capabilityArray.end();) {
        if (iter->codecType == codecType) {
            ++iter;
        } else {
            iter = capabilityArray.erase(iter);
        }
    }
}
}
}