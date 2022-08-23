/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "vcodecfile_fuzzer.h"
#include <iostream>
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"


using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::Media::VCodecTestParam;


VCodecFileFuzzer::VCodecFileFuzzer()
{
}

VCodecFileFuzzer::~VCodecFileFuzzer()
{
}

bool VCodecFileFuzzer::FuzzVideoFile(uint8_t* data, size_t size)
{
    while(true) {
        std::shared_ptr<VDecSignal> vdecSignal = std::make_shared<VDecSignal>();
        vdecCallback_ = std::make_shared<VDecCallbackTest>(vdecSignal);
        videoDec_ = std::make_shared<VDecMock>(vdecSignal);
        bool createDecSuccess = videoDec_->CreateVideoDecMockByMine("video/avc");
        if (createDecSuccess == false) {
            cout << "create video decoder fail" << endl;
            return false;   
        }
        int32_t ret = videoDec_->SetCallback(vdecCallback_);
        if (ret != 0) {
            cout << "DEC SetPlayerCallback fail" << endl;
        }

        std::shared_ptr<VEncSignal> vencSignal = std::make_shared<VEncSignal>();
        vencCallback_ = std::make_shared<VEncCallbackTest>(vencSignal);
        videoEnc_ = std::make_shared<VEncMock>(vencSignal);
        bool createEncSuccess = videoEnc_->CreateVideoEncMockByMine("video/avc");
        if (createEncSuccess == false) {
            cout << "create video encoder fail" << endl;
            return false;
        }
        ret = videoEnc_->SetCallback(vencCallback_);
        if (ret != 0) {
            cout << "DEC SetPlayerCallback fail" << endl;
        }
        string prefix = "/data/test/media/";
        string fileName = "avc_out";
        string suffix = ".es";
        videoEnc_->SetOutPath(prefix + fileName + suffix);

        const string path = "/data/test/media/fuzztest.mp4";
        ret = WriteDataToFile(path, data, size);
        if (ret != 0) {
            cout << "WriteDataToFile fail" << endl;
            return false;
        }
        videoDec_->SetReadPath(path);
        std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
        if (format == nullptr) {
            cout << "create format fail" << endl;
            return false;
        }
        string width = "width";
        string height = "height";
        string pixelFormat = "pixel_format";
        string frame_rate = "frame_rate";

        (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
        (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
        (void)format->PutIntValue(pixelFormat.c_str(), NV12);
        (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
        ret =  videoEnc_->Configure(format);
        if (ret == 0) {
            cout << "enc configure success" << endl;
        } else {
            cout << "enc configure fail" << endl;
            break;
        }
        ret = videoDec_->Configure(format);
        if (ret == 0) {
            cout << "dec configure success" << endl;
        } else {
            cout << "dec configure fail" << endl;
            break;
        }
        std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
        if (surface == nullptr) {
            cout << "enc GetInputSurface fail" << endl;
            return false;
        } else {
            cout << "enc GetInputSurface success" << endl;
        }
        ret = videoDec_->SetOutputSurface(surface);
        if (ret == 0) {
            cout << "dec SetOutputSurface success" << endl;
        } else {
            cout << "dec SetOutputSurface fail" << endl;
            break;
        }
        ret = videoDec_->Prepare();
        if (ret == 0) {
            cout << "dec Prepare success" << endl;
        } else {
            cout << "dec Prepare fail" << endl;
            break;
        }
        ret = videoEnc_->Prepare();
        if (ret == 0) {
            cout << "enc Prepare success" << endl;
        } else {
            cout << "enc Prepare fail" << endl;
            break;
        }
        ret = videoDec_->Start();
        if (ret == 0) {
            cout << "dec start success" << endl;
        } else {
            cout << "dec start fail" << endl;
            break;
        }
        ret = videoEnc_->Start();
        if (ret == 0) {
            cout << "enc start success" << endl;
        } else {
            cout << "enc start fail" << endl;
            break;
        }
        sleep(3);
        break;
    }
    if (videoEnc_ != nullptr) {
        cout << "videoEnc_ is not nullptr" << endl;
        int32_t ret = videoEnc_->Reset();
        if (ret == 0) {
            cout << "enc reset success" << endl;
        } else {
            cout << "enc reset fail" << endl;
            return false;
        }
        ret = videoEnc_->Release();
        if (ret == 0) {
            cout << "enc release success" << endl;
        } else {
            cout << "enc release fail" << endl;
            return false;
        }
        cout << "exit enc release" << endl;
    }
    if (videoDec_ != nullptr) {
        cout << "videoDec_ is not nullptr" << endl;
        int32_t ret2 = videoDec_->Reset();
        if (ret2 == 0) {
            cout << "dec reset success" << endl;
        } else {
            cout << "dec reset fail" << endl;
            return false;
        }
        ret2 = videoDec_->Release();
        if (ret2 == 0) {
            cout << "dec release success" << endl;
        } else {
            cout << "dec release fail" << endl;
            return false;
        }
    }
    cout << "next return true" << endl;
    return true;
}

int32_t OHOS::Media::WriteDataToFile(const string &path, const uint8_t* data, size_t size)
{
    FILE *file = nullptr;
    file = fopen(path.c_str(), "w+");
    if (file == nullptr) {
        std::cout << "[fuzz] open file fstab.test failed";
        return -1;
    }
    const uint32_t* data_ = reinterpret_cast<const uint32_t *>(data);
    cout << "data_ is " << data_ << endl;
    cout << "*data_ is " << *data_ << endl;

    if (fwrite(data_, 1, size, file) != size) {
        std::cout << "[fuzz] write data failed";
        (void)fclose(file);
        return -1;
    }
    (void)fclose(file);
    return 0;
}


bool OHOS::Media::FuzzVCodecFile(uint8_t* data, size_t size)
{
    auto codecfuzzer = std::make_unique<VCodecFileFuzzer>();
    if (codecfuzzer == nullptr) {
        cout << "codecfuzzer is null" << endl;
        return 0;
    }
    return codecfuzzer->FuzzVideoFile(data, size);
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::FuzzVCodecFile(data, size);
    return 0;
}

