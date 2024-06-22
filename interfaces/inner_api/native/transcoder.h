/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef TRANSCODER_H
#define TRANSCODER_H

#include <cstdint>
#include <string>
#include <map>
#include <set>
#include <parcel.h>
#include "meta/format.h"
#include "surface.h"
#include "recorder.h"
#include "av_common.h"
#include "codec_capability.h"
#include "media_core.h"

namespace OHOS {
namespace Media {
using ConfigMap = std::map<std::string, int32_t>;

/**
 * @brief Enumerates transcodering error types.
 *
 * @since 5.0
 * @version 5.0
 */
enum TransCoderErrorType : int32_t {
    TRANSCODER_ERROR_INTERNAL
};

enum TransCoderOnInfoType : int32_t {
    /* return the current progress of transcoder automatically. */
    INFO_TYPE_TRANSCODER_COMPLETED = 0,
    /* return the current progress of transcoder automatically. */
    INFO_TYPE_PROGRESS_UPDATE = 1,
};

/**
 * @brief Provides listeners for transcodering errors and information events.
 *
 * @since 5.0
 * @version 5.0
 */
class TransCoderCallback {
public:
    virtual ~TransCoderCallback() = default;

    /**
     * @brief Called when an error occurs during transcodering. This callback is used to report transcodering errors.
     *
     * @param errorType Indicates the error type. For details, see {@link TransCoderErrorType}.
     * @param errorCode Indicates the error code.
     * @since 1.0
     * @version 1.0
     */
    virtual void OnError(TransCoderErrorType errorType, int32_t errorCode) = 0;

    /**
     * @brief Called when an information event occurs during transcodering. This callback is used to report
     * transcodering information.
     *
     * @param type Indicates the information type. For details, see {@link TransCoderInfoType}.
     * @param extra Indicates other information, for example, the start time position of a transcodering file.
     * @since 1.0
     * @version 1.0
     */
    virtual void OnInfo(int32_t type, int32_t extra) = 0;
};

/**
 * @brief Provides functions for audio and video transcodering.
 *
 * @since 1.0
 * @version 1.0
 */
class TransCoder {
public:
    virtual ~TransCoder() = default;

    /**
     * @brief Sets the output file format.
     *
     * This function must be called before {@link Prepare} and after after all required sources have been set. After
     * this function called, no more source settings allowed.
     *
     * @param format Indicates the output file format. For details, see {@link OutputFormatType}.
     * @return Returns {@link MSERR_OK} if the setting is successful; returns an error code otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetOutputFormat(OutputFormatType format) = 0;

    /**
     * @brief Sets the encoder of the video to transcoder.
     *
     * If this function is not called, the output file does not contain the video track when the video source is
     * YUV or RGB.
     * This function must be called after {@link SetOutputFormat} but before {@link Prepare}.
     *
     * @param encoder Indicates the video encoder to set. For details, see {@link VideoCodecFormat}.
     * @return Returns {@link MSERR_OK} if the setting is successful; returns an error code otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetVideoEncoder(VideoCodecFormat encoder) = 0;

    /**
     * @brief Sets the encoding bit rate of the video to transcoder.
     *
     * This function must be called after {@link SetOutputFormat} but before {@link Prepare}.
     *
     * @param rate Indicates the encoding bit rate to set.
     * @return Returns {@link MSERR_OK} if the setting is successful; returns an error code otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetVideoEncodingBitRate(int32_t rate) = 0;

    /**
     * @brief Sets the encoding video size of the video to transcoder.
     *
     * This function must be called after {@link SetOutputFormat} but before {@link Prepare}.
     *
     * @param rate Indicates the encoding bit rate to set.
     * @return Returns {@link MSERR_OK} if the setting is successful; returns an error code otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetVideoSize(int32_t videoFrameWidth, int32_t videoFrameHeight) = 0;

    /**
     * @brief Sets the encoder of the audio to transcoder.
     *
     * If this function is not called, the output file does not contain the audio track.
     * This function must be called after {@link SetOutputFormat} but before {@link Prepare}.
     *
     * @param encoder Indicates the audio encoder to set.
     * @return Returns {@link MSERR_OK} if the setting is successful; returns an error code otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetAudioEncoder(AudioCodecFormat encoder) = 0;

    /**
     * @brief Sets the encoding bit rate of the audio to record.
     *
     * This function must be called after {@link SetOutputFormat} but before {@link Prepare}.
     *
     * @param bitRate Indicates the audio encoding bit rate, in bit/s.
     * @return Returns {@link MSERR_OK} if the setting is successful; returns an error code otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetAudioEncodingBitRate(int32_t bitRate) = 0;

    /**
     * @brief Sets the file url of the input file.
     *
     * @param url Indicates the url of the file.
     * @return Returns {@link MSERR_OK} if the setting is successful; returns an error code otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetInputFile(std::string url) = 0;

    /**
     * @brief Sets the file descriptor (FD) of the input file.
     *
     * @param fd Indicates the FD of the file.
     * @return Returns {@link MSERR_OK} if the setting is successful; returns an error code otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetInputFile(int32_t fd, int64_t offset, int64_t size) = 0;

    /**
     * @brief Sets the file descriptor (FD) of the output file.
     *
     * This function must be called after {@link SetOutputFormat} but before {@link Prepare}
     *
     * @param fd Indicates the FD of the file.
     * @return Returns {@link MSERR_OK} if the setting is successful; returns an error code otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetOutputFile(int32_t fd) = 0;

    /**
     * @brief Registers a transcodering listener.
     *
     * This function must be called after {@link SetOutputFormat} but before {@link Prepare}
     *
     * @param callback Indicates the recording listener to register. For details, see {@link TransCoderCallback}.
     * @return Returns {@link MSERR_OK} if the setting is successful; returns an error code otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetTransCoderCallback(const std::shared_ptr<TransCoderCallback> &callback) = 0;

    /**
     * @brief Prepares for transcodering.
     *
     * This function must be called before {@link Start}.
     *
     * @return Returns {@link MSERR_OK} if the preparation is successful; returns an error code otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Prepare() = 0;

    /**
     * @brief Starts transcodering.
     *
     * This function must be called after {@link Prepare}.
     *
     * @return Returns {@link MSERR_OK} if the transcodering is started; returns an error code otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Start() = 0;

    /**
     * @brief Pauses transcodering.
     *
     * After {@link Start} is called, you can call this function to pause transcodering.
     *
     * @return Returns {@link MSERR_OK} if the transcodering is paused; returns an error code otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Pause() = 0;

    /**
    * @brief Resumes transcodering.
    *
    * You can call this function to resume transcodering after {@link Pause} is called.
     *
     * @return Returns {@link MSERR_OK} if the transcodering is resumed; returns an error code otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Resume() = 0;

    /**
     * @brief Cancel transcodering.
     *
     * @param block Indicates the stop mode. The value <b>true</b> indicates that the processing stops after all caches
     * are processed, and <b>false</b> indicates that the processing stops immediately and all caches are discarded.
     * After the transcodering stopped, all sources and parameters must be set again to restore transcodering.
     * The function is like to {@link Reset}, except that the block parameter is allowed to be specified.
     * @return Returns {@link MSERR_OK} if the transcodering is stopped; returns an error code otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Cancel() = 0;

    /**
     * @brief Releases transcodering resources. After this function called, none of interfaces of {@link Transcoder}
     * can be used.
     *
     * @return Returns {@link MSERR_OK} if the transcodering is stopped; returns an error code otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Release() = 0;
};

class __attribute__((visibility("default"))) TransCoderFactory {
public:
#ifdef UNSUPPORT_TRANSCODER
    static std::shared_ptr<TransCoder> CreateTransCoder()
    {
        return nullptr;
    }
#else
    static std::shared_ptr<TransCoder> CreateTransCoder();
#endif
private:
    TransCoderFactory() = default;
    ~TransCoderFactory() = default;
};
} // namespace Media
} // namespace OHOS
#endif // TRANSCODER_H
