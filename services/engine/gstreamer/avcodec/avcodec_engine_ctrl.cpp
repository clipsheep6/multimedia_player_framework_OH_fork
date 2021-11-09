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

#include "avcodec_engine_ctrl.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecEngineCtrl"};
}

namespace OHOS {
namespace Media {
AVCodecEngineCtrl::AVCodecEngineCtrl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecEngineCtrl::~AVCodecEngineCtrl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVCodecEngineCtrl::Init(AVCodecType type, bool useSoftware)
{
    gstPipeline_ = reinterpret_cast<GstPipeline *>(gst_pipeline_new("recorder-pipeline"));
    CHECK_AND_RETURN_RET(gstPipeline_ != nullptr, MSERR_NO_MEMORY);

    codecBin_ = gst_element_factory_make("gstcodecbin", "codec_bin");
    CHECK_AND_RETURN_RET(codecBin_ != nullptr, MSERR_NO_MEMORY);

    //todo
    g_object_set(codecBin_, "use-software", static_cast<gboolean>(useSoftware), nullptr);
    //g_object_set(codecBin_, "type", static_cast<GstCodecBinType>(type), nullptr);
    g_object_set(codecBin_, "stream-input", static_cast<gboolean>(false), nullptr);

    gst_bin_add(GST_BIN(gstPipeline_), codecBin_);

    if (gst_element_set_state(GST_ELEMENT_CAST(gstPipeline_), GST_STATE_READY) != GST_STATE_CHANGE_SUCCESS) {
        return MSERR_UNKNOWN;
    }
    return MSERR_OK;
}

int32_t AVCodecEngineCtrl::Prepare()
{
    return MSERR_OK;
}

int32_t AVCodecEngineCtrl::Start()
{
    return MSERR_OK;
}

int32_t AVCodecEngineCtrl::Stop()
{
    return MSERR_OK;
}

int32_t AVCodecEngineCtrl::Flush()
{
    return MSERR_OK;
}

int32_t AVCodecEngineCtrl::Release()
{
    return MSERR_OK;
}
} // Media
} // OHOS
