/*
 * Copyright (C) 2021-2024 Huawei Device Co., Ltd.
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

#include <sys/stat.h>
#include <unistd.h>

#include "v1_0/cm_color_space.h"
#include "v1_0/hdr_static_metadata.h"
#include "v1_0/buffer_handle_meta_key_type.h"

#include "securec.h"
#include "image_source.h"
#include "i_media_service.h"
#include "media_log.h"
#include "media_errors.h"
#include "scope_guard.h"
#include "hisysevent.h"
#include "image_format_convert.h"
#include "color_space.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_METADATA, "AVMetadatahelperImpl" };
constexpr int32_t SCENE_CODE_EFFECTIVE_DURATION_MS = 20000;
static constexpr char PERFORMANCE_STATS[] = "PERFORMANCE";
static std::atomic<uint32_t> concurrentWorkCount_ = 0;
static constexpr uint8_t HIGH_CONCRENT_WORK_NUM = 4;
}

namespace OHOS {
namespace Media {
using namespace OHOS::HDI::Display::Graphic::Common::V1_0;
static constexpr uint8_t PIXEL_SIZE_HDR_YUV = 3;
static constexpr float PIXEL_SIZE_SDR_YUV = 1.5;
static std::map<Scene, long> SCENE_CODE_MAP = { { Scene::AV_META_SCENE_CLONE, 1 },
                                                { Scene::AV_META_SCENE_BATCH_HANDLE, 2 } };
static std::map<Scene, int64_t> SCENE_TIMESTAMP_MAP = { { Scene::AV_META_SCENE_CLONE, 0 },
                                                        { Scene::AV_META_SCENE_BATCH_HANDLE, 0 } };

struct PixelMapMemHolder {
    bool isShmem;
    std::shared_ptr<AVSharedMemory> shmem;
    uint8_t *heap;
};

struct AVBufferHolder {
    std::shared_ptr<AVBuffer> buffer;
};

static void FreePixelMapData(void *addr, void *context, uint32_t size)
{
    (void)size;

    MEDIA_LOGD("free pixel map data");

    CHECK_AND_RETURN_LOG(context != nullptr, "context is nullptr");
    PixelMapMemHolder *holder = reinterpret_cast<PixelMapMemHolder *>(context);
    if (holder->isShmem) {
        if (holder->shmem == nullptr) {
            MEDIA_LOGE("shmem is nullptr");
        }
        holder->shmem = nullptr;
        holder->heap = nullptr;
    } else {
        if (holder->heap == nullptr || holder->heap != addr) {
            MEDIA_LOGE("heap is invalid");
        } else {
            delete [] holder->heap;
            holder->heap = nullptr;
        }
    }
    delete holder;
}

static void FreeAvBufferData(void *addr, void *context, uint32_t size)
{
    (void)addr;
    (void)size;
    CHECK_AND_RETURN_LOG(context != nullptr, "context is nullptr");
    AVBufferHolder *holder = reinterpret_cast<AVBufferHolder *>(context);
    delete holder;
}

static void FreeSurfaceBuffer(void *addr, void *context, uint32_t size)
{
    (void)addr;
    (void)size;
    CHECK_AND_RETURN_LOG(context != nullptr, "context is nullptr");
    void* nativeBuffer = context;
    RefBase *ref = reinterpret_cast<RefBase *>(nativeBuffer);
    ref->DecStrongRef(ref);
}

static PixelMapMemHolder *CreatePixelMapData(const std::shared_ptr<AVSharedMemory> &mem, const OutputFrame &frame)
{
    PixelMapMemHolder *holder = new (std::nothrow) PixelMapMemHolder;
    CHECK_AND_RETURN_RET_LOG(holder != nullptr, nullptr, "alloc pixelmap mem holder failed");

    ON_SCOPE_EXIT(0) {
        delete holder;
    };

    int32_t minStride = frame.width_ * frame.bytesPerPixel_;
    CHECK_AND_RETURN_RET_LOG(minStride <= frame.stride_, nullptr, "stride info wrong");

    if (frame.stride_ == minStride) {
        CANCEL_SCOPE_EXIT_GUARD(0);
        holder->isShmem = true;
        holder->shmem = mem;
        holder->heap = frame.GetFlattenedData();
        return holder;
    }

    static constexpr int64_t maxAllowedSize = 100 * 1024 * 1024;
    int64_t memSize = static_cast<int64_t>(minStride) * frame.height_;
    CHECK_AND_RETURN_RET_LOG(memSize <= maxAllowedSize, nullptr, "alloc heap size too large");

    uint8_t *heap = new (std::nothrow) uint8_t[memSize];
    CHECK_AND_RETURN_RET_LOG(heap != nullptr, nullptr, "alloc heap failed");

    ON_SCOPE_EXIT(1) {
        delete [] heap;
    };

    uint8_t *currDstPos = heap;
    uint8_t *currSrcPos = frame.GetFlattenedData();
    for (int32_t row = 0; row < frame.height_; ++row) {
        errno_t rc = memcpy_s(currDstPos, static_cast<size_t>(memSize), currSrcPos, static_cast<size_t>(minStride));
        CHECK_AND_RETURN_RET_LOG(rc == EOK, nullptr, "memcpy_s failed");

        currDstPos += minStride;
        currSrcPos += frame.stride_;
        memSize -= minStride;
    }

    holder->isShmem = false;
    holder->heap = heap;

    CANCEL_SCOPE_EXIT_GUARD(0);
    CANCEL_SCOPE_EXIT_GUARD(1);
    return holder;
}

static AVBufferHolder *CreateAVBufferHolder(const std::shared_ptr<AVBuffer> &avBuffer)
{
    CHECK_AND_RETURN_RET(avBuffer != nullptr && avBuffer->memory_ != nullptr, nullptr);
    AVBufferHolder *holder = new (std::nothrow) AVBufferHolder;
    CHECK_AND_RETURN_RET_LOG(holder != nullptr, nullptr, "alloc avBuffer holder failed");
    holder->buffer = avBuffer;
    return holder;
}

static std::shared_ptr<PixelMap> CreatePixelMap(const std::shared_ptr<AVSharedMemory> &mem, PixelFormat color,
    int32_t &rotation)
{
    CHECK_AND_RETURN_RET_LOG(mem != nullptr, nullptr, "Fetch frame failed");
    CHECK_AND_RETURN_RET_LOG(mem->GetBase() != nullptr, nullptr, "Addr is nullptr");
    CHECK_AND_RETURN_RET_LOG(mem->GetSize() > 0, nullptr, "size is incorrect");
    CHECK_AND_RETURN_RET_LOG(static_cast<uint32_t>(mem->GetSize()) >= sizeof(OutputFrame),
                             nullptr, "size is incorrect");

    OutputFrame *frame = reinterpret_cast<OutputFrame *>(mem->GetBase());
    MEDIA_LOGD("width: %{public}d, stride : %{public}d, height: %{public}d, size: %{public}d, format: %{public}d",
        frame->width_, frame->stride_, frame->height_, frame->size_, color);

    rotation = frame->rotation_;
    InitializationOptions opts;
    opts.size.width = frame->width_;
    opts.size.height = frame->height_;
    opts.pixelFormat = color;
    opts.editable = true;
    std::shared_ptr<PixelMap> pixelMap = PixelMap::Create(opts);

    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, nullptr, "pixelMap create failed");
    CHECK_AND_RETURN_RET_LOG(pixelMap->GetByteCount() <= frame->size_, nullptr, "Size inconsistent !");

    PixelMapMemHolder *holder = CreatePixelMapData(mem, *frame);
    CHECK_AND_RETURN_RET_LOG(holder != nullptr, nullptr, "create pixel map data failed");

    pixelMap->SetPixelsAddr(holder->heap, holder, static_cast<uint32_t>(pixelMap->GetByteCount()),
        AllocatorType::CUSTOM_ALLOC, FreePixelMapData);
    return pixelMap;
}

std::shared_ptr<PixelMap> AVMetadataHelperImpl::CreatePixelMapYuv(const std::shared_ptr<AVBuffer> &frameBuffer,
                                                                  PixelMapInfo &pixelMapInfo)
{
    bool isValid = frameBuffer != nullptr && frameBuffer->meta_ != nullptr && frameBuffer->memory_ != nullptr;
    CHECK_AND_RETURN_RET_LOG(isValid, nullptr, "invalid frame buffer");
    auto bufferMeta = frameBuffer->meta_;

    int32_t width = 0;
    auto hasProperty = bufferMeta->Get<Tag::VIDEO_WIDTH>(width);
    CHECK_AND_RETURN_RET_LOG(hasProperty, nullptr, "invalid width");
    int32_t height = 0;
    hasProperty = bufferMeta->Get<Tag::VIDEO_HEIGHT>(height);
    CHECK_AND_RETURN_RET_LOG(hasProperty, nullptr, "invalid height");

    Plugins::VideoRotation rotation = Plugins::VideoRotation::VIDEO_ROTATION_0;
    bufferMeta->Get<Tag::VIDEO_ROTATION>(rotation);
    pixelMapInfo.rotation = static_cast<int32_t>(rotation);

    bufferMeta->Get<Tag::VIDEO_IS_HDR_VIVID>(pixelMapInfo.isHdr);
    pixelMapInfo.isHdr &= frameBuffer->memory_->GetSurfaceBuffer() != nullptr;

    if (pixelMapInfo.isHdr) {
        auto surfaceBuffer = frameBuffer->memory_->GetSurfaceBuffer();
        sptr<SurfaceBuffer> mySurfaceBuffer = CopySurfaceBuffer(surfaceBuffer);
        CHECK_AND_RETURN_RET_LOG(mySurfaceBuffer != nullptr, nullptr, "Create SurfaceBuffer failed");
        InitializationOptions options = { .size = { .width = mySurfaceBuffer->GetWidth(),
                                                    .height = mySurfaceBuffer->GetHeight() },
                                          .srcPixelFormat = PixelFormat::YCBCR_P010,
                                          .pixelFormat = PixelFormat::YCBCR_P010 };
        uint32_t colorLength = mySurfaceBuffer->GetWidth() * mySurfaceBuffer->GetHeight() * PIXEL_SIZE_HDR_YUV;
        auto pixelMap =
            PixelMap::Create(reinterpret_cast<const uint32_t *>(mySurfaceBuffer->GetVirAddr()), colorLength, options);
        pixelMap->InnerSetColorSpace(OHOS::ColorManager::ColorSpace(ColorManager::ColorSpaceName::BT2020_HLG));
        pixelMap->SetPixelsAddr(mySurfaceBuffer->GetVirAddr(), mySurfaceBuffer.GetRefPtr(), mySurfaceBuffer->GetSize(),
                                AllocatorType::DMA_ALLOC, FreeSurfaceBuffer);
        void* nativeBuffer = mySurfaceBuffer.GetRefPtr();
        RefBase *ref = reinterpret_cast<RefBase *>(nativeBuffer);
        ref->IncStrongRef(ref);
        return pixelMap;
    }

    AVBufferHolder *holder = CreateAVBufferHolder(frameBuffer);
    InitializationOptions options = { .size = { .width = width, .height = height }, .srcPixelFormat = PixelFormat::NV12,
                                      .pixelFormat = PixelFormat::NV12 };
    uint32_t colorLength = width * height * PIXEL_SIZE_SDR_YUV;
    auto pixelMap =
        PixelMap::Create(reinterpret_cast<const uint32_t *>(frameBuffer->memory_->GetAddr()), colorLength, options);
    CHECK_AND_RETURN_RET_LOG(holder != nullptr, nullptr, "Create buffer holder failed");
    uint8_t *pixelAddr = frameBuffer->memory_->GetAddr();
    pixelMap->SetPixelsAddr(pixelAddr, holder, pixelMap->GetByteCount(), AllocatorType::CUSTOM_ALLOC, FreeAvBufferData);
    return pixelMap;
}

sptr<SurfaceBuffer> AVMetadataHelperImpl::CopySurfaceBuffer(sptr<SurfaceBuffer> &srcSurfaceBuffer)
{
    sptr<SurfaceBuffer> dstSurfaceBuffer = SurfaceBuffer::Create();
    BufferRequestConfig requestConfig = {
        .width = srcSurfaceBuffer->GetWidth(),
        .height = srcSurfaceBuffer->GetHeight(),
        .strideAlignment = 0x2,
        .format = srcSurfaceBuffer->GetFormat(),  // always yuv
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_MEM_MMZ_CACHE,
        .timeout = 0,
    };
    GSError allocRes = dstSurfaceBuffer->Alloc(requestConfig);
    CHECK_AND_RETURN_RET_LOG(allocRes == 0, nullptr, "Alloc surfaceBuffer failed, ecode %{public}d", allocRes);

    MEDIA_LOGI("winddraw %{public}d %{public}d", dstSurfaceBuffer->GetSize(), srcSurfaceBuffer->GetSize());

    CopySurfaceBufferInfo(srcSurfaceBuffer, dstSurfaceBuffer);
    CopySurfaceBufferPixels(srcSurfaceBuffer, dstSurfaceBuffer);
    return dstSurfaceBuffer;
}

void AVMetadataHelperImpl::CopySurfaceBufferInfo(sptr<SurfaceBuffer> &source, sptr<SurfaceBuffer> &dst)
{
    if (source == nullptr || dst == nullptr) {
        MEDIA_LOGI("CopySurfaceBufferInfo failed, source or dst is nullptr");
        return;
    }
    std::vector<uint8_t> hdrMetadataTypeVec;
    std::vector<uint8_t> colorSpaceInfoVec;
    std::vector<uint8_t> staticData;
    std::vector<uint8_t> dynamicData;

    if (source->GetMetadata(ATTRKEY_HDR_METADATA_TYPE, hdrMetadataTypeVec) == GSERROR_OK) {
        dst->SetMetadata(ATTRKEY_HDR_METADATA_TYPE, hdrMetadataTypeVec);
    }
    if (source->GetMetadata(ATTRKEY_COLORSPACE_INFO, colorSpaceInfoVec) == GSERROR_OK) {
        dst->SetMetadata(ATTRKEY_COLORSPACE_INFO, colorSpaceInfoVec);
    }
    if (GetSbStaticMetadata(source, staticData) && (staticData.size() > 0)) {
        SetSbStaticMetadata(dst, staticData);
    }
    if (GetSbDynamicMetadata(source, dynamicData) && (dynamicData.size()) > 0) {
        SetSbDynamicMetadata(dst, dynamicData);
    }
}

bool AVMetadataHelperImpl::GetSbStaticMetadata(sptr<SurfaceBuffer> &buffer, std::vector<uint8_t> &staticMetadata)
{
    return buffer->GetMetadata(ATTRKEY_HDR_STATIC_METADATA, staticMetadata) == GSERROR_OK;
}

bool AVMetadataHelperImpl::GetSbDynamicMetadata(sptr<SurfaceBuffer> &buffer, std::vector<uint8_t> &dynamicMetadata)
{
    return buffer->GetMetadata(ATTRKEY_HDR_DYNAMIC_METADATA, dynamicMetadata) == GSERROR_OK;
}

bool AVMetadataHelperImpl::SetSbStaticMetadata(sptr<SurfaceBuffer> &buffer, const std::vector<uint8_t> &staticMetadata)
{
    return buffer->SetMetadata(ATTRKEY_HDR_STATIC_METADATA, staticMetadata) == GSERROR_OK;
}

bool AVMetadataHelperImpl::SetSbDynamicMetadata(sptr<SurfaceBuffer> &buffer,
                                                const std::vector<uint8_t> &dynamicMetadata)
{
    return buffer->SetMetadata(ATTRKEY_HDR_DYNAMIC_METADATA, dynamicMetadata) == GSERROR_OK;
}

int32_t AVMetadataHelperImpl::CopySurfaceBufferPixels(sptr<SurfaceBuffer> &srcSurfaceBuffer,
                                                      sptr<SurfaceBuffer> &dstSurfaceBuffer)
{
    auto res = memcpy_s(dstSurfaceBuffer->GetVirAddr(), dstSurfaceBuffer->GetSize(), srcSurfaceBuffer->GetVirAddr(),
        srcSurfaceBuffer->GetSize());
    return res == EOK ? MSERR_OK : MSERR_NO_MEMORY;
}

std::shared_ptr<AVMetadataHelper> AVMetadataHelperFactory::CreateAVMetadataHelper()
{
    std::shared_ptr<AVMetadataHelperImpl> impl = std::make_shared<AVMetadataHelperImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "failed to new AVMetadataHelperImpl");

    int32_t ret = impl->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to init AVMetadataHelperImpl");

    return impl;
}

int32_t AVMetadataHelperImpl::Init()
{
    avMetadataHelperService_ = MediaServiceFactory::GetInstance().CreateAVMetadataHelperService();
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperService_ != nullptr, MSERR_NO_MEMORY,
        "failed to create avmetadatahelper service");
    return MSERR_OK;
}

AVMetadataHelperImpl::AVMetadataHelperImpl()
{
    MEDIA_LOGD("AVMetadataHelperImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVMetadataHelperImpl::~AVMetadataHelperImpl()
{
    if (avMetadataHelperService_ != nullptr) {
        (void)MediaServiceFactory::GetInstance().DestroyAVMetadataHelperService(avMetadataHelperService_);
        avMetadataHelperService_ = nullptr;
    }
    MEDIA_LOGD("AVMetadataHelperImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVMetadataHelperImpl::SetHelperCallback(const std::shared_ptr<HelperCallback> &callback)
{
    MEDIA_LOGD("AVMetadataHelperImpl:0x%{public}06" PRIXPTR " SetHelperCallback in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperService_ != nullptr, MSERR_SERVICE_DIED,
        "metadata helper service does not exist..");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_INVALID_VAL, "callback is nullptr");
    return avMetadataHelperService_->SetHelperCallback(callback);
}

void AVMetadataHelperImpl::SetScene(Scene scene)
{
    ReportSceneCode(scene);
}

void AVMetadataHelperImpl::ReportSceneCode(Scene scene)
{
    if (scene != Scene::AV_META_SCENE_CLONE && scene != Scene::AV_META_SCENE_BATCH_HANDLE) {
        return;
    }
    if (scene == Scene::AV_META_SCENE_BATCH_HANDLE && concurrentWorkCount_ < HIGH_CONCRENT_WORK_NUM) {
        return;
    }
    auto sceneCode = SCENE_CODE_MAP[scene];
    auto lastTsp = SCENE_TIMESTAMP_MAP[scene];
    auto now =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    auto duration = now - std::chrono::milliseconds(lastTsp);
    if (duration < std::chrono::milliseconds(SCENE_CODE_EFFECTIVE_DURATION_MS)) {
        return;
    }
    SCENE_TIMESTAMP_MAP[scene] = now.count();
    MEDIA_LOGI("Report scene code %{public}ld", sceneCode);
    int32_t ret = HiSysEventWrite(
        PERFORMANCE_STATS, "CPU_SCENE_ENTRY", OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR, "PACKAGE_NAME",
        "media_service", "SCENE_ID", std::to_string(sceneCode).c_str(), "HAPPEN_TIME", now.count());
    if (ret != MSERR_OK) {
        MEDIA_LOGW("report error");
    }
}

int32_t AVMetadataHelperImpl::SetSource(const std::string &uri, int32_t usage)
{
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperService_ != nullptr, MSERR_NO_MEMORY,
        "avmetadatahelper service does not exist..");
    CHECK_AND_RETURN_RET_LOG(!uri.empty(), MSERR_INVALID_VAL, "uri is empty.");

    concurrentWorkCount_++;
    ReportSceneCode(AV_META_SCENE_BATCH_HANDLE);
    auto res = avMetadataHelperService_->SetSource(uri, usage);
    concurrentWorkCount_--;
    return res;
}

int32_t AVMetadataHelperImpl::SetSource(int32_t fd, int64_t offset, int64_t size, int32_t usage)
{
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperService_ != nullptr, MSERR_NO_MEMORY,
        "avmetadatahelper service does not exist..");
    MEDIA_LOGI("Set file source fd: %{public}d, offset: %{public}" PRIu64 ", size: %{public}" PRIu64,
        fd, offset, size);

    concurrentWorkCount_++;
    ReportSceneCode(AV_META_SCENE_BATCH_HANDLE);
    auto res = avMetadataHelperService_->SetSource(fd, offset, size, usage);
    concurrentWorkCount_--;
    return res;
}

int32_t AVMetadataHelperImpl::SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    MEDIA_LOGD("AVMetadataHelperImpl:0x%{public}06" PRIXPTR " SetSource in(dataSrc)", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(dataSrc != nullptr, MSERR_INVALID_VAL, "failed to create data source");

    concurrentWorkCount_++;
    ReportSceneCode(AV_META_SCENE_BATCH_HANDLE);
    auto res = avMetadataHelperService_->SetSource(dataSrc);
    concurrentWorkCount_--;
    return res;
}

std::string AVMetadataHelperImpl::ResolveMetadata(int32_t key)
{
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperService_ != nullptr, "",
        "avmetadatahelper service does not exist.");

    return avMetadataHelperService_->ResolveMetadata(key);
}

std::unordered_map<int32_t, std::string> AVMetadataHelperImpl::ResolveMetadata()
{
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperService_ != nullptr, {},
        "avmetadatahelper service does not exist.");

    return avMetadataHelperService_->ResolveMetadata();
}

std::shared_ptr<Meta> AVMetadataHelperImpl::GetAVMetadata()
{
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperService_ != nullptr, nullptr,
        "avmetadatahelper service does not exist.");

    return avMetadataHelperService_->GetAVMetadata();
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperImpl::FetchArtPicture()
{
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperService_ != nullptr, nullptr,
        "avmetadatahelper service does not exist.");

    return avMetadataHelperService_->FetchArtPicture();
}

std::shared_ptr<PixelMap> AVMetadataHelperImpl::FetchFrameAtTime(
    int64_t timeUs, int32_t option, const PixelMapParams &param)
{
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperService_ != nullptr, nullptr,
        "avmetadatahelper service does not exist.");

    concurrentWorkCount_++;
    ReportSceneCode(AV_META_SCENE_BATCH_HANDLE);

    OutputConfiguration config;
    config.colorFormat = param.colorFormat;
    config.dstHeight = param.dstHeight;
    config.dstWidth = param.dstWidth;

    auto mem = avMetadataHelperService_->FetchFrameAtTime(timeUs, option, config);
    auto pixelMap = CreatePixelMap(mem, PixelFormat::NV12, rotation_);

    concurrentWorkCount_--;

    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, nullptr, "pixelMap does not exist.");
 
    const InitializationOptions opts = { .size = { .width = pixelMap->GetWidth(), .height = pixelMap->GetHeight() },
                                         .srcPixelFormat = PixelFormat::NV12 };
    pixelMap =
        PixelMap::Create(reinterpret_cast<const uint32_t *>(pixelMap->GetPixels()), pixelMap->GetByteCount(), opts);
    if (pixelMap == nullptr) {
        return nullptr;
    }
    if (rotation_ > 0) {
        pixelMap->rotate(rotation_);
    }
    int32_t srcWidth = pixelMap->GetWidth();
    int32_t srcHeight = pixelMap->GetHeight();
    bool needScale = (param.dstWidth > 0 && param.dstHeight > 0) &&
                     (param.dstWidth <= srcWidth && param.dstHeight <= srcHeight) &&
                     (param.dstWidth < srcWidth || param.dstHeight < srcHeight) && srcWidth > 0 && srcHeight > 0;
    if (needScale) {
        pixelMap->scale((1.0f * param.dstWidth) / srcWidth, (1.0f * param.dstHeight) / srcHeight);
    }
    return pixelMap;
}


std::shared_ptr<PixelMap> AVMetadataHelperImpl::FetchFrameYuv(int64_t timeUs, int32_t option,
                                                              const PixelMapParams &param)
{
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperService_ != nullptr, nullptr, "avmetadatahelper service does not exist.");

    concurrentWorkCount_++;
    ReportSceneCode(AV_META_SCENE_BATCH_HANDLE);
    OutputConfiguration config = { .colorFormat = param.colorFormat,
                                   .dstHeight = param.dstHeight,
                                   .dstWidth = param.dstWidth };
    auto frameBuffer = avMetadataHelperService_->FetchFrameYuv(timeUs, option, config);
    CHECK_AND_RETURN_RET(frameBuffer != nullptr, nullptr);
    concurrentWorkCount_--;

    PixelMapInfo pixelMapInfo;
    auto pixelMap = CreatePixelMapYuv(frameBuffer, pixelMapInfo);
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, nullptr, "convert to pixelMap failed");

    int32_t srcWidth = pixelMap->GetWidth();
    int32_t srcHeight = pixelMap->GetHeight();
    YUVDataInfo yuvDataInfo = { .yWidth = srcWidth,
                                .yHeight = srcHeight,
                                .uvWidth = srcWidth / 2,
                                .uvHeight = srcHeight / 2,
                                .yStride = srcWidth,
                                .uvStride = srcWidth,
                                .uvOffset = srcWidth * srcHeight };
    pixelMap->SetImageYUVInfo(yuvDataInfo);
    bool needScale = (param.dstWidth > 0 && param.dstHeight > 0) &&
                     (param.dstWidth <= srcWidth && param.dstHeight <= srcHeight) &&
                     (param.dstWidth < srcWidth || param.dstHeight < srcHeight) && srcWidth > 0 && srcHeight > 0;
    if (needScale) {
        if (!pixelMapInfo.isHdr) {
            pixelMap->SetAllocatorType(AllocatorType::SHARE_MEM_ALLOC);
        }
        pixelMap->scale((1.0f * param.dstWidth) / srcWidth, (1.0f * param.dstHeight) / srcHeight);
    }
    if (pixelMapInfo.rotation > 0) {
        pixelMap->rotate(pixelMapInfo.rotation);
    }
    if ((needScale || pixelMapInfo.rotation > 0) && pixelMapInfo.isHdr) {
        pixelMap->SetAllocatorType(AllocatorType::CUSTOM_ALLOC);
    }
    return pixelMap;
}

int32_t AVMetadataHelperImpl::GetTimeByFrameIndex(uint32_t index, int64_t &time)
{
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperService_ != nullptr, 0, "avmetadatahelper service does not exist.");
    return avMetadataHelperService_->GetTimeByFrameIndex(index, time);
}

int32_t AVMetadataHelperImpl::GetFrameIndexByTime(int64_t time, uint32_t &index)
{
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperService_ != nullptr, 0, "avmetadatahelper service does not exist.");
    return avMetadataHelperService_->GetFrameIndexByTime(time, index);
}

void AVMetadataHelperImpl::Release()
{
    CHECK_AND_RETURN_LOG(avMetadataHelperService_ != nullptr, "avmetadatahelper service does not exist.");
    avMetadataHelperService_->Release();
    (void)MediaServiceFactory::GetInstance().DestroyAVMetadataHelperService(avMetadataHelperService_);
    avMetadataHelperService_ = nullptr;
}
} // namespace Media
} // namespace OHOS
