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

#include "media_errors.h"
#include <map>
#include <string>

namespace OHOS {
namespace Media {
using ErrorMessageFunc = std::function<std::string(const std::string& param1, const std::string& param2)>;
const std::map<MediaServiceErrCode, std::string> MSERRCODE_INFOS = {
    {MSERR_OK, "success"},
    {MSERR_NO_MEMORY, "no memory"},
    {MSERR_INVALID_OPERATION, "operation not be permitted"},
    {MSERR_INVALID_VAL, "invalid argument"},
    {MSERR_UNKNOWN, "unkown error"},
    {MSERR_SERVICE_DIED, "media service died"},
    {MSERR_CREATE_REC_ENGINE_FAILED, "create recorder engine failed"},
    {MSERR_CREATE_PLAYER_ENGINE_FAILED, "create player engine failed"},
    {MSERR_CREATE_AVMETADATAHELPER_ENGINE_FAILED, "create avmetadatahelper engine failed"},
    {MSERR_INVALID_STATE, "the state is not support this operation"},
    {MSERR_UNSUPPORT, "unsupport interface"},
    {MSERR_UNSUPPORT_AUD_SRC_TYPE, "unsupport audio source type"},
    {MSERR_UNSUPPORT_AUD_SAMPLE_RATE, "unsupport audio sample rate"},
    {MSERR_UNSUPPORT_AUD_CHANNEL_NUM, "unsupport audio channel"},
    {MSERR_UNSUPPORT_AUD_ENC_TYPE, "unsupport audio encoder type"},
    {MSERR_UNSUPPORT_AUD_PARAMS, "unsupport audio params(other params)"},
    {MSERR_UNSUPPORT_VID_SRC_TYPE, "unsupport video source type"},
    {MSERR_UNSUPPORT_VID_ENC_TYPE, "unsupport video encoder type"},
    {MSERR_UNSUPPORT_VID_PARAMS, "unsupport video params(other params)"},
    {MSERR_UNSUPPORT_CONTAINER_TYPE, "unsupport container format type"},
    {MSERR_UNSUPPORT_PROTOCOL_TYPE, "unsupport protocol type"},
    {MSERR_UNSUPPORT_VID_DEC_TYPE, "unsupport video decoder type"},
    {MSERR_UNSUPPORT_AUD_DEC_TYPE, "unsupport audio decoder type"},
    {MSERR_UNSUPPORT_STREAM, "internal data stream error"},
    {MSERR_UNSUPPORT_FILE, "this appears to be a text file"},
    {MSERR_UNSUPPORT_SOURCE, "unsupport source type"},
    {MSERR_AUD_ENC_FAILED, "audio encode failed"},
    {MSERR_AUD_RENDER_FAILED, "audio render failed"},
    {MSERR_VID_ENC_FAILED, "video encode failed"},
    {MSERR_AUD_DEC_FAILED, "audio decode failed"},
    {MSERR_VID_DEC_FAILED, "video decode failed"},
    {MSERR_MUXER_FAILED, "stream avmuxer failed"},
    {MSERR_DEMUXER_FAILED, "stream demuxer or parser failed"},
    {MSERR_OPEN_FILE_FAILED, "open file failed"},
    {MSERR_FILE_ACCESS_FAILED, "read or write file failed"},
    {MSERR_START_FAILED, "audio or video start failed"},
    {MSERR_PAUSE_FAILED, "audio or video pause failed"},
    {MSERR_STOP_FAILED, "audio or video stop failed"},
    {MSERR_SEEK_FAILED, "audio or video seek failed"},
    {MSERR_NETWORK_TIMEOUT, "network timeout"},
    {MSERR_NOT_FIND_CONTAINER, "not find a demuxer"},
    {MSERR_EXTEND_START, "extend start error code"},
};

const std::map<MediaServiceErrCode, MediaServiceExtErrCode> MSERRCODE_TO_EXTERRORCODE = {
    {MSERR_OK,                                  MSERR_EXT_OK},
    {MSERR_NO_MEMORY,                           MSERR_EXT_NO_MEMORY},
    {MSERR_INVALID_OPERATION,                   MSERR_EXT_OPERATE_NOT_PERMIT},
    {MSERR_INVALID_VAL,                         MSERR_EXT_INVALID_VAL},
    {MSERR_UNKNOWN,                             MSERR_EXT_UNKNOWN},
    {MSERR_SERVICE_DIED,                        MSERR_EXT_SERVICE_DIED},
    {MSERR_CREATE_REC_ENGINE_FAILED,            MSERR_EXT_UNKNOWN},
    {MSERR_CREATE_PLAYER_ENGINE_FAILED,         MSERR_EXT_UNKNOWN},
    {MSERR_INVALID_STATE,                       MSERR_EXT_INVALID_STATE},
    {MSERR_UNSUPPORT,                           MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_AUD_SRC_TYPE,              MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_AUD_SAMPLE_RATE,           MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_AUD_CHANNEL_NUM,           MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_AUD_ENC_TYPE,              MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_AUD_PARAMS,                MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_VID_SRC_TYPE,              MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_VID_ENC_TYPE,              MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_VID_PARAMS,                MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_CONTAINER_TYPE,            MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_PROTOCOL_TYPE,             MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_VID_DEC_TYPE,              MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_AUD_DEC_TYPE,              MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_STREAM,                    MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_FILE,                      MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_SOURCE,                    MSERR_EXT_UNSUPPORT},
    {MSERR_AUD_RENDER_FAILED,                   MSERR_EXT_UNSUPPORT},
    {MSERR_AUD_ENC_FAILED,                      MSERR_EXT_UNKNOWN},
    {MSERR_VID_ENC_FAILED,                      MSERR_EXT_UNKNOWN},
    {MSERR_AUD_DEC_FAILED,                      MSERR_EXT_UNKNOWN},
    {MSERR_VID_DEC_FAILED,                      MSERR_EXT_UNKNOWN},
    {MSERR_MUXER_FAILED,                        MSERR_EXT_UNKNOWN},
    {MSERR_DEMUXER_FAILED,                      MSERR_EXT_UNKNOWN},
    {MSERR_OPEN_FILE_FAILED,                    MSERR_EXT_UNKNOWN},
    {MSERR_FILE_ACCESS_FAILED,                  MSERR_EXT_UNKNOWN},
    {MSERR_START_FAILED,                        MSERR_EXT_UNKNOWN},
    {MSERR_PAUSE_FAILED,                        MSERR_EXT_UNKNOWN},
    {MSERR_STOP_FAILED,                         MSERR_EXT_UNKNOWN},
    {MSERR_SEEK_FAILED,                         MSERR_EXT_UNKNOWN},
    {MSERR_NETWORK_TIMEOUT,                     MSERR_EXT_TIMEOUT},
    {MSERR_NOT_FIND_CONTAINER,                  MSERR_EXT_UNSUPPORT},
    {MSERR_EXTEND_START,                        MSERR_EXT_EXTEND_START},
};

const std::map<MediaServiceExtErrCode, std::string> MSEXTERRCODE_INFOS = {
    {MSERR_EXT_OK, "success"},
    {MSERR_EXT_NO_MEMORY, "no memory"},
    {MSERR_EXT_OPERATE_NOT_PERMIT, "operation not be permitted"},
    {MSERR_EXT_INVALID_VAL, "invalid argument"},
    {MSERR_EXT_IO, "IO error"},
    {MSERR_EXT_TIMEOUT, "network timeout"},
    {MSERR_EXT_UNKNOWN, "unkown error"},
    {MSERR_EXT_SERVICE_DIED, "media service died"},
    {MSERR_EXT_INVALID_STATE, "the state is not support this operation"},
    {MSERR_EXT_UNSUPPORT, "unsupport interface"},
    {MSERR_EXT_EXTEND_START, "extend err start"},
};

const std::map<MediaServiceErrCode, MediaServiceExtErrCodeAPI9> MSERRCODE_TO_EXTERRORCODEAPI9 = {
    {MSERR_OK,                                  MSERR_EXT_API9_OK},
    {MSERR_NO_MEMORY,                           MSERR_EXT_API9_NO_MEMORY},
    {MSERR_INVALID_OPERATION,                   MSERR_EXT_API9_OPERATE_NOT_PERMIT},
    {MSERR_INVALID_VAL,                         MSERR_EXT_API9_INVALID_PARAMETER},
    {MSERR_SERVICE_DIED,                        MSERR_EXT_API9_SERVICE_DIED},
    {MSERR_CREATE_REC_ENGINE_FAILED,            MSERR_EXT_API9_NO_MEMORY},
    {MSERR_CREATE_PLAYER_ENGINE_FAILED,         MSERR_EXT_API9_NO_MEMORY},
    {MSERR_INVALID_STATE,                       MSERR_EXT_API9_OPERATE_NOT_PERMIT},
    {MSERR_UNSUPPORT,                           MSERR_EXT_API9_UNSUPPORT_FORMAT},
    {MSERR_UNSUPPORT_AUD_SRC_TYPE,              MSERR_EXT_API9_UNSUPPORT_FORMAT},
    {MSERR_UNSUPPORT_AUD_SAMPLE_RATE,           MSERR_EXT_API9_UNSUPPORT_FORMAT},
    {MSERR_UNSUPPORT_AUD_CHANNEL_NUM,           MSERR_EXT_API9_UNSUPPORT_FORMAT},
    {MSERR_UNSUPPORT_AUD_ENC_TYPE,              MSERR_EXT_API9_UNSUPPORT_FORMAT},
    {MSERR_UNSUPPORT_AUD_PARAMS,                MSERR_EXT_API9_UNSUPPORT_FORMAT},
    {MSERR_UNSUPPORT_VID_SRC_TYPE,              MSERR_EXT_API9_UNSUPPORT_FORMAT},
    {MSERR_UNSUPPORT_VID_ENC_TYPE,              MSERR_EXT_API9_UNSUPPORT_FORMAT},
    {MSERR_UNSUPPORT_VID_PARAMS,                MSERR_EXT_API9_UNSUPPORT_FORMAT},
    {MSERR_UNSUPPORT_CONTAINER_TYPE,            MSERR_EXT_API9_UNSUPPORT_FORMAT},
    {MSERR_UNSUPPORT_PROTOCOL_TYPE,             MSERR_EXT_API9_UNSUPPORT_FORMAT},
    {MSERR_UNSUPPORT_VID_DEC_TYPE,              MSERR_EXT_API9_UNSUPPORT_FORMAT},
    {MSERR_UNSUPPORT_AUD_DEC_TYPE,              MSERR_EXT_API9_UNSUPPORT_FORMAT},
    {MSERR_UNSUPPORT_STREAM,                    MSERR_EXT_API9_UNSUPPORT_FORMAT},
    {MSERR_UNSUPPORT_FILE,                      MSERR_EXT_API9_UNSUPPORT_FORMAT},
    {MSERR_UNSUPPORT_SOURCE,                    MSERR_EXT_API9_UNSUPPORT_FORMAT},
    {MSERR_AUD_RENDER_FAILED,                   MSERR_EXT_API9_IO},
    {MSERR_AUD_ENC_FAILED,                      MSERR_EXT_API9_IO},
    {MSERR_VID_ENC_FAILED,                      MSERR_EXT_API9_IO},
    {MSERR_AUD_DEC_FAILED,                      MSERR_EXT_API9_IO},
    {MSERR_VID_DEC_FAILED,                      MSERR_EXT_API9_IO},
    {MSERR_MUXER_FAILED,                        MSERR_EXT_API9_UNSUPPORT_FORMAT},
    {MSERR_DEMUXER_FAILED,                      MSERR_EXT_API9_UNSUPPORT_FORMAT},
    {MSERR_OPEN_FILE_FAILED,                    MSERR_EXT_API9_IO},
    {MSERR_FILE_ACCESS_FAILED,                  MSERR_EXT_API9_IO},
    {MSERR_START_FAILED,                        MSERR_EXT_API9_OPERATE_NOT_PERMIT},
    {MSERR_PAUSE_FAILED,                        MSERR_EXT_API9_OPERATE_NOT_PERMIT},
    {MSERR_STOP_FAILED,                         MSERR_EXT_API9_OPERATE_NOT_PERMIT},
    {MSERR_SEEK_FAILED,                         MSERR_EXT_API9_OPERATE_NOT_PERMIT},
    {MSERR_NETWORK_TIMEOUT,                     MSERR_EXT_API9_TIMEOUT},
    {MSERR_NOT_FIND_CONTAINER,                  MSERR_EXT_API9_INVALID_PARAMETER},
    {MSERR_UNKNOWN,                             MSERR_EXT_API9_IO},
};

const std::map<MediaServiceExtErrCodeAPI9, std::string> MSEXTERRCODE_API9_INFOS = {
    {MSERR_EXT_API9_OK, "Success: "},
    {MSERR_EXT_API9_NO_PERMISSION, "No Permission: "},
    {MSERR_EXT_API9_PERMISSION_DENIED, "Permission Denied"},
    {MSERR_EXT_API9_INVALID_PARAMETER, "Invalid Parameter: "},
    {MSERR_EXT_API9_UNSUPPORT_CAPABILITY, "Unsupport Capability: "},
    {MSERR_EXT_API9_NO_MEMORY, "No Memory: "},
    {MSERR_EXT_API9_OPERATE_NOT_PERMIT, "Operate Not Permit: "},
    {MSERR_EXT_API9_IO, "IO Error: "},
    {MSERR_EXT_API9_TIMEOUT, "Network Timeout: "},
    {MSERR_EXT_API9_SERVICE_DIED, "Service Died: "},
    {MSERR_EXT_API9_UNSUPPORT_FORMAT, "Unsupport Format: "},
};

std::string ErrorMessageOk(const std::string& param1, const std::string& param2)
{
    (void)param1;
    (void)param2;
    return "success";
}

std::string ErrorMessageNoPermission(const std::string& param1, const std::string& param2)
{
    std::string message = "Try to do " + param1 + " failed. User should request permission " + param2 +" first.";
    return message;
}

std::string ErrorMessageInvalidParameter(const std::string& param1, const std::string& param2)
{
    (void)param2;
    std::string message = "The Parameter " + param1 + " is invalid. Please check the type and range.";
    return message;
}

std::string ErrorMessageUnsupportCapability(const std::string& param1, const std::string& param2)
{
    (void)param2;
    std::string message = "Function " + param1 + " can not work correctly due to limited device capability.";
    return message;
}

std::string ErrorMessageNoMemory(const std::string& param1, const std::string& param2)
{
    (void)param2;
    std::string message = "Create " + param1 + " failed due to system memory.";
    return message;
}

std::string ErrorMessageOperateNotPermit(const std::string& param1, const std::string& param2)
{
    (void)param2;
    std::string message = "The operate " + param1 + " failed due to not permit in current state.";
    return message;
}

std::string ErrorMessageIO(const std::string& param1, const std::string& param2)
{
    (void)param2;
    std::string message = "IO error happened due to " + param1 + ".";
    return message;
}

std::string ErrorMessageTimeout(const std::string& param1, const std::string& param2)
{
    std::string message = "Timeout happend when " + param1 + " due to " + param2 + ".";
    return message;
}

std::string ErrorMessageServiceDied(const std::string& param1, const std::string& param2)
{
    (void)param1;
    (void)param2;
    std::string message = "Media Serviced Died.";
    return message;
}

std::string ErrorMessageUnsupportFormat(const std::string& param1, const std::string& param2)
{
    (void)param2;
    std::string message = "The format " + param1 + " is not support.";
    return message;
}

const std::map<MediaServiceExtErrCodeAPI9, ErrorMessageFunc> MSEXTERRAPI9CODE_FUNCS = {
    {MSERR_EXT_API9_OK, &ErrorMessageOk},
    {MSERR_EXT_API9_NO_PERMISSION, &ErrorMessageNoPermission},
    {MSERR_EXT_API9_INVALID_PARAMETER, &ErrorMessageInvalidParameter},
    {MSERR_EXT_API9_UNSUPPORT_CAPABILITY, &ErrorMessageUnsupportCapability},
    {MSERR_EXT_API9_NO_MEMORY, &ErrorMessageNoMemory},
    {MSERR_EXT_API9_OPERATE_NOT_PERMIT, &ErrorMessageOperateNotPermit},
    {MSERR_EXT_API9_IO, &ErrorMessageIO},
    {MSERR_EXT_API9_TIMEOUT, &ErrorMessageTimeout},
    {MSERR_EXT_API9_SERVICE_DIED, &ErrorMessageServiceDied},
    {MSERR_EXT_API9_UNSUPPORT_FORMAT, &ErrorMessageUnsupportFormat},
};

std::string MSErrorToString(MediaServiceErrCode code)
{
    if (MSERRCODE_INFOS.count(code) != 0) {
        return MSERRCODE_INFOS.at(code);
    }

    if (code > MSERR_EXTEND_START) {
        return "extend error:" + std::to_string(static_cast<int32_t>(code - MSERR_EXTEND_START));
    }

    return "invalid error code:" + std::to_string(static_cast<int32_t>(code));
}

std::string MSExtErrorToString(MediaServiceExtErrCode code)
{
    if (MSEXTERRCODE_INFOS.count(code) != 0) {
        return MSEXTERRCODE_INFOS.at(code);
    }

    if (code > MSERR_EXT_EXTEND_START) {
        return "extend error:" + std::to_string(static_cast<int32_t>(code - MSERR_EXTEND_START));
    }

    return "invalid error code:" + std::to_string(static_cast<int32_t>(code));
}

std::string MSExtErrorAPI9ToString(MediaServiceExtErrCodeAPI9 code,
    const std::string& param1, const std::string& param2)
{
    if (MSEXTERRAPI9CODE_FUNCS.count(code) != 0) {
        return MSEXTERRAPI9CODE_FUNCS.at(code)(param1, param2);
    }

    return "invalid error code:" + std::to_string(static_cast<int32_t>(code));
}

std::string MSExtAVErrorToString(MediaServiceExtErrCodeAPI9 code)
{
    if (MSEXTERRCODE_API9_INFOS.count(code) != 0) {
        return MSEXTERRCODE_API9_INFOS.at(code);
    }

    return "invalid error code:";
}

std::string MSErrorToExtErrorString(MediaServiceErrCode code)
{
    if (MSERRCODE_INFOS.count(code) != 0 && MSERRCODE_TO_EXTERRORCODE.count(code) != 0) {
        MediaServiceExtErrCode extCode = MSERRCODE_TO_EXTERRORCODE.at(code);
        if (MSEXTERRCODE_INFOS.count(extCode) != 0) {
            return MSEXTERRCODE_INFOS.at(extCode);
        }
    }

    return "unkown error";
}

std::string MSErrorToExtErrorAPI9String(MediaServiceErrCode code, const std::string& param1, const std::string& param2)
{
    if (MSERRCODE_INFOS.count(code) != 0 && MSERRCODE_TO_EXTERRORCODEAPI9.count(code) != 0) {
        MediaServiceExtErrCodeAPI9 extCode = MSERRCODE_TO_EXTERRORCODEAPI9.at(code);
        if (MSEXTERRAPI9CODE_FUNCS.count(extCode) != 0) {
            return MSEXTERRAPI9CODE_FUNCS.at(extCode)(param1, param2);
        }
    }

    return "unkown error";
}

MediaServiceExtErrCode MSErrorToExtError(MediaServiceErrCode code)
{
    if (MSERRCODE_INFOS.count(code) != 0 && MSERRCODE_TO_EXTERRORCODE.count(code) != 0) {
        return MSERRCODE_TO_EXTERRORCODE.at(code);
    }

    return MSERR_EXT_UNKNOWN;
}

MediaServiceExtErrCodeAPI9 MSErrorToExtErrorAPI9(MediaServiceErrCode code)
{
    if (MSERRCODE_INFOS.count(code) != 0 && MSERRCODE_TO_EXTERRORCODEAPI9.count(code) != 0) {
        return MSERRCODE_TO_EXTERRORCODEAPI9.at(code);
    }
    // If error not in map, need add error and should not return default MSERR_EXT_API9_IO.
    return MSERR_EXT_API9_IO;
}
} // namespace Media
} // namespace OHOS
