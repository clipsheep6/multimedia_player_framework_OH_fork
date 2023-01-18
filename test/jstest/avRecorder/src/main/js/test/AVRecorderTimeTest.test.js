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
import * as mediaTestBase from '../../../../../MediaTestBase.js';
import {describe, beforeAll, beforeEach, afterEach, afterAll, it, expect} from 'deccjsunit/index'

describe('RecorderLocalTestAudioFUNC', function () {
    const RECORDER_TIME = 3000;
    const PAUSE_TIME = 1000;

    const END = 0;
    const CREATE_PROMISE = 1;
    const CREATE_CALLBACK = 2;
    const PREPARE_PROMISE = 3;
    const PREPARE_CALLBACK = 4;
    const START = 5;
    const PAUSE = 6;
    const RESUME = 7;
    const STOP = 8;
    const RESET_PROMISE = 9;
    const RESET_CALLBACK = 10;
    const RELEASE_PROMISE = 11;
    const RELEASE_CALLBACK = 12;

    let mySteps = new Array();
    let caseCount = 0;
    let avRecorder = null;
    let count = 0;

    const CHANNEL_TWO = 2;
    const FORMAT_MP4 = media.AudioOutputFormat.MPEG_4;
    const FORMAT_M4A = media.AudioOutputFormat.AAC_ADTS;
    const ENCORDER_AACLC = media.AudioEncoder.AAC_LC;


    let fdPath;
    let fdObject;

    let avProfile = {
        audioBitrate : 48000,
        audioChannels : CHANNEL_TWO,
        audioCodec : media.CodecMimeType.AUDIO_AAC,
        audioSampleRate : 48000,
        fileFormat : media.ContainerFormatType.CFT_MPEG_4,
    }

    let avConfig = {
        audioSourceType : media.AudioSourceType.AUDIO_SOURCE_TYPE_MIC,
        profile : avProfile,
        url : 'fd://',
        rotation : 0,
        location : { latitude : 30, longitude : 130 }
    }

    beforeAll(function () {
        console.info('beforeAll case');
    })

    beforeEach(async function () {
        console.info('beforeEach case');
        avRecorder = undefined;
        count = 0;
        caseCount += 1;
        await getRecorderFileFd();
        console.info('beforeEach case');
    })

    afterEach(async function () {
        console.info('afterEach case In');
        await mediaTestBase.closeFd(fdObject.fileAsset, fdObject.fdNumber);
        mySteps = new Array();
        await releaseByPromise();
        console.info('afterEach case Out');
    })

    afterAll(function () {
        console.info('afterAll case');
    })

    async function getRecorderFileFd() {
        let fileName = 'audioRecorder_func_0'+ caseCount +'.m4a';
        console.info("case current fileName is: " + fileName);
        fdObject = await mediaTestBase.getAudioFd(fileName);
        fdPath = "fd://" + fdObject.fdNumber.toString();
        console.info("case fdPath is: " + fdPath);
        avConfig.url = fdPath;
        console.info("case to out getRecorderFileFd");
    }

    function printfError(error) {
        console.error(`case callback error called,errMessage is ${error.message}`);
    }

    async function createAVRecorderByPromise(done) {
        console.info(`case to create avRecorder by promise`);
        await media.createAVRecorder().then((recorder) => {
            console.info('case createAVRecorder promise called');
            if (typeof (recorder) != 'undefined') {
                avRecorder = recorder;
                setCallback(done);
                nextStep(done);
            } else {
                console.info('case create avRecorder failed!!!');
                expect().assertFail();
                done();
            }
        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
    }

    function createAVRecorderByCallback(done) {
        console.info(`case to create avRecorder by callback`);
        media.createAVRecorder((err, recorder) => {
            if (typeof (err) == 'undefined') {
                console.info('case createAVRecorder callback success ');
                avRecorder = recorder;
                setCallback(done);
                nextStep(done);
            } else {
                printfError(err);
            }
        });
    }

    async function prepareByPromise() {
        console.info(`case to prepare by promise`);
        await avRecorder.prepare(avConfig).then(() => {
            console.info('case recorder prepare by promise called');
        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
    }

    function prepareByCallback() {
        console.info(`case to prepare by callback`);
        avRecorder.prepare(avConfig, (err) => {
            if (typeof (err) == 'undefined') {
                console.info('case recorder prepare by callback called');
            } else {
                printfError(err);
            }
        });
    }

    async function resetByPromise() {
        console.info(`case to reset by promise`);
        await avRecorder.reset().then(() => {
            console.info('case recorder reset by promise called');
        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
    }

    function resetByCallback() {
        console.info(`case to reset by callback`);
        avRecorder.reset((err) => {
            if (typeof (err) == 'undefined') {
                console.info('case recorder reset by callback called');
            } else {
                printfError(err);
            }
        });
    }

    async function releaseByPromise() {
        console.info(`case to release by promise`);
        if (avRecorder) {
            await avRecorder.release().then(() => {
                console.info('case recorder release by promise called');
                avRecorder = undefined;
            }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
        }
    }

    async function releaseByCallback() {
        console.info(`case to release by callback`);
        if (avRecorder) {
            avRecorder.release(async(err) => {
                if (typeof (err) == 'undefined') {
                    console.info('case recorder release by callback called');
                    avRecorder = undefined;
                } else {
                    printfError(err);
                }
            });
        }
    }

    async function nextStep(done) {
        console.info("case myStep[0]: " + mySteps[0]);
        if (mySteps[0] == END) {
            console.info("case to END");
            done();
        }
        switch (mySteps[0]) {
            case CREATE_PROMISE:
                mySteps.shift();
                await createAVRecorderByPromise(done);
                break;
            case CREATE_CALLBACK:
                mySteps.shift();
                createAVRecorderByCallback(done);
                break;
            case PREPARE_PROMISE:
                mySteps.shift();
                await prepareByPromise();
                break;
            case PREPARE_CALLBACK:
                mySteps.shift();
                prepareByCallback();
                break;
            case START:
                mySteps.shift();
                console.info(`case to start`);
                avRecorder.start();
                break;
            case PAUSE:
                mySteps.shift();
                console.info(`case to pause`);
                avRecorder.pause();
                break;
            case RESUME:
                mySteps.shift();
                console.info(`case to resume`);
                avRecorder.resume();
                break;
            case STOP:
                mySteps.shift();
                console.info(`case to stop`);
                avRecorder.stop();
                break;
            case RESET_PROMISE:
                mySteps.shift();
                await resetByPromise();
                break;
            case RESET_CALLBACK:
                mySteps.shift();
                resetByCallback();
                break;
            case RELEASE_PROMISE:
                mySteps.shift();
                await releaseByPromise();
                break;
            case RELEASE_CALLBACK:
                mySteps.shift();
                await releaseByCallback();
                break;
        }
    }

    function setCallback(done) {
        console.info('case callback');
        avRecorder.on('stateChange', async (state, reason) => {
            console.info('case state has changed, new state is :' + state + ',and new reason is : ' + reason);
            switch (state) {
                case 'idle':
                    nextStep(done);
                    break;
                case 'prepared':
                    nextStep(done);
                    break;
                case 'started':
                    mediaTestBase.msleep(RECORDER_TIME);
                    nextStep(done);
                    break;
                case 'paused':
                    mediaTestBase.msleep(PAUSE_TIME);
                    nextStep(done);
                    break;
                case 'stopped':
                    nextStep(done);
                    break;
                case 'released':
                    avRecorder = undefined;
                    nextStep(done);
                    break;
                case 'error':
                    console.info("case error state!!!");
                    break;
                default:
                    console.info('case start is unknown');
                    nextStep(done);
            }
        });
        avRecorder.on('error', (err) => {
            console.info('case avRecorder.on(error) called, errMessage is ' + err.message);
            nextStep(done);
        });
    }

    /* *
        * @tc.number    : SUB_MULTIMEDIA_MEDIA_AUDIO_RECORDER_FUNC_PROMISE_0100
        * @tc.name      : 01.AAC
        * @tc.desc      : Audio recordr control test
        * @tc.size      : MediumTest
        * @tc.type      : Function
        * @tc.level     : Level0
    */
    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_TIME_PROMISE_0100', 0, async function (done) {
        console.info(`case to create avRecorder by promise`);
        let d = new Date();
        let t1 = d.getTime();
        await media.createAVRecorder().then((recorder) => {
            let t2 = d.getTime();
            console.info('case createAVRecorder promise called');
            console.info('case create time promise t1 is: ' + t1);
            console.info('case create time promise t2 is: ' + t2);
            console.info('case create time promise t2-t1 is: ' + (t2-t1));
            if (typeof (recorder) != 'undefined') {
                avRecorder = recorder;
            } else {
                console.info('case create avRecorder failed!!!');
                expect().assertFail();
                done();
            }
        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
        console.info(`case to release by promise`);
        if (avRecorder) {
            await avRecorder.release().then(() => {
                console.info('case recorder release by promise called');
                avRecorder = undefined;
            }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
        }
    })


    /* *
    * @tc.number    : SUB_MULTIMEDIA_MEDIA_AUDIO_RECORDER_FUNC_CALLBACK_0100
    * @tc.name      : 01.AAC
    * @tc.desc      : Audio recordr control test
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level0
    */
    it('SUB_MULTIMEDIA_MEDIA_AUDIO_RECORDER_FUNC_CALLBACK_0100', 0, async function (done) {
        console.info(`case to create avRecorder by callback`);
        let d = new Date();
        let t1 = d.getTime();
        media.createAVRecorder(async(err, recorder) => {
            let t2 = d.getTime();
            console.info('case create time callback t1 is: ' + t1);
            console.info('case create time callback t2 is: ' + t2);
            console.info('case create time callback t2-t1 is: ' + (t2-t1));
            if (typeof (err) == 'undefined') {
                console.info('case createAVRecorder callback success ');
                avRecorder = recorder;
                console.info(`case to release by promise`);
                if (avRecorder) {
                    await avRecorder.release().then(() => {
                        console.info('case recorder release by promise called');
                        avRecorder = undefined;
                    }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                }
            } else {
                printfError(err);
            }
        });
    })
})
