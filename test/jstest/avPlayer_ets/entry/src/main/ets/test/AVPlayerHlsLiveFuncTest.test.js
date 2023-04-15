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
import media from '@ohos.multimedia.media'
import audio from '@ohos.multimedia.audio';
import * as mediaTestBase from '../../../../../../MediaTestBase.js';
import * as AVPlayerTestBase from '../../../../../../AVPlayerTestBase.js';
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from 'deccjsunit/index';

export default function AVPlayerHlsLiveFuncTest() {
    describe('AVPlayerHlsLiveFuncTest', function () {
        const HLS_PATH = 'http://xxx.xxx.xxx.xxx:xx/xx/index.m3u8';
        const MULTI_HLS_PATH = 'http://xxx.xxx.xxx.xxx:xx/multi/index.m3u8'
        const PLAY_TIME = 2000;
        let avPlayer = null;
        let surfaceID = globalThis.value;

        const AV_MULTI_HLS_LIVE_BITRATE = [
            224726,
            389070,
            592145,
            883770,
            1239184,
            1801257
        ]

        let trackDescription = {
            'codec_mime': 'video/mpegts',
            'track_index': '0',
            'track_type': '1'
        }

        const audioRendererInfo = {
            content: audio.ContentType.CONTENT_TYPE_MOVIE,
            usage: audio.StreamUsage.STREAM_USAGE_MEDIA,
            rendererFlags: 0,
        }
        const videoScaleType1 = media.VideoScaleType.VIDEO_SCALE_TYPE_FIT;
        const audioInterruptMode1 = audio.InterruptMode.INDEPENDENT_MODE;
        const videoScaleType2 = media.VideoScaleType.VIDEO_SCALE_TYPE_FIT_CROP;
        const audioInterruptMode2 = audio.InterruptMode.SHARE_MODE;

        beforeAll(async function() {
            console.info('beforeAll case');
        })

        beforeEach(async function() {
            console.info('beforeEach case');
        })

        afterEach(async function() {
            console.info('afterEach case');
        })

        afterAll(async function() {
            console.info('afterAll case');
            if (avPlayer != null) {
                avPlayer.release().then(() => {
                }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
            }
        })

        function checkArray(realArray, expectArray) {
            expect(realArray.length).assertEqual(expectArray.length);
            for (let i = 0; i < expectArray.length; i++) {
                console.info('case expect ' + expectArray[i] + ', real value is ' + realArray[i]);
                expect(realArray[i]).assertEqual(expectArray[i]);
            }
        }

        function setAttribute(avPlayer, videoScaleTypeValue, audioInterruptModeValue) {
            avPlayer.videoScaleType = videoScaleTypeValue;
            avPlayer.audioInterruptMode = audioInterruptModeValue;
        }

        function expectAttributeEqual(avPlayer, expectRender, expectScaleType, expectInterruptMode) {
            expect(avPlayer.audioRendererInfo.content).assertEqual(expectRender.content);
            expect(avPlayer.audioRendererInfo.usage).assertEqual(expectRender.usage);
            expect(avPlayer.audioRendererInfo.rendererFlags).assertEqual(expectRender.rendererFlags);
            expect(avPlayer.videoScaleType).assertEqual(expectScaleType);
            expect(avPlayer.audioInterruptMode).assertEqual(expectInterruptMode);
        }

        function NextSteps(avPlayer, src, steps) {
            switch(steps[0]) {
                case 'src':
                    steps.shift();
                    avPlayer.url = src;
                    break;
                case 'prepare':
                    steps.shift();
                    avPlayer.prepare();
                    break;
                case 'play':
                    steps.shift();
                    avPlayer.play();
                    break;
                case 'seek':
                    steps.shift();
                    const seekNum = steps[0];
                    steps.shift();
                    avPlayer.seek(seekNum);
                    break;
                case 'loop':
                    steps.shift();
                    const loopFlag = steps[0];
                    steps.shift();
                    avPlayer.loop = loopFlag;
                    break;
                case 'setSpeed':
                    steps.shift();
                    const speedNum = steps[0];
                    steps.shift();
                    avPlayer.setSpeed(speedNum);
                    break;
                case 'duration':
                    steps.shift();
                    expect(avPlayer.duration).assertEqual(steps[0]);
                    steps.shift();
                    NextSteps(avPlayer, src, steps);
                    break;
                case 'currentTime':
                    steps.shift();
                    expect(avPlayer.currentTime).assertEqual(steps[0]);
                    steps.shift();
                    NextSteps(avPlayer, src, steps);
                    break;
                case 'setBitrate':
                    steps.shift();
                    avPlayer.setBitrate(steps[0]);
                    steps.shift();
                    NextSteps(avPlayer, src, steps);
                    break;
                case 'pause':
                    steps.shift();
                    avPlayer.pause();
                    break;
                case 'stop':
                    steps.shift();
                    avPlayer.stop();
                    break;
                case 'reset':
                    steps.shift();
                    avPlayer.reset();
                    break;
                case 'release':
                    steps.shift();
                    avPlayer.release();
                    break;
                default:
                    break;
            }
        }

        function testFun(avPlayer, src, surfaceID, Steps, done) {
            avPlayer.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AVPlayerTestBase.AV_PLAYER_STATE.IDLE:
                        NextSteps(avPlayer, src, Steps);
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.INITIALIZED:
                        avPlayer.surfaceId = surfaceID;
                        NextSteps(avPlayer, src, Steps);
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PREPARED:
                        NextSteps(avPlayer, src, Steps);
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PLAYING:
                        await mediaTestBase.msleepAsync(PLAY_TIME);
                        NextSteps(avPlayer, src, Steps);
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PAUSED:
                        await mediaTestBase.msleepAsync(PLAY_TIME);
                        NextSteps(avPlayer, src, Steps);
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.STOPPED:
                        NextSteps(avPlayer, src, Steps);
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.RELEASED:
                        avPlayer = null;
                        done();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.ERROR:
                        console.info(`case AVPlayerTestBase.AV_PLAYER_STATE.ERROR`);
                        expect().assertFail();
                        avPlayer.release();
                        break;
                    default:
                        break; 
                }
            });
            avPlayer.on('error', (err) => {
                console.info(`case error called,errName is ${err.name},errCode is ${err.code},errMessage is ${err.message}`);
                avPlayer.release();
                expect().assertFail();
            });
            console.info(`case src is ${src}`);
            Steps.shift();
            avPlayer.url = src;
        }

        function testReliability(avPlayer, src, surfaceID, Steps, done) {
            avPlayer.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AVPlayerTestBase.AV_PLAYER_STATE.IDLE:
                        NextSteps(avPlayer, src, Steps);
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.INITIALIZED:
                        avPlayer.surfaceId = surfaceID;
                        NextSteps(avPlayer, src, Steps);
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PREPARED:
                        NextSteps(avPlayer, src, Steps);
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PLAYING:
                        await mediaTestBase.msleepAsync(PLAY_TIME);
                        NextSteps(avPlayer, src, Steps);
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PAUSED:
                        await mediaTestBase.msleepAsync(PLAY_TIME);
                        NextSteps(avPlayer, src, Steps);
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.STOPPED:
                        NextSteps(avPlayer, src, Steps);
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.RELEASED:
                        avPlayer = null;
                        done();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.ERROR:
                        console.info(`case AVPlayerTestBase.AV_PLAYER_STATE.ERROR`);
                        expect().assertFail();
                        avPlayer.release();
                        break;
                    default:
                        break; 
                }
            });
            avPlayer.on('error', (err) => {
                console.info(`case error called,errName is ${err.name},errCode is ${err.code},errMessage is ${err.message}`);
                expect(Steps[0]).assertEqual('error');
                Steps.shift();
                NextSteps(avPlayer, src, Steps);
            });
            avPlayer.on('bufferingUpdate', (infoType, value) => {
                console.info('case bufferingUpdate success, infoType is ' + infoType + ', value is ' + value);
            });
            console.info(`case src is ${src}`);
            Steps.shift();
            avPlayer.url = src;
        }

        function testBufferingUpdate(avPlayer, src, surfaceID, Steps, done) {
            let isUpdate = false;
            avPlayer.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AVPlayerTestBase.AV_PLAYER_STATE.IDLE:
                        NextSteps(avPlayer, src, Steps);
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.INITIALIZED:
                        avPlayer.surfaceId = surfaceID;
                        NextSteps(avPlayer, src, Steps);
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PREPARED:
                        NextSteps(avPlayer, src, Steps);
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PLAYING:
                        await mediaTestBase.msleepAsync(8000);   // wait play 8000 ms
                        NextSteps(avPlayer, src, Steps);
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.STOPPED:
                        NextSteps(avPlayer, src, Steps);
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.RELEASED:
                        avPlayer = null;
                        expect(isUpdate).assertEqual(true);
                        done();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.ERROR:
                        expect().assertFail();
                        avPlayer.release();
                        break;
                    default:
                        break; 
                }
            });
            avPlayer.on('error', (err) => {
                console.info(`case error called,errName is ${err.name},errCode is ${err.code},errMessage is ${err.message}`);
                expect(Steps[0]).assertEqual('error');
                Steps.shift();
                NextSteps(avPlayer, src, Steps);
            });
            avPlayer.on('bufferingUpdate', (infoType, value) => {
                console.info('case bufferingUpdate success, infoType is ' + infoType + ', value is ' + value);
                isUpdate = true;
            });
            console.info(`case src is ${src}`);
            Steps.shift();
            avPlayer.url = src;
        }

        function testSetMultiBitrate(avPlayer, src, surfaceID, done) {
            let nowBitRate = -1;
            avPlayer.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AVPlayerTestBase.AV_PLAYER_STATE.INITIALIZED:
                        avPlayer.surfaceId = surfaceID;
                        avPlayer.prepare();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PREPARED:
                        nowBitRate = AV_MULTI_HLS_LIVE_BITRATE[0];
                        avPlayer.setBitrate(nowBitRate);
                        avPlayer.play();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PLAYING:
                        nowBitRate = AV_MULTI_HLS_LIVE_BITRATE[1];
                        avPlayer.setBitrate(nowBitRate);
                        await mediaTestBase.msleepAsync(8000); // wait 8000ms
                        avPlayer.pause();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PAUSED:
                        nowBitRate = AV_MULTI_HLS_LIVE_BITRATE[2];
                        avPlayer.setBitrate(nowBitRate);
                        await mediaTestBase.msleepAsync(8000); // wait 8000ms
                        avPlayer.stop();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.STOPPED:
                        avPlayer.release();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.RELEASED:
                        console.info(`case AVPlayerTestBase.AV_PLAYER_STATE.RELEASED`);
                        avPlayer = null;
                        done();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.ERROR:
                        expect().assertFail();
                        avPlayer.release();
                        break;
                    default:
                        break; 
                }
            });
            avPlayer.on('error', (err) => {
                console.info(`case error called,errName is ${err.name},errCode is ${err.code},errMessage is ${err.message}`);
                avPlayer.release();
                expect().assertFail();
            });
            avPlayer.on('bitrateDone', (bitrate) => {
                expect(bitrate).assertEqual(nowBitRate);
            });
            console.info(`case src is ${src}`);
            avPlayer.url = src;
        }

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_FUNCTION_0100
            * @tc.name      : 001.test hls live - stop
            * @tc.desc      : Hls live playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_FUNCTION_0100', 0, async function (done) {
            await media.createAVPlayer().then((video) => {
                if (typeof (video) != 'undefined') {
                    console.info('case createAVPlayer success');
                    avPlayer = video;
                } else {
                    console.error('case createAVPlayer failed');
                    expect().assertFail();
                    done();
                }
            }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
            let Steps = new Array('src', 'prepare', 'play', 'pause', 'stop', 'prepare', 'stop',
                        'prepare', 'play', 'stop', 'release');
            let surfaceID = globalThis.value;
            testFun(avPlayer, HLS_PATH, surfaceID, Steps, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_FUNCTION_0200
            * @tc.name      : 002.test hls live - reset
            * @tc.desc      : Hls live playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_FUNCTION_0200', 0, async function (done) {
            await media.createAVPlayer().then((video) => {
                if (typeof (video) != 'undefined') {
                    console.info('case createAVPlayer success');
                    avPlayer = video;
                } else {
                    console.error('case createAVPlayer failed');
                    expect().assertFail();
                    done();
                }
            }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
            let Steps = new Array('src', 'reset', 'src', 'prepare', 'reset', 'src', 'prepare', 'play',
                        'reset', 'src', 'prepare', 'play', 'pause', 'reset','src', 'prepare', 'play',
                        'stop', 'reset', 'release');
            let surfaceID = globalThis.value;
            testFun(avPlayer, HLS_PATH, surfaceID, Steps, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_FUNCTION_0300
            * @tc.name      : 003.test hls live - set
            * @tc.desc      : Hls live playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level2
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_FUNCTION_0300', 0, async function (done) {
            avPlayer = await media.createAVPlayer();
            avPlayer.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AVPlayerTestBase.AV_PLAYER_STATE.INITIALIZED:
                        avPlayer.surfaceId = surfaceID;
                        avPlayer.audioRendererInfo = audioRendererInfo;
                        avPlayer.prepare();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PREPARED:
                        setAttribute(avPlayer, videoScaleType2, audioInterruptMode2);
                        expectAttributeEqual(avPlayer, audioRendererInfo, videoScaleType2, audioInterruptMode2);
                        avPlayer.play();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PLAYING:
                        setAttribute(avPlayer, videoScaleType1, audioInterruptMode1);
                        expectAttributeEqual(avPlayer, audioRendererInfo, videoScaleType1, audioInterruptMode1);
                        avPlayer.setVolume(0.1); // set volume value is 0.1
                        avPlayer.pause();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PAUSED:
                        setAttribute(avPlayer, videoScaleType2, audioInterruptMode2);
                        expectAttributeEqual(avPlayer, audioRendererInfo, videoScaleType2, audioInterruptMode2);
                        avPlayer.release();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.RELEASED:
                        avPlayer = null;
                        done();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.ERROR:
                        avPlayer.release();
                        expect().assertFail();
                        break;
                    default:
                        break; 
                }
            });
            avPlayer.on('volumeChange', (vol) => {
                expect(true).assertEqual(Math.abs(0.1 - vol) < 0.05); // set volume value is 0.1, deviation is 0.05
            })
            avPlayer.on('error', (err) => {
                console.info(`case error called,errName ${err.name},errCode ${err.code},errMessage ${err.message}`);
                avPlayer.release();
                expect().assertFail();
            });
            avPlayer.url = HLS_PATH;
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_FUNCTION_0400
            * @tc.name      : 004.test hls live - getTrackDescription
            * @tc.desc      : Hls live playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level2
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_FUNCTION_0400', 0, async function (done) {
            await media.createAVPlayer().then((video) => {
                if (typeof (video) != 'undefined') {
                    avPlayer = video;
                } else {
                    expect().assertFail();
                    done();
                }
            }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
            avPlayer.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AVPlayerTestBase.AV_PLAYER_STATE.INITIALIZED:
                        avPlayer.surfaceId = surfaceID;
                        avPlayer.prepare();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PREPARED:
                        await avPlayer.getTrackDescription((error, arrList) => {
                            if ((arrList) != null) {
                                for (let i = 0; i < arrList.length; i++) {
                                    for (let item in arrList[i]) {
                                        expect(arrList[i][item].toString()).assertEqual(trackDescription[item].toString());
                                    }
                                }
                            } else {
                                console.log(`video getTrackDescription fail, error:${error}`);
                            }
                        });
                        avPlayer.release();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.RELEASED:
                        avPlayer = null;
                        done();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.ERROR:
                        console.info(`case AVPlayerTestBase.AV_PLAYER_STATE.ERROR`);
                        avPlayer.release();
                        expect().assertFail();
                        break;
                    default:
                        break; 
                }
            });
            avPlayer.on('error', (err) => {
                console.info(`case error called,errName is ${err.name}, case error called,errCode is ${err.code},
                            case error called,errMessage is ${err.message}`);
                avPlayer.release();
                expect().assertFail();
            });
            console.info(`case src is ${HLS_PATH}`);
            avPlayer.url = HLS_PATH;
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_FUNCTION_0500
            * @tc.name      : 005.test hls live - bufferingUpdate
            * @tc.desc      : Hls live playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_FUNCTION_0500', 0, async function (done) {
            await media.createAVPlayer().then((video) => {
                if (typeof (video) != 'undefined') {
                    console.info('case createAVPlayer success');
                    avPlayer = video;
                } else {
                    console.error('case createAVPlayer failed');
                    expect().assertFail();
                    done();
                }
            }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
            let Steps = new Array('src', 'prepare', 'play', 'stop', 'reset', 'release');
            let surfaceID = globalThis.value;
            testBufferingUpdate(avPlayer, HLS_PATH, surfaceID, Steps, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_FUNCTION_0600
            * @tc.name      : 006.test hls live - multiBitrate
            * @tc.desc      : Hls live playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_FUNCTION_0600', 0, async function (done) {
            await media.createAVPlayer().then((video) => {
                if (typeof (video) != 'undefined') {
                    avPlayer = video;
                } else {
                    expect().assertFail();
                    done();
                }
            }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
            avPlayer.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AVPlayerTestBase.AV_PLAYER_STATE.INITIALIZED:
                        avPlayer.surfaceId = surfaceID;
                        avPlayer.prepare();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PREPARED:
                        avPlayer.play();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PLAYING:
                        console.info(`case AVPlayerTestBase.AV_PLAYER_STATE.PLAYING`);
                        await mediaTestBase.msleepAsync(8000); // play time 8000ms
                        avPlayer.release();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.RELEASED:
                        console.info(`case AVPlayerTestBase.AV_PLAYER_STATE.RELEASED`);
                        avPlayer = null;
                        done();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.ERROR:
                        console.info(`case AVPlayerTestBase.AV_PLAYER_STATE.ERROR`);
                        expect().assertFail();
                        avPlayer.release().then(() => {
                        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                        break;
                    default:
                        break; 
                }
            });
            avPlayer.on('error', (err) => {
                console.info(`case error called,errName is ${err.name}, case error called,errCode is ${err.code},
                            case error called,errMessage is ${err.message}`);
            });

            avPlayer.on('availableBitrates', (bitrates) => {
                for (let i = 0; i < bitrates.length; i++) { 
                    console.info('case availableBitrates : '  + bitrates[i]);
                }
                checkArray(bitrates, AV_MULTI_HLS_LIVE_BITRATE);
            });
            console.info(`case src is ${MULTI_HLS_PATH}`);
            avPlayer.url = MULTI_HLS_PATH;
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_FUNCTION_0700
            * @tc.name      : 007.test hls live - multi setBitrate
            * @tc.desc      : Hls live playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_FUNCTION_0700', 0, async function (done) {
            await media.createAVPlayer().then((video) => {
                if (typeof (video) != 'undefined') {
                    console.info('case createAVPlayer success');
                    avPlayer = video;
                } else {
                    console.error('case createAVPlayer failed');
                    expect().assertFail();
                    done();
                }
            }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
            let surfaceID = globalThis.value;
            testSetMultiBitrate(avPlayer, MULTI_HLS_PATH, surfaceID, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_RELIABILITY_0100
            * @tc.name      : 008.test hls live - seek
            * @tc.desc      : Hls live playback control test
            * @tc.size      : MediumTest
            * @tc.type      : RELIABILITY test
            * @tc.level     : Level2
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_RELIABILITY_0100', 0, async function (done) {
            await media.createAVPlayer().then((video) => {
                if (typeof (video) != 'undefined') {
                    console.info('case createAVPlayer success');
                    avPlayer = video;
                } else {
                    console.error('case createAVPlayer failed');
                    expect().assertFail();
                    done();
                }
            }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
            let Steps = new Array('src', 'prepare', 'seek', 6, 'error', 'play', 'seek', 0, 'error',
                        'pause', 'seek', 5, 'error', 'stop', 'seek', 1, 'error', 'release'); // 6, 0, 5, 1 is seek target
            testReliability(avPlayer, HLS_PATH, surfaceID, Steps, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_RELIABILITY_0200
            * @tc.name      : 009.test hls live - currentTime
            * @tc.desc      : HLS setBitrate test
            * @tc.size      : MediumTest
            * @tc.type      : RELIABILITY test
            * @tc.level     : Level2
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_RELIABILITY_0200', 0, async function (done) {
            await media.createAVPlayer().then((video) => {
                if (typeof (video) != 'undefined') {
                    console.info('case createAVPlayer success');
                    avPlayer = video;
                } else {
                    console.error('case createAVPlayer failed');
                    expect().assertFail();
                    done();
                }
            }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
            let Steps = new Array('src', 'prepare', 'currentTime', -1, 'play', 'currentTime', -1, 'pause',
                        'currentTime', -1, 'stop', 'currentTime', -1, 'release');
            testReliability(avPlayer, HLS_PATH, surfaceID, Steps, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_RELIABILITY_0300
            * @tc.name      : 010.test hls live - loop
            * @tc.desc      : Hls live playback control test
            * @tc.size      : MediumTest
            * @tc.type      : RELIABILITY test
            * @tc.level     : Level2
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_RELIABILITY_0300', 0, async function (done) {
            await media.createAVPlayer().then((video) => {
                if (typeof (video) != 'undefined') {
                    console.info('case createAVPlayer success');
                    avPlayer = video;
                } else {
                    console.error('case createAVPlayer failed');
                    expect().assertFail();
                    done();
                }
            }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
            let Steps = new Array('src', 'prepare', 'loop', true, 'error', 'play', 'loop', false,
                        'error', 'pause', 'loop', true, 'error', 'stop', 'loop', false, 'error', 'release');
            testReliability(avPlayer, HLS_PATH, surfaceID, Steps, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_RELIABILITY_0400
            * @tc.name      : 011.test hls live - duration
            * @tc.desc      : Hls live playback control test
            * @tc.size      : MediumTest
            * @tc.type      : RELIABILITY test
            * @tc.level     : Level2
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_RELIABILITY_0400', 0, async function (done) {
            await media.createAVPlayer().then((video) => {
                if (typeof (video) != 'undefined') {
                    console.info('case createAVPlayer success');
                    avPlayer = video;
                } else {
                    console.error('case createAVPlayer failed');
                    expect().assertFail();
                    done();
                }
            }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
            let Steps = new Array('src', 'prepare', 'duration', -1, 'play', 'duration', -1, 'pause',
                        'duration', -1, 'stop', 'duration', -1, 'release');
            testReliability(avPlayer, HLS_PATH, surfaceID, Steps, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_RELIABILITY_0500
            * @tc.name      : 012.test hls live - setSpeed
            * @tc.desc      : Hls live reliability control test
            * @tc.size      : MediumTest
            * @tc.type      : Reliability test
            * @tc.level     : Level2
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_RELIABILITY_0500', 0, async function (done) {
            await media.createAVPlayer().then((video) => {
                if (typeof (video) != 'undefined') {
                    console.info('case createAVPlayer success');
                    avPlayer = video;
                } else {
                    console.error('case createAVPlayer failed');
                    expect().assertFail();
                    done();
                }
            }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
            let Steps = new Array('src', 'prepare', 'setSpeed', media.PlaybackSpeed.SPEED_FORWARD_2_00_X, 'error', 'play', 'setSpeed',
                        media.PlaybackSpeed.SPEED_FORWARD_0_75_X, 'error', 
                        'pause', 'setSpeed', media.PlaybackSpeed.SPEED_FORWARD_1_75_X, 'error', 'stop', 'setSpeed', 
                        media.PlaybackSpeed.SPEED_FORWARD_1_00_X, 'error', 'release');
            testReliability(avPlayer, HLS_PATH, surfaceID, Steps, done);
        })

        /* *
        //     * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_RELIABILITY_0600
        //     * @tc.name      : 013.test hls live - bitrate adaptation
        //     * @tc.desc      : Hls live stability test
        //     * @tc.size      : MediumTest
        //     * @tc.type      : Reliability test
        //     * @tc.level     : Level1
        // */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_RELIABILITY_0600', 0, async function (done) {
            let isAdaption = false;
            await media.createAVPlayer().then((video) => {
                if (typeof (video) != 'undefined') {
                    avPlayer = video;
                } else {
                    expect().assertFail();
                    done();
                }
            }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);

            avPlayer.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AVPlayerTestBase.AV_PLAYER_STATE.INITIALIZED:
                        avPlayer.surfaceId = surfaceID;
                        avPlayer.prepare();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PREPARED:
                        avPlayer.play();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PLAYING:
                        await mediaTestBase.msleepAsync(2000); // play time 2000ms, ensure network had changed
                        avPlayer.release();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.RELEASED:
                        avPlayer = null;
                        expect(isAdaption).assertEqual(true);
                        done();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.ERROR:
                        console.info(`case AVPlayerTestBase.AV_PLAYER_STATE.ERROR`);
                        expect().assertFail();
                        avPlayer.release();
                        break;
                    default:
                        break; 
                }
            });
            avPlayer.on('videoSizeChange', (width, height) => {
                console.info('videoSizeChange success,and width is:' + width + ', height is :' + height)
                isAdaption = true;
            })
            avPlayer.on('error', (err) => {
                console.info(`case error called,errName is ${err.name}, case error called,errCode is ${err.code},
                            case error called,errMessage is ${err.message}`);
                avPlayer.release();
                expect().assertFail();
            });
            console.info(`case src is ${HLS_PATH}`);
            avPlayer.url = MULTI_HLS_PATH;
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_STABILITY_0100
            * @tc.name      : 013.test hls live - 1000 times to pause
            * @tc.desc      : Hls live stability test
            * @tc.size      : MediumTest
            * @tc.type      : Stability test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_HLS_Live_STABILITY_0100', 0, async function (done) {
            let pauseTimes = 0;
            await media.createAVPlayer().then((video) => {
                if (typeof (video) != 'undefined') {
                    avPlayer = video;
                } else {
                    expect().assertFail();
                    done();
                }
            }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);

            avPlayer.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AVPlayerTestBase.AV_PLAYER_STATE.INITIALIZED:
                        avPlayer.surfaceId = surfaceID;
                        avPlayer.prepare();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PREPARED:
                        avPlayer.play();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PLAYING:
                        if ((pauseTimes++) < 1000) {
                            avPlayer.pause();
                        } else {
                            avPlayer.release();
                        }
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.PAUSED:
                        console.info(`case AVPlayerTestBase.AV_PLAYER_STATE.PAUSED`);
                        avPlayer.play();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.RELEASED:
                        avPlayer = null;
                        done();
                        break;
                    case AVPlayerTestBase.AV_PLAYER_STATE.ERROR:
                        console.info(`case AVPlayerTestBase.AV_PLAYER_STATE.ERROR`);
                        expect().assertFail();
                        avPlayer.release();
                        break;
                    default:
                        break; 
                }
            });
            avPlayer.on('error', (err) => {
                console.info(`case error called,errName is ${err.name}, case error called,errCode is ${err.code},
                            case error called,errMessage is ${err.message}`);
                avPlayer.release();
                expect().assertFail();
            });
            console.info(`case src is ${HLS_PATH}`);
            avPlayer.url = HLS_PATH;
        })
    })
}
