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

#include "av_thumbnail_generator.h"

#include "buffer/avbuffer_common.h"
#include "common/media_source.h"
#include "ibuffer_consumer_listener.h"
#include "graphic_common_c.h"
#include "media_errors.h"
#include "media_log.h"
#include "media_description.h"
#include "meta/meta.h"
#include "meta/meta_key.h"
#include "plugin/plugin_time.h"
#include "sync_fence.h"
#include "uri_helper.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN, "AVThumbnailGenerator" };
}

namespace OHOS {
namespace Media {
constexpr int32_t RATE_UV = 2;
constexpr int32_t SHIFT_BITS_P010_2_NV12 = 8;

class ThumnGeneratorCodecCallback : public OHOS::MediaAVCodec::MediaCodecCallback {
public:
    explicit ThumnGeneratorCodecCallback(AVThumbnailGenerator *generator) : generator_(generator) {}

    ~ThumnGeneratorCodecCallback() = default;

    void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode) override
    {
        generator_->OnError(errorType, errorCode);
    }

    void OnOutputFormatChanged(const MediaAVCodec::Format &format) override
    {
        generator_->OnOutputFormatChanged(format);
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        generator_->OnInputBufferAvailable(index, buffer);
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        generator_->OnOutputBufferAvailable(index, buffer);
    }

private:
    AVThumbnailGenerator *generator_;
};

AVThumbnailGenerator::AVThumbnailGenerator(std::shared_ptr<MediaDemuxer> &mediaDemuxer) : mediaDemuxer_(mediaDemuxer)
{
    MEDIA_LOGI("Constructor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

AVThumbnailGenerator::~AVThumbnailGenerator()
{
    MEDIA_LOGI("Destructor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    Destroy();
}

void AVThumbnailGenerator::OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode)
{
    MEDIA_LOGE("OnError errorType:%{public}d, errorCode:%{public}d", static_cast<int32_t>(errorType), errorCode);
    stopProcessing_ = true;
}

Status AVThumbnailGenerator::InitDecoder()
{
    MEDIA_LOGD("Init decoder start.");
    if (videoDecoder_ != nullptr) {
        MEDIA_LOGD("InitDecoder already.");
        videoDecoder_->Start();
        return Status::OK;
    }
    videoDecoder_ = MediaAVCodec::VideoDecoderFactory::CreateByMime(trackMime_);
    CHECK_AND_RETURN_RET_LOG(videoDecoder_ != nullptr, Status::ERROR_NO_MEMORY, "Create videoDecoder_ is nullptr");
    Format trackFormat{};
    trackFormat.SetMeta(GetVideoTrackInfo());
    int32_t width;
    int32_t height;
    trackFormat.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    trackFormat.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Init decoder trackFormat width:%{public}d, height:%{public}d",
        FAKE_POINTER(this), width, height);
    trackFormat.PutIntValue(
        MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(Plugins::VideoPixelFormat::NV12));
    videoDecoder_->Configure(trackFormat);
    std::shared_ptr<MediaAVCodec::MediaCodecCallback> mediaCodecCallback =
        std::make_shared<ThumnGeneratorCodecCallback>(this);
    videoDecoder_->SetCallback(mediaCodecCallback);
    videoDecoder_->Prepare();
    videoDecoder_->Start();
    MEDIA_LOGD("Init decoder success.");
    return Status::OK;
}

std::shared_ptr<Meta> AVThumbnailGenerator::GetVideoTrackInfo()
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
            CHECK_AND_RETURN_RET_LOG(mediaType == Plugins::MediaType::VIDEO, nullptr,
                "GetTargetTrackInfo mediaType is not video, index:%{public}d, mediaType:%{public}d", index,
                static_cast<int32_t>(mediaType));
            trackIndex_ = index;
            MEDIA_LOGI("0x%{public}06" PRIXPTR " GetTrackInfo success trackIndex_:%{public}d, trackMime_:%{public}s",
                FAKE_POINTER(this), trackIndex_, trackMime_.c_str());
            if (trackInfos[index]->Get<Tag::VIDEO_ROTATION>(rotation_)) {
                MEDIA_LOGD("rotation %{public}d", static_cast<int32_t>(rotation_));
            } else {
                MEDIA_LOGD("no rotation");
            }
            return trackInfos[trackIndex_];
        }
    }
    MEDIA_LOGW("GetTargetTrackInfo FAILED.");
    return nullptr;
}

void AVThumbnailGenerator::OnOutputFormatChanged(const MediaAVCodec::Format &format)
{
    MEDIA_LOGD("OnOutputFormatChanged");
    outputFormat_ = format;
}

void AVThumbnailGenerator::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    MEDIA_LOGD("OnInputBufferAvailable index:%{public}u", index);
    if (stopProcessing_.load()) {
        MEDIA_LOGD("Has stopped processing, will not queue input buffer.");
        return;
    }
    CHECK_AND_RETURN_LOG(mediaDemuxer_ != nullptr, "OnInputBufferAvailable demuxer is nullptr.");
    mediaDemuxer_->ReadSample(trackIndex_, buffer);
    CHECK_AND_RETURN_LOG(videoDecoder_ != nullptr, "OnInputBufferAvailable decoder is nullptr.");
    videoDecoder_->QueueInputBuffer(index);
}

void AVThumbnailGenerator::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    MEDIA_LOGD("OnOutputBufferAvailable index:%{public}u", index);
    if (buffer == nullptr || buffer->memory_ == nullptr) {
        return;
    }
    if (buffer->pts_ >= seekTime_ && !hasFetchedFrame_.load() && !stopProcessing_.load()) {
        bufferIndex_ = index;
        surfaceBuffer_ = buffer->memory_->GetSurfaceBuffer();
        hasFetchedFrame_ = true;
        cond_.notify_all();
    }
    CHECK_AND_RETURN_LOG(videoDecoder_ != nullptr, "OnOutputBufferAvailable videoDecoder_ is nullptr");
    videoDecoder_->ReleaseOutputBuffer(index, false);
}

std::shared_ptr<AVSharedMemory> AVThumbnailGenerator::FetchFrameAtTime(
    int64_t timeUs, int32_t option, const OutputConfiguration &param)
{
    if (hasFetchedFrame_.load() && seekTime_ == timeUs && outputConfig_ == param) {
        return fetchedFrameAtTime_;
    }
    MEDIA_LOGI("Fetch frame 0x%{public}06" PRIXPTR " timeUs:%{public}" PRId64 ", option:%{public}d,"
               "dstWidth:%{public}d, dstHeight:%{public}d, colorFormat:%{public}d",
        FAKE_POINTER(this), timeUs, option, param.dstWidth, param.dstHeight, static_cast<int32_t>(param.colorFormat));
    CHECK_AND_RETURN_RET_LOG(mediaDemuxer_ != nullptr, nullptr, "FetchFrameAtTime demuxer is nullptr");

    hasFetchedFrame_ = false;
    outputConfig_ = param;
    seekTime_ = timeUs;
    if (trackInfo_ == nullptr) {
        trackInfo_ = GetVideoTrackInfo();
    }
    CHECK_AND_RETURN_RET_LOG(trackInfo_ != nullptr, nullptr, "FetchFrameAtTime trackInfo_ is nullptr.");

    mediaDemuxer_->SelectTrack(trackIndex_);
    int64_t realSeekTime = timeUs;
    auto res = SeekToTime(Plugins::Us2Ms(timeUs), static_cast<Plugins::SeekMode>(option), realSeekTime);
    CHECK_AND_RETURN_RET_LOG(res == Status::OK, nullptr, "Seek fail");
    CHECK_AND_RETURN_RET_LOG(InitDecoder() == Status::OK, nullptr, "FetchFrameAtTime InitDecoder failed.");
    {
        std::unique_lock<std::mutex> lock(mutex_);

        // wait up to 3s to fetch frame AVSharedMemory at time.
        if (cond_.wait_for(lock, std::chrono::seconds(3), [this] { return hasFetchedFrame_.load(); })) {
            MEDIA_LOGI("0x%{public}06" PRIXPTR " Fetch frame OK width:%{public}d, height:%{public}d",
                FAKE_POINTER(this), outputConfig_.dstWidth, outputConfig_.dstHeight);
            ConvertToAVSharedMemory(surfaceBuffer_);
            videoDecoder_->ReleaseOutputBuffer(bufferIndex_, false);
            videoDecoder_->Flush();
            mediaDemuxer_->Flush();
        } else {
            hasFetchedFrame_ = true;
            videoDecoder_->Flush();
            mediaDemuxer_->Flush();
        }
    }
    return fetchedFrameAtTime_;
}

Status AVThumbnailGenerator::SeekToTime(int64_t timeMs, Plugins::SeekMode option, int64_t realSeekTime)
{
    auto res = mediaDemuxer_->SeekTo(timeMs, option, realSeekTime);
    if (res != Status::OK && option != Plugins::SeekMode::SEEK_CLOSEST_SYNC) {
        res = mediaDemuxer_->SeekTo(timeMs, Plugins::SeekMode::SEEK_CLOSEST_SYNC, realSeekTime);
    }
    return res;
}

bool AVThumbnailGenerator::ConvertToAVSharedMemory(const sptr<SurfaceBuffer> &surfaceBuffer)
{
    int32_t format = surfaceBuffer->GetFormat();
    int32_t size = surfaceBuffer->GetSize();
    int32_t width = surfaceBuffer->GetWidth();
    int32_t height = surfaceBuffer->GetHeight();
    MEDIA_LOGI("0x%{public}06" PRIXPTR " format:%{public}d, size:%{public}d, "
               "width:%{public}d, height:%{public}d",
        FAKE_POINTER(this), format, size, width, height);

    std::unique_ptr<PixelMap> pixelMap = GetYuvDataAlignStride(surfaceBuffer);
    auto fetchedFrameAtTime =
        std::make_shared<AVSharedMemoryBase>(sizeof(OutputFrame) + pixelMap->GetRowStride() * pixelMap->GetHeight(),
            AVSharedMemory::Flags::FLAGS_READ_WRITE, "FetchedFrameMemory");

    int32_t ret = fetchedFrameAtTime->Init();
    CHECK_AND_RETURN_RET_LOG(
        ret == static_cast<int32_t>(Status::OK), false, "Create AVSharedmemory failed, ret:%{public}d", ret);
    OutputFrame *frame = reinterpret_cast<OutputFrame *>(fetchedFrameAtTime->GetBase());
    frame->width_ = pixelMap->GetWidth();
    frame->height_ = pixelMap->GetHeight();
    frame->stride_ = pixelMap->GetRowStride();
    frame->bytesPerPixel_ = pixelMap->GetPixelBytes();
    frame->size_ = pixelMap->GetRowStride() * pixelMap->GetHeight();
    frame->rotation_ = static_cast<int32_t>(rotation_);
    fetchedFrameAtTime->Write(pixelMap->GetPixels(), frame->size_, sizeof(OutputFrame));
    fetchedFrameAtTime_ = fetchedFrameAtTime;
    return true;
}

void AVThumbnailGenerator::ConvertP010ToNV12(
    const sptr<SurfaceBuffer> &surfaceBuffer, uint8_t *dstNV12, int32_t strideWidth, int32_t strideHeight)
{
    int32_t width = surfaceBuffer->GetWidth();
    int32_t height = surfaceBuffer->GetHeight();
    uint8_t *srcP010 = static_cast<uint8_t *>(surfaceBuffer->GetVirAddr());

    // copy src Y component to dst
    for (int32_t i = 0; i < height; i++) {
        uint16_t *srcY = reinterpret_cast<uint16_t *>(srcP010 + strideWidth * i);
        uint8_t *dstY = dstNV12 + width * i;
        for (int32_t j = 0; j < width; j++) {
            *dstY = static_cast<uint8_t>(*srcY >> SHIFT_BITS_P010_2_NV12);
            srcY++;
            dstY++;
        }
    }

    uint8_t *maxDstAddr = dstNV12 + width * height + width * height / RATE_UV;
    uint8_t *originSrcAddr = srcP010;
    uint16_t *maxSrcAddr = reinterpret_cast<uint16_t *>(originSrcAddr) + surfaceBuffer->GetSize();

    srcP010 = static_cast<uint8_t *>(surfaceBuffer->GetVirAddr()) + strideWidth * strideHeight;
    dstNV12 = dstNV12 + width * height;

    // copy src UV component to dst, height(UV) = height(Y) / 2;
    for (int32_t i = 0; i < height / 2; i++) {
        uint16_t *srcUV = reinterpret_cast<uint16_t *>(srcP010 + strideWidth * i);
        uint8_t *dstUV = dstNV12 + width * i;
        for (int32_t j = 0; j < width && srcUV < maxSrcAddr && dstUV < maxDstAddr; j++) {
            *dstUV = static_cast<uint8_t>(*srcUV >> SHIFT_BITS_P010_2_NV12);
            *(dstUV + 1) = static_cast<uint8_t>(*(srcUV + 1) >> SHIFT_BITS_P010_2_NV12);
            srcUV += 2;  // srcUV move by 2 to process U and V component
            dstUV += 2;  // dstUV move by 2 to process U and V component
        }
    }
}

std::unique_ptr<PixelMap> AVThumbnailGenerator::GetYuvDataAlignStride(const sptr<SurfaceBuffer> &surfaceBuffer)
{
    int32_t width = surfaceBuffer->GetWidth();
    int32_t height = surfaceBuffer->GetHeight();
    int32_t stride = surfaceBuffer->GetStride();
    int32_t outputHeight;
    auto isOutputFormatValid = outputFormat_.GetIntValue(Tag::VIDEO_SLICE_HEIGHT, outputHeight);
    if (!isOutputFormatValid && outputHeight == 0) {
        outputHeight = height;
    }
    MEDIA_LOGD("GetYuvDataAlignStride stride:%{public}d, strideWidth:%{public}d, outputHeight:%{public}d", stride,
        stride, outputHeight);

    InitializationOptions initOpts;
    initOpts.size = { width, height };
    initOpts.srcPixelFormat = PixelFormat::NV12;
    std::unique_ptr<PixelMap> yuvPixelMap = PixelMap::Create(initOpts);
    uint8_t *dstData = static_cast<uint8_t *>(yuvPixelMap->GetWritablePixels());
    uint8_t *srcPtr = static_cast<uint8_t *>(surfaceBuffer->GetVirAddr());
    uint8_t *dstPtr = dstData;
    int32_t format = surfaceBuffer->GetFormat();
    if (format == static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_P010)) {
        ConvertP010ToNV12(surfaceBuffer, dstPtr, stride, outputHeight);
        return yuvPixelMap;
    }

    // copy src Y component to dst
    for (int32_t y = 0; y < height; y++) {
        auto ret = memcpy_s(dstPtr, width, srcPtr, width);
        if (ret != EOK) {
            MEDIA_LOGW("Memcpy Y component failed.");
        }
        srcPtr += stride;
        dstPtr += width;
    }

    srcPtr = static_cast<uint8_t *>(surfaceBuffer->GetVirAddr()) + stride * outputHeight;

    // copy src UV component to dst, height(UV) = height(Y) / 2
    for (int32_t uv = 0; uv < height / 2; uv++) {
        auto ret = memcpy_s(dstPtr, width, srcPtr, width);
        if (ret != EOK) {
            MEDIA_LOGW("Memcpy UV component failed.");
        }
        srcPtr += stride;
        dstPtr += width;
    }

    return yuvPixelMap;
}

void AVThumbnailGenerator::Reset()
{
    if (mediaDemuxer_ != nullptr) {
        mediaDemuxer_->Reset();
    }

    if (videoDecoder_ != nullptr) {
        videoDecoder_->Reset();
    }

    hasFetchedFrame_ = false;
    trackInfo_ = nullptr;
}

void AVThumbnailGenerator::Destroy()
{
    stopProcessing_ = true;
    if (videoDecoder_ != nullptr) {
        videoDecoder_->Stop();
        videoDecoder_->Release();
    }
    mediaDemuxer_ = nullptr;
    videoDecoder_ = nullptr;
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Finish Destroy.", FAKE_POINTER(this));
}
}  // namespace Media
}  // namespace OHOS