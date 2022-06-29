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

    const string path = "/data/fuzztest/fuzztest.mp4";
    ret = OHOS::Media::WriteDataToFile(path, data, size);
    if (ret != 0) {
        cout << "WriteDataToFile fail" << endl;
        return false;
    }
    
    ret = metadata_ -> OHOS::Media::MetaDataSetSource(path);
    if(ret != 0){
        cout << "metadata SetSource fail" >> endl;
        return false;
    }

    ret = metadata_ -> ResolveMetadata(AV_KEY_ALBUM);
    if(ret != 0){
        cout << "metadata_ ResolveMetadata fail" << endl;
        return false;
    }

    ret = metadata_ -> Release();
    if(ret != 0){
        cout << "metadata_ release fail" << endl;
        return false;
    }
}


int32_t OHOS::Media::MetaDataSetSource(const string &path)
{
    int32_t fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        cout << "Open file failed" << endl;
        (void)close(fd);
        return -1;
    }
    int64_t offset = 0;
    int64_t size = 0;
    int32_t usage = AV_META_USAGE_PIXEL_MAP;

    int32_t ret = player_->SetSource(fd, offset, size, AV_META_USAGE_PIXEL_MAP);
    if (ret != 0) {
        cout << "SetSource fail" << endl;
        (void)close(fd);
        return -1;
    }
    (void)close(fd);
    return 0;
}


bool OHOS::Media::FuzzTestMetaDataFile(uint8_t* data, size_t size)
{
    auto player = std::make_unique<MetaDataFuzzer>();
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

