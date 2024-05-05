/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <memory>
#include <iostream>
#include "gtest/gtest.h"
#include "media_dfx.h"
#include "common/media_core.h"
#include "meta/meta.h"

using namespace testing::ext;
using namespace OHOS::Media;

namespace {
    constexpr uint32_t TEST_UID_ID_1 = 1;
    constexpr int32_t ERROR_CODE = 5;
}

namespace OHOS {
namespace Media {
class MediaDfxTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp(void) {};
    void TearDown(void) {};
};

HWTEST_F(MediaDfxTest, CREATE_MEDIA_INFO, TestSize.Level1)
{
    uint64_t instanceId = 1;
    int32_t ret = CreateMediaInfo(CallType::AVPLAYER, TEST_UID_ID_1, instanceId);
    ASSERT_EQ(ret, MSERR_OK);
    
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::SCREEN_CAPTURE_ERR_MSG, "SCREEN_CAPTURE_ERR_MSG");
    meta->SetData(Tag::SCREEN_CAPTURE_ERR_CODE, ERROR_CODE);
    ret = AppendMediaInfo(meta, instanceId);
    ASSERT_EQ(ret, MSERR_OK);

    ret = ReportMediaInfo(instanceId);
    ASSERT_EQ(ret, MSERR_OK);
}

HWTEST_F(MediaDfxTest, CREATE_MEDIA_INFO_1, TestSize.Level1)
{
    uint64_t instanceId = 1;
    int32_t ret = CreateMediaInfo(CallType::AVPLAYER, TEST_UID_ID_1, instanceId);
    ASSERT_EQ(ret, MSERR_OK);
    
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::SCREEN_CAPTURE_ERR_MSG, "SCREEN_CAPTURE_ERR_MSG");
    meta->SetData(Tag::SCREEN_CAPTURE_ERR_CODE, ERROR_CODE);
    ret = AppendMediaInfo(meta, instanceId);
    ASSERT_EQ(ret, MSERR_OK);

    std::shared_ptr<Meta> meta1 = std::make_shared<Meta>();
    ret = AppendMediaInfo(meta1, instanceId);
    ASSERT_NE(ret, MSERR_OK);
    ASSERT_EQ(ret, MSERR_INVALID_OPERATION);

    ret = ReportMediaInfo(instanceId);
    ASSERT_EQ(ret, MSERR_OK);
}

HWTEST_F(MediaDfxTest, CREATE_MEDIA_INFO_2, TestSize.Level1)
{
    uint64_t instanceId = 1;
    int32_t ret = CreateMediaInfo(CallType::AVPLAYER, TEST_UID_ID_1, instanceId);
    ASSERT_EQ(ret, MSERR_OK);
    
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::SCREEN_CAPTURE_ERR_MSG, "SCREEN_CAPTURE_ERR_MSG");
    meta->SetData(Tag::SCREEN_CAPTURE_ERR_CODE, ERROR_CODE);
    ret = AppendMediaInfo(meta, instanceId);
    ASSERT_EQ(ret, MSERR_OK);

    ret = ReportMediaInfo(instanceId);
    ASSERT_EQ(ret, MSERR_OK);
    ret = ReportMediaInfo(instanceId);
    ASSERT_NE(ret, MSERR_OK);
    ASSERT_EQ(ret, MSERR_INVALID_OPERATION);
}

HWTEST_F(MediaDfxTest, FAULT_SOURCE_EVENT, TestSize.Level1)
{
    std::string appName = "appName";
    uint64_t instanceId = 1;
    std::string callerType = "callerType";
    int8_t sourceType = 1;
    std::string sourceUrl = "sourceUrl";
    std::string errorMessage = "errorMessage";
    FaultSourceEventWrite(appName, instanceId, callerType, sourceType, sourceUrl, errorMessage);
}

HWTEST_F(MediaDfxTest, FAULT_RECORD_AUDIO_EVENT, TestSize.Level1)
{
    std::string appName = "appName";
    uint64_t instanceId = 1;
    int8_t sourceType = 1;
    std::string errorMessage = "errorMessage";
    FaultRecordAudioEventWrite(appName, instanceId, sourceType, errorMessage);
}

HWTEST_F(MediaDfxTest, FAULT_SCREEN_CAPTURE_EVENT, TestSize.Level1)
{
    std::string appName = "appName";
    uint64_t instanceId = 1;
    int8_t captureMode = 1;
    int8_t dataMode = 1;
    int32_t errorCode = 1;
    std::string errorMessage = "errorMessage";
    FaultScreenCaptureEventWrite(appName, instanceId, captureMode, dataMode, errorCode, errorMessage);
}
}
}