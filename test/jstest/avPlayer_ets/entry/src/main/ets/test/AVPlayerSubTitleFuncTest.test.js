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

import * as mediaTestBase from '../../../../../../MediaTestBase.js';
import media from '@ohos.multimedia.media'
import audio from '@ohos.multimedia.audio';
import { testAVPlayerFun, AV_PLAYER_STATE, setSource } from '../../../../../../AVPlayerTestBase.js';
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from 'deccjsunit/index';

export default function AVPlayerSubTitleFuncTest() {
    describe('AVPlayerSubTitleFuncTest', function () {
        const SUBTITLE_LIMIT = 8;
        const VIDEO_SOURCE = 'H264_AAC.mp4';
        let fileDescriptor = null;
        let isHls = false;
        let subTitleFdSrc = new Array();
        let subTitleUrlSrc = new Array('http://xxx/utf8_01.srt', 'http://xxx/utf8_02.srt',
                                       'http://xxx/utf8_03.srt', 'http://xxx/utf8_04.srt',
                                       'http://xxx/utf8_05.srt', 'http://xxx/utf8_06.srt',
                                       'http://xxx/utf8_07.srt', 'http://xxx/utf8_08.srt',
                                       'http://xxx/utf8_09.srt');
        let subTitleValue = new Array('AVPlayer: test1 for subtitle_1', 'AVPlayer: test1 for subtitle_2', 'AVPlayer: test1 for subtitle_3',
                                      'AVPlayer: test1 for subtitle_9', 'AVPlayer: test1 for subtitle_5', 'AVPlayer: test1 for subtitle_6',
                                      'AVPlayer: test1 for subtitle_7', 'AVPlayer: test1 for subtitle_8', 'AVPlayer: test1 for subtitle_3',
                                      'AVPlayer: test1 for subtitle_4', 'AVPlayer: test1 for subtitle_5', 'AVPlayer: test1 for subtitle_6', 
                                      'AVPlayer: test1 for subtitle_7', 'AVPlayer: test1 for subtitle_8', 'AVPlayer: test1 for subtitle_10', 
                                      'AVPlayer: test1 for subtitle_6', 'AVPlayer: test1 for subtitle_7', 'AVPlayer: test1 for subtitle_8', 
                                      'AVPlayer: test1 for subtitle_9', 'AVPlayer: test1 for subtitle_10');
        beforeAll(async function() {
            console.info('beforeAll case');
            await mediaTestBase.getFileDescriptor(VIDEO_SOURCE).then((res) => {
                fileDescriptor = res;
            });
            openSubTitle();
        })

        beforeEach(async function() {
            console.info('beforeEach case');
        })

        afterEach(async function() {
            console.info('afterEach case');
        })

        afterAll(async function() {
            console.info('afterAll case');
        })

        async function openSubTitle() {
            await mediaTestBase.getFileDescriptor('utf8_01.srt').then((res) => {
                subTitleFdSrc.push(res);
            });
            await mediaTestBase.getFileDescriptor('utf8_02.srt').then((res) => {
                subTitleFdSrc.push(res);
            });
            await mediaTestBase.getFileDescriptor('utf8_03.srt').then((res) => {
                subTitleFdSrc.push(res);
            });
            await mediaTestBase.getFileDescriptor('utf8_04.srt').then((res) => {
                subTitleFdSrc.push(res);
            });
            await mediaTestBase.getFileDescriptor('utf8_05.srt').then((res) => {
                subTitleFdSrc.push(res);
            });
            await mediaTestBase.getFileDescriptor('utf8_06.srt').then((res) => {
                subTitleFdSrc.push(res);
            });
            await mediaTestBase.getFileDescriptor('utf8_07.srt').then((res) => {
                subTitleFdSrc.push(res);
            });
            await mediaTestBase.getFileDescriptor('utf8_08.srt').then((res) => {
                subTitleFdSrc.push(res);
            });
            await mediaTestBase.getFileDescriptor('utf8_09.srt').then((res) => {
                subTitleFdSrc.push(res);
            });
        }

        // testcast for add subtitle in prepared/playing/paused/completed state
        async function addSubTitle(avPlayer, mode, subTitleFile) {
            if (mode === 'local') {
                console.info('case this is local subtitle');
                avPlayer.addSubtitleFdSrc(subTitleFile);
            } else {
                console.info('case this is network subtitle');
                avPlayer.addSubtitleUrl(subTitleFile);
            }
        }

        // testcase for add subtitile
        async function testAddSubTitle(avPlayer, mode, subTitleSrc, done) {
            let surfaceID = globalThis.value;
            let count = 0;
            let isError = 0;
            avPlayer.on('error', (error) => {
                console.error('case error hanppened: error message is:' + error.message);
                if (isError == 1) {
                    console.info('case add subtitle in error state');
                    isError = 0;
                } else  if (isError == 2) {
                    console.info('case add subtitle out of limit');
                    isError = 0;
                    avPlayer.release();
                } else {
                    console.error('case has unknown error hanppened');
                    expect().assertFail();
                }
            })
            avPlayer.on('trackInfoUpdate', (trackInfo) => {
                console.info('case trackInfoUpdate called, length is:' + trackInfo.length);
                for (let i = 0; i < trackInfo.length; i++) {
                    mediaTestBase.printDescription(trackInfo[i]);
                }
            })

            avPlayer.on('subtitleTextUpdate', (textInfo) => {
                console.info('case subtitleTextUpdate is: ' + textInfo.text);
            })

            avPlayer.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AV_PLAYER_STATE.INITIALIZED:
                        console.info(`case AV_PLAYER_STATE.INITIALIZED`);
                        avPlayer.surfaceId = surfaceID;
                        expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.INITIALIZED);
                        isError = 1;
                        addSubTitle(avPlayer, mode, subTitleSrc[count]);
                        avPlayer.prepare((err) => {
                            console.info('case prepare called' + err);
                            if (err != null) {
                                console.error(`case prepare error, errMessage is ${err.message}`);
                                expect().assertFail();
                                done();
                            } else {
                                console.info('case avPlayer.duration: ' + avPlayer.duration);
                            }
                        });
                        break;     
                    case AV_PLAYER_STATE.PREPARED:
                        console.info('case start add first subtitle');
                        addSubTitle(avPlayer, mode, subTitleSrc[count]);
                        count++;
                        console.info('case start getTrackDescription');
                        let arrayDescription;
                        await avPlayer.getTrackDescription().then((arrayList) => {
                            console.info('case getTrackDescription called!!');
                            if (typeof (arrayList) != 'undefined') {
                                arrayDescription = arrayList;
                            } else {
                                console.info('case getTrackDescription is failed');
                                expect().assertFail();
                            }
                        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                        for (let i = 0; i < arrayDescription.length; i++) {
                            mediaTestBase.printDescription(arrayDescription[i]);
                        }
                        avPlayer.play();
                        break;
                    case AV_PLAYER_STATE.PLAYING:
                        if (count <= 1) {
                            console.info('case start add second subtitle');
                            addSubTitle(avPlayer, mode, subTitleSrc[count]);
                            count++;
                            avPlayer.pause();
                        }
                        break;
                    case AV_PLAYER_STATE.PAUSED:
                        console.info('case start add third subtitle');
                        addSubTitle(avPlayer, mode, subTitleSrc[count]);
                        count++;
                        avPlayer.play();
                        break;
                    case AV_PLAYER_STATE.COMPLETED:
                        console.info('case start add fourth subtitle');
                        addSubTitle(avPlayer, mode, subTitleSrc[count]);
                        console.info('case start add out of limit subtitle');
                        isError = 2;
                        // 添加字幕超过8个，报错
                        for (let i = 4; i < subTitleSrc.length; i++) {
                            console.info('case add limit subtitle: ' + i);
                            addSubTitle(avPlayer, mode, subTitleSrc[i]);
                        }
                        break;
                    case AV_PLAYER_STATE.RELEASED:
                        done();
                        break;
                    case AV_PLAYER_STATE.ERROR:
                        expect().assertFail();
                        avPlayer.release().then(() => {
                        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                        avPlayer = null;
                        break;
                    default:
                        break; 
                }
            })
            avPlayer.fdSrc = fileDescriptor;
        }

        // testcase for diplay subtitle in all state
        async function testPlaySubTitle(avPlayer, mode, subTitleSrc, url, done) {
            console.info('case testPlaySubTitle mode is: ' + mode + ', src is:' + subTitleSrc);
            let surfaceID = globalThis.value;
            let count = 0;
            let testArray = new Array();
            avPlayer.on('trackInfoUpdate', (trackInfo) => {
                console.info('case trackInfoUpdate called');
            });

            avPlayer.on('seekDone', async (seekDoneTime) => {
                console.info('case seek success,and seekDoneTime is:' + seekDoneTime);
            });

            avPlayer.on('timeUpdate', (time) => {
                console.info('case timeUpdate callback, time:' + time);
            });

            avPlayer.on('error', (error) => {
                console.info('case error callback, error is:' + error.message);
                expect().assertFail();
                avPlayer.release();
            });

            avPlayer.on('subtitleTextUpdate', (textInfo) => {
                console.info('case subtitleTextUpdate is: ' + textInfo.text);
                if (textInfo.text.length > 0) {
                    console.info('case add this subtitle');
                    testArray.push(textInfo.text);
                }
            });

            if (isHls) {
                avPlayer.on('availableBitrates', (bitrates) => { 
                    Logger.info(TAG, 'availableBitrates success,and availableBitrates length is:' + bitrates.length);
                });
            }

            avPlayer.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AV_PLAYER_STATE.INITIALIZED:
                        console.info(`case AV_PLAYER_STATE.INITIALIZED`);
                        avPlayer.surfaceId = surfaceID;
                        expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.INITIALIZED);
                        avPlayer.prepare((err) => {
                            console.info('case prepare called' + err);
                            if (err != null) {
                                console.error(`case prepare error, errMessage is ${err.message}`);
                                expect().assertFail();
                                done();
                            } else {
                                console.info('case avPlayer.duration: ' + avPlayer.duration);
                            }
                        });
                        break;
                    case AV_PLAYER_STATE.PREPARED:
                        console.info(`case AV_PLAYER_STATE.PREPARED`);
                        if (count === 0) {
                            addSubTitle(avPlayer, mode, subTitleSrc);
                            count++;
                            avPlayer.play();
                        } else if (count === 3) {
                            count++;
                            avPlayer.play();
                        } else if (count === 5) {
                            avPlayer.play();
                        }
                        break;
                    case AV_PLAYER_STATE.PLAYING:
                        console.info(`case AV_PLAYER_STATE.PLAYING`);
                        if (count === 1) {
                            // time 0 ~ 2500,字幕新增 1、2、3
                            await mediaTestBase.msleepAsync(2500);
                            count++;
                            avPlayer.pause();
                        } else if (count === 2) {
                            // time 5100 ~ 7600,字幕新增7、8
                            await mediaTestBase.msleepAsync(3500);
                            count++;
                            avPlayer.stop();
                        } else if(count === 4) {
                            // 无字幕新增
                            await mediaTestBase.msleepAsync(2500);
                            // 设置两倍速
                            avPlayer.setSpeed(media.PlaybackSpeed.SPEED_FORWARD_2_00_X);
                            // 再次设置字幕轨道
                            addSubTitle(avPlayer, mode, subTitleSrc);
                            // time 2500 ~ 7500,新增字幕 3、4、5、6、7、8
                            await mediaTestBase.msleepAsync(3000);
                            avPlayer.seek(avPlayer.duration, 0); // seek duration,新增字幕10
                        } else if (count === 5) {
                            avPlayer.setSpeed(media.PlaybackSpeed.SPEED_FORWARD_1_00_X);
                            await mediaTestBase.msleepAsync(5500);
                            addSubTitle(avPlayer, mode, subTitleSrc);
                        }
                        break;
                    case AV_PLAYER_STATE.PAUSED:
                        console.info(`case AV_PLAYER_STATE.PAUSED`);
                        // pause 2 s,字幕无新增
                        await mediaTestBase.msleepAsync(2000);
                        avPlayer.seek(5000, 0); // seek next mode,字幕新增9
                        await mediaTestBase.msleepAsync(1000);
                        avPlayer.seek(5000); //  seek prev mode，字幕新增5
                        await mediaTestBase.msleepAsync(1500);
                        avPlayer.play();
                        break;
                    case AV_PLAYER_STATE.STOPPED:
                        console.info(`case AV_PLAYER_STATE.STOPPED`);
                        await mediaTestBase.msleepAsync(2000);
                        avPlayer.reset();
                        break;
                    case AV_PLAYER_STATE.IDLE:
                        console.info(`case AV_PLAYER_STATE.IDLE`);
                        avPlayer.fdSrc = fileDescriptor;
                        break;
                    case AV_PLAYER_STATE.COMPLETED:
                        console.info(`case AV_PLAYER_STATE.COMPLETED`);
                        // 无字幕新增
                        if (count === 4) {
                            count++;
                            avPlayer.reset();
                        } else {
                            await mediaTestBase.msleepAsync(2000);
                            avPlayer.release();
                        }
                        break;
                    case AV_PLAYER_STATE.RELEASED:
                        console.info(`case AV_PLAYER_STATE.RELEASED`);
                        console.info('case testArray length is: ' + testArray.length + ': subTitle is: \n');
                        for (let i = 0; i < testArray.length; i++) {
                            console.info('actuallity value is: ' + testArray[i] + '\n');
                            console.info('hope value is: ' + subTitleValue[i] + '\n');
                            expect(testArray[i]).assertEqual(subTitleValue[i]);
                        }
                        done();
                        break;
                    case AV_PLAYER_STATE.ERROR:
                        expect().assertFail();
                        avPlayer.release().then(() => {
                        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                        avPlayer = null;
                        break;
                    default:
                        break; 
                }
            })
            if (typeof(url) == 'string') {
                avPlayer.url = url;
            } else {
                avPlayer.fdSrc = url;
            }
        }

        // testcase for error format subtitle
        async function testErrorFormat(avPlayer, subTitleSrc, url, done) {
            let mode = 'local';
            avPlayer.on('trackInfoUpdate', (trackInfo) => {
                console.info('case trackInfoUpdate called');
                expect().assertFail();
                avPlayer.release();
            });

            avPlayer.on('error', (error) => {
                console.info('case error callback, error is:' + error.message);
                avPlayer.release();
            });

            avPlayer.on('subtitleTextUpdate', (textInfo) => {
                console.info('case subtitleTextUpdate is: ' + textInfo.text);
                expect().assertFail();
                avPlayer.release();
            });

            avPlayer.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AV_PLAYER_STATE.INITIALIZED:
                        console.info(`case AV_PLAYER_STATE.INITIALIZED`);
                        avPlayer.prepare();
                        break;
                    case AV_PLAYER_STATE.PREPARED:
                        console.info(`case AV_PLAYER_STATE.PREPARED`);
                        addSubTitle(avPlayer, mode, subTitleSrc);
                        break;
                    case AV_PLAYER_STATE.RELEASED:
                        console.info(`case AV_PLAYER_STATE.RELEASED`);
                        done();
                        break;
                    case AV_PLAYER_STATE.ERROR:
                        break;
                    default:
                        break; 
                }
            })
            if (typeof(url) == 'string') {
                mode = 'network';
                avPlayer.url = url;
            } else {
                mode = 'local';
                avPlayer.fdSrc = url;
            }
        }

        // testcase for select subtitle
        async function testSelectSubtitle(avPlayer, subTitleSrc, url, done) {
            let surfaceID = globalThis.value;
            let count = 0;
            let index = 2;
            let playCount = 0;
            let isError = 0;
            let testArray = new Array();
            avPlayer.on('trackInfoUpdate', (trackInfo) => {
                console.info('case trackInfoUpdate called, trackInfo length is:' + trackInfo.length);
                count++;
            });

            avPlayer.on('seekDone', async (seekDoneTime) => {
                console.info('case seek success,and seekDoneTime is:' + seekDoneTime);
            });

            avPlayer.on('timeUpdate', (time) => {
                console.info('case timeUpdate callback, time:' + time);
            });

            avPlayer.on('error', (error) => {
                console.info('case error callback, error is:' + error.message);
                if (isError == 1) {
                    console.info('case this error is select subtitle in stop state');
                    avPlayer.reset();
                } else if (isError == 2) {
                    console.info('case this error is select subtitle in idle state');
                    avPlayer.release();
                } else {
                    console.info('case this is an unkonw error');
                    expect().assertFail();
                    avPlayer.release();
                }
                isError = 0;
            });

            avPlayer.on('subtitleTextUpdate', (textInfo) => {
                console.info('case subtitleTextUpdate is: ' + textInfo.text);
                if (textInfo.test !== '') {
                    testArray.push(textInfo.text);
                }
            });

            if (isHls) {
                avPlayer.on('availableBitrates', (bitrates) => { 
                    Logger.info(TAG, 'availableBitrates success,and availableBitrates length is:' + bitrates.length);
                });
            }

            avPlayer.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AV_PLAYER_STATE.INITIALIZED:
                        console.info(`case AV_PLAYER_STATE.INITIALIZED`);
                        avPlayer.surfaceId = surfaceID;
                        expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.INITIALIZED);
                        avPlayer.prepare((err) => {
                            console.info('case prepare called' + err);
                            if (err != null) {
                                console.error(`case prepare error, errMessage is ${err.message}`);
                                expect().assertFail();
                                done();
                            } else {
                                console.info('case avPlayer.duration: ' + avPlayer.duration);
                            }
                        });
                        break;
                    case AV_PLAYER_STATE.PREPARED:
                        console.info(`case AV_PLAYER_STATE.PREPARED`);
                        // add subtitles
                        for (let i = 0; i < SUBTITLE_LIMIT; i++) {
                            addSubTitle(avPlayer, 'local', subTitleSrc[i]);
                        }
                        await mediaTestBase.msleepAsync(2000);
                        console.info('case subtitle count is:' + count);
                        expect(count).assertEqual(SUBTITLE_LIMIT);
                        // select subtitle
                        // avPlayer.selectTrack(index);
                        index++;
                        avPlayer.play();
                        break;
                    case AV_PLAYER_STATE.PLAYING:
                        console.info(`case AV_PLAYER_STATE.PLAYING`);
                        await mediaTestBase.msleepAsync(2500);
                        avPlayer.selectTrack(index);
                        index++
                        await mediaTestBase.msleepAsync(2000);
                        if (playCount == 0) {
                            avPlayer.pause();
                        } else if (playCount == 1) {
                            console.info('cast wait for play completed');
                        } else if (playCount == 2) {
                            await mediaTestBase.msleepAsync(2500);
                            avPlayer.stop();
                        }
                        break;
                    case AV_PLAYER_STATE.PAUSED:
                        console.info(`case AV_PLAYER_STATE.PAUSED 11`);
                        avPlayer.selectTrack(index);
                        index++
                        playCount++;
                        await mediaTestBase.msleepAsync(2000);
                        avPlayer.play();
                        break;
                    case AV_PLAYER_STATE.STOPPED:
                        console.info(`case AV_PLAYER_STATE.STOPPED`);
                        isError = 1;
                        avPlayer.selectTrack(index);
                        break;
                    case AV_PLAYER_STATE.IDLE:
                        console.info(`case AV_PLAYER_STATE.IDLE`);
                        isError = 2;
                        avPlayer.selectTrack(index);
                        break;
                    case AV_PLAYER_STATE.COMPLETED:
                        console.info(`case AV_PLAYER_STATE.COMPLETED`);
                        avPlayer.selectTrack(index);
                        index++
                        playCount++;
                        console.info('case start to play');
                        avPlayer.play();
                        break;
                    case AV_PLAYER_STATE.RELEASED:
                        console.info(`case AV_PLAYER_STATE.RELEASED`);
                        done();
                        break;
                    case AV_PLAYER_STATE.ERROR:
                        expect().assertFail();
                        avPlayer.release().then(() => {
                        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                        avPlayer = null;
                        break;
                    default:
                        break; 
                }
            })
            if (typeof(url) == 'string') {
                avPlayer.url = url;
            } else {
                avPlayer.fdSrc = url;
            }
        }

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_SUBTITLE_0100
            * @tc.name      : 001.test add local subtitle in prepared/playing/paused/completed state
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_SUBTITLE_0100', 0, async function (done) {
            let avPlayer = await media.createAVPlayer();
            testAddSubTitle(avPlayer, 'local', subTitleFdSrc, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_SUBTITLE_0150
            * @tc.name      : 002.test add network(http/https、HLS) subtitle in prepared/playing/paused/completed state
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_SUBTITLE_0150', 0, async function (done) {
            let avPlayer = await media.createAVPlayer();
            testAddSubTitle(avPlayer, 'network', subTitleUrlSrc, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_SUBTITLE_0200
            * @tc.name      : 003.test subtitle display in all playback in local
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_SUBTITLE_0200', 0, async function (done) {
            let avPlayer = await media.createAVPlayer();
            testPlaySubTitle(avPlayer, 'local', subTitleFdSrc[0], fileDescriptor, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_SUBTITLE_0220
            * @tc.name      : 004.test subtitle display in all playback in network
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_SUBTITLE_0220', 0, async function (done) {
            let avPlayer = await media.createAVPlayer();
            testPlaySubTitle(avPlayer, 'network', subTitleUrlSrc[0], fileDescriptor, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_SUBTITLE_0240
            * @tc.name      : 005.test subtitle display in all playback in HLS video
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_SUBTITLE_0240', 0, async function (done) {
            let avPlayer = await media.createAVPlayer();
            testPlaySubTitle(avPlayer, 'network', subTitleUrlSrc[0], 'http://xxx/index.m3u8', done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_SUBTITLE_0300
            * @tc.name      : 006.test subtitle with ass source
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_SUBTITLE_0300', 0, async function (done) {
            let avPlayer = await media.createAVPlayer();
            testErrorFormat(avPlayer, fileDescriptor, 'http://xxx/testAss.ass', done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_SUBTITLE_0350
            * @tc.name      : 006.test subtitle with ass source
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_SUBTITLE_0350', 0, async function (done) {
            let avPlayer = await media.createAVPlayer();
            let assFile;
            await mediaTestBase.getFileDescriptor('testAss.ass').then((res) => {
                assFile = res;
            });
            testErrorFormat(avPlayer, fileDescriptor, assFile, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_SUBTITLE_0400
            * @tc.name      : 007.test subtitle with select subtitle (local)
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_SUBTITLE_0400', 0, async function (done) {
            let avPlayer = await media.createAVPlayer();
            testSelectSubtitle(avPlayer, subTitleFdSrc, fileDescriptor, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_SUBTITLE_0450
            * @tc.name      : 008.test subtitle with select subtitle (network)
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_SUBTITLE_0450', 0, async function (done) {
            let avPlayer = await media.createAVPlayer();
            testSelectSubtitle(avPlayer, subTitleFdSrc, 'http://xxx/index.m3u8', done);
        })
    }) 
} 