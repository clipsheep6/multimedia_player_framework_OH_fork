/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "system_sound_manager_unit_test.h"

using namespace OHOS::AbilityRuntime;
using namespace testing::ext;

namespace OHOS {
namespace Media {
void SystemSoundManagerUnitTest::SetUpTestCase(void) {}
void SystemSoundManagerUnitTest::TearDownTestCase(void) {}
void SystemSoundManagerUnitTest::SetUp(void) {}
void SystemSoundManagerUnitTest::TearDown(void) {}

/**
 * @tc.name  : Test GetDefaultRingtoneAttrs API
 * @tc.number: Media_SoundManager_GetDefaultRingtoneAttrs_001
 * @tc.desc  : Test GetDefaultRingtoneAttrs interface. Returns attributes of the default ringtone on success.
 */
HWTEST(SystemSoundManagerUnitTest, Media_SoundManager_GetDefaultRingtoneAttrs_001, TestSize.Level0)
{
    std::shared_ptr<SystemSoundManager> systemSoundManager_ = SystemSoundManagerFactory::CreateSystemSoundManager();
    std::shared_ptr<AbilityRuntime::Context> context_ = std::make_shared<ContextImpl>();
    std::shared_ptr<ToneAttrs> toneAttrs_ = systemSoundManager_->GetDefaultRingtoneAttrs(context_, RingtoneType::RINGTONE_TYPE_SIM_CARD_0);
    EXPECT_NE(toneAttrs_, nullptr);
}

/**
 * @tc.name  : Test GetRingtoneAttrList API
 * @tc.number: Media_SoundManager_GetRingtoneAttrList_001
 * @tc.desc  : Test GetRingtoneAttrList interface. Returns attribute list of ringtones on success.
 */
HWTEST(SystemSoundManagerUnitTest, Media_SoundManager_GetRingtoneAttrList_001, TestSize.Level0)
{
    std::shared_ptr<SystemSoundManager> systemSoundManager_ = SystemSoundManagerFactory::CreateSystemSoundManager();
    std::shared_ptr<AbilityRuntime::Context> context_ = std::make_shared<ContextImpl>();
    std::vector<std::shared_ptr<ToneAttrs>> ringtoneAttrsArray_ = systemSoundManager_->GetRingtoneAttrList(context_, RingtoneType::RINGTONE_TYPE_SIM_CARD_0);
    EXPECT_GT(ringtoneAttrsArray_.size(), 0);
}

/**
 * @tc.name  : Test GetDefaultSystemToneAttrs API
 * @tc.number: Media_SoundManager_GetDefaultSystemToneAttrs_001
 * @tc.desc  : Test GetDefaultSystemToneAttrs interface. Returns attributes of the default system tone.
 */
HWTEST(SystemSoundManagerUnitTest, Media_SoundManager_GetDefaultSystemToneAttrs_001, TestSize.Level0)
{
    std::shared_ptr<SystemSoundManager> systemSoundManager_ = SystemSoundManagerFactory::CreateSystemSoundManager();
    std::shared_ptr<AbilityRuntime::Context> context_ = std::make_shared<ContextImpl>();
    std::shared_ptr<ToneAttrs> toneAttrs_ = systemSoundManager_->GetDefaultSystemToneAttrs(context_, SystemToneType::SYSTEM_TONE_TYPE_SIM_CARD_0);
    EXPECT_NE(toneAttrs_, nullptr);
}

/**
 * @tc.name  : Test GetDefaultSystemToneAttrs API
 * @tc.number: Media_SoundManager_GetDefaultSystemToneAttrs_002
 * @tc.desc  : Test GetDefaultSystemToneAttrs interface. Returns attributes of the default system tone.
 */
HWTEST(SystemSoundManagerUnitTest, Media_SoundManager_GetDefaultSystemToneAttrs_002, TestSize.Level0)
{
    std::shared_ptr<SystemSoundManager> systemSoundManager_ = SystemSoundManagerFactory::CreateSystemSoundManager();
    std::shared_ptr<AbilityRuntime::Context> context_ = std::make_shared<ContextImpl>();
    std::shared_ptr<ToneAttrs> toneAttrs_ = systemSoundManager_->GetDefaultSystemToneAttrs(context_, SystemToneType::SYSTEM_TONE_TYPE_SIM_CARD_1);
    EXPECT_NE(toneAttrs_, nullptr);
}

/**
 * @tc.name  : Test GetDefaultSystemToneAttrs API
 * @tc.number: Media_SoundManager_GetDefaultSystemToneAttrs_003
 * @tc.desc  : Test GetDefaultSystemToneAttrs interface. Returns attributes of the default system tone.
 */
HWTEST(SystemSoundManagerUnitTest, Media_SoundManager_GetDefaultSystemToneAttrs_003, TestSize.Level0)
{
    std::shared_ptr<SystemSoundManager> systemSoundManager_ = SystemSoundManagerFactory::CreateSystemSoundManager();
    std::shared_ptr<AbilityRuntime::Context> context_ = std::make_shared<ContextImpl>();
    std::shared_ptr<ToneAttrs> toneAttrs_ = systemSoundManager_->GetDefaultSystemToneAttrs(context_, SystemToneType::SYSTEM_TONE_TYPE_NOTIFICATION);
    EXPECT_NE(toneAttrs_, nullptr);
}

/**
 * @tc.name  : Test GetSystemToneAttrList API
 * @tc.number: Media_SoundManager_GetSystemToneAttrList_001
 * @tc.desc  : Test GetSystemToneAttrList interface. Returns attributes of the default system tone.
 */
HWTEST(SystemSoundManagerUnitTest, Media_SoundManager_GetSystemToneAttrList_001, TestSize.Level0)
{
    std::shared_ptr<SystemSoundManager> systemSoundManager_ = SystemSoundManagerFactory::CreateSystemSoundManager();
    std::shared_ptr<AbilityRuntime::Context> context_ = std::make_shared<ContextImpl>();
    std::vector<std::shared_ptr<ToneAttrs>> ringtoneAttrsArray_ = systemSoundManager_->GetSystemToneAttrList(context_, SystemToneType::SYSTEM_TONE_TYPE_SIM_CARD_0);
    EXPECT_GT(ringtoneAttrsArray_.size(), 0);
}

/**
 * @tc.name  : Test GetSystemToneAttrList API
 * @tc.number: Media_SoundManager_GetSystemToneAttrList_002
 * @tc.desc  : Test GetSystemToneAttrList interface. Returns attributes of the default system tone.
 */
HWTEST(SystemSoundManagerUnitTest, Media_SoundManager_GetSystemToneAttrList_002, TestSize.Level0)
{
    std::shared_ptr<SystemSoundManager> systemSoundManager_ = SystemSoundManagerFactory::CreateSystemSoundManager();
    std::shared_ptr<AbilityRuntime::Context> context_ = std::make_shared<ContextImpl>();
    std::vector<std::shared_ptr<ToneAttrs>> ringtoneAttrsArray_ = systemSoundManager_->GetSystemToneAttrList(context_, SystemToneType::SYSTEM_TONE_TYPE_SIM_CARD_1);
    EXPECT_GT(ringtoneAttrsArray_.size(), 0);
}

/**
 * @tc.name  : Test GetSystemToneAttrList API
 * @tc.number: Media_SoundManager_GetSystemToneAttrList_003
 * @tc.desc  : Test GetSystemToneAttrList interface. Returns attributes of the default system tone.
 */
HWTEST(SystemSoundManagerUnitTest, Media_SoundManager_GetSystemToneAttrList_003, TestSize.Level0)
{
    std::shared_ptr<SystemSoundManager> systemSoundManager_ = SystemSoundManagerFactory::CreateSystemSoundManager();
    std::shared_ptr<AbilityRuntime::Context> context_ = std::make_shared<ContextImpl>();
    std::vector<std::shared_ptr<ToneAttrs>> ringtoneAttrsArray_ = systemSoundManager_->GetSystemToneAttrList(context_, SystemToneType::SYSTEM_TONE_TYPE_NOTIFICATION);
    EXPECT_GT(ringtoneAttrsArray_.size(), 0);
}

/**
 * @tc.name  : Test GetDefaultAlarmToneAttrs API
 * @tc.number: Media_SoundManager_GetDefaultAlarmToneAttrs_001
 * @tc.desc  : Test GetDefaultAlarmToneAttrs interface. Returns attributes of the default system tone.
 */
HWTEST(SystemSoundManagerUnitTest, Media_SoundManager_GetDefaultAlarmToneAttrs_001, TestSize.Level0)
{
    std::shared_ptr<SystemSoundManager> systemSoundManager_ = SystemSoundManagerFactory::CreateSystemSoundManager();
    std::shared_ptr<AbilityRuntime::Context> context_ = std::make_shared<ContextImpl>();
    std::shared_ptr<ToneAttrs> toneAttrs_ = systemSoundManager_->GetDefaultAlarmToneAttrs(context_);
    EXPECT_NE(toneAttrs_, nullptr);
}

/**
 * @tc.name  : Test GetAlarmToneAttrList API
 * @tc.number: Media_SoundManager_GetAlarmToneAttrList_001
 * @tc.desc  : Test GetAlarmToneAttrList interface. Returns attributes of the default system tone.
 */
HWTEST(SystemSoundManagerUnitTest, Media_SoundManager_GetAlarmToneAttrList_001, TestSize.Level0)
{
    std::shared_ptr<SystemSoundManager> systemSoundManager_ = SystemSoundManagerFactory::CreateSystemSoundManager();
    std::shared_ptr<AbilityRuntime::Context> context_ = std::make_shared<ContextImpl>();
    std::vector<std::shared_ptr<ToneAttrs>> alarmtoneAttrsArray_ = systemSoundManager_->GetAlarmToneAttrList(context_);
    EXPECT_GT(alarmtoneAttrsArray_.size(), 0);
}

/**
 * @tc.name  : Test GetAlarmToneUri API
 * @tc.number: Media_SoundManager_GetAlarmToneUri_001
 * @tc.desc  : Test GetAlarmToneUri interface. Returns attributes of the default system tone.
 */
HWTEST(SystemSoundManagerUnitTest, Media_SoundManager_GetAlarmToneUri_001, TestSize.Level0)
{
    std::shared_ptr<SystemSoundManager> systemSoundManager_ = SystemSoundManagerFactory::CreateSystemSoundManager();
    std::shared_ptr<AbilityRuntime::Context> context_ = std::make_shared<ContextImpl>();
    std::string uri = systemSoundManager_->GetAlarmToneUri(context_);
    EXPECT_NE(uri.empty(), false);
}

/**
 * @tc.name  : Test SetAlarmToneUri API
 * @tc.number: Media_SoundManager_SetAlarmToneUri_001
 * @tc.desc  : Test SetAlarmToneUri interface. Returns attributes of the default system tone.
 */
HWTEST(SystemSoundManagerUnitTest, Media_SoundManager_SetAlarmToneUri_001, TestSize.Level0)
{
    std::shared_ptr<SystemSoundManager> systemSoundManager_ = SystemSoundManagerFactory::CreateSystemSoundManager();
    std::shared_ptr<AbilityRuntime::Context> context_ = std::make_shared<ContextImpl>();
    std::string srcUri, dstUri;
    srcUri = systemSoundManager_->GetAlarmToneUri(context_);
    systemSoundManager_->SetAlarmToneUri(context_, srcUri);
    dstUri = systemSoundManager_->GetAlarmToneUri(context_);
    EXPECT_EQ(srcUri, dstUri);

    srcUri = "test";
    systemSoundManager_->SetAlarmToneUri(context_, srcUri);
    dstUri = systemSoundManager_->GetAlarmToneUri(context_);
    EXPECT_EQ(srcUri, dstUri);
}

/**
 * @tc.name  : Test OpenAlarmTone API
 * @tc.number: Media_SoundManager_OpenAlarmTone_001
 * @tc.desc  : Test OpenAlarmTone interface. Returns attributes of the default system tone.
 */
HWTEST(SystemSoundManagerUnitTest, Media_SoundManager_OpenAlarmTone_001, TestSize.Level0)
{
    std::shared_ptr<SystemSoundManager> systemSoundManager_ = SystemSoundManagerFactory::CreateSystemSoundManager();
    std::shared_ptr<AbilityRuntime::Context> context_ = std::make_shared<ContextImpl>();
    int fd = systemSoundManager_->OpenAlarmTone(context_, "test");
    EXPECT_LT(fd, 0);
    std::string uri = systemSoundManager_->GetAlarmToneUri(context_);
    fd = systemSoundManager_->OpenAlarmTone(context_, uri);
    EXPECT_GT(fd, 0);
}

/**
 * @tc.name  : Test AddCustomizedToneByExternalUri API
 * @tc.number: Media_SoundManager_AddCustomizedToneByExternalUri_001
 * @tc.desc  : Test AddCustomizedToneByExternalUri interface. Returns attributes of the default system tone.
 */
HWTEST(SystemSoundManagerUnitTest, Media_SoundManager_AddCustomizedToneByExternalUri_001, TestSize.Level0)
{
    std::shared_ptr<SystemSoundManager> systemSoundManager_ = SystemSoundManagerFactory::CreateSystemSoundManager();
    std::shared_ptr<AbilityRuntime::Context> context_ = std::make_shared<ContextImpl>();
    std::shared_ptr<ToneAttrs> toneAttrs_ = systemSoundManager_->GetDefaultRingtoneAttrs(context_, RingtoneType::RINGTONE_TYPE_SIM_CARD_0);
    std::string uri = systemSoundManager_->GetAlarmToneUri(context_);
    
    std::string res = systemSoundManager_->AddCustomizedToneByExternalUri(context_, toneAttrs_, "test");
    EXPECT_EQ(res.empty(), true);
    res = systemSoundManager_->AddCustomizedToneByExternalUri(context_, toneAttrs_, uri);
    EXPECT_EQ(res.empty(), false);
}

/**
 * @tc.name  : Test AddCustomizedToneByFdAndOffset API
 * @tc.number: Media_SoundManager_AddCustomizedToneByFdAndOffset_001
 * @tc.desc  : Test AddCustomizedToneByFdAndOffset interface. Returns attributes of the default system tone.
 */
HWTEST(SystemSoundManagerUnitTest, Media_SoundManager_AddCustomizedToneByFdAndOffset_001, TestSize.Level0)
{
    std::shared_ptr<SystemSoundManager> systemSoundManager_ = SystemSoundManagerFactory::CreateSystemSoundManager();
    std::shared_ptr<AbilityRuntime::Context> context_ = std::make_shared<ContextImpl>();
    std::shared_ptr<ToneAttrs> toneAttrs_ = systemSoundManager_->GetDefaultRingtoneAttrs(context_, RingtoneType::RINGTONE_TYPE_SIM_CARD_0);
    std::string uri = systemSoundManager_->GetAlarmToneUri(context_);

    std::string res;
    res = systemSoundManager_->AddCustomizedToneByFdAndOffset(context_, toneAttrs_, -1, 10, 1);
    EXPECT_EQ(res.empty(), true);
    res = systemSoundManager_->AddCustomizedToneByFdAndOffset(context_, nullptr, 1, 10, 0);
    EXPECT_EQ(res.empty(), true);
    res = systemSoundManager_->AddCustomizedToneByFdAndOffset(context_, toneAttrs_, 1, 10, 0);
    EXPECT_EQ(res.empty(), true);

    int fd = systemSoundManager_->OpenAlarmTone(context_, uri);
    res = systemSoundManager_->AddCustomizedToneByFdAndOffset(context_, toneAttrs_, fd, 10, 0);
    EXPECT_NE(res.empty(), true);
    res = systemSoundManager_->AddCustomizedToneByFdAndOffset(context_, toneAttrs_, fd, 10, 1);
    EXPECT_NE(res.empty(), true);
    systemSoundManager_->Close(fd);
}

/**
 * @tc.name  : Test RemoveCustomizedTone API
 * @tc.number: Media_SoundManager_RemoveCustomizedTone_001
 * @tc.desc  : Test RemoveCustomizedTone interface. Returns attributes of the default system tone.
 */
HWTEST(SystemSoundManagerUnitTest, Media_SoundManager_RemoveCustomizedTone_001, TestSize.Level0)
{
    std::shared_ptr<SystemSoundManager> systemSoundManager_ = SystemSoundManagerFactory::CreateSystemSoundManager();
    std::shared_ptr<AbilityRuntime::Context> context_ = std::make_shared<ContextImpl>();
    int res;
    res = systemSoundManager_->RemoveCustomizedTone(context_, "test");
    EXPECT_NE(res, 0);
    res = systemSoundManager_->RemoveCustomizedTone(context_, "");
    EXPECT_EQ(res, 0);
}

} // namespace Media
} // namespace OHOS