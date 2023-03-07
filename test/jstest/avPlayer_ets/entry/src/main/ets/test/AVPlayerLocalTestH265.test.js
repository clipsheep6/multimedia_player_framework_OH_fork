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

import * as mediaTestBase from '../../../../../../MediaTestBase.js';
import { testAVPlayerFun } from '../../../../../../AVPlayerTestBase.js';
import { describe, beforeAll, beforeEach, afterEach, afterAll, it } from 'deccjsunit/index';

export default function AVPlayerLocalTestH265() {
    describe('AVPlayerLocalTestH265', function () {
        const PLAY_TIME = 3000;
        const TAG = 'AVPlayerLocalTestH265:';
        let fileDescriptor = null;
        let avPlayer = null;
        let avPlayTest = {
            width: 0,
            height: 0,
            duration: -1,
        }

        beforeAll(async function() {
            console.info(TAG + 'beforeAll case');
        })

        beforeEach(async function() {
            console.info(TAG + 'beforeEach case');
        })

        afterEach(async function() {
            if (avPlayer != null) {
                avPlayer.release().then(() => {
                }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
            }
            console.info(TAG + 'afterEach case');
        })

        afterAll(async function() {
            console.info(TAG + 'afterAll case');
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_0100
            * @tc.name      : 0100.H265_MP3_3840x2160_60fps.mp4
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_0100', 0, async function (done) {
            avPlayTest = { width: 3840, height: 2160, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_MP3_3840x2160_60fps.mp4').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_0200
            * @tc.name      : 0200.H265_MP3_1920x1080_60fps.mp4
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_0200', 0, async function (done) {
            avPlayTest = { width: 1920, height: 1080, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_MP3_1920x1080_60fps.mp4').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_0300
            * @tc.name      : 0300.H265_MP3_1280x720_60fps.mp4
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_0300', 0, async function (done) {
            avPlayTest = { width: 1280, height: 720, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_MP3_1280x720_60fps.mp4').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_0400
            * @tc.name      : 0400.H265_MP3_720x480_60fps.mp4
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_0400', 0, async function (done) {
            avPlayTest = { width: 720, height: 480, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_MP3_720x480_60fps.mp4').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_0500
            * @tc.name      : 0500.H265_MP3_480x270_60fps.mp4
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_0500', 0, async function (done) {
            avPlayTest = { width: 480, height: 270, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_MP3_480x270_60fps.mp4').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_0600
            * @tc.name      : 0600.H265_MP3_720x480_10fps.mp4
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_0600', 0, async function (done) {
            avPlayTest = { width: 720, height: 480, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_MP3_720x480_10fps.mp4').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_0700
            * @tc.name      : 0700.H265_MP3_720x480_25fps.mp4
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_0700', 0, async function (done) {
            avPlayTest = { width: 720, height: 480, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_MP3_720x480_25fps.mp4').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_0800
            * @tc.name      : 0800.H265_MP3_720x480_30fps.mp4
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_0800', 0, async function (done) {
            avPlayTest = { width: 720, height: 480, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_MP3_720x480_30fps.mp4').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_0900
            * @tc.name      : 0900.H265_MP3_720x480_60fps.ts
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_0900', 0, async function (done) {
            avPlayTest = { width: 720, height: 480, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_MP3_720x480_60fps.ts').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1000
            * @tc.name      : 1000.H265_MP3_720x480_60fps.mkv
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1000', 0, async function (done) {
            avPlayTest = { width: 720, height: 480, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_MP3_720x480_60fps.mkv').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1100
            * @tc.name      : 1100.H265_AAC_3840x2160_60fps.mp4
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1100', 0, async function (done) {
            avPlayTest = { width: 3840, height: 2160, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_AAC_3840x2160_60fps.mp4').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1200
            * @tc.name      : 1200.H265_AAC_1920x1080_60fps.mp4
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1200', 0, async function (done) {
            avPlayTest = { width: 1920, height: 1080, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_AAC_1920x1080_60fps.mp4').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1300
            * @tc.name      : 1300.H265_AAC_1280x720_60fps.mp4
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1300', 0, async function (done) {
            avPlayTest = { width: 1280, height: 720, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_AAC_1280x720_60fps.mp4').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1400
            * @tc.name      : 1400.H265_AAC_720x480_60fps.mp4
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1400', 0, async function (done) {
            avPlayTest = { width: 720, height: 480, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_AAC_720x480_60fps.mp4').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1500
            * @tc.name      : 1500.H265_AAC_480x270_60fps.mp4
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1500', 0, async function (done) {
            avPlayTest = { width: 480, height: 270, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_AAC_480x270_60fps.mp4').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1600
            * @tc.name      : 1600.H265_AAC_720x480_10fps.mp4
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1600', 0, async function (done) {
            avPlayTest = { width: 720, height: 480, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_AAC_720x480_10fps.mp4').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1700
            * @tc.name      : 1700.H265_AAC_720x480_25fps.mp4
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1700', 0, async function (done) {
            avPlayTest = { width: 720, height: 480, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_AAC_720x480_25fps.mp4').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1800
            * @tc.name      : 1800.H265_AAC_720x480_30fps.mp4
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1800', 0, async function (done) {
            avPlayTest = { width: 720, height: 480, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_AAC_720x480_30fps.mp4').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1900
            * @tc.name      : 1900.H265_AAC_720x480_60fps.ts
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_1900', 0, async function (done) {
            avPlayTest = { width: 720, height: 480, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_AAC_720x480_60fps.ts').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_2000
            * @tc.name      : 2000.H265_AAC_720x480_60fps.mkv
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Compatibility test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_H265_COMPATIBILITY_2000', 0, async function (done) {
            avPlayTest = { width: 720, height: 480, duration: 10085 };
            await mediaTestBase.getFileDescriptor('H265_AAC_720x480_60fps.mkv').then((res) => {
                fileDescriptor = res;
            });
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })
    })
}
