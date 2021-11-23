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

#ifndef MEDIA_TYPES_H
#define MEDIA_TYPES_H

#include <cstdint>

namespace OHOS {
namespace Media {
/**
 * @brief Enumetates frame flags
 */
enum FrameFlags : uint32_t {
    /**
     * @brief This flag indicates the end of stream, no buffers will be aviailable after this.
     */
    EOS_FRAME = 1,
    /**
     * @brief This flag indicates that the sample is a key frame.
     */
    SYNC_FRAME = 1 << 1,
    /**
     * @brief This flag indicates that the sample only contains part of a frame, the data
     * consumer should batch the data until a sample without this flag appears.
     */
    PARTIAL_FRAME = 1 << 2,
    /**
     * @brief This flag indicates that the sample contains codec specific data.
     */
    CODEC_DATA = 1 << 3,
};

/**
 * @brief Description information of a sample.
 */
struct SampleInfo {
    /**
     * @brief the presentation timestamp in microseconds.
     */
    int64_t timeUs;
    /**
     * @brief the amount of data (in bytes) in the sample.
     */
    int32_t size;
    /**
     * @brief the start-offset of the sample data in the buffer.
     */
    int32_t offset;
    /**
     * @brief the frame flags associated with the sample, this
     * maybe be a combination of multiple {@link FrameFlags}.
     */
    FrameFlags flags;
};

/**
 * @brief Description information of a sample associated a media track.
 */
struct TrackSampleInfo : public SampleInfo {
    /**
     * @brief the id of track that this sample belongs to.
     */
    int32_t trackIdx;
};

/**
 * @brief Enumetates the media track type
 */
enum MediaTrackType : uint8_t {
    /**
     * @brief Track is audio
     */
    MEDIA_TYPE_AUDIO = 0,
    /**
     * @brief Track is video
     */
    MEDIA_TYPE_VIDEO = 1,
    /**
     * @brief Track is subtitle
     */
    MEDIA_TYPE_SUBTITLE = 2,
};

/**
 * @brief Enumerates the video or image pixel format supported by media service
 */
enum MediaPixelFormat : uint32_t {
    /**
     * @brief YUV 420 planar.
     */
    YUVI420 = 0,
    /**
     * @brief NV12. yuv 420 semiplanar
     */
    NV12 = 1,
    /**
     * @brief NV21. yuv 420 semiplanar
     */
    NV21 = 2,
};
}
}
#endif
