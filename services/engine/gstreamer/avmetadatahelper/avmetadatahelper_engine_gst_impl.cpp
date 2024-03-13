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

#include "avmetadatahelper_engine_gst_impl.h"
#include <gst/gst.h>
#include "media_errors.h"
#include "media_log.h"
#include "i_playbin_ctrler.h"
#include "avmeta_sinkprovider.h"
#include "avmeta_frame_extractor.h"
#include "avmeta_meta_collector.h"
#include "scope_guard.h"
#include "uri_helper.h"
#include "media_dfx.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetaEngineGstImpl"};
    const std::map<OHOS::Media::PlayBinState, std::string> AVMETADATA_STATE_MAP = {
        {OHOS::Media::PLAYBIN_STATE_IDLE, "idle"},
        {OHOS::Media::PLAYBIN_STATE_INITIALIZED, "initialized"},
        {OHOS::Media::PLAYBIN_STATE_PREPARING, "preparing"},
        {OHOS::Media::PLAYBIN_STATE_PREPARED, "prepared"},
        {OHOS::Media::PLAYBIN_STATE_PLAYING, "playing"},
        {OHOS::Media::PLAYBIN_STATE_PAUSED, "paused"},
        {OHOS::Media::PLAYBIN_STATE_STOPPED, "stopped"},
        {OHOS::Media::PLAYBIN_STATE_PLAYBACK_COMPLETE, "playbackcomplete"},
    };
}

namespace OHOS {
namespace Media {
static const std::set<PixelFormat> SUPPORTED_PIXELFORMAT = {
    PixelFormat::RGB_565, PixelFormat::RGB_888, PixelFormat::RGBA_8888
};

static bool CheckFrameFetchParam(int64_t timeUsOrIndex, int32_t option, const OutputConfiguration &param)
{
    (void)timeUsOrIndex;
    if ((option != AV_META_QUERY_CLOSEST) && (option != AV_META_QUERY_CLOSEST_SYNC) &&
        (option != AV_META_QUERY_NEXT_SYNC) && (option != AV_META_QUERY_PREVIOUS_SYNC)) {
        MEDIA_LOGE("Invalid query option: %{public}d", option);
        return false;
    }

    if (SUPPORTED_PIXELFORMAT.count(param.colorFormat) == 0) {
        MEDIA_LOGE("Unsupported pixelformat: %{public}d", param.colorFormat);
        return false;
    }

    static constexpr int32_t maxDstWidth = 7680;
    static constexpr int32_t minDstWidth = 32;
    if (param.dstWidth > maxDstWidth || (param.dstWidth < minDstWidth && param.dstWidth != -1)) {
        MEDIA_LOGE("Invalid dstWidth: %{public}d", param.dstWidth);
        return false;
    }

    static constexpr int32_t maxDstHeight = 4320;
    static constexpr int32_t minDstHeight = 32;
    if (param.dstHeight > maxDstHeight || (param.dstHeight < minDstHeight && param.dstHeight != -1)) {
        MEDIA_LOGE("Invalid dstHeight: %{public}d", param.dstHeight);
        return false;
    }

    return true;
}

AVMetadataHelperEngineGstImpl::AVMetadataHelperEngineGstImpl()
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

AVMetadataHelperEngineGstImpl::~AVMetadataHelperEngineGstImpl()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    Reset();
    appsrcWrap_ = nullptr;
}

int32_t AVMetadataHelperEngineGstImpl::SetSource(const std::string &uri, int32_t usage)
{
    if ((usage != AVMetadataUsage::AV_META_USAGE_META_ONLY) &&
        (usage != AVMetadataUsage::AV_META_USAGE_PIXEL_MAP)) {
        MEDIA_LOGE("Invalid avmetadatahelper usage: %{public}d", usage);
        return MSERR_INVALID_VAL;
    }

    UriHelper uriHelper(uri);
    if (uriHelper.UriType() != UriHelper::URI_TYPE_FILE && uriHelper.UriType() != UriHelper::URI_TYPE_FD) {
        MEDIA_LOGE("Unsupported uri type : %{public}s", uri.c_str());
        return MSERR_UNSUPPORT;
    }

    MEDIA_LOGI("uri: %{public}s, usage: %{public}d", uri.c_str(), usage);

    int32_t ret = SetSourceInternel(uri, usage);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call SetSourceInternel");

    MEDIA_LOGI("set source success");
    return MSERR_OK;
}

int32_t AVMetadataHelperEngineGstImpl::SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    CHECK_AND_RETURN_RET_LOG(dataSrc != nullptr, MSERR_INVALID_VAL, "input dataSrc is empty!");
    appsrcWrap_ = GstAppsrcEngine::Create(dataSrc);
    CHECK_AND_RETURN_RET_LOG(appsrcWrap_ != nullptr, MSERR_NO_MEMORY, "new appsrcwrap failed!");

    const std::string uri = "";
    int32_t ret = SetSourceInternel(uri, AVMetadataUsage::AV_META_USAGE_META_ONLY);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call SetSourceInternel");

    MEDIA_LOGI("set data source success");
    return MSERR_OK;
}

std::string AVMetadataHelperEngineGstImpl::ResolveMetadata(int32_t key)
{
    MEDIA_LOGD("enter");
    std::string result;

    int32_t ret = ExtractMetadata();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, result, "Failed to call ExtractMetadata");

    if (collectedMeta_.count(key) == 0 || collectedMeta_.at(key).empty()) {
        MEDIA_LOGE("The specified metadata %{public}d cannot be obtained from the specified stream.", key);
        return result;
    }

    MEDIA_LOGD("exit");
    result = collectedMeta_[key];
    return result;
}

std::unordered_map<int32_t, std::string> AVMetadataHelperEngineGstImpl::ResolveMetadata()
{
    MEDIA_LOGD("enter");

    int32_t ret = ExtractMetadata();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, {}, "Failed to call ExtractMetadata");

    MEDIA_LOGD("exit");
    return collectedMeta_;
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperEngineGstImpl::FetchArtPicture()
{
    MEDIA_LOGD("enter FetchArtPicture");

    int32_t ret = ExtractMetadata();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "Failed to call ExtractMetadata");

    auto result = metaCollector_->FetchArtPicture();
    if (result == nullptr) {
        MEDIA_LOGE("FetchArtPicture result is nullptr.");
    }
    MEDIA_LOGD("exit FetchArtPicture");
    return result;
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperEngineGstImpl::FetchFrameAtTime(
    int64_t timeUs, int32_t option, const OutputConfiguration &param)
{
    MEDIA_LOGD("enter FetchFrameAtTime");

    if (usage_ != AVMetadataUsage::AV_META_USAGE_PIXEL_MAP) {
        MEDIA_LOGE("current instance is unavailable for fetch frame, check usage !");
        return nullptr;
    }

    std::vector<std::shared_ptr<AVSharedMemory>> outFrames;
    int32_t ret = FetchFrameInternel(timeUs, option, 1, param, outFrames);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "fetch frame failed");

    MEDIA_LOGD("exit FetchFrameAtTime");
    return outFrames[0];
}

GValueArray *AVMetadataHelperEngineGstImpl::OnNotifyAutoPlugSort(GValueArray &factories)
{
    GValueArray *result = g_value_array_new(factories.n_values);

    for (uint32_t i = 0; i < factories.n_values; i++) {
        GstElementFactory *factory =
            static_cast<GstElementFactory *>(g_value_get_object(g_value_array_get_nth(&factories, i)));
        GValue val = G_VALUE_INIT;
        g_value_init(&val, G_TYPE_OBJECT);
        g_value_set_object(&val, factory);
        result = g_value_array_append(result, &val);
        g_value_unset(&val);
    }
    return result;
}

int32_t AVMetadataHelperEngineGstImpl::SetSourceInternel(const std::string &uri, int32_t usage)
{
    Reset();
    ON_SCOPE_EXIT(0) { Reset(); };

    uint8_t renderMode = IPlayBinCtrler::PlayBinRenderMode::NATIVE_STREAM;
    renderMode = renderMode | IPlayBinCtrler::PlayBinRenderMode::DISABLE_TEXT;
    auto notifier = std::bind(&AVMetadataHelperEngineGstImpl::OnNotifyMessage, this, std::placeholders::_1);
    sinkProvider_ = std::make_shared<AVMetaSinkProvider>(usage);

    IPlayBinCtrler::PlayBinCreateParam createParam = {
        static_cast<IPlayBinCtrler::PlayBinRenderMode>(renderMode), notifier, sinkProvider_
    };

    playBinCtrler_ = IPlayBinCtrler::Create(IPlayBinCtrler::PlayBinKind::PLAYBIN2, createParam);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_UNKNOWN, "Failed to call IPlayBinCtrler::Create");

    int32_t ret = MSERR_INVALID_VAL;
    if (appsrcWrap_ != nullptr) {
        ret = playBinCtrler_->SetSource(appsrcWrap_);
    } else {
        ret = playBinCtrler_->SetSource(uri);
    }
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call playBinCtrler_->SetSource");

    metaCollector_ = std::make_unique<AVMetaMetaCollector>();
    auto listener = std::bind(&AVMetadataHelperEngineGstImpl::OnNotifyElemSetup, this, std::placeholders::_1);
    playBinCtrler_->SetElemSetupListener(listener);

    listener = std::bind(&AVMetadataHelperEngineGstImpl::OnNotifyElemUnSetup, this, std::placeholders::_1);
    playBinCtrler_->SetElemUnSetupListener(listener);

    auto autoPlugSortListener =
        std::bind(&AVMetadataHelperEngineGstImpl::OnNotifyAutoPlugSort, this, std::placeholders::_1);
    playBinCtrler_->SetAutoPlugSortListener(autoPlugSortListener);

    if (usage == AVMetadataUsage::AV_META_USAGE_PIXEL_MAP) {
        auto vidSink = sinkProvider_->CreateVideoSink();
        CHECK_AND_RETURN_RET_LOG(vidSink != nullptr, MSERR_UNKNOWN, "get video sink failed");
        frameExtractor_ = std::make_unique<AVMetaFrameExtractor>();
        ret = frameExtractor_->Init(playBinCtrler_, *vidSink);
        if (ret != MSERR_OK) {
            MEDIA_LOGE("frameExtractor init failed");
        }
    }

    metaCollector_->Start();
    usage_ = usage;

    ret = PrepareInternel(true);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call PrepareInternel");

    std::string mimeType = metaCollector_->GetMetadata(AV_KEY_MIME_TYPE);
    if (mimeType.empty()) {
        MEDIA_LOGE("can not recognize the media source's mimetype, set source failed");
        Reset();
        return MSERR_INVALID_OPERATION;
    }

    CANCEL_SCOPE_EXIT_GUARD(0);
    return MSERR_OK;
}

int32_t AVMetadataHelperEngineGstImpl::PrepareInternel(bool async)
{
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_OPERATION, "set source firstly");

    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (status_ == PLAYBIN_STATE_PREPARED || status_ == PLAYBIN_STATE_PLAYING || status_ == PLAYBIN_STATE_PAUSED) {
            return MSERR_OK;
        }
    }
    asyncDone_ = false;
    int32_t ret = playBinCtrler_->PrepareAsync();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "prepare failed");

    if (!async) {
        metaCollector_->Stop(true);
        static constexpr int32_t timeout = 3;
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait_for(lock, std::chrono::seconds(timeout), [this]() {
            return (status_ == PLAYBIN_STATE_PREPARED && asyncDone_) || errHappened_;
        });
        CHECK_AND_RETURN_RET_LOG(!errHappened_, MSERR_UNKNOWN, "prepare failed");
    }

    return MSERR_OK;
}

int32_t AVMetadataHelperEngineGstImpl::FetchFrameInternel(int64_t timeUsOrIndex, int32_t option, int32_t numFrames,
    const OutputConfiguration &param, std::vector<std::shared_ptr<AVSharedMemory>> &outFrames)
{
    (void)numFrames;

    if (!CheckFrameFetchParam(timeUsOrIndex, option, param)) {
        MEDIA_LOGE("fetch frame's param invalid");
        return MSERR_INVALID_OPERATION;
    }

    CHECK_AND_RETURN_RET_LOG(frameExtractor_ != nullptr, MSERR_INVALID_OPERATION, "frameExtractor is nullptr");

    int32_t ret = ExtractMetadata();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call ExtractMetadata");

    if (collectedMeta_.find(AV_KEY_HAS_VIDEO) == collectedMeta_.end() ||
        collectedMeta_[AV_KEY_HAS_VIDEO] != "yes") {
        MEDIA_LOGE("There is no video track in the current media source !");
        return MSERR_INVALID_OPERATION;
    }

    if (!metaCollector_->IsCollecteCompleted()) {
        MEDIA_LOGE("extract meta failed, exit");
        return MSERR_UNKNOWN;
    }

    ret = PrepareInternel(false);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call PrepareInternel");

    auto frame = frameExtractor_->ExtractFrame(timeUsOrIndex, option, param);
    if (frame == nullptr) {
        MEDIA_LOGE("fetch frame failed");
        return MSERR_UNKNOWN;
    }

    if (firstFetch_) {
        firstFetch_ = false;
    }

    outFrames.push_back(frame);
    return MSERR_OK;
}

int32_t AVMetadataHelperEngineGstImpl::ExtractMetadata()
{
    CHECK_AND_RETURN_RET_LOG(metaCollector_ != nullptr, MSERR_INVALID_OPERATION, "metaCollector is nullptr");

    if (!hasCollectMeta_) {
        collectedMeta_ = metaCollector_->GetMetadata();
        hasCollectMeta_ = true;
    }
    return MSERR_OK;
}

void AVMetadataHelperEngineGstImpl::Reset()
{
    // If call Reset immediately after setSource, the state of multiple plug-ins (for example, qtdemux)
    // will be confused, the ExtractMetadata call is added here to ensure stable state
    (void)ExtractMetadata();
    std::unique_lock<std::mutex> lock(mutex_);

    if (metaCollector_ != nullptr) {
        metaCollector_->Stop();
        hasCollectMeta_ = false;
    }

    if (frameExtractor_ != nullptr) {
        frameExtractor_->Reset();
    }

    if (playBinCtrler_ != nullptr) {
        playBinCtrler_->SetElemSetupListener(nullptr);
        playBinCtrler_->SetAutoPlugSortListener(nullptr);

        auto tmp = playBinCtrler_;
        playBinCtrler_ = nullptr;
        // Some msg maybe be reported by the playbinCtrler_ during the playbinCtler_ destroying.
        // unlock to avoid the deadlock.
        lock.unlock();
        tmp->Stop(false);
        tmp = nullptr;
        lock.lock();
    }

    sinkProvider_ = nullptr;
    metaCollector_ = nullptr;
    frameExtractor_ = nullptr;

    errHappened_ = false;
    status_ = PLAYBIN_STATE_IDLE;

    firstFetch_ = true;
    asyncDone_ = false;

    lock.unlock();
    lock.lock();
}

void AVMetadataHelperEngineGstImpl::OnNotifyMessage(const PlayBinMessage &msg)
{
    switch (msg.type) {
        case PLAYBIN_MSG_STATE_CHANGE: {
            std::unique_lock<std::mutex> lock(mutex_);
            status_ = msg.code;
            BehaviorEventWrite(
                GetStatusDescription(static_cast<OHOS::Media::PlayBinState>(status_)), "AVMetadata");
            cond_.notify_all();
            if (msg.code == PLAYBIN_STATE_PREPARED) {
                MEDIA_LOGI("prepare finished");
            }
            if (msg.code == PLAYBIN_STATE_STOPPED) {
                MEDIA_LOGI("stop finished");
            }
            break;
        }
        case PLAYBIN_MSG_ERROR: {
            std::unique_lock<std::mutex> lock(mutex_);
            errHappened_ = true;
            if (metaCollector_ != nullptr) {
                metaCollector_->Stop();
            }
            if (frameExtractor_ != nullptr) {
                frameExtractor_->Reset();
            }
            cond_.notify_all();
            MEDIA_LOGE("error happened, cancel inprocessing job");
            FaultEventWrite("error happened, cancel inprocessing job", "AVMetadata");
            break;
        }
        case PLAYBIN_MSG_SEEKDONE: {
            std::unique_lock<std::mutex> lock(mutex_);
            if (frameExtractor_ != nullptr) {
                frameExtractor_->NotifyPlayBinMsg(msg);
            }
            break;
        }
        case PLAYBIN_MSG_ASYNC_DONE: {
            asyncDone_ = true;
            cond_.notify_all();
            MEDIA_LOGI("async done");
            break;
        }
        default:
            break;
    }
}

void AVMetadataHelperEngineGstImpl::OnNotifyElemSetup(GstElement &elem)
{
    const gchar *metadata = gst_element_get_metadata(&elem, GST_ELEMENT_METADATA_KLASS);
    std::string metaStr(metadata);
    if (metaStr.find("Codec/Decoder/Video") != std::string::npos) {
        MEDIA_LOGI("Only need one frame!");
        g_object_set(const_cast<GstElement *>(&elem), "only-one-frame-required", TRUE, nullptr);
        MEDIA_LOGI("Set avmeta mode!");
        g_object_set(const_cast<GstElement *>(&elem), "metadata-mode", TRUE, nullptr);
    }

    if (metaStr.find("Codec/Decoder/Video/Hardware") != std::string::npos) {
        g_object_set(const_cast<GstElement *>(&elem), "player-scene", TRUE, nullptr);
    }

    std::unique_lock<std::mutex> lock(mutex_);
    if (metaCollector_ != nullptr) {
        metaCollector_->AddMetaSource(elem);
    }
}

void AVMetadataHelperEngineGstImpl::OnNotifyElemUnSetup(GstElement &elem)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (metaCollector_ != nullptr) {
        metaCollector_->RemoveMetaSource(elem);
    }
}

const std::string &AVMetadataHelperEngineGstImpl::GetStatusDescription(OHOS::Media::PlayBinState status)
{
    static const std::string ILLEGAL_STATE = "PLAYER_STATUS_ILLEGAL";
    if (status < OHOS::Media::PLAYBIN_STATE_IDLE || status > OHOS::Media::PLAYBIN_STATE_PLAYBACK_COMPLETE) {
        return ILLEGAL_STATE;
    }

    return AVMETADATA_STATE_MAP.find(status)->second;
}
} // namespace Media
} // namespace OHOS