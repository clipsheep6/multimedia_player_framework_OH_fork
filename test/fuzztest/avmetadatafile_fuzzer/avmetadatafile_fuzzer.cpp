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

#include "avmetadatafile_fuzzer.h"
#include <iostream>
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"
#include "common.h"

using namespace std;
using namespace OHOS;
using namespace Media;

AVMetadataFileFuzzer::AVMetadataFileFuzzer()
{
}

AVMetadataFileFuzzer::~AVMetadataFileFuzzer()
{
}

bool AVMetadataFileFuzzer::FuzzAVMetadataFile(uint8_t* data, size_t size)
{
    metadata_ = AVMetadataHelperFactory::CreateAVMetadataHelper();
    if (metadata_ == nullptr) {
        cout << "metadata_ is null" << endl;
        return false;
    }

    const string path = "/data/test/resource/fuzztest.mp4";

    int32_t ret_writefile = WriteDataToFile(path, data, size);
    if (ret_writefile != 0) {
        cout << "metadata_ ret_writefile fail!" << endl;
        return false;
    }

    int32_t ret_metadatasetsource = MetaDataSetSource(path);
    if (ret_metadatasetsource != 0) {
        cout << "expect metadata_ SetSource file" << endl;
        return true;
    }

    std::unordered_map<int32_t, std::string> ret_resolve = metadata_ -> ResolveMetadata();
    if (ret_resolve.empty()) {
        cout << "expext metadata_ ResolveMetadata file" << endl;
        return true;
    }

    std::shared_ptr<AVSharedMemory> ret_fetchartpicture = metadata_ -> FetchArtPicture();
    if (ret_fetchartpicture == nullptr) {
        cout << "expect metadata_ FetchArtPicture file" << endl;
        return true;
    }
    
    metadata_ -> Release();

    return true;
}

bool OHOS::Media::FuzzTestavMetadataFile(uint8_t* data, size_t size)
{
    auto player = std::make_unique<AVMetadataFileFuzzer>();
    if (player == nullptr) {
        cout << "player is null" << endl;
        return 0;
    }
    return player -> FuzzAVMetadataFile(data, size);
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::FuzzTestavMetadataFile(data, size);
    return 0;
}

