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

describe('AVRecorderStabilityTest', function () {
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

    let videoProfile = {
        fileFormat : media.ContainerFormatType.CFT_MPEG_4,
        videoBitrate : 48000,
        videoCodec : media.CodecMimeType.VIDEO_MPEG4,
        videoFrameWidth : 640,
        videoFrameHeight : 480,
        videoFrameRate : 30
    }
    let videoConfig = {
        videoSourceType : media.VideoSourceType.VIDEO_SOURCE_TYPE_SURFACE_YUV,
        profile : videoProfile,
        url : 'fd://',
        rotation : 0,
        location : { latitude : 30, longitude : 130 }
    }


    let audioProfile = {
        audioBitrate : 48000,
        audioChannels : 2,
        audioCodec : media.CodecMimeType.AUDIO_AAC,
        audioSampleRate : 48000,
        fileFormat : media.ContainerFormatType.CFT_MPEG_4,
    }

    let audioConfig = {
        audioSourceType : media.AudioSourceType.AUDIO_SOURCE_TYPE_MIC,
        profile : audioProfile,
        url : 'fd://',
        rotation : 0,
        location : { latitude : 30, longitude : 130 }
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
        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
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
        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback)
        if (previewProfiles[0].format == camera.CameraFormat.CAMERA_FORMAT_YUV_420_SP) {
            console.info('[camera] case format is VIDEO_SOURCE_TYPE_SURFACE_YUV');
            avConfig.videoSourceType = media.VideoSourceType.VIDEO_SOURCE_TYPE_SURFACE_YUV;
            videoConfig.videoSourceType = media.VideoSourceType.VIDEO_SOURCE_TYPE_SURFACE_YUV;

        } else {
            console.info('[camera] case format is VIDEO_SOURCE_TYPE_SURFACE_ES');
            avConfig.videoSourceType = media.VideoSourceType.VIDEO_SOURCE_TYPE_SURFACE_ES;
            videoConfig.videoSourceType = media.VideoSourceType.VIDEO_SOURCE_TYPE_SURFACE_ES;
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
            () => {}, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
        playerSurfaceId = globalThis.value;
        avRecorder = undefined;
        surfaceID = '';
        count = 0;
        videoOutput = undefined;
        previewOutput = undefined;
        mySteps = new Array();
        caseCount += 1;
        console.info('beforeEach case Out');
    })

    afterEach(async function () {
        console.info('afterEach case In');
        await mediaTestBase.clearRouter();
        mySteps = new Array();
        await releaseByPromise();
        await mediaTestBase.closeFd(fdObject.fileAsset, fdObject.fdNumber);
        console.info('afterEach case Out');
    })

    afterAll(function () {
        console.info('afterAll case');
    })

    function printfError(error) {
        console.error(`case callback error called,errMessage is ${error.message}`);
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

    async function getRecorderFileFd(fileName, fileType) {
        // let fileName = 'videoRecorder_func_0'+ caseCount +'.mp4';
        console.info("case current fileName is: " + fileName);
        fdObject = await mediaTestBase.getFd(fileName, fileType);
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
                expect(avRecorder.state).assertEqual('idle');
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

    async function getInputSurfaceByPromise(done) {
        console.info(`case to getsurface by promise`);
        await avRecorder.getInputSurface().then((outputSurface) => {
            console.info('case getInputSurface by promise called');
            surfaceID = outputSurface;
            console.info('case outputSurface surfaceID is: ' + surfaceID);
            nextStep(done);
        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
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
            case GETSURFACE_PROMISE:
                mySteps.shift();
                await getInputSurfaceByPromise(done);
                break;
            case GETSURFACE_CALLBACK:
                mySteps.shift();
                getInputSurfaceByCallback(done);
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
                break;
            case STOPVIDEOOUTPUT:
                mySteps.shift();
                console.info(`case to stop videoOutput`);
                await stopVideoOutput();
                videoOutput = undefined;
                previewOutput = undefined;
                nextStep(done);
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
                    nextStep(done);
                    break;
                case 'prepared':
                    nextStep(done);
                    break;
                case 'started':
                    mediaTestBase.msleep(1000);
                    nextStep(done);
                    break;
                case 'paused':
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
    * @tc.number    : Sub_Multimedia_Media_AVRecorder_Stability_0100
    * @tc.name      : 01. test avRecorder basic function by promise interfaces
    * @tc.desc      : test avRecorder operation: start-pause-resume-stop-reset-release
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level0
    */
    it('Sub_Multimedia_Media_AVRecorder_Stability_0100', 0, async function (done) {
        let fileName = 'avRecorder_stability_promise_'+ caseCount +'.mp4';
        await getRecorderFileFd(fileName, 'video');
        for (let i=0; i < 1000; i++)
        { 
            console.info("case i is: " + i);
            mySteps.push(CREATE_PROMISE, PREPARE_PROMISE, GETSURFACE_PROMISE, STARTCAMERA, START, PAUSE, 
                RESUME, STOP, STOPVIDEOOUTPUT, RESET_PROMISE, RELEASE_PROMISE, SLEEP);
            console.info("case mySteps is: " + mySteps);
        }
        mySteps.push(END);
        console.info("case mySteps is: " + mySteps);
        nextStep(done);
    })

    /* *
    * @tc.number    : Sub_Multimedia_Media_AVRecorder_Stability_0200
    * @tc.name      : 01. test avRecorder basic function by callback interfaces
    * @tc.desc      : test avRecorder operation: start-pause-resume-stop-reset-release
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level0
    */
    it('Sub_Multimedia_Media_AVRecorder_Stability_0200', 0, async function (done) {
        let fileName = 'avRecorder_stability_callback_'+ caseCount +'.mp4';
        await getRecorderFileFd(fileName, 'video');
        mySteps = new Array();
        for (let i=0; i < 1000; i++)
        { 
            console.info("case i is: " + i);
            mySteps.push(CREATE_CALLBACK, PREPARE_CALLBACK, GETSURFACE_CALLBACK, STARTCAMERA, START, PAUSE, 
                RESUME, STOP, STOPVIDEOOUTPUT, RESET_CALLBACK, RELEASE_CALLBACK, SLEEP);
            console.info("case mySteps is: " + mySteps);
        }
        mySteps.push(END);
        console.info("case mySteps is: " + mySteps);
        nextStep(done);
    })
})
