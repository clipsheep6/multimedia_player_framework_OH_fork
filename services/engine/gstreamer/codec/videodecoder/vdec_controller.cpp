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

#include "vdec_controller.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VdecController"};
    constexpr uint32_t DEFAULT_WIDTH = 480;
    constexpr uint32_t DEFAULT_HEIGHT = 640;
    constexpr uint32_t BUFFER_SIZE = 200000;
}

namespace OHOS {
namespace Media {
VdecController::VdecController()
{
    MEDIA_LOGD("enter ctor");
}

VdecController::~VdecController()
{
    MEDIA_LOGD("enter dtor");
    if (taskQueue_ != nullptr) {
        (void)Stop();
        (void)taskQueue_->Stop();
    }
    memInput_ = nullptr;
    vdec_ = nullptr;
    vdecRender_ = nullptr;
}

int32_t VdecController::Init()
{
    taskQueue_ = std::make_unique<TaskQueue>("vdec");
    int32_t ret = taskQueue_->Start();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    vdec_ = std::make_shared<VdecH264>();
    CHECK_AND_RETURN_RET(vdec_ != nullptr, MSERR_NO_MEMORY);

    memInput_ = AVSharedMemory::Create(BUFFER_SIZE, AVSharedMemory::Flags::FLAGS_READ_WRITE, "vdec_input");
    CHECK_AND_RETURN_RET_LOG(memInput_ != nullptr, MSERR_INVALID_VAL, "No memory");

    vdecRender_ = std::make_shared<VdecRenderer>();
    CHECK_AND_RETURN_RET_LOG(vdecRender_ != nullptr, MSERR_NO_MEMORY, "No memory");

    return MSERR_OK;
}

int32_t VdecController::Configure(sptr<Surface> surface)
{
    CHECK_AND_RETURN_RET(vdec_ != nullptr, MSERR_UNKNOWN);
    auto task = std::make_shared<TaskHandler<int32_t>>([this] {
        return vdec_->Configure(DEFAULT_WIDTH, DEFAULT_HEIGHT); });
    int32_t ret = taskQueue_->EnqueueTask(task);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    auto result = task->GetResult();
    CHECK_AND_RETURN_RET(!result.HasResult() || (result.Value() == MSERR_OK), result.Value());

    (void)SetSurface(surface);

    return MSERR_OK;
}

int32_t VdecController::Start()
{
    CHECK_AND_RETURN_RET(vdec_ != nullptr, MSERR_UNKNOWN);
    OnInputBufferAvailableCallback();
    return MSERR_OK;
}

int32_t VdecController::Stop()
{
    CHECK_AND_RETURN_RET(vdec_ != nullptr, MSERR_UNKNOWN);
    auto task = std::make_shared<TaskHandler<int32_t>>([this] { return vdec_->Release(); });
    int32_t ret = taskQueue_->EnqueueTask(task);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    auto result = task->GetResult();
    CHECK_AND_RETURN_RET(!result.HasResult() || (result.Value() == MSERR_OK), result.Value());

    return MSERR_OK;
}

int32_t VdecController::SetSurface(sptr<Surface> surface)
{
    CHECK_AND_RETURN_RET(vdecRender_ != nullptr, MSERR_UNKNOWN);
    auto task = std::make_shared<TaskHandler<int32_t>>([this, surface] { return vdecRender_->SetSurface(surface); });
    int32_t ret = taskQueue_->EnqueueTask(task);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    auto result = task->GetResult();
    CHECK_AND_RETURN_RET(!result.HasResult() || (result.Value() == MSERR_OK), result.Value());

    return MSERR_OK;
}

void VdecController::SetObs(const std::weak_ptr<IVideoDecoderEngineObs> &obs)
{
    obs_ = obs;
}

std::shared_ptr<AVSharedMemory> VdecController::GetInputBuffer(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(index == 0, nullptr, "Invalid index");
    return memInput_;
}

int32_t VdecController::PushInputBuffer(uint32_t index, uint32_t offset, uint32_t size,
    int64_t presentationTimeUs, VideoDecoderInputBufferFlag flags)
{
    CHECK_AND_RETURN_RET(vdec_ != nullptr, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET_LOG(index == 0, MSERR_INVALID_VAL, "Invalid index");
    CHECK_AND_RETURN_RET(memInput_ != nullptr, MSERR_UNKNOWN);
    uint8_t *address = memInput_->GetBase() + offset;
    auto task = std::make_shared<TaskHandler<int32_t>>([this, address, size] {
        return vdec_->Decode(address, size); });
    int32_t ret = taskQueue_->EnqueueTask(task);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    auto result = task->GetResult();
    if (!result.HasResult() || result.Value() != MSERR_OK) {
        MEDIA_LOGD("retry");
        OnInputBufferAvailableCallback();
        return MSERR_OK;
    }

    AVFrame *frame = vdec_->GetOutputFrame();
    CHECK_AND_RETURN_RET(frame != nullptr, MSERR_UNKNOWN);
    frame_ = frame;
    OnOutputBufferAvailableCallback();

    return MSERR_OK;
}

int32_t VdecController::ReleaseOutputBuffer(uint32_t index, bool render)
{
    CHECK_AND_RETURN_RET(vdecRender_ != nullptr, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET_LOG(index == 0, MSERR_UNKNOWN, "Invalid index");
    AVFrame *frame = frame_;
    auto task = std::make_shared<TaskHandler<int32_t>>([this, frame] { return vdecRender_->Render(frame); });
    int32_t ret = taskQueue_->EnqueueTask(task);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    auto result = task->GetResult();
    CHECK_AND_RETURN_RET(!result.HasResult() || (result.Value() == MSERR_OK), result.Value());

    if (dump_) {
        FILE *file = fopen("/data/dump.yuv", "ab");
        if (file != nullptr) {
            uint32_t pitchY = frame->linesize[0];
            uint32_t pitchU = frame->linesize[1];
            uint32_t pitchV = frame->linesize[2];

            uint8_t *avY = frame->data[0];
            uint8_t *avU = frame->data[1];
            uint8_t *avV = frame->data[2];

            for (uint32_t i = 0; i < frame->height; i++) {
                fwrite(avY, frame->width, 1, file);
                avY += pitchY;
            }

            for (uint32_t i = 0; i < frame->height/2; i++) {
                fwrite(avU, frame->width/2, 1, file);
                avU += pitchU;
            }

            for (uint32_t i = 0; i < frame->height/2; i++) {
                fwrite(avV, frame->width/2, 1, file);
                avV += pitchV;
            }

            fclose(file);
        }
    }

    OnInputBufferAvailableCallback();
    return MSERR_OK;
}

void VdecController::OnOutputBufferAvailableCallback()
{
    MEDIA_LOGD("OnOutputBufferAvailableCallback");
    std::shared_ptr<IVideoDecoderEngineObs> tempObs = obs_.lock();
    if (tempObs != nullptr) {
        VideoDecoderBufferInfo bufferInfo = {};
        tempObs->OnOutputBufferAvailable(0, bufferInfo);
    }
}

void VdecController::OnInputBufferAvailableCallback()
{
    MEDIA_LOGD("OnInputBufferAvailableCallback");
    std::shared_ptr<IVideoDecoderEngineObs> tempObs = obs_.lock();
    if (tempObs != nullptr) {
        tempObs->OnInputBufferAvailable(0);
    }
}
}
}
