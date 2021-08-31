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
#include "frame_converter.h"
#include "scope_guard.h"
#include "uri_helper.h"
#include "inner_meta_define.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetaEngineGstImpl"};
}

namespace OHOS {
namespace Media {
struct KeyToXMap {
    std::string_view keyName;
    int32_t innerKey;
};

#define METADATA_KEY_TO_X_MAP_ITEM(key, innerKey) { key, { #key, innerKey }}
static const std::unordered_map<int32_t, KeyToXMap> METAKEY_TO_X_MAP = {
    METADATA_KEY_TO_X_MAP_ITEM(AV_KEY_ALBUM, INNER_META_KEY_ALBUM),
    METADATA_KEY_TO_X_MAP_ITEM(AV_KEY_ALBUMARTIST, INNER_META_KEY_ALBUMARTIST),
    METADATA_KEY_TO_X_MAP_ITEM(AV_KEY_ARTIST, INNER_META_KEY_ARTIST),
    METADATA_KEY_TO_X_MAP_ITEM(AV_KEY_AUTHOR, INNER_META_KEY_AUTHOR),
    METADATA_KEY_TO_X_MAP_ITEM(AV_KEY_COMPOSER, INNER_META_KEY_COMPOSER),
    METADATA_KEY_TO_X_MAP_ITEM(AV_KEY_DURATION, INNER_META_KEY_DURATION),
    METADATA_KEY_TO_X_MAP_ITEM(AV_KEY_GENRE, INNER_META_KEY_GENRE),
    METADATA_KEY_TO_X_MAP_ITEM(AV_KEY_HAS_AUDIO, INNER_META_KEY_HAS_AUDIO),
    METADATA_KEY_TO_X_MAP_ITEM(AV_KEY_HAS_VIDEO, INNER_META_KEY_HAS_VIDEO),
    METADATA_KEY_TO_X_MAP_ITEM(AV_KEY_MIMETYPE, INNER_META_KEY_MIMETYPE),
    METADATA_KEY_TO_X_MAP_ITEM(AV_KEY_NUM_TRACKS, INNER_META_KEY_NUM_TRACKS),
    METADATA_KEY_TO_X_MAP_ITEM(AV_KEY_SAMPLERATE, INNER_META_KEY_SAMPLERATE),
    METADATA_KEY_TO_X_MAP_ITEM(AV_KEY_TITLE, INNER_META_KEY_TITLE),
    METADATA_KEY_TO_X_MAP_ITEM(AV_KEY_VIDEO_HEIGHT, INNER_META_KEY_VIDEO_HEIGHT),
    METADATA_KEY_TO_X_MAP_ITEM(AV_KEY_VIDEO_WIDTH, INNER_META_KEY_VIDEO_WIDTH),
};

static const std::unordered_map<int32_t, int32_t> INNER_META_KEY_TO_AVMETA_KEY_TABLE = {
    { INNER_META_KEY_ALBUM, AV_KEY_ALBUM },
    { INNER_META_KEY_ALBUMARTIST, AV_KEY_ALBUMARTIST },
    { INNER_META_KEY_ARTIST, AV_KEY_ARTIST },
    { INNER_META_KEY_AUTHOR, AV_KEY_AUTHOR },
    { INNER_META_KEY_COMPOSER, AV_KEY_COMPOSER },
    { INNER_META_KEY_DURATION, AV_KEY_DURATION },
    { INNER_META_KEY_GENRE, AV_KEY_GENRE },
    { INNER_META_KEY_HAS_AUDIO, AV_KEY_HAS_AUDIO },
    { INNER_META_KEY_HAS_VIDEO, AV_KEY_HAS_VIDEO },
    { INNER_META_KEY_MIMETYPE, AV_KEY_MIMETYPE },
    { INNER_META_KEY_NUM_TRACKS, AV_KEY_NUM_TRACKS },
    { INNER_META_KEY_SAMPLERATE, AV_KEY_SAMPLERATE },
    { INNER_META_KEY_TITLE, AV_KEY_TITLE },
    { INNER_META_KEY_VIDEO_HEIGHT, AV_KEY_VIDEO_HEIGHT },
    { INNER_META_KEY_VIDEO_WIDTH, AV_KEY_VIDEO_WIDTH },
};

AVMetadataHelperEngineGstImpl::AVMetadataHelperEngineGstImpl()
    : playbinState_(PLAYBIN_STATE_IDLE)
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

AVMetadataHelperEngineGstImpl::~AVMetadataHelperEngineGstImpl()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    Reset();
}

int32_t AVMetadataHelperEngineGstImpl::SetSource(const std::string &uri, int32_t usage)
{
    if ((usage != AVMetadataUsage::AV_META_USAGE_META_ONLY) &&
        (usage != AVMetadataUsage::AV_META_USAGE_PIXEL_MAP)) {
        MEDIA_LOGE("Invalid avmetadatahelper usage: %{public}d", usage);
        return MSERR_INVALID_VAL;
    }

    if (UriHelper(uri).FormatMe().UriType() != UriHelper::URI_TYPE_FILE) {
        MEDIA_LOGE("Unsupported uri type : %{public}s", uri.c_str());
        return MSERR_UNSUPPORT;
    }

    MEDIA_LOGI("uri: %{public}s, usage: %{public}d", uri.c_str(), usage);

    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(TakeOwnerShip(lock), {});
    ON_SCOPE_EXIT(0) { ReleaseOwnerShip(); };

    int32_t ret = InitPipeline(usage);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    ret = playBinCtrler_->SetSource(uri);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
    usage_ = usage;

    return MSERR_OK;
}

std::string AVMetadataHelperEngineGstImpl::ResolveMetadata(int32_t key)
{
    std::string result;

    if (METAKEY_TO_X_MAP.find(key) == METAKEY_TO_X_MAP.end()) {
        MEDIA_LOGE("Unsupported metadata key: %{public}d", key);
        return result;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(playBinCtrler_ != nullptr, result);

    CHECK_AND_RETURN_RET(TakeOwnerShip(lock), result);
    ON_SCOPE_EXIT(0) { ReleaseOwnerShip(); };

    int32_t ret = playBinCtrler_->SetScene(IPlayBinCtrler::PlayBinScene::PLAYBIN_SCENE_METADATA);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("internel error, can not resolve the meta, ret = %{public}d", ret);
        return result;
    }

    ret = PrepareInternel(true, lock);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, result);

    result = playBinCtrler_->GetMetadata(METAKEY_TO_X_MAP.at(key).innerKey);
    if (result.empty()) {
        MEDIA_LOGE("The specified metadata %{public}s cannot be obtained from the specified stream.",
                   METAKEY_TO_X_MAP.at(key).keyName.data());
    }

    return result;
}

std::unordered_map<int32_t, std::string> AVMetadataHelperEngineGstImpl::ResolveMetadata()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(playBinCtrler_ != nullptr, {});

    CHECK_AND_RETURN_RET(TakeOwnerShip(lock), {});
    ON_SCOPE_EXIT(0) { ReleaseOwnerShip(); };

    int32_t ret = playBinCtrler_->SetScene(IPlayBinCtrler::PlayBinScene::PLAYBIN_SCENE_METADATA);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("internel error, can not resolve the meta, ret = %{public}d", ret);
        return {};
    }

    ret = PrepareInternel(true, lock);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, {});

    // no need to block wait if the prepare process already finished by the FetchFrame action
    std::unordered_map<int32_t, std::string> tmpResult = playBinCtrler_->GetMetadata();
    if (tmpResult.empty()) {
        MEDIA_LOGE("Can not get all metedata from the specified stream.");
        return {};
    }

    std::unordered_map<int32_t, std::string> result;
    for (auto &item : tmpResult) {
        if (item.first >= INNER_META_KEY_BUTT || item.first < 0) {
            continue;
        }
        (void)result.emplace(INNER_META_KEY_TO_AVMETA_KEY_TABLE.at(item.first), item.second);
    }

    return result;
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperEngineGstImpl::FetchFrameAtTime(
    int64_t timeUs, int32_t option, OutputConfiguration param)
{
    if ((option != AV_META_QUERY_CLOSEST) && (option != AV_META_QUERY_CLOSEST_SYNC) &&
        (option != AV_META_QUERY_NEXT_SYNC) && (option != AV_META_QUERY_PREVIOUS_SYNC)) {
        MEDIA_LOGE("Invalid query option: %{public}d", option);
        return nullptr;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, nullptr, "Call SetSource firstly !");

    if (usage_ != AVMetadataUsage::AV_META_USAGE_PIXEL_MAP) {
        MEDIA_LOGE("current instance is unavaiable for pixel map, check usage !");
        return nullptr;
    }

    CHECK_AND_RETURN_RET(TakeOwnerShip(lock), nullptr);
    ON_SCOPE_EXIT(0) { ReleaseOwnerShip(); };

    int32_t ret = InitConverter(param);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, nullptr);

    ret = playBinCtrler_->SetScene(IPlayBinCtrler::PlayBinScene::PLAYBIN_SCENE_THUBNAIL);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, nullptr);

    ret = PrepareInternel(false, lock);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, nullptr);

    ret = SeekInternel(timeUs, option, lock);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, nullptr);

    std::shared_ptr<AVSharedMemory> frame = converter_->GetOneFrame(); // need exception awaken up.
    CHECK_AND_RETURN_RET(ret == MSERR_OK, nullptr);

    return frame;
}

int32_t AVMetadataHelperEngineGstImpl::InitPipeline(int32_t usage)
{
    DoReset();

    auto notifier = std::bind(&AVMetadataHelperEngineGstImpl::OnNotifyMessage, this, std::placeholders::_1);
    auto playBinCtrler = IPlayBinCtrler::Create(IPlayBinCtrler::PlayBinKind::PLAYBIN_KIND_PLAYBIN2, notifier);
    CHECK_AND_RETURN_RET(playBinCtrler != nullptr, MSERR_UNKNOWN);

    auto sinkProvider = std::make_shared<AVMetaSinkProvider>(usage);
    int32_t ret = playBinCtrler->SetSinkProvider(sinkProvider);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    playBinCtrler_ = std::move(playBinCtrler);
    sinkProvider_ = sinkProvider;

    return MSERR_OK;
}

int32_t AVMetadataHelperEngineGstImpl::InitConverter(const OutputConfiguration &config)
{
    if (converter_ == nullptr) {
        converter_ = std::make_shared<FrameConverter>();
        auto provider = std::static_pointer_cast<AVMetaSinkProvider>(sinkProvider_);
        provider->SetFrameCallback(converter_);
    }

    // need to skip the same config
    int32_t ret = converter_->Init(config);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, MSERR_INVALID_OPERATION);

    ret = converter_->StartConvert();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, MSERR_INVALID_OPERATION);

    return MSERR_OK;
}

bool AVMetadataHelperEngineGstImpl::TakeOwnerShip(std::unique_lock<std::mutex> &lock, bool canceling)
{
    if (canceling) {
        cond_.wait(lock, [this]() { return !inProgressing_; });
    } else {
        cond_.wait(lock, [this]() { return canceled_ || !inProgressing_; });
    }

    if (!canceling && canceled_) {
        MEDIA_LOGI("canceling: %{public}d, canceled_: %{public}d", canceling, canceled_);
        return false;
    }
    inProgressing_ = true;

    return true;
}

void AVMetadataHelperEngineGstImpl::ReleaseOwnerShip()
{
    inProgressing_ = false;
    cond_.notify_all();
}

int32_t AVMetadataHelperEngineGstImpl::PrepareInternel(bool async, std::unique_lock<std::mutex> &lock)
{
    if (playbinState_ < PLAYBIN_STATE_PREPARING) {
        int32_t ret = playBinCtrler_->PrepareAsync();
        CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
        playbinState_ = PLAYBIN_STATE_PREPARING;
    }

    if (!async) {
        cond_.wait(lock, [this]() { return canceled_ || playbinState_ >= PLAYBIN_STATE_PREPARED; });
        CHECK_AND_RETURN_RET_LOG(!canceled_, MSERR_UNKNOWN, "Canceled !");
    }

    return MSERR_OK;
}

int32_t AVMetadataHelperEngineGstImpl::SeekInternel(int64_t timeUs, int32_t option, std::unique_lock<std::mutex> &lock)
{
    int32_t ret = playBinCtrler_->Seek(timeUs, option);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    cond_.wait(lock, [this]() { return canceled_ || seekDone_; });
    CHECK_AND_RETURN_RET_LOG(!canceled_, MSERR_UNKNOWN, "Canceled !");
    seekDone_ = false;

    return MSERR_OK;
}

void AVMetadataHelperEngineGstImpl::DoReset()
{
    if (playBinCtrler_ != nullptr) {
        (void)playBinCtrler_->Stop();
        playBinCtrler_ = nullptr;
        sinkProvider_ = nullptr;
    }

    if (converter_ != nullptr) {
        (void)converter_->StopConvert();
        converter_ = nullptr;
    }
}

void AVMetadataHelperEngineGstImpl::Reset()
{
    std::unique_lock<std::mutex> lock(mutex_);
    DoReset();
    canceled_ = true;
    cond_.notify_all();
}

void AVMetadataHelperEngineGstImpl::OnNotifyMessage(const PlayBinMessage &msg)
{
    switch (msg.type) {
        case PLAYBIN_MSG_ERROR: {
            MEDIA_LOGE("internel error happened, reset");
            Reset();
            break;
        }
        case PLAYBIN_MSG_STATE_CHANGE: {
            std::unique_lock<std::mutex> lock(mutex_);
            playbinState_ = msg.code;
            if (msg.code == PLAYBIN_STATE_PREPARED) {
                cond_.notify_one();
            }
            break;
        }
        case PLAYBIN_MSG_SEEKDONE: {
            std::unique_lock<std::mutex> lock(mutex_);
            seekDone_ = true;
            cond_.notify_one();
            break;
        }
        default:
            break;
    }
}
}
}