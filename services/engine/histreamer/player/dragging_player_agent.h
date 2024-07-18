/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
 
#ifndef DRAGGING_PLAYER_AGENT_H
#define DRAGGING_PLAYER_AGENT_H
 
#include "dragging_player.h"
#include "common/status.h"
 
namespace OHOS {
namespace Media {
using namespace std;
using namespace Pipeline;
class DraggingPlayerAgent : public std::enable_shared_from_this<DraggingPlayerAgent> {
public:
    static shared_ptr<DraggingPlayerAgent> Create();
    DraggingPlayerAgent() {};
    DraggingPlayerAgent(const DraggingPlayerAgent &) = delete;
    DraggingPlayerAgent operator=(const DraggingPlayerAgent &) = delete;
    ~DraggingPlayerAgent();
    Status Init(const shared_ptr<DemuxerFilter> &demuxer, const shared_ptr<DecoderSurfaceFilter> &decoder);
    void ConsumeVideoFrame(const std::shared_ptr<AVBuffer> avBuffer, uint32_t bufferIndex);
    bool IsVideoStreamDiscardable(const std::shared_ptr<AVBuffer> avBuffer);
    void UpdateSeekPos(int64_t seekMs);
    void Release();
 
private:
    static bool LoadSymbol();
    static void *LoadLibrary(const std::string &path);
    static bool CheckSymbol(void *handler);
    static mutex mtx_;
    static void *handler_;
    using CreateFunc = DraggingPlayer *(*)();
    using DestroyFunc = void (*)(DraggingPlayer *);
    static CreateFunc createFunc_;
    static DestroyFunc destroyFunc_;
    DraggingPlayer *draggingPlayer_ {nullptr};
    shared_ptr<VideoStreamReadyCallback> videoStreamReadyCb_ {nullptr};
    shared_ptr<VideoFrameReadyCallback> videoFrameReadyCb_ {nullptr};
    shared_ptr<DemuxerFilter> demuxer_ {nullptr};
    shared_ptr<DecoderSurfaceFilter> decoder_ {nullptr};
    bool isReleased_ {false};
};
 
} // namespace Media
} // namespace OHOS
#endif // DRAGGING_PLAYER_AGENT_H