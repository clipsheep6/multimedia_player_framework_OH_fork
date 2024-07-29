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

#ifndef I_TRANSCODER_SERVICE_H
#define I_TRANSCODER_SERVICE_H

#include <string>
#include "transcoder.h"
#include "refbase.h"

namespace OHOS {
namespace Media {
class ITransCoderService {
public:
    virtual ~ITransCoderService() = default;

    /**
     * @brief Sets the encoder of the video to transcoder.
     *
     * If this function is not called, the output file does not contain the video track.
     * This function must be called after {@link SetVideoSource} but before {@link Prepare}.
     *
     * @param encoder Indicates the video encoder to set.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetVideoEncoder(VideoCodecFormat encoder) = 0;

    /**
     * @brief Sets the encoding video size of the video to transcoder.
     *
     * This function must be called after {@link SetVideoSource} but before {@link Prepare}.
     *
     * @param width Indicates the video width to set.
     * @param height Indicates the video height to set.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetVideoSize(int32_t width, int32_t height) = 0;

    /**
     * @brief Sets the encoding bit rate of the video to transcoder.
     *
     * This function must be called after {@link SetVideoSource} but before {@link Prepare}.
     *
     * @param rate Indicates the encoding bit rate to set.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetVideoEncodingBitRate(int32_t rate) = 0;

    /**
     * @brief Sets the encoder of the audio to transcoder.
     *
     * If this function is not called, the output file does not contain the audio track.
     * This function must be called after {@link SetAudioSource} but before {@link Prepare}.
     *
     * @param encoder Indicates the audio encoder to set.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetAudioEncoder(AudioCodecFormat encoder) = 0;

    /**
     * @brief Sets the encoding bit rate of the audio to transcoder.
     *
     * This function must be called after {@link SetAudioSource} but before {@link Prepare}.
     *
     * @param bitRate Indicates the audio encoding bit rate, in bit/s.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetAudioEncodingBitRate(int32_t bitRate) = 0;

    /**
     * @brief Sets the output file format.
     *
     * This function must be called before {@link Prepare}.
     *
     * @param format Indicates the output file format. For details, see {@link OutputFormatType}.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetOutputFormat(OutputFormatType format) = 0;

    /**
     * @brief Sets the file descriptor (FD) of the input file.
     *
     * This function must be called before {@link Prepare}.
     *
     * @param fd Indicates the FD of the file.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetInputFile(int32_t fd, int64_t offset, int64_t size) = 0;

    /**
     * @brief Sets the file descriptor (FD) of the output file.
     *
     * This function must be called before {@link Prepare}.
     *
     * @param fd Indicates the FD of the file.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetOutputFile(int32_t fd) = 0;

    /**
     * @brief Registers a transcodering listener.
     *
     * This function must be called before {@link Prepare}.
     *
     * @param callback Indicates the transcodering listener to register. For details, see {@link TransCoderCallback}.
     * @return Returns {@link SUCCESS} if the listener is registered; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetTransCoderCallback(const std::shared_ptr<TransCoderCallback> &callback) = 0;

    /**
     * @brief Prepares for transcodering.
     *
     * This function must be called before {@link Start}.
     *
     * @return Returns {@link SUCCESS} if the preparation is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Prepare() = 0;

    /**
     * @brief Starts transcodering.
     *
     * This function must be called after {@link Prepare}.
     *
     * @return Returns {@link SUCCESS} if the transcodering is started; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Start() = 0;

    /**
     * @brief Pauses transcodering.
     *
     * After {@link Start} is called, you can call this function to pause transcodering.
     *
     * @return Returns {@link SUCCESS} if the transcodering is paused; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Pause() = 0;

    /**
    * @brief Resumes transcodering.
    *
    * You can call this function to resume transcodering after {@link Pause} is called.
     *
     * @return Returns {@link SUCCESS} if the transcodering is resumed; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Resume() = 0;

    /**
     * @brief Cancels the transcodering.
     *
     * After the function is called, add a transcodering is cancelled.
     *
     * @return Returns {@link SUCCESS} if the transcodering is cancel; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Cancel() = 0;

    /**
     * @brief Releases transcodering resources.
     *
     * @return Returns {@link SUCCESS} if transcodering resources are released; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Release() = 0;
};
} // namespace Media
} // namespace OHOS
#endif // I_TRANSCODER_SERVICE_H
