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

#include "pixelmap_fuzzer.h"
#include <iostream>
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"


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
    pixelmap_ = AVMetadataHelperFactory::CreateAVMetadataHelper();
    if(pixelmap_ == nullptr){
        cout << "pixelmap_ is null" << endl;
        return false;
    }

    const string path = "/data/media/H264_AAC.mp4";

    ret = pixelmap_ -> MetaDataSetSource(path);
    if(ret != 0){
        cout << "pixelmap_ SetSource file" << endl;
        return false;
    }

    if (size >= sizeof(int64_t)) {
        struct PixelMapParams pixelMapParams_;
        ret = pixelmap_ -> FetchFrameAtTime(*reinterpret_cast<int64_t *>(data), AV_META_QUERY_NEXT_SYNC, pixelMapParams_);
        if(ret != 0){
            cout << "pixelmap_ FetchFrameAtTime fail" << endl;
            return false;
        }
    }

    ret = pixelmap_ -> Release();
    if(ret != 0){
        cout << "pixelmap_ release fail" << endl;
        return false;
    }

}

bool OHOS::Media::FuzzTestPixelMap(uint8_t* data, size_t size)
{
    auto player = std::make_unique<MetaDataFuzzer>();
    if(player == nullptr){
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

