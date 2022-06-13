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

#include "hdi_init.h"
#include <hdf_base.h>
#include "media_log.h"
#include "media_errors.h"
#include "display_type.h"
#include "scope_guard.h"
#include "hdi_codec_util.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HdiInit"};
    using namespace OHOS::Media;
    const std::unordered_map<int32_t, int32_t> AVC_PROFILE_MAP = {
        {OMX_VIDEO_AVCProfileBaseline, AVC_PROFILE_BASELINE},
        {OMX_VIDEO_AVCProfileMain, AVC_PROFILE_MAIN},
        {OMX_VIDEO_AVCProfileHigh, AVC_PROFILE_HIGH},
        {OMX_VIDEO_AVCProfileExtended, AVC_PROFILE_EXTENDED},
    };
    const std::unordered_map<int32_t, int32_t> AVC_LEVEL_MAP = {
        {OMX_VIDEO_AVCLevel1, AVC_LEVEL_1},
        {OMX_VIDEO_AVCLevel1b, AVC_LEVEL_1b},
        {OMX_VIDEO_AVCLevel11, AVC_LEVEL_11},
        {OMX_VIDEO_AVCLevel12, AVC_LEVEL_12},
        {OMX_VIDEO_AVCLevel13, AVC_LEVEL_13},
        {OMX_VIDEO_AVCLevel2, AVC_LEVEL_2},
        {OMX_VIDEO_AVCLevel21, AVC_LEVEL_21},
        {OMX_VIDEO_AVCLevel22, AVC_LEVEL_22},
        {OMX_VIDEO_AVCLevel3, AVC_LEVEL_3},
        {OMX_VIDEO_AVCLevel31, AVC_LEVEL_31},
        {OMX_VIDEO_AVCLevel32, AVC_LEVEL_32},
        {OMX_VIDEO_AVCLevel4, AVC_LEVEL_4},
        {OMX_VIDEO_AVCLevel41, AVC_LEVEL_41},
        {OMX_VIDEO_AVCLevel42, AVC_LEVEL_42},
        {OMX_VIDEO_AVCLevel5, AVC_LEVEL_5},
        {OMX_VIDEO_AVCLevel51, AVC_LEVEL_51},
    };
}

namespace OHOS {
namespace Media {
const std::unordered_map<int32_t, HdiInit::GetProfileLevelsFunc> HdiInit::PROFILE_LEVEL_FUNC_MAP = {
    {MEDIA_ROLETYPE_VIDEO_AVC, HdiInit::GetH264ProfileLevels},
};

HdiInit &HdiInit::GetInstance()
{
    static HdiInit omx;
    return omx;
}

HdiInit::HdiInit()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    mgr_ = GetCodecComponentManager();
}

HdiInit::~HdiInit()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    CodecComponentManagerRelease();
}

int32_t HdiInit::GetCodecType(CodecType hdiType)
{
    switch (hdiType) {
        case VIDEO_DECODER:
            return AVCODEC_TYPE_VIDEO_DECODER;
        case VIDEO_ENCODER:
            return AVCODEC_TYPE_VIDEO_ENCODER;
        case AUDIO_DECODER:
            return AVCODEC_TYPE_AUDIO_DECODER;
        case AUDIO_ENCODER:
            return AVCODEC_TYPE_AUDIO_ENCODER;
        default:
            MEDIA_LOGW("Unknow codecType");
            break;
    }
    return AVCODEC_TYPE_NONE;
}

std::string HdiInit::GetCodecMime(AvCodecRole &role)
{
    switch (role) {
        case MEDIA_ROLETYPE_VIDEO_AVC:
            return "video/avc";
        case MEDIA_ROLETYPE_VIDEO_HEVC:
            return "video/hevc";
        default:
            MEDIA_LOGW("Unknow codecRole");
            break;
    }
    return "invalid";
}

std::vector<int32_t> HdiInit::GetOmxFormats(VideoPortCap &port)
{
    int32_t index = 0;
    std::vector<int32_t> formats;
    while (index < PIX_FORMAT_NUM) {
        switch (port.supportPixFmts[index]) {
            case OMX_COLOR_FormatYUV420SemiPlanar:
                formats.push_back(NV12);
                break;
            case OMX_COLOR_FormatYUV422SemiPlanar:
                formats.push_back(NV16);
                break;
            default:
                MEDIA_LOGW("Unknow Format %{public}d", port.supportPixFmts[index]);
                return formats;
        }
        index++;
    }

    return formats;
}

std::vector<int32_t> HdiInit::GetCodecFormats(VideoPortCap &port)
{
    int32_t index = 0;
    std::vector<int32_t> formats;
    while (index < PIX_FORMAT_NUM) {
        switch (port.supportPixFmts[index]) {
            case PIXEL_FMT_YCBCR_420_SP:
                formats.push_back(NV12);
                break;
            case PIXEL_FMT_YCRCB_420_SP:
                formats.push_back(NV21);
                break;
            case PIXEL_FMT_YCBCR_422_SP:
                formats.push_back(NV16);
                break;
            default:
                MEDIA_LOGW("Unknow Format %{public}d", port.supportPixFmts[index]);
                return formats;
        }
        index++;
    }

    return formats;
}

std::map<int32_t, std::vector<int32_t>> HdiInit::GetH264ProfileLevels(CodecCompCapability &hdiCap)
{
    std::map<int32_t, std::vector<int32_t>> profileLevelsMap;
    int32_t index = 0;
    std::vector<int32_t> formats;
    while (index < PROFILE_NUM) {
        if (AVC_PROFILE_MAP.find(hdiCap.supportProfiles[index]) == AVC_PROFILE_MAP.end()) {
            MEDIA_LOGW("Unknow profile %{public}d", hdiCap.supportProfiles[index]);
            break;
        }
        int32_t profile = AVC_PROFILE_MAP.at(hdiCap.supportProfiles[index]);
        if (profileLevelsMap.find(profile) == profileLevelsMap.end()) {
            profileLevelsMap[profile] = std::vector<int32_t>();
        }
        index++;
        if (AVC_LEVEL_MAP.find(hdiCap.supportProfiles[index]) == AVC_LEVEL_MAP.end()) {
            MEDIA_LOGW("Unknow level %{public}d", hdiCap.supportProfiles[index]);
            break;
        }
        profileLevelsMap[profile].push_back(AVC_LEVEL_MAP.at(hdiCap.supportProfiles[index]));
        index++;
    }

    return profileLevelsMap;
}

std::map<int32_t, std::vector<int32_t>> HdiInit::GetCodecProfileLevels(CodecCompCapability &hdiCap)
{
    if (PROFILE_LEVEL_FUNC_MAP.find(hdiCap.role) == PROFILE_LEVEL_FUNC_MAP.end()) {
        MEDIA_LOGW("Unknow role %{public}d", hdiCap.role);
        return std::map<int32_t, std::vector<int32_t>>();
    }
    return PROFILE_LEVEL_FUNC_MAP.at(hdiCap.role)(hdiCap);
}

void HdiInit::AddHdiCap(CodecCompCapability &hdiCap)
{
    CapabilityData codecCap;
    codecCap.codecName = hdiCap.compName;
    codecCap.codecType = GetCodecType(hdiCap.type);
    codecCap.mimeType = GetCodecMime(hdiCap.role);
    codecCap.isVendor = !hdiCap.isSoftwareCodec;
    codecCap.alignment = {hdiCap.port.video.whAlignment.widthAlignment, hdiCap.port.video.whAlignment.heightAlignment};
    codecCap.bitrateMode = {1}; // need to do
    codecCap.width = {hdiCap.port.video.minSize.width, hdiCap.port.video.maxSize.width};
    codecCap.height = {hdiCap.port.video.minSize.height, hdiCap.port.video.maxSize.height};
    codecCap.bitrate = {hdiCap.bitRate.min, hdiCap.bitRate.max};
    codecCap.frameRate = {0, 60}; //need to do
    codecCap.format = GetOmxFormats(hdiCap.port.video); // need to do
    codecCap.blockPerFrame = {hdiCap.port.video.blockCount.min, hdiCap.port.video.blockCount.max}; //need to check
    codecCap.blockPerSecond = {hdiCap.port.video.blocksPerSecond.min, hdiCap.port.video.blocksPerSecond.max};
    codecCap.blockSize = {hdiCap.port.video.blockSize.width, hdiCap.port.video.blockSize.height};
    codecCap.measuredFrameRate = std::map<ImgSize, Range>(); //need to do
    codecCap.profileLevelsMap = GetCodecProfileLevels(hdiCap);
    capabilitys_.push_back(codecCap);
}

void HdiInit::InitCaps()
{
    if (!capabilitys_.empty()) {
        return;
    }
    auto len = mgr_->GetComponentNum();
    CodecCompCapability *hdiCaps = new CodecCompCapability[len];
    CHECK_AND_RETURN_LOG(hdiCaps != nullptr, "New CodecCompCapability fail");
    ON_SCOPE_EXIT(0) { delete[] hdiCaps; };
    auto ret = mgr_->GetComponentCapabilityList(hdiCaps, len);
    CHECK_AND_RETURN_LOG(ret == HDF_SUCCESS, "GetComponentCapabilityList fail");
    for (auto i = 0; i < len; ++i) {
        AddHdiCap(hdiCaps[i]);
    }
}

std::vector<CapabilityData> HdiInit::GetCapabilitys()
{
    CHECK_AND_RETURN_RET_LOG(mgr_ != nullptr, capabilitys_, "mgr is nullptr");
    InitCaps();
    return capabilitys_;
}

int32_t HdiInit::GetHandle(CodecComponentType **component, uint32_t &id, std::string name,
    void *appData, CodecCallbackType *callbacks)
{
    CHECK_AND_RETURN_RET_LOG(mgr_ != nullptr, HDF_FAILURE, "mgr is nullptr");
    CHECK_AND_RETURN_RET_LOG(component != nullptr, HDF_FAILURE, "component is nullptr");
    return mgr_->CreateComponent(component, &id, const_cast<char *>(name.c_str()),
        reinterpret_cast<int64_t>(appData), callbacks);
}

int32_t HdiInit::FreeHandle(uint32_t id)
{
    CHECK_AND_RETURN_RET_LOG(mgr_ != nullptr, HDF_FAILURE, "mgr_ is nullptr");
    return mgr_->DestoryComponent(id);
}
}  // namespace Media
}  // namespace OHOS
