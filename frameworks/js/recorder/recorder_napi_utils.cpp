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

#include "recorder_napi_utils.h"
#include <map>
#include <string>
#include "tokenid_kit.h"
#include "ipc_skeleton.h"
#include "access_token.h"
#include "accesstoken_kit.h"
#include "media_errors.h"

using namespace OHOS::Security::AccessToken;

namespace OHOS {
namespace Media {
using namespace MediaAVCodec;
const std::map<std::string_view, int32_t> g_mimeStrToCodecFormat = {
    { CodecMimeType::AUDIO_AAC, AudioCodecFormat::AAC_LC },
    { CodecMimeType::VIDEO_AVC, VideoCodecFormat::H264 },
    { CodecMimeType::VIDEO_MPEG4, VideoCodecFormat::MPEG4 },
};

const std::map<std::string, OutputFormatType> g_extensionToOutputFormat = {
    { "mp4", OutputFormatType::FORMAT_MPEG_4 },
    { "m4a", OutputFormatType::FORMAT_M4A },
};

bool IsSystemApp()
{
    uint64_t accessTokenIDEx = IPCSkeleton::GetCallingFullTokenID();
    bool isSystemApp = TokenIdKit::IsSystemAppByFullTokenID(accessTokenIDEx);
    return isSystemApp;
}

bool SystemPermission()
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto tokenType = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (tokenType == Security::AccessToken::TOKEN_NATIVE || tokenType == Security::AccessToken::TOKEN_SHELL) {
        return true;
    }
    return IsSystemApp();
}

int32_t MapMimeToAudioCodecFormat(const std::string &mime, AudioCodecFormat &codecFormat)
{
    auto iter = g_mimeStrToCodecFormat.find(mime);
    if (iter != g_mimeStrToCodecFormat.end()) {
        codecFormat = static_cast<AudioCodecFormat>(iter->second);
    }
    return MSERR_INVALID_VAL;
}

int32_t MapMimeToVideoCodecFormat(const std::string &mime, VideoCodecFormat &codecFormat)
{
    auto iter = g_mimeStrToCodecFormat.find(mime);
    if (iter != g_mimeStrToCodecFormat.end()) {
        codecFormat = static_cast<VideoCodecFormat>(iter->second);
    }
    return MSERR_INVALID_VAL;
}

int32_t MapExtensionNameToOutputFormat(const std::string &extension, OutputFormatType &type)
{
    auto iter = g_extensionToOutputFormat.find(extension);
    if (iter != g_extensionToOutputFormat.end()) {
        type = iter->second;
    }
    return MSERR_INVALID_VAL;
}
} // namespace Media
} // namespace OHOS