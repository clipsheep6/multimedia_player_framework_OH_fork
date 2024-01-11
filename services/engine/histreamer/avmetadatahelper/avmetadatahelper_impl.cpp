/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "avmetadatahelper_impl.h"

#include "buffer/avbuffer_common.h"
#include "common/media_source.h"
#include "ibuffer_consumer_listener.h"
#include "image_source.h"
#include "graphic_common_c.h"
#include "media_errors.h"
#include "media_log.h"
#include "media_description.h"
#include "meta/meta.h"
#include "meta/meta_key.h"
#include "sync_fence.h"
#include "uri_helper.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetadataHelperImpl"};
}

namespace OHOS {
namespace Media {
namespace {
const std::set<PixelFormat> SUPPORTED_PIXELFORMAT = {
    PixelFormat::RGB_565,
    PixelFormat::RGB_888,
    PixelFormat::RGBA_8888
};

constexpr uint64_t DECODER_USAGE = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA
    | BUFFER_USAGE_VIDEO_DECODER;
}

class FetchedFrameConsumerListener : public IBufferConsumerListener {
public:
    explicit FetchedFrameConsumerListener(AVMetadataHelperImpl *helperImpl) : helperImpl_(helperImpl) {}
    ~FetchedFrameConsumerListener() override = default;

    void OnBufferAvailable() override
    {
        MEDIA_LOGI("FetchedFrameConsumerListener OnBufferAvailable");
        helperImpl_->OnFetchedFrameBufferAvailable();
    }

private:
    AVMetadataHelperImpl *helperImpl_;
};

class MetadataHelperCodecCallback : public OHOS::MediaAVCodec::MediaCodecCallback {
public:
    explicit MetadataHelperCodecCallback(AVMetadataHelperImpl *helperImpl) : helperImpl_(helperImpl) {}

    ~MetadataHelperCodecCallback() = default;

    void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode) override
    {
        helperImpl_->OnError(errorType, errorCode);
    }

    void OnOutputFormatChanged(const MediaAVCodec::Format &format) override
    {
        helperImpl_->OnOutputFormatChanged(format);
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        helperImpl_->OnInputBufferAvailable(index, buffer);
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        helperImpl_->OnOutputBufferAvailable(index, buffer);
    }

private:
    AVMetadataHelperImpl *helperImpl_;
};

class HelperEventReceiver : public Pipeline::EventReceiver {
public:
    explicit HelperEventReceiver(AVMetadataHelperImpl *helperImpl) : helperImpl_(helperImpl)
    {
    }

    void OnEvent(const Event &event)
    {
        helperImpl_->OnEvent(event);
    }

private:
    AVMetadataHelperImpl* helperImpl_;
};

class HelperFilterCallback : public Pipeline::FilterCallback {
public:
    explicit HelperFilterCallback(AVMetadataHelperImpl* helperImpl) : helperImpl_(helperImpl)
    {
    }

    void OnCallback(const std::shared_ptr<Pipeline::Filter>& filter, Pipeline::FilterCallBackCommand cmd,
        Pipeline::StreamType outType)
    {
        helperImpl_->OnCallback(filter, cmd, outType);
    }

private:
    AVMetadataHelperImpl* helperImpl_;
};

void AVMetadataHelperImpl::OnEvent(const Event &event)
{
}

void AVMetadataHelperImpl::OnCallback(std::shared_ptr<Pipeline::Filter> filter,
    const Pipeline::FilterCallBackCommand cmd, Pipeline::StreamType outType)
{
}

void AVMetadataHelperImpl::OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode)
{
    MEDIA_LOGI("OnError errorType:%{public}d, errorCode:%{public}d",
        static_cast<int32_t>(errorType), errorCode);
    Destroy();
}

void AVMetadataHelperImpl::OnOutputFormatChanged(const MediaAVCodec::Format &format)
{
    MEDIA_LOGI("OnOutputFormatChanged");
    outputFormat_ = format;
}

void AVMetadataHelperImpl::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    MEDIA_LOGD("OnInputBufferAvailable index:%{public}u", index);
    if (stopProcessing_.load()) {
        MEDIA_LOGI("Has stopped processing, will not queue input buffer.");
        return;
    }
    CHECK_AND_RETURN_LOG(mediaDemuxer_ != nullptr, "OnInputBufferAvailable demuxer is nullptr.");
    mediaDemuxer_->ReadSample(trackIndex_, buffer);
    CHECK_AND_RETURN_LOG(videoDecoder_ != nullptr, "OnInputBufferAvailable decoder is nullptr.");
    videoDecoder_->QueueInputBuffer(index);
}

void AVMetadataHelperImpl::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    MEDIA_LOGD("OnOutputBufferAvailable index:%{public}u", index);
    if (stopProcessing_.load()) {
        MEDIA_LOGI("Has stopped processing, will not release output buffer.");
        return;
    }
    CHECK_AND_RETURN_LOG(videoDecoder_ != nullptr, "OnOutputBufferAvailable videoDecoder_ is nullptr");
    videoDecoder_->ReleaseOutputBuffer(index, true);
}

void AVMetadataHelperImpl::OnFetchedFrameBufferAvailable()
{
    CHECK_AND_RETURN_LOG(consumerSurface_ != nullptr, "consumerSurface_ is nullptr");
    sptr<SurfaceBuffer> surfaceBuffer;
    sptr<SyncFence> fence;
    int64_t timestamp;
    OHOS::Rect damage;
    GSError err = consumerSurface_->AcquireBuffer(surfaceBuffer, fence, timestamp, damage);
    if (err != GSERROR_OK || surfaceBuffer == nullptr) {
        MEDIA_LOGW("OnFetchedFrameBufferAvailable AcquireBuffer failed, err:%{public}d", err);
        return;
    }
    MEDIA_LOGD("OnFetchedFrameBufferAvailable AcquireBuffer OK timestamp:%{public}" PRId64"", timestamp);

    if (timestamp >= seekTime_ && !hasFetchedFrame_.load()) {
        ConvertToAVSharedMemory(surfaceBuffer);
        hasFetchedFrame_ = true;
        cond_.notify_all();
    }
    err = consumerSurface_->ReleaseBuffer(surfaceBuffer, -1);
    CHECK_AND_RETURN_LOG(err == GSERROR_OK, "ReleaseBuffer failed, err:%{public}d", err);
}

std::unique_ptr<PixelMap> AVMetadataHelperImpl::GetYuvDataAlignStride(const sptr<SurfaceBuffer> &surfaceBuffer)
{
    int32_t width = surfaceBuffer->GetWidth();
    int32_t height = surfaceBuffer->GetHeight();
    int32_t outputWidth;
    int32_t outputHeight;
    outputFormat_.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, outputWidth);
    outputFormat_.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, outputHeight);
    MEDIA_LOGI("GetYuvDataAlignStride outputWidth:%{public}d, outputHeight:%{public}d",
        outputWidth, outputHeight);
    
    InitializationOptions initOpts;
    initOpts.size = {width, height};
    initOpts.srcPixelFormat = PixelFormat::NV12;
    std::unique_ptr<PixelMap> yuvPixelMap = PixelMap::Create(initOpts);
    uint8_t *dstData = static_cast<uint8_t *>(yuvPixelMap->GetWritablePixels());
    uint8_t *srcPtr = static_cast<uint8_t *>(surfaceBuffer->GetVirAddr());
    uint8_t *dstPtr = dstData;

    // copy src Y component to dst
    for (int32_t y = 0; y < height; y++) {
        auto ret = memcpy_s(dstPtr, width, srcPtr, width);
        if (ret != EOK) {
            MEDIA_LOGW("Memcpy Y component failed.");
        }
        srcPtr += outputWidth;
        dstPtr += width;
    }

    srcPtr = static_cast<uint8_t *>(surfaceBuffer->GetVirAddr()) + outputWidth * outputHeight;

    // copy src UV component to dst, height(UV) = height(Y) / 2
    for (int32_t uv = 0; uv < height / 2; uv++) {
        auto ret = memcpy_s(dstPtr, width, srcPtr, width);
        if (ret != EOK) {
            MEDIA_LOGW("Memcpy UV component failed.");
        }
        srcPtr += outputWidth;
        dstPtr += width;
    }

    return yuvPixelMap;
}

bool AVMetadataHelperImpl::ConvertToAVSharedMemory(const sptr<SurfaceBuffer> &surfaceBuffer)
{
    int32_t format = surfaceBuffer->GetFormat();
    int32_t size = surfaceBuffer->GetSize();
    int32_t stride = surfaceBuffer->GetStride();
    int32_t width = surfaceBuffer->GetWidth();
    int32_t height = surfaceBuffer->GetHeight();
    MEDIA_LOGI("Convert to AVSharedMemory format:%{public}d, size:%{public}d, stride:%{public}d, "
        "width:%{public}d, height:%{public}d", format, size, stride, width, height);

    std::unique_ptr<PixelMap> yuvPixelMap = GetYuvDataAlignStride(surfaceBuffer);
    SourceOptions srcOpts;
    srcOpts.pixelFormat = PixelFormat::NV12;
    srcOpts.size = {width, height};
    uint32_t errorCode;
    std::unique_ptr<ImageSource> imageSource =
        ImageSource::CreateImageSource(yuvPixelMap->GetPixels(), size, srcOpts, errorCode);
    CHECK_AND_RETURN_RET_LOG(errorCode == 0, false, "CreateImageSource failed:%{public}d", errorCode);

    DecodeOptions decodeOpts;
    decodeOpts.desiredSize = {
        outputConfig_.dstWidth > 0 ? outputConfig_.dstWidth : width,
        outputConfig_.dstHeight > 0 ? outputConfig_.dstHeight : height
    };
    decodeOpts.desiredPixelFormat = outputConfig_.colorFormat;
    std::unique_ptr<PixelMap> pixelMap = imageSource->CreatePixelMapEx(0, decodeOpts, errorCode);
    CHECK_AND_RETURN_RET_LOG(errorCode == 0, false, "CreatePixelMapEx failed:%{public}d", errorCode);

    fetchedFrameAtTime_ = std::make_shared<AVSharedMemoryBase>(sizeof(OutputFrame) + pixelMap->GetRowStride() *
        pixelMap->GetHeight(), AVSharedMemory::Flags::FLAGS_READ_WRITE, "FetchedFrameMemory");
    
    int32_t ret = fetchedFrameAtTime_->Init();
    CHECK_AND_RETURN_RET_LOG(ret == static_cast<int32_t>(Status::OK), false,
        "Create AVSharedmemory failed, ret:%{public}d", ret);
    OutputFrame *frame = reinterpret_cast<OutputFrame *>(fetchedFrameAtTime_->GetBase());
    frame->width_ = pixelMap->GetWidth();
    frame->height_ = pixelMap->GetHeight();
    frame->stride_ = pixelMap->GetRowStride();
    frame->bytesPerPixel_ = pixelMap->GetPixelBytes();
    frame->size_ = pixelMap->GetRowStride() * pixelMap->GetHeight();
    fetchedFrameAtTime_->Write(pixelMap->GetPixels(), frame->size_, sizeof(OutputFrame));
    return true;
}

AVMetadataHelperImpl::AVMetadataHelperImpl()
{
    MEDIA_LOGI("Constructor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    metaCollector_ = std::make_shared<AVMetaDataCollector>();
}

AVMetadataHelperImpl::~AVMetadataHelperImpl()
{
    MEDIA_LOGI("Destructor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    Destroy();
}

int32_t AVMetadataHelperImpl::SetSource(const std::string &uri, int32_t usage)
{
    if ((usage != AVMetadataUsage::AV_META_USAGE_META_ONLY) &&
        (usage != AVMetadataUsage::AV_META_USAGE_PIXEL_MAP)) {
        MEDIA_LOGE("Invalid avmetadatahelper usage: %{public}d", usage);
        return MSERR_INVALID_VAL;
    }
    UriHelper uriHelper(uri);
    if (uriHelper.UriType() != UriHelper::URI_TYPE_FILE && uriHelper.UriType() != UriHelper::URI_TYPE_FD) {
        MEDIA_LOGE("Unsupported uri type : %{private}s", uri.c_str());
        return MSERR_UNSUPPORT;
    }

    usage_ = usage;
    MEDIA_LOGI("SetSource uri: %{private}s, type:%{public}d, usage: %{public}d", uri.c_str(),
        uriHelper.UriType(), usage);

    Status ret = SetSourceInternel(uri, usage);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, MSERR_INVALID_VAL, "Failed to call SetSourceInternel");
    return MSERR_OK;
}

int32_t AVMetadataHelperImpl::SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    MEDIA_LOGI("SetSource dataSrc");
    mediaDemuxer_ = std::make_shared<MediaDemuxer>();
    Status ret = SetSourceInternel(dataSrc);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, MSERR_INVALID_VAL, "Failed to call SetSourceInternel");

    MEDIA_LOGI("set source success");
    return MSERR_OK;
}

std::string AVMetadataHelperImpl::ResolveMetadata(int32_t key)
{
    MEDIA_LOGI("enter ResolveMetadata with key: %{public}d", key);
    std::string result;

    int32_t ret = ExtractMetadata();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, result, "Failed to call ExtractMetadata");

    auto it = collectedMeta_.find(key);
    if (it == collectedMeta_.end() || it->second.empty()) {
        MEDIA_LOGE("The specified metadata %{public}d cannot be obtained from the specified stream.", key);
        return result;
    }

    MEDIA_LOGI("exit ResolveMetadata with key");
    result = collectedMeta_[key];
    return result;
}

std::unordered_map<int32_t, std::string> AVMetadataHelperImpl::ResolveMetadata()
{
    MEDIA_LOGI("enter ResolveMetadata");

    int32_t ret = ExtractMetadata();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, {}, "Failed to call ExtractMetadata");

    MEDIA_LOGI("exit ResolveMetadata");
    return collectedMeta_;
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperImpl::FetchArtPicture()
{
    MEDIA_LOGI("enter FetchArtPicture");
    if (collectedArtPicture_ != nullptr) {
        MEDIA_LOGI("Repeated ExtractArtPicture");
        return collectedArtPicture_;
    }
    if (mediaDemuxer_ == nullptr) {
        MEDIA_LOGE("ExtractArtPicutre Failed : mediaDemuxer_ is nullptr");
        return nullptr;
    }
    const std::vector<std::shared_ptr<Meta>> trackInfos = mediaDemuxer_->GetStreamMetaInfo();
    collectedArtPicture_ = metaCollector_->GetArtPicture(trackInfos);
    if (collectedArtPicture_ == nullptr) {
        MEDIA_LOGE("ExtractArtPicutre Failed");
    }
    return collectedArtPicture_;
}

std::shared_ptr<Meta> AVMetadataHelperImpl::GetTargetTrackInfo()
{
    CHECK_AND_RETURN_RET_LOG(mediaDemuxer_ != nullptr, nullptr, "GetTargetTrackInfo demuxer is nullptr");
    std::vector<std::shared_ptr<Meta>> trackInfos = mediaDemuxer_->GetStreamMetaInfo();
    size_t trackCount = trackInfos.size();
    CHECK_AND_RETURN_RET_LOG(trackCount > 0, nullptr, "GetTargetTrackInfo trackCount is invalid");
    for (size_t index = 0; index < trackCount; index++) {
        if (!(trackInfos[index]->GetData(Tag::MIME_TYPE, trackMime_))) {
            MEDIA_LOGW("GetTargetTrackInfo get mime type failed %{public}s", trackMime_.c_str());
            continue;
        }
        if (trackMime_.find("video/") == 0) {
            Plugins::MediaType mediaType;
            CHECK_AND_RETURN_RET_LOG(trackInfos[index]->GetData(Tag::MEDIA_TYPE, mediaType), nullptr,
                "GetTargetTrackInfo failed to get mediaType, index:%{public}d", index);
            MEDIA_LOGI("GetData mediaType:%{public}d", static_cast<int32_t>(mediaType));
            CHECK_AND_RETURN_RET_LOG(mediaType == Plugins::MediaType::VIDEO, nullptr,
                "GetTargetTrackInfo mediaType is not video, index:%{public}d", index);
            trackIndex_ = index;
            MEDIA_LOGI("GetTargetTrackInfo success trackIndex_:%{public}d, trackMime_:%{public}s",
                trackIndex_, trackMime_.c_str());
            return trackInfos[trackIndex_];
        }
    }
    MEDIA_LOGW("GetTargetTrackInfo FAILED.");
    return nullptr;
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperImpl::FetchFrameAtTime(
    int64_t timeUs, int32_t option, const OutputConfiguration &param)
{
    MEDIA_LOGI("Fetch frame 0x%{public}06" PRIXPTR " timeUs:%{public}" PRId64", option:%{public}d,"
        "dstWidth:%{public}d, dstHeight:%{public}d, colorFormat:%{public}d", FAKE_POINTER(this),
        timeUs, option, param.dstWidth, param.dstHeight, static_cast<int32_t>(param.colorFormat));
    CHECK_AND_RETURN_RET_LOG(usage_ == AVMetadataUsage::AV_META_USAGE_PIXEL_MAP, nullptr,
        "Fetch frame usage invalid");
    CHECK_AND_RETURN_RET_LOG(mediaDemuxer_ != nullptr, nullptr, "FetchFrameAtTime demuxer is nullptr");

    hasFetchedFrame_ = false;
    outputConfig_ = param;
    seekTime_ = timeUs;
    if (trackInfo_ == nullptr) {
        trackInfo_ = GetTargetTrackInfo();
    }
    CHECK_AND_RETURN_RET_LOG(trackInfo_ != nullptr, nullptr, "FetchFrameAtTime trackInfo_ is nullptr.");

    CHECK_AND_RETURN_RET_LOG(InitSurface() == Status::OK, nullptr, "FetchFrameAtTime InitSurface failed.");
    CHECK_AND_RETURN_RET_LOG(InitDecoder() == Status::OK, nullptr, "FetchFrameAtTime InitDecoder failed.");

    mediaDemuxer_->SelectTrack(trackIndex_);
    int64_t realSeekTime = timeUs;
    mediaDemuxer_->SeekTo(timeUs, static_cast<Plugins::SeekMode>(option), realSeekTime);
    MEDIA_LOGI("FetchFrameAtTime realSeekTime:%{public}" PRId64"", realSeekTime);
    {
        std::unique_lock<std::mutex> lock(mutex_);

        // wait up to 3s to fetch frame AVSharedMemory at time.
        if (cond_.wait_for(lock, std::chrono::seconds(3), [this] {return hasFetchedFrame_.load();})) {
            MEDIA_LOGI("Fetch frame OK srcUri_:%{private}s, width:%{public}d, height:%{public}d",
                srcUri_.c_str(), outputConfig_.dstWidth, outputConfig_.dstHeight);
        } else {
            hasFetchedFrame_ = true;
            MEDIA_LOGI("Fetch frame timeout srcUri_:%{private}s, width:%{public}d, height:%{public}d",
                srcUri_.c_str(), outputConfig_.dstWidth, outputConfig_.dstHeight);
        }
    }
    
    return fetchedFrameAtTime_;
}

Status AVMetadataHelperImpl::SetSourceInternel(const std::string &uri, int32_t usage)
{
    Reset();
    srcUri_ = uri;
    mediaDemuxer_ = std::shared_ptr<MediaDemuxer>(new(std::nothrow) MediaDemuxer());
    CHECK_AND_RETURN_RET_LOG(mediaDemuxer_ != nullptr, Status::ERROR_INVALID_DATA,
        "SetSourceInternel demuxer is nullptr");
    mediaDemuxer_->SetEventReceiver(std::make_shared<HelperEventReceiver>(this));
    Status ret = mediaDemuxer_->SetDataSource(std::make_shared<MediaSource>(srcUri_));
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, ret, "SetSourceInternel demuxer failed to call SetDataSource");
    return Status::OK;
}

Status AVMetadataHelperImpl::SetSourceInternel(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    Status ret = mediaDemuxer_->SetDataSource(std::make_shared<MediaSource>(dataSrc));
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, ret, "Failed to call SetDataSource");
    return Status::OK;
}

int32_t AVMetadataHelperImpl::ExtractMetadata()
{
    CHECK_AND_RETURN_RET_LOG(mediaDemuxer_ != nullptr, MSERR_INVALID_OPERATION, "mediaDemuxer_ is nullptr");

    if (!hasCollectMeta_) {
        const std::shared_ptr<Meta> globalInfo = mediaDemuxer_->GetGlobalMetaInfo();
        const std::vector<std::shared_ptr<Meta>> trackInfos = mediaDemuxer_->GetStreamMetaInfo();
        collectedMeta_ = metaCollector_->GetMetadata(globalInfo, trackInfos);
        hasCollectMeta_ = true;
    }
    return MSERR_OK;
}

void AVMetadataHelperImpl::Reset()
{
    if (mediaDemuxer_ != nullptr) {
        mediaDemuxer_->Reset();
    }

    if (videoDecoder_ != nullptr) {
        videoDecoder_->Reset();
    }

    hasFetchedFrame_ = false;
    trackInfo_ = nullptr;
    hasCollectMeta_ = false;
    collectedArtPicture_ = nullptr;

    errHappened_ = false;
    firstFetch_ = true;
}

void AVMetadataHelperImpl::Destroy()
{
    stopProcessing_ = true;

    if (videoDecoder_ != nullptr) {
        videoDecoder_->Stop();
        videoDecoder_->Release();
    }

    MEDIA_LOGI("Finish Destroy.");
}

Status AVMetadataHelperImpl::InitDecoder()
{
    if (videoDecoder_ != nullptr) {
        MEDIA_LOGD("InitDecoder already.");
        return Status::OK;
    }
    MEDIA_LOGI("Init decoder start.");
    videoDecoder_ = MediaAVCodec::VideoDecoderFactory::CreateByMime(trackMime_);
    CHECK_AND_RETURN_RET_LOG(videoDecoder_ != nullptr, Status::ERROR_NO_MEMORY,
        "Create videoDecoder_ is nullptr");
    Format trackFormat {};
    trackFormat.SetMeta(trackInfo_);
    int32_t width;
    int32_t height;
    trackFormat.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    trackFormat.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    MEDIA_LOGI("Init decoder trackFormat width:%{public}d, height:%{public}d", width, height);
    trackFormat.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(Plugins::VideoPixelFormat::NV12));
    videoDecoder_->Configure(trackFormat);
    std::shared_ptr<MediaAVCodec::MediaCodecCallback> mediaCodecCallback =
        std::make_shared<MetadataHelperCodecCallback>(this);
    videoDecoder_->SetCallback(mediaCodecCallback);
    videoDecoder_->SetOutputSurface(producerSurface_);
    videoDecoder_->Prepare();
    videoDecoder_->Start();
    MEDIA_LOGI("Init decoder success.");
    return Status::OK;
}

Status AVMetadataHelperImpl::InitSurface()
{
    if (producerSurface_ != nullptr) {
        MEDIA_LOGD("InitSurface already.");
        return Status::OK;
    }
    MEDIA_LOGD("Init surface start.");
    consumerSurface_ = Surface::CreateSurfaceAsConsumer("AVMetadataHelperSurface");
    CHECK_AND_RETURN_RET_LOG(consumerSurface_ != nullptr, Status::ERROR_NO_MEMORY,
        "InitSurface create consumer surface failed.");

    GSError err = consumerSurface_->SetDefaultUsage(DECODER_USAGE);
    CHECK_AND_RETURN_RET_LOG(err == GSERROR_OK, Status::ERROR_INVALID_OPERATION,
        "InitSurface SetDefaultUsage failed.");
    sptr<IBufferConsumerListener> listener = new FetchedFrameConsumerListener(this);
    err = consumerSurface_->RegisterConsumerListener(listener);
    CHECK_AND_RETURN_RET_LOG(err == GSERROR_OK, Status::ERROR_INVALID_OPERATION,
        "InitSurface RegisterConsumerListener failed.");

    sptr<IBufferProducer> producer = consumerSurface_->GetProducer();
    producerSurface_ = Surface::CreateSurfaceAsProducer(producer);
    CHECK_AND_RETURN_RET_LOG(producerSurface_ != nullptr, Status::ERROR_NO_MEMORY,
        "InitSurface create producer surface failed.");
    MEDIA_LOGD("Init surface success.");
    return Status::OK;
}
} // namespace Media
} // namespace OHOS