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

#include "vdec_h264.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VdecH264"};
}

namespace OHOS {
namespace Media {
VdecH264::VdecH264()
{
    MEDIA_LOGD("enter ctor");
}

VdecH264::~VdecH264()
{
    MEDIA_LOGD("enter dtor");
    if (codecCtx_ != nullptr) {
        avcodec_free_context(&codecCtx_);
        codecCtx_ = nullptr;
    }
    if (pictureYUV_ != nullptr) {
        av_frame_free(&pictureYUV_);
        pictureYUV_ = nullptr;
    }
}

int32_t VdecH264::Configure(uint32_t width, uint32_t height)
{
    codec_ = avcodec_find_decoder(AV_CODEC_ID_H264);
    CHECK_AND_RETURN_RET_LOG(codec_ != nullptr, MSERR_UNKNOWN, "Failed to find h264 decoder");

    codecCtx_ = avcodec_alloc_context3(codec_);
    CHECK_AND_RETURN_RET_LOG(codecCtx_ != nullptr, MSERR_UNKNOWN, "Failed to init context");
    codecCtx_->codec_type = AVMEDIA_TYPE_VIDEO;
    codecCtx_->pix_fmt = AV_PIX_FMT_YUV420P;
    width_ = width;
    height_ = height;

    CHECK_AND_RETURN_RET_LOG(avcodec_open2(codecCtx_, codec_, nullptr) == 0, MSERR_UNKNOWN, "Failed to open context");

    pictureYUV_ = av_frame_alloc();
    CHECK_AND_RETURN_RET_LOG(pictureYUV_ != nullptr, MSERR_NO_MEMORY, "No memory");

    return MSERR_OK;
}

int32_t VdecH264::Decode(uint8_t *buffer, int32_t size)
{
    MEDIA_LOGD("VdecH264 Decode, size:%{public}d", size);
    av_init_packet(&pkt_);
    pkt_.size = size;
    pkt_.data = buffer;
    if (avcodec_send_packet(codecCtx_, &pkt_) != 0) {
        MEDIA_LOGW("VdecH264 Decode retry");
        return MSERR_UNKNOWN;
    }
    if (avcodec_receive_frame(codecCtx_, pictureYUV_) != 0) {
        MEDIA_LOGW("VdecH264 Decode no output");
        return MSERR_UNKNOWN;
    }
    return MSERR_OK;
}

AVFrame *VdecH264::GetOutputFrame()
{
    return pictureYUV_;
}

int32_t VdecH264::Release()
{
    return MSERR_OK;
}
}
}
