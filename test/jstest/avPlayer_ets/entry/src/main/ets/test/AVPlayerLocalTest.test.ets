/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

import {testAVPlayerSeek, } from '../../../../../../AVPlayerTestBase.js';
import * as mediaTestBase from '../../../../../../MediaTestBase.js';
import media from '@ohos.multimedia.media'
import {testErrorAudioInterruptMode, testAVPlayerFun, testCheckTrackDescription, testVideoScaleType, testAudioInterruptMode} from '../../../../../../AVPlayerTestBase.js';
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from 'deccjsunit/index';

export default function AVPlayerLocalTest() {
    describe('AVPlayerLocalTest', function () {
        const VIDEO_SOURCE = 'H264_AAC.mp4';
        const AUDIO_SOURCE = '01.mp3';
        const VIDEO_NOAUDIO = 'H264_NONE.mp4'
        const PLAY_TIME = 3000;
        const TAG = 'AVPlayerLocalTest:';
        let fileDescriptor = null;
        let fileDescriptor2 = null;
        let fileDescriptor3 = null;
        let avPlayer = null;
        let avPlayTest = {
            width: 0,
            height: 0,
            duration: -1,
        }
        let fdPath = '';
        let fdNumber = 0;

        beforeAll(async function() {
            console.info('beforeAll case');
            await mediaTestBase.getFileDescriptor(VIDEO_SOURCE).then((res) => {
                fileDescriptor = res;
            });
            await mediaTestBase.getFileDescriptor(AUDIO_SOURCE).then((res) => {
                fileDescriptor2 = res;
            });
            await mediaTestBase.getFileDescriptor(VIDEO_NOAUDIO).then((res) => {
                fileDescriptor3 = res;
            });
        })

        beforeEach(async function() {
            console.info('beforeEach case');
        })

        afterEach(async function() {
            await mediaTestBase.clearRouter();
            console.info('afterEach case');
        })

        afterAll(async function() {
            console.info('afterAll case');
            if (avPlayer != null) {
                avPlayer.release().then(() => {
                }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
            }
            await mediaTestBase.closeFileDescriptor(VIDEO_SOURCE);
            await mediaTestBase.closeFileDescriptor(AUDIO_SOURCE);
            await mediaTestBase.closeFileDescriptor(VIDEO_NOAUDIO);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_FDSRC_0100
            * @tc.name      : 001.test fdsrc
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_FDSRC_0100', 0, async function (done) {
            avPlayTest = { width: 720, height: 480, duration: 10100 };
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_GETTRECKDESCRIPTION_0100
            * @tc.name      : 001.test getTrackDescription
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_GETTRECKDESCRIPTION_0100', 0, async function (done) {
            let videoTrackKey = new Array('bitrate', 'codec_mime', 'frame_rate', 'height',
                                        'track_index', 'track_type', 'width');
            let audioTrackKey = new Array('bitrate', 'channel_count', 'codec_mime', 'sample_rate',
                                        'track_index', 'track_type');
            let videoTrackValue = new Array(1366541, 0, 6000, 480, 0, 1, 720);
            let audioTrackValue = new Array(129207, 2, 1, 44100, 1, 0);
            let descriptionKey = new Array(videoTrackKey, audioTrackKey);
            let descriptionValue = new Array(videoTrackValue, audioTrackValue);
            testCheckTrackDescription(fileDescriptor, avPlayer, descriptionKey, descriptionValue, done)
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_GETTRECKDESCRIPTION_0200
            * @tc.name      : 002.test getTrackDescription
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_GETTRECKDESCRIPTION_0200', 0, async function (done) {
            let audioTrackKey = new Array('channel_count', 'codec_mime', 'sample_rate', 'track_index',
            'track_type');
            let audioTrackValue = new Array(1, 2, 48000, 0, 0);
            let descriptionKey = new Array(audioTrackKey);
            let descriptionValue = new Array(audioTrackValue);
            testCheckTrackDescription(fileDescriptor2, avPlayer, descriptionKey, descriptionValue, done)
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_GETTRECKDESCRIPTION_0300
            * @tc.name      : 003.test getTrackDescription
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_GETTRECKDESCRIPTION_0300', 0, async function (done) {
            let videoTrackKey = new Array('bitrate', 'codec_mime', 'frame_rate', 'height',
                                        'track_index', 'track_type', 'width');
            let videoTrackValue = new Array(1506121, 0, 6000, 480, 0, 1, 720);
            let descriptionKey = new Array(videoTrackKey);
            let descriptionValue = new Array(videoTrackValue);
            testCheckTrackDescription(fileDescriptor3, avPlayer, descriptionKey, descriptionValue, done)
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_VIDEOSCALETYPE_0100
            * @tc.name      : 001.test video player videoScaleTpe
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_VIDEOSCALETYPE_0100', 0, async function (done) {
            testVideoScaleType(fileDescriptor, avPlayer, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_AUDIOINTERRUPTMODE_0100
            * @tc.name      : 001.test audioInterruptMode Function
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_AUDIOINTERRUPTMODE_0100', 0, async function (done) {
            testAudioInterruptMode(fileDescriptor2, fileDescriptor, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_AUDIOINTERRUPTMODE_0200
            * @tc.name      : 002.test audioInterruptMode Function
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_AUDIOINTERRUPTMODE_0200', 0, async function (done) {
            testAudioInterruptMode(fileDescriptor, fileDescriptor2, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_AUDIOINTERRUPTMODE_0300
            * @tc.name      : 003.test audioInterruptMode error Function
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_AUDIOINTERRUPTMODE_0300', 0, async function (done) {
            testErrorAudioInterruptMode(fileDescriptor, done);
        })
    })
}