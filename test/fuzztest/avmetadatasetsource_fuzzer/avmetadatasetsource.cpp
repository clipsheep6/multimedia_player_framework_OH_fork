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

#include "avmetadatasetsource.h"
#include <iostream>
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"
#include "image_type.h"
#define random(x) ( rand()%x )


using namespace std;
using namespace OHOS;
using namespace Media;

AVMetadataSetSourceFuzzer::AVMetadataSetSourceFuzzer()
{
}

AVMetadataSetSourceFuzzer::~AVMetadataSetSourceFuzzer()
{
}

bool AVMetadataSetSourceFuzzer::FuzzAVMetadataSetSource(uint8_t* data, size_t size)
{
    metadata_ = AVMetadataHelperFactory::CreateAVMetadataHelper();
    if (metadata_ == nullptr) {
        cout << "metadata_ is null" << endl;
        return false;
    }

    const string path = "/data/test/resource/H264_AAC.mp4";
    int32_t setsource_fd = open(path.c_str(), O_RDONLY);
    if (setsource_fd < 0) {
        cout << "Open file failed" << endl;
        (void)close(setsource_fd);
        return false;
    }

    int64_t setsource_offset = *reinterpret_cast<int64_t *>(data);

    struct stat64 buffer;
    if (fstat64(setsource_fd, &buffer) != 0) {
        cout << "Get file state failed" << endl;
        (void)close(setsource_fd);
        return false;
    }
    int64_t setsource_size = static_cast<int64_t>(buffer.st_size);
    AVMetadataUsage usage[2] {AVMetadataUsage::AV_META_USAGE_META_ONLY, AVMetadataUsage::AV_META_USAGE_PIXEL_MAP};
    int32_t setsource_usage = usage[random(2)];
    cout << "setsource_fd : " << setsource_fd << "; setsource_offset : " << setsource_offset << "; setsource_size : " << setsource_size << ": setsource_usage : " << setsource_usage << endl;



    int32_t ret_setsource = metadata_ -> SetSource(setsource_fd, setsource_offset, setsource_size, setsource_usage);
    if (ret_setsource != 0) {
        cout << "SetSource fail!" << endl;
        (void)close(setsource_fd);
        return false;
    }
    (void)close(setsource_fd);

    std::unordered_map<int32_t, std::string> ret_resolvenetadata = metadata_ -> ResolveMetadata();

    if (ret_resolvenetadata.empty()) {
        cout << "expect metadata_ FetchFrameAtTime fail" << endl;
        return false;
    }
    
    metadata_ -> Release();

    return true;
}

bool OHOS::Media::FuzzTestAVMetadataSetSource(uint8_t* data, size_t size)
{
    auto player = std::make_unique<AVMetadataSetSourceFuzzer>();
    if (player == nullptr) {
        cout << "player is null" << endl;
        return 0;
    }
    return player -> FuzzAVMetadataSetSource(data, size);
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::FuzzTestAVMetadataSetSource(data, size);
    return 0;
}

