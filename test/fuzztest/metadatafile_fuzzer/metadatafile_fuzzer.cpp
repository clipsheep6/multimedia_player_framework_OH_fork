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

#include "metadatafile_fuzzer.h"
#include <iostream>
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;

MetaDataFileFuzzer::MetaDataFileFuzzer()
{
}
MetaDataFileFuzzer::~MetaDataFileFuzzer()
{
}

bool MetaDataFileFuzzer :: FuzzMetaDataFile(uint8_t* data, size_t size)
{
    metadata_ = OHOS::Media::AVMetadataHelperFactory::CreateAVMetadataHelper();
    if(metadata_ == nullptr){
        cout << "metadata_ is null" << endl;
        return false;
    }
    cout << "create metadata_ success" << endl;
    const string path = "/data/media/fuzztest.mp4";
    int32_t ret = WriteDataToFile_METADATA(path, data, size);
    if (ret != 0) {
        cout << "WriteDataToFile_METADATA fail" << endl;
        return false;
    }
    cout << "metadata_ WriteDataToFile_METADATA success" << endl;
    int32_t ret2 = MetaDataSetSource(path);
    if(ret2 != 0){
        cout << "metadata SetSource fail" << endl;
        return false;
    }
    cout << "metadata_ SetSource success" << endl;
    std::string ret3 = metadata_ -> ResolveMetadata(AV_KEY_ALBUM);
    if(ret3.empty()){
        cout << "metadata_ ResolveMetadata fail" << endl;
        return false;
    }
    cout << "metadata_ ResolveMetadata fail" << endl;
    metadata_ -> Release();
    cout << "success!" << endl;
    return true;
}

bool OHOS::Media::FuzzTestMetaDataFile(uint8_t* data, size_t size)
{
    auto player = std::make_unique<MetaDataFileFuzzer>();
    if (player == nullptr){
        cout << "player is null" << endl;
        return 0;
    }
    return player -> FuzzMetaDataFile(data, size);
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::FuzzTestMetaDataFile(data, size);
    return 0;
}

