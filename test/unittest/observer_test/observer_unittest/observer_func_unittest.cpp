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

#include <gtest/gtest.h>
#include <string>
#include "incall_observer.h"
#include <cstdlib>

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::Media;

namespace OHOS {
namespace Media {
namespace InCallObserverFuncUT {

class InCallObserverTestCallBack : public InCallObserverCallBack {
public:
    InCallObserverTestCallBack() {}
    ~InCallObserverTestCallBack() {}
    bool StopAndRelease()
    {
        return true;
    }
};

class InCallObserverTestFalseCallBack : public InCallObserverCallBack {
public:
    InCallObserverTestFalseCallBack() {}
    ~InCallObserverTestFalseCallBack() {}
    bool StopAndRelease()
    {
        return false;
    }
};

class InCallObserverInnerUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

void InCallObserverInnerUnitTest::SetUpTestCase(void) {}

void InCallObserverInnerUnitTest::TearDownTestCase(void) {}

void InCallObserverInnerUnitTest::SetUp(void)
{
    std::cout << "[SetUp]: SetUp!!!, test: "<< std::endl;
}

void InCallObserverInnerUnitTest::TearDown(void)
{
    std::cout << "[TearDown]: over!!!" << std::endl;
}

/**
 * @tc.name: RegisterObserver_01
 * @tc.desc: RegisterObserver_01
 * @tc.type: FUNC
 */
HWTEST_F(InCallObserverInnerUnitTest, RegisterObserver_01, TestSize.Level1)
{
    ASSERT_TRUE(InCallObserver::GetInstance().RegisterObserver());
    InCallObserver::GetInstance().UnRegisterObserver();
}

/**
 * @tc.name: OnCallStateUpdated_01
 * @tc.desc: OnCallStateUpdated_01
 * @tc.type: FUNC
 */
HWTEST_F(InCallObserverInnerUnitTest, OnCallStateUpdated_01, TestSize.Level1)
{
    ASSERT_FALSE(InCallObserver::GetInstance().IsInCall());
    InCallObserver::GetInstance().OnCallStateUpdated(true);
    ASSERT_TRUE(InCallObserver::GetInstance().IsInCall());
    InCallObserver::GetInstance().OnCallStateUpdated(false);
    ASSERT_FALSE(InCallObserver::GetInstance().IsInCall());
}

/**
 * @tc.name: RegisterInCallObserverCallBack_01
 * @tc.desc: RegisterInCallObserverCallBack_01
 * @tc.type: FUNC
 */
HWTEST_F(InCallObserverInnerUnitTest, RegisterInCallObserverCallBack_01, TestSize.Level1)
{
    auto inCallObserverCallBack = std::make_shared<InCallObserverTestCallBack>();
    ASSERT_TRUE(InCallObserver::GetInstance().RegisterInCallObserverCallBack(inCallObserverCallBack));
    InCallObserver::GetInstance().UnRegisterInCallObserverCallBack();
    ASSERT_TRUE(inCallObserverCallBack->StopAndRelease());
}

/**
 * @tc.name: InCallCallBackReturn_01
 * @tc.desc: InCallCallBackReturn_01
 * @tc.type: FUNC
 */
HWTEST_F(InCallObserverInnerUnitTest, InCallCallBackReturn_01, TestSize.Level1)
{
    InCallObserver::GetInstance().UnRegisterObserver();
    ASSERT_TRUE(InCallObserver::GetInstance().RegisterObserver());
    auto inCallObserverCallBack = std::make_shared<InCallObserverTestCallBack>();
    ASSERT_TRUE(InCallObserver::GetInstance().RegisterInCallObserverCallBack(inCallObserverCallBack));
    ASSERT_TRUE(InCallObserver::GetInstance().OnCallStateUpdated(true));
    ASSERT_TRUE(InCallObserver::GetInstance().IsInCall());
    InCallObserver::GetInstance().UnRegisterObserver();
}

/**
 * @tc.name: InCallCallBackReturn_02
 * @tc.desc: InCallCallBackReturn_02
 * @tc.type: FUNC
 */
HWTEST_F(InCallObserverInnerUnitTest, InCallCallBackReturn_02, TestSize.Level1)
{
    InCallObserver::GetInstance().UnRegisterObserver();
    ASSERT_TRUE(InCallObserver::GetInstance().RegisterObserver());
    auto inCallObserverCallBack = std::make_shared<InCallObserverTestCallBack>();
    ASSERT_TRUE(InCallObserver::GetInstance().RegisterInCallObserverCallBack(inCallObserverCallBack));
    ASSERT_TRUE(InCallObserver::GetInstance().OnCallStateUpdated(true));
    ASSERT_TRUE(InCallObserver::GetInstance().IsInCall());
    ASSERT_TRUE(InCallObserver::GetInstance().OnCallStateUpdated(false));
    ASSERT_FALSE(InCallObserver::GetInstance().IsInCall());
    sleep(3); // 3 second
    ASSERT_TRUE(InCallObserver::GetInstance().OnCallStateUpdated(true));
    ASSERT_TRUE(InCallObserver::GetInstance().IsInCall());
    InCallObserver::GetInstance().UnRegisterInCallObserverCallBack();
    ASSERT_TRUE(InCallObserver::GetInstance().OnCallStateUpdated(true));
    ASSERT_TRUE(InCallObserver::GetInstance().IsInCall());
    InCallObserver::GetInstance().UnRegisterObserver();
}

/**
 * @tc.name: InCallCallBackReturn_03
 * @tc.desc: InCallCallBackReturn_03
 * @tc.type: FUNC
 */
HWTEST_F(InCallObserverInnerUnitTest, InCallCallBackReturn_03, TestSize.Level1)
{
    InCallObserver::GetInstance().UnRegisterObserver();
    ASSERT_TRUE(InCallObserver::GetInstance().RegisterObserver());
    auto inCallObserverTestFalseCallBack = std::make_shared<InCallObserverTestFalseCallBack>();
    ASSERT_TRUE(InCallObserver::GetInstance().RegisterInCallObserverCallBack(inCallObserverTestFalseCallBack));
    ASSERT_TRUE(InCallObserver::GetInstance().OnCallStateUpdated(false));
    ASSERT_FALSE(InCallObserver::GetInstance().IsInCall());
    sleep(3); // 3 second
    ASSERT_FALSE(InCallObserver::GetInstance().OnCallStateUpdated(true));
    ASSERT_TRUE(InCallObserver::GetInstance().IsInCall());
    InCallObserver::GetInstance().UnRegisterInCallObserverCallBack();
    ASSERT_TRUE(InCallObserver::GetInstance().OnCallStateUpdated(true));
    ASSERT_TRUE(InCallObserver::GetInstance().IsInCall());
    InCallObserver::GetInstance().UnRegisterObserver();
}
} // namespace InCallObserverFuncUT
} // namespace Media
} // namespace OHOS
