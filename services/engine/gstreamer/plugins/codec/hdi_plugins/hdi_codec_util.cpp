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

#include "hdi_codec_util.h"
#include <unordered_map>
#include "display_type.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HdiCodecUtil"};
}

namespace OHOS {
namespace Media {
static const std::unordered_map<GstCompressionFormat, OMX_VIDEO_CODINGTYPE> COMPRESS_GST_OMX = {
    {GST_AVC, OMX_VIDEO_CodingAVC},
    {GST_HEVC, static_cast<OMX_VIDEO_CODINGTYPE>(CODEC_OMX_VIDEO_CodingHEVC)}
};

static const std::unordered_map<GstVideoFormat, PixelFormat> FORMAT_GST_HDI = {
    {GST_VIDEO_FORMAT_NV12, PIXEL_FMT_YCBCR_420_SP},
    {GST_VIDEO_FORMAT_NV21, PIXEL_FMT_YCRCB_420_SP},
    {GST_VIDEO_FORMAT_RGBA, PIXEL_FMT_RGBA_8888}
};

static const std::unordered_map<PixelFormat, GstVideoFormat> FORMAT_HDI_GST = {
    {PIXEL_FMT_YCBCR_420_SP, GST_VIDEO_FORMAT_NV12},
    {PIXEL_FMT_YCRCB_420_SP, GST_VIDEO_FORMAT_NV21},
    {PIXEL_FMT_RGBA_8888, GST_VIDEO_FORMAT_RGBA}
};

OMX_VIDEO_CODINGTYPE HdiCodecUtil::CompressionGstToHdi(GstCompressionFormat format)
{
    OMX_VIDEO_CODINGTYPE ret = OMX_VIDEO_CodingUnused;
    if (COMPRESS_GST_OMX.find(format) != COMPRESS_GST_OMX.end()) {
        ret = COMPRESS_GST_OMX.at(format);
    }
    return ret;
}

PixelFormat HdiCodecUtil::FormatGstToHdi(GstVideoFormat format)
{
    PixelFormat ret = PIXEL_FMT_BUTT;
    if (FORMAT_GST_HDI.find(format) != FORMAT_GST_HDI.end()) {
        ret = FORMAT_GST_HDI.at(format);
    }
    MEDIA_LOGD("GstFormat %{public}d", format);
    return ret;
}

GstVideoFormat HdiCodecUtil::FormatHdiToGst(PixelFormat format)
{
    GstVideoFormat ret = GST_VIDEO_FORMAT_UNKNOWN;
    if (FORMAT_HDI_GST.find(format) != FORMAT_HDI_GST.end()) {
        ret = FORMAT_HDI_GST.at(format);
    }
    MEDIA_LOGI("PixelFormat %{public}d", format);
    return ret;
}
}  // namespace Media
}  // namespace OHOS
