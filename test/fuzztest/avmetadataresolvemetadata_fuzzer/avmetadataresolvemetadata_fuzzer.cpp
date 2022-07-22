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

#include "avmetadataresolvemetadata_fuzzer.h"
#include <iostream>
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"
#include "image_type.h"


using namespace std;
using namespace OHOS;
using namespace Media;

ResolveMetadataFuzzer::ResolveMetadataFuzzer()
{
}

ResolveMetadataFuzzer::~ResolveMetadataFuzzer()
{
}

bool ResolveMetadataFuzzer::FuzzResolveMetadata(uint8_t* data, size_t size)
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

    int32_t AVMetadataCode_[17] {
        AV_KEY_ALBUM,
        AV_KEY_ALBUM_ARTIST,
        AV_KEY_ARTIST,
        AV_KEY_AUTHOR,
        AV_KEY_DATE_TIME,
        AV_KEY_COMPOSER,
        AV_KEY_DURATION,
        AV_KEY_GENRE,
        AV_KEY_HAS_AUDIO,
        AV_KEY_HAS_VIDEO,
        AV_KEY_MIME_TYPE,
        AV_KEY_NUM_TRACKS,
        AV_KEY_SAMPLE_RATE,
        AV_KEY_TITLE,
        AV_KEY_VIDEO_HEIGHT,
        AV_KEY_VIDEO_WIDTH,
        AV_KEY_VIDEO_ORIENTATION
    };
    int32_t key = AVMetadataCode_[*reinterpret_cast<int64_t *>(data) % 17];
    std::string ret_resolvemetadata = metadata_ -> ResolveMetadata(key);
    if (ret_resolvemetadata.empty()) {
        cout << "metadata_ ResolveMetadata fail" << endl;
        return false;
    }
    
    metadata_ -> Release();

    return true;
}

bool OHOS::Media::FuzzTestResolveMetadata(uint8_t* data, size_t size)
{
    auto player = std::make_unique<ResolveMetadataFuzzer>();
    if (player == nullptr) {
        cout << "player is null" << endl;
        return 0;
    }
    return player -> FuzzResolveMetadata(data, size);
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::FuzzTestResolveMetadata(data, size);
    return 0;
}

