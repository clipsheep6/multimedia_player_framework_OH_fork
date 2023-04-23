/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an 'AS IS' BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import hilog from '@ohos.hilog';
import { describe, it, expect } from '@ohos/hypium';
import AbilityDelegatorRegistry from '@ohos.application.abilityDelegatorRegistry';
// @ts-ignore
import { Driver, ON } from '@ohos.UiTest';

const BUNDLE = 'AVPlayer'
const TAG = '[Sample_AVPlayer]'
const DOMAIN = 0xF811

export default function abilityTest() {
  describe('ActsAbilityTest', function () {
    /**
     * 启动应用
     */
    it(BUNDLE + '_StartAbility',0, async function (done) {
      hilog.info(DOMAIN, TAG, BUNDLE + '_StartAbility start');
      let abilityDelegator = AbilityDelegatorRegistry.getAbilityDelegator();
      try {
        await abilityDelegator.startAbility({
          bundleName: 'com.example.ohmediademo',
          abilityName: 'EntryAbility'
        });
      } catch (exception) {
        hilog.info(DOMAIN, TAG, BUNDLE + '_StartAbility error');
        expect().assertFail();
      }
      hilog.info(DOMAIN, TAG, BUNDLE + '_StartAbility end');
      done();
    })
    /**
     * 点击权限
     */
    it(BUNDLE + '_Permission', 0, async () => {
      hilog.info(DOMAIN, TAG, BUNDLE + '_Permission begin');
      let driver = await Driver.create();
      await driver.delayMs(1000);
      await driver.assertComponentExist(ON.text('允许'));
      await driver.delayMs(1000);
      let btnAccept = await driver.findComponent(ON.text('允许'));
      await btnAccept.click();
      await driver.delayMs(1000);
      hilog.info(DOMAIN, TAG, BUNDLE + '_Permission end');
    })
    /**
     * 点击播放
     */
    it(BUNDLE + '_TestPlay', 0, async () => {
      hilog.info(DOMAIN, TAG, BUNDLE + '_TestPlay begin');
      let driver = await Driver.create();
      await driver.delayMs(1000);
      await driver.assertComponentExist(ON.id('本地视频播放_fdSrc'));
      await driver.delayMs(1000);
      let btnAccept = await driver.findComponent(ON.id('本地视频播放_fdSrc'));
      await btnAccept.click();
      await driver.delayMs(1000);
      await driver.assertComponentExist(ON.id('H264_AAC.mp4'));
      await driver.delayMs(1000);
      btnAccept = await driver.findComponent(ON.id('H264_AAC.mp4'));
      await btnAccept.click();
      await driver.delayMs(1000);
      await driver.assertComponentExist(ON.id('play'));
      await driver.delayMs(1000);
      btnAccept = await driver.findComponent(ON.id('play'));
      await btnAccept.click();
      await driver.delayMs(2000);
      hilog.info(DOMAIN, TAG, BUNDLE + '_TestPlay end');
    })
    /**
     * 点击暂停
     */
    it(BUNDLE + '_TestPause', 0, async () => {
      hilog.info(DOMAIN, TAG, BUNDLE + '_TestPause begin');
      let driver = await Driver.create();
      await driver.delayMs(1000);
      await driver.assertComponentExist(ON.id('play'));
      await driver.delayMs(1000);
      let btnAccept = await driver.findComponent(ON.id('play'));
      await btnAccept.click();
      await driver.delayMs(2000);
      hilog.info(DOMAIN, TAG, BUNDLE + '_TestPause end');
    })
    /**
     * 点击切换下一个视频
     */
    it(BUNDLE + '_TestNext', 0, async () => {
      hilog.info(DOMAIN, TAG, BUNDLE + '_TestNext begin');
      let driver = await Driver.create();
      await driver.delayMs(1000);
      await driver.assertComponentExist(ON.id('next'));
      await driver.delayMs(1000);
      let btnAccept = await driver.findComponent(ON.id('next'));
      await btnAccept.click();
      await driver.delayMs(2000);
      hilog.info(DOMAIN, TAG, BUNDLE + '_TestNext end');
    })
  })
}