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

#include "avmuxer_demo.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include "securec.h"
#include "media_errors.h"

namespace {
    constexpr uint32_t VIDEO_AUDIO_MODE = 0;
    constexpr uint32_t VIDEO_MODE = 1;
    constexpr uint32_t AUDIO_MODE = 2;
    constexpr uint32_t MULT_AUDIO_MODE = 3;
    constexpr uint32_t SLEEP_UTIME = 50000;
    constexpr uint32_t H264_WIDTH = 480;
    constexpr uint32_t H264_HEIGHT = 640;
    constexpr uint32_t H264_FRAME_RATE = 30;
    constexpr uint32_t MPEG4_WIDTH = 720;
    constexpr uint32_t MPEG4_HEIGHT = 480;
    constexpr uint32_t MPEG4_FRAME_RATE = 30;
    constexpr uint32_t AAC_SAMPLE_RATE = 44100;
    constexpr uint32_t AAC_CHANNEL = 2;
    constexpr uint32_t MP3_SAMPLE_RATE = 48000;
    constexpr uint32_t MP3_CHANNEL = 2;
    constexpr float LATITUDE = 30.1111;
    constexpr float LONGITUDE = 150.2222;
    constexpr int32_t ROTATION = 90;
    constexpr uint32_t H264_FRAME_DURATION = 33333;
    constexpr uint32_t MPEG4_FRAME_DURATION = 33333;
    constexpr uint32_t AAC_FRAME_DURATION = 23220;
    constexpr uint32_t MP3_FRAME_DURATION = 23220;
    
    const int32_t H264_FRAME_SIZE[] = {
        646, 5855, 3185, 3797, 3055, 5204, 2620, 6262, 2272, 3702, 4108, 4356, 4975, 4590, 3083, 1930, 1801, 1945,
        3475, 4028, 1415, 1930, 2802, 2176, 1727, 2287, 2274, 2033, 2432, 4447, 4130, 5229, 5792, 4217, 5804,
        6586, 7506, 5128, 5549, 6685, 5248, 4819, 5385, 4818, 5239, 4148, 6980, 5124, 4255, 5666, 4756, 4975,
        3840, 4913, 3649, 4002, 4926, 4284, 5329, 4305, 3750, 4770, 4090, 4767, 3995, 5039, 3820, 4566, 5556,
        4029, 3755, 5059, 3888, 3572, 4680, 4662, 4259, 3869, 4306, 3519, 3160, 4400, 4426, 4370, 3489, 4907,
        4102, 3723, 4420, 4347, 4117, 4578, 4470, 4579, 4128, 4157, 4226, 4742, 3616, 4476, 4084, 4623, 3736,
        4207, 3644, 4349, 4948, 4009, 3583, 4658, 3974, 5441, 4049, 3786, 4093, 3375, 4207, 3787, 4365, 2905,
        4371, 4132, 3633, 3652, 2977, 4387, 3368, 3887, 3464, 4198, 4690, 4467, 2931, 3573, 4652, 3901, 4403,
        3120, 3494, 4666, 3898, 3607, 3272, 4070, 3151, 3237, 3936, 3962, 3637, 3716, 3735, 4371, 3141, 3322,
        4401, 3579, 4006, 2720, 3526, 4796, 3737, 3824, 3257, 4310, 2992, 3537, 3209, 3453, 3819, 3212, 4384,
        3571, 3682, 3344, 3017, 3960, 2737, 1970, 2433, 1442, 1560, 4710, 1070, 877, 833, 838, 776, 735, 1184,
        1172, 699, 723, 2828, 4257, 4329, 3567, 5365, 4213, 3612, 4833, 3388, 3553, 3535, 4937, 4057, 3990,
        5047, 4197, 4656, 3219, 3661, 3666, 3908, 4385, 4350, 3636, 4038, 5213, 3677, 3789, 4221, 4137, 4440,
        3447, 3836, 3912, 4806, 3100, 2963, 5204, 2394, 2391, 1772, 1586, 1598, 2558, 2663, 4537, 3530, 4045,
        4641, 5723, 3688, 4231, 3420, 3462, 3828, 4764, 3944, 4499, 4375, 4597, 4305, 3872, 3969, 2805, 4398,
        3480, 4105, 3890, 3761, 3652, 4356, 2771, 3972, 2930, 3456, 3236, 4648, 3627, 2689, 3827, 2254, 3492,
        2988, 4408, 3007, 4611, 3018, 4783, 2556, 3263, 4536, 4159, 3818, 5093, 3539, 4336, 3400, 3871, 4019,
        4619, 5520, 3781, 4026, 4864, 3340, 4153, 4641, 4292, 4071, 4144, 5109, 3695, 4512, 3882, 3943, 4152,
        4133, 3862, 4717, 3431, 4984, 4164, 4359, 3401, 3727, 4256, 3563, 4694, 3225, 3984, 2432, 3790, 2827,
        3595, 4124, 3854, 2890, 3477, 3989, 3251, 3714, 3345, 4742, 1967, 3931, 1985, 1737, 1854, 2192, 2370,
        2083, 3265, 3312, 3071, 4255, 3994, 4563, 4650, 4885, 3868, 4698, 3103, 3682, 4197, 5532, 3963, 4756,
        4067, 3917, 3667, 3812, 4793, 3260, 3763, 4670, 3184, 2930, 3558, 3245, 4120, 4700, 3671, 4442, 3406,
        4862, 4331, 5064, 4058, 4075, 3160, 3930, 5187, 3816, 3795, 3085, 3564, 3856, 3948, 4474, 3511, 4108,
        4789, 2944, 3323, 2162, 2657, 2219, 1653, 2824, 2716, 3523, 2760, 3328, 3042, 3828, 3759, 3950, 3830,
        3336, 4457, 3193, 3706, 4314, 3937, 3422, 4067, 5328, 3693, 4567, 3444, 4317, 4929, 3838, 4129, 2975,
        4227, 4639, 4348, 2935, 3999, 4745, 3919, 3694, 2602, 4538, 4637, 4250, 3716, 3513, 3856, 4916, 4460,
        4263, 4153, 4299, 3577, 5527, 2486, 3332, 4133, 4145, 3369, 3576, 3940, 4304, 3179, 5266, 3536, 3622,
        2684, 3449, 3621, 4363, 4216, 4913, 5026, 3336, 3057, 2782, 3716, 3036, 4438, 3904, 4823, 1761, 2045,
        1446, 3210, 1625, 2400, 3489, 4719, 3954, 3756, 4940, 2371, 4516, 3739, 3572, 2644, 3837, 4915, 2251,
        4248, 4019, 4407, 4217, 2913, 5106
    };
    const int32_t AAC_FRAME_SIZE[] = {
        361, 368, 22, 20, 20, 20, 20, 20, 18, 198, 513, 499, 534, 522, 541, 608, 596, 613, 631, 543,
        563, 505, 411, 375, 402, 361, 396, 405, 372, 402, 382, 371, 363, 366, 401, 390, 519, 325, 367,
        365, 389, 358, 389, 413, 327, 493, 378, 360, 359, 387, 356, 391, 362, 360, 393, 408, 384, 348,
        361, 437, 494, 381, 397, 329, 365, 376, 354, 376, 347, 541, 366, 377, 370, 365, 368, 366, 358,
        366, 401, 395, 393, 385, 348, 365, 386, 375, 381, 371, 549, 335, 376, 362, 390, 391, 350, 380,
        368, 360, 387, 408, 392, 383, 390, 418, 353, 383, 375, 410, 355, 375, 437, 413, 393, 427, 397,
        397, 363, 418, 393, 412, 373, 433, 323, 381, 396, 391, 372, 400, 397, 294, 280, 391, 405, 378,
        410, 505, 411, 385, 380, 377, 345, 393, 375, 377, 378, 351, 401, 377, 404, 368, 370, 402, 386,
        398, 393, 392, 386, 403, 360, 393, 373, 386, 362, 344, 416, 355, 380, 353, 398, 422, 386, 365,
        335, 340, 383, 371, 386, 375, 399, 398, 352, 387, 380, 390, 391, 390, 385, 404, 384, 407, 365,
        348, 388, 388, 385, 392, 383, 462, 463, 466, 392, 351, 358, 385, 358, 389, 378, 359, 349, 424,
        335, 392, 347, 348, 379, 302, 295, 357, 458, 466, 460, 370, 390, 402, 405, 411, 378, 383, 351,
        413, 420, 362, 343, 369, 367, 378, 355, 385, 410, 420, 375, 398, 394, 384, 376, 400, 395, 389,
        395, 363, 422, 364, 365, 380, 395, 364, 395, 350, 319, 308, 374, 560, 503, 500, 438, 427, 445,
        416, 366, 424, 331, 354, 376, 381, 387, 369, 382, 343, 366, 442, 419, 348, 362, 354, 405, 419,
        332, 376, 388, 405, 365, 428, 379, 384, 387, 403, 385, 344, 366, 381, 366, 371, 286, 328, 470,
        413, 404, 409, 406, 376, 370, 380, 393, 345, 400, 386, 397, 376, 407, 364, 362, 351, 377, 345,
        375, 413, 353, 419, 382, 379, 383, 444, 392, 368, 374, 376, 349, 413, 405, 374, 355, 412, 385,
        356, 277, 361, 461, 398, 431, 381, 405, 389, 374, 392, 377, 407, 377, 389, 406, 391, 378, 420,
        388, 372, 372, 350, 373, 446, 399, 354, 368, 350, 373, 418, 390, 366, 367, 351, 414, 413, 362,
        373, 364, 312, 400, 391, 371, 384, 478, 391, 400, 344, 360, 383, 349, 370, 393, 369, 364, 366,
        401, 377, 360, 392, 398, 388, 358, 374, 386, 395, 374, 419, 376, 393, 376, 348, 416, 381, 363,
        376, 381, 369, 378, 416, 366, 379, 363, 430, 368, 358, 470, 296, 358
    };
    const int32_t MPEG4_FRAME_SIZE[] = {
        40288,  32946,  8929,   2315,   821,    1240,   647,    578,    724,    595,    627,    647,
        13350,  714,    425,    441,    509,    453,    501,    607,    589,    483,    521,    616,
        8819,   643,    455,    612,    558,    534,    596,    527,    552,    503,    587,    637,
        7794,   905,    718,    668,    606,    619,    697,    851,    799,    763,    735,    747,
        7682,   1026,   700,    758,    899,    824,    795,    762,    770,    849,    797,    959,
        9310,   755,    685,    693,    714,    783,    801,    810,    894,    834,    833,    814,
        8007,   976,    808,    949,    1023,   905,    855,    854,    909,    913,    1011,   1093,
        7931,   1160,   942,    978,    934,    1063,   972,    1057,   987,    987,    1019,   1019,
        7805,   1250,   1074,   1019,   1003,   1086,   1007,   1008,   1069,   1151,   1009,   1018,
        7875,   1221,   1009,   1017,   1095,   1108,   1049,   1011,   1119,   1089,   1023,   1011,
        9214,   976,    935,    1124,   1157,   1094,   1135,   1080,   1150,   1084,   1092,   1246,
        7798,   1340,   1050,   995,    1080,   987,    1038,   1192,   1183,   1007,   973,    1024,
        7633,   1210,   875,    999,    1010,   1049,   1058,   879,    989,    826,    915,    991,
        7436,   1256,   1006,   845,    986,    832,    905,    930,    918,    925,    837,    788,
        7289,   1126,   784,    843,    876,    837,    809,    727,    847,    705,    767,    834,
        8302,   712,    586,    651,    685,    617,    693,    686,    771,    694,    772,    587,
        7294,   786,    572,    632,    708,    719,    638,    564,    624,    607,    611,    748,
        7161,   996,    715,    547,    665,    557,    685,    494,    604,    549,    526,    547,
        7093,   906,    722,    475,    598,    525,    631,    511,    548,    564,    577,    504,
        7034,   940,    636,    544,    598,    581,    615,    555,    538,    606,    599,    605,
        7974,   570,    461,    445,    540,    546,    582,    535,    602,    500,    821,    503,
        7189,   778,    522,    560,    554,    568,    524,    530,    586,    607,    543,    622,
        7032,   909,    533,    611,    559,    575,    607,    562,    559,    592,    553,    598,
        6927,   924,    612,    576,    583,    575,    553,    620,    649,    653,    542,    571,
        6871,   983,    492,    548,    593,    613,    477,    555,    620,    570,    521,    615,
        7837,   583,    440,    510,    550,    567,    502,    638,    624,    561,    561,    627,
        7035,   858,    507,    579,    550,    503,    470,    573,    560,    546,    482,    667,
        6896,   864,    460,    598,    514,    558,    539,    618,    673,    579,    517,    591,
        6872,   964,    585,    622,    618,    610,    504,    652,    614,    661,    545,    709,
        6946,   894,    505,    652,    596,    693,    526,    671,    602,    623,    537,    686,
        8042,   687,    448,    526,    572,    574,    557,    660,    704,    643,    567,    704,
        6972,   874,    548,    652,    579,    691,    568,    638,    721,    600,    670,    698,
        6999,   922,    573,    604,    639,    681,    672,    622,    662,    599,    627,    640,
        7046,   951,    678,    605,    676,    615,    642,    508,    680,    753,    613,    650,
        7058,   987,    633,    632,    645,    687,    622,    741,    648,    767,    638,    703,
        8547,   779,    548,    674,    659,    812,    697,    776,    733,    825,    715,    723,
        7344,   990,    625,    707,    738,    770,    693,    732,    721,    817,    724,    818,
        7327,   1114,   680,    726,    740,    793,    821,    771,    731,    818,    707,    794,
        7356,   1085,   748,    748,    731,    777,    699,    756,    743,    837,    777,    839,
        7380,   1120,   740,    748,    786,    909,    818,    849,    819,    953,    744,    766,
        9423,   829,    668,    771,    810,    915,    845,    894,    892,    983,    882,    872,
        7751,   1131,   804,    830,    820,    887,    800,    775,    1082,   845,    874,    1034,
        7872,   1115,   871,    884,    917,    856,    1010,   915,    837,    887,    993,    913,
        7986,   1123,   980,    866,    928,    869,    907,    880,    842,    902,    910,    857,
        7987,   1066,   877,    879,    904,    898,    994,    909,    893,    882,    972,    953,
        10693,  913,    884,    906,    963,    977,    1103,   1097,   1114,   1047,   1124,   1056,
        8313,   1183,   1076,   957,    948,    977,    1079,   989,    1000,   1044,   1129,   1090,
        8338,   1206,   1013,   1003,   1043,   1091,   1153,   1012,   989,    970,    1025,   1004,
        8281,   1376,   1092,   1012,   977,    971,    1020,   1004,   976,    993,    1010,   1035,
        8213,   1241,   957,    967,    970,    989,    1017,   1001,   969,    933,    937,    942,
        10633,  941,
    };
    const int32_t MP3_FRAME_SIZE[] = {
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192
    };

    struct StreamInfo {
        uint32_t frameDuration_;
        uint32_t frameNum_;
        const int32_t *framePointer_;
        std::string path_;
    };

    std::map<std::string, StreamInfo> CODEC_PARAMETER = {
        {"h264", {H264_FRAME_DURATION, sizeof(H264_FRAME_SIZE) / sizeof(int32_t),
            H264_FRAME_SIZE, "/data/media/test.h264"}},
        {"mpeg4", {MPEG4_FRAME_DURATION, sizeof(MPEG4_FRAME_SIZE) / sizeof(int32_t),
            MPEG4_FRAME_SIZE, "/data/media/test.mpeg4"}},
        {"aac", {AAC_FRAME_DURATION, sizeof(AAC_FRAME_SIZE) / sizeof(int32_t),
            AAC_FRAME_SIZE, "/data/media/test.aac"}},
        {"mp3", {MP3_FRAME_DURATION, sizeof(MP3_FRAME_SIZE) / sizeof(int32_t),
            MP3_FRAME_SIZE, "/data/media/test.mp3"}}
    };
}
namespace OHOS {
namespace Media {
bool AVMuxerDemo::PushBuffer(std::shared_ptr<std::ifstream> File, const int32_t frameSize,
    int32_t i, int32_t trackId, int64_t stamp)
{
    if (frameSize == 0) {
        std::cout << "Frame size error" << std::endl;
        return false;
    }
    uint8_t *buffer = (uint8_t *)malloc(sizeof(char) * (frameSize));
    if (buffer == nullptr) {
        std::cout << "no memory" << std::endl;
        return false;
    }
    (void)File->read((char *)buffer, frameSize);
    std::shared_ptr<AVContainerMemory> aVMem = std::make_shared<AVContainerMemory>(buffer, frameSize);
    aVMem->SetRange(0, frameSize);
    TrackSampleInfo info;
    info.size = frameSize;
    info.trackIdx = trackId;

    if (i == 0) {
        info.timeUs = 0;
        info.flags = AVCODEC_BUFFER_FLAG_CODEC_DATA;
    } else if ((i == 1 && trackId == videoTrackId_) || trackId == audioTrackId_) {
        info.timeUs = stamp;
        info.flags = AVCODEC_BUFFER_FLAG_SYNC_FRAME;
    } else {
        info.timeUs = stamp;
        info.flags = AVCODEC_BUFFER_FLAG_PARTIAL_FRAME;
    }

    if (avmuxer_->WriteTrackSample(aVMem, info) != MSERR_OK) {
        std::cout << "WriteTrackSample failed" << std::endl;
        free(buffer);
        return false;
    };
    free(buffer);
    usleep(SLEEP_UTIME);

    return true;
}

std::shared_ptr<std::ifstream> OpenFile(const std::string &filePath)
{
    auto file = std::make_unique<std::ifstream>();
    file->open(filePath, std::ios::in | std::ios::binary);
    if (!file->good()) {
      std::cout << "file: " << filePath << " is not exist." << std::endl;
      return nullptr;
    }
    if (!file->is_open()) {
      std::cout << "file: " << filePath << " open failed." << std::endl;
      return nullptr;
    }

    return file;
}

void AVMuxerDemo::WriteTrackSample()
{
    double videoStamp = 0;
    double audioStamp = 0;
    double audioStamp_1 = 0;
    int32_t i = 0;
    int32_t videoLen = videoFile_ == nullptr ? INT32_MAX : videoFrameNum_;
    int32_t audioLen = audioFile_ == nullptr ? INT32_MAX : audioFrameNum_;
    int32_t audioLen1 = audioFile_1_ == nullptr ? INT32_MAX : audioFrameNum_;
    while (i < videoLen && i < audioLen && i < audioLen1) {
        if (videoFile_ != nullptr) {
            if (!PushBuffer(videoFile_, *videoFrameArray_, i, videoTrackId_, videoStamp)) {
                break;
            }
            videoFrameArray_++;
            videoStamp += videoTimeDuration_;
        }
        if (audioFile_ != nullptr) {
            if (!PushBuffer(audioFile_, *audioFrameArray_, i, audioTrackId_, audioStamp)) {
                break;
            }
            audioFrameArray_++;
            audioStamp += audioTimeDuration_;
        }
        if (audioFile_1_ != nullptr) {
            if (!PushBuffer(audioFile_1_, *audioFrameArray_1_, i, audioTrackId_1_, audioStamp_1)) {
                break;
            }
            audioFrameArray_1_++;
            audioStamp_1 += audioTimeDuration_1_;
        }
        i++;
        std::cout << videoStamp << std::endl;
        std::cout << audioStamp << std::endl;
        std::cout << audioStamp_1 << std::endl;
    }
}

void AVMuxerDemo::SetParameter(const std::string &type, int32_t trackNum)
{
    if (type == "h264" || type == "mpeg4") {
        videoTimeDuration_ = CODEC_PARAMETER[type].frameDuration_;
        videoFrameNum_ = CODEC_PARAMETER[type].frameNum_;
        videoFrameArray_ = CODEC_PARAMETER[type].framePointer_;
        videoFile_ = OpenFile(CODEC_PARAMETER[type].path_);
    } else {
        if (trackNum == 0) {
            audioTimeDuration_ = CODEC_PARAMETER[type].frameDuration_;
            audioFrameNum_ = CODEC_PARAMETER[type].frameNum_;
            audioFrameArray_ = CODEC_PARAMETER[type].framePointer_;
            audioFile_ = OpenFile(CODEC_PARAMETER[type].path_);
        } else {
            audioTimeDuration_1_ = CODEC_PARAMETER[type].frameDuration_;
            audioFrameNum_1_ = CODEC_PARAMETER[type].frameNum_;
            audioFrameArray_1_ = CODEC_PARAMETER[type].framePointer_;
            audioFile_1_ = OpenFile(CODEC_PARAMETER[type].path_);
        }
    }
}

bool AVMuxerDemo::AddTrackVideo(std::string &videoType)
{
    MediaDescription trackDesc;
    if (videoType == "h264") {
        trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, "video/avc");
        trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, H264_WIDTH);
        trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, H264_HEIGHT);
        trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, H264_FRAME_RATE);
        SetParameter("h264");
    } else if (videoType == "mpeg4") {
        trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, "video/mp4v-es");
        trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, MPEG4_WIDTH);
        trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, MPEG4_HEIGHT);
        trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, MPEG4_FRAME_RATE);
        SetParameter("mpeg4");
    } else {
        std::cout << "Failed to check video type" << std::endl;
        return false;
    }
    if (avmuxer_->AddTrack(trackDesc, videoTrackId_) != MSERR_OK) {
        return false;
    }
    std::cout << "trackId is: " << videoTrackId_ << std::endl;

    return true;
}

bool AVMuxerDemo::AddTrackAudio(std::string &audioType, int32_t trackNum)
{
    MediaDescription trackDesc;
    int ret = MSERR_OK;
    if (audioType == "aac") {
        trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, "audio/mp4a-latm");
        trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, AAC_SAMPLE_RATE);
        trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, AAC_CHANNEL);
        SetParameter("aac", trackNum);
    } else if (audioType == "mp3") {
        trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, "audio/mpeg");
        trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, MP3_SAMPLE_RATE);
        trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, MP3_CHANNEL);
        SetParameter("mp3", trackNum);
    } else {
        std::cout << "Failed to check audio type" << std::endl;
        return false;
    }

    if (trackNum == 0) {
        ret = avmuxer_->AddTrack(trackDesc, audioTrackId_);
    } else {
        ret = avmuxer_->AddTrack(trackDesc, audioTrackId_1_);
    }

    if (ret != MSERR_OK) {
        std::cout << "Add Audio track: " << trackNum << "failed" << std::endl;
        return false;
    }

    if (trackNum == 0) {
        std::cout << "trackNum:0 trackId is: " << audioTrackId_ << std::endl;
    } else {
        std::cout << "trackNum:1 trackId is: " << audioTrackId_1_ << std::endl;
    }

    return true;
}

void AVMuxerDemo::SetMode(int32_t mode)
{
    switch (mode) {
        case VIDEO_AUDIO_MODE:
            std::cout << "Please enter video type, note: only support h264 and mpeg4" << std::endl;
            std::cin >> videoType_;
            std::cout << "Please enter audio type, note: only support aac and mp3" << std::endl;
            std::cin >> audioType_;
            format_ = "mp4";
            break;
        case VIDEO_MODE:
            std::cout << "Please enter video type, note: only support h264 and mpeg4" << std::endl;
            std::cin >> videoType_;
            format_ = "mp4";
            break;
        case AUDIO_MODE:
            std::cout << "Please enter audio type, note: only support aac" << std::endl;
            std::cin >> audioType_;
            format_ = "m4a";
            break;
        case MULT_AUDIO_MODE:
            std::cout << "Please enter video type, note: only support h264 and mpeg4" << std::endl;
            std::cin >> videoType_;
            std::cout << "Please enter audio type, note: only support aac and mp3" << std::endl;
            std::cin >> audioType_;
            std::cout << "Please enter audio type, note: only support aac and mp3" << std::endl;
            std::cin >> audioType_1_;
            format_ = "mp4";
            isMultAudioTrack = true;
            break;
        default:
            std::cout << "Failed to check mode" << std::endl;
    }
}

void AVMuxerDemo::DoNext()
{
    std::cout << "Please enter mode, 0: video + audio, 1: video, 2: audio, 3: multiple audio track video" << std::endl;
    int32_t mode;
    std::cin >> mode;
    SetMode(mode);

    path_ = "/data/media/" + videoType_ + audioType_ + audioType_1_ + "." + format_;
    std::cout << "filePath is: " << path_ << std::endl;
    int32_t fd = open(path_.c_str(), O_CREAT | O_WRONLY, 0777);
    std::cout << "fd is: " << fd << std::endl;
    if (fd < 0) {
        std::cout << "Open file failed! filePath is: " << path_ << std::endl;
        return;
    }
    if (avmuxer_->SetOutput(fd, format_) != MSERR_OK ||
        avmuxer_->SetLocation(LATITUDE, LONGITUDE) != MSERR_OK ||
        avmuxer_->SetRotation(ROTATION) != MSERR_OK) {
        (void)::close(fd);
        std::cout << "SetOutput failed!" << std::endl;
        return;
    }

    if ((mode == VIDEO_AUDIO_MODE && (AddTrackVideo(videoType_) == false ||
        AddTrackAudio(audioType_) == false)) ||
        (mode == VIDEO_MODE && (AddTrackVideo(videoType_) == false)) ||
        (mode == AUDIO_MODE && (AddTrackAudio(audioType_) == false)) ||
        ((mode == MULT_AUDIO_MODE && (AddTrackVideo(videoType_) == false ||
         AddTrackAudio(audioType_, 0) == false || AddTrackAudio(audioType_1_, 1) == false)))) {
        (void)::close(fd);
        std::cout << "AddTrackVideo failed!" << std::endl;
        return;
    }

    if (avmuxer_->Start() != MSERR_OK) {
        (void)::close(fd);
        std::cout << "Start failed!" << std::endl;
        return;
    }
    WriteTrackSample();

    if (avmuxer_->Stop() != MSERR_OK) {
        (void)::close(fd);
        std::cout << "Stop failed!" << std::endl;
        return;
    }
    avmuxer_->Release();


    if (audioFile_) {
        audioFile_->close();
        audioFile_ = nullptr;
    }


    if (videoFile_) {
        videoFile_->close();
        videoFile_ = nullptr;
    }

    if (audioFile_1_) {
        audioFile_1_->close();
        audioFile_1_ = nullptr;
    }

    (void)::close(fd);
}

void AVMuxerDemo::RunCase()
{
    avmuxer_ = OHOS::Media::AVMuxerFactory::CreateAVMuxer();
    if (avmuxer_ == nullptr) {
        std::cout << "avmuxer_ is null" << std::endl;
        return;
    }
    DoNext();
}
}  // namespace Media
}  // namespace OHOS