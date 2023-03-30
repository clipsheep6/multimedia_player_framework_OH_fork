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
import { testAVPlayerFun, AV_PLAYER_STATE } from '../../../../../../AVPlayerTestBase.js';
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from 'deccjsunit/index';

export default function AVPlayerLocalAPITest() {
    describe('AVPlayerLocalAPITest', function () {
        const VIDEO_SOURCE = 'H264_AAC.mp4';
        const AUDIO_SOURCE = '01.mp3';
        const VIDEO_NOAUDIO = 'H264_NONE.mp4'
        const ERROR_VIDEO = 'error.mp4'
        const TAG = 'AVPlayerLocalAPITest:';
        const CREATE_EVENT = 'create';
        const SETFDSRC_EVENT = 'setfdSrc';
        const SETSOURCE_EVENT = 'setSource';
        const SETSURFACE_EVENT = 'setDisplaySurface';
        const GETDESCRIPTION_PROMISE = 'getTrackDescriptionPromise';
        const GETDESCRIPTION_CALLBACK = 'getTrackDescriptionCallback';
        const PREPARE_EVENT = 'prepare';
        const PLAY_EVENT = 'play';
        const PAUSE_EVENT = 'pause';
        const STOP_EVENT = 'stop';
        const RESET_EVENT = 'reset';
        const RELEASE_EVENT = 'release';
        const SEEK_EVENT = 'seek';
        const SEEK_FUNCTION_EVENT = 'seek_function';
        const SPEED_FUNCTION_EVENT = 'speed_function';
        const PLAY_FUNCTION_EVENT = 'play_function';
        const SETVOLUME_EVENT = 'volume';
        const SETSPEED_EVENT = 'speed';
        const FINISH_EVENT = 'finish';
        const ERROR_EVENT = 'error';
        const LOOP_EVENT = 'loop';
        const CHECKLOOP_EVENT = 'check_loop'
        const CHECKINTERRUPT_EVENT = 'check_interrupt'
        const END_EVENT = 'end';
        const SCALETYPE_EVENT = 'scaletype'
        const INTERRUPTMODE_EVENT = 'audioInterruptMode'
        const AUDIORENDERINFO_EVENT = 'audioRenderInfo'
        const CHECK_PARAMETER = 'checkParameter'
        const DELTA_TIME = 1000;
        let PLAY_TIME = 1000;
        let seekTime = 0;
        let videoNameList = new Array(VIDEO_SOURCE, AUDIO_SOURCE, VIDEO_NOAUDIO);
        let stepCount = 0;
        let testTag = 'test';
        let events = require('events');
        let eventEmitter = new events.EventEmitter();
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
        let fdPath02 = '';
        let fdNumber02 = 0;
        let errorFdPath = '';
        let errorFdNumber = 0;
        let fdHead = 'fd://';
        let surfaceID = globalThis.value;
        let testCount = 0;
        let myStepsList;
        let videoList;
        let testFdSource;
        let videoCount = 0;
        let duration = 0;
        let durationTag = -100;
        let halfDurationTag = -50;
        let endEOS = 0;
        let completedValue = 0;
        let videoInfor = new Array({
            type : 'video_audio',
            width: 720,
            height: 480,
            duration: 10034,
            state: '',
            PREV_FRAME: 4166,
            NEXT_FRAME: 8333,
        }, {
            type : 'audio',
            width: 0,
            height: 0,
            duration: 219600,
            state: '',
            PREV_FRAME: 0,
            NEXT_FRAME: 0,
        }, {
            type : 'video',
            width: 720,
            height: 480,
            duration: 10034,
            state: '',
            PREV_FRAME: 5000,
            NEXT_FRAME: 10001,
        });

        let descriptionKey01 = new Array(['bitrate', 'codec_mime', 'frame_rate', 'height',
        'track_index', 'track_type', 'width'], ['bitrate', 'channel_count', 'codec_mime', 'sample_rate',
        'track_index', 'track_type']);
        let descriptionValue01 = new Array([1366541, 0, 6000, 480, 0, 1, 720], [129207, 2, 1, 44100, 1, 0]);
        let descriptionKey02 = new Array(['channel_count', 'codec_mime', 'sample_rate', 'track_index', 'track_type']);
        let descriptionValue02 = new Array([1, 2, 48000, 0, 0]);
        let descriptionKey03 = new Array(['bitrate', 'codec_mime', 'frame_rate', 'height', 'track_index', 'track_type', 'width']);
        let descriptionValue03 = new Array([1506121, 0, 6000, 480, 0, 1, 720]);
        let descriptionKey = new Array(descriptionKey01, descriptionKey02, descriptionKey03);
        let descriptionValue = new Array(descriptionValue01, descriptionValue02, descriptionValue03);

        beforeAll(async function () {
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
            await mediaTestBase.getFdRead(VIDEO_SOURCE, openFileFailed).then((testNumber) => {
                fdNumber = testNumber;
                fdPath = fdHead + '' + fdNumber;
            })
            await mediaTestBase.getFdRead(AUDIO_SOURCE, openFileFailed).then((testNumber) => {
                fdNumber02 = testNumber;
                fdPath02 = fdHead + '' + fdNumber02;
            })
            await mediaTestBase.getFdRead(ERROR_VIDEO, openFileFailed).then((testNumber) => {
                errorFdNumber = testNumber;
                errorFdPath = fdHead + '' + errorFdNumber;
            })
            videoList = new Array(fileDescriptor, fileDescriptor2, fileDescriptor3);
            testFdSource = videoList[0];
        })

        beforeEach(async function () {
            testFdSource = videoList[0];
            videoCount = 0;
            testCount = 0;
            stepCount = 0;
            console.info('beforeEach case');
        })

        afterEach(async function () {
            if (avPlayer != null) {
                avPlayer.release().then(() => {
                }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
            }
            avPlayer = null;
            console.info('afterEach case');
        })

        function openFileFailed() {
            console.info('case file fail');
        }

        afterAll(async function () {
            console.info('afterAll case');
            await mediaTestBase.closeFileDescriptor(VIDEO_SOURCE);
            await mediaTestBase.closeFileDescriptor(AUDIO_SOURCE);
            await mediaTestBase.closeFileDescriptor(VIDEO_NOAUDIO);
            await mediaTestBase.closeFdNumber(fdNumber);
            await mediaTestBase.closeFdNumber(fdNumber02);
            await mediaTestBase.closeFdNumber(errorFdNumber);
        })

        async function setOtherCallback(avPlayer, steps, done) {
            avPlayer.on('error', (error) => {
                console.error(TAG + 'error happened,name is :' + error.name)
                console.error(TAG + 'error happened,code is :' + error.code)
                console.error(TAG + 'error happened,message is :' + error.message)
                if (steps[stepCount] == ERROR_EVENT) {
                    stepCount++;
                    toNextStep(avPlayer, steps, done);
                } else {
                    expect().assertFail();
                    steps[stepCount] = END_EVENT;
                    avPlayer.release((err) => {
                        if (err == undefined) {
                            console.info(TAG + 'error happened, release success');
                        } else {
                            console.info(TAG + 'error happened, release failed,message is :' + err.message);
                        }
                    })
                }
            })
            // 时间戳变化上报函数
            avPlayer.on('timeUpdate', (time) => {
                console.info(TAG + 'timeUpdate called: time is :' + time)
            })
            // 音量变化上报函数
            avPlayer.on('volumeChange', (vol) => {
                console.info(TAG + 'volumeChange success,and new volume is :' + vol)
                toNextStep(avPlayer, steps, done);
            })
            // 视频播放结束触发回调
            avPlayer.on('endOfStream', () => {
                console.info(TAG + 'endOfStream success')
                endEOS++;
                toNextStep(avPlayer, steps, done);
            })
            // seek操作回调函数
            avPlayer.on('seekDone', (seekDoneTime) => {
                console.info(TAG + 'seekDone success,and seekDoneTime time is:' + seekDoneTime)
                console.info(TAG + 'seekDone success,and test duration time is:' + duration)
                console.info(TAG + 'seekDone success,and test steps time is:' + seekTime)
                expect(seekDoneTime).assertEqual(seekTime == durationTag ? duration : seekTime);
                toNextStep(avPlayer, steps, done);
            })
            // 设置倍速播放回调函数
            avPlayer.on('speedDone', (speed) => {
                console.info(TAG + 'speedDone success,and speed value is:' + speed)
                expect(speed).assertEqual(steps[stepCount]);
                stepCount++;
                toNextStep(avPlayer, steps, done);
            })
            // bitrate设置成功回调函数
            avPlayer.on('bitrateDone', (bitrate) => {
                console.info(TAG + 'bitrateDone success,and bitrate value is:' + bitrate)
            })
            // 缓冲上报回调函数
            avPlayer.on('bufferingUpdate', (infoType, value) => {
                console.info(TAG + 'bufferingUpdate success,and infoType value is:' + infoType + ', value is :' + value)
            })
            // duration 变化函数上报
            avPlayer.on('durationUpdate', (duration) => {
                console.info(TAG + 'durationUpdate called,duration:' + duration)
            })
            // 首帧上报回调函数
            avPlayer.on('startRenderFrame', () => {
                console.info(TAG + 'startRenderFrame success')
            })
            // 视频宽高上报回调函数
            avPlayer.on('videoSizeChange', (width, height) => {
                console.info(TAG + 'videoSizeChange success,and width is:' + width + ', height is :' + height)
            })
            // 焦点上报回调函数
            avPlayer.on('audioInterrupt', (info) => {
                console.info(TAG + 'audioInterrupt success,and InterruptEvent info is:' + info)
            })
            // HLS上报所有支持的比特率
            avPlayer.on('availableBitrates', (bitrates) => {
                console.info(TAG + 'availableBitrates success,and availableBitrates length is:' + bitrates.length)
            })
        }

        // 状态机变化回调函数
        async function setStateChangeCallback(avPlayer, steps, done) {
            avPlayer.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AV_PLAYER_STATE.IDLE:
                        console.info(TAG + 'state idle called')
                        break;
                    case AV_PLAYER_STATE.INITIALIZED:
                        console.info(TAG + 'state initialized called ')
                        toNextStep(avPlayer, steps, done);
                        break;
                    case AV_PLAYER_STATE.PREPARED:
                        console.info(TAG + 'state prepared called')
                        break;
                    case AV_PLAYER_STATE.PLAYING:
                        console.info(TAG + 'state playing called')
                        break;
                    case AV_PLAYER_STATE.PAUSED:
                        console.info(TAG + 'state paused called')
                        break;
                    case AV_PLAYER_STATE.COMPLETED:
                        completedValue++;
                        console.info(TAG + 'state completed called')
                        break;
                    case AV_PLAYER_STATE.STOPPED:
                        console.info(TAG + 'state stopped called')
                        break;
                    case AV_PLAYER_STATE.RELEASED:
                        console.info(TAG + 'state released called')
                        break;
                    case AV_PLAYER_STATE.ERROR:
                        console.info(TAG + 'state error called')
                        break;
                    default:
                        console.info(TAG + 'unkown state :' + state)
                        break;
                }
            })
        }

        // 释放数组内存
        function deleteArrayList() {
            for (let i = 0; i < myStepsList.length; i++) {
                myStepsList[i].length = 0;
            }
            myStepsList.length = 0;
        }

        // 执行下一步操作函数
        async function toNextStep(avPlayer, steps, done) {
            console.info(TAG + 'next step is :' + steps[stepCount]);
            if (steps[stepCount] == END_EVENT) {
                avPlayer = null;
                if ((testCount == myStepsList.length)) {
                    if (videoCount == (videoList.length - 1)) {
                        console.info(TAG + 'case success!!');
                        deleteArrayList();
                        done();
                    } else {
                        stepCount = 0;
                        videoCount++;
                        testFdSource = videoList[videoCount];
                        testCount = 0;
                        eventEmitter.emit(testTag);
                    }
                } else {
                    stepCount = 0;
                    console.info(TAG + 'start next step,and length is:' + testCount)
                    eventEmitter.emit(testTag);
                }
            } else {
                eventEmitter.emit(steps[stepCount], avPlayer, steps, done);
            }
        }

        // 触发create事件,创建avplayer实例对象
        eventEmitter.on(CREATE_EVENT, (avPlayer, steps, done) => {
            stepCount++;
            media.createAVPlayer((err, video) => {
                if (video != null) {
                    avPlayer = video;
                    // 设置状态机回调函数
                    setStateChangeCallback(avPlayer, steps, done);
                    setOtherCallback(avPlayer, steps, done);
                    expect(avPlayer.state).assertEqual('idle');
                    console.info(TAG + 'createVideoPlayer success!!');
                    toNextStep(avPlayer, steps, done);
                } else if ((err != null) && (steps[stepCount] == ERROR_EVENT)) {
                    stepCount++;
                    toNextStep(avPlayer, steps, done);
                } else {
                    mediaTestBase.printError(err, done);
                }
            });
        });

        // 设置播放源，即avplayer.url的值
        eventEmitter.on(SETSOURCE_EVENT, (avPlayer, steps, done) => {
            stepCount++;
            if (typeof (steps[stepCount]) == 'string') {
                console.error('case url test');
                avPlayer.url = steps[stepCount];
            } else {
                console.error('case fdsrc test');
                avPlayer.fdSrc = steps[stepCount];
            }
            stepCount++;
        });

        // 设置播放源，即avplayer.fdSrc的值
        eventEmitter.on(SETFDSRC_EVENT, (avPlayer, steps, done) => {
            console.info(TAG + 'test setFdSrc: fs id:' + testFdSource.fd);
            console.info(TAG + 'test setFdSrc: fs offset:' + testFdSource.offset);
            console.info(TAG + 'test setFdSrc: fs length:' + testFdSource.length);
            stepCount++;
            avPlayer.fdSrc = testFdSource;
        });

        // 设置surfaceID
        eventEmitter.on(SETSURFACE_EVENT, (avPlayer, steps, done) => {
            stepCount++;
            avPlayer.surfaceId = steps[stepCount];
            stepCount++;
            toNextStep(avPlayer, steps, done);
        });

        // prepare 操作
        eventEmitter.on(PREPARE_EVENT, (avPlayer, steps, done) => {
            stepCount++;
            avPlayer.prepare((err) => {
                if (err == null) {
                    console.info(TAG + 'prepare success!!');
                    duration = avPlayer.duration;
                    toNextStep(avPlayer, steps, done);
                } else if ((err != null) && (steps[stepCount] == ERROR_EVENT)) {
                    stepCount++;
                    console.info(TAG + 'prepare error happened,and message is :' + err.message);
                    toNextStep(avPlayer, steps, done);
                } else {
                    mediaTestBase.printError(err, done);
                }
            });
        });

        // play 操作
        eventEmitter.on(PLAY_EVENT, (avPlayer, steps, done) => {
            stepCount++;
            avPlayer.play(async (err) => {
                if (err == null) {
                    console.info(TAG + 'play success!!');
                    await mediaTestBase.msleepAsync(PLAY_TIME);
                    toNextStep(avPlayer, steps, done);
                } else if ((err != null) && (steps[stepCount] == ERROR_EVENT)) {
                    stepCount++;
                    console.info(TAG + 'play error happened,and message is :' + err.message);
                    toNextStep(avPlayer, steps, done);
                } else {
                    mediaTestBase.printError(err, done);
                }
            });
        });

        // pause 操作
        eventEmitter.on(PAUSE_EVENT, (avPlayer, steps, done) => {
            stepCount++;
            avPlayer.pause(async (err) => {
                if (err == null) {
                    console.info(TAG + 'pause success!!');
                    toNextStep(avPlayer, steps, done);
                } else if ((err != null) && (steps[stepCount] == ERROR_EVENT)) {
                    stepCount++;
                    console.info(TAG + 'pause error happened,and message is :' + err.message);
                    toNextStep(avPlayer, steps, done);
                } else {
                    mediaTestBase.printError(err, done);
                }
            });
        });

        // stop 操作
        eventEmitter.on(STOP_EVENT, (avPlayer, steps, done) => {
            stepCount++;
            avPlayer.stop(async (err) => {
                if (err == null) {
                    console.info(TAG + 'stop success!!');
                    toNextStep(avPlayer, steps, done);
                } else if ((err != null) && (steps[stepCount] == ERROR_EVENT)) {
                    stepCount++;
                    console.info(TAG + 'stop error happened,and message is :' + err.message);
                    toNextStep(avPlayer, steps, done);
                } else {
                    mediaTestBase.printError(err, done);
                }
            });
        });

        // reset 操作
        eventEmitter.on(RESET_EVENT, (avPlayer, steps, done) => {
            stepCount++;
            avPlayer.reset(async (err) => {
                if (err == null) {
                    console.info(TAG + 'reset success!!');
                    toNextStep(avPlayer, steps, done);
                } else if ((err != null) && (steps[stepCount] == ERROR_EVENT)) {
                    stepCount++;
                    console.info(TAG + 'reset error happened,and message is :' + err.message);
                    toNextStep(avPlayer, steps, done);
                } else {
                    mediaTestBase.printError(err, done);
                }
            });
        });

        // seek 操作
        eventEmitter.on(SEEK_EVENT, (avPlayer, steps, done) => {
            seekTime = steps[stepCount + 1];
            if (seekTime == durationTag) {
                seekTime = videoInfor[videoCount].duration;
            }
            let seekMode = steps[stepCount + 2];
            stepCount = stepCount + 3
            avPlayer.seek(seekTime, seekMode);
        });

        // 设置音量操作
        eventEmitter.on(SETVOLUME_EVENT, (avPlayer, steps, done) => {
            let volumeValue = steps[stepCount + 1];
            stepCount = stepCount + 2;
            avPlayer.setVolume(volumeValue)
        });

        // 设置倍数操作
        eventEmitter.on(SETSPEED_EVENT, (avPlayer, steps, done) => {
            let speedValue = steps[stepCount + 1];
            stepCount++;
            avPlayer.setSpeed(speedValue);
        });

        // 设置loop值
        eventEmitter.on(LOOP_EVENT, (avPlayer, steps, done) => {
            let loopValue = steps[stepCount + 1];
            stepCount = stepCount + 2
            avPlayer.loop = loopValue;
            toNextStep(avPlayer, steps, done);
        });

        // 视频缩放设置
        eventEmitter.on(SCALETYPE_EVENT, (avPlayer, steps, done) => {
            let scaletypeValue = steps[stepCount + 1];
            stepCount = stepCount + 2
            avPlayer.videoScaleType = scaletypeValue;
            toNextStep(avPlayer, steps, done);
        });

        // 焦点上报设置
        eventEmitter.on(INTERRUPTMODE_EVENT, (avPlayer, steps, done) => {
            let interruptValue = steps[stepCount + 1];
            stepCount = stepCount + 2
            avPlayer.audioInterruptMode = interruptValue;
            toNextStep(avPlayer, steps, done);
        });

        // 音频渲染属性设置
        eventEmitter.on(AUDIORENDERINFO_EVENT, (avPlayer, steps, done) => {
            let renderValue = steps[stepCount + 1];
            stepCount = stepCount + 2
            console.info(TAG + 'set audioRenderInfo content is:' + renderValue[0] + ', usage is ' + renderValue[1] + ',renderFlags is '+ renderValue[2]);
            let testRendorValue = {
               'content':  renderValue[0],
               'usage': renderValue[1],
               'rendererFlags': renderValue[2]   
            }
            avPlayer.audioRendererInfo = testRendorValue;
            toNextStep(avPlayer, steps, done);
        });
        
        // 等待complete上报
        eventEmitter.on(FINISH_EVENT, (avPlayer, steps, done) => {
            stepCount++
            console.info(TAG + 'wait for completed called');
        });


        // 释放avplayer实例对象
        eventEmitter.on(RELEASE_EVENT, (avPlayer, steps, done) => {
            stepCount++;
            avPlayer.release((err) => {
                if (err == null) {
                    console.info(TAG + 'release success!!');
                    toNextStep(avPlayer, steps, done);
                } else if ((err != null) && (steps[stepCount] == ERROR_EVENT)) {
                    stepCount++;
                    toNextStep(avPlayer, steps, done);
                    console.info(TAG + 'release failed, and message is:' + err.message);
                } else {
                    mediaTestBase.printError(err, done);
                }
            });
        });

        // function seek 调用接口： SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_SEEK
        async function checkSeekTime(avPlayer, seekTime, seekMode) {
            console.info(TAG + 'test currentTime is:' + avPlayer.currentTime);
            switch (seekTime) {
                case 0:
                    if (seekMode == media.SeekMode.SEEK_PREV_SYNC) {
                        expect(avPlayer.currentTime - 0).assertLess(DELTA_TIME);
                    } else {
                        expect(avPlayer.currentTime - videoInfor[videoCount].PREV_FRAME).assertLess(DELTA_TIME);
                    }
                    break;
                case videoInfor[videoCount].duration / 2:
                    if (seekMode == media.SeekMode.SEEK_PREV_SYNC) {
                        expect(avPlayer.currentTime - videoInfor[videoCount].PREV_FRAME).assertLess(DELTA_TIME);
                    } else {
                        expect(avPlayer.currentTime - videoInfor[videoCount].NEXT_FRAME).assertLess(DELTA_TIME);
                    }
                    break;
                case videoInfor[videoCount].duration:
                    if (seekMode == media.SeekMode.SEEK_PREV_SYNC) {
                        expect(avPlayer.currentTime - videoInfor[videoCount].NEXT_FRAME).assertLess(DELTA_TIME);
                    } else {
                        expect(avPlayer.currentTime - seekTime).assertLess(DELTA_TIME);
                    }
                    break;
            }
        }

        eventEmitter.on(SEEK_FUNCTION_EVENT, (avPlayer, steps, done) => {
            let seekTime;
            if (steps[stepCount + 1] == halfDurationTag) {
                seekTime = videoInfor[videoCount].duration / 2;
            } else if (steps[stepCount + 1] == durationTag) {
                seekTime = videoInfor[videoCount].duration;
            } else {
                seekTime = steps[stepCount + 1];
            }
            let seekMode = steps[stepCount + 2];
            // seek操作回调函数
            avPlayer.on('seekDone', (seekDoneTime) => {
                console.info(TAG + 'seekDone success, seekTime is ' + seekTime + ' seekDoneTime time is:' + seekDoneTime);
                expect(seekDoneTime).assertEqual(seekTime);
                if (videoInfor[videoCount].type == 'audio') {
                    expect(avPlayer.currentTime - seekDoneTime).assertLess(DELTA_TIME);
                } else {
                    checkSeekTime(avPlayer, seekTime, seekMode);
                }
                toNextStep(avPlayer, steps, done);
            })
            stepCount = stepCount + 3
            avPlayer.seek(seekTime, seekMode);
        });

        eventEmitter.on(SPEED_FUNCTION_EVENT, (avPlayer, steps, done) => {
            let testSpeedValue = steps[stepCount + 1];
            stepCount = stepCount + 2;
            avPlayer.on('speedDone', (speed) => {
                console.info(TAG + 'speedDone success, set speed value is ' + testSpeedValue + ' speed value is:' + speed)
                expect(speed).assertEqual(testSpeedValue);
                toNextStep(avPlayer, steps, done);
            })
            avPlayer.setSpeed(testSpeedValue);
        });

        eventEmitter.on(PLAY_FUNCTION_EVENT, (avPlayer, steps, done) => {
            stepCount++;
            avPlayer.play(async (err) => {
                if (err == null) {
                    await mediaTestBase.msleepAsync(PLAY_TIME);
                    toNextStep(avPlayer, steps, done);
                } else if ((err != null) && (steps[stepCount] == ERROR_EVENT)) {
                    stepCount++;
                    console.info(TAG + 'play error happened,and message is :' + err.message);
                    toNextStep(avPlayer, steps, done);
                } else {
                    mediaTestBase.printError(err, done);
                }
            });
        });

        // gettreckdescription promise
        eventEmitter.on(GETDESCRIPTION_PROMISE, async (avPlayer, steps, done) => {
            let arrayDescription;
            stepCount++;
            await avPlayer.getTrackDescription().then((arrayList) => {
                console.info('case getTrackDescription called!!');
                if (typeof (arrayList) != 'undefined') {
                    arrayDescription = arrayList;
                    expect(descriptionKey[videoCount].length).assertEqual(arrayDescription.length);
                    for (let i = 0; i < arrayDescription.length; i++) {
                        mediaTestBase.checkDescription(arrayDescription[i], descriptionKey[videoCount][i], descriptionValue[videoCount][i]);
                    }
                    toNextStep(avPlayer, steps, done);
                } else {
                    console.info('case getTrackDescription is null');
                    expect().assertFail();
                    done();
                }
            }, (error) => {
                console.info(TAG + 'getTrackDescription error happened,and message is :' + error.message);
                if (steps[stepCount] == ERROR_EVENT) {
                    stepCount++;
                    toNextStep(avPlayer, steps, done);
                }
            }).catch(mediaTestBase.catchCallback);
        });

        // gettreckdescription callback
        eventEmitter.on(GETDESCRIPTION_CALLBACK, async (avPlayer, steps, done) => {
            stepCount++;
            avPlayer.getTrackDescription((error, arrayList) => {
                if (error == null) {
                    expect(descriptionKey[videoCount].length).assertEqual(arrayList.length);
                    for (let i = 0; i < arrayList.length; i++) {
                        mediaTestBase.checkDescription(arrayList[i], descriptionKey[videoCount][i], descriptionValue[videoCount][i]);
                    }
                    toNextStep(avPlayer, steps, done);
                } else {
                    if (steps[stepCount] == ERROR_EVENT) {
                        stepCount++;
                        console.info(TAG + 'getTrackDescription error happened,and message is :' + error.message);
                        toNextStep(avPlayer, steps, done);
                    }
                }
            })
        });

        // check loop value
        eventEmitter.on(CHECKLOOP_EVENT, async (avPlayer, steps, done) => {
            stepCount++;
            let loopValue = steps[stepCount];
            let state = avPlayer.state;
            endEOS = 0;
            completedValue = 0;
            avPlayer.on('seekDone', (seekDoneTime) => {
                console.info(TAG + 'seekDone success,and seek time is:' + seekDoneTime)
            })
            avPlayer.on('endOfStream', () => {
                console.info(TAG + 'endOfStream success')
                endEOS++;
            })
            console.info(TAG + 'check loop value, state is:' + state + ', loop value is:' + loopValue + ', avplayer loop is ' + avPlayer.loop);
            switch (state) {
                case AV_PLAYER_STATE.PREPARED:
                    console.info(TAG + 'check loop value, state prepared state check')
                    await avPlayer.play();
                    avPlayer.seek(videoInfor[videoCount].duration, media.SeekMode.SEEK_PREV_SYNC);
                    await mediaTestBase.msleepAsync(PLAY_TIME * 4);
                    if (loopValue) {
                        expect(endEOS).assertEqual(1);
                        expect(completedValue).assertEqual(0);
                        expect(avPlayer.state).assertEqual('playing');
                        await avPlayer.pause();
                    } else {
                        expect(endEOS).assertEqual(1);
                        expect(completedValue).assertEqual(1);
                        expect(avPlayer.state).assertEqual('completed');
                    }
                    break;
                case AV_PLAYER_STATE.PLAYING:
                    console.info(TAG + 'check loop value, state playing state check')
                    avPlayer.seek(videoInfor[videoCount].duration, media.SeekMode.SEEK_PREV_SYNC);
                    await mediaTestBase.msleepAsync(PLAY_TIME * 4);
                    if (loopValue) {
                        expect(endEOS).assertEqual(1);
                        expect(completedValue).assertEqual(0);
                        expect(avPlayer.state).assertEqual('playing');
                        await avPlayer.pause();
                    } else {
                        expect(endEOS).assertEqual(1);
                        expect(completedValue).assertEqual(1);
                        expect(avPlayer.state).assertEqual('completed');
                    }
                    break;
                case AV_PLAYER_STATE.PAUSED:
                    console.info(TAG + 'check loop value, state paused state check')
                    await avPlayer.play();
                    avPlayer.seek(videoInfor[videoCount].duration, media.SeekMode.SEEK_PREV_SYNC);
                    await mediaTestBase.msleepAsync(PLAY_TIME * 4);
                    if (loopValue) {
                        expect(endEOS).assertEqual(1);
                        expect(completedValue).assertEqual(0);
                        expect(avPlayer.state).assertEqual('playing');
                        await avPlayer.pause();
                    } else {
                        expect(endEOS).assertEqual(1);
                        expect(completedValue).assertEqual(1);
                        expect(avPlayer.state).assertEqual('completed');
                    }
                    break;
                case AV_PLAYER_STATE.COMPLETED:
                    console.info(TAG + 'check loop value, state completed state check')
                    await avPlayer.play();
                    avPlayer.seek(videoInfor[videoCount].duration, media.SeekMode.SEEK_PREV_SYNC);
                    await mediaTestBase.msleepAsync(PLAY_TIME * 4);
                    if (loopValue) {
                        expect(endEOS).assertEqual(1);
                        expect(completedValue).assertEqual(0);
                        expect(avPlayer.state).assertEqual('playing');
                        await avPlayer.pause();
                    } else {
                        expect(endEOS).assertEqual(1);
                        expect(completedValue).assertEqual(1);
                        expect(avPlayer.state).assertEqual('completed');
                    }
                    break;
            }
            stepCount++;
            toNextStep(avPlayer, steps, done);
        });

        async function startNewAvPlayer() {
            let testAVPLayer = await media.createAVPlayer();
            testAVPLayer.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AV_PLAYER_STATE.INITIALIZED:
                        await testAVPLayer.prepare();
                        console.info(TAG + 'audioInterruptMode state initialized called ')
                        break;
                    case AV_PLAYER_STATE.PREPARED:
                        await testAVPLayer.play();
                        console.info(TAG + 'audioInterruptMode state prepared called')
                        break;
                    case AV_PLAYER_STATE.PLAYING:
                        await mediaTestBase.msleepAsync(1000);
                        await testAVPLayer.stop();
                        console.info(TAG + 'audioInterruptMode state playing called')
                        break;
                    case AV_PLAYER_STATE.STOPPED:
                        await testAVPLayer.release();
                        console.info(TAG + 'audioInterruptMode state stopped called')
                        break;
                    case AV_PLAYER_STATE.RELEASED:
                        testAVPLayer = null;
                        console.info(TAG + 'audioInterruptMode state released called')
                        break;
                    case AV_PLAYER_STATE.ERROR:
                        console.info(TAG + 'audioInterruptMode state error called')
                        break;
                    default:
                }
            })
            testAVPLayer.fdSrc = videoList[1];
        }

        // check audioInterruptMode value
        eventEmitter.on(CHECKINTERRUPT_EVENT, async (avPlayer, steps, done) => {
            stepCount++;
            let interruptValue = steps[stepCount];
            let state = avPlayer.state;
            let interruptCount = 0;
            avPlayer.on('audioInterrupt', (info) => {
                interruptCount++;
                console.info(TAG + 'audioInterrupt success,and InterruptEvent info is:' + JSON.stringify(info))
            })
            console.info(TAG + 'check audioInterruptMode value, state is:' + state + ', audioInterruptMode value is:' + interruptValue);
            switch (state) {
                case AV_PLAYER_STATE.PREPARED:
                    console.info(TAG + 'check audioInterruptMode value, state prepared state check')
                    await avPlayer.play();
                    await startNewAvPlayer();
                    await mediaTestBase.msleepAsync(2000);
                    if (videoCount != 2 && interruptValue == audio.InterruptMode.INDEPENDENT_MODE) {
                        console.info(TAG + 'hope interruptCount is 1, act interruptCount is: ' + interruptCount);
                        expect(interruptCount).assertEqual(1);
                    } else {
                        console.info(TAG + 'hope interruptCount is 0, act interruptCount is: ' + interruptCount);
                        expect(interruptCount).assertEqual(0);
                    }
                    break;
                case AV_PLAYER_STATE.PLAYING:
                    console.info(TAG + 'check audioInterruptMode value, state playing state check')
                    await startNewAvPlayer();
                    await mediaTestBase.msleepAsync(2000);
                    if (videoCount != 2 && interruptValue == audio.InterruptMode.INDEPENDENT_MODE) {
                        console.info(TAG + 'hope interruptCount is 1, act interruptCount is: ' + interruptCount);
                        expect(interruptCount).assertEqual(1);
                    } else {
                        console.info(TAG + 'hope interruptCount is 0, act interruptCount is: ' + interruptCount);
                        expect(interruptCount).assertEqual(0);
                    }
                    break;
                case AV_PLAYER_STATE.PAUSED:
                    console.info(TAG + 'check audioInterruptMode value, state paused state check')
                    await avPlayer.play();
                    await startNewAvPlayer();
                    await mediaTestBase.msleepAsync(2000);
                    if (videoCount != 2 && interruptValue == audio.InterruptMode.INDEPENDENT_MODE) {
                        console.info(TAG + 'hope interruptCount is 1, act interruptCount is: ' + interruptCount);
                        expect(interruptCount).assertEqual(1);
                    } else {
                        console.info(TAG + 'hope interruptCount is 0, act interruptCount is: ' + interruptCount);
                        expect(interruptCount).assertEqual(0);
                    }
                    break;
                case AV_PLAYER_STATE.COMPLETED:
                    console.info(TAG + 'check audioInterruptMode value, state completed state check')
                    await avPlayer.play();
                    await startNewAvPlayer();
                    await mediaTestBase.msleepAsync(2000);
                    if (videoCount != 2 && interruptValue == audio.InterruptMode.INDEPENDENT_MODE) {
                        console.info(TAG + 'hope interruptCount is 1, act interruptCount is: ' + interruptCount);
                        expect(interruptCount).assertEqual(1);
                    } else {
                        console.info(TAG + 'hope interruptCount is 0, act interruptCount is: ' + interruptCount);
                        expect(interruptCount).assertEqual(0);
                    }
                    break;
            }
            stepCount++;
            toNextStep(avPlayer, steps, done);
        });

        eventEmitter.on(CHECK_PARAMETER, (avPlayer, steps, done) => {
            stepCount++;
            let valueKey = steps[stepCount];
            let value = steps[stepCount + 1];
            console.info(TAG + 'check name is:' + valueKey);
            switch (valueKey) {
                case 'url':
                    console.info(TAG + 'expect url value is:' + value + ', actual url value is:' + avPlayer.url);
                    expect(avPlayer.url).assertEqual(value);
                    break;
                case 'fdSrc':
                    console.info(TAG + 'expect fdSrc value is:' + JSON.stringify(value) + ', actual fdSrc value is:' + JSON.stringify(avPlayer.fdSrc));
                    expect(avPlayer.fdSrc.fd).assertEqual(value.fd);
                    expect(avPlayer.fdSrc.offset).assertEqual(value.offset);
                    expect(avPlayer.fdSrc.length).assertEqual(value.length);
                    break;
                case 'surafaceId':
                    console.info(TAG + 'expect surafaceId value is:' + JSON.stringify(value) + ', actual surafaceId value is:' + JSON.stringify(avPlayer.surfaceId));
                    expect(avPlayer.surfaceId).assertEqual(value);
                    break;
                case 'loop':
                    console.info(TAG + 'expect loop value is:' + JSON.stringify(value) + ', actual loop value is:' + JSON.stringify(avPlayer.loop));
                    expect(avPlayer.loop).assertEqual(value);
                    break;
                case 'videoScaeType':
                    console.info(TAG + 'expect videoScaeType value is:' + JSON.stringify(value) + ', actual videoScaeType value is:' + JSON.stringify(avPlayer.videoScaleType));
                    expect(avPlayer.videoScaleType).assertEqual(value);
                    break;
                case 'audioInterruptMode':
                    console.info(TAG + 'expect audioInterruptMode value is:' + JSON.stringify(value) + ', actual audioInterruptMode value is:' + JSON.stringify(avPlayer.audioInterruptMode));
                    expect(avPlayer.audioInterruptMode).assertEqual(value);                    
                    break;
                case 'audioRenderInfo':
                    console.info(TAG + 'expect audioRenderInfo value is:' + JSON.stringify(value) + ', actual audioRenderInfo value is:' + JSON.stringify(avPlayer.audioRendererInfo));
                    expect(avPlayer.audioRendererInfo.content).assertEqual(value[0]);
                    expect(avPlayer.audioRendererInfo.usage).assertEqual(value[1]);
                    expect(avPlayer.audioRendererInfo.rendererFlags).assertEqual(value[2]); 
                    break;
                case 'state':
                    console.info(TAG + 'expect state value is:' + JSON.stringify(value) + ', actual state value is:' + JSON.stringify(avPlayer.state));
                    expect(avPlayer.state).assertEqual(value);
                    break;
                case 'time':
                    console.info(TAG + 'expect currenttime is:' + value[0] + ', actual currenttime is:' + avPlayer.currentTime);
                    console.info(TAG + 'expect duration is:' + value[1] + ', actual duration is:' + avPlayer.duration);
                    expect(Math.abs(avPlayer.currentTime - value[0])).assertLess(DELTA_TIME);
                    expect(avPlayer.duration).assertEqual(value[1]);
                    break;
                case 'size':
                    console.info(TAG + 'expect width is:' + value[0] + ', actual width is:' + avPlayer.width);
                    console.info(TAG + 'expect height is:' + value[1] + ', actual height is:' + avPlayer.height);
                    expect(avPlayer.width).assertEqual(value[0]);
                    expect(avPlayer.height).assertEqual(value[1]);
                    break;
            }
            stepCount = stepCount + 2;
            toNextStep(avPlayer, steps, done);
        });

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_SEEK
            * @tc.name      : 001.test seek 0、seek duration/2、 duration by PREV and NEXT seekMode
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_SEEK', 0, async function (done) {
            testTag = 'test_seek';
            // create -> seek(error) -> url -> seek(error) -> prepare -> seek(ok) -> play -> seek(ok) -> completed -> seek(ok) -> play -> pause -> seek(ok)
            // -> stop -> seek(error) -> reset -> seek(error)-> prepare  -> seek(duration) -> play -> completed ->  release
            let mySteps01 = new Array(CREATE_EVENT, SEEK_FUNCTION_EVENT, 0 ,media.SeekMode.SEEK_NEXT_SYNC, ERROR_EVENT,
                 SETFDSRC_EVENT, SEEK_FUNCTION_EVENT, 0 , media.SeekMode.SEEK_NEXT_SYNC, ERROR_EVENT, SETSURFACE_EVENT, surfaceID, 
                 PREPARE_EVENT, SEEK_FUNCTION_EVENT, halfDurationTag, media.SeekMode.SEEK_PREV_SYNC, PLAY_EVENT,
                 SEEK_FUNCTION_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, /*SEEK_FUNCTION_EVENT, 0 , media.SeekMode.SEEK_NEXT_SYNC,*/
                 PLAY_EVENT, PAUSE_EVENT, SEEK_FUNCTION_EVENT, halfDurationTag, media.SeekMode.SEEK_NEXT_SYNC, STOP_EVENT,
                 SEEK_FUNCTION_EVENT, 0 , media.SeekMode.SEEK_NEXT_SYNC, ERROR_EVENT, RESET_EVENT, SEEK_FUNCTION_EVENT, 0 , media.SeekMode.SEEK_NEXT_SYNC,
                 ERROR_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, SEEK_FUNCTION_EVENT, durationTag, media.SeekMode.SEEK_NEXT_SYNC,
                 PLAY_EVENT, FINISH_EVENT, RELEASE_EVENT, END_EVENT);
            myStepsList = new Array(mySteps01);

            eventEmitter.on('test_seek', () => {    
                console.info(TAG + '**************************test video or audio name is :'+ videoNameList[videoCount] +'*****************');
                console.info(TAG + '**************************this is function seek test: ' + (testCount + 1) + ' testcase start****************');
                let avPlayer = null;
                eventEmitter.emit(myStepsList[testCount][stepCount], avPlayer, myStepsList[testCount], done);
                testCount++; 
            });
            eventEmitter.emit(testTag);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_SETSOURCE_0100
            * @tc.name      : 001.test url
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_SETSOURCE_0100', 0, async function (done) {
            avPlayTest = { width: 720, height: 480, duration: 10100 };
            testAVPlayerFun(fdPath, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_SETSOURCE_0200
            * @tc.name      : 002.test fdsrc
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_SETSOURCE_0200', 0, async function (done) {
            avPlayTest = { width: 720, height: 480, duration: 10100 };
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_SPEED
            * @tc.name      : 001.test setSpeed(0.75/1/1.25/1.75/2)in all states
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_SPEED', 0, async function (done) {
            testTag = 'test_speed';
            // create -> setspeed(error) -> url -> setspeed(error) -> surfaceID -> prepare -> play -> reset -> url -> surfaceID
            // -> prepare -> setspeed(1.25) -> play -> setSpeed(1.75/2/0.75/1)-> pasue -> setSpeed(0.75) -> play -> seek duration -> setSpeed(1)-> stop
            // -> setSpeed(error) -> reset -> url -> surfaceID -> prepare -> play -> reset -> setSpeed(0.75) 
            // -> release
            let mySteps01 = new Array(CREATE_EVENT, SPEED_FUNCTION_EVENT, media.PlaybackSpeed.SPEED_FORWARD_0_75_X, ERROR_EVENT,
                 SETFDSRC_EVENT, SPEED_FUNCTION_EVENT, media.PlaybackSpeed.SPEED_FORWARD_1_00_X, ERROR_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT,
                 SPEED_FUNCTION_EVENT, media.PlaybackSpeed.SPEED_FORWARD_1_25_X, PLAY_FUNCTION_EVENT, SPEED_FUNCTION_EVENT, media.PlaybackSpeed.SPEED_FORWARD_0_75_X,
                 PLAY_FUNCTION_EVENT, SPEED_FUNCTION_EVENT, media.PlaybackSpeed.SPEED_FORWARD_0_75_X, PLAY_FUNCTION_EVENT, SPEED_FUNCTION_EVENT, media.PlaybackSpeed.SPEED_FORWARD_1_75_X,
                 PLAY_FUNCTION_EVENT, SPEED_FUNCTION_EVENT, media.PlaybackSpeed.SPEED_FORWARD_2_00_X, PLAY_FUNCTION_EVENT, PAUSE_EVENT, SPEED_FUNCTION_EVENT, media.PlaybackSpeed.SPEED_FORWARD_0_75_X,
                 PLAY_FUNCTION_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SPEED_FUNCTION_EVENT, media.PlaybackSpeed.SPEED_FORWARD_1_00_X,
                 STOP_EVENT, SPEED_FUNCTION_EVENT, media.PlaybackSpeed.SPEED_FORWARD_1_75_X, ERROR_EVENT, RESET_EVENT, SETFDSRC_EVENT, 
                 SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_FUNCTION_EVENT, RESET_EVENT, SPEED_FUNCTION_EVENT, media.PlaybackSpeed.SPEED_FORWARD_2_00_X, ERROR_EVENT,
                 RELEASE_EVENT, END_EVENT);
            myStepsList = new Array(mySteps01);

            eventEmitter.on('test_speed', () => {    
                console.info(TAG + '**************************test video or audio name is :'+ videoNameList[videoCount] +'*****************');
                console.info(TAG + '**************************this is function speed test: ' + (testCount + 1) + ' testcase start****************');
                let avPlayer = null;
                eventEmitter.emit(myStepsList[testCount][stepCount], avPlayer, myStepsList[testCount], done);
                testCount++; 
            });
            eventEmitter.emit(testTag);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_SETVOLUME
            * @tc.name      : 001.test setVolume[0, 1]]in all states
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_SETVOLUME', 0, async function (done) {
            testTag = 'test_volume';
            // create -> setVolume(error) -> url -> setVolume(error) -> surfaceID -> setVolume(error) -> prepare -> setVolume(0.3) -> play -> setVolume(0.4)
            // -> pause -> setVolume(0.5) -> seek duration -> setVolume(0.6) -> finish -> setVolume(0.7) -> setVolume(0) -> stop -> setVolume(error)
            // -> reset -> setVolume(error) -> url -> prepare -> play -> release
            let mySteps01 = new Array(CREATE_EVENT, SETVOLUME_EVENT, 0, ERROR_EVENT, SETFDSRC_EVENT, SETVOLUME_EVENT, 0.1, ERROR_EVENT, SETSURFACE_EVENT, surfaceID,
                SETVOLUME_EVENT, 0.2, ERROR_EVENT, PREPARE_EVENT, SETVOLUME_EVENT, 0.3, PLAY_EVENT, SETVOLUME_EVENT, 0.4, PAUSE_EVENT, SETVOLUME_EVENT, 0.5, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, SETVOLUME_EVENT, 0.6, FINISH_EVENT, SETVOLUME_EVENT, 0.7, SETVOLUME_EVENT, 0,
                STOP_EVENT, SETVOLUME_EVENT, 0.8, ERROR_EVENT, RESET_EVENT, SETVOLUME_EVENT, 0.9, ERROR_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID,
                PREPARE_EVENT, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            myStepsList = new Array(mySteps01);

            eventEmitter.on('test_volume', () => {    
                console.info(TAG + '**************************test video or audio name is :'+ videoNameList[videoCount] +'*****************');
                console.info(TAG + '**************************this is function speed test: ' + (testCount + 1) + ' testcase start****************');
                let avPlayer = null;
                eventEmitter.emit(myStepsList[testCount][stepCount], avPlayer, myStepsList[testCount], done);
                testCount++; 
            });
            eventEmitter.emit(testTag);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_GETTRECKDESCRITION_0100
            * @tc.name      : 001.test gettreckdescription(promise) in all states
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_GETTRECKDESCRITION_0100', 0, async function (done) {
            testTag = 'test_treck_promise';
            // create -> getTrackDescription(error) -> url -> getTrackDescription(error) -> surfaceID -> getTrackDescription(error)
            // -> prepare -> getTrackDescription -> play -> getTrackDescription -> pause -> getTrackDescription -> seek duration -> completed
            // -> getTrackDescription -> stop -> getTrackDescription(error) -> reset -> getTrackDescription(error) -> release -> getTrackDescription(error)
            let mySteps01 = new Array(CREATE_EVENT, GETDESCRIPTION_PROMISE, ERROR_EVENT, SETFDSRC_EVENT, GETDESCRIPTION_PROMISE, ERROR_EVENT,
                SETSURFACE_EVENT, surfaceID, GETDESCRIPTION_PROMISE, ERROR_EVENT, PREPARE_EVENT, GETDESCRIPTION_PROMISE, PLAY_EVENT,
                GETDESCRIPTION_PROMISE, PAUSE_EVENT, GETDESCRIPTION_PROMISE, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT,
                GETDESCRIPTION_PROMISE, STOP_EVENT, GETDESCRIPTION_PROMISE, ERROR_EVENT, RESET_EVENT, GETDESCRIPTION_PROMISE, ERROR_EVENT, RELEASE_EVENT,
                GETDESCRIPTION_PROMISE, ERROR_EVENT, END_EVENT);
            myStepsList = new Array(mySteps01);

            eventEmitter.on('test_treck_promise', () => {    
                console.info(TAG + '**************************test video or audio name is :'+ videoNameList[videoCount] +'*****************');
                console.info(TAG + '**************************this is function speed test: ' + (testCount + 1) + ' testcase start****************');
                let avPlayer = null;
                eventEmitter.emit(myStepsList[testCount][stepCount], avPlayer, myStepsList[testCount], done);
                testCount++; 
            });
            eventEmitter.emit(testTag);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_GETTRECKDESCRITION_0200
            * @tc.name      : 002.test gettreckdescription(callback) in all states
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_GETTRECKDESCRITION_0200', 0, async function (done) {
            testTag = 'test_treck_callback';
            // create -> getTrackDescription(error) -> url -> getTrackDescription(error) -> surfaceID -> getTrackDescription(error)
            // -> prepare -> getTrackDescription -> play -> getTrackDescription -> pause -> getTrackDescription -> seek duration -> completed
            // -> getTrackDescription -> stop -> getTrackDescription(error) -> reset -> getTrackDescription(error) -> release -> getTrackDescription(error)
            let mySteps01 = new Array(CREATE_EVENT, GETDESCRIPTION_CALLBACK, ERROR_EVENT, SETFDSRC_EVENT, GETDESCRIPTION_CALLBACK, ERROR_EVENT,
                SETSURFACE_EVENT, surfaceID, GETDESCRIPTION_CALLBACK, ERROR_EVENT, PREPARE_EVENT, GETDESCRIPTION_CALLBACK, PLAY_EVENT,
                GETDESCRIPTION_CALLBACK, PAUSE_EVENT, GETDESCRIPTION_CALLBACK, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT,
                GETDESCRIPTION_CALLBACK, STOP_EVENT, GETDESCRIPTION_CALLBACK, ERROR_EVENT, RESET_EVENT, GETDESCRIPTION_CALLBACK, ERROR_EVENT, RELEASE_EVENT,
                GETDESCRIPTION_CALLBACK, ERROR_EVENT, END_EVENT);
            myStepsList = new Array(mySteps01);

            eventEmitter.on('test_treck_callback', () => {    
                console.info(TAG + '**************************test video or audio name is :'+ videoNameList[videoCount] +'*****************');
                console.info(TAG + '**************************this is function speed test: ' + (testCount + 1) + ' testcase start****************');
                let avPlayer = null;
                eventEmitter.emit(myStepsList[testCount][stepCount], avPlayer, myStepsList[testCount], done);
                testCount++; 
            });
            eventEmitter.emit(testTag);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_LOOP
            * @tc.name      : 001.test loop in all states
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_LOOP', 0, async function (done) {
            testTag = 'test_loop';
            // create -> loop(error) -> url -> loop(error) -> surfaceID -> loop(error) -> prepare -> loop(true) -> check
            // -> play -> loop(false) -> check -> play -> loop(true) -> check(paused) -> loop(fales) -> check(paused) ->loop(false)
            // check(completed) -> loop(false) -> check(completed) -> loop(true) -> check(paused) -> stop -> loop(error)
            // -> reset -> loop(error) -> url -> surfaceID -> prepare -> play -> check -> release
            let mySteps01 = new Array(CREATE_EVENT, LOOP_EVENT, true, ERROR_EVENT, SETFDSRC_EVENT, LOOP_EVENT, true, ERROR_EVENT,
                SETSURFACE_EVENT, surfaceID, LOOP_EVENT, true, ERROR_EVENT, PREPARE_EVENT, LOOP_EVENT, true, CHECKLOOP_EVENT, true,
                PLAY_EVENT, LOOP_EVENT, false, CHECKLOOP_EVENT, false, PLAY_EVENT, LOOP_EVENT, true, CHECKLOOP_EVENT, true, LOOP_EVENT, false,
                CHECKLOOP_EVENT, false, LOOP_EVENT, false, CHECKLOOP_EVENT, false, LOOP_EVENT, true, CHECKLOOP_EVENT, true,
                STOP_EVENT, LOOP_EVENT, true, ERROR_EVENT, RESET_EVENT, LOOP_EVENT, false, ERROR_EVENT, SETFDSRC_EVENT,
                SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, CHECKLOOP_EVENT, false, RELEASE_EVENT, END_EVENT);
            myStepsList = new Array(mySteps01);

            eventEmitter.on('test_loop', () => {    
                console.info(TAG + '**************************test video or audio name is :'+ videoNameList[videoCount] +'*****************');
                console.info(TAG + '**************************this is function loop test: ' + (testCount + 1) + ' testcase start****************');
                let avPlayer = null;
                eventEmitter.emit(myStepsList[testCount][stepCount], avPlayer, myStepsList[testCount], done);
                testCount++; 
            });
            eventEmitter.emit(testTag);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_VIDEOSCALETYPE
            * @tc.name      : 001.test videoscaletype in all states
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_VIDEOSCALETYPE', 0, async function (done) {
            testTag = 'test_scaletype';
            // create -> scaletype(error) -> url -> scaletype(error) -> surfaceID -> scaletype(error) -> prepare -> scaletype
            // -> play -> scaletype -> pause -> scaletype -> play -> seek duration -> completed -> scaletype -> stop
            // -> scaletype(error) -> reset -> scaletype(error) -> release 
            let mySteps01 = new Array(CREATE_EVENT, SCALETYPE_EVENT, media.VideoScaleType.VIDEO_SCALE_TYPE_FIT, ERROR_EVENT,
                SETFDSRC_EVENT, SCALETYPE_EVENT, media.VideoScaleType.VIDEO_SCALE_TYPE_FIT_CROP, ERROR_EVENT, SETSURFACE_EVENT, surfaceID,
                SCALETYPE_EVENT, media.VideoScaleType.VIDEO_SCALE_TYPE_FIT, ERROR_EVENT, PREPARE_EVENT, SCALETYPE_EVENT, media.VideoScaleType.VIDEO_SCALE_TYPE_FIT_CROP,
                PLAY_EVENT, SCALETYPE_EVENT, media.VideoScaleType.VIDEO_SCALE_TYPE_FIT, PAUSE_EVENT, SCALETYPE_EVENT, media.VideoScaleType.VIDEO_SCALE_TYPE_FIT_CROP,
                PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SCALETYPE_EVENT, media.VideoScaleType.VIDEO_SCALE_TYPE_FIT,
                STOP_EVENT, SCALETYPE_EVENT, media.VideoScaleType.VIDEO_SCALE_TYPE_FIT, ERROR_EVENT, RESET_EVENT, SCALETYPE_EVENT, media.VideoScaleType.VIDEO_SCALE_TYPE_FIT,
                ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            myStepsList = new Array(mySteps01);

            eventEmitter.on('test_scaletype', () => {    
                console.info(TAG + '**************************test video or audio name is :'+ videoNameList[videoCount] +'*****************');
                console.info(TAG + '**************************this is function videoscaletype test: ' + (testCount + 1) + ' testcase start****************');
                let avPlayer = null;
                eventEmitter.emit(myStepsList[testCount][stepCount], avPlayer, myStepsList[testCount], done);
                testCount++; 
            });
            eventEmitter.emit(testTag);
        })


        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_PARAMETER
            * @tc.name      : 001.test set and get parameter in all states
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_FUNCTION_PARAMETER', 0, async function (done) {
            testTag = 'test_parameter';
            // url
            let mySteps01 = new Array(CREATE_EVENT, CHECK_PARAMETER, 'url', '', SETSOURCE_EVENT, fdPath, CHECK_PARAMETER, 'url', fdPath, SETSURFACE_EVENT, surfaceID, 
                SETSOURCE_EVENT, fdPath02, ERROR_EVENT, CHECK_PARAMETER, 'url', fdPath, PREPARE_EVENT, SETSOURCE_EVENT, fdPath02, ERROR_EVENT,
                CHECK_PARAMETER, 'url', fdPath, PLAY_EVENT, SETSOURCE_EVENT, fdPath02, ERROR_EVENT, CHECK_PARAMETER, 'url', fdPath,
                PAUSE_EVENT, SETSOURCE_EVENT, fdPath02, ERROR_EVENT, CHECK_PARAMETER, 'url', fdPath, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT,
                SETSOURCE_EVENT, fdPath02, ERROR_EVENT, CHECK_PARAMETER, 'url', fdPath, STOP_EVENT, SETSOURCE_EVENT, fdPath02, ERROR_EVENT, CHECK_PARAMETER, 'url', fdPath,
                RESET_EVENT, SETSOURCE_EVENT, fdPath02, CHECK_PARAMETER, 'url', fdPath02, RELEASE_EVENT, END_EVENT);
            // fdSrc
            let mySteps02 = new Array(CREATE_EVENT, CHECK_PARAMETER, 'fdSrc', {"fd":0,"offset":0,"length":-1}, SETSOURCE_EVENT, videoList[0], CHECK_PARAMETER, 'fdSrc', videoList[0], SETSURFACE_EVENT, surfaceID, 
                SETSOURCE_EVENT, videoList[1], ERROR_EVENT, CHECK_PARAMETER, 'fdSrc', videoList[0], PREPARE_EVENT, SETSOURCE_EVENT, videoList[1], ERROR_EVENT,
                CHECK_PARAMETER, 'fdSrc', videoList[0], PLAY_EVENT, SETSOURCE_EVENT, videoList[1], ERROR_EVENT, CHECK_PARAMETER, 'fdSrc', videoList[0],
                PAUSE_EVENT, SETSOURCE_EVENT, videoList[1], ERROR_EVENT, CHECK_PARAMETER, 'fdSrc', videoList[0], PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT,
                SETSOURCE_EVENT, videoList[1], ERROR_EVENT, CHECK_PARAMETER, 'fdSrc', videoList[0], STOP_EVENT, SETSOURCE_EVENT, videoList[1], ERROR_EVENT, CHECK_PARAMETER, 'fdSrc', videoList[0],
                RESET_EVENT, SETSOURCE_EVENT, videoList[1], CHECK_PARAMETER, 'fdSrc', videoList[1], RELEASE_EVENT, END_EVENT);
            // syrfaceID
            let mySteps03 = new Array(CREATE_EVENT, CHECK_PARAMETER, 'surafaceId', '', SETSURFACE_EVENT, surfaceID, ERROR_EVENT, CHECK_PARAMETER, 'surafaceId', '',
                SETSOURCE_EVENT, fdPath, SETSURFACE_EVENT, surfaceID, CHECK_PARAMETER, 'surafaceId', surfaceID, PREPARE_EVENT, SETSURFACE_EVENT, '12345678', ERROR_EVENT,
                CHECK_PARAMETER, 'surafaceId', surfaceID, PLAY_EVENT, SETSURFACE_EVENT, '12345678', ERROR_EVENT, CHECK_PARAMETER, 'surafaceId', surfaceID,
                PAUSE_EVENT, SETSURFACE_EVENT, '12345678', ERROR_EVENT, CHECK_PARAMETER, 'surafaceId', surfaceID, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT,
                SETSURFACE_EVENT, '12345678', ERROR_EVENT, CHECK_PARAMETER, 'surafaceId', surfaceID, STOP_EVENT, SETSURFACE_EVENT, '12345678', ERROR_EVENT, CHECK_PARAMETER, 'surafaceId', surfaceID,
                RESET_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, CHECK_PARAMETER, 'surafaceId', surfaceID, RELEASE_EVENT, END_EVENT);
            // loop
            let mySteps04 = new Array(CREATE_EVENT, CHECK_PARAMETER, 'loop', false, LOOP_EVENT, true, ERROR_EVENT, CHECK_PARAMETER, 'loop', false,
                SETSOURCE_EVENT, fdPath, LOOP_EVENT, true, ERROR_EVENT, CHECK_PARAMETER, 'loop', false, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, LOOP_EVENT, true,
                CHECK_PARAMETER, 'loop', true, PLAY_EVENT, LOOP_EVENT, false, CHECK_PARAMETER, 'loop', false, PAUSE_EVENT, LOOP_EVENT, true, CHECK_PARAMETER, 'loop', true,
                PLAY_EVENT, LOOP_EVENT, false, CHECK_PARAMETER, 'loop', false, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT,
                LOOP_EVENT, true, CHECK_PARAMETER, 'loop', true, STOP_EVENT, LOOP_EVENT, false, ERROR_EVENT, CHECK_PARAMETER, 'loop', true, RESET_EVENT,
                LOOP_EVENT, false, ERROR_EVENT, CHECK_PARAMETER, 'loop', false, RELEASE_EVENT, END_EVENT);
            // videoScaeType
            let mySteps05 = new Array(CREATE_EVENT, CHECK_PARAMETER, 'videoScaeType', media.VideoScaleType.VIDEO_SCALE_TYPE_FIT, SCALETYPE_EVENT, media.VideoScaleType.VIDEO_SCALE_TYPE_FIT_CROP, ERROR_EVENT,
                CHECK_PARAMETER, 'videoScaeType', media.VideoScaleType.VIDEO_SCALE_TYPE_FIT, SETSOURCE_EVENT, fdPath, SCALETYPE_EVENT, media.VideoScaleType.VIDEO_SCALE_TYPE_FIT_CROP, ERROR_EVENT,
                CHECK_PARAMETER, 'videoScaeType', media.VideoScaleType.VIDEO_SCALE_TYPE_FIT, SETSURFACE_EVENT, surfaceID, SCALETYPE_EVENT, media.VideoScaleType.VIDEO_SCALE_TYPE_FIT_CROP, ERROR_EVENT,
                CHECK_PARAMETER, 'videoScaeType', media.VideoScaleType.VIDEO_SCALE_TYPE_FIT, PREPARE_EVENT, SCALETYPE_EVENT, media.VideoScaleType.VIDEO_SCALE_TYPE_FIT_CROP,
                CHECK_PARAMETER, 'videoScaeType', media.VideoScaleType.VIDEO_SCALE_TYPE_FIT_CROP, PLAY_EVENT, SCALETYPE_EVENT, media.VideoScaleType.VIDEO_SCALE_TYPE_FIT,
                CHECK_PARAMETER, 'videoScaeType', media.VideoScaleType.VIDEO_SCALE_TYPE_FIT, PAUSE_EVENT, SCALETYPE_EVENT, media.VideoScaleType.VIDEO_SCALE_TYPE_FIT_CROP,
                CHECK_PARAMETER, 'videoScaeType', media.VideoScaleType.VIDEO_SCALE_TYPE_FIT_CROP, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT,
                SCALETYPE_EVENT, media.VideoScaleType.VIDEO_SCALE_TYPE_FIT, CHECK_PARAMETER, 'videoScaeType', media.VideoScaleType.VIDEO_SCALE_TYPE_FIT, STOP_EVENT,
                SCALETYPE_EVENT, media.VideoScaleType.VIDEO_SCALE_TYPE_FIT_CROP, ERROR_EVENT, CHECK_PARAMETER, 'videoScaeType', media.VideoScaleType.VIDEO_SCALE_TYPE_FIT,
                RESET_EVENT, SCALETYPE_EVENT, media.VideoScaleType.VIDEO_SCALE_TYPE_FIT_CROP, ERROR_EVENT, CHECK_PARAMETER, 'videoScaeType', media.VideoScaleType.VIDEO_SCALE_TYPE_FIT, END_EVENT);
            // audioInterruptMode
            let mySteps06 = new Array(CREATE_EVENT, CHECK_PARAMETER, 'audioInterruptMode', audio.InterruptMode.SHARE_MODE, INTERRUPTMODE_EVENT, audio.InterruptMode.INDEPENDENT_MODE, ERROR_EVENT,
                CHECK_PARAMETER, 'audioInterruptMode', audio.InterruptMode.SHARE_MODE, SETSOURCE_EVENT, fdPath, INTERRUPTMODE_EVENT, audio.InterruptMode.INDEPENDENT_MODE, ERROR_EVENT,
                CHECK_PARAMETER, 'audioInterruptMode', audio.InterruptMode.SHARE_MODE, SETSURFACE_EVENT, surfaceID, INTERRUPTMODE_EVENT, audio.InterruptMode.INDEPENDENT_MODE, ERROR_EVENT,
                CHECK_PARAMETER, 'audioInterruptMode', audio.InterruptMode.SHARE_MODE, PREPARE_EVENT, INTERRUPTMODE_EVENT, audio.InterruptMode.INDEPENDENT_MODE,
                CHECK_PARAMETER, 'audioInterruptMode', audio.InterruptMode.INDEPENDENT_MODE, PLAY_EVENT, INTERRUPTMODE_EVENT, audio.InterruptMode.SHARE_MODE,
                CHECK_PARAMETER, 'audioInterruptMode', audio.InterruptMode.SHARE_MODE, PAUSE_EVENT, INTERRUPTMODE_EVENT, audio.InterruptMode.INDEPENDENT_MODE,
                CHECK_PARAMETER, 'audioInterruptMode', audio.InterruptMode.INDEPENDENT_MODE, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT,
                INTERRUPTMODE_EVENT, audio.InterruptMode.SHARE_MODE, CHECK_PARAMETER, 'audioInterruptMode', audio.InterruptMode.SHARE_MODE, STOP_EVENT,
                INTERRUPTMODE_EVENT, audio.InterruptMode.INDEPENDENT_MODE, ERROR_EVENT, CHECK_PARAMETER, 'audioInterruptMode', audio.InterruptMode.SHARE_MODE,
                RESET_EVENT, INTERRUPTMODE_EVENT, audio.InterruptMode.INDEPENDENT_MODE, ERROR_EVENT, CHECK_PARAMETER, 'audioInterruptMode', audio.InterruptMode.SHARE_MODE, END_EVENT);
            // audioRenderInfo
            let mySteps07 = new Array(CREATE_EVENT, CHECK_PARAMETER, 'audioRenderInfo', [2, 1, 0], AUDIORENDERINFO_EVENT, [1, 2, 0], ERROR_EVENT, CHECK_PARAMETER, 'audioRenderInfo', [2, 1, 0],
                SETSOURCE_EVENT, fdPath, AUDIORENDERINFO_EVENT, [1, 2, 0], CHECK_PARAMETER, 'audioRenderInfo', [1, 2, 0], SETSURFACE_EVENT, surfaceID, AUDIORENDERINFO_EVENT, [2, 3, 0],
                CHECK_PARAMETER, 'audioRenderInfo', [2, 3, 0], PREPARE_EVENT, AUDIORENDERINFO_EVENT, [1, 2, 1], ERROR_EVENT, CHECK_PARAMETER, 'audioRenderInfo', [2, 3, 0],
                PLAY_EVENT, AUDIORENDERINFO_EVENT, [1, 2, 1], ERROR_EVENT, CHECK_PARAMETER, 'audioRenderInfo', [2, 3, 0],
                PAUSE_EVENT, AUDIORENDERINFO_EVENT, [1, 2, 1], ERROR_EVENT, CHECK_PARAMETER, 'audioRenderInfo', [2, 3, 0], PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT,
                AUDIORENDERINFO_EVENT, [1, 2, 1], ERROR_EVENT, CHECK_PARAMETER, 'audioRenderInfo', [2, 3, 0], STOP_EVENT, AUDIORENDERINFO_EVENT, [1, 2, 1], ERROR_EVENT, CHECK_PARAMETER, 'audioRenderInfo', [2, 3, 0],
                RESET_EVENT, AUDIORENDERINFO_EVENT, [1, 2, 1], ERROR_EVENT, CHECK_PARAMETER, 'audioRenderInfo', [2, 3, 0], RELEASE_EVENT, END_EVENT);
            // state
            let mySteps08 = new Array(CREATE_EVENT, CHECK_PARAMETER, 'state', 'idle', SETSOURCE_EVENT, fdPath, CHECK_PARAMETER, 'state', 'initialized',
                SETSURFACE_EVENT, surfaceID, CHECK_PARAMETER, 'state', 'initialized', PREPARE_EVENT, CHECK_PARAMETER, 'state', 'prepared', PLAY_EVENT,
                CHECK_PARAMETER, 'state', 'playing', PAUSE_EVENT, CHECK_PARAMETER, 'state', 'paused', PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT,
                CHECK_PARAMETER, 'state', 'completed', STOP_EVENT, CHECK_PARAMETER, 'state', 'stopped', RESET_EVENT, CHECK_PARAMETER, 'state', 'idle',
                SETSOURCE_EVENT, errorFdPath, PREPARE_EVENT, ERROR_EVENT, ERROR_EVENT, CHECK_PARAMETER, 'state', 'error', RELEASE_EVENT, CHECK_PARAMETER, 'state', 'released', END_EVENT); 
            // time
            let mySteps09 = new Array(CREATE_EVENT, CHECK_PARAMETER, 'time', [-1, -1], SETSOURCE_EVENT, fdPath, CHECK_PARAMETER, 'time', [-1, -1], SETSURFACE_EVENT, surfaceID,
                CHECK_PARAMETER, 'time', [-1, -1], PREPARE_EVENT, CHECK_PARAMETER, 'time', [0, videoInfor[0].duration], PLAY_EVENT, CHECK_PARAMETER, 'time', [PLAY_TIME, videoInfor[0].duration],
                PAUSE_EVENT, CHECK_PARAMETER, 'time', [PLAY_TIME, videoInfor[0].duration], PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT,
                CHECK_PARAMETER, 'time', [videoInfor[0].duration, videoInfor[0].duration], STOP_EVENT, CHECK_PARAMETER, 'time', [-1, -1],
                RESET_EVENT, CHECK_PARAMETER, 'time', [-1, -1], RELEASE_EVENT, END_EVENT);
            // size
            let mySteps10 = new Array(CREATE_EVENT, CHECK_PARAMETER, 'size', [0, 0], SETSOURCE_EVENT, fdPath, CHECK_PARAMETER, 'size', [0, 0], SETSURFACE_EVENT, surfaceID,
                CHECK_PARAMETER, 'size', [0, 0], PREPARE_EVENT, CHECK_PARAMETER, 'size', [videoInfor[0].width, videoInfor[0].height], PLAY_EVENT, CHECK_PARAMETER, 'size', [videoInfor[0].width, videoInfor[0].height],
                PAUSE_EVENT, CHECK_PARAMETER, 'size', [videoInfor[0].width, videoInfor[0].height], PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT,
                CHECK_PARAMETER, 'size', [videoInfor[0].width, videoInfor[0].height], STOP_EVENT, CHECK_PARAMETER, 'size', [0, 0],
                RESET_EVENT, CHECK_PARAMETER, 'size', [0, 0], RELEASE_EVENT, END_EVENT);            

            myStepsList = new Array(mySteps01, mySteps02, mySteps03, mySteps04, mySteps05,
                                    mySteps06, mySteps07, mySteps08, mySteps09, mySteps10);

            eventEmitter.on('test_parameter', () => {   
                console.info(TAG + '**************************test video or audio name is :'+ videoNameList[videoCount] +'*****************');
                console.info(TAG + '**************************this is function parameter test: ' + (testCount + 1) + ' testcase start****************');
                let avPlayer = null;
                eventEmitter.emit(myStepsList[testCount][stepCount], avPlayer, myStepsList[testCount], done);
                videoCount = videoNameList.length - 1;
                testCount++; 
            });
            eventEmitter.emit(testTag);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_STATE_INITIALIZED
            * @tc.name      : 001.test initialized stateChange
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_STATE_INITIALIZED', 0, async function (done) {
            testTag = 'test_initialized';
            // initialized -> release (released)
            let mySteps01 = new Array(CREATE_EVENT, SETFDSRC_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> reset -> setSource (initialized)
            let mySteps02 = new Array(CREATE_EVENT, SETFDSRC_EVENT, RESET_EVENT, SETSOURCE_EVENT, fdPath, RELEASE_EVENT, END_EVENT);
            // initialized -> reset -> setFdSource (initialized)
            let mySteps03 = new Array(CREATE_EVENT, SETFDSRC_EVENT, RESET_EVENT, SETFDSRC_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> reset -> released (released)
            let mySteps04 = new Array(CREATE_EVENT, SETFDSRC_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> reset -> prepare  (error)
            let mySteps05 = new Array(CREATE_EVENT, SETFDSRC_EVENT, RESET_EVENT, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> reset -> play  (error)
            let mySteps06 = new Array(CREATE_EVENT, SETFDSRC_EVENT, RESET_EVENT, PLAY_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> reset -> pause  (error)
            let mySteps07 = new Array(CREATE_EVENT, SETFDSRC_EVENT, RESET_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> reset -> stop  (error)
            let mySteps08 = new Array(CREATE_EVENT, SETFDSRC_EVENT, RESET_EVENT, STOP_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> reset -> seek  (error)
            let mySteps09 = new Array(CREATE_EVENT, SETFDSRC_EVENT, RESET_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> reset -> setsurfaceID  (error)
            let mySteps10 = new Array(CREATE_EVENT, SETFDSRC_EVENT, RESET_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> reset -> reset  (idle)
            let mySteps11 = new Array(CREATE_EVENT, SETFDSRC_EVENT, RESET_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> surfaceID -> release  (released)
            let mySteps12 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, RELEASE_EVENT, END_EVENT);
            // initialized -> surfaceID -> reset  (idle)
            let mySteps13 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> surfaceID -> surfaceID  (initialized)
            let mySteps14 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, SETSURFACE_EVENT, surfaceID, RELEASE_EVENT, END_EVENT);
            // initialized -> surfaceID -> prepare  (prepared)
            let mySteps15 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> surfaceID -> url  (error)
            let mySteps16 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> surfaceID -> play  (error)
            let mySteps17 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PLAY_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> surfaceID -> pause  (error)
            let mySteps18 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> surfaceID -> stop  (error)
            let mySteps19 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, STOP_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);            
            // initialized -> surfaceID -> seek  (error)
            let mySteps20 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> prepare -> release  (released)
            let mySteps21 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> prepare -> reset  (idle)
            let mySteps22 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> prepare -> seek  (prepared)
            let mySteps23 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, RELEASE_EVENT, END_EVENT);
            // initialized -> prepare -> play  (playing)
            let mySteps24 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> prepare -> stop  (stopped)
            let mySteps25 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> prepare -> url  (error)
            let mySteps26 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> prepare -> pause  (error)
            let mySteps27 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> prepare -> surfaceID  (error)
            let mySteps28 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> prepare -> prepare  (prepared)
            let mySteps29 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PREPARE_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> url(error) -> release  (released)
            let mySteps30 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSOURCE_EVENT, '', ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> url(error) -> reset  (idle)
            let mySteps31 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSOURCE_EVENT, '', ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> play(error) -> release  (released)
            let mySteps32 = new Array(CREATE_EVENT, SETFDSRC_EVENT, PLAY_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> play(error) -> reset  (idle)
            let mySteps33 = new Array(CREATE_EVENT, SETFDSRC_EVENT, PLAY_EVENT, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> pause(error) -> release  (released)
            let mySteps34 = new Array(CREATE_EVENT, SETFDSRC_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> pause(error) -> reset  (idle)
            let mySteps35 = new Array(CREATE_EVENT, SETFDSRC_EVENT, PAUSE_EVENT, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> stop(error) -> release  (released)
            let mySteps36 = new Array(CREATE_EVENT, SETFDSRC_EVENT, STOP_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> stop(error) -> reset  (idle)
            let mySteps37 = new Array(CREATE_EVENT, SETFDSRC_EVENT, STOP_EVENT, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> seek(error) -> release  (released)
            let mySteps38 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // initialized -> seek(error) -> reset  (idle)
            let mySteps39 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);

            myStepsList = new Array(mySteps01, mySteps02, mySteps03, mySteps04, mySteps05,
                                    mySteps06, mySteps07, mySteps08, mySteps09, mySteps10,
                                    mySteps11, mySteps12, mySteps13, mySteps14, mySteps15,
                                    mySteps16, mySteps17, mySteps18, mySteps19, mySteps20,
                                    mySteps21, mySteps22, mySteps23, mySteps24, mySteps25,
                                    mySteps26, mySteps27, mySteps28, mySteps29, mySteps30,
                                    mySteps31, mySteps32, mySteps33, mySteps34, mySteps35,
                                    mySteps36, mySteps37, mySteps38, mySteps39);
            eventEmitter.on('test_initialized', () => {
                console.info(TAG + '**************************test video or audio name is :'+ videoNameList[videoCount] +'*****************');
                console.info(TAG + '**************************this is play api test: ' + (testCount + 1) + ' testcase start****************');
                let avPlayer = null;
                eventEmitter.emit(myStepsList[testCount][stepCount], avPlayer, myStepsList[testCount], done);
                testCount++;
            });
            eventEmitter.emit(testTag);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_STATE_PREPARED
            * @tc.name      : 001.test prepared stateChange
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_STATE_PREPARED', 0, async function (done) {
            testTag = 'test_prepared';
            // prepared -> release (released)
            let mySteps01 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> reset -> setSource (initialized)
            let mySteps02 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, RESET_EVENT, SETSOURCE_EVENT, fdPath, RELEASE_EVENT, END_EVENT);
            // prepared -> reset -> setFdSource (initialized)
            let mySteps03 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, RESET_EVENT, SETFDSRC_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> reset -> released (released)
            let mySteps04 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> reset -> prepare  (error)
            let mySteps05 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, RESET_EVENT, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> reset -> play  (error)
            let mySteps06 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, RESET_EVENT, PLAY_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> reset -> pause  (error)
            let mySteps07 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, RESET_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> reset -> stop  (error)
            let mySteps08 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, RESET_EVENT, STOP_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> reset -> seek  (error)
            let mySteps09 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, RESET_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> reset -> setsurfaceID  (error)
            let mySteps10 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, RESET_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> reset -> reset  (idle)
            let mySteps11 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, RESET_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);            
            // prepared -> seek -> release  (released)
            let mySteps12 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, RELEASE_EVENT, END_EVENT);
            // prepared -> seek -> reset  (idle)
            let mySteps13 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> seek -> seek  (prepared)
            let mySteps14 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, SEEK_EVENT, 8000, media.SeekMode.SEEK_NEXT_SYNC, RELEASE_EVENT, END_EVENT);
            // prepared -> seek -> play  (playing)
            let mySteps15 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> seek -> stop (stopped)
            let mySteps16 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> seek -> setSource  (error)
            let mySteps17 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> seek -> pause  (error)
            let mySteps18 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> seek -> surfaceID (error)
            let mySteps19 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);            
            // prepared -> seek -> prepared (error)
            let mySteps20 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, PREPARE_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> play -> release  (released)
            let mySteps21 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> play -> reset  (idle)
            let mySteps22 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> play -> seek  (prepared)
            let mySteps23 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, RELEASE_EVENT, END_EVENT);
            // prepared -> play(Loop = 1) -> eos (playing)
            let mySteps24 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, LOOP_EVENT, true, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> play -> stop  (stopped)
            let mySteps25 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> play -> pause  (paused)
            let mySteps26 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> play(loop = 0) -> eos (completed)
            let mySteps27 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, LOOP_EVENT, false, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> play -> setSource  (error)
            let mySteps28 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> play -> prepare  (error)
            let mySteps29 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> play -> surfaceid  (error)
            let mySteps30 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> play -> play  (playing)
            let mySteps31 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> stop -> release  (released)
            let mySteps32 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> stop -> reset  (idle)
            let mySteps33 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> stop -> prepare  (prepared)
            let mySteps34 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, PREPARE_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> stop -> setSource  (error)
            let mySteps35 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> stop -> play  (error)
            let mySteps36 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, PLAY_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> stop -> pause  (error)
            let mySteps37 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> stop -> seek  (error)
            let mySteps38 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> stop -> surfaceid  (error)
            let mySteps39 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> stop -> stop  (stopped)
            let mySteps40 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> url -> release  (released)
            let mySteps41 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> url -> reset  (idle)
            let mySteps42 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> pause -> release  (released)
            let mySteps43 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> pause -> reset  (idle)
            let mySteps44 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PAUSE_EVENT, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> surfaceid -> release  (released)
            let mySteps45 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> surfaceid -> reset  (idle)
            let mySteps46 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> prepared -> release  (released)
            let mySteps47 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PREPARE_EVENT, RELEASE_EVENT, END_EVENT);
            // prepared -> prepared -> reset  (idle)
            let mySteps48 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PREPARE_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);

            myStepsList = new Array(mySteps01, mySteps02, mySteps03, mySteps04, mySteps05,
                                    mySteps06, mySteps07, mySteps08, mySteps09, mySteps10,
                                    mySteps11, mySteps12, mySteps13, mySteps14, mySteps15,
                                    mySteps16, mySteps17, mySteps18, mySteps19, mySteps20,
                                    mySteps21, mySteps22, mySteps23, mySteps24, mySteps25,
                                    mySteps26, mySteps27, mySteps28, mySteps29, mySteps30,
                                    mySteps31, mySteps32, mySteps33, mySteps34, mySteps35,
                                    mySteps36, mySteps37, mySteps38, mySteps39, mySteps40,
                                    mySteps41, mySteps42, mySteps43, mySteps44, mySteps45,
                                    mySteps46, mySteps47, mySteps48);
            eventEmitter.on('test_prepared', () => {
                console.info(TAG + '**************************test video or audio name is :'+ videoNameList[videoCount] +'*****************');
                console.info(TAG + '**************************this is state prepared test: ' + (testCount + 1) + ' testcase start****************');
                let avPlayer = null;
                eventEmitter.emit(myStepsList[testCount][stepCount], avPlayer, myStepsList[testCount], done);
                testCount++;
            });
            eventEmitter.emit(testTag);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_STATE_PLAYING
            * @tc.name      : 001.test playing stateChange
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_STATE_PLAYING', 0, async function (done) {
            testTag = 'test_playing';
            // playing -> release (released)
            let mySteps01 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> reset -> setSource (initialized)
            let mySteps02 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, RESET_EVENT, SETSOURCE_EVENT, fdPath, RELEASE_EVENT, END_EVENT);
            // playing -> reset -> setFdSource (initialized)
            let mySteps03 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, RESET_EVENT, SETFDSRC_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> reset -> released (released)
            let mySteps04 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> reset -> prepare  (error)
            let mySteps05 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, RESET_EVENT, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> reset -> play  (error)
            let mySteps06 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, RESET_EVENT, PLAY_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> reset -> pause  (error)
            let mySteps07 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, RESET_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> reset -> stop  (error)
            let mySteps08 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, RESET_EVENT, STOP_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> reset -> seek  (error)
            let mySteps09 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, RESET_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> reset -> setsurfaceID  (error)
            let mySteps10 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, RESET_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> reset -> reset  (idle)
            let mySteps11 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, RESET_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);            
            // playing -> seek -> release  (released)
            let mySteps12 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, RELEASE_EVENT, END_EVENT);
            // playing -> seek -> reset  (idle)
            let mySteps13 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> seek -> seek  (prepared)
            let mySteps14 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, SEEK_EVENT, 8000, media.SeekMode.SEEK_NEXT_SYNC, RELEASE_EVENT, END_EVENT);
            // playing -> seek (loop = 1) -> eos(playing)
            let mySteps15 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, LOOP_EVENT, true, FINISH_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> seek -> stop (stopped)
            let mySteps16 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> seek -> pause  (paused)
            let mySteps17 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, PAUSE_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> seek(loop = 0) -> eos  (completed)
            let mySteps18 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, LOOP_EVENT, false, FINISH_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> seek -> url (error)
            let mySteps19 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);            
            // playing -> seek -> prepared (error)
            let mySteps20 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> seek -> surfaceid  (error)
            let mySteps21 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> seek -> play  (playing)
            let mySteps22 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            // playing(loop = 1) -> eos(playing) -> release  (released)
            let mySteps23 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, LOOP_EVENT, true, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RELEASE_EVENT, END_EVENT);
            // playing(loop = 1) -> eos(playing) -> reset  (idle)
            let mySteps24 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, LOOP_EVENT, true, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // playing(loop = 1) -> eos(playing) -> seek  (playing)
            let mySteps25 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, LOOP_EVENT, true, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_PREV_SYNC, RELEASE_EVENT, END_EVENT);
            // playing(loop = 1) -> eos(playing) -> eos(playing)
            let mySteps26 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, LOOP_EVENT, true, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RELEASE_EVENT, END_EVENT);
            // playing(loop = 1) -> eos(playing) -> stop(stopped)
            let mySteps27 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, LOOP_EVENT, true, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // playing(loop = 1) -> eos(playing) -> pause(paused)
            let mySteps28 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, LOOP_EVENT, true, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, PAUSE_EVENT, RELEASE_EVENT, END_EVENT);
            // playing(loop = 1) -> eos(playing) loop = 0 -> eos(completed)
            let mySteps29 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, LOOP_EVENT, true, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT,  SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, LOOP_EVENT, false, FINISH_EVENT, RELEASE_EVENT, END_EVENT);
            // playing(loop = 1) -> eos(playing) -> url(error)
            let mySteps30 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, LOOP_EVENT, true, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing(loop = 1) -> eos(playing) -> prepare(error)
            let mySteps31 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, LOOP_EVENT, true, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing(loop = 1) -> eos(playing) -> surfaceid(error)
            let mySteps32 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, LOOP_EVENT, true, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing(loop = 1) -> eos(playing) -> play(playing)
            let mySteps33 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, LOOP_EVENT, true, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> stop -> release  (released)
            let mySteps34 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> stop -> reset  (idle)
            let mySteps35 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, STOP_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> stop -> prepare  (prepared)
            let mySteps36 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, STOP_EVENT, PREPARE_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> stop -> url  (error)
            let mySteps37 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, STOP_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> stop -> play  (error)
            let mySteps38 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, STOP_EVENT, PLAY_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> stop -> pause  (error)
            let mySteps39 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, STOP_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> stop -> seek  (error)
            let mySteps40 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, STOP_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> stop -> surfaceid  (error)
            let mySteps41 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, STOP_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> stop -> stop  (stopped)
            let mySteps42 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, STOP_EVENT, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> pause -> release  (released)
            let mySteps43 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> pause -> reset  (idle)
            let mySteps44 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> pause -> play  (playing)
            let mySteps45 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> pause -> seek  (paused)
            let mySteps46 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, RELEASE_EVENT, END_EVENT);
            // playing -> pause -> stop  (stopped)
            let mySteps47 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> pause -> url  (error)
            let mySteps48 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> pause -> prepare  (error)
            let mySteps49 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> pause -> surfaceid  (error)
            let mySteps50 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> pause -> pause  (paused)
            let mySteps51 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, PAUSE_EVENT, RELEASE_EVENT, END_EVENT);
            // playing(loop = 0) -> eos(completed) -> release  (released)
            let mySteps52 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RELEASE_EVENT, END_EVENT);
            // playing(loop = 0) -> eos(completed) -> reset  (idle)
            let mySteps53 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // playing(loop = 0) -> eos(completed) -> play  (playing)
            let mySteps54 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            // playing(loop = 0) -> eos(completed) -> stop  (stopped)
            let mySteps55 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // playing(loop = 0) -> eos(completed) -> url  (error)
            let mySteps56 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing(loop = 0) -> eos(completed) -> pause  (error)
            let mySteps57 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing(loop = 0) -> eos(completed) -> seek  (completed)
            let mySteps58 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, RELEASE_EVENT, END_EVENT);
            // playing(loop = 0) -> eos(completed) -> prepare  (error)
            let mySteps59 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing(loop = 0) -> eos(completed) -> surfaceid  (error)
            let mySteps60 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);            
            // playing -> url(error) -> release  (released)
            let mySteps61 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> url(error) -> reset  (idle)
            let mySteps62 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> prepare(error) -> release  (released)
            let mySteps63 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> prepare(error) -> reset  (idle)
            let mySteps64 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PREPARE_EVENT, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);            
            // playing -> surfaceid(error) -> release  (released)
            let mySteps65 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // playing -> surfaceid(error) -> reset  (idle)
            let mySteps66 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT); 
            // playing -> play
            let mySteps67 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PLAY_EVENT, RELEASE_EVENT, END_EVENT);

            myStepsList = new Array(mySteps01, mySteps02, mySteps03, mySteps04, mySteps05,
                                    mySteps06, mySteps07, mySteps08, mySteps09, mySteps10,
                                    mySteps11, mySteps12, mySteps13, mySteps14, mySteps15,
                                    mySteps16, mySteps17, mySteps18, mySteps19, mySteps20,
                                    mySteps21, mySteps22, mySteps23, mySteps24, mySteps25,
                                    mySteps26, mySteps27, mySteps28, mySteps29, mySteps30,
                                    mySteps31, mySteps32, mySteps33, mySteps34, mySteps35,
                                    mySteps36, mySteps37, mySteps38, mySteps39, mySteps40,
                                    mySteps41, mySteps42, mySteps43, mySteps44, mySteps45,
                                    mySteps46, mySteps47, mySteps48, mySteps49, mySteps50,
                                    mySteps51, mySteps52, mySteps53, mySteps54, mySteps55,
                                    mySteps56, mySteps57, mySteps58, mySteps59, mySteps60,
                                    mySteps61, mySteps62, mySteps63, mySteps64, mySteps65,
                                    mySteps66, mySteps67);
            eventEmitter.on('test_playing', () => {
                console.info(TAG + '**************************test video or audio name is :'+ videoNameList[videoCount] +'*****************');
                console.info(TAG + '**************************this is state playing test: ' + (testCount + 1) + ' testcase start****************');
                let avPlayer = null;
                eventEmitter.emit(myStepsList[testCount][stepCount], avPlayer, myStepsList[testCount], done);
                testCount++;
            });
            eventEmitter.emit(testTag);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_STATE_PAUSED
            * @tc.name      : 001.test paused stateChange
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_STATE_PAUSED', 0, async function (done) {
            testTag = 'test_paused';
            // paused -> release (released)
            let mySteps01 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> reset -> setSource (initialized)
            let mySteps02 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, RESET_EVENT, SETSOURCE_EVENT, fdPath, RELEASE_EVENT, END_EVENT);
            // paused -> reset -> setFdSource (initialized)
            let mySteps03 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, RESET_EVENT, SETFDSRC_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> reset -> released (released)
            let mySteps04 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> reset -> prepare  (error)
            let mySteps05 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, RESET_EVENT, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> reset -> play  (error)
            let mySteps06 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, RESET_EVENT, PLAY_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> reset -> pause  (error)
            let mySteps07 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, RESET_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> reset -> stop  (error)
            let mySteps08 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, RESET_EVENT, STOP_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> reset -> seek  (error)
            let mySteps09 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, RESET_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> reset -> setsurfaceID  (error)
            let mySteps10 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, RESET_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> reset -> reset  (idle)
            let mySteps11 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, RESET_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);            
            // paused -> seek -> release  (released)
            let mySteps12 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, RELEASE_EVENT, END_EVENT);
            // paused -> seek -> reset  (idle)
            let mySteps13 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> seek -> seek  (prepared)
            let mySteps14 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, SEEK_EVENT, 8000, media.SeekMode.SEEK_NEXT_SYNC, RELEASE_EVENT, END_EVENT);
            // paused -> seek -> play  (playing)
            let mySteps15 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> seek -> stop (stopped)
            let mySteps16 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> seek -> setSource  (error)
            let mySteps17 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> seek -> prepare  (error)
            let mySteps18 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> seek -> surfaceID (error)
            let mySteps19 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);            
            // paused -> seek -> pause (paused)
            let mySteps20 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, PAUSE_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> play -> release  (released)
            let mySteps21 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> play -> reset  (idle)
            let mySteps22 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, PLAY_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> play -> seek  (prepared)
            let mySteps23 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, PLAY_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, RELEASE_EVENT, END_EVENT);
            // paused -> play(Loop = 1) -> eos (playing)
            let mySteps24 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, LOOP_EVENT, true, PLAY_EVENT, PAUSE_EVENT, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> play -> stop  (stopped)
            let mySteps25 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, PLAY_EVENT, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> play -> pause  (paused)
            let mySteps26 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, PLAY_EVENT, PAUSE_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> play(loop = 0) -> eos (completed)
            let mySteps27 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, LOOP_EVENT, false, PLAY_EVENT, PAUSE_EVENT, PLAY_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> play -> setSource  (error)
            let mySteps28 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, PLAY_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> play -> prepare  (error)
            let mySteps29 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, PLAY_EVENT, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> play -> surfaceid  (error)
            let mySteps30 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, PLAY_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> play -> play  (playing)
            let mySteps31 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, PLAY_EVENT, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> stop -> release  (released)
            let mySteps32 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> stop -> reset  (idle)
            let mySteps33 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, STOP_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> stop -> prepare  (prepared)
            let mySteps34 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, STOP_EVENT, PREPARE_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> stop -> setSource  (error)
            let mySteps35 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, STOP_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> stop -> play  (error)
            let mySteps36 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, STOP_EVENT, PLAY_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> stop -> pause  (error)
            let mySteps37 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, STOP_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> stop -> seek  (error)
            let mySteps38 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, STOP_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> stop -> surfaceid  (error)
            let mySteps39 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, STOP_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> stop -> stop  (stopped)
            let mySteps40 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, STOP_EVENT, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> url -> release  (released)
            let mySteps41 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> url -> reset  (idle)
            let mySteps42 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> surfaceid -> release  (released)
            let mySteps43 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> surfaceid -> reset  (idle)
            let mySteps44 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> prepared -> release  (released)
            let mySteps45 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> prepared -> reset  (idle)
            let mySteps46 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, PREPARE_EVENT, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // paused -> pause (paused)
            let mySteps47 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT, PAUSE_EVENT, PAUSE_EVENT, RELEASE_EVENT, END_EVENT);


            myStepsList = new Array(mySteps01, mySteps02, mySteps03, mySteps04, mySteps05,
                                    mySteps06, mySteps07, mySteps08, mySteps09, mySteps10,
                                    mySteps11, mySteps12, mySteps13, mySteps14, mySteps15,
                                    mySteps16, mySteps17, mySteps18, mySteps19, mySteps20,
                                    mySteps21, mySteps22, mySteps23, mySteps24, mySteps25,
                                    mySteps26, mySteps27, mySteps28, mySteps29, mySteps30,
                                    mySteps31, mySteps32, mySteps33, mySteps34, mySteps35,
                                    mySteps36, mySteps37, mySteps38, mySteps39, mySteps40,
                                    mySteps41, mySteps42, mySteps43, mySteps44, mySteps45,
                                    mySteps46, mySteps47);
            eventEmitter.on('test_paused', () => {
                console.info(TAG + '**************************test video or audio name is :'+ videoNameList[videoCount] +'*****************');
                console.info(TAG + '**************************this is state paused test: ' + (testCount + 1) + ' testcase start****************');
                let avPlayer = null;
                eventEmitter.emit(myStepsList[testCount][stepCount], avPlayer, myStepsList[testCount], done);
                testCount++;
            });
            eventEmitter.emit(testTag);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_STATE_STOPPED
            * @tc.name      : 001.test stopped stateChange
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_STATE_STOPPED', 0, async function (done) {
            testTag = 'test_stopped';
            // stopped -> release (released)
            let mySteps01 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> reset -> setSource (initialized)
            let mySteps02 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, RESET_EVENT, SETSOURCE_EVENT, fdPath, RELEASE_EVENT, END_EVENT);
            // stopped -> reset -> setFdSource (initialized)
            let mySteps03 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, RESET_EVENT, SETFDSRC_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> reset -> released (released)
            let mySteps04 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> reset -> prepare  (error)
            let mySteps05 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, RESET_EVENT, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> reset -> play  (error)
            let mySteps06 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, RESET_EVENT, PLAY_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> reset -> pause  (error)
            let mySteps07 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, RESET_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> reset -> stop  (error)
            let mySteps08 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, RESET_EVENT, STOP_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> reset -> seek  (error)
            let mySteps09 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, RESET_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> reset -> setsurfaceID  (error)
            let mySteps10 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, RESET_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> reset -> reset  (idle)
            let mySteps11 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, RESET_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);            
            // stopped -> prepare -> release  (released)
            let mySteps12 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, PREPARE_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> prepare -> reset  (idle)
            let mySteps13 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, PREPARE_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> prepare -> seek  (prepared)
            let mySteps14 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, PREPARE_EVENT, SEEK_EVENT, 8000, media.SeekMode.SEEK_NEXT_SYNC, RELEASE_EVENT, END_EVENT);
            // stopped -> prepare -> play  (playing)
            let mySteps15 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, PREPARE_EVENT, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> prepare -> stop (stopped)
            let mySteps16 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, PREPARE_EVENT, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> prepare -> setSource  (error)
            let mySteps17 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, PREPARE_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> prepare -> pause  (error)
            let mySteps18 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, PREPARE_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> prepare -> surfaceID (error)
            let mySteps19 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, PREPARE_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);            
            // stopped -> prepare -> prepare (prepared)
            let mySteps20 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, PREPARE_EVENT, PREPARE_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> url -> release  (released)
            let mySteps21 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> url -> reset  (idle)
            let mySteps22 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);            
            // stopped -> play -> release  (released)
            let mySteps23 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, PLAY_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> play -> reset (idle)
            let mySteps24 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, PLAY_EVENT, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> pause -> release  (released)
            let mySteps25 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> pause -> reset  (idle)
            let mySteps26 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, PAUSE_EVENT, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);            
            // stopped -> seek -> release  (released)
            let mySteps27 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_PREV_SYNC, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> seek -> reset  (idle)
            let mySteps28 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_PREV_SYNC, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);            
            // stopped -> surfaceID -> release  (released)
            let mySteps29 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> surfaceID -> reset  (idle)
            let mySteps30 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // stopped -> stop
            let mySteps31 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, STOP_EVENT, STOP_EVENT, RELEASE_EVENT, END_EVENT);

            myStepsList = new Array(mySteps01, mySteps02, mySteps03, mySteps04, mySteps05,
                                    mySteps06, mySteps07, mySteps08, mySteps09, mySteps10,
                                    mySteps11, mySteps12, mySteps13, mySteps14, mySteps15,
                                    mySteps16, mySteps17, mySteps18, mySteps19, mySteps20,
                                    mySteps21, mySteps22, mySteps23, mySteps24, mySteps25,
                                    mySteps26, mySteps27, mySteps28, mySteps29, mySteps30,
                                    mySteps31);
            eventEmitter.on('test_stopped', () => {
                console.info(TAG + '**************************test video or audio name is :'+ videoNameList[videoCount] +'*****************');
                console.info(TAG + '**************************this is state stopped test: ' + (testCount + 1) + ' testcase start****************');
                let avPlayer = null;
                eventEmitter.emit(myStepsList[testCount][stepCount], avPlayer, myStepsList[testCount], done);
                testCount++;
            });
            eventEmitter.emit(testTag);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_STATE_COMPLETED
            * @tc.name      : 001.test completed stateChange
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */ 
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_STATE_COMPLETED', 0, async function (done) {
            testTag = 'test_completed';
            // completed -> release (released)
            let mySteps01 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> reset -> setSource (initialized)
            let mySteps02 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RESET_EVENT, SETSOURCE_EVENT, fdPath, RELEASE_EVENT, END_EVENT);
            // completed -> reset -> setFdSource (initialized)
            let mySteps03 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RESET_EVENT, SETFDSRC_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> reset -> released (released)
            let mySteps04 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> reset -> prepare  (error)
            let mySteps05 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RESET_EVENT, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> reset -> play  (error)
            let mySteps06 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RESET_EVENT, PLAY_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> reset -> pause  (error)
            let mySteps07 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RESET_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> reset -> stop  (error)
            let mySteps08 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RESET_EVENT, STOP_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> reset -> seek  (error)
            let mySteps09 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RESET_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> reset -> setsurfaceID  (error)
            let mySteps10 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RESET_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> reset -> reset  (idle)
            let mySteps11 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, RESET_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> play -> release  (released)
            let mySteps12 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> play -> reset  (idle)
            let mySteps13 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, PLAY_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> play -> seek  (prepared)
            let mySteps14 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, PLAY_EVENT, SEEK_EVENT, 8000, media.SeekMode.SEEK_NEXT_SYNC, RELEASE_EVENT, END_EVENT);
            // completed -> play(loop = 1) -> eos  (playing)
            let mySteps15 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, LOOP_EVENT, true, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> play -> stop (stopped)
            let mySteps16 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, PLAY_EVENT, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> play(loop = 0) -> eos  (completed)
            let mySteps17 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, LOOP_EVENT, false, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> play -> url  (error)
            let mySteps18 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, PLAY_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> play -> prepare (error)
            let mySteps19 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, PLAY_EVENT, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> play -> surfaceID (error)
            let mySteps20 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, PLAY_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> play -> play  (playing)
            let mySteps21 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, PLAY_EVENT, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> stop -> release  (released)
            let mySteps22 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> stop -> reset  (idle)
            let mySteps23 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, STOP_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> stop -> prepare (prepared)
            let mySteps24 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, STOP_EVENT, PREPARE_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> stop -> url  (error)
            let mySteps25 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, STOP_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> stop -> play  (error)
            let mySteps26 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, STOP_EVENT, PLAY_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> stop -> pause  (error)
            let mySteps27 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, STOP_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> stop -> seek  (error)
            let mySteps28 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, STOP_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_PREV_SYNC, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> stop -> surfaceID  (error)
            let mySteps29 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, STOP_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> stop -> stop  (stopped)
            let mySteps30 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, STOP_EVENT, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> url -> release (released)
            let mySteps31 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> url -> reset (idle)
            let mySteps32 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> pause -> release (released)
            let mySteps33 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> pause -> reset (idle)
            let mySteps34 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, PAUSE_EVENT, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> seek -> release (released)
            let mySteps35 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_PREV_SYNC, RELEASE_EVENT, END_EVENT);
            // completed -> seek -> reset (idle)
            let mySteps36 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_PREV_SYNC, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> seek -> play (playing)
            let mySteps37 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_PREV_SYNC, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> seek -> stop (stopped)
            let mySteps38 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_PREV_SYNC, STOP_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> seek -> url (error)
            let mySteps39 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_PREV_SYNC, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> seek -> pause (error)
            let mySteps40 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_PREV_SYNC, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> seek -> seek (completed)
            let mySteps41 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_PREV_SYNC, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, RELEASE_EVENT, END_EVENT);
            // completed -> seek -> prepare (error)
            let mySteps42 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_PREV_SYNC, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> seek -> surfaceID (error)
            let mySteps43 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_PREV_SYNC, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> prepare -> release (released)
            let mySteps44 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> prepare -> reset (idle)
            let mySteps45 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, PREPARE_EVENT, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> surfaceID -> release (released)
            let mySteps46 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // completed -> surfaceID -> reset (idle)
            let mySteps47 = new Array(CREATE_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, PLAY_EVENT,
                SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, FINISH_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);

            myStepsList = new Array(mySteps01, mySteps02, mySteps03, mySteps04, mySteps05,
                                    mySteps06, mySteps07, mySteps08, mySteps09, mySteps10,
                                    mySteps11, mySteps12, mySteps13, mySteps14, mySteps15,
                                    mySteps16, mySteps17, mySteps18, mySteps19, mySteps20,
                                    mySteps21, mySteps22, mySteps23, mySteps24, mySteps25,
                                    mySteps26, mySteps27, mySteps28, mySteps29, mySteps30,
                                    mySteps31, mySteps32, mySteps33, mySteps34, mySteps35,
                                    mySteps36, mySteps37, mySteps38, mySteps39, mySteps40,
                                    mySteps41, mySteps42, mySteps43, mySteps44, mySteps45,
                                    mySteps46, mySteps47);
            eventEmitter.on('test_completed', () => {
                console.info(TAG + '**************************test video or audio name is :' + videoNameList[videoCount] + '*****************');
                console.info(TAG + '**************************this is state completed test: ' + (testCount + 1) + ' testcase start****************');
                let avPlayer = null;
                eventEmitter.emit(myStepsList[testCount][stepCount], avPlayer, myStepsList[testCount], done);
                testCount++;
            });
            eventEmitter.emit(testTag);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_STATE_ERROR
            * @tc.name      : 001.test error stateChange
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_STATE_ERROR', 0, async function (done) {
            testTag = 'test_error';
            // error -> release (released)
            let mySteps01 = new Array(CREATE_EVENT, SETSOURCE_EVENT, errorFdPath, PREPARE_EVENT, ERROR_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // error -> reset -> setSource (initialized)
            let mySteps02 = new Array(CREATE_EVENT, SETSOURCE_EVENT, errorFdPath, PREPARE_EVENT, ERROR_EVENT, ERROR_EVENT, RESET_EVENT, SETSOURCE_EVENT, fdPath, RELEASE_EVENT, END_EVENT);
            // error -> reset -> setFdSource (initialized)
            let mySteps03 = new Array(CREATE_EVENT, SETSOURCE_EVENT, errorFdPath, PREPARE_EVENT, ERROR_EVENT, ERROR_EVENT, RESET_EVENT, SETFDSRC_EVENT, RELEASE_EVENT, END_EVENT);
            // error -> reset -> released (released)
            let mySteps04 = new Array(CREATE_EVENT, SETSOURCE_EVENT, errorFdPath, PREPARE_EVENT, ERROR_EVENT, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // error -> reset -> prepare  (error)
            let mySteps05 = new Array(CREATE_EVENT, SETSOURCE_EVENT, errorFdPath, PREPARE_EVENT, ERROR_EVENT, ERROR_EVENT, RESET_EVENT, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // error -> reset -> play  (error)
            let mySteps06 = new Array(CREATE_EVENT, SETSOURCE_EVENT, errorFdPath, PREPARE_EVENT, ERROR_EVENT, ERROR_EVENT, RESET_EVENT, PLAY_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // error -> reset -> pause  (error)
            let mySteps07 = new Array(CREATE_EVENT, SETSOURCE_EVENT, errorFdPath, PREPARE_EVENT, ERROR_EVENT, ERROR_EVENT, RESET_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // error -> reset -> stop  (error)
            let mySteps08 = new Array(CREATE_EVENT, SETSOURCE_EVENT, errorFdPath, PREPARE_EVENT, ERROR_EVENT, ERROR_EVENT, RESET_EVENT, STOP_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // error -> reset -> seek  (error)
            let mySteps09 = new Array(CREATE_EVENT, SETSOURCE_EVENT, errorFdPath, PREPARE_EVENT, ERROR_EVENT, ERROR_EVENT, RESET_EVENT, SEEK_EVENT, 5000, media.SeekMode.SEEK_NEXT_SYNC, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // error -> reset -> setsurfaceID  (error)
            let mySteps10 = new Array(CREATE_EVENT, SETSOURCE_EVENT, errorFdPath, PREPARE_EVENT, ERROR_EVENT, ERROR_EVENT, RESET_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // error -> reset -> reset  (idle)
            let mySteps11 = new Array(CREATE_EVENT, SETSOURCE_EVENT, errorFdPath, PREPARE_EVENT, ERROR_EVENT, ERROR_EVENT, RESET_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);            

            myStepsList = new Array(mySteps01, mySteps02, mySteps03, mySteps04, mySteps05,
                                    mySteps06, mySteps07, mySteps08, mySteps09, mySteps10,
                                    mySteps11);
            eventEmitter.on('test_error', () => {
                console.info(TAG + '**************************test video or audio name is :'+ videoNameList[videoCount] +'*****************');
                console.info(TAG + '**************************this is state error test: ' + (testCount + 1) + ' testcase start****************');
                let avPlayer = null;
                eventEmitter.emit(myStepsList[testCount][stepCount], avPlayer, myStepsList[testCount], done);
                testCount++;
            });
            eventEmitter.emit(testTag);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_STATE_IDLE
            * @tc.name      : 001.test idle stateChange
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_VIDEO_PLAYER_STATE_IDLE', 0, async function (done) {
            testTag = 'test_idle';
            // idle -> url -> release (released)
            let mySteps01 = new Array(CREATE_EVENT, RESET_EVENT, SETSOURCE_EVENT, fdPath, RELEASE_EVENT, END_EVENT);
            // idle -> url -> reset  (idle)
            let mySteps02 = new Array(CREATE_EVENT, RESET_EVENT, SETSOURCE_EVENT, fdPath, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> url -> setsurfaceID (init)
            let mySteps03 = new Array(CREATE_EVENT, RESET_EVENT, SETSOURCE_EVENT, fdPath, SETSURFACE_EVENT, surfaceID, RELEASE_EVENT, END_EVENT);
            // idle -> url  -> prepare (prepared)
            let mySteps04 = new Array(CREATE_EVENT, RESET_EVENT, SETSOURCE_EVENT, fdPath, PREPARE_EVENT, PLAY_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> url -> url  (error)
            let mySteps05 = new Array(CREATE_EVENT, RESET_EVENT, SETSOURCE_EVENT, fdPath, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> url -> play  (error)
            let mySteps06 = new Array(CREATE_EVENT, RESET_EVENT, SETSOURCE_EVENT, fdPath, PLAY_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> url -> pause  (error)
            let mySteps07 = new Array(CREATE_EVENT, RESET_EVENT, SETSOURCE_EVENT, fdPath, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> url -> stop  (error)
            let mySteps08 = new Array(CREATE_EVENT, RESET_EVENT, SETSOURCE_EVENT, fdPath, STOP_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> url -> seek  (error)
            let mySteps09 = new Array(CREATE_EVENT, RESET_EVENT, SETSOURCE_EVENT, fdPath, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> fdSrc -> release (released)
            let mySteps10 = new Array(CREATE_EVENT, RESET_EVENT, SETFDSRC_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> fdSrc -> reset  (idle)
            let mySteps11 = new Array(CREATE_EVENT, RESET_EVENT, SETFDSRC_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> fdSrc -> setsurfaceID (init)
            let mySteps12 = new Array(CREATE_EVENT, RESET_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, RELEASE_EVENT, END_EVENT);
            // idle -> fdSrc  -> prepare (prepared)
            let mySteps13 = new Array(CREATE_EVENT, RESET_EVENT, SETFDSRC_EVENT, SETSURFACE_EVENT, surfaceID, PREPARE_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> fdSrc -> url  (error)
            let mySteps14 = new Array(CREATE_EVENT, RESET_EVENT, SETFDSRC_EVENT, SETSOURCE_EVENT, fdPath, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> fdSrc -> play  (error)
            let mySteps15 = new Array(CREATE_EVENT, RESET_EVENT, SETFDSRC_EVENT, PLAY_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> fdSrc -> pause  (error)
            let mySteps16 = new Array(CREATE_EVENT, RESET_EVENT, SETFDSRC_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> fdSrc -> stop  (error)
            let mySteps17 = new Array(CREATE_EVENT, RESET_EVENT, SETFDSRC_EVENT, STOP_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> fdSrc -> seek  (error)
            let mySteps18 = new Array(CREATE_EVENT, RESET_EVENT, SETFDSRC_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, ERROR_EVENT, RELEASE_EVENT, END_EVENT);            
            // idle -> release (released)
            let mySteps19 = new Array(CREATE_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> prepare -> release (released)
            let mySteps20 = new Array(CREATE_EVENT, RESET_EVENT, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> prepare -> reset  (idle)
            let mySteps21 = new Array(CREATE_EVENT, RESET_EVENT, PREPARE_EVENT, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> play -> release (released)
            let mySteps22 = new Array(CREATE_EVENT, RESET_EVENT, PLAY_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> play -> reset  (idle)
            let mySteps23 = new Array(CREATE_EVENT, RESET_EVENT, PLAY_EVENT, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> pause -> release (released)
            let mySteps24 = new Array(CREATE_EVENT, RESET_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> pause -> reset  (idle)
            let mySteps25 = new Array(CREATE_EVENT, RESET_EVENT, PAUSE_EVENT, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> stop -> release (released)
            let mySteps26 = new Array(CREATE_EVENT, RESET_EVENT, STOP_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> stop -> reset  (idle)
            let mySteps27 = new Array(CREATE_EVENT, RESET_EVENT, STOP_EVENT, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> seek -> release (released)
            let mySteps28 = new Array(CREATE_EVENT, RESET_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> seek -> reset  (idle)
            let mySteps29 = new Array(CREATE_EVENT, RESET_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> suraceID -> release (released)
            let mySteps30 = new Array(CREATE_EVENT, RESET_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> suraceID -> reset  (idle)
            let mySteps31 = new Array(CREATE_EVENT, RESET_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);            
            // idle -> reset -> url (init)
            let mySteps32 = new Array(CREATE_EVENT, RESET_EVENT, RESET_EVENT, SETSOURCE_EVENT, fdPath, RELEASE_EVENT, END_EVENT);
            // idle -> reset -> fdSrc (released)
            let mySteps33 = new Array(CREATE_EVENT, RESET_EVENT, RESET_EVENT, SETFDSRC_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> reset -> release (released)
            let mySteps34 = new Array(CREATE_EVENT, RESET_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> reset -> prepare (error)
            let mySteps35 = new Array(CREATE_EVENT, RESET_EVENT, RESET_EVENT, PREPARE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> reset -> play (error)
            let mySteps36 = new Array(CREATE_EVENT, RESET_EVENT, RESET_EVENT, PLAY_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> reset -> pause (error)
            let mySteps37 = new Array(CREATE_EVENT, RESET_EVENT, RESET_EVENT, PAUSE_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> reset -> stop (error)
            let mySteps38 = new Array(CREATE_EVENT, RESET_EVENT, RESET_EVENT, STOP_EVENT, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> reset -> seek (error)
            let mySteps39 = new Array(CREATE_EVENT, RESET_EVENT, RESET_EVENT, SEEK_EVENT, durationTag, media.SeekMode.SEEK_PREV_SYNC, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> reset -> surfaceID (error)
            let mySteps40 = new Array(CREATE_EVENT, RESET_EVENT, RESET_EVENT, SETSURFACE_EVENT, surfaceID, ERROR_EVENT, RELEASE_EVENT, END_EVENT);
            // idle -> reset -> reset (idle)
            let mySteps41 = new Array(CREATE_EVENT, RESET_EVENT, RESET_EVENT, RESET_EVENT, RELEASE_EVENT, END_EVENT);

            myStepsList = new Array(mySteps01, mySteps02, mySteps03, mySteps04, mySteps05,
                                    mySteps06, mySteps07, mySteps08, mySteps09, mySteps10,
                                    mySteps11, mySteps12, mySteps13, mySteps14, mySteps15,
                                    mySteps16, mySteps17, mySteps18, mySteps19, mySteps20,
                                    mySteps21, mySteps22, mySteps23, mySteps24, mySteps25,
                                    mySteps26, mySteps27, mySteps28, mySteps29, mySteps30,
                                    mySteps31, mySteps32, mySteps33, mySteps34, mySteps35,
                                    mySteps36, mySteps37, mySteps38, mySteps39, mySteps40,
                                    mySteps41);
            eventEmitter.on('test_idle', () => {
                console.info(TAG + '**************************test video or audio name is :' + videoNameList[videoCount] + '*****************');
                console.info(TAG + '**************************this is state idle test: ' + (testCount + 1) + ' testcase start****************');
                let avPlayer = null;
                eventEmitter.emit(myStepsList[testCount][stepCount], avPlayer, myStepsList[testCount], done);
                testCount++;
            });
            eventEmitter.emit(testTag);
        })
    })
}
