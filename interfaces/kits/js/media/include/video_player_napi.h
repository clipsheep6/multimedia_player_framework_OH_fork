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

#ifndef VIDEO_PLAYER_NAPI_H_
#define VIDEO_PLAYER_NAPI_H_

#include "player.h"
#include "media_errors.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "media_data_source_callback.h"

namespace OHOS {
namespace Media {
class VideoPlayerNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);

private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    static napi_value CreateVideoPlayer(napi_env env, napi_callback_info info);
    static napi_value Prepare(napi_env env, napi_callback_info info);
    static napi_value Play(napi_env env, napi_callback_info info);
    static napi_value Pause(napi_env env, napi_callback_info info);
    static napi_value Stop(napi_env env, napi_callback_info info);
    static napi_value Reset(napi_env env, napi_callback_info info);
    static napi_value Release(napi_env env, napi_callback_info info);
    /**
     * seek(timeMs: number, callback:AsyncCallback<number>): void
     * seek(timeMs: number, mode:SeekMode, callback:AsyncCallback<number>): void
     * seek(timeMs: number): Promise
     * seek(timeMs: number, mode:SeekMode): Promise
     */
    static napi_value Seek(napi_env env, napi_callback_info info);
    static napi_value SetSpeed(napi_env env, napi_callback_info info);
    static napi_value SetVolume(napi_env env, napi_callback_info info);
    static napi_value SetUrl(napi_env env, napi_callback_info info);
    static napi_value GetUrl(napi_env env, napi_callback_info info);
    static napi_value GetDataSrc(napi_env env, napi_callback_info info);
    static napi_value SetDataSrc(napi_env env, napi_callback_info info);
    static napi_value SetLoop(napi_env env, napi_callback_info info);
    static napi_value GetLoop(napi_env env, napi_callback_info info);
    static napi_value GetCurrentTime(napi_env env, napi_callback_info info);
    static napi_value GetDuration(napi_env env, napi_callback_info info);
    static napi_value GetState(napi_env env, napi_callback_info info);
    static napi_value GetWidth(napi_env env, napi_callback_info info);
    static napi_value GetHeight(napi_env env, napi_callback_info info);
    static napi_value GetTrackDescription(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);

    static void AsyncCreateVideoPlayer(napi_env env, void *data);
    static void AsyncWork(napi_env env, void *data);
    static void AsyncGetTrackDescription(napi_env env, void *data);
    static void CompleteAsyncFunc(napi_env env, napi_status status, void *data);
    void OnErrorCallback(MediaServiceExtErrCode errCode);
    void ReleaseDataSource(std::shared_ptr<MediaDataSourceCallback> dataSourceCb);
    VideoPlayerNapi();
    ~VideoPlayerNapi();

    static napi_ref constructor_;
    napi_env env_ = nullptr;
    napi_ref wrapper_ = nullptr;
    std::shared_ptr<Player> nativePlayer_ = nullptr;
    std::shared_ptr<PlayerCallback> callbackNapi_ = nullptr;
    std::shared_ptr<MediaDataSourceCallback> dataSrcCallBack_ = nullptr;
    std::string url_ = "";
    std::vector<VideoTrackInfo> videoTranckInfoVec_;
};
} // namespace Media
} // namespace OHOS
#endif /* VIDEO_PLAYER_NAPI_H_ */
