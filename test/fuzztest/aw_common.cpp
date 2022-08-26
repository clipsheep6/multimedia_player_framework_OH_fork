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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "aw_common.h"


using namespace std;
using namespace OHOS;
using namespace Media;
using namespace PlayerTestParam;

namespace OHOS {
namespace Media {
namespace PlayerTestParam {
int32_t WriteDataToFile(const std::string &path, const std::uint8_t *data, std::size_t size)
{
    FILE *file = nullptr;
    file = fopen(path.c_str(), "w+");
    if (file == nullptr) {
        cout << "[fuzz] open file fstab.test failed" << endl;
        return -1;
    }
    if (fwrite(data, 1, size, file) != size) {
        cout << "[fuzz] write data failed" << endl;
        (void)fclose(file);
        return -1;
    }
    (void)fclose(file);
    return 0;
}

int32_t ProduceRandomNumberCrypt(void)
{
    int32_t r = 0;
    int fd = open("/dev/random", O_RDONLY);
    if (fd > 0) {
        read(fd, &r, sizeof(int32_t));
    }
    close(fd);
    return r;
}

uint32_t GetUintVariable(const std::string name)
{
    constexpr uint32_t STUB_STREAM_SIZE = 451;
    constexpr uint32_t FRAME_RATE = 30000;
    constexpr uint32_t CODEC_BUFFER_WIDTH = 1024;
    constexpr uint32_t CODEC_BUFFER_HEIGHT = 25;
    constexpr uint32_t YUV_BUFFER_WIDTH = 1280;
    constexpr uint32_t YUV_BUFFER_HEIGHT = 768;
    constexpr uint32_t STRIDE_ALIGN = 8;
    constexpr uint32_t FRAME_DURATION = 40000000;
    constexpr uint32_t RECORDER_TIME = 5;
    constexpr uint32_t YUV_BUFFER_SIZE = 1474560;
    constexpr uint32_t SEC_TO_NS = 1000000000;
    switch (name)
    {
    case "STUB_STREAM_SIZE":
        return STUB_STREAM_SIZE;
    case "FRAME_RATE":
        return FRAME_RATE;
    case "CODEC_BUFFER_WIDTH":
        return CODEC_BUFFER_WIDTH;
    case "CODEC_BUFFER_HEIGHT":
        return CODEC_BUFFER_HEIGHT;
    case "YUV_BUFFER_WIDTH":
        return YUV_BUFFER_WIDTH;
    case "YUV_BUFFER_HEIGHT":
        return YUV_BUFFER_HEIGHT;
    case "STRIDE_ALIGN":
        return STRIDE_ALIGN;
    case "FRAME_DURATION":
        return FRAME_DURATION;
    case "RECORDER_TIME":
        return RECORDER_TIME;
    case "YUV_BUFFER_SIZE":
        return YUV_BUFFER_SIZE;
    case "SEC_TO_NS":
        return SEC_TO_NS;
    default:
        break;
    }
}

const std::string GetRecordVariable(const std::string name)
{
    const std::string PURE_VIDEO = "1";
    const std::string PURE_AUDIO = "2";
    const std::string AUDIO_VIDEO = "3";
    switch (name)
    {
    case "PURE_VIDEO":
       return PURE_VIDEO;
    case "PURE_AUDIO":
       return PURE_AUDIO;
    case "AUDIO_VIDEO":
       return AUDIO_VIDEO;
    default:
        break;
    }
}

const uint32_t GetHIGH_VIDEO_FRAME_SIZE(void)
{
    constexpr uint32_t STUB_STREAM_SIZE = 451;
    const uint32_t HIGH_VIDEO_FRAME_SIZE[STUB_STREAM_SIZE] = {
        12145, 6247, 9203, 4810, 7751, 3429, 2285, 2906, 4461, 4392, 4620, 4251, 3973, 3652, 4469, 5208, 4661,
        4712, 4139, 3199, 4299, 4429, 4726, 4612, 4428, 3709, 2715, 3006, 4048, 3939, 9648, 2796, 3900, 4119,
        3732, 3721, 3145, 2753, 3526, 3801, 4413, 4175, 4579, 4439, 3619, 3697, 4021, 3178, 3420, 3721, 3726,
        4671, 3823, 4565, 4667, 4040, 4174, 3884, 3398, 2933, 12095, 1126, 1514, 2308, 925, 3799, 2556, 1904,
        3721, 2380, 2152, 4190, 3415, 3248, 5801, 3772, 3070, 4871, 4584, 5653, 4888, 6578, 7284, 6345, 5787,
        4957, 5071, 4985, 4158, 3549, 15231, 2242, 1281, 2843, 1693, 3056, 3372, 4148, 3848, 5967, 3637, 3006,
        3347, 4323, 2136, 1865, 2548, 1899, 3360, 2289, 3669, 2452, 4872, 4113, 3268, 4847, 3861, 3157, 5288,
        4090, 20183, 5006, 2972, 4122, 3377, 3608, 3627, 3387, 3594, 3276, 5284, 4684, 4237, 3983, 3601, 3459,
        2489, 2609, 3443, 3135, 3489, 3464, 3888, 3893, 4359, 4363, 4263, 4480, 3545, 4654, 24011, 2565, 2118,
        4948, 2468, 2889, 2671, 4277, 2402, 3828, 3580, 3123, 4562, 3065, 5313, 2884, 3170, 2653, 3530, 3093,
        3002, 3199, 4546, 3232, 3347, 2991, 4202, 3994, 3740, 3771, 30196, 2683, 4039, 3324, 4274, 3866, 3910,
        3214, 3016, 3079, 4113, 2674, 4028, 3957, 4141, 3585, 5638, 4704, 3764, 3483, 3457, 3365, 3520, 3619,
        3664, 3490, 3979, 2686, 3735, 3601, 29065, 4288, 4371, 3420, 4585, 3705, 4230, 3887, 3310, 2815, 3030,
        2440, 2133, 2255, 2506, 2080, 2833, 2694, 3390, 3140, 3265, 3067, 3000, 3754, 3743, 3662, 3439, 3698,
        3420, 3340, 36130, 2170, 2589, 2361, 3101, 3011, 2942, 2832, 3550, 3614, 3620, 3443, 3363, 3518, 2684,
        2650, 2680, 3285, 3121, 3471, 3343, 3250, 3207, 3269, 3169, 3147, 3247, 3367, 3791, 3039, 43578,
        1813, 2615, 2529, 2501, 2491, 2466, 2436, 2966, 2805, 2777, 3260, 3270, 3416, 2819, 3154, 3273, 3334,
        3280, 3201, 3192, 3429, 3128, 3150, 3073, 3182, 3084, 3238, 3049, 3137, 49379, 2086, 2785, 2795, 2900,
        2768, 2770, 2794, 2780, 2872, 2896, 3538, 3473, 6350, 4433, 4241, 3684, 5019, 4919, 3985, 3135, 3637,
        3819, 3976, 4129, 4270, 2629, 3921, 3068, 3730, 33887, 3123, 4353, 4059, 5657, 5496, 6541, 5528, 5715,
        4582, 4379, 3810, 2871, 2204, 3205, 4453, 4865, 4691, 4724, 3618, 3361, 2881, 2864, 2067, 1682, 1415,
        1464, 908, 588, 1492, 20448, 1248, 2027, 2262, 3769, 4053, 4676, 5594, 5713, 6662, 5951, 5873, 4495,
        3375, 4793, 5842, 5924, 5875, 4928, 4075, 3795, 3342, 2234, 2668, 1435, 1630, 1064, 1842, 216, 796,
        18987, 932, 1994, 2344, 3205, 3743, 4465, 5631, 6009, 7096, 6377, 6694, 5381, 4557, 2976, 3019, 3607,
        5581, 5026, 6058, 6325, 5990, 5147, 4521, 3488, 3509, 2779, 2785, 2902, 2759, 13536, 1871, 1485, 927,
        509, 864, 681, 1821, 2644, 2779, 3309, 3580, 3798, 2824, 2444, 1766, 3737, 2738, 2149, 3066, 4515, 2084,
        8322, 4885, 5643, 3339, 3379, 3299, 2804, 2362, 18116
    };
    return HIGH_VIDEO_FRAME_SIZE;
}
}
}
}