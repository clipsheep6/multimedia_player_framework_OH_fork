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

#ifndef AV_THUMBNAIL_GENERATOR
#define AV_THUMBNAIL_GENERATOR

#include <unordered_map>
#include <set>
#include <condition_variable>
#include <mutex>
#include <nocopyable.h>

#include "buffer/avsharedmemorybase.h"
#include "common/status.h"
#include "i_avmetadatahelper_service.h"
#include "media_demuxer.h"
#include "pipeline/pipeline.h"
#include "video_decoder_adapter.h"

namespace OHOS {
namespace Media {
class AVThumbnailGenerator : public NoCopyable {
public:
    explicit AVThumbnailGenerator(std::shared_ptr<MediaDemuxer> &mediaDemuxer);
    ~AVThumbnailGenerator();
    std::shared_ptr<AVSharedMemory> FetchFrameAtTime(int64_t timeUs, int32_t option, const OutputConfiguration &param);
    std::shared_ptr<AVSharedMemory> FetchArtPicture();

    void Reset();
    void Destroy();
    void OnEvent(const Event &event);
    void OnCallback(std::shared_ptr<OHOS::Media::Pipeline::Filter> filter,
        const OHOS::Media::Pipeline::FilterCallBackCommand cmd, OHOS::Media::Pipeline::StreamType outType);
    void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode);
    void OnOutputFormatChanged(const MediaAVCodec::Format &format);
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);
    void OnFetchedFrameBufferAvailable();

private:
    OutputConfiguration outputConfig_;
    Format outputFormat_;
    int64_t seekTime_{0};
    std::atomic_bool hasFetchedFrame_{false};
    std::atomic_bool stopProcessing_{false};
    std::string trackMime_;
    Plugins::VideoRotation rotation_ = Plugins::VideoRotation::VIDEO_ROTATION_0;
    size_t trackIndex_{0};
    std::shared_ptr<Meta> trackInfo_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<uint32_t> bufferIndex_;
    sptr<SurfaceBuffer> surfaceBuffer_;
    std::shared_ptr<AVSharedMemoryBase> fetchedFrameAtTime_;
    std::shared_ptr<OHOS::Media::MediaDemuxer> mediaDemuxer_;
    std::shared_ptr<MediaAVCodec::AVCodecVideoDecoder> videoDecoder_;

    Status InitDecoder();
    std::shared_ptr<Meta> GetVideoTrackInfo();
    bool ConvertToAVSharedMemory(const sptr<SurfaceBuffer> &surfaceBuffer);
    void ConvertP010ToNV12(
        const sptr<SurfaceBuffer> &surfaceBuffer, uint8_t *dstNV12, int32_t strideWidth, int32_t strideHeight);
    std::unique_ptr<PixelMap> GetYuvDataAlignStride(const sptr<SurfaceBuffer> &surfaceBuffer);
    Status SeekToTime(int64_t timeMs, Plugins::SeekMode option, int64_t realSeekTime);
};
}  // namespace Media
}  // namespace OHOS
#endif  // AV_THUMBNAIL_GENERATOR