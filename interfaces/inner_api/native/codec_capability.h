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

#ifndef CODEC_CAPABILITY_H
#define CODEC_CAPABILITY_H

#include <cstdint>
#include <memory>
#include <vector>
#include "av_common.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class CodecRange {
public:
    int32_t minVal;
    int32_t maxVal;
    CodecRange() : minVal(0), maxVal(0) {}
    CodecRange(const int32_t &min, const int32_t &max)
    {
        if (min <= max) {
            this->minVal = min;
            this->maxVal = max;
        } else {
            this->minVal = 0;
            this->maxVal = 0;
        }
    }

    CodecRange Create(const int32_t &min, const int32_t &max)
    {
        return CodecRange(min, max);
    }

    bool Marshalling(Parcel &parcel) const
    {
        return parcel.WriteInt32(minVal)
            && parcel.WriteInt32(maxVal);
    }

    void Unmarshalling(Parcel &parcel)
    {
        minVal = parcel.ReadInt32();
        maxVal = parcel.ReadInt32();
    }
};

class EncoderCapabilityData {
public:
    std::string mimeType = "";
    std::string type = "";
    CodecRange bitrate;
    CodecRange frameRate;
    CodecRange width;
    CodecRange height;
    CodecRange channels;
    std::vector<int32_t> sampleRate;

    bool Marshalling(Parcel &parcel) const
    {
        if (!parcel.WriteString(mimeType)) {
            return false;
        }
        if (!parcel.WriteString(type)) {
            return false;
        }
        if (!(bitrate.Marshalling(parcel) && frameRate.Marshalling(parcel)
            && width.Marshalling(parcel) && height.Marshalling(parcel)
            && channels.Marshalling(parcel))) {
            return false;
        }
        size_t size = sampleRate.size();
        if (!parcel.WriteUint64(size)) {
            return false;
        }
        for (const auto &i : sampleRate) {
            if (!parcel.WriteInt32(i)) {
                return false;
            }
        }
        return true;
    }

    void Unmarshalling(Parcel &parcel)
    {
        mimeType = parcel.ReadString();
        type = parcel.ReadString();
        bitrate.Unmarshalling(parcel);
        frameRate.Unmarshalling(parcel);
        width.Unmarshalling(parcel);
        height.Unmarshalling(parcel);
        channels.Unmarshalling(parcel);
        size_t size = parcel.ReadUint64();
        for (size_t i = 0; i < size; i++) {
            sampleRate.push_back(parcel.ReadInt32());
        }
    }
};
} // namespace Media
} // namespace OHOS
#endif // CODEC_CAPABILITY_H