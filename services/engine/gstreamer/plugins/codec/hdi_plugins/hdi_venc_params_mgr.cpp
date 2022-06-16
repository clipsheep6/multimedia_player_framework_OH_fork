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

#include "hdi_venc_params_mgr.h"
#include <hdf_base.h>
#include "media_log.h"
#include "media_errors.h"
#include "gst_venc_base.h"
#include "hdi_codec_util.h"
#include "hdi_codec.h"
namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HdiVencParamsMgr"};
    constexpr uint32_t OMX_FRAME_RATE_MOVE = 16; // hdi frame rate need move 16
}

namespace OHOS {
namespace Media {
HdiVencParamsMgr::HdiVencParamsMgr()
    : handle_(nullptr)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

HdiVencParamsMgr::~HdiVencParamsMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void HdiVencParamsMgr::Init(CodecComponentType *handle,
    const OMX_PORT_PARAM_TYPE &portParam, const CompVerInfo &verInfo)
{
    handle_ = handle;
    verInfo_ = verInfo;
    InitParam(inPortDef_, verInfo_);
    InitParam(outPortDef_, verInfo_);
    InitParam(videoFormat_, verInfo_);
    InitParam(bitrateConfig_, verInfo_);
    inPortDef_.nPortIndex = portParam.nStartPortNumber;
    outPortDef_.nPortIndex = portParam.nStartPortNumber + 1;
    videoFormat_.nPortIndex = portParam.nStartPortNumber;
    (void)InitBitRateMode();
}

int32_t HdiVencParamsMgr::SetParameter(GstCodecParamKey key, GstElement *element)
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
        case GST_REQUEST_I_FRAME:
            RequestIFrame();
            break;
        case GST_VENDOR:
            MEDIA_LOGD("Set vendor property");
            break;
        case GST_DYNAMIC_BITRATE:
            SetDynamicBitrate(element);
            break;
        default:
            break;
    }
    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::SetDynamicBitrate(GstElement *element)
{
    GstVencBase *base = GST_VENC_BASE(element);
    OMX_VIDEO_CONFIG_BITRATETYPE bitrateConfig;
    InitParam(bitrateConfig, verInfo_);
    bitrateConfig.nPortIndex = outPortDef_.nPortIndex;
    bitrateConfig.nEncodeBitrate = base->bitrate;
    auto ret = HdiSetConfig(handle_, OMX_IndexConfigVideoBitrate, bitrateConfig);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetConfig failed");
    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::SetInputVideoCommon(GstElement *element)
{
    GstVencBase *base = GST_VENC_BASE(element);
    MEDIA_LOGD("Set video frame rate %{public}d", base->frame_rate);
    inPortDef_.format.video.nFrameHeight = (uint32_t)base->height;
    inPortDef_.format.video.nFrameWidth = (uint32_t)base->width;
    inPortDef_.format.video.xFramerate = (uint32_t)(base->frame_rate) << OMX_FRAME_RATE_MOVE;
    inPortDef_.format.video.nSliceHeight = (uint32_t)base->nslice_height;
    inPortDef_.format.video.nStride = base->nstride;
    inPortDef_.nBufferSize = base->input.buffer_size;
    inPortDef_.nBufferCountActual = base->input.buffer_cnt;
    auto ret = HdiSetParameter(handle_, OMX_IndexParamPortDefinition, inPortDef_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetParameter failed");
    OMX_CONFIG_FRAMERATETYPE frameRateConfig;
    InitParam(frameRateConfig, verInfo_);
    frameRateConfig.nPortIndex = inPortDef_.nPortIndex;
    frameRateConfig.xEncodeFramerate = base->frame_rate << OMX_FRAME_RATE_MOVE;
    HdiSetConfig(handle_, OMX_IndexConfigVideoFramerate, frameRateConfig);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetConfig failed");
    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::SetOutputVideoCommon(GstElement *element)
{
    GstVencBase *base = GST_VENC_BASE(element);
    MEDIA_LOGD("Set video frame rate %{public}d", base->frame_rate);
    outPortDef_.format.video.nFrameHeight = (uint32_t)base->height;
    outPortDef_.format.video.nFrameWidth = (uint32_t)base->width;
    outPortDef_.format.video.xFramerate = (uint32_t)(base->frame_rate) << OMX_FRAME_RATE_MOVE;
    outPortDef_.nBufferCountActual = base->output.buffer_cnt;
    outPortDef_.nBufferSize = base->output.buffer_size;
    auto ret = HdiSetParameter(handle_, OMX_IndexParamPortDefinition, outPortDef_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetParameter failed");
    OMX_CONFIG_FRAMERATETYPE frameRateConfig;
    InitParam(frameRateConfig, verInfo_);
    frameRateConfig.nPortIndex = outPortDef_.nPortIndex;
    frameRateConfig.xEncodeFramerate = base->frame_rate << OMX_FRAME_RATE_MOVE;
    HdiSetConfig(handle_, OMX_IndexConfigVideoFramerate, frameRateConfig);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetConfig failed");
    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::SetVideoFormat(GstElement *element)
{
    GstVencBase *base = GST_VENC_BASE(element);
    videoFormat_.eColorFormat = (OMX_COLOR_FORMATTYPE)HdiCodecUtil::FormatGstToOmx(base->format); // need to do
    videoFormat_.xFramerate = (uint32_t)(base->frame_rate) << OMX_FRAME_RATE_MOVE;
    auto ret = HdiSetParameter(handle_, OMX_IndexParamVideoPortFormat, videoFormat_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetParameter failed");
    return GST_CODEC_OK;
}

void HdiVencParamsMgr::SetDfxNode(const std::shared_ptr<DfxNode> &node)
{
    dfxNode_ = node;
    dfxClassHelper_.Init(this, "HdiVdecParamsMgr", dfxNode_);
}

int32_t HdiVencParamsMgr::GetInputVideoCommon(GstElement *element)
{
    GstVencBase *base = GST_VENC_BASE(element);
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

int32_t HdiVencParamsMgr::GetOutputVideoCommon(GstElement *element)
{
    GstVencBase *base = GST_VENC_BASE(element);
    auto ret = HdiGetParameter(handle_, OMX_IndexParamPortDefinition, outPortDef_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiGetParameter failed");
    base->output.min_buffer_cnt = outPortDef_.nBufferCountMin;
    base->output.buffer_cnt = outPortDef_.nBufferCountActual;
    base->output.buffer_size = outPortDef_.nBufferSize;
    base->output.height = (int32_t)outPortDef_.format.video.nFrameHeight;
    base->output.width = (int32_t)outPortDef_.format.video.nFrameWidth;
    base->output.frame_rate = (int32_t)outPortDef_.format.video.xFramerate;
    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::GetVideoFormat(GstElement *element)
{
    GstVencBase *base = GST_VENC_BASE(element);
    auto ret = HdiGetParameter(handle_, OMX_IndexParamVideoPortFormat, videoFormat_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiGetParameter failed");

    while (HdiGetParameter(handle_, OMX_IndexParamVideoPortFormat, videoFormat_) == HDF_SUCCESS) {
        base->formats.push_back(HdiCodecUtil::FormatOmxToGst(videoFormat_.eColorFormat)); // need to do
        videoFormat_.nIndex++;
    }
    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::VideoSurfaceInit(GstElement *element)
{
    MEDIA_LOGD("Enter surface init");
    SupportBufferType supportBufferTypes;
    InitHdiParam(supportBufferTypes, verInfo_);
    supportBufferTypes.portIndex = inPortDef_.nPortIndex;
    auto ret = HdiGetParameter(handle_, OMX_IndexParamSupportBufferType, supportBufferTypes); // need to do
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiGetParameter failed");
    if (!(supportBufferTypes.bufferTypes & CODEC_BUFFER_TYPE_DYNAMIC_HANDLE)) {
        MEDIA_LOGD("No CODEC_BUFFER_TYPE_DYNAMIC_HANDLE, support bufferType %{public}d",
            supportBufferTypes.bufferTypes);
        return GST_CODEC_ERROR;
    }

    UseBufferType useBufferTypes;
    InitHdiParam(useBufferTypes, verInfo_);
    useBufferTypes.portIndex = inPortDef_.nPortIndex;
    useBufferTypes.bufferType = CODEC_BUFFER_TYPE_DYNAMIC_HANDLE;
    ret = HdiSetParameter(handle_, OMX_IndexParamUseBufferType, useBufferTypes); // need to do
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetParameter failed");
    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::RequestIFrame()
{
    MEDIA_LOGD("Request I frame");

    OMX_CONFIG_INTRAREFRESHVOPTYPE param;
    InitParam(param, verInfo_);
    param.nPortIndex = outPortDef_.nPortIndex;
    param.IntraRefreshVOP = OMX_TRUE;
    auto ret = HdiSetConfig(handle_, (OMX_INDEXTYPE)OMX_IndexConfigVideoIntraVOPRefresh, param);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "Set I frame Config Failed");

    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::InitBitRateMode()
{
    MEDIA_LOGD("Init bitrate");
    bitrateConfig_.nPortIndex = outPortDef_.nPortIndex;
    auto ret = HdiGetParameter(handle_, OMX_IndexParamVideoBitrate, bitrateConfig_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "OMX_IndexParamVideoBitrate Failed");
    bitrateConfig_.eControlRate = OMX_Video_ControlRateVariable;
    ret = HdiSetParameter(handle_, OMX_IndexParamVideoBitrate, bitrateConfig_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "OMX_IndexParamVideoBitrate Failed");
    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::GetParameter(GstCodecParamKey key, GstElement *element)
{
    CHECK_AND_RETURN_RET_LOG(element != nullptr, GST_CODEC_ERROR, "Element is nullptr");
    CHECK_AND_RETURN_RET_LOG(handle_ != nullptr, GST_CODEC_ERROR, "handle_ is nullptr");
    switch (key) {
        case GST_VIDEO_INPUT_COMMON:
            GetInputVideoCommon(element);
            break;
        case GST_VIDEO_OUTPUT_COMMON:
            GetOutputVideoCommon(element);
            break;
        case GST_VIDEO_FORMAT:
            SetVideoFormat(element);
            break;
        default:
            break;
    }
    return GST_CODEC_OK;
}
}  // namespace Media
}  // namespace OHOS
