/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include <stdio.h>
#include <string.h>
#include "securec.h"
#include "audiodec_native_sample.h"
#include "audioenc_native_sample.h"
#include "videodec_native_sample.h"
#include "videoenc_native_sample.h"
#include "native_sample_log.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;


void RunVideoCodecSample(void)
{
    auto vcodec = std::make_unique<VCodecNdkSample>();
    if (vcodec == nullptr) {
        cout << "vcodec is null " << endl;
    }
    vcodec->RunVideoCodec();
    cout << "video dec and enc sample end " << endl;
}

void RunVideoEncSample(void)
{
    auto venc = std::make_unique<VEncNdkSample>();
    if (venc == nullptr) {
        cout << "venc is null " << endl;
    }
    venc->RunVideoEnc();
    cout << "video encoder sample end " << endl;
}

void RunAudioDecSample(void)
{
    auto adec = std::make_unique<ADecNdkSample>();
    if (adec == nullptr) {
        cout << "adec is null " << endl;
    }
    adec->RunAudioDec();
    cout << "audio decoder sample end " << endl;
}

void RunAudioEncSample(void)
{
    auto aenc = std::make_unique<AEncNdkSample>();
    if (aenc == nullptr) {
        cout << "aenc is null " << endl;
    }
    aenc->RunAudioEnc();
    cout << "audio encoder sample end " << endl;
}

void RunAVCodec(string mode)
{
    if (mode == "0") {
        RunVideoCodecSample(); // video dec
    } else if (mode == "1") {
        RunVideoEncSample(); // video dec
    } else if (mode == "2") {
        RunAudioDecSample(); // audio dec
    } else if (mode == "3") {
        RunAudioEncSample(); // audio enc
    }
}

// void RunAVCodec(void)
// {
//     (void)printf("Please select a demo scenario number: \n");
//     (void)printf("0:video dec and enc \n");
//     (void)printf("1:video enc \n");
//     (void)printf("2:audio dec \n");
//     (void)printf("3:audio enc \n");

//     int input = 0;
//     scanf("%d", &input);
//     (void)printf("input: %d\n", input);

//     switch (input) {
//         case 0: // video dec
//             // (void)printf("fail, video dec none \n");
//             RunVideoCodecSample();
//             break;
//         case 1: // video dec
//             RunVideoEncSample();
//             break;
//         case 2: // audio dec
//             RunAudioDecSample();
//             break;
//         case 3: // audio enc
//             RunAudioEncSample();
//             break;
//         default:
//             (void)printf("invalid select\n");
//     }
// }

int main(int argc, char *argv[])
{
    constexpr int minRequiredArgCount = 2;
    string mode;
    if (argc >= minRequiredArgCount && argv[1] != nullptr) {
        mode = argv[1];
    }

    (void)printf("Please select a demo scenario number: \n");
    RunAVCodec(mode);
    return 0;
}

// int main(int argc, char *argv[])
// {
//     (void)printf("Please select a demo scenario number: \n");
//     RunAVCodec();
//     return 0;
// }
