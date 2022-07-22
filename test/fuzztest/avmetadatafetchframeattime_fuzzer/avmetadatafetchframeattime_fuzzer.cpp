/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "avmetadatafetchframeattime_fuzzer.h"
#include <iostream>
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"
#include "image_type.h"
#define random(x) (rand() % x)


using namespace std;
using namespace OHOS;
using namespace Media;

PixelMapFuzzer::PixelMapFuzzer()
{
}

PixelMapFuzzer::~PixelMapFuzzer()
{
}

bool PixelMapFuzzer::FuzzPixelMap(uint8_t* data, size_t size)
{
    metadata_ = AVMetadataHelperFactory::CreateAVMetadataHelper();
    if (metadata_ == nullptr) {
        cout << "metadata_ is null" << endl;
        return false;
    }

    const string path = "/data/test/resource/H264_AAC.mp4";
    int32_t ret_metadatasetsource = MetaDataSetSource(path);
    if (ret_metadatasetsource != 0) {
        cout << "metadata_ SetSource file" << endl;
        return false;
    }

    if (size >= sizeof(int64_t)) {
        int32_t nameListAVMetadataQueryOption[4] {
            AV_META_QUERY_NEXT_SYNC, 
            AV_META_QUERY_PREVIOUS_SYNC, 
            AV_META_QUERY_CLOSEST_SYNC, 
            AV_META_QUERY_CLOSEST
        };
        int32_t option = nameListAVMetadataQueryOption[random(4)];
        PixelFormat colorFormat_[11] {
            PixelFormat :: UNKNOWN, 
            PixelFormat :: ARGB_8888,
            PixelFormat :: RGB_565,
            PixelFormat :: RGBA_8888,
            PixelFormat :: BGRA_8888,
            PixelFormat :: RGB_888,
            PixelFormat :: ALPHA_8,
            PixelFormat :: RGBA_F16,
            PixelFormat :: NV21,
            PixelFormat :: NV12,
            PixelFormat :: CMYK
        };
        PixelFormat colorFormat = colorFormat_[random(11)];

        int32_t dstWidth = rand();

        int32_t dstHeight = rand();

        struct PixelMapParams pixelMapParams_ = {
            dstWidth, 
            dstHeight, 
            colorFormat
        };
        
        std::shared_ptr<PixelMap> ret_fetchframeattime = metadata_ -> FetchFrameAtTime(*reinterpret_cast<int64_t *>(data),
            option, pixelMapParams_);

        if (ret_fetchframeattime != 0) {
            cout << "metadata_ FetchFrameAtTime fail" << endl;
            return false;
        }
    }
    metadata_ -> Release();

    return true;
}

bool OHOS::Media::FuzzTestPixelMap(uint8_t* data, size_t size)
{
    auto player = std::make_unique<PixelMapFuzzer>();
    if (player == nullptr) {
        cout << "player is null" << endl;
        return 0;
    }
    return player -> FuzzPixelMap(data, size);
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::FuzzTestPixelMap(data, size);
    return 0;
}

