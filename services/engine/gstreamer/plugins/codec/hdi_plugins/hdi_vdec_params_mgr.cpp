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

#include "hdi_vdec_params_mgr.h"
#include <hdf_base.h>
#include "media_log.h"
#include "media_errors.h"
#include "gst_vdec_base.h"
#include "hdi_codec_util.h"
#include "hdi_codec.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HdiVdecParamsMgr"};
    constexpr uint32_t HDI_FRAME_RATE_MOVE = 16; // hdi frame rate need move 16
}

namespace OHOS {
namespace Media {
HdiVdecParamsMgr::HdiVdecParamsMgr()
{
    InitParam(inPortDef_);
    InitParam(outPortDef_);
    InitParam(videoFormat_);
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

HdiVdecParamsMgr::~HdiVdecParamsMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void HdiVdecParamsMgr::Init(CodecComponentType *handle, const OMX_PORT_PARAM_TYPE &portParam)
{
    handle_ = handle;
    inPortDef_.nPortIndex = portParam.nStartPortNumber;
    outPortDef_.nPortIndex = portParam.nStartPortNumber + 1;
    videoFormat_.nPortIndex = portParam.nStartPortNumber + 1;
}

int32_t HdiVdecParamsMgr::SetParameter(GstCodecParamKey key, GstElement *element)
{
    CHECK_AND_RETURN_RET_LOG(element != nullptr, GST_CODEC_ERROR, "Element is nullptr");
    switch (key) {
        case GST_VIDEO_INPUT_COMMON:
            SetInputVideoCommon(element);
            break;
        case GST_VIDEO_OUTPUT_COMMON:
            SetOutputVideoCommon(element);
            break;
        case GST_VIDEO_FORMAT:
            SetVideoFormat(element);
            break;
        case GST_VIDEO_SURFACE_INIT:
            VideoSurfaceInit(element);
            break;
        case GST_VENDOR:
            MEDIA_LOGD("Set vendor property");
            break;
        default:
            break;
    }
    return GST_CODEC_OK;
}

int32_t HdiVdecParamsMgr::SetInputVideoCommon(GstElement *element)
{
    GstVdecBase *base = GST_VDEC_BASE(element);
    inPortDef_.format.video.eCompressionFormat = HdiCodecUtil::CompressionGstToHdi(base->compress_format);
    inPortDef_.format.video.nFrameHeight = (uint32_t)base->height;
    inPortDef_.format.video.nFrameWidth = (uint32_t)base->width;
    inPortDef_.format.video.xFramerate = (uint32_t)(base->frame_rate) << HDI_FRAME_RATE_MOVE;
    inPortDef_.nBufferCountActual = base->input.buffer_cnt;
    MEDIA_LOGD("frame_rate %{public}d", base->frame_rate);
    auto ret = HdiSetParameter(handle_, OMX_IndexParamPortDefinition, inPortDef_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetParameter failed");
    return GST_CODEC_OK;
}

int32_t HdiVdecParamsMgr::SetOutputVideoCommon(GstElement *element)
{
    GstVdecBase *base = GST_VDEC_BASE(element);
    outPortDef_.format.video.nFrameHeight = (uint32_t)base->height;
    outPortDef_.format.video.nFrameWidth = (uint32_t)base->width;
    outPortDef_.format.video.xFramerate = (uint32_t)(base->frame_rate) << HDI_FRAME_RATE_MOVE;
    outPortDef_.nBufferCountActual = base->output.buffer_cnt;
    MEDIA_LOGD("frame_rate %{public}d", base->frame_rate);
    auto ret = HdiSetParameter(handle_, OMX_IndexParamPortDefinition, outPortDef_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetParameter failed");
    return GST_CODEC_OK;
}

int32_t HdiVdecParamsMgr::SetVideoFormat(GstElement *element)
{
    GstVdecBase *base = GST_VDEC_BASE(element);
    videoFormat_.eColorFormat = (OMX_COLOR_FORMATTYPE)HdiCodecUtil::FormatGstToHdi(base->format); // need to do
    MEDIA_LOGD("videoFormat_.eColorFormat %{public}d", videoFormat_.eColorFormat);
    auto ret = HdiSetParameter(handle_, OMX_IndexParamVideoPortFormat, videoFormat_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetParameter failed");
    return GST_CODEC_OK;
}

int32_t HdiVdecParamsMgr::GetInputVideoCommon(GstElement *element)
{
    GstVdecBase *base = GST_VDEC_BASE(element);
    auto ret = HdiGetParameter(handle_, OMX_IndexParamPortDefinition, inPortDef_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiGetParameter failed");
    base->input.min_buffer_cnt = inPortDef_.nBufferCountMin;
    base->input.buffer_cnt = inPortDef_.nBufferCountActual;
    base->input.buffer_size = inPortDef_.nBufferSize;
    base->input.height = (int32_t)inPortDef_.format.video.nFrameHeight;
    base->input.width = (int32_t)inPortDef_.format.video.nFrameWidth;
    base->input.frame_rate = (int32_t)inPortDef_.format.video.xFramerate;
    return GST_CODEC_OK;
}

int32_t HdiVdecParamsMgr::GetOutputVideoCommon(GstElement *element)
{
    GstVdecBase *base = GST_VDEC_BASE(element);
    auto ret = HdiGetParameter(handle_, OMX_IndexParamPortDefinition, outPortDef_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiGetParameter failed");
    base->output.min_buffer_cnt = outPortDef_.nBufferCountMin;
    base->output.buffer_cnt = outPortDef_.nBufferCountActual;
    base->output.buffer_size = outPortDef_.nBufferSize;
    base->output.height = (int32_t)outPortDef_.format.video.nFrameHeight;
    base->output.width = (int32_t)outPortDef_.format.video.nFrameWidth;
    base->output.frame_rate = (int32_t)outPortDef_.format.video.xFramerate;

    OMX_CONFIG_RECTTYPE rect;
    InitParam(rect);
    rect.nPortIndex = outPortDef_.nPortIndex;
    if (HdiSetConfig(handle_, OMX_IndexConfigCommonOutputCrop, rect) == HDF_SUCCESS) {
        MEDIA_LOGD("frame nStride %{public}d nSliceHeight %{public}d",
            outPortDef_.format.video.nStride, outPortDef_.format.video.nSliceHeight);
        MEDIA_LOGD("rect left %{public}d top %{public}d width %{public}d height %{public}d",
            rect.nLeft, rect.nTop, rect.nWidth, rect.nHeight);
        base->stride = outPortDef_.format.video.nStride;
        base->stride_height = (int32_t)outPortDef_.format.video.nSliceHeight;
        base->rect.x = rect.nLeft;
        base->rect.y = rect.nTop;
        base->rect.width = rect.nWidth;
        base->rect.height = rect.nHeight;
    }

    return GST_CODEC_OK;
}

int32_t HdiVdecParamsMgr::GetVideoFormat(GstElement *element)
{
    GstVdecBase *base = GST_VDEC_BASE(element);
    auto ret = HdiGetParameter(handle_, OMX_IndexParamVideoPortFormat, videoFormat_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiGetParameter failed");

    while (HdiGetParameter(handle_, OMX_IndexParamVideoPortFormat, videoFormat_) == HDF_SUCCESS) {
        base->formats.push_back(HdiCodecUtil::FormatHdiToGst((PixelFormat)videoFormat_.eColorFormat)); // need to do
        videoFormat_.nIndex++;
    }
    return GST_CODEC_OK;
}

int32_t HdiVdecParamsMgr::VideoSurfaceInit(GstElement *element)
{
    (void)element;
    auto ret = HdiSetParameter(handle_, OMX_IndexParamVideoPortFormat, outPortDef_); // need to do
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetParameter failed");
    return GST_CODEC_OK;
}

int32_t HdiVdecParamsMgr::GetParameter(GstCodecParamKey key, GstElement *element)
{
    CHECK_AND_RETURN_RET_LOG(element != nullptr, GST_CODEC_ERROR, "Element is nullptr");
    CHECK_AND_RETURN_RET_LOG(handle_ != nullptr, GST_CODEC_ERROR, "Handle is nullptr");
    switch (key) {
        case GST_VIDEO_INPUT_COMMON:
            GetInputVideoCommon(element);
            break;
        case GST_VIDEO_OUTPUT_COMMON:
            GetOutputVideoCommon(element);
            break;
        case GST_VIDEO_FORMAT:
            GetVideoFormat(element);
            break;
        default:
            break;
    }
    return GST_CODEC_OK;
}
}  // namespace Media
}  // namespace OHOS
