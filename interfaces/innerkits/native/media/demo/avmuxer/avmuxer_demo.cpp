#include "avmuxer_demo.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <securec.h>
#include <unistd.h>
#include "securec.h"
namespace OHOS {
namespace Media {
std::vector<std::string> MDKey = {
    "track_index",
    "track_type",
    "codec_mime",
    "duration",
    "bitrate",
    "max-input-size",
    "width",
    "height",
    "pixel-format",
    "frame-rate",
    "capture-rate",
    "i_frame_interval",
    "req_i_frame"
    "channel-count",
    "sample-rate",
    "track-count",
    "vendor."
};

static const int32_t ES[501] =
    { 646, 5855, 3185, 3797, 3055, 5204, 2620, 6262, 2272, 3702, 4108, 4356, 4975, 4590, 3083, 1930, 1801, 1945,
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
      4248, 4019, 4407, 4217, 2913, 5106 };

// static const int32_t ES[] = {1456, 46417, 1528, 2692, 1375, 12396, 1466, 2862, 1454, 12953, 1363, 2808, 1470, 13142, 1335, 2696, 1353, 10668, 1639, 3026, 1439, 11045, 1464, 7371, 1690, 9560, 1405, 6376, 1401, 12969, 1753, 10126, 1750, 9484, 1734, 7191, 1916, 8163, 1588, 8055, 1982, 8581, 2032, 17236, 3806, 14044, 2166, 18195, 1688, 15321, 1380, 12810, 2618, 7268, 1777, 12185, 1808, 11208, 1744, 10660, 2761, 12832, 2057, 10177, 1977, 15004, 2772, 12074, 1935, 17002, 1821, 13965, 2605, 12570, 1900, 13298, 2644, 7555, 1981, 23224, 1715, 16844, 2415, 9343, 2724, 14956, 1878, 11369, 1727, 9209, 2094, 7510, 22994, 2176, 14958, 1706, 11296, 2265, 11311, 1921, 10401, 1997, 9708, 2704, 9134, 3843, 18433, 2454, 6202, 2405, 18306, 3214, 7879, 2756, 17223, 3119, 8492, 3387, 15780, 2752, 8730, 4531, 12966, 3260, 10081, 3263, 8866, 3901, 16831, 3172, 13221, 2441, 13473, 2792, 12678, 2320, 8903, 3066, 7083, 3354, 9368, 2799, 7061, 3632, 13849, 2728, 8027, 2850, 7697, 2873, 6801, 4028, 10287, 3895, 7503, 3384, 7013, 2708, 7210, 2697, 7755, 3197, 7079, 3813, 9440, 3050, 8462, 2705, 9626, 2125, 9005, 2022, 8957, 2296, 6984, 2157, 8170, 2141, 4956, 2134, 11577, 5002, 6408, 2994, 14425, 7768, 2020, 9396, 2659, 9595, 2395, 8964, 2403, 9241, 7023, 17647, 2254, 7359, 3445, 19055, 2061, 9592, 2509, 9871, 2318, 10901, 1936, 12106, 2853, 9192, 18086, 6518, 2320, 18112, 2009, 15053, 2828, 14695, 2328, 15010, 3569, 17741, 2283, 10444, 2129, 13660, 3321, 16030, 3022, 17118, 2832, 14722, 2678, 14920, 3726, 11159, 7184, 23568, 3463, 13766, 4732, 15831, 3284, 12696, 3110, 18083, 2403, 13145, 2461, 11996, 2821, 9400, 4543, 23055, 2870, 13868, 2753, 30861, 2072, 13289, 2753, 14279, 3735, 18899, 4015, 10475, 5401, 16044, 3477, 13948, 3695, 13013, 23549, 5371, 16996, 2766, 14296, 2491, 14550, 2654, 10621, 2795, 12234, 2379, 30694, 2244, 10415, 2417, 7980, 2862, 14730, 3033, 18750, 2310, 15405, 2754, 15951, 3571, 14040, 3218, 26655, 3979, 15055, 4120, 16333, 5524, 11743, 4292, 13172, 3973, 23554, 4228, 15784, 6749, 11652, 19128, 22798, 3741, 11314, 2489, 9618, 2959, 10249, 5714, 11738, 8242, 12706, 7312, 21907, 5158, 13146, 5234, 21364, 5664, 11697, 4959, 14745, 8649, 11422, 6664, 13635, 4637, 12711, 5793, 14143, 4943, 12080, 3989, 17458, 8979, 9506, 11097, 19059, 5241, 14385, 12392, 18507, 3124, 21861, 1529, 11839, 8085, 15251, 7494, 26194, 10767, 14638, 11127, 21652, 13375, 5806, 13668, 5544, 10638, 4535, 14204, 4637, 12488, 5514, 12476, 3818, 21576, 4740, 15112, 3570, 14084, 3450, 29567, 2482, 13984, 2954, 13364, 2889, 20237, 2792, 14247, 2438, 16337, 1918, 12071, 2032, 13481, 1850, 9608, 2513, 11988, 2235, 16283, 1977, 16503, 2444, 14098, 3949, 21025, 5280, 13029, 3417, 27137, 2182, 12629, 2065, 14450, 2371, 14906, 2089, 16293, 2508, 14677, 2192, 16173, 2625, 16377, 3391, 13867, 3655, 22187, 4159, 14223, 4494, 21920, 5923, 14538, 4914, 21582, 5957, 13547, 6628, 20572, 5330, 12683, 4342, 18740, 8155, 12903, 8926, 23517, 10434, 6051, 13068, 3327, 11512, 3801, 10854, 4359, 8782, 3749, 15484, 6115, 12955, 6065, 13192, 5791, 15118, 6858, 17522, 23942, 6210, 9683, 9227, 17817, 7437, 12502, 9475, 20907, 10496, 14608, 9400, 18645, 9692, 10734, 9872, 19079, 8918, 4821, 16812, 5716, 11477, 3573, 19751, 4120, 12259, 3920, 19999, 4228, 12644, 4321, 19398, 4445, 12384, 4365, 16918, 4300, 13275, 5380, 11042, 4493, 17936, 4866, 11676, 4135, 16451, 3676, 12605, 4400, 12819, 5909, 19577, 4370, 11621, 4209, 17002, 3336, 11059, 3262, 17307, 4142, 12183, 2973, 19660, 3142, 9572, 9145, 3325, 17998, 3549, 11184, 2860, 19377, 4444, 14313, 3847, 13857, 7025, 9021, 14994, 16896, 4457, 12566, 6643, 19584, 4751, 12476, 3145, 19033, 3256, 14301, 3782, 13184, 3379, 23376, 3760, 14123, 3797, 13181, 3861, 18143, 4027, 11853, 3378, 18297, 4298, 14142, 3007, 14745, 3145, 14745, 2952, 13910, 3003, 13814, 3188, 13271, 3141, 13191, 3689, 14175, 3684, 14918, 3713, 13645, 3565, 11361, 2981, 14567, 3058, 8906, 3294, 11345, 3712, 8033, 3166, 13600, 3241, 11667, 3757, 12161, 4305, 12879, 4774, 10380, 3877, 14744, 4632, 8455, 2971, 14320, 3157, 12599, 4443, 13280, 3972, 14526, 8056, 11949, 6289, 19576, 6210, 8667, 6235, 15772, 6987, 4823, 12097, 4434, 10191, 4159, 13917, 4105, 11811, 4088, 11869, 4275, 9594, 4376, 15610, 4301, 10726, 4388, 13850, 4429, 8972, 4737, 13682, 5653, 11101, 5118, 17694, 4552, 9881, 4371, 15564, 4350, 12994, 4449, 13284, 4644, 13575, 4661, 10309, 4888, 16329, 4976, 10707, 4844, 17477, 4160, 11654, 4230, 12783, 4595, 13581, 3924, 10811, 4214, 18870, 4174, 14423, 4811, 14371, 3731, 13954, 4143, 13030, 3838, 13575, 3987, 13391, 3791, 13606, 4770, 13350, 3789, 13304, 3572, 10342, 3102, 16991, 3477, 11236, 3633, 10299, 8410, 21449, 7991, 3979, 17864, 3453, 13098, 3727, 12874, 7933, 21307, 2997, 10002, 3883, 9759, 4220, 17953, 4146, 12266, 2882, 12697, 3511, 8841, 2937, 12557, 2838, 11755, 2967, 10839, 3401, 9888, 4027, 15343, 3108, 12227, 3839, 10053, 4148, 15826, 3711, 14185, 3196, 12681, 3209, 12808, 3672, 10559, 3325, 14804, 3114, 10096, 5171, 17479, 5397, 11922, 3209, 11119, 3227, 11845, 2979, 11289, 2799, 11554, 3282, 12225, 3124, 10022, 3460, 7493, 3470, 12662, 3338, 14218, 3377, 11828, 3216, 10819, 2809, 10197, 3102, 9322, 3723, 10662, 3786, 11413, 5749, 7003, 7568, 7603, 12975, 5033, 8076, 3682, 15331, 3386, 6449, 2900, 12132, 2755, 5735, 2803, 10484, 3273, 6000, 4551, 9437, 3705, 8670, 4894, 10648, 2963, 8346};
// static const char KEY[] = {'C', 'I', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'P', 'I', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'P', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'B', 'P', 'I', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'P', 'P', 'P', 'I', 'B', 'P', 'P', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'I', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'P', 'I', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'P', 'I', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'B', 'B', 'P', 'B', 'P'};

void AVMuxerDemo::ReadTrackInfoAVC() {
    uint32_t i;
    fmt_ctx_ = avformat_alloc_context();
    avformat_open_input(&fmt_ctx_, url_, nullptr,nullptr);
    avformat_find_stream_info(fmt_ctx_, NULL);

    for (i = 0; i < fmt_ctx_->nb_streams; i++) {
        if(fmt_ctx_->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex_ = i;
            std::cout << "videoIndex_ is: " << videoIndex_ << std::endl;
            std::cout << "fmt_ctx_->nb_streams is: " << fmt_ctx_->nb_streams << std::endl;
            width_ = fmt_ctx_->streams[i]->codec->width;
            height_ = fmt_ctx_->streams[i]->codec->height;
            frameRate_ = fmt_ctx_->streams[i]->avg_frame_rate.num/fmt_ctx_->streams[i]->avg_frame_rate.den;
        } else if (fmt_ctx_->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            std::cout << "audioIndex_ is: " << i << std::endl;
            sampleRate_ = fmt_ctx_->streams[i]->codec->sample_rate;
            channels_ = fmt_ctx_->streams[i]->codec->channels;
            audioIndex_ = i;
        }
    }
}

void AVMuxerDemo::WriteTrackSampleAVC() {
    AVPacket pkt;
    memset_s(&pkt, sizeof(pkt), 0, sizeof(pkt));

    for (int i = 0; i <= 1; i++) {
        AVCodecContext *pCodecContext = fmt_ctx_->streams[i]->codec;
        TrackSampleInfo infoCodec;
        infoCodec.timeUs = 0;
        infoCodec.size = pCodecContext->extradata_size;
        infoCodec.offset = 0;
        infoCodec.flags = CODEC_DATA;
        infoCodec.trackIdx = i + 1;
        std::cout << "pCodecContext->extradata_size: " << pCodecContext->extradata_size << std::endl;
        std::shared_ptr<AVMemory> avMem1 = std::make_shared<AVMemory>(pCodecContext->extradata, infoCodec.size);
	    avMem1->SetRange(infoCodec.offset, infoCodec.size);
        std::cout << "avMem1->Capacity() is: " << avMem1->Capacity() << std::endl;
        std::cout << "avMem1->Size() is: " << avMem1->Size() << std::endl;
        avmuxer_->WriteTrackSample(avMem1, infoCodec);
    }

    while (1) {
        av_init_packet(&pkt);
        if (av_read_frame(fmt_ctx_, &pkt) < 0) {
            av_packet_unref(&pkt);
            break;
        }
        TrackSampleInfo info;
        info.timeUs = static_cast<int64_t>(av_rescale_q(pkt.dts, fmt_ctx_->streams[pkt.stream_index]->time_base, AV_TIME_BASE_Q));
        if (info.timeUs < 0) {
            info.timeUs = 0;
        }
        info.size = pkt.size;
        info.offset = 0;
        if (pkt.flags == AV_PKT_FLAG_KEY) {
            info.flags = SYNC_FRAME;
        }
        std::cout << "pkt.dts is: " << info.timeUs << ", pkt.size is: " << pkt.size << std::endl;
        std::shared_ptr<AVMemory> avMem = std::make_shared<AVMemory>(pkt.data, pkt.size);
		avMem->SetRange(info.offset, pkt.size);
        std::cout << "avMem->Capacity() is: " << avMem->Capacity() << std::endl;
        std::cout << "avMem->Size() is: " << avMem->Size() << std::endl;
        std::cout << "avMem->Data() is: " << (int *)(avMem->Data()) << std::endl;
        std::cout << "avMem->Base() is: " << (int *)(avMem->Base()) << std::endl;
        if (pkt.stream_index == videoIndex_) {  
            info.trackIdx = 1;
            std::cout << "info.trackIdx is: " << info.trackIdx << std::endl;
        } else if(pkt.stream_index == audioIndex_) {
            info.trackIdx = 2;
            std::cout << "info.trackIdx is: " << info.trackIdx << std::endl;
        }
        avmuxer_->WriteTrackSample(avMem, info);
        usleep(100000);
    }
    avformat_close_input(&fmt_ctx_);
}

void AVMuxerDemo::ReadTrackInfoByteStream() {
    width_ = 480;
    height_ = 640;
    // width_ = 544;
    // height_ = 960;
    frameRate_ = 30;
}

void AVMuxerDemo::WriteTrackSampleByteStream()
{
    const int32_t *frameLenArray_ = ES;
    const std::string filePath = "/data/media/video.es";
    // const std::string filePath = "/data/media/out_480_270.h264";
    auto testFile = std::make_unique<std::ifstream>();
    testFile->open(filePath, std::ios::in | std::ios::binary);
    bool isFirstStream = true;
    bool isSecondStream = true;
    int64_t timeStamp = 0;
    int i = 0;
    int len = sizeof(ES) / sizeof(int32_t);
    while (i < len) {
        uint8_t *tempBuffer = (uint8_t *)malloc(sizeof(char) * (*frameLenArray_));
        if (tempBuffer == nullptr) {
            std::cout << "no memory" << std::endl;
            break;
        }
        size_t num = testFile->read((char *)tempBuffer, *frameLenArray_).gcount();
        std::cout << "num is: " << num << std::endl;
        std::cout << "tempBuffer is: " << tempBuffer << std::endl;
        std::shared_ptr<AVMemory> avMem = std::make_shared<AVMemory>(tempBuffer, *frameLenArray_);
        // memcpy_s(avMem->Base(), *frameLenArray_, tempBuffer, *frameLenArray_);
        avMem->SetRange(0, *frameLenArray_);
        if (avMem->Data() == nullptr || avMem->Base() == nullptr) {
            std::cout << "no memory" << std::endl;
            break;
        } 
        std::cout << "avMem->Capacity() is: " << avMem->Capacity() << std::endl;
        std::cout << "avMem->Size() is: " << avMem->Size() << std::endl;
        std::cout << "avMem->Data() is: " << (int *)(avMem->Data()) << std::endl;
        std::cout << "avMem->Base() is: " << (int *)(avMem->Base()) << std::endl;
        TrackSampleInfo info;
        info.size = *frameLenArray_;
        info.offset = 0;
        info.trackIdx = 1;
        if (isFirstStream) {
            info.timeUs = 0;
            info.flags = CODEC_DATA;
            isFirstStream = false;
            avmuxer_->WriteTrackSample(avMem, info);
        } else {
            info.timeUs = timeStamp;
            if (isSecondStream) {
                info.flags = SYNC_FRAME;
                isSecondStream = false;
            } else {
                info.flags = PARTIAL_FRAME;
            }
            timeStamp += 33333;
            avmuxer_->WriteTrackSample(avMem, info);
        }
        frameLenArray_++;
        free(tempBuffer);
        i++;
        usleep(200000);
    }
    // while (i < len) {
    //     uint8_t *tempBuffer = (uint8_t *)malloc(sizeof(char) * (*frameLenArray_) + 1);
    //     if (tempBuffer == nullptr) {
    //         std::cout << "no memory" << std::endl;
    //         break;
    //     }
    //     size_t num = testFile->read((char *)tempBuffer, *frameLenArray_).gcount();
    //     std::cout << "num is: " << num << std::endl;
    //     std::cout << "tempBuffer is: " << tempBuffer << std::endl;
    //     std::shared_ptr<AVMemory> avMem = std::make_shared<AVMemory>(tempBuffer, *frameLenArray_);
    //     // memcpy_s(avMem->Base(), *frameLenArray_, tempBuffer, *frameLenArray_);
    //     avMem->SetRange(0, *frameLenArray_);
    //     if (avMem->Data() == nullptr || avMem->Base() == nullptr) {
    //         std::cout << "no memory" << std::endl;
    //         break;
    //     } 
    //     std::cout << "avMem->Capacity() is: " << avMem->Capacity() << std::endl;
    //     std::cout << "avMem->Size() is: " << avMem->Size() << std::endl;
    //     std::cout << "avMem->Data() is: " << avMem->Data() << std::endl;
    //     std::cout << "avMem->Data() is: " << (int*)(avMem->Data()) << std::endl;
    //     std::cout << "avMem->Base() is: " << (int*)(avMem->Base()) << std::endl;
    //     TrackSampleInfo info;
    //     info.size = *frameLenArray_;
    //     info.offset = 0;
    //     info.trackIdx = 1;
    //     if (isFirstStream) {
    //         info.timeUs = 0;
    //         info.flags = CODEC_DATA;
    //         isFirstStream = false;
    //         avmuxer_->WriteTrackSample(avMem, info);
    //     } else {
    //         info.timeUs = timeStamp;
    //         if (KEY[i] == 'I') {
    //             info.flags = SYNC_FRAME;
    //         } else {
    //             info.flags = PARTIAL_FRAME;
    //         }
    //         timeStamp += 33333;
    //         avmuxer_->WriteTrackSample(avMem, info);
    //     }
    //     frameLenArray_++;
    //     free(tempBuffer);
    //     i++;
    //     usleep(200000);
    // }
}

void AVMuxerDemo::AddTrackVideo()
{
    MediaDescription trackDesc;
    trackDesc.PutStringValue(std::string(MD_KEY_CODEC_MIME), "video/x-h264");
    trackDesc.PutIntValue(std::string(MD_KEY_WIDTH), width_);
    trackDesc.PutIntValue(std::string(MD_KEY_HEIGHT), height_);
    trackDesc.PutIntValue(std::string(MD_KEY_FRAME_RATE), frameRate_);
    std::cout << "width is: " << width_ << std::endl;
    std::cout << "height is: " << height_ << std:: endl;
    std::cout << "frameRate is: " << frameRate_ << std::endl;
    int32_t trackId;
    avmuxer_->AddTrack(trackDesc, trackId);
    std::cout << "trackId is: " << trackId << std::endl;
}

void AVMuxerDemo::AddTrackAudio()
{
    MediaDescription trackDesc;
    trackDesc.PutStringValue(std::string(MD_KEY_CODEC_MIME), "audio/mpeg");
    trackDesc.PutIntValue(std::string(MD_KEY_CHANNEL_COUNT), channels_);
    trackDesc.PutIntValue(std::string(MD_KEY_SAMPLE_RATE), sampleRate_);
    std::cout << "channels_ is: " << channels_ << std::endl;
    std::cout << "sampleRate_ is: " << sampleRate_ << std:: endl;
    int32_t trackId;
    avmuxer_->AddTrack(trackDesc, trackId);
    std::cout << "trackId is: " << trackId << std::endl;
}

void AVMuxerDemo::DoNext()
{
    std::string cmd;
    std::string path = "file:///data/media/output.mp4";
    std::string format = "mp4";
    avmuxer_->SetOutput(path, format);
    avmuxer_->SetLocation(0, 0);
    avmuxer_->SetOrientationHint(0);
    std::cout << "Enter AVC or byte-stream:" << std::endl;
    std::cin >> cmd;

    if (cmd == "AVC") {
        ReadTrackInfoAVC();
        AddTrackVideo();
        AddTrackAudio();
        avmuxer_->Start();
        WriteTrackSampleAVC();
    } else if (cmd == "byte-stream") {
        ReadTrackInfoByteStream();
        AddTrackVideo();
        avmuxer_->Start();
        WriteTrackSampleByteStream();
    }

    avmuxer_->Stop();
    avmuxer_->Release();
    // std::string cmd;
    // do {
    //     std::cout << "Enter your step:" << std::endl;
    //     std::cin >> cmd;

    //     if (cmd == "SetOutput") {
    //         std::string path = "/data/media/output.mp4";
    //         std::string format = "mp4";
    //         // std::cout << "Enter path:" << std::endl;
    //         // std::cin >> path;
    //         // std::cout << "Enter format:" << std::endl;
    //         // std::cin >> format;
    //         avmuxer_->SetOutput(path, format);
    //         continue;
    //     } else if (cmd == "SetLocation") {
    //         float latitude = 1;
    //         float longitude = 1;
    //         // std::cout << "Enter latitude:" << std::endl;
    //         // std::cin >> latitude;
    //         // std::cout << "Enter longitude:" << std::endl;
    //         // std::cin >> longitude;
    //         avmuxer_->SetLocation(latitude, longitude);
    //         continue;
    //     } else if (cmd == "SetOrientationHint") {                                                    
    //         int degrees = 180;
    //         // std::cout << "Enter degrees:" << std::endl;
    //         // std::cin >> degrees;
    //         avmuxer_->SetOrientationHint(degrees);
    //         continue;
    //     } else if (cmd == "AddTrack") {
    //         ReadTrackInfo();
    //         MediaDescription trackDesc;
    //         // for (auto s : MDKey) {
    //         //     std:: cout << "Enter " << s << std::endl;
    //         //     if (s == "mime-type") {
    //         //         std::string value;
    //         //         std::cin >> value;
    //         //         trackDesc.PutStringValue(s, value);
    //         //     } else {
    //         //         int32_t value;
    //         //         std::cin >> value;
    //         //         trackDesc.PutIntValue(s, value);
    //         //     }
    //         // }
    //         trackDesc.PutStringValue(std::string(MD_KEY_CODEC_MIME), "video/x-h264");
    //         trackDesc.PutIntValue(std::string(MD_KEY_WIDTH), width_);
    //         trackDesc.PutIntValue(std::string(MD_KEY_HEIGHT), height_);
    //         std::cout << "width is: " << width_ << std::endl;
    //         std::cout << "height is: " << height_ << std:: endl;
    //         int32_t trackId;
    //         avmuxer_->AddTrack(trackDesc, trackId);
    //         std::cout << "trackId is: " << trackId << std::endl;
    //         continue;
    //     } else if (cmd == "Start") {
    //         avmuxer_->Start();
    //         continue;
    //     } else if (cmd == "WriteTrackSample") {
    //         WriteTrackSample();
    //         continue;
    //     } else if (cmd == "Stop") {
    //         avmuxer_->Stop();
    //         continue;
    //     } else if (cmd == "Release") {
    //         avmuxer_->Release();
    //         break;
    //     } else {
    //         std::cout << "Unknow cmd, try again" << std::endl;
    //         continue;
    //     }
    // } while (1);
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