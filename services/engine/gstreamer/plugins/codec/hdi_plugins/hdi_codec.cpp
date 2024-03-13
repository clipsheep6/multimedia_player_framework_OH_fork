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

#include "hdi_codec.h"
#include <unordered_map>
#include <hdf_base.h>
#include "codec_callback_type_stub.h"
#include "media_log.h"
#include "media_errors.h"
#include "hdi_init.h"
#include "hdi_codec_util.h"
#include "securec.h"
#include "media_dfx.h"
#include "scope_guard.h"

using namespace std;
namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HdiCodec"};
    static const std::unordered_map<OMX_EVENTTYPE, std::string> OMX_EVENT_TO_STRING = {
        {OMX_EventCmdComplete, "OMX_EventCmdComplete"},
        {OMX_EventError, "OMX_EventError"},
        {OMX_EventMark, "OMX_EventMark"},
        {OMX_EventPortSettingsChanged, "OMX_EventPortSettingsChanged"},
        {OMX_EventBufferFlag, "OMX_EventBufferFlag"},
        {OMX_EventResourcesAcquired, "OMX_EventResourcesAcquired"},
        {OMX_EventComponentResumed, "OMX_EventComponentResumed"},
        {OMX_EventPortFormatDetected, "OMX_EventPortFormatDetected"},
        {OMX_EventDynamicResourcesAvailable, "OMX_EventDynamicResourcesAvailable"},
        {OMX_EventVendorStartUnused, "OMX_EventVendorStartUnused"},
        {OMX_EventMax, "OMX_EventMax"},
    };
}
namespace OHOS {
namespace Media {
HdiCodec::HdiCodec(const string& component)
    : componentName_(component), taskQue_("HDICallback")
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

HdiCodec::~HdiCodec()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    DeinitInner();
}

void HdiCodec::InitVersion()
{
    (void)memset_s(&verInfo_, sizeof(verInfo_), 0, sizeof(verInfo_));
    int32_t ret = HDF_SUCCESS;
    LISTENER(ret = handle_->GetComponentVersion(handle_, &verInfo_),
        "HdiCodec::GetComponentVersion", PlayerXCollie::timerTimeout)
    CHECK_AND_RETURN_LOG(ret == HDF_SUCCESS, "get version failed");
}

int32_t HdiCodec::Init()
{
    MEDIA_LOGD("Init");
    (void)taskQue_.Start();
    callback_ = CodecCallbackTypeStubGetInstance();
    CHECK_AND_RETURN_RET_LOG(callback_ != nullptr, GST_CODEC_ERROR, "GetCallBack failed");
    callback_->EventHandler = &HdiCodec::Event;
    callback_->EmptyBufferDone = &HdiCodec::EmptyBufferDone;
    callback_->FillBufferDone = &HdiCodec::FillBufferDone;
    appData_ = new AppData();
    appData_->instance = weak_from_this();
    int32_t ret;
    {
        MediaTrace trace("HdiCodec::Init");
        std::weak_ptr<IGstCodec> codec = weak_from_this();
        HdiInfo info = {componentName_, appData_, callback_, codec};
        ret = HdiInit::GetInstance().GetHandle(&handle_, id_, info);
        CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "GetHandle failed");
        CHECK_AND_RETURN_RET_LOG(handle_ != nullptr, GST_CODEC_ERROR, "Handle is nullptr");
        InitVersion();
        InitParam(portParam_, verInfo_);
        ret = HdiGetParameter(handle_, OMX_IndexParamVideoInit, portParam_);
    }
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "VideoInit failed");
    inPortIndex_ = portParam_.nStartPortNumber;
    outPortIndex_ = portParam_.nStartPortNumber + 1;
    return GST_CODEC_OK;
}

void HdiCodec::DeinitInner()
{
    if (handle_) {
        MEDIA_LOGD("FreeHandle");
        MediaTrace trace("HdiCodec::Deinit");
        (void)HdiInit::GetInstance().FreeHandle(handle_, id_);
    }
    CodecComponentTypeRelease(handle_);
    handle_ = nullptr;
    if (appData_) {
        delete appData_;
        appData_ = nullptr;
    }
    if (callback_) {
        CodecCallbackTypeStubRelease(callback_);
        callback_ = nullptr;
    }
    (void)taskQue_.Stop();
}

void HdiCodec::Deinit()
{
    MEDIA_LOGD("Deinit");
    if (curState_ > OMX_StateLoaded) {
        (void)WaitForState(OMX_StateLoaded);
    }
    DeinitInner();
}

void HdiCodec::OnCodecDied()
{
    MEDIA_LOGE("OnCodecDied");
    MediaTrace trace("HdiCodec::OnCodecDied");

    {
        unique_lock<mutex> lock(mutex_);
        start_ = false;
        isError_ = true;
        cond_.notify_all();
    }

    {
        std::shared_lock<std::shared_mutex> rLock(bufferMgrMutex_);
        if (inBufferMgr_) {
            inBufferMgr_->BufferReleased();
        }
        if (outBufferMgr_) {
            outBufferMgr_->BufferReleased();
        }
    }

    {
        std::unique_lock<std::shared_mutex> wLock(bufferMgrMutex_);
        inBufferMgr_ = nullptr;
        outBufferMgr_ = nullptr;
    }
}

void HdiCodec::SetHdiInBufferMgr(shared_ptr<HdiBufferMgr> bufferMgr)
{
    std::unique_lock<std::shared_mutex> wLock(bufferMgrMutex_);
    inBufferMgr_ = bufferMgr;
    inBufferMgr_->Init(handle_, inPortIndex_, verInfo_);
}

void HdiCodec::SetHdiOutBufferMgr(shared_ptr<HdiBufferMgr> bufferMgr)
{
    std::unique_lock<std::shared_mutex> wLock(bufferMgrMutex_);
    outBufferMgr_ = bufferMgr;
    outBufferMgr_->Init(handle_, outPortIndex_, verInfo_);
}

void HdiCodec::SetHdiParamsMgr(shared_ptr<HdiParamsMgr> paramsMgr)
{
    paramsMgr_ = paramsMgr;
    paramsMgr_->Init(handle_, portParam_, verInfo_);
}

int32_t HdiCodec::SetParameter(GstCodecParamKey key, GstElement *element)
{
    CHECK_AND_RETURN_RET_LOG(paramsMgr_ != nullptr, GST_CODEC_ERROR, "paramsMgr_ is nullptr");
    return paramsMgr_->SetParameter(key, element);
}

int32_t HdiCodec::GetParameter(GstCodecParamKey key, GstElement *element)
{
    CHECK_AND_RETURN_RET_LOG(paramsMgr_ != nullptr, GST_CODEC_ERROR, "paramsMgr_ is nullptr");
    return paramsMgr_->GetParameter(key, element);
}

int32_t HdiCodec::Start()
{
    CHECK_AND_RETURN_RET_LOG(!isError_, GST_CODEC_ERROR, "codec error");
    MEDIA_LOGD("Start begin");
    if (curState_ != OMX_StateExecuting) {
        CHECK_AND_RETURN_RET_LOG(ChangeState(OMX_StateExecuting) == GST_CODEC_OK, GST_CODEC_ERROR, "Change failed");
        CHECK_AND_RETURN_RET_LOG(WaitForState(OMX_StateExecuting) == GST_CODEC_OK, GST_CODEC_ERROR, "Wait failed");
    }
    {
        std::shared_lock<std::shared_mutex> rLock(bufferMgrMutex_);
        CHECK_AND_RETURN_RET_LOG(inBufferMgr_ != nullptr, GST_CODEC_ERROR, "inBufferMgr_ is nullptr");
        CHECK_AND_RETURN_RET_LOG(outBufferMgr_ != nullptr, GST_CODEC_ERROR, "outBufferMgr_ is nullptr");
        MediaTrace trace("HdiCodec::Start");
        inBufferMgr_->Start();
        outBufferMgr_->Start();
    }
    unique_lock<mutex> lock(mutex_);
    start_ = true;
    MEDIA_LOGD("Start end");
    return GST_CODEC_OK;
}

int32_t HdiCodec::Stop()
{
    CHECK_AND_RETURN_RET_LOG(!isError_, GST_CODEC_OK, "codec error");
    {
        std::shared_lock<std::shared_mutex> rLock(bufferMgrMutex_);
        CHECK_AND_RETURN_RET_LOG(inBufferMgr_ != nullptr, GST_CODEC_ERROR, "inBufferMgr_ is nullptr");
        CHECK_AND_RETURN_RET_LOG(outBufferMgr_ != nullptr, GST_CODEC_ERROR, "outBufferMgr_ is nullptr");
        MediaTrace trace("HdiCodec::Stop");
        inBufferMgr_->Stop(false);
        outBufferMgr_->Stop(false);
    }
    if (curState_ == OMX_StateExecuting) {
        CHECK_AND_RETURN_RET_LOG(ChangeState(OMX_StateIdle) == GST_CODEC_OK, GST_CODEC_ERROR, "ChangeState failed");
        CHECK_AND_RETURN_RET_LOG(WaitForState(OMX_StateIdle) == GST_CODEC_OK, GST_CODEC_ERROR, "Wait failed");
    }
    if (curState_ == OMX_StateIdle) {
        CHECK_AND_RETURN_RET_LOG(ChangeState(OMX_StateLoaded) == GST_CODEC_OK, GST_CODEC_ERROR, "ChangeState failed");
    }
    unique_lock<mutex> lock(mutex_);
    start_ = false;
    return GST_CODEC_OK;
}

int32_t HdiCodec::ChangeState(OMX_STATETYPE state)
{
    MEDIA_LOGD("Change state from %{public}u to %{public}u", targetState_, state);
    CHECK_AND_RETURN_RET_LOG(!isError_, GST_CODEC_ERROR, "codec error");
    if (targetState_ != state && curState_ != state) {
        auto ret = HdiSendCommand(handle_, OMX_CommandStateSet, state, 0);
        CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSendCommand failed");
        targetState_ = state;
    }
    return GST_CODEC_OK;
}

int32_t HdiCodec::UseInputBuffers(std::vector<GstBuffer*> buffers)
{
    std::shared_lock<std::shared_mutex> rLock(bufferMgrMutex_);
    CHECK_AND_RETURN_RET_LOG(!isError_, GST_CODEC_ERROR, "codec error");
    CHECK_AND_RETURN_RET_LOG(inBufferMgr_ != nullptr, GST_CODEC_ERROR, "inBufferMgr_ is nullptr");
    MediaTrace trace("HdiCodec::UseInputBuffers");
    return inBufferMgr_->UseBuffers(buffers);
}

int32_t HdiCodec::PushInputBuffer(GstBuffer *buffer)
{
    std::shared_lock<std::shared_mutex> rLock(bufferMgrMutex_);
    CHECK_AND_RETURN_RET_LOG(!isError_, GST_CODEC_ERROR, "codec error");
    CHECK_AND_RETURN_RET_LOG(inBufferMgr_ != nullptr, GST_CODEC_ERROR, "inBufferMgr_ is nullptr");
    return inBufferMgr_->PushBuffer(buffer);
}

int32_t HdiCodec::Flush(GstCodecDirect direct)
{
    MEDIA_LOGD("Flush start");
    if (!start_) {
        return GST_CODEC_OK;
    }

    std::shared_lock<std::shared_mutex> rLock(bufferMgrMutex_);
    CHECK_AND_RETURN_RET_LOG(handle_ != nullptr, GST_CODEC_ERROR, "Handle is nullptr");
    CHECK_AND_RETURN_RET_LOG(inBufferMgr_ != nullptr, GST_CODEC_ERROR, "inBufferMgr_ is nullptr");
    CHECK_AND_RETURN_RET_LOG(outBufferMgr_ != nullptr, GST_CODEC_ERROR, "outBufferMgr_ is nullptr");
    int32_t ret = HDF_SUCCESS;
    if (direct == GST_CODEC_ALL) {
        inBufferMgr_->Flush(true);
        outBufferMgr_->Flush(true);
        ret = HdiSendCommand(handle_, OMX_CommandFlush, -1, 0);
    }
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSendCommand failed");
    inBufferMgr_->WaitFlushed();
    outBufferMgr_->WaitFlushed();
    startFormatChange_.store(false);
    MEDIA_LOGD("Flush end");
    return GST_CODEC_OK;
}

int32_t HdiCodec::ActiveBufferMgr(GstCodecDirect direct, bool active)
{
    CHECK_AND_RETURN_RET_LOG(!isError_, GST_CODEC_ERROR, "codec error");
    CHECK_AND_RETURN_RET_LOG(handle_ != nullptr, GST_CODEC_ERROR, "Handle is nullptr");
    int32_t ret = HDF_SUCCESS;
    // do not deal with input
    if (direct == GST_CODEC_OUTPUT) {
        if (active) {
            ret = HdiSendCommand(handle_, OMX_CommandPortEnable, outPortIndex_, 0);
            unique_lock<mutex> lock(mutex_);
            outState_ = ACTIVING;
        } else {
            MEDIA_LOGD("OMX_CommandPortDisable out %{public}u", outPortIndex_);
            ret = HdiSendCommand(handle_, OMX_CommandPortDisable, outPortIndex_, 0);
            unique_lock<mutex> lock(mutex_);
            outState_ = DEACTIVING;
        }
    }
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "port change fail");
    return GST_CODEC_OK;
}

int32_t HdiCodec::FreeInputBuffers()
{
    std::shared_lock<std::shared_mutex> rLock(bufferMgrMutex_);
    CHECK_AND_RETURN_RET_LOG(!isError_, GST_CODEC_OK, "codec error");
    CHECK_AND_RETURN_RET_LOG(inBufferMgr_ != nullptr, GST_CODEC_ERROR, "inBufferMgr_ is nullptr");
    CHECK_AND_RETURN_RET_LOG(inBufferMgr_->FreeBuffers() == GST_CODEC_OK, GST_CODEC_ERROR, "Freebuffer fail");
    if (inState_ == DEACTIVING) {
        WaitForEvent(OMX_CommandPortDisable);
    }
    return GST_CODEC_OK;
}

int32_t HdiCodec::UseOutputBuffers(std::vector<GstBuffer*> buffers)
{
    CHECK_AND_RETURN_RET_LOG(!isError_, GST_CODEC_ERROR, "codec error");
    MEDIA_LOGD("UseOutputBuffers");
    MediaTrace trace("HdiCodec::UseOutputBuffers");
    if (curState_ < OMX_StateIdle) {
        std::shared_lock<std::shared_mutex> rLock(bufferMgrMutex_);
        CHECK_AND_RETURN_RET_LOG(inBufferMgr_ != nullptr, GST_CODEC_ERROR, "inBufferMgr_ is nullptr");
        CHECK_AND_RETURN_RET_LOG(outBufferMgr_ != nullptr, GST_CODEC_ERROR, "outBufferMgr_ is nullptr");
        CHECK_AND_RETURN_RET_LOG(ChangeState(OMX_StateIdle) == GST_CODEC_OK,
            GST_CODEC_ERROR, "ChangeState failed");
        // m40 need input and output allocate or use common
        CHECK_AND_RETURN_RET_LOG(inBufferMgr_->Preprocessing() == GST_CODEC_OK,
            GST_CODEC_ERROR, "Allocatebuffer fail");
        CHECK_AND_RETURN_RET_LOG(outBufferMgr_->UseBuffers(buffers) == GST_CODEC_OK,
            GST_CODEC_ERROR, "UseBuffers fail");
        CHECK_AND_RETURN_RET_LOG(WaitForState(OMX_StateIdle) == GST_CODEC_OK,
            GST_CODEC_ERROR, "Wait failed");
    } else if (outState_ == ACTIVING) {
        std::shared_lock<std::shared_mutex> rLock(bufferMgrMutex_);
        CHECK_AND_RETURN_RET_LOG(outBufferMgr_ != nullptr, GST_CODEC_ERROR, "outBufferMgr_ is nullptr");
        CHECK_AND_RETURN_RET_LOG(outBufferMgr_->UseBuffers(buffers) == GST_CODEC_OK,
            GST_CODEC_ERROR, "Usebuffer fail");
        WaitForEvent(OMX_CommandPortEnable);
    }
    return GST_CODEC_OK;
}

int32_t HdiCodec::PushOutputBuffer(GstBuffer *buffer)
{
    std::shared_lock<std::shared_mutex> rLock(bufferMgrMutex_);
    CHECK_AND_RETURN_RET_LOG(outBufferMgr_ != nullptr, GST_CODEC_ERROR, "outBufferMgr_ is nullptr");
    return outBufferMgr_->PushBuffer(buffer);
}

int32_t HdiCodec::PullOutputBuffer(GstBuffer **buffer)
{
    CHECK_AND_RETURN_RET_LOG(!isError_, GST_CODEC_ERROR, "codec error");
    CHECK_AND_RETURN_RET_LOG(outBufferMgr_ != nullptr, GST_CODEC_ERROR, "outBufferMgr_ is nullptr");
    int32_t ret = GST_CODEC_OK;
    {
        unique_lock<mutex> lock(mutex_);
        if ((ret_ != GST_CODEC_OK && ret_ != GST_CODEC_FORMAT_CHANGE) ||
            (ret_ == GST_CODEC_FORMAT_CHANGE && outBufferMgr_->GetWaitDisPlayBufNum() == 0)) {
            MEDIA_LOGD("change ret from ret %{public}d to ret %{public}d", ret, ret_);
            ret = ret_;
            ret_ = GST_CODEC_OK;
            return ret;
        }
    }
    {
        std::shared_lock<std::shared_mutex> rLock(bufferMgrMutex_);
        CHECK_AND_RETURN_RET_LOG(outBufferMgr_ != nullptr, GST_CODEC_ERROR, "outBufferMgr_ is nullptr");
        ret = outBufferMgr_->PullBuffer(buffer);
    }
    MEDIA_LOGD("ret %{public}d", ret);
    {
        unique_lock<mutex> lock(mutex_);
        if ((ret_ == GST_CODEC_FORMAT_CHANGE && outBufferMgr_->GetWaitDisPlayBufNum() == 0) ||
            (ret_ != GST_CODEC_FORMAT_CHANGE && ret_ != GST_CODEC_OK)) {
            MEDIA_LOGD("change ret from %{public}d to %{public}d", ret, ret_);
            ret = ret_;
            ret_ = GST_CODEC_OK;
            return ret;
        }
    }
    return ret;
}

void HdiCodec::SetOutputPool(GstBufferPool *pool)
{
    std::shared_lock<std::shared_mutex> rLock(bufferMgrMutex_);
    CHECK_AND_RETURN_LOG(outBufferMgr_ != nullptr, "outBufferMgr_ is nullptr");
    outBufferMgr_->SetOutputPool(pool);
}

int32_t HdiCodec::FreeOutputBuffers()
{
    std::shared_lock<std::shared_mutex> rLock(bufferMgrMutex_);
    CHECK_AND_RETURN_RET_LOG(!isError_, GST_CODEC_OK, "codec error");
    CHECK_AND_RETURN_RET_LOG(outBufferMgr_ != nullptr, GST_CODEC_ERROR, "outBufferMgr_ is nullptr");
    MEDIA_LOGD("FreeOutputBuffers");
    CHECK_AND_RETURN_RET_LOG(outBufferMgr_->FreeBuffers() == GST_CODEC_OK, GST_CODEC_ERROR, "Freebuffers fail");
    if (outState_ == DEACTIVING) {
        WaitForEvent(OMX_CommandPortDisable);
    }
    return GST_CODEC_OK;
}

int32_t HdiCodec::Event(CodecCallbackType *self, OMX_EVENTTYPE event, EventInfo *info)
{
    MediaTrace trace("HdiCodec::Event");
    (void)self;
    CHECK_AND_RETURN_RET_LOG(info != nullptr, HDF_ERR_INVALID_PARAM, "appData is null");
    if (OMX_EVENT_TO_STRING.find(event) != OMX_EVENT_TO_STRING.end()) {
        MEDIA_LOGD("Event %{public}s %{public}d data1 %{public}d data2 %{public}d",
            OMX_EVENT_TO_STRING.at(event).c_str(), event, info->data1, info->data2);
    } else {
        MEDIA_LOGW("Unknown event %{public}d data1 %{public}d data2 %{public}d",
            event, info->data1, info->data2);
    }
    AppData *mAppData = reinterpret_cast<AppData *>(info->appData);
    auto instance = mAppData->instance.lock();
    CHECK_AND_RETURN_RET_LOG(instance != nullptr, HDF_ERR_INVALID_PARAM, "HdiCodec is null");
    return instance->OnEvent(event, info);
}

int32_t HdiCodec::OnEvent(OMX_EVENTTYPE event, EventInfo *info)
{
    auto task = std::make_shared<TaskHandler<void>>([this, event, info] {
        switch (event) {
            case OMX_EventCmdComplete:
                return HandelEventCmdComplete(info->data1, info->data2);
            case OMX_EventPortSettingsChanged:
                return HandleEventPortSettingsChanged(info->data1, info->data2);
            case OMX_EventBufferFlag:
                return HandleEventBufferFlag(info->data1, info->data2);
            case OMX_EventError:
                return HandleEventError(info->data1);
            default:
                break;
        }
    });
    (void)taskQue_.EnqueueTask(task);
    (void)task->GetResult();
    return HDF_SUCCESS;
}

int32_t HdiCodec::WaitForState(OMX_STATETYPE state)
{
    WaitForEvent(OMX_CommandStateSet);
    CHECK_AND_RETURN_RET_LOG(curState_ == state, GST_CODEC_ERROR, "Wait state failed");
    return GST_CODEC_OK;
}

void HdiCodec::WaitForEvent(OMX_U32 cmd)
{
    unique_lock<mutex> lock(mutex_);
    int32_t newCmd = static_cast<int32_t>(cmd);
    MEDIA_LOGD("wait eventdone %{public}d lastcmd %{public}d cmd %{public}u", eventDone_, lastCmd_, cmd);
    static constexpr int32_t timeout = 2;
    cond_.wait_for(lock, std::chrono::seconds(timeout),
        [this, &newCmd]() { return (eventDone_ && (lastCmd_ == newCmd || lastCmd_ == -1)) || isError_; });
    eventDone_ = false;
}

void HdiCodec::HandelEventStateSet(OMX_U32 data)
{
    MEDIA_LOGD("change curState_ from %{public}u to %{public}u", curState_, data);
    curState_ = static_cast<OMX_STATETYPE>(data);
    eventDone_ = true;
}

void HdiCodec::HandelEventFlush(OMX_U32 data)
{
    std::shared_lock<std::shared_mutex> rLock(bufferMgrMutex_);
    if (data == inPortIndex_) {
        CHECK_AND_RETURN_LOG(inBufferMgr_ != nullptr, "inBufferMgr_ is nullptr");
        inBufferMgr_->Flush(false);
    } else {
        CHECK_AND_RETURN_LOG(outBufferMgr_ != nullptr, "outBufferMgr_ is nullptr");
        outBufferMgr_->Flush(false);
    }
}

void HdiCodec::HandelEventPortDisable(OMX_U32 data)
{
    if (data != inPortIndex_) {
        outState_ = DEACTIVATED;
    }
    eventDone_ = true;
}

void HdiCodec::HandelEventPortEnable(OMX_U32 data)
{
    if (data != inPortIndex_) {
        outState_ = ACTIVATED;
    }
    eventDone_ = true;
}

void HdiCodec::HandelEventCmdComplete(OMX_U32 data1, OMX_U32 data2)
{
    unique_lock<mutex> lock(mutex_);
    lastCmd_ = (int32_t)data1;
    switch (data1) {
        case OMX_CommandStateSet:
            HandelEventStateSet(data2);
            break;
        case OMX_CommandFlush:
            HandelEventFlush(data2);
            break;
        case OMX_CommandPortDisable:
            HandelEventPortDisable(data2);
            break;
        case OMX_CommandPortEnable:
            HandelEventPortEnable(data2);
            break;
        default:
            break;
    }
    cond_.notify_all();
}

void HdiCodec::HandleEventPortSettingsChanged(OMX_U32 data1, OMX_U32 data2)
{
    MEDIA_LOGD("handle change");
    unique_lock<mutex> lock(mutex_);
    if (data2 == OMX_IndexParamPortDefinition) {
        MEDIA_LOGD("GST_CODEC_FORMAT_CHANGE");
        ret_ = GST_CODEC_FORMAT_CHANGE;
        startFormatChange_.store(true);
        std::shared_lock<std::shared_mutex> rLock(bufferMgrMutex_);
        CHECK_AND_RETURN_LOG(data1 != inPortIndex_, "inPortIndex_ setting changed");
        if (data1 != inPortIndex_) {
            CHECK_AND_RETURN_LOG(outBufferMgr_ != nullptr, "outBufferMgr_ is nullptr");
            outBufferMgr_->Stop(true);
        }
    }
    MEDIA_LOGD("handle change end");
}

void HdiCodec::HandleEventBufferFlag(OMX_U32 data1, OMX_U32 data2)
{
    unique_lock<mutex> lock(mutex_);
    if (data1 == 1 && (data2 & OMX_BUFFERFLAG_EOS)) {
        MEDIA_LOGD("it is eos, wait buffer eos");
    }
}

void HdiCodec::HandleEventError(OMX_U32 data)
{
    (void)data;
    unique_lock<mutex> lock(mutex_);
    ret_ = GST_CODEC_ERROR;
    eventDone_ = true;
    lastCmd_ = -1;
    cond_.notify_all();
}

int32_t HdiCodec::EmptyBufferDone(CodecCallbackType *self, int64_t appData, const OmxCodecBuffer *buffer)
{
    MediaTrace trace("HdiCodec::EmptyBufferDone");
    MEDIA_LOGD("EmptyBufferDone");
    (void)self;
    AppData *mAppData = reinterpret_cast<AppData *>(appData);
    CHECK_AND_RETURN_RET_LOG(mAppData != nullptr, HDF_ERR_INVALID_PARAM, "appData is null");
    auto instance = mAppData->instance.lock();
    CHECK_AND_RETURN_RET_LOG(instance != nullptr, HDF_ERR_INVALID_PARAM, "HdiCodec is null");
    CHECK_AND_RETURN_RET_LOG(instance->OnEmptyBufferDone(buffer) == GST_CODEC_OK,
        OMX_ErrorBadParameter, "empty buffer done failed");
    return HDF_SUCCESS;
}

int32_t HdiCodec::OnEmptyBufferDone(const OmxCodecBuffer *buffer)
{
    std::shared_lock<std::shared_mutex> rLock(bufferMgrMutex_);
    CHECK_AND_RETURN_RET_LOG(inBufferMgr_ != nullptr, GST_CODEC_ERROR, "inBufferMgr_ is nullptr");
    auto task = std::make_shared<TaskHandler<int32_t>>([this, buffer] {
        return inBufferMgr_->CodecBufferAvailable(buffer);
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    int32_t ret = GST_CODEC_OK;
    if (result.HasResult()) {
        ret = result.Value();
    }
    return ret;
}

int32_t HdiCodec::FillBufferDone(CodecCallbackType *self, int64_t appData, const OmxCodecBuffer *buffer)
{
    MediaTrace trace("HdiCodec::FillBufferDone");
    MEDIA_LOGD("FillBufferDone");
    (void)self;
    AppData *mAppData = reinterpret_cast<AppData *>(appData);
    CHECK_AND_RETURN_RET_LOG(mAppData != nullptr, OMX_ErrorBadParameter, "appData is null");
    auto instance = mAppData->instance.lock();
    CHECK_AND_RETURN_RET_LOG(instance != nullptr, OMX_ErrorBadParameter, "HdiCodec is null");
    if (instance->OnFillBufferDone(buffer) != GST_CODEC_OK) {
        MEDIA_LOGE("fill buffer done failed");
        return OMX_ErrorBadParameter;
    }
    return HDF_SUCCESS;
}

int32_t HdiCodec::OnFillBufferDone(const OmxCodecBuffer *buffer)
{
    std::shared_lock<std::shared_mutex> rLock(bufferMgrMutex_);
    CHECK_AND_RETURN_RET_LOG(outBufferMgr_ != nullptr, GST_CODEC_ERROR, "outBufferMgr_ is nullptr");
    auto task = std::make_shared<TaskHandler<int32_t>>([this, buffer] {
        return outBufferMgr_->CodecBufferAvailable(buffer);
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    int32_t ret = GST_CODEC_OK;
    if (result.HasResult()) {
        ret = result.Value();
    }
    return ret;
}

bool HdiCodec::IsFormatChanged()
{
    bool ret = startFormatChange_.load();
    startFormatChange_.store(false);
    return ret;
}
}  // namespace Media
}  // namespace OHOS
