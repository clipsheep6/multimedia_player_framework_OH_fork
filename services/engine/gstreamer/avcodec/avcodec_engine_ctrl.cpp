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
#include <vector>
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
    src_ = nullptr;
    sink_ = nullptr;
    if (codecBin_ != nullptr) {
        gst_object_unref(codecBin_);
        codecBin_ = nullptr;
    }
    if (gstPipeline_ != nullptr) {
        gst_object_unref(gstPipeline_);
        gstPipeline_ = nullptr;
    }
    if (bus_ != nullptr) {
        g_clear_object(&bus_);
        bus_ = nullptr;
    }
}

int32_t AVCodecEngineCtrl::Init(AVCodecType type, bool useSoftware, const std::string &name)
{
    MEDIA_LOGD("Enter Init");
    codecType_ = type;
    gstPipeline_ = GST_PIPELINE_CAST(gst_pipeline_new("codec-pipeline"));
    CHECK_AND_RETURN_RET(gstPipeline_ != nullptr, MSERR_NO_MEMORY);

    bus_ = gst_pipeline_get_bus(gstPipeline_);
    CHECK_AND_RETURN_RET(bus_ != nullptr, MSERR_UNKNOWN);
    gst_bus_set_sync_handler(bus_, BusSyncHandler, this, nullptr);

    codecBin_ = GST_ELEMENT_CAST(gst_object_ref(gst_element_factory_make("codecbin", "the_codec_bin")));
    CHECK_AND_RETURN_RET(codecBin_ != nullptr, MSERR_NO_MEMORY);

    gboolean ret = gst_bin_add(GST_BIN_CAST(gstPipeline_), codecBin_);
    CHECK_AND_RETURN_RET(ret == TRUE, MSERR_NO_MEMORY);

    g_object_set(codecBin_, "use-software", static_cast<gboolean>(useSoftware), nullptr);
    g_object_set(codecBin_, "type", static_cast<int32_t>(type), nullptr);
    g_object_set(codecBin_, "stream-input", static_cast<gboolean>(false), nullptr);
    g_object_set(codecBin_, "sink-convert", static_cast<gboolean>(true), nullptr);
    g_object_set(codecBin_, "coder-name", name.c_str(), nullptr);
    return MSERR_OK;
}

int32_t AVCodecEngineCtrl::Prepare(std::shared_ptr<ProcessorConfig> inputConfig,
    std::shared_ptr<ProcessorConfig> outputConfig)
{
    MEDIA_LOGD("Enter Prepare");
    if (src_ == nullptr) {
        MEDIA_LOGD("Use buffer src");
        src_ = AVCodecEngineFactory::CreateSrc(SrcType::SRC_TYPE_BYTEBUFFER);
        CHECK_AND_RETURN_RET_LOG(src_ != nullptr, MSERR_NO_MEMORY, "No memory");
        CHECK_AND_RETURN_RET(src_->Init() == MSERR_OK, MSERR_UNKNOWN);
        CHECK_AND_RETURN_RET(src_->SetCallback(obs_) == MSERR_OK, MSERR_UNKNOWN);
    }

    if (sink_ == nullptr) {
        MEDIA_LOGD("Use buffer sink");
        sink_ = AVCodecEngineFactory::CreateSink(SinkType::SINK_TYPE_BYTEBUFFER);
        CHECK_AND_RETURN_RET_LOG(sink_ != nullptr, MSERR_NO_MEMORY, "No memory");
        CHECK_AND_RETURN_RET(sink_->Init() == MSERR_OK, MSERR_UNKNOWN);
        CHECK_AND_RETURN_RET(sink_->SetCallback(obs_) == MSERR_OK, MSERR_UNKNOWN);
    }

    CHECK_AND_RETURN_RET(codecBin_ != nullptr, MSERR_UNKNOWN);
    g_object_set(codecBin_, "src", static_cast<gpointer>(src_->GetElement()), nullptr);
    CHECK_AND_RETURN_RET(src_->Configure(inputConfig) == MSERR_OK, MSERR_UNKNOWN);

    if (useSurfaceRender_) {
        MEDIA_LOGD("Need to overwrite pixel format by RGBA");
        GstCaps *caps = outputConfig->caps_;
        CHECK_AND_RETURN_RET(caps != nullptr, MSERR_UNKNOWN);
        GValue value = G_VALUE_INIT;
        g_value_init (&value, G_TYPE_STRING);
        g_value_set_string (&value, "RGBA");
        gst_caps_set_value(caps, "format", &value);
        g_value_unset(&value);
    }

    g_object_set(codecBin_, "sink", static_cast<gpointer>(sink_->GetElement()), nullptr);
    CHECK_AND_RETURN_RET(sink_->Configure(outputConfig) == MSERR_OK, MSERR_UNKNOWN);

    CHECK_AND_RETURN_RET(gstPipeline_ != nullptr, MSERR_UNKNOWN);
    GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(gstPipeline_), GST_STATE_PAUSED);
    CHECK_AND_RETURN_RET(ret != GST_STATE_CHANGE_FAILURE, MSERR_UNKNOWN);
    if (ret == GST_STATE_CHANGE_ASYNC) {
        MEDIA_LOGD("Wait state change");
        std::unique_lock<std::mutex> lock(gstPipeMutex_);
        gstPipeCond_.wait(lock);
    }
    MEDIA_LOGD("Prepare success");
    return MSERR_OK;
}

int32_t AVCodecEngineCtrl::Start()
{
    MEDIA_LOGD("Enter Start");
    CHECK_AND_RETURN_RET(gstPipeline_ != nullptr, MSERR_UNKNOWN);
    GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(gstPipeline_), GST_STATE_PLAYING);
    CHECK_AND_RETURN_RET(ret != GST_STATE_CHANGE_FAILURE, MSERR_UNKNOWN);
    if (ret == GST_STATE_CHANGE_ASYNC) {
        MEDIA_LOGD("Wait state change");
        std::unique_lock<std::mutex> lock(gstPipeMutex_);
        gstPipeCond_.wait(lock);
    }
    MEDIA_LOGD("Start success");
    return MSERR_OK;
}

int32_t AVCodecEngineCtrl::Stop()
{
    MEDIA_LOGD("Enter Stop");
    GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(gstPipeline_), GST_STATE_PAUSED);
    CHECK_AND_RETURN_RET(ret != GST_STATE_CHANGE_FAILURE, MSERR_UNKNOWN);
    if (ret == GST_STATE_CHANGE_ASYNC) {
        std::unique_lock<std::mutex> lock(gstPipeMutex_);
        gstPipeCond_.wait(lock);
    }
    MEDIA_LOGD("Stop success");
    return MSERR_OK;
}

int32_t AVCodecEngineCtrl::Flush()
{
    MEDIA_LOGD("Enter Flush");
    CHECK_AND_RETURN_RET(gstPipeline_ != nullptr, MSERR_UNKNOWN);

    CHECK_AND_RETURN_RET(src_ != nullptr, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(src_->Flush() == MSERR_OK, MSERR_UNKNOWN);

    CHECK_AND_RETURN_RET(sink_ != nullptr, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(sink_->Flush() == MSERR_OK, MSERR_UNKNOWN);

    CHECK_AND_RETURN_RET(codecBin_ != nullptr, MSERR_UNKNOWN);
    GstEvent *event = gst_event_new_flush_start();
    CHECK_AND_RETURN_RET(event != nullptr, MSERR_NO_MEMORY);
    (void)gst_element_send_event(codecBin_, event);

    event = gst_event_new_flush_stop(FALSE);
    CHECK_AND_RETURN_RET(event != nullptr, MSERR_NO_MEMORY);
    (void)gst_element_send_event(codecBin_, event);
    return MSERR_OK;
}

void AVCodecEngineCtrl::SetObs(const std::weak_ptr<IAVCodecEngineObs> &obs)
{
    obs_ = obs;
}

sptr<Surface> AVCodecEngineCtrl::CreateInputSurface()
{
    MEDIA_LOGD("Enter CreateInputSurface");
    CHECK_AND_RETURN_RET(codecType_ == AVCODEC_TYPE_VIDEO_ENCODER, nullptr);
    if (src_ == nullptr) {
        MEDIA_LOGD("Use surface src");
        src_ = AVCodecEngineFactory::CreateSrc(SrcType::SRC_TYPE_SURFACE);
        CHECK_AND_RETURN_RET_LOG(src_ != nullptr, nullptr, "No memory");
        CHECK_AND_RETURN_RET_LOG(src_->Init() == MSERR_OK, nullptr, "Failed to create input surface");
    }
    auto surface =  src_->CreateInputSurface();
    CHECK_AND_RETURN_RET(surface != nullptr, nullptr);
    return surface;
}

int32_t AVCodecEngineCtrl::SetOutputSurface(sptr<Surface> surface)
{
    MEDIA_LOGD("Enter SetOutputSurface");
    CHECK_AND_RETURN_RET(codecType_ == AVCODEC_TYPE_VIDEO_DECODER, MSERR_INVALID_OPERATION);
    if (sink_ == nullptr) {
        MEDIA_LOGD("Use surface sink");
        sink_ = AVCodecEngineFactory::CreateSink(SinkType::SINK_TYPE_SURFACE);
        CHECK_AND_RETURN_RET_LOG(sink_ != nullptr, MSERR_NO_MEMORY, "No memory");
        CHECK_AND_RETURN_RET(sink_->Init() == MSERR_OK, MSERR_UNKNOWN);
    }
    if (sink_->SetOutputSurface(surface) == MSERR_OK) {
        useSurfaceRender_ = true;
    } else {
        return MSERR_UNKNOWN;
    }
    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> AVCodecEngineCtrl::GetInputBuffer(uint32_t index)
{
    MEDIA_LOGD("Enter GetInputBuffer");
    CHECK_AND_RETURN_RET(src_ != nullptr, nullptr);
    return src_->GetInputBuffer(index);
}

int32_t AVCodecEngineCtrl::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    MEDIA_LOGD("Enter QueueInputBuffer");
    CHECK_AND_RETURN_RET(src_ != nullptr, MSERR_UNKNOWN);
    if (static_cast<int32_t>(flag) | static_cast<int32_t>(AVCODEC_BUFFER_FLAG_EOS)) {
        MEDIA_LOGE("Unsupport flush");
        //return Flush;
    }
    return src_->QueueInputBuffer(index, info, flag);
}

std::shared_ptr<AVSharedMemory> AVCodecEngineCtrl::GetOutputBuffer(uint32_t index)
{
    MEDIA_LOGD("Enter GetOutputBuffer");
    CHECK_AND_RETURN_RET(sink_ != nullptr, nullptr);
    return sink_->GetOutputBuffer(index);
}

int32_t AVCodecEngineCtrl::ReleaseOutputBuffer(uint32_t index, bool render)
{
    MEDIA_LOGD("Enter ReleaseOutputBuffer");
    CHECK_AND_RETURN_RET(sink_ != nullptr, MSERR_UNKNOWN);
    return sink_->ReleaseOutputBuffer(index, render);
}

int32_t AVCodecEngineCtrl::SetParameter(const Format &format)
{
    return MSERR_OK;
}

GstBusSyncReply AVCodecEngineCtrl::BusSyncHandler(GstBus *bus, GstMessage *message, gpointer userData)
{
    CHECK_AND_RETURN_RET(message != nullptr, GST_BUS_DROP);

    auto self = reinterpret_cast<AVCodecEngineCtrl *>(userData);
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_STATE_CHANGED: {
            CHECK_AND_RETURN_RET(message->src != nullptr, GST_BUS_DROP);
            if (GST_IS_BIN(message->src)) {
                MEDIA_LOGD("Finish state change");
                std::unique_lock<std::mutex> lock(self->gstPipeMutex_);
                self->gstPipeCond_.notify_all();
            }
            break;
        }
        case GST_MESSAGE_ERROR: {
            int32_t errCode = MSERR_UNKNOWN;
            GError *err = nullptr;
            gst_message_parse_error(message, &err, nullptr);
            if (err->domain == GST_CORE_ERROR) {
                errCode = MSERR_UNKNOWN;
            } else if (err->domain == GST_LIBRARY_ERROR) {
                errCode = MSERR_OPEN_FILE_FAILED;
            } else if (err->domain == GST_RESOURCE_ERROR) {
                errCode = MSERR_INVALID_VAL;
            } else if (err->domain == GST_STREAM_ERROR) {
                errCode = MSERR_DATA_SOURCE_ERROR_UNKNOWN;
            }
            auto obs = self->obs_.lock();
            CHECK_AND_RETURN_RET(obs != nullptr, GST_BUS_DROP);
            obs->OnError(AVCODEC_ERROR_INTERNAL, errCode);
            break;
        }
        default: {
            break;
        }
    }
    return GST_BUS_PASS;
}
} // Media
} // OHOS
