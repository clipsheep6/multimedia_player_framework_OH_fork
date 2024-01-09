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

#include <mutex>
#include <condition_variable>
#include "nocopyable.h"
#include "i_avmetadatahelper_service.h"
#include "i_avmetadatahelper_engine.h"
#include "common/status.h"
#include "pipeline/pipeline.h"
#include "filter/filter_factory.h"
#include "demuxer_filter.h"
#include "media_demuxer.h"
#include "avmetadata_collector.h"

namespace OHOS {
namespace Media {
class AVMetadataHelperImpl : public IAVMetadataHelperEngine, public NoCopyable {
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

private:
    Status SetSourceInternel(const std::string &uri, int32_t usage);
    Status SetSourceInternel(const std::shared_ptr<IMediaDataSource> &dataSrc);
    Status PrepareInternel();
    int32_t ExtractMetadata();
    void Reset();

    std::unordered_map<int32_t, std::string> collectedMeta_;
    bool hasCollectMeta_ = false;
    int32_t usage_ = AVMetadataUsage::AV_META_USAGE_PIXEL_MAP;

    std::shared_ptr<AVSharedMemory> collectedArtPicture_;
    
    std::mutex mutex_;
    std::condition_variable cond_;
    bool errHappened_ = false;
    bool firstFetch_ = true;

    std::shared_ptr<OHOS::Media::Pipeline::Pipeline> pipeline_;
    std::shared_ptr<OHOS::Media::Pipeline::DemuxerFilter> demuxerFilter_;
    std::shared_ptr<OHOS::Media::MediaDemuxer> mediaDemuxer_;
    std::shared_ptr<AVMetaDataCollector> metaCollector_;
};
} // namespace Media
} // namespace OHOS
#endif // AVMETADATAHELPER_HST_IMPL_H