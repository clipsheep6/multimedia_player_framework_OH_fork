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
    const START_PROMISE = 8;
    const START_CALLBACK = 9;
    const PAUSE_PROMISE = 10;
    const PAUSE_CALLBACK = 11;
    const RESUME_PROMISE = 12;
    const RESUME_CALLBACK = 13;
    const STOP_PROMISE = 14;
    const STOP_CALLBACK = 15;
    const RESET_PROMISE = 16;
    const RESET_CALLBACK = 17;
    const RELEASE_PROMISE = 18;
    const RELEASE_CALLBACK = 19;
    const SETCALLBACKOFF = 20;
    const STOPVIDEOOUTPUT = 21;
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
    let needDone = false;
    
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

    let wrongConfig1 = {
        videoSourceType : media.VideoSourceType.VIDEO_SOURCE_TYPE_SURFACE_YUV,
        url : 'fd://',
        rotation : 0,
        location : { latitude : 30, longitude : 130 }
    }

    let wrongConfig2 = {
        videoSourceType : media.VideoSourceType.VIDEO_SOURCE_TYPE_SURFACE_YUV,
        profile : videoProfile,
        url : 'fd://',
        rotation : 99,
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
        caseCount += 1;
        needDone = false;
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

    function failureCallback(error) {
        console.info(`case failureCallback called,errMessage is ${error.message}`);
    }
    
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

    async function getRecorderFileFd(fileName, fileType) {
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
                setCallbackOn(done);
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
                setCallbackOn(done);
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

    async function startByPromise() {
        console.info(`case to start by promise`);
        await avRecorder.start().then(() => {
            console.info('case recorder start by promise called');
        }, failureCallback).catch(catchCallback);
    }

    function startByCallback() {
        console.info(`case to start by callback`);
        avRecorder.start((err) => {
            if (typeof (err) == 'undefined') {
                console.info('case recorder start by callback called');
            } else {
                printfError(err);
            }
        });
    }

    async function pauseByPromise() {
        console.info(`case to pause by promise`);
        await avRecorder.pause().then(() => {
            console.info('case recorder pause by promise called');
        }, failureCallback).catch(catchCallback);
    }

    function pauseByCallback() {
        console.info(`case to pause by callback`);
        avRecorder.pause((err) => {
            if (typeof (err) == 'undefined') {
                console.info('case recorder pause by callback called');
            } else {
                printfError(err);
            }
        });
    }

    async function resumeByPromise() {
        console.info(`case to resume by promise`);
        await avRecorder.resume().then(() => {
            console.info('case recorder resume by promise called');
        }, failureCallback).catch(catchCallback);
    }

    function resumeByCallback() {
        console.info(`case to resume by callback`);
        avRecorder.resume((err) => {
            if (typeof (err) == 'undefined') {
                console.info('case recorder resume by callback called');
            } else {
                printfError(err);
            }
        });
    }

    async function stopByPromise() {
        console.info(`case to stop by promise`);
        await avRecorder.stop().then(() => {
            console.info('case recorder stop by promise called');
        }, failureCallback).catch(catchCallback);
    }

    function stopByCallback() {
        console.info(`case to stop by callback`);
        avRecorder.stop((err) => {
            if (typeof (err) == 'undefined') {
                console.info('case recorder stop by callback called');
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

    async function releaseByPromise(done) {
        console.info(`case to release by promise`);
        if (avRecorder) {
            await avRecorder.release().then(() => {
                console.info('case recorder release by promise called');
            }, failureCallback).catch(catchCallback);
        }
        if(needDone) {
            avRecorder = undefined;
            done();
        }
    }

    async function releaseByCallback(done) {
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
        if(needDone) {
            avRecorder = undefined;
            done();
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
            case START_PROMISE:
                mySteps.shift();
                await startByPromise();
                break;
            case START_CALLBACK:
                mySteps.shift();
                startByCallback();
                break;
            case PAUSE_PROMISE:
                mySteps.shift();
                await pauseByPromise();
                break;
            case PAUSE_CALLBACK:
                mySteps.shift();
                pauseByCallback();
                break;
            case RESUME_PROMISE:
                mySteps.shift();
                await resumeByPromise();
                break;
            case RESUME_CALLBACK:
                mySteps.shift();
                resumeByCallback();
                break;
            case STOP_PROMISE:
                mySteps.shift();
                await stopByPromise();
                break;
            case STOP_CALLBACK:
                mySteps.shift();
                stopByCallback();
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
                await releaseByPromise(done);
                break;
            case RELEASE_CALLBACK:
                mySteps.shift();
                await releaseByCallback(done);
                break;
            case SETCALLBACKOFF:
                mySteps.shift();
                setCallbackOff(done);
                break;
            case STOPVIDEOOUTPUT:
                mySteps.shift();
                console.info(`case to stop videoOutput`);
                await stopVideoOutput();
                nextStep(done);
                break;
            default:
                console.info('do nothing');
        }
    }

    function setCallbackOn(done) {
        console.info('case callback on');
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
                    // setCallbackOff();
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

    function setCallbackOff(done) {
        console.info('case callback off called');
        if (avRecorder != undefined) {
            console.info('case to call avRecorder.off("stateChange")');
            avRecorder.off('stateChange');
            console.info('case to call avRecorder.off("error")');
            avRecorder.off('error');
            console.info('case call avRecorder.off done');
        }
        needDone = true;
        nextStep(done);
        console.info('case callback off done');
    }


    // it('Sub_Multimedia_Media_AVRecorder_Stability_Prepare_Promise', 0, async function (done) {
    //     let fileName = 'avRecorder_func_0'+ caseCount +'.mp4';
    //     await getRecorderFileFd(fileName, 'video');
    //     await createAVRecorderByPromise(done);
    //     for (let i=0; i< 1000; i++) {
    //         console.info("case current prepare promise loop is: " + i);
    //         await prepareByPromise();
    //     }
    //     await releaseByPromise();
    //     done();
    // })

    // it('Sub_Multimedia_Media_AVRecorder_Stability_Prepare_Callback', 0, async function (done) {
    //     let fileName = 'avRecorder_func_0'+ caseCount +'.mp4';
    //     await getRecorderFileFd(fileName, 'video');
    //     await createAVRecorderByPromise(done);
    //     for (let i=0; i< 1000; i++) {
    //         console.info("case current prepare callback loop is: " + i);
    //         prepareByCallback();
    //     }
    //     await releaseByPromise();
    //     done();
    // })

    // it('Sub_Multimedia_Media_AVRecorder_Stability_Getsurface_Promise', 0, async function (done) {
    //     let fileName = 'avRecorder_func_0'+ caseCount +'.mp4';
    //     await getRecorderFileFd(fileName, 'video');
    //     await createAVRecorderByPromise(done);
    //     await prepareByPromise();
    //     for (let i=0; i< 1000; i++) {
    //         console.info("case current getSurface promise loop is: " + i);
    //         await getInputSurfaceByPromise();
    //     }
    //     await releaseByPromise();
    //     done();
    // })

    // it('Sub_Multimedia_Media_AVRecorder_Stability_Getsurface_Callback', 0, async function (done) {
    //     let fileName = 'avRecorder_func_0'+ caseCount +'.mp4';
    //     await getRecorderFileFd(fileName, 'video');
    //     await createAVRecorderByPromise(done);
    //     await prepareByPromise();
    //     for (let i=0; i< 1000; i++) {
    //         console.info("case current getSurface callback loop is: " + i);
    //         getInputSurfaceByCallback();
    //     }
    //     await releaseByPromise();
    //     done();
    // })

    // it('Sub_Multimedia_Media_AVRecorder_Stability_Start_Promise', 0, async function (done) {
    //     let fileName = 'avRecorder_func_0'+ caseCount +'.mp4';
    //     await getRecorderFileFd(fileName, 'video');
    //     await createAVRecorderByPromise(done);
    //     await prepareByPromise();
    //     await getInputSurfaceByPromise();
    //     await startVideoOutput();
    //     for (let i=0; i< 1000; i++) {
    //         console.info("case current start promise loop is: " + i);
    //         await startByPromise();
    //     }
    //     await stopVideoOutput();
    //     await releaseByPromise();
    //     done();
    // })

    // it('Sub_Multimedia_Media_AVRecorder_Stability_Start_Callback', 0, async function (done) {
    //     let fileName = 'avRecorder_func_0'+ caseCount +'.mp4';
    //     await getRecorderFileFd(fileName, 'video');
    //     await createAVRecorderByPromise(done);
    //     await prepareByPromise();
    //     await getInputSurfaceByPromise();
    //     await startVideoOutput();
    //     await startByPromise();
    //     for (let i=0; i< 1000; i++) {
    //         console.info("case current start callback loop is: " + i);
    //         startByCallback();
    //     }
    //     await stopVideoOutput();
    //     await releaseByPromise();
    //     done();
    // })

    // it('Sub_Multimedia_Media_AVRecorder_Stability_Pause_Promise', 0, async function (done) {
    //     let fileName = 'avRecorder_func_0'+ caseCount +'.mp4';
    //     await getRecorderFileFd(fileName, 'video');
    //     await createAVRecorderByPromise(done);
    //     await prepareByPromise();
    //     await getInputSurfaceByPromise();
    //     await startVideoOutput();
    //     await startByPromise();
    //     for (let i=0; i< 1000; i++) {
    //         console.info("case current pause promise loop is: " + i);
    //         await pauseByPromise();
    //     }
    //     await stopVideoOutput();
    //     await releaseByPromise();
    //     done();
    // })

    // it('Sub_Multimedia_Media_AVRecorder_Stability_Pause_Callback', 0, async function (done) {
    //     let fileName = 'avRecorder_func_0'+ caseCount +'.mp4';
    //     await getRecorderFileFd(fileName, 'video');
    //     await createAVRecorderByPromise(done);
    //     await prepareByPromise();
    //     await getInputSurfaceByPromise();
    //     await startVideoOutput();
    //     await startByPromise();
    //     for (let i=0; i< 1000; i++) {
    //         console.info("case current pause callback loop is: " + i);
    //         pauseByCallback();
    //     }
    //     await stopVideoOutput();
    //     await releaseByPromise();
    //     done();
    // })

    // it('Sub_Multimedia_Media_AVRecorder_Stability_Resume_Promise', 0, async function (done) {
    //     let fileName = 'avRecorder_func_0'+ caseCount +'.mp4';
    //     await getRecorderFileFd(fileName, 'video');
    //     await createAVRecorderByPromise(done);
    //     await prepareByPromise();
    //     await getInputSurfaceByPromise();
    //     await startVideoOutput();
    //     await startByPromise();
    //     await pauseByPromise();
    //     for (let i=0; i< 1000; i++) {
    //         console.info("case current resume promise loop is: " + i);
    //         await resumeByPromise();
    //     }
    //     await stopVideoOutput();
    //     await releaseByPromise();
    //     done();
    // })

    // it('Sub_Multimedia_Media_AVRecorder_Stability_Resume_Callback', 0, async function (done) {
    //     let fileName = 'avRecorder_func_0'+ caseCount +'.mp4';
    //     await getRecorderFileFd(fileName, 'video');
    //     await createAVRecorderByPromise(done);
    //     await prepareByPromise();
    //     await getInputSurfaceByPromise();
    //     await startVideoOutput();
    //     await startByPromise();
    //     await pauseByPromise();
    //     for (let i=0; i< 1000; i++) {
    //         console.info("case current resume callback loop is: " + i);
    //         resumeByCallback();
    //     }
    //     await stopVideoOutput();
    //     await releaseByPromise();
    //     done();
    // })

    // it('Sub_Multimedia_Media_AVRecorder_Stability_Stop_Promise', 0, async function (done) {
    //     let fileName = 'avRecorder_func_0'+ caseCount +'.mp4';
    //     await getRecorderFileFd(fileName, 'video');
    //     await createAVRecorderByPromise(done);
    //     await prepareByPromise();
    //     await getInputSurfaceByPromise();
    //     await startVideoOutput();
    //     await startByPromise();
    //     for (let i=0; i< 1000; i++) {
    //         console.info("case current stop promise loop is: " + i);
    //         await stopByPromise();
    //     }
    //     await stopVideoOutput();
    //     await releaseByPromise();
    //     done();
    // })
 
    // it('Sub_Multimedia_Media_AVRecorder_Stability_Stop_Callback', 0, async function (done) {
    //     let fileName = 'avRecorder_func_0'+ caseCount +'.mp4';
    //     await getRecorderFileFd(fileName, 'video');
    //     await createAVRecorderByPromise(done);
    //     await prepareByPromise();
    //     await getInputSurfaceByPromise();
    //     await startVideoOutput();
    //     await startByPromise();
    //     for (let i=0; i< 1000; i++) {
    //         console.info("case current stop callback loop is: " + i);
    //         stopByCallback();
    //     }
    //     await stopVideoOutput();
    //     await releaseByPromise();
    //     done();
    // })

    // it('Sub_Multimedia_Media_AVRecorder_Stability_Reset_Promise', 0, async function (done) {
    //     let fileName = 'avRecorder_func_0'+ caseCount +'.mp4';
    //     await getRecorderFileFd(fileName, 'video');
    //     await createAVRecorderByPromise(done);
    //     for (let i=0; i< 1000; i++) {
    //         console.info("case current reset promise loop is: " + i);
    //         await resetByPromise();
    //     }
    //     await releaseByPromise();
    //     done();
    // })

    // it('Sub_Multimedia_Media_AVRecorder_Stability_Reset_Callback', 0, async function (done) {
    //     let fileName = 'avRecorder_func_0'+ caseCount +'.mp4';
    //     await getRecorderFileFd(fileName, 'video');
    //     await createAVRecorderByPromise(done);
    //     for (let i=0; i< 1000; i++) {
    //         console.info("case current reset callback loop is: " + i);
    //         resetByCallback();
    //     }
    //     await releaseByPromise();
    //     done();
    // })

    // it('Sub_Multimedia_Media_AVRecorder_Stability_Release_Promise', 0, async function (done) {
    //     let fileName = 'avRecorder_func_0'+ caseCount +'.mp4';
    //     await getRecorderFileFd(fileName, 'video');
    //     await createAVRecorderByPromise(done);
    //     for (let i=0; i< 1000; i++) {
    //         console.info("case current release promise loop is: " + i);
    //         await releaseByPromise();
    //     }
    //     done();
    // })

    // it('Sub_Multimedia_Media_AVRecorder_Stability_Release_Callback', 0, async function (done) {
    //     let fileName = 'avRecorder_func_0'+ caseCount +'.mp4';
    //     await getRecorderFileFd(fileName, 'video');
    //     await createAVRecorderByPromise(done);
    //     for (let i=0; i< 1000; i++) {
    //         console.info("case current release callback loop is: " + i);
    //         releaseByCallback();
    //     }
    //     done();
    // })

    it('Sub_Multimedia_Media_AVRecorder_Stability_On_Off_State_Promise', 0, async function (done) {
        let fileName = 'avRecorder_func_0'+ caseCount +'.mp4';
        await getRecorderFileFd(fileName, 'video');
        await createAVRecorderByPromise(done);
        for (let i=0; i< 1000; i++) {
            console.info("case current state-on loop is: " + i);
            console.info('avRecorder.state is ' + avRecorder.state);
            avRecorder.on('stateChange', async (state, reason) => {});
            avRecorder.on('error', (err) => {});
            console.info("case current off loop is: " + i);
            avRecorder.off('stateChange');
            avRecorder.off('error');
        }
        await releaseByPromise();
        done();
    })
})
