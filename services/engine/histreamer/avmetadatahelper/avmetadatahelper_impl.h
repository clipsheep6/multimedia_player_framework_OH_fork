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

#ifndef AVMETADATAHELPER_HST_IMPL_H
#define AVMETADATAHELPER_HST_IMPL_H

#include <condition_variable>
#include <mutex>

#include "avmetadata_collector.h"
#include "buffer/avsharedmemorybase.h"
#include "common/status.h"
#include "demuxer_filter.h"
#include "filter/filter_factory.h"
#include "i_avmetadatahelper_engine.h"
#include "i_avmetadatahelper_service.h"
#include "media_demuxer.h"
#include "nocopyable.h"
#include "pipeline/pipeline.h"
#include "video_decoder_adapter.h"

namespace OHOS {
namespace Media {
class AVMetadataHelperImpl : public IAVMetadataHelperEngine, public std::enable_shared_from_this<AVMetadataHelperImpl>,
    public NoCopyable {
public:
    AVMetadataHelperImpl();
    ~AVMetadataHelperImpl();

    int32_t SetSource(const std::string &uri, int32_t usage) override;
    int32_t SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc) override;
    std::string ResolveMetadata(int32_t key) override;
    std::unordered_map<int32_t, std::string> ResolveMetadata() override;
    std::shared_ptr<AVSharedMemory> FetchFrameAtTime(
        int64_t timeUs, int32_t option, const OutputConfiguration &param) override;
    std::shared_ptr<AVSharedMemory> FetchArtPicture() override;

    void OnEvent(const Event &event);
    void OnCallback(std::shared_ptr<OHOS::Media::Pipeline::Filter> filter,
        const OHOS::Media::Pipeline::FilterCallBackCommand cmd, OHOS::Media::Pipeline::StreamType outType);
    void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode);
    void OnOutputFormatChanged(const MediaAVCodec::Format &format);
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);
    void OnFetchedFrameBufferAvailable();

private:
    Status SetSourceInternel(const std::string &uri, int32_t usage);
    Status SetSourceInternel(const std::shared_ptr<IMediaDataSource> &dataSrc);
    int32_t ExtractMetadata();
    void Reset();
    void Destroy();
    Status InitDecoder();

    std::shared_ptr<Meta> GetTargetTrackInfo();
    std::unique_ptr<PixelMap> GetYuvDataAlignStride(const sptr<SurfaceBuffer> &surfaceBuffer);
    bool ConvertToAVSharedMemory(const sptr<SurfaceBuffer> &surfaceBuffer);
    std::unique_ptr<PixelMap> ConvertPixelMap(PixelFormat format, std::unique_ptr<PixelMap> pixelMap);

    std::unordered_map<int32_t, std::string> collectedMeta_;
    bool hasCollectMeta_{false};
    int32_t usage_{AVMetadataUsage::AV_META_USAGE_PIXEL_MAP};

    std::string srcUri_;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool errHappened_{false};
    bool firstFetch_{true};

    std::shared_ptr<AVSharedMemory> collectedArtPicture_;
    std::shared_ptr<AVSharedMemoryBase> fetchedFrameAtTime_;

    std::shared_ptr<AVMetaDataCollector> metaCollector_;
    std::shared_ptr<OHOS::Media::MediaDemuxer> mediaDemuxer_;
    std::shared_ptr<MediaAVCodec::AVCodecVideoDecoder> videoDecoder_;

    BufferRequestConfig requestCfg_;
    OutputConfiguration outputConfig_;
    Format outputFormat_;

    int64_t seekTime_{0};
    std::atomic_bool hasFetchedFrame_{false};
    std::atomic_bool stopProcessing_{false};
    std::string trackMime_;
    std::shared_ptr<Meta> trackInfo_;
    size_t trackIndex_{0};

    sptr<Surface> producerSurface_{nullptr};
    sptr<Surface> consumerSurface_{nullptr};
};
} // namespace Media
} // namespace OHOS
#endif // AVMETADATAHELPER_HST_IMPL_H