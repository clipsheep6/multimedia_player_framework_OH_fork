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

#ifndef VIDEODECODER_SERVICE_SERVER_H
#define VIDEODECODER_SERVICE_SERVER_H

#include "i_videodecoder_service.h"
#include "i_videodecoder_engine.h"
#include "time_monitor.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class VideoDecoderServer : public IVideoDecoderService, public IVideoDecoderEngineObs {
public:
    static std::shared_ptr<IVideoDecoderService> Create();
    VideoDecoderServer();
    ~VideoDecoderServer();
    DISALLOW_COPY_AND_MOVE(VideoDecoderServer);

    enum VideoDecoderStatus {
        VDEC_UNINITIALIZED = 0,
        VDEC_CONFIGURED,
        VDEC_RUNNING,
        VDEC_EOS,
        VDEC_ERROR,
    };

    int32_t Configure(const std::vector<Format> &format, sptr<Surface> surface) override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    int32_t Release() override;
    std::shared_ptr<AVSharedMemory> GetInputBuffer(uint32_t index) override;
    int32_t PushInputBuffer(uint32_t index, uint32_t offset, uint32_t size, int64_t presentationTimeUs,
                            VideoDecoderInputBufferFlag flags) override;
    int32_t ReleaseOutputBuffer(uint32_t index, bool render) override;
    int32_t SetCallback(const std::shared_ptr<VideoDecoderCallback> &callback) override;

    // IVideoDecoderEngineObs override
    void OnError(int32_t errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(std::vector<Format> format) override;
    void OnOutputBufferAvailable(uint32_t index, VideoDecoderBufferInfo info) override;
    void OnInputBufferAvailable(uint32_t index) override;

private:
    int32_t Init();

    VideoDecoderStatus status_ = VDEC_UNINITIALIZED;
    std::unique_ptr<IVideoDecoderEngine> videoDecoderEngine_;
    std::shared_ptr<VideoDecoderCallback> videoDecoderCb_;
    std::mutex mutex_;
    std::mutex cbMutex_;
};
} // namespace Media
} // namespace OHOS
#endif // VIDEODECODER_SERVICE_SERVER_H
