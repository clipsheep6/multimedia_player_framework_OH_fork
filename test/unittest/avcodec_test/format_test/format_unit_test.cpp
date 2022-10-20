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


#include "format_unit_test.h"
#include <cmath>
#include "gtest/gtest.h"
#include "media_errors.h"
#include "securec.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace OHOS::Media::FormatTestParam;

void FormatUnitTest::SetUpTestCase(void) {}

void FormatUnitTest::TearDownTestCase(void) {}

void FormatUnitTest::SetUp(void)
{
    format_ = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);
}

void FormatUnitTest::TearDown(void)
{
    if (format_ != nullptr) {
        format_->Destroy();
    }
}

/**
 * @tc.name: format_value_0100
 * @tc.desc: format set and get value
 * @tc.type: FUNC
 * @tc.require: issueI5OWXY issueI5OXCD
 */
HWTEST_F(FormatUnitTest, format_value_0100, TestSize.Level0)
{
    const std::string_view intKey = "IntKey";
    const std::string_view longKey = "LongKey";
    const std::string_view floatKey = "FloatKey";
    const std::string_view doubleKey = "DoubleKey";
    const std::string_view stringKey = "StringKey";
    int32_t intValue = 1;
    int64_t longValue = 1;
    float floatValue = 1.0;
    double doubleValue = 1.0;
    const std::string stringValue = "StringValue";

    int32_t getIntValue = 0;
    int64_t getLongValue = 0;
    float getFloatValue = 0.0;
    double getDoubleValue = 0.0;
    std::string getStringValue = "";
    
    ASSERT_TRUE(format_->PutIntValue(intKey, intValue));
    ASSERT_TRUE(format_->GetIntValue(intKey, getIntValue));
    ASSERT_TRUE(intValue == getIntValue);

    ASSERT_TRUE(format_->PutLongValue(longKey, longValue));
    ASSERT_TRUE(format_->GetLongValue(longKey, getLongValue));
    ASSERT_TRUE(longValue == getLongValue);

    ASSERT_TRUE(format_->PutFloatValue(floatKey, floatValue));
    ASSERT_TRUE(format_->GetFloatValue(floatKey, getFloatValue));
    ASSERT_TRUE(fabs(floatValue - getFloatValue) < EPSINON_FLOAT);

    ASSERT_TRUE(format_->PutDoubleValue(doubleKey, doubleValue));
    ASSERT_TRUE(format_->GetDoubleValue(doubleKey, getDoubleValue));
    ASSERT_TRUE(fabs(doubleValue - getDoubleValue) < EPSINON_DOUBLE);

    ASSERT_TRUE(format_->PutStringValue(stringKey, stringValue.c_str()));
    ASSERT_TRUE(format_->GetStringValue(stringKey, getStringValue));
    ASSERT_TRUE(stringValue == getStringValue);
}

/**
 * @tc.name: format_buffer_0100
 * @tc.desc: format put and get buffer
 * @tc.type: FUNC
 * @tc.require: issueI5OWXY issueI5OXCD
 */
HWTEST_F(FormatUnitTest, format_buffer_0100, TestSize.Level0)
{
    constexpr int32_t num = 10;
    const std::string_view key = "BufferKey";
    size_t size = num * sizeof(uint8_t);
    uint8_t *buffer = reinterpret_cast<uint8_t *>(malloc(size));
    (void)memset_s(buffer, size, 1, size);

    ASSERT_TRUE(format_->PutBuffer(key, buffer, size));
    uint8_t *getBuffer;
    size_t getSize;
    ASSERT_TRUE(format_->GetBuffer(key, &getBuffer, getSize));
    ASSERT_TRUE(size == getSize);
    for (int32_t i = 0; i < num; i++) {
        ASSERT_TRUE(buffer[i] == getBuffer[i]);
    }
    free(buffer);
}

/**
 * @tc.name: format_dump_info_0100
 * @tc.desc: format dump info
 * @tc.type: FUNC
 * @tc.require: issueI5OWXY issueI5OXCD
 */
HWTEST_F(FormatUnitTest, format_dump_info_0100, TestSize.Level0)
{
    ASSERT_TRUE(format_->PutIntValue("width", 1));
    ASSERT_TRUE(format_->PutIntValue("height", 1));
    ASSERT_TRUE(format_->PutStringValue("codec_mime", "video/avc"));
    const char *info = format_->DumpInfo();
    ASSERT_TRUE(info != nullptr);
    std::cout << info << std::endl;
}