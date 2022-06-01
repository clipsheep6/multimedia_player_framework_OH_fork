/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "recorder_profiles_impl.h"
#include "media_log.h"
#include "media_errors.h"
#include "i_media_service.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderProfilesImpl"};
}

namespace OHOS {
namespace Media {
RecorderProfiles& RecorderProfilesFactory::CreateRecorderProfiles()
{
    return RecorderProfilesImpl::GetInstance();
}

RecorderProfiles& RecorderProfilesImpl::GetInstance()
{
    static RecorderProfilesImpl instance;
    instance.Init();
    return instance;
}

int32_t RecorderProfilesImpl::Init()
{
    if (recorderProfilesService_ == nullptr) {
        recorderProfilesService_ = MediaServiceFactory::GetInstance().CreateRecorderProfilesService();
    }
    CHECK_AND_RETURN_RET_LOG(recorderProfilesService_ != nullptr,
        MSERR_NO_MEMORY, "failed to create RecorderProfiles service");
    return MSERR_OK;
}

RecorderProfilesImpl::RecorderProfilesImpl()
{
    MEDIA_LOGD("RecorderProfilesImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

RecorderProfilesImpl::~RecorderProfilesImpl()
{
    if (recorderProfilesService_ != nullptr) {
        (void)MediaServiceFactory::GetInstance().DestroyMediaProfileService(recorderProfilesService_);
        recorderProfilesService_ = nullptr;
    }
    MEDIA_LOGD("RecorderProfilesImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool RecorderProfilesImpl::IsAudioRecoderConfigSupported(const AudioRecorderProfile &profile)
{
    CHECK_AND_RETURN_RET_LOG(recorderProfilesService_ != nullptr, false, "RecorderProfiles service does not exist.");
    RecorderProfilesData profileData;
    profileData.containerFormatType = profile.containerFormatType;
    profileData.audioCodec = profile.audioCodec;
    profileData.audioBitrate = profile.audioBitrate;
    profileData.audioSampleRate = profile.audioSampleRate;
    profileData.audioChannels = profile.audioChannels;
    return recorderProfilesService_->IsAudioRecoderConfigSupported(profileData);
}

bool RecorderProfilesImpl::HasVideoRecorderProfile(int32_t sourceId, int32_t qualityLevel)
{
    CHECK_AND_RETURN_RET_LOG(recorderProfilesService_ != nullptr, false, "RecorderProfiles service does not exist.");
    return recorderProfilesService_->HasVideoRecorderProfile(sourceId, qualityLevel);
}

std::vector<std::shared_ptr<AudioRecorderCaps>> RecorderProfilesImpl::GetAudioRecorderCaps()
{
    std::vector<RecorderProfilesData> capabilityArray = recorderProfilesService_->GetAudioRecorderCapsInfo();
    std::vector<std::shared_ptr<AudioRecorderCaps>> audioRecorderCapsArray;
    RecorderProfilesData profileData;
    for (auto iter = capabilityArray.begin(); iter != capabilityArray.end(); iter++) {
        profileData = *iter;
        std::shared_ptr<AudioRecorderCaps> audioRecorderCaps = std::make_shared<AudioRecorderCaps>();
        CHECK_AND_RETURN_RET_LOG(audioRecorderCaps != nullptr, audioRecorderCapsArray, "Is null mem");
        audioRecorderCaps->containerFormatType = profileData.containerFormatType;
        audioRecorderCaps->mimeType = profileData.mimeType;
        audioRecorderCaps->bitrate = profileData.bitrate;
        audioRecorderCaps->channels = profileData.channels;
        audioRecorderCaps->sampleRate = profileData.sampleRate;
        audioRecorderCapsArray.push_back(audioRecorderCaps);
    }
    return audioRecorderCapsArray;
}

std::vector<std::shared_ptr<VideoRecorderCaps>> RecorderProfilesImpl::GetVideoRecorderCaps()
{
    std::vector<RecorderProfilesData> capabilityArray = recorderProfilesService_->GetVideoRecorderCapsInfo();
    std::vector<std::shared_ptr<VideoRecorderCaps>> videoRecorderCapsArray;
    RecorderProfilesData profileData;
    for (auto iter = capabilityArray.begin(); iter != capabilityArray.end(); iter++) {
        profileData = *iter;
        std::shared_ptr<VideoRecorderCaps> videoRecorderCaps = std::make_shared<VideoRecorderCaps>();
        CHECK_AND_RETURN_RET_LOG(videoRecorderCaps != nullptr, videoRecorderCapsArray, "Is null mem");
        videoRecorderCaps->containerFormatType = profileData.containerFormatType;
        videoRecorderCaps->audioEncoderMime = profileData.audioEncoderMime;
        videoRecorderCaps->audioBitrateRange = profileData.audioBitrateRange;
        videoRecorderCaps->audioSampleRates = profileData.audioSampleRates;
        videoRecorderCaps->audioChannelRange = profileData.audioChannelRange;
        videoRecorderCaps->videoBitrateRange = profileData.videoBitrateRange;
        videoRecorderCaps->videoFramerateRange = profileData.videoFramerateRange;
        videoRecorderCaps->videoEncoderMime = profileData.videoEncoderMime;
        videoRecorderCaps->videoWidthRange = profileData.videoWidthRange;
        videoRecorderCaps->videoHeightRange = profileData.videoHeightRange;
        videoRecorderCapsArray.push_back(videoRecorderCaps);
    }
    return videoRecorderCapsArray;
}

std::shared_ptr<VideoRecorderProfile> RecorderProfilesImpl::GetVideoRecorderProfile(int32_t sourceId,
    int32_t qualityLevel)
{
    RecorderProfilesData capability = recorderProfilesService_->GetVideoRecorderProfileInfo(sourceId, qualityLevel);
    std::shared_ptr<VideoRecorderProfile> videoRecorderProfile = std::make_shared<VideoRecorderProfile>();
    CHECK_AND_RETURN_RET_LOG(videoRecorderProfile != nullptr, videoRecorderProfile, "Is null mem");
    videoRecorderProfile->containerFormatType = capability.containerFormatType;
    videoRecorderProfile->audioCodec = capability.audioCodec;
    videoRecorderProfile->videoCodec = capability.videoCodec;
    videoRecorderProfile->audioBitrate = capability.audioBitrate;
    videoRecorderProfile->audioChannels = capability.audioChannels;
    videoRecorderProfile->audioSampleRate = capability.audioSampleRate;
    videoRecorderProfile->durationTime = capability.durationTime;
    videoRecorderProfile->qualityLevel = capability.qualityLevel;
    videoRecorderProfile->videoBitrate = capability.videoBitrate;
    videoRecorderProfile->videoFrameWidth = capability.videoFrameWidth;
    videoRecorderProfile->videoFrameHeight = capability.videoFrameHeight;
    videoRecorderProfile->videoFrameRate = capability.videoFrameRate;
    return videoRecorderProfile;
}

}  // namespace Media
}  // namespace OHOS