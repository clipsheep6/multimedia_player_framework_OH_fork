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

#ifndef ITRANSCODER_ENGINE_H
#define ITRANSCODER_ENGINE_H

#include <cstdint>
#include <string>
#include <memory>
#include <refbase.h>
#include "nocopyable.h"
#include "transcoder.h"
#include "transcoder_param.h"

namespace OHOS {
class Surface;

namespace Media {

/**
 * TransCoder Engine Observer. This is a abstract class, engine's user need to implement it and register
 * its instance into engine. The  transCoder engine will report itself's information or error asynchronously
 * to observer.
 */
class ITransCoderEngineObs : public std::enable_shared_from_this<ITransCoderEngineObs> {
public:
    virtual ~ITransCoderEngineObs() = default;
    virtual void OnError(TransCoderErrorType errorType, int32_t errorCode) = 0;
    virtual void OnInfo(TransCoderOnInfoType type, int32_t extra) = 0;
};

/**
 * TransCoder Engine Interface.
 */
class ITransCoderEngine {
public:
    virtual ~ITransCoderEngine() = default;

    /**
     * Sets the input file. The function must be called before Prepare.
     * After this interface called, the engine will not accept any source setting interface call.
     * Return MSERR_OK indicates success, or others indicate failed.
     */
    virtual int32_t SetInputFile(const std::string &url) = 0;

    /**
     * Sets the output file. The function must be called before Prepare. 
     * After this interface called, the engine will not accept any destination setting interface call.
     * Return MSERR_OK indicates success, or others indicate failed.
     */
    virtual int32_t SetOutputFile(const int32_t fd) = 0;

    /**
     * Sets the output file format. The function must be called after SetVideoSource or SetAudioSource, and before
     * Prepare. After this interface called, the engine will not accept any source setting interface call.
     * Return MSERR_OK indicates success, or others indicate failed.
     */
    virtual int32_t SetOutputFormat(OutputFormatType format) = 0;

    /**
     * Register a transCodering observer.
     * Return MSERR_OK indicates success, or others indicate failed.
     */
    virtual int32_t SetObs(const std::weak_ptr<ITransCoderEngineObs> &obs) = 0;

    /**
     * Configure static transCoder parameters before calling the Prepare interface. The interface must be called after
     * SetOutputFormat. The sourceId indicates the source ID, which can be obtained from SetVideoSource
     * or SetAudioSource. Use the DUMMY_SOURCE_ID  to configure the source-independent parameters.
     * Return MSERR_OK indicates success, or others indicate failed.
     */
    virtual int32_t Configure(const TransCoderParam &recParam) = 0;

    /**
     * Prepares for transCodering. This function must be called before Start. Ensure all required transCoder parameter
     * have already been set, or this call will be failed.
     * Return MSERR_OK indicates success, or others indicate failed.
     */
    virtual int32_t Prepare() = 0;

    /**
     * Starts transCodering. This function must be called after Prepare.
     * Return MSERR_OK indicates success, or others indicate failed.
     */
    virtual int32_t Start() = 0;

    /**
     * Pause transCodering. This function must be called during transCodering.
     * Return MSERR_OK indicates success, or others indicate failed.
     */
    virtual int32_t Pause() = 0;

    /**
     * Resume transCodering. This function must be called during transCodering. After called, the paused
     * transCodering willbe resumed.
     * Return MSERR_OK indicates success, or others indicate failed.
     */
    virtual int32_t Resume() = 0;

    /**
     * Resets the transCodering. After this interface called, anything need to be reconfigured.
     * Return MSERR_OK indicates success, or others indicate failed.
     */
    virtual int32_t Cancel() = 0;

    /**
     * Get current transcodering time.
     * Return MSERR_OK indicates success, or others indicate failed.
     */
    virtual int32_t GetCurrentTime(int32_t &currentTime) = 0;

    /**
     * Get the input file duration.
     * Return MSERR_OK indicates success, or others indicate failed.
     */
    virtual int32_t GetDuration(int32_t &duration) = 0;
};
} // namespace Media
} // namespace OHOS
#endif
