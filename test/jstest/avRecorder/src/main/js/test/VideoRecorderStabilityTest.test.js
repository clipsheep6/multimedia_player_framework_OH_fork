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

import media from '@ohos.multimedia.media'
import camera from '@ohos.multimedia.camera'
import * as mediaTestBase from '../../../../../MediaTestBase.js';
import * as videoRecorderBase from '../../../../../VideoRecorderTestBase.js';
import {describe, beforeAll, beforeEach, afterEach, afterAll, it, expect} from 'deccjsunit/index'

describe('VideoRecorderStabilityTest', function () {
    const RECORDER_TIME = 3000;
    const PAUSE_TIME = 1000;
    const END = 0;
    const CREATE_PROMISE = 1;
    const CREATE_CALLBACK = 2;
    const PREPARE_PROMISE = 3;
    const PREPARE_CALLBACK = 4;
    const GETSURFACE_PROMISE = 5;
    const GETSURFACE_CALLBACK = 6;
    const STARTCAMERA = 7;
    const START = 8;
    const PAUSE = 9;
    const RESUME = 10;
    const STOP = 11;
    const RESET_PROMISE = 12;
    const RESET_CALLBACK = 13;
    const RELEASE_PROMISE = 14;
    const RELEASE_CALLBACK = 15;
    const STOPVIDEOOUTPUT = 16;
    const SLEEP = 17;

    let mySteps = new Array();
    let caseCount = 0;
    let cameraManager;
    let cameras;
    let captureSession;
    let fdPath;
    let fdObject;
    let cameraID = 0;
    let playerSurfaceId = '';
    let pageId = 0;
    let videoProfiles;
    let previewProfiles;
    const pagePath1 = 'pages/surfaceTest/surfaceTest';
    const pagePath2 = 'pages/surfaceTest2/surfaceTest2';

    let avRecorder = null;
    let surfaceID = '';
    let count = 0;
    let videoOutput;
    let previewOutput;

    let avProfile = {
        audioBitrate : 48000,
        audioChannels : 2,
        audioCodec : media.CodecMimeType.AUDIO_AAC,
        audioSampleRate : 48000,
        fileFormat : media.ContainerFormatType.CFT_MPEG_4,
        videoBitrate : 48000,
        videoCodec : media.CodecMimeType.VIDEO_MPEG4,
        videoFrameWidth : 640,
        videoFrameHeight : 480,
        videoFrameRate : 30
    }
    let avConfig = {
        audioSourceType : media.AudioSourceType.AUDIO_SOURCE_TYPE_MIC,
        videoSourceType : media.VideoSourceType.VIDEO_SOURCE_TYPE_SURFACE_YUV,
        profile : avProfile,
        url : 'fd://',
        rotation : 0,
        location : { latitude : 30, longitude : 130 }
    }

    let onlyVideoProfile = {
        durationTime : 1000,
        fileFormat : media.ContainerFormatType.CFT_MPEG_4,
        videoBitrate : 48000,
        videoCodec : media.CodecMimeType.VIDEO_MPEG4,
        videoFrameWidth : 640,
        videoFrameHeight : 480,
        videoFrameRate : 10
    }

    let onlyVideoConfig = {
        videoSourceType : media.VideoSourceType.VIDEO_SOURCE_TYPE_SURFACE_ES,
        profile : onlyVideoProfile,
        url : 'fd://',
        rotation : 0,
        location : { latitude : 30, longitude : 130 },
        maxSize : 100,
        maxDuration : 500
    }

    beforeAll(async function () {
        console.info('beforeAll case In');
        cameraManager = await camera.getCameraManager(null);
        if (cameraManager != null) {
            console.info('[camera] case getCameraManager success');
        } else {
            console.info('[camera] case getCameraManager failed');
            return;
        }
        await cameraManager.getSupportedCameras().then((cameraDevices)=> {
            cameras = cameraDevices;
        }, failureCallback).catch(catchCallback);
        if (cameras != null) {
            console.info('[camera] case getCameras success');
        } else {
            console.info('[camera] case getCameras failed');
        }
        await cameraManager.getSupportedOutputCapability(cameras[0]).then((cameraoutputcapability) => {
            console.info('[camera] case getSupportedOutputCapability success');
            videoProfiles = cameraoutputcapability.videoProfiles;
            videoProfiles[0].size.height = 480;
            videoProfiles[0].size.width = 640;
            previewProfiles = cameraoutputcapability.previewProfiles;
            previewProfiles[0].size.height = 480;
            previewProfiles[0].size.width = 640;
        }, failureCallback).catch(catchCallback)
        if (previewProfiles[0].format == camera.CameraFormat.CAMERA_FORMAT_YUV_420_SP) {
            console.info('[camera] case format is VIDEO_SOURCE_TYPE_SURFACE_YUV');
            avConfig.videoSourceType = media.VideoSourceType.VIDEO_SOURCE_TYPE_SURFACE_YUV;
        } else {
            console.info('[camera] case format is VIDEO_SOURCE_TYPE_SURFACE_ES');
            avConfig.videoSourceType = media.VideoSourceType.VIDEO_SOURCE_TYPE_SURFACE_ES;
        }
        console.info('beforeAll case Out');
    })

    beforeEach(async function () {
        console.info('beforeEach case In');
        await mediaTestBase.toNewPage(pagePath1, pagePath2, pageId);
        pageId = (pageId + 1) % 2;
        if (previewProfiles[0].format == camera.CameraFormat.CAMERA_FORMAT_YUV_420_SP) {
            if (pageId == 0) {
                avProfile.videoCodec = media.CodecMimeType.VIDEO_MPEG4;
            } else {
                avProfile.videoCodec = media.CodecMimeType.VIDEO_AVC;
            }
        } else {
            avProfile.videoCodec = media.CodecMimeType.VIDEO_MPEG4;
        }
        await mediaTestBase.msleepAsync(1000).then(
            () => {}, failureCallback).catch(catchCallback);
        playerSurfaceId = globalThis.value;
        cameraID = 0;
        avRecorder = undefined;
        surfaceID = '';
        count = 0;
        videoOutput = undefined;
        previewOutput = undefined;
        caseCount += 1;
        await getRecorderFileFd();
        console.info('beforeEach case Out');
    })

    afterEach(async function () {
        console.info('afterEach case In');
        await mediaTestBase.clearRouter();
        mySteps = new Array();
        await mediaTestBase.closeFd(fdObject.fileAsset, fdObject.fdNumber);
        console.info('afterEach case Out');
    })

    afterAll(function () {
        console.info('afterAll case');
    })

    function printfError(error) {
        console.error(`case callback error called,errMessage is ${error.message}`);
    }
    
    // callback function for promise call back error
    function failureCallback(error) {
        console.info(`case failureCallback called,errMessage is ${error.message}`);
    }
    
    // callback function for promise catch error
    function catchCallback(error) {
        console.info(`case catchCallback called,errMessage is ${error.message}`);
    }



    async function startVideoOutput() {
        console.info(`case to start camera`);
        videoOutput = await cameraManager.createVideoOutput(videoProfiles[0], surfaceID);
        previewOutput = await cameraManager.createPreviewOutput(previewProfiles[0], playerSurfaceId);
        captureSession = await videoRecorderBase.initCaptureSession(videoOutput, cameraManager,
            cameras[0], previewOutput);
        if (videoOutput == null) {
            console.info('[camera] case videoOutPut is null');
            return;
        }
        await videoOutput.start().then(() => {
            console.info('[camera] case videoOutput start success');
        });
    }

    async function stopVideoOutput() {
        await videoOutput.stop();
        await videoRecorderBase.stopCaptureSession(captureSession);
    }

    async function stopCaptureSession() {
        await videoRecorderBase.stopCaptureSession(captureSession);
    }

    async function getRecorderFileFd() {
        let fileName = 'videoRecorder_func_0'+ caseCount +'.mp4';
        console.info("case current fileName is: " + fileName);
        fdObject = await mediaTestBase.getFd(fileName);
        fdPath = "fd://" + fdObject.fdNumber.toString();
        console.info("case fdPath is: " + fdPath);
        avConfig.url = fdPath;
        console.info("case to out getRecorderFileFd");
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
        }, failureCallback).catch(catchCallback);
    }

    function createAVRecorderByCallback(done) {
        console.info(`case to create avRecorder by callback`);
        media.createAVRecorder((err, recorder) => {
            if (typeof (err) == 'undefined') {
                console.info('case createAVRecorder callback success ');
                avRecorder = recorder;
                expect(avRecorder.state).assertEqual('idle');
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
        }, failureCallback).catch(catchCallback);
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

    async function getInputSurfaceByPromise(done) {
        console.info(`case to getsurface by promise`);
        await avRecorder.getInputSurface().then((outputSurface) => {
            console.info('case getInputSurface by promise called');
            surfaceID = outputSurface;
            console.info('case outputSurface surfaceID is: ' + surfaceID);
            nextStep(done);
        }, failureCallback).catch(catchCallback);
    }

    function getInputSurfaceByCallback(done) {
        console.info(`case to getsurface by callback`);
        avRecorder.getInputSurface((err, outputSurface) => {
            if (typeof (err) == 'undefined') {
                console.info('case getInputSurface by callback called');
                surfaceID = outputSurface;
                console.info('case outputSurface surfaceID is: ' + surfaceID);
                nextStep(done);
            } else {
                printfError(err);
            }
        });
    }

    async function resetByPromise() {
        console.info(`case to reset by promise`);
        await avRecorder.reset().then(() => {
            console.info('case recorder reset by promise called');
        }, failureCallback).catch(catchCallback);
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
            }, failureCallback).catch(catchCallback);
        }
    }

    async function releaseByCallback() {
        console.info(`case to release by callback`);
        if (avRecorder)
        avRecorder.release(async(err) => {
            if (typeof (err) == 'undefined') {
                console.info('case recorder release by callback called');
                avRecorder = undefined;
            } else {
                printfError(err);
            }
        });
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
                nextStep(done);
                break;
            case CREATE_CALLBACK:
                mySteps.shift();
                createAVRecorderByCallback(done);
                nextStep(done);
                break;
            case PREPARE_PROMISE:
                mySteps.shift();
                await prepareByPromise();
                nextStep(done);
                break;
            case PREPARE_CALLBACK:
                mySteps.shift();
                prepareByCallback();
                nextStep(done);
                break;
            case GETSURFACE_PROMISE:
                mySteps.shift();
                await getInputSurfaceByPromise(done);
                nextStep(done);
                break;
            case GETSURFACE_CALLBACK:
                mySteps.shift();
                getInputSurfaceByCallback(done);
                nextStep(done);
                break;
            case STARTCAMERA:
                mySteps.shift();
                await startVideoOutput();
                nextStep(done);
                break;
            case START:
                mySteps.shift();
                console.info(`case to start`);
                avRecorder.start();
                nextStep(done);
                break;
            case STOPVIDEOOUTPUT:
                mySteps.shift();
                console.info(`case to stop videoOutput`);
                await stopVideoOutput();
                nextStep(done);
                break;
            case PAUSE:
                mySteps.shift();
                console.info(`case to pause`);
                avRecorder.pause();
                nextStep(done);
                break;
            case RESUME:
                mySteps.shift();
                console.info(`case to resume`);
                avRecorder.resume();
                nextStep(done);
                break;
            case STOP:
                mySteps.shift();
                console.info(`case to stop`);
                avRecorder.stop();
                nextStep(done);
                break;
            case RESET_PROMISE:
                mySteps.shift();
                await resetByPromise();
                nextStep(done);
                break;
            case RESET_CALLBACK:
                mySteps.shift();
                resetByCallback();
                nextStep(done);
                break;
            case RELEASE_PROMISE:
                mySteps.shift();
                await releaseByPromise();
                nextStep(done);
                break;
            case RELEASE_CALLBACK:
                mySteps.shift();
                await releaseByCallback();
                nextStep(done);
                break;
            case SLEEP:
                mySteps.shift();
                mediaTestBase.msleep(2000);
                nextStep(done);
                break;
        }
    }

    function setCallback(done) {
        console.info('case callback');
        avRecorder.on('stateChange', async (state, reason) => {
            console.info('case state has changed, new state is :' + state + ',and new reason is : ' + reason);
            switch (state) {
                case 'idle':
                    break;
                case 'prepared':
                    break;
                case 'started':
                    // mediaTestBase.msleep(RECORDER_TIME);
                    break;
                case 'paused':
                    // mediaTestBase.msleep(PAUSE_TIME);
                    break;
                case 'stopped':
                    break;
                case 'released':
                    avRecorder = undefined;
                    break;
                case 'error':
                    console.info("case error state!!!");
                    break;
                default:
                    console.info('case start is unknown');
            }
        });
        avRecorder.on('error', (err) => {
            console.info('case avRecorder.on(error) called, errMessage is ' + err.message);
            nextStep(done);
        });
    }


    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_Stability_Create_Reset_Release_Promise', 0, async function (done) {
        for (let i=0; i< 10; i++)
        { 
            mySteps.push(CREATE_PROMISE);
            mySteps.push(RESET_PROMISE);
            mySteps.push(RELEASE_PROMISE);
            mySteps.push(SLEEP);
        }
        mySteps.push(END);
        nextStep(done);
    })

    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_Stability_Create_Reset_Release_Callback', 0, async function (done) {
        for (let i=0; i< 10; i++)
        { 
            mySteps.push(CREATE_CALLBACK);
            mySteps.push(PREPARE_CALLBACK);
            mySteps.push(GETSURFACE_PROMISE);
            mySteps.push(STARTCAMERA);
            mySteps.push(START);
            mySteps.push(PAUSE);
            mySteps.push(RESUME);
            mySteps.push(STOP);
            mySteps.push(RESET_CALLBACK);
            mySteps.push(RELEASE_CALLBACK);
            mySteps.push(SLEEP);
        }
        mySteps.push(END);
        nextStep(done);
    })
})
