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

#ifndef VIDEO_CAPTURE_SF_IMPL_H
#define VIDEO_CAPTURE_SF_IMPL_H

#include <fstream>
#include <atomic>
#include <thread>
#include "video_capture.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class VideoCaptureSfImpl : public VideoCapture {
public:
    VideoCaptureSfImpl();
    virtual ~VideoCaptureSfImpl();
    DISALLOW_COPY_AND_MOVE(VideoCaptureSfImpl);

    int32_t Prepare() override;
    int32_t Start() override;
    int32_t Pause() override;
    int32_t Resume() override;
    int32_t Stop() override;
    int32_t SetVideoWidth(uint32_t width) override;
    int32_t SetVideoHeight(uint32_t height) override;
    int32_t SetStreamType(VideoStreamType streamType) override;
    sptr<Surface> GetSurface() override;
    std::shared_ptr<EsAvcCodecBuffer> GetCodecBuffer() override;
    std::shared_ptr<VideoFrameBuffer> GetFrameBuffer() override;
    void SetSuspend(bool suspend) override;
    void SetRepeat(uint64_t time) override;
    void UnLock(bool start) override;

protected:
    virtual std::shared_ptr<EsAvcCodecBuffer> DoGetCodecBuffer() = 0;
    virtual std::shared_ptr<VideoFrameBuffer> DoGetFrameBuffer() = 0;

    class ConsumerListenerProxy : public IBufferConsumerListener {
    public:
        explicit ConsumerListenerProxy(VideoCaptureSfImpl &owner) : owner_(owner) {}
        ~ConsumerListenerProxy() = default;
        DISALLOW_COPY_AND_MOVE(ConsumerListenerProxy);
        void OnBufferAvailable() override;
    private:
        VideoCaptureSfImpl &owner_;
    };
    void OnBufferAvailable();
    int32_t GetBufferExtraData();

    uint32_t videoWidth_;
    uint32_t videoHeight_;
    int32_t fence_;
    int32_t bufferAvailableCount_;
    int64_t timestamp_;
    Rect damage_;
    sptr<SurfaceBuffer> surfaceBuffer_;
    std::atomic<bool> started_;
    std::atomic<bool> paused_;
    std::mutex mutex_;
    std::condition_variable bufferAvailableCondition_;
    VideoStreamType streamType_;
    bool streamTypeUnknown_;
    sptr<Surface> dataConSurface_;
    sptr<Surface> producerSurface_;
    int32_t dataSize_ = 0;
    int64_t pts_ = 0;
    int32_t isCodecFrame_ = 0;

private:
    void SetSurfaceUserData();
    int32_t AcquireSurfaceBuffer();
    std::shared_ptr<VideoFrameBuffer> GetFrameBufferInner();
    void ProbeStreamType();
    void HandleSuspendBuffer();

    uint32_t bufferNumber_ = 0;
    int64_t previousTimestamp_ = 0;
    int64_t pauseTime_ = 0;
    int64_t resumeTime_ = 0;
    int64_t persistTime_ = 0;
    uint32_t pauseCount_ = 0;
    int64_t totalPauseTime_ = 0;
    uint64_t repeatFrameAfterUs_ = 0;
    bool needRepeatFrame_ = false;
    bool suspend_ = false;
    bool resourceLock_ = false;
};
}  // namespace Media
}  // namespace OHOS
#endif // VIDEO_CAPTURE_AS_IMPL_H
