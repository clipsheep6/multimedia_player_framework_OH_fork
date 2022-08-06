/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gtest/gtest.h"
#include "media_errors.h"
#include "avcodec_list_unit_test.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
void AVCodecListUnitTest::SetUpTestCase(void) {}

void AVCodecListUnitTest::TearDownTestCase(void) {}

void AVCodecListUnitTest::SetUp(void)
{
    avCodecList_ = std::make_shared<AVCodecListMock>();
    ASSERT_NE(nullptr, avCodecList_);
    EXPECT_TRUE(avCodecList_->CreateAVCodecList());
}

void AVCodecListUnitTest::TearDown(void)
{
}

/**
 * @tc.name: AVCdecList_FindVideoDecoder_0100
 * @tc.desc: AVCdecList FindVideoDecoder
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AVCodecListUnitTest, AVCdecList_FindVideoDecoder_0100, TestSize.Level0)
{
    std::string codecName;
    Format format;
    (void)format.PutStringValue("codec_mime", "video/avc");
    (void)format.PutIntValue("bitrate", MAX_VIDEO_BITRATE);
    (void)format.PutIntValue("width", DEFAULT_WIDTH);
    (void)format.PutIntValue("height", DEFAULT_HEIGHT);
    (void)format.PutIntValue("pixel_format", NV12);
    (void)format.PutIntValue("frame_rate", MAX_FRAME_RATE);
    codecName = avCodecList_->FindVideoDecoder(format);
    EXPECT_NE("", codecName);
}

/**
 * @tc.name: AVCdecList_FindVideoEncoder_0100
 * @tc.desc: AVCdecList FindVideoEncoder
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AVCodecListUnitTest, AVCdecList_FindVideoEncoder_0100, TestSize.Level0)
{
    std::string codecName;
    Format format;
    (void)format.PutStringValue("codec_mime", "video/mp4v-es");
    (void)format.PutIntValue("bitrate", MAX_VIDEO_BITRATE);
    (void)format.PutIntValue("width", DEFAULT_WIDTH);
    (void)format.PutIntValue("height", DEFAULT_HEIGHT);
    (void)format.PutIntValue("pixel_format", NV21);
    (void)format.PutIntValue("frame_rate", MAX_FRAME_RATE);
    codecName = avCodecList_->FindVideoEncoder(format);
    EXPECT_NE("", codecName);
}

/**
 * @tc.name: AVCdecList_FindAudioDecoder_0100
 * @tc.desc: AVCdecList FindAudioDecoder
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AVCodecListUnitTest, AVCdecList_FindAudioDecoder_0100, TestSize.Level0)
{
    std::string codecName;
    Format format;
    (void)format.PutStringValue("codec_mime", "audio/mpeg");
    (void)format.PutIntValue("bitrate", MAX_AUDIO_BITRATE);
    (void)format.PutIntValue("channel_count", MAX_CHANNEL_COUNT);
    (void)format.PutIntValue("samplerate", DEFAULT_SAMPLERATE);
    codecName = avCodecList_->FindAudioDecoder(format);
    EXPECT_EQ("avdec_mp3", codecName);
}

/**
 * @tc.name: AVCdecList_FindAudioEncoder_0100
 * @tc.desc: AVCdecList FindAudioEncoder
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AVCodecListUnitTest, AVCdecList_FindAudioEncoder_0100, TestSize.Level0)
{
    std::string codecName;
    Format format;
    (void)format.PutStringValue("codec_mime", "audio/mp4a-latm");
    (void)format.PutIntValue("bitrate", MAX_AUDIO_BITRATE);
    (void)format.PutIntValue("channel_count", MAX_CHANNEL_COUNT);
    (void)format.PutIntValue("samplerate", DEFAULT_SAMPLERATE);
    codecName = avCodecList_->FindAudioEncoder(format);
    EXPECT_EQ("avenc_aac", codecName);
}

void AVCodecListUnitTest::CheckVideoCapsArray(const std::vector<std::shared_ptr<VideoCaps>> &videoCapsArray) const
{
    for (auto iter = videoCapsArray.begin(); iter != videoCapsArray.end(); iter++) {
        std::shared_ptr<VideoCaps> pVideoCaps = *iter;
        if (pVideoCaps == nullptr) {
            cout << "pVideoCaps is nullptr" << endl;
            break;
        }
        CheckVideoCaps(pVideoCaps);
    }
}

void AVCodecListUnitTest::CheckVideoCaps(const std::shared_ptr<VideoCaps> &videoCaps) const
{
    std::shared_ptr<AVCodecInfo> videoCodecCaps;
    videoCodecCaps = videoCaps->GetCodecInfo();
    std::string codecNmae = videoCodecCaps->GetName();
    if (codecNmae.compare("avdec_h264") == 0) {
        CheckAVDecH264(videoCaps);
    } else if (codecNmae.compare("avdec_h263") == 0) {
        CheckAVDecH263(videoCaps);
    } else if (codecNmae.compare("avdec_mpeg2video") == 0) {
        CheckAVDecMpeg2Video(videoCaps);
    } else if (codecNmae.compare("avdec_mpeg4") == 0) {
        CheckAVDecMpeg4(videoCaps);
    } else if (codecNmae.compare("avenc_mpeg4") == 0) {
        CheckAVEncMpeg4(videoCaps);
    }
}

void AVCodecListUnitTest::CheckAVDecH264(const std::shared_ptr<VideoCaps> &videoCaps) const
{
    std::shared_ptr<AVCodecInfo> videoCodecCaps = videoCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_VIDEO_DECODER, videoCodecCaps->GetType());
    EXPECT_EQ("video/avc", videoCodecCaps->GetMimeType());
    EXPECT_EQ(0, videoCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, videoCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, videoCodecCaps->IsVendor());
    EXPECT_EQ(1, videoCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_VIDEO_BITRATE, videoCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedWidthAlignment());
    EXPECT_EQ(MIN_WIDTH, videoCaps->GetSupportedWidth().minVal);
    EXPECT_EQ(MAX_WIDTH, videoCaps->GetSupportedWidth().maxVal);
    EXPECT_EQ(MIN_HEIGHT, videoCaps->GetSupportedHeight().minVal);
    EXPECT_EQ(MAX_HEIGHT, videoCaps->GetSupportedHeight().maxVal);
    EXPECT_EQ(1, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_EQ(MAX_FRAME_RATE, videoCaps->GetSupportedFrameRate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(2, videoCaps->GetSupportedFormats().size()); // 2: supported formats count
    EXPECT_EQ(3, videoCaps->GetSupportedProfiles().size()); // 3: supported profile count
    EXPECT_EQ(0, videoCaps->GetSupportedBitrateMode().size());
    EXPECT_EQ(0, videoCaps->GetSupportedLevels().size());
    EXPECT_EQ(false, videoCaps->IsSupportDynamicIframe());
    EXPECT_EQ(true, videoCaps->IsSizeSupported(videoCaps->GetSupportedWidth().minVal,
        videoCaps->GetSupportedHeight().maxVal));
}

void AVCodecListUnitTest::CheckAVDecH263(const std::shared_ptr<VideoCaps> &videoCaps) const
{
    std::shared_ptr<AVCodecInfo> videoCodecCaps = videoCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_VIDEO_DECODER, videoCodecCaps->GetType());
    EXPECT_EQ("video/h263", videoCodecCaps->GetMimeType());
    EXPECT_EQ(0, videoCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, videoCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, videoCodecCaps->IsVendor());
    EXPECT_EQ(1, videoCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_VIDEO_BITRATE, videoCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedWidthAlignment());
    EXPECT_EQ(0, videoCaps->GetSupportedHeightAlignment());
    EXPECT_EQ(MIN_WIDTH, videoCaps->GetSupportedWidth().minVal);
    EXPECT_EQ(MAX_WIDTH, videoCaps->GetSupportedWidth().maxVal);
    EXPECT_EQ(MIN_HEIGHT, videoCaps->GetSupportedHeight().minVal);
    EXPECT_EQ(MAX_HEIGHT, videoCaps->GetSupportedHeight().maxVal);
    EXPECT_EQ(1, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_EQ(MAX_FRAME_RATE, videoCaps->GetSupportedFrameRate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(2, videoCaps->GetSupportedFormats().size()); // 2: supported formats count
    EXPECT_EQ(1, videoCaps->GetSupportedProfiles().size());
    EXPECT_EQ(0, videoCaps->GetSupportedBitrateMode().size());
    EXPECT_EQ(0, videoCaps->GetSupportedLevels().size());
    EXPECT_EQ(false, videoCaps->IsSupportDynamicIframe());
    EXPECT_EQ(false, videoCaps->IsSizeSupported(videoCaps->GetSupportedWidth().minVal - 1,
        videoCaps->GetSupportedHeight().maxVal));
}

void AVCodecListUnitTest::CheckAVDecMpeg2Video(const std::shared_ptr<VideoCaps> &videoCaps) const
{
    std::shared_ptr<AVCodecInfo> videoCodecCaps = videoCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_VIDEO_DECODER, videoCodecCaps->GetType());
    EXPECT_EQ("video/mpeg2", videoCodecCaps->GetMimeType());
    EXPECT_EQ(0, videoCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, videoCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, videoCodecCaps->IsVendor());
    EXPECT_EQ(1, videoCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_VIDEO_BITRATE, videoCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedWidthAlignment());
    EXPECT_EQ(0, videoCaps->GetSupportedHeightAlignment());
    EXPECT_EQ(MIN_WIDTH, videoCaps->GetSupportedWidth().minVal);
    EXPECT_EQ(MAX_WIDTH, videoCaps->GetSupportedWidth().maxVal);
    EXPECT_EQ(MIN_HEIGHT, videoCaps->GetSupportedHeight().minVal);
    EXPECT_EQ(MAX_HEIGHT, videoCaps->GetSupportedHeight().maxVal);
    EXPECT_EQ(1, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_EQ(MAX_FRAME_RATE, videoCaps->GetSupportedFrameRate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(2, videoCaps->GetSupportedFormats().size()); // 2: supported formats count
    EXPECT_EQ(2, videoCaps->GetSupportedProfiles().size()); // 2: supported profile count
    EXPECT_EQ(0, videoCaps->GetSupportedBitrateMode().size());
    EXPECT_EQ(0, videoCaps->GetSupportedLevels().size());
    EXPECT_EQ(false, videoCaps->IsSupportDynamicIframe());
    EXPECT_EQ(0, videoCaps->IsSizeAndRateSupported(videoCaps->GetSupportedWidth().minVal,
        videoCaps->GetSupportedHeight().maxVal, videoCaps->GetSupportedFrameRate().maxVal));
}

void AVCodecListUnitTest::CheckAVDecMpeg4(const std::shared_ptr<VideoCaps> &videoCaps) const
{
    std::shared_ptr<AVCodecInfo> videoCodecCaps = videoCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_VIDEO_DECODER, videoCodecCaps->GetType());
    EXPECT_EQ("video/mp4v-es", videoCodecCaps->GetMimeType());
    EXPECT_EQ(0, videoCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, videoCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, videoCodecCaps->IsVendor());
    EXPECT_EQ(1, videoCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_VIDEO_BITRATE, videoCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedWidthAlignment());
    EXPECT_EQ(0, videoCaps->GetSupportedHeightAlignment());
    EXPECT_EQ(MIN_WIDTH, videoCaps->GetSupportedWidth().minVal);
    EXPECT_EQ(MAX_WIDTH, videoCaps->GetSupportedWidth().maxVal);
    EXPECT_EQ(MIN_HEIGHT, videoCaps->GetSupportedHeight().minVal);
    EXPECT_EQ(MAX_HEIGHT, videoCaps->GetSupportedHeight().maxVal);
    EXPECT_EQ(1, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_EQ(MAX_FRAME_RATE, videoCaps->GetSupportedFrameRate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(2, videoCaps->GetSupportedFormats().size()); // 2: supported formats count
    EXPECT_EQ(2, videoCaps->GetSupportedProfiles().size()); // 2: supported profile count
    EXPECT_EQ(0, videoCaps->GetSupportedBitrateMode().size());
    EXPECT_EQ(0, videoCaps->GetSupportedLevels().size());
    EXPECT_EQ(false, videoCaps->IsSupportDynamicIframe());
    EXPECT_EQ(false, videoCaps->IsSizeAndRateSupported(videoCaps->GetSupportedWidth().minVal,
        videoCaps->GetSupportedHeight().maxVal, videoCaps->GetSupportedFrameRate().maxVal + 1));
}

void AVCodecListUnitTest::CheckAVEncMpeg4(const std::shared_ptr<VideoCaps> &videoCaps) const
{
    std::shared_ptr<AVCodecInfo> videoCodecCaps = videoCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_VIDEO_ENCODER, videoCodecCaps->GetType());
    EXPECT_EQ("video/mp4v-es", videoCodecCaps->GetMimeType());
    EXPECT_EQ(0, videoCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, videoCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, videoCodecCaps->IsVendor());
    EXPECT_EQ(1, videoCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_VIDEO_BITRATE, videoCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedWidthAlignment());
    EXPECT_EQ(0, videoCaps->GetSupportedHeightAlignment());
    EXPECT_EQ(MIN_WIDTH, videoCaps->GetSupportedWidth().minVal);
    EXPECT_EQ(DEFAULT_WIDTH, videoCaps->GetSupportedWidth().maxVal);
    EXPECT_EQ(MIN_HEIGHT, videoCaps->GetSupportedHeight().minVal);
    EXPECT_EQ(DEFAULT_HEIGHT, videoCaps->GetSupportedHeight().maxVal);
    EXPECT_EQ(1, videoCaps->GetSupportedFrameRate().minVal);
    EXPECT_EQ(MAX_FRAME_RATE, videoCaps->GetSupportedFrameRate().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedEncodeQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedQuality().maxVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, videoCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(2, videoCaps->GetSupportedFormats().size()); // 2: supported formats count
    EXPECT_EQ(2, videoCaps->GetSupportedProfiles().size()); // 2: supported profile count
    EXPECT_EQ(2, videoCaps->GetSupportedBitrateMode().size()); // 2: supported bitretemode count
    EXPECT_EQ(0, videoCaps->GetSupportedLevels().size());
    EXPECT_EQ(false, videoCaps->IsSupportDynamicIframe());
}

void AVCodecListUnitTest::CheckAudioCapsArray(const std::vector<std::shared_ptr<AudioCaps>> &audioCapsArray) const
{
    for (auto iter = audioCapsArray.begin(); iter != audioCapsArray.end(); iter++) {
        std::shared_ptr<AudioCaps> pAudioCaps = *iter;
        if (pAudioCaps == nullptr) {
            cout << "pAudioCaps is nullptr" << endl;
            break;
        }
        CheckAudioCaps(pAudioCaps);
    }
}

void AVCodecListUnitTest::CheckAudioCaps(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps;
    audioCodecCaps = audioCaps->GetCodecInfo();
    std::string codecNmae = audioCodecCaps->GetName();
    if (codecNmae.compare("avdec_mp3") == 0) {
        CheckAVDecMP3(audioCaps);
    } else if (codecNmae.compare("avdec_aac") == 0) {
        CheckAVDecAAC(audioCaps);
    } else if (codecNmae.compare("avdec_vorbis") == 0) {
        CheckAVDecVorbis(audioCaps);
    } else if (codecNmae.compare("avdec_flac") == 0) {
        CheckAVDecFlac(audioCaps);
    } else if (codecNmae.compare("avdec_opus") == 0) {
        CheckAVDecOpus(audioCaps);
    } else if (codecNmae.compare("avenc_aac") == 0) {
        CheckAVEncAAC(audioCaps);
    } else if (codecNmae.compare("avenc_opus") == 0) {
        CheckAVEncOpus(audioCaps);
    }
}

void AVCodecListUnitTest::CheckAVDecMP3(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_DECODER, audioCodecCaps->GetType());
    EXPECT_EQ("audio/mpeg", audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_EQ(1, audioCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_AUDIO_BITRATE, audioCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_EQ(MAX_CHANNEL_COUNT, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedFormats().size());
    EXPECT_EQ(12, audioCaps->GetSupportedSampleRates().size()); // 12: supported samplerate count
    EXPECT_EQ(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_EQ(0, audioCaps->GetSupportedLevels().size());
}

void AVCodecListUnitTest::CheckAVDecAAC(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_DECODER, audioCodecCaps->GetType());
    EXPECT_EQ("audio/mp4a-latm", audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_EQ(1, audioCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_AUDIO_BITRATE, audioCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_EQ(MAX_CHANNEL_COUNT, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedFormats().size());
    EXPECT_EQ(12, audioCaps->GetSupportedSampleRates().size()); // 12: supported samplerate count
    EXPECT_EQ(1, audioCaps->GetSupportedProfiles().size());
    EXPECT_EQ(0, audioCaps->GetSupportedLevels().size());
}

void AVCodecListUnitTest::CheckAVDecVorbis(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_DECODER, audioCodecCaps->GetType());
    EXPECT_EQ("audio/vorbis", audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_EQ(1, audioCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_AUDIO_BITRATE, audioCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_EQ(MAX_CHANNEL_COUNT_VORBIS, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedFormats().size());
    EXPECT_EQ(14, audioCaps->GetSupportedSampleRates().size()); // 14: supported samplerate count
    EXPECT_EQ(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_EQ(0, audioCaps->GetSupportedLevels().size());
}

void AVCodecListUnitTest::CheckAVDecFlac(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_DECODER, audioCodecCaps->GetType());
    EXPECT_EQ("audio/flac", audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_EQ(1, audioCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_AUDIO_BITRATE, audioCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_EQ(MAX_CHANNEL_COUNT, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedFormats().size());
    EXPECT_EQ(12, audioCaps->GetSupportedSampleRates().size()); // 12: supported samplerate count
    EXPECT_EQ(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_EQ(0, audioCaps->GetSupportedLevels().size());
}

void AVCodecListUnitTest::CheckAVDecOpus(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_DECODER, audioCodecCaps->GetType());
    EXPECT_EQ("audio/opus", audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_EQ(1, audioCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_AUDIO_BITRATE, audioCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_EQ(MAX_CHANNEL_COUNT, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedFormats().size());
    EXPECT_EQ(1, audioCaps->GetSupportedSampleRates().size());
    EXPECT_EQ(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_EQ(0, audioCaps->GetSupportedLevels().size());
}

void AVCodecListUnitTest::CheckAVEncAAC(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_ENCODER, audioCodecCaps->GetType());
    EXPECT_EQ("audio/mp4a-latm", audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_EQ(1, audioCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_AUDIO_BITRATE, audioCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_EQ(MAX_CHANNEL_COUNT, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedFormats().size());
    EXPECT_EQ(11, audioCaps->GetSupportedSampleRates().size()); // 11: supported samplerate count
    EXPECT_EQ(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_EQ(0, audioCaps->GetSupportedLevels().size());
}

void AVCodecListUnitTest::CheckAVEncOpus(const std::shared_ptr<AudioCaps> &audioCaps) const
{
    std::shared_ptr<AVCodecInfo> audioCodecCaps = audioCaps->GetCodecInfo();
    EXPECT_EQ(AVCODEC_TYPE_AUDIO_ENCODER, audioCodecCaps->GetType());
    EXPECT_EQ("audio/opus", audioCodecCaps->GetMimeType());
    EXPECT_EQ(0, audioCodecCaps->IsHardwareAccelerated());
    EXPECT_EQ(1, audioCodecCaps->IsSoftwareOnly());
    EXPECT_EQ(0, audioCodecCaps->IsVendor());
    EXPECT_EQ(1, audioCaps->GetSupportedBitrate().minVal);
    EXPECT_EQ(MAX_AUDIO_BITRATE, audioCaps->GetSupportedBitrate().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedChannel().minVal);
    EXPECT_EQ(MAX_CHANNEL_COUNT, audioCaps->GetSupportedChannel().maxVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().minVal);
    EXPECT_EQ(0, audioCaps->GetSupportedComplexity().maxVal);
    EXPECT_EQ(1, audioCaps->GetSupportedFormats().size());
    EXPECT_EQ(1, audioCaps->GetSupportedSampleRates().size());
    EXPECT_EQ(0, audioCaps->GetSupportedProfiles().size());
    EXPECT_EQ(0, audioCaps->GetSupportedLevels().size());
}

/**
 * @tc.name: AVCdecList_GetVideoDecoderCaps_0100
 * @tc.desc: AVCdecList GetVideoDecoderCaps
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AVCodecListUnitTest, AVCdecList_GetVideoDecoderCaps_0100, TestSize.Level0)
{
    std::vector<std::shared_ptr<VideoCaps>> videoDecoderArray;
    videoDecoderArray = avCodecList_->GetVideoDecoderCaps();
    CheckVideoCapsArray(videoDecoderArray);
}

/**
 * @tc.name: AVCdecList_GetVideoEncoderCaps_0100
 * @tc.desc: AVCdecList GetVideoEncoderCaps
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AVCodecListUnitTest, AVCdecList_GetVideoEncoderCaps_0100, TestSize.Level0)
{
    std::vector<std::shared_ptr<VideoCaps>> videoEncoderArray;
    videoEncoderArray = avCodecList_->GetVideoEncoderCaps();
    CheckVideoCapsArray(videoEncoderArray);
}

/**
 * @tc.name: AVCdecList_GetAudioDecoderCaps_0100
 * @tc.desc: AVCdecList GetAudioDecoderCaps
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AVCodecListUnitTest, AVCdecList_GetAudioDecoderCaps_0100, TestSize.Level0)
{
    std::vector<std::shared_ptr<AudioCaps>> audioDecoderArray;
    audioDecoderArray = avCodecList_->GetAudioDecoderCaps();
    CheckAudioCapsArray(audioDecoderArray);
}

/**
 * @tc.name: AVCdecList_GetAudioEncoderCaps_0100
 * @tc.desc: AVCdecList GetAudioEncoderCaps
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AVCodecListUnitTest, AVCdecList_GetAudioEncoderCaps_0100, TestSize.Level0)
{
    std::vector<std::shared_ptr<AudioCaps>> audioEncoderArray;
    audioEncoderArray = avCodecList_->GetAudioEncoderCaps();
    CheckAudioCapsArray(audioEncoderArray);
}

/**
 * @tc.name: AVCdecList_GetSupportedFrameRatesFor_0100
 * @tc.desc: AVCdecList GetSupportedFrameRatesFor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AVCodecListUnitTest, AVCdecList_GetSupportedFrameRatesFor_0100, TestSize.Level0)
{
    Range ret;
    ImgSize size = ImgSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    std::vector<std::shared_ptr<VideoCaps>> videoDecoderArray = avCodecList_->GetVideoDecoderCaps();
    for (auto iter = videoDecoderArray.begin(); iter != videoDecoderArray.end(); iter++) {
        std::shared_ptr<VideoCaps> pVideoCaps = *iter;
        ret = (*iter)->GetSupportedFrameRatesFor(size.width, size.height);
        EXPECT_GE(ret.minVal, 1);
        EXPECT_LE(ret.maxVal, MAX_FRAME_RATE);
    }
    std::vector<std::shared_ptr<VideoCaps>> videoEncoderArray = avCodecList_->GetVideoEncoderCaps();
    for (auto iter = videoEncoderArray.begin(); iter != videoEncoderArray.end(); iter++) {
        ret = (*iter)->GetSupportedFrameRatesFor(size.width, size.height);
        EXPECT_GE(ret.minVal, 1);
        EXPECT_LE(ret.maxVal, MAX_FRAME_RATE);
    }
}

/**
 * @tc.name: AVCdecList_GetPreferredFrameRate_0100
 * @tc.desc: AVCdecList GetPreferredFrameRate
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AVCodecListUnitTest, AVCdecList_GetPreferredFrameRate_0100, TestSize.Level0)
{
    Range ret;
    ImgSize size = ImgSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    std::vector<std::shared_ptr<VideoCaps>> videoEncoderArray = avCodecList_->GetVideoEncoderCaps();
    for (auto iter = videoEncoderArray.begin(); iter != videoEncoderArray.end(); iter++) {
        ret = (*iter)->GetPreferredFrameRate(size.width, size.height);
        EXPECT_GE(ret.minVal, 0);
        EXPECT_LE(ret.maxVal, MAX_FRAME_RATE);
    }
    std::vector<std::shared_ptr<VideoCaps>> videoDecoderArray = avCodecList_->GetVideoDecoderCaps();
    for (auto iter = videoDecoderArray.begin(); iter != videoDecoderArray.end(); iter++) {
        ret = (*iter)->GetPreferredFrameRate(size.width, size.height);
        EXPECT_GE(ret.minVal, 1);
        EXPECT_LE(ret.maxVal, MAX_FRAME_RATE);
    }
}
} // namespace Media
} // namespace OHOS
