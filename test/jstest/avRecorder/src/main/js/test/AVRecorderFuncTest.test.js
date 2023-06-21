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
import router from '@system.router'
import * as mediaTestBase from '../../../../../MediaTestBase.js';
import * as videoRecorderBase from '../../../../../VideoRecorderTestBase.js';
import {describe, beforeAll, beforeEach, afterEach, afterAll, it, expect} from 'deccjsunit/index'

export default function AVRecorderFuncTest() {
describe('AVRecorderFuncTest', function () {
    const RECORDER_TIME = 3000;
    const PAUSE_TIME = 1000;
    const END = 'end of switch';
    const CREATE_PROMISE = 'create by promise';
    const CREATE_CALLBACK = 'create by callback';
    const PREPARE_PROMISE = 'prepare by promise';
    const PREPARE_CALLBACK = 'prepare by callback';
    const GETSURFACE_PROMISE = 'getInputSurface by promise';
    const GETSURFACE_CALLBACK = 'getInputSurface by callback';
    const START_CAMERA = 'start camera Input';
    const START_PROMISE = 'start by promise';
    const START_CALLBACK = 'start by callback';
    const PAUSE_PROMISE = 'pause by promise';
    const PAUSE_CALLBACK = 'pause by callback';
    const RESUME_PROMISE = 'resume by promise';
    const RESUME_CALLBACK = 'resume by callback';
    const STOP_PROMISE = 'stop by promise';
    const STOP_CALLBACK = 'stop by callback';
    const RESET_PROMISE = 'reset by promise';
    const RESET_CALLBACK = 'reset by callback';
    const RELEASE_PROMISE = 'release by promise';
    const RELEASE_CALLBACK = 'release by callback';
    const SET_CALLBACKOFF = 'set callback off';
    const STOP_VIDEOOUTPUT = 'stop video output';
    const RELEASE_CAMERA = 'release camera';
    const PRINT_INFO = 'print operation info';
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
    let cameraInput;
    let videoOutput;
    let previewOutput;
    let needDone = false;
    const RESOLUTION480_WIDTH = 640;
    const RESOLUTION480_HEIGHT = 480;
    const RESOLUTION1080_WIDTH = 1920;
    const RESOLUTION1080_HEIGHT = 1080;
    const FORMAT_MP4 = '.mp4';
    
    let avProfile = {
        audioBitrate : 200000,
        audioChannels : 2,
        audioCodec : media.CodecMimeType.AUDIO_AAC,
        audioSampleRate : 48000,
        fileFormat : media.ContainerFormatType.CFT_MPEG_4,
        videoBitrate : 2000000,
        videoCodec : media.CodecMimeType.VIDEO_AVC,
        videoFrameWidth : RESOLUTION480_WIDTH,
        videoFrameHeight : RESOLUTION480_HEIGHT,
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

    beforeAll(async function () {
        console.info('beforeAll case In');
        let permissionName1 = 'ohos.permission.MICROPHONE';
        let permissionName2 = 'ohos.permission.MEDIA_LOCATION';
        let permissionName3 = 'ohos.permission.READ_MEDIA';
        let permissionName4 = 'ohos.permission.WRITE_MEDIA';
        let permissionName5 = 'ohos.permission.CAMERA';
        let permissionNames = [permissionName1, permissionName2, permissionName3, permissionName4, permissionName5];
        await mediaTestBase.getPermission(permissionNames);
        await mediaTestBase.msleepAsync(2000);
        await mediaTestBase.driveFn(3);
        console.info('[camera] case to getCameraManager');
        cameraManager = camera.getCameraManager(null);
        if (cameraManager != null) {
            console.info('[camera] case getCameraManager success');
        } else {
            console.info('[camera] case getCameraManager failed');
            return;
        }
        cameras = cameraManager.getSupportedCameras();
        if (cameras != null) {
            console.info('[camera] case getCameras success');
        } else {
            console.info('[camera] case getCameras failed');
            return;
        }
        let cameraoutputcapability = cameraManager.getSupportedOutputCapability(cameras[0]);
        if (cameraoutputcapability != null) {
            console.info('[camera] case getSupportedOutputCapability success');
            videoProfiles = cameraoutputcapability.videoProfiles;
            videoProfiles[0].size.height = RESOLUTION480_HEIGHT;
            videoProfiles[0].size.width = RESOLUTION480_WIDTH;
            previewProfiles = cameraoutputcapability.previewProfiles;
            previewProfiles[0].size.height = RESOLUTION480_HEIGHT;
            previewProfiles[0].size.width = RESOLUTION480_WIDTH;
        } else {
            console.info('[camera] case getSupportedOutputCapability failed');
            return;
        }
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
            avProfile.videoFrameWidth = RESOLUTION1080_WIDTH;
            avProfile.videoFrameHeight = RESOLUTION1080_HEIGHT;
            videoProfiles[0].size.width = RESOLUTION1080_WIDTH;
            videoProfiles[0].size.height = RESOLUTION1080_HEIGHT;
            previewProfiles[0].size.width = RESOLUTION1080_WIDTH;
            previewProfiles[0].size.height = RESOLUTION1080_HEIGHT;
        } else {
            avProfile.videoCodec = media.CodecMimeType.VIDEO_AVC;
            avProfile.videoFrameWidth = RESOLUTION480_WIDTH;
            avProfile.videoFrameHeight = RESOLUTION480_HEIGHT;
            videoProfiles[0].size.width = RESOLUTION480_WIDTH;
            videoProfiles[0].size.height = RESOLUTION480_HEIGHT;
            previewProfiles[0].size.width = RESOLUTION480_WIDTH;
            previewProfiles[0].size.height = RESOLUTION480_HEIGHT;
        }
        await mediaTestBase.msleepAsync(1000).then(
            () => {}, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
        playerSurfaceId = globalThis.value;
        avRecorder = undefined;
        surfaceID = '';
        videoOutput = undefined;
        previewOutput = undefined;
        caseCount += 1;
        needDone = false;
        console.info('beforeEach case Out');
    })

    afterEach(async function () {
        console.info('afterEach case In');
        await router.clear();
        mySteps = new Array();
        if (avRecorder != undefined) {
            await releaseByPromise();
        }
        await mediaTestBase.closeFd(fdObject.fileAsset, fdObject.fdNumber);
        console.info('afterEach case Out');
    })

    afterAll(function () {
        console.info('afterAll case');
    })

    async function startVideoOutput() {
        console.info(`case to start camera`);
        cameraInput = cameraManager.createCameraInput(cameras[0]);
        console.info(`case camera create cameraInput done`);
        videoOutput = cameraManager.createVideoOutput(videoProfiles[0], surfaceID);
        console.info(`case camera create videoOutput done`);
        previewOutput = cameraManager.createPreviewOutput(previewProfiles[0], playerSurfaceId);
        console.info(`case camera create previewOutput done`);
        captureSession = await videoRecorderBase.initCaptureSession(cameraInput, videoOutput, cameraManager, previewOutput);
        if (cameraInput == null || videoOutput == null || previewOutput == null) {
            console.info('[camera] case cameraInput/videoOutput/previewOutput is null');
            return;
        }
        await videoOutput.start().then(() => {
            console.info('[camera] case videoOutput start success');
        });
    }

    async function stopVideoOutput() {
        await captureSession.stop().then(() => {
            console.info('[camera] case main captureSession stop called');
        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
        captureSession.beginConfig();
        if (videoOutput) {
            await videoOutput.stop().then(() => {
                console.info('[camera] case main videoOutput stop called');
            }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
            console.info('case camera to remove videoOutput');
            captureSession.removeOutput(videoOutput);
            console.info('case remove videoOutput done');
        }
        console.info('case camera to commitConfig');
        await captureSession.commitConfig().then(() => {
            console.info('[camera] case main commitConfig called');
        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
        console.info('case camera commitConfig done');
        await cameraInput.close().then(() => {
            console.info('[camera] case main cameraInput.close() called');
        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
        console.info('case camera cameraInput close done');
    }

    async function releaseCamera() {
        console.info('[camera] case main to release videoOutput');
        await videoOutput.release().then(() => {
            console.info('[camera] case main videoOutput release called');
        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
        console.info('[camera] case videoOutput release success');

        console.info('[camera] case main to release previewOutput');
        await previewOutput.release().then(() => {
            console.info('[camera] case main previewOutput release called');
        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
        console.info('[camera] case previewOutput release success');

        console.info('[camera] case to release captureSession');
        await captureSession.release().then(() => {
            console.info('[camera] case captureSession release called');
        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
        console.info('[camera] case captureSession release success');
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
        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
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
                mediaTestBase.failureCallback(err);
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
                mediaTestBase.failureCallback(err);
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
                mediaTestBase.failureCallback(err);
            }
        });
    }

    async function startByPromise() {
        console.info(`case to start by promise`);
        await avRecorder.start().then(() => {
            console.info('case recorder start by promise called');
        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
    }

    function startByCallback() {
        console.info(`case to start by callback`);
        avRecorder.start((err) => {
            if (typeof (err) == 'undefined') {
                console.info('case recorder start by callback called');
            } else {
                mediaTestBase.failureCallback(err);
            }
        });
    }

    async function pauseByPromise() {
        console.info(`case to pause by promise`);
        await avRecorder.pause().then(() => {
            console.info('case recorder pause by promise called');
        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
    }

    function pauseByCallback() {
        console.info(`case to pause by callback`);
        avRecorder.pause((err) => {
            if (typeof (err) == 'undefined') {
                console.info('case recorder pause by callback called');
            } else {
                mediaTestBase.failureCallback(err);
            }
        });
    }

    async function resumeByPromise() {
        console.info(`case to resume by promise`);
        await avRecorder.resume().then(() => {
            console.info('case recorder resume by promise called');
        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
    }

    function resumeByCallback() {
        console.info(`case to resume by callback`);
        avRecorder.resume((err) => {
            if (typeof (err) == 'undefined') {
                console.info('case recorder resume by callback called');
            } else {
                mediaTestBase.failureCallback(err);
            }
        });
    }

    async function stopByPromise() {
        console.info(`case to stop by promise`);
        await avRecorder.stop().then(() => {
            console.info('case recorder stop by promise called');
        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
    }

    function stopByCallback() {
        console.info(`case to stop by callback`);
        avRecorder.stop((err) => {
            if (typeof (err) == 'undefined') {
                console.info('case recorder stop by callback called');
            } else {
                mediaTestBase.failureCallback(err);
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
                mediaTestBase.failureCallback(err);
            }
        });
    }

    async function releaseByPromise(done) {
        console.info(`case to release by promise`);
        if (avRecorder) {
            await avRecorder.release().then(() => {
                console.info('case recorder release by promise called');
            }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
        }
        if (needDone) {
            avRecorder = undefined;
            nextStep(done);
        }
    }

    async function releaseByCallback(done) {
        console.info(`case to release by callback`);
        if (avRecorder) {
            avRecorder.release(async(err) => {
                if (typeof (err) == 'undefined') {
                    console.info('case recorder release by callback called');
                } else {
                    mediaTestBase.failureCallback(err);
                }
            });
        }
        if (needDone) {
            avRecorder = undefined;
            nextStep(done);
        }
    }

    async function nextStep(done) {
        console.info("case myStep[0]: " + mySteps[0]);
        if (mySteps[0] == END) {
            console.info("case to END");
            done();
            return;
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
            case START_CAMERA:
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
            case SET_CALLBACKOFF:
                mySteps.shift();
                setCallbackOff(done);
                break;
            case STOP_VIDEOOUTPUT:
                mySteps.shift();
                await stopVideoOutput();
                nextStep(done);
                break;
            case RELEASE_CAMERA:
                mySteps.shift();
                await releaseCamera();
                nextStep(done);
                break;
            case PRINT_INFO:
                mySteps.shift();
                console.info(mySteps[0]);
                mySteps.shift();
                nextStep(done);
                break;
            default:
                console.info('do nothing');
        }
    }

    function setCallbackOn(done) {
        console.info('case callback on');
        avRecorder.on('stateChange', async (state, reason) => {
            console.info('case state has changed, new state is :' + state);
            switch (state) {
                case 'idle':
                    nextStep(done);
                    break;
                case 'prepared':
                    nextStep(done);
                    break;
                case 'started':
                    await mediaTestBase.msleepAsync(RECORDER_TIME);
                    console.info('case state is started, after recording 3s');
                    nextStep(done);
                    break;
                case 'paused':
                    await mediaTestBase.msleepAsync(PAUSE_TIME);
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
                    console.info('case state is unknown');
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

    function CallbackOn(count) {
        console.info('case callback on ' + count);
        avRecorder.on('stateChange', async (state, reason) => {
            console.info(count + 'case state has changed, new state is :' + state);
            })
        avRecorder.on('error', (err) => {
            console.info(count + 'case avRecorder.on(error) called, errMessage is ' + err.message);
        });
    }

    function CallbackOff() {
        console.info('case callback off called');
        if (avRecorder != undefined) {
            console.info('case to call avRecorder.off("stateChange")');
            avRecorder.off('stateChange');
            console.info('case to call avRecorder.off("error")');
            avRecorder.off('error');
            console.info('case call avRecorder.off done');
        }
        console.info('case callback off done');
    }
 
    /* *
    * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_0100
    * @tc.name      : 01. test avRecorder basic function by promises
    * @tc.desc      : test avRecorder operation: start-pause-resume-stop-reset-release
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level2
    */
    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_0100', 0, async function (done) {
        let fileName = 'avRecorder_func_0'+ caseCount + FORMAT_MP4;
        await getRecorderFileFd(fileName, 'video');
        mySteps = new Array(CREATE_PROMISE, PREPARE_PROMISE, GETSURFACE_PROMISE, START_CAMERA, START_PROMISE, PAUSE_PROMISE, 
            RESUME_PROMISE, STOP_PROMISE, STOP_VIDEOOUTPUT, RESET_PROMISE, SET_CALLBACKOFF, RELEASE_PROMISE, RELEASE_CAMERA, END);
        nextStep(done);
    })

    /* *
    * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_0200
    * @tc.name      : 01. test avRecorder basic function by callbacks
    * @tc.desc      : test avRecorder operation: start-pause-resume-stop-reset-release
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level2
    */
    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_0200', 0, async function (done) {
        let fileName = 'avRecorder_func_0'+ caseCount + FORMAT_MP4;
        await getRecorderFileFd(fileName, 'video');
        mySteps = new Array(CREATE_CALLBACK, PREPARE_CALLBACK, GETSURFACE_CALLBACK, START_CAMERA, START_CALLBACK, PAUSE_CALLBACK,
            RESUME_CALLBACK, STOP_CALLBACK, STOP_VIDEOOUTPUT, RESET_CALLBACK, RELEASE_CALLBACK, RELEASE_CAMERA, END);
        nextStep(done);
    })

    /* *
    * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_0300
    * @tc.name      : 01. test avRecorder basic function by promises
    * @tc.desc      : test avRecorder operation: start-pause-resume-pause-reset-release
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level2
    */
    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_0300', 0, async function (done) {
        let fileName = 'avRecorder_func_0'+ caseCount + FORMAT_MP4;
        await getRecorderFileFd(fileName, 'video');
        mySteps = new Array(CREATE_PROMISE, PREPARE_PROMISE, GETSURFACE_PROMISE, START_CAMERA, START_PROMISE, PAUSE_PROMISE,
            RESUME_PROMISE, PAUSE_PROMISE, RESET_PROMISE, STOP_VIDEOOUTPUT, RELEASE_PROMISE, RELEASE_CAMERA, END);
        nextStep(done);
    })

    /* *
    * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_0400
    * @tc.name      : 01. test avRecorder basic function by callbacks
    * @tc.desc      : test avRecorder operation: start-pause-resume-pause-reset-release 
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level2
    */
    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_0400', 0, async function (done) {
        let fileName = 'avRecorder_func_0'+ caseCount + FORMAT_MP4;
        await getRecorderFileFd(fileName, 'video');
        mySteps = new Array(CREATE_CALLBACK, PREPARE_CALLBACK, GETSURFACE_CALLBACK, START_CAMERA, START_CALLBACK, PAUSE_CALLBACK,
            RESUME_CALLBACK, PAUSE_CALLBACK, RESET_CALLBACK, STOP_VIDEOOUTPUT, SET_CALLBACKOFF, RELEASE_CALLBACK, RELEASE_CAMERA, END);
        nextStep(done);
    })

    /* *
    * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_0500
    * @tc.name      : 01. test avRecorder basic function by promises
    * @tc.desc      : test avRecorder operation: start-stop-reset-release
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level2
    */
    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_0500', 0, async function (done) {
        let fileName = 'avRecorder_func_0'+ caseCount + FORMAT_MP4;
        await getRecorderFileFd(fileName, 'video');
        mySteps = new Array(CREATE_PROMISE, PREPARE_PROMISE, GETSURFACE_PROMISE, START_CAMERA, START_PROMISE, STOP_PROMISE, 
            RESET_PROMISE, STOP_VIDEOOUTPUT, RELEASE_PROMISE, RELEASE_CAMERA, END);
        nextStep(done);
    })

    /* *
    * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_0600
    * @tc.name      : 01. test avRecorder basic function by callbacks
    * @tc.desc      : test avRecorder operation: start-stop-reset-release
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level2
    */
    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_0600', 0, async function (done) {
        let fileName = 'avRecorder_func_0'+ caseCount + FORMAT_MP4;
        await getRecorderFileFd(fileName, 'video');
        mySteps = new Array(CREATE_CALLBACK, PREPARE_CALLBACK, GETSURFACE_CALLBACK, START_CAMERA, START_CALLBACK, STOP_CALLBACK,
            RESET_CALLBACK, STOP_VIDEOOUTPUT, RELEASE_CALLBACK, RELEASE_CAMERA, END);
        nextStep(done);
    })

    /* *
    * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_0700
    * @tc.name      : 01. test avRecorder basic function by promises
    * @tc.desc      : test avRecorder operation: start-pause-stop-reset-release
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level2
    */
    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_0700', 0, async function (done) {
        let fileName = 'avRecorder_func_0'+ caseCount + FORMAT_MP4;
        await getRecorderFileFd(fileName, 'video');
        mySteps = new Array(CREATE_PROMISE, PREPARE_PROMISE, GETSURFACE_PROMISE, START_CAMERA, START_PROMISE, PAUSE_PROMISE, 
            STOP_PROMISE, STOP_VIDEOOUTPUT, RESET_PROMISE, RELEASE_PROMISE, RELEASE_CAMERA, END);
        nextStep(done);
    })

    /* *
    * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_0800
    * @tc.name      : 01. test avRecorder basic function by callbacks
    * @tc.desc      : test avRecorder operation: start-pause-stop-reset-release
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level2
    */
    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_0800', 0, async function (done) {
        let fileName = 'avRecorder_func_0'+ caseCount + FORMAT_MP4;
        await getRecorderFileFd(fileName, 'video');
        mySteps = new Array(CREATE_CALLBACK, PREPARE_CALLBACK, GETSURFACE_CALLBACK, START_CAMERA, START_CALLBACK, PAUSE_CALLBACK,
            STOP_CALLBACK, STOP_VIDEOOUTPUT, RESET_CALLBACK, RELEASE_CALLBACK, RELEASE_CAMERA, END);
        nextStep(done);
    })

    /* *
    * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_0900
    * @tc.name      : 01. test avRecorder basic function by promises
    * @tc.desc      : test avRecorder operation: start-pause-resume-stop-reset-release
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level2
    */
    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_0900', 0, async function (done) {
        let fileName = 'avRecorder_func_0'+ caseCount + FORMAT_MP4;
        await getRecorderFileFd(fileName, 'video');
        mySteps = new Array(CREATE_PROMISE, PREPARE_PROMISE, GETSURFACE_PROMISE, START_CAMERA, START_PROMISE, PAUSE_PROMISE, 
            RESUME_PROMISE, STOP_PROMISE, RESET_PROMISE, RELEASE_PROMISE, STOP_VIDEOOUTPUT, RELEASE_CAMERA, END);
        nextStep(done);
    })

    /* *
    * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_1000
    * @tc.name      : 01. test avRecorder basic function by callbacks
    * @tc.desc      : test avRecorder operation: start-pause-resume-stop-reset-release
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level2
    */
    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_1000', 0, async function (done) {
        let fileName = 'avRecorder_func_0'+ caseCount + FORMAT_MP4;
        await getRecorderFileFd(fileName, 'video');
        mySteps = new Array(CREATE_CALLBACK, PREPARE_CALLBACK, GETSURFACE_CALLBACK, START_CAMERA, START_CALLBACK, PAUSE_CALLBACK,
            RESUME_CALLBACK, STOP_CALLBACK, RESET_CALLBACK, RELEASE_CALLBACK, STOP_VIDEOOUTPUT, RELEASE_CAMERA, END);
        nextStep(done);
    })

    /* *
    * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_1100
    * @tc.name      : 01. test avRecorder prepare interface
    * @tc.desc      : test avRecorder reset-prepare-xxx-END
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level2
    */
    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_1100', 0, async function (done) {
        let fileName = 'avRecorder_func_0'+ caseCount + FORMAT_MP4;
        await getRecorderFileFd(fileName, 'video');
        mySteps = new Array(CREATE_PROMISE, PREPARE_PROMISE, GETSURFACE_PROMISE, START_CAMERA, START_PROMISE, 
            RESET_PROMISE, STOP_VIDEOOUTPUT, RELEASE_CAMERA, PREPARE_PROMISE, GETSURFACE_PROMISE, START_CAMERA, START_PROMISE, 
            RESET_PROMISE, RELEASE_PROMISE, STOP_VIDEOOUTPUT, RELEASE_CAMERA, END);
        nextStep(done);
    })

    /* *
    * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_1200
    * @tc.name      : 01. test avRecorder prepare interface
    * @tc.desc      : test avRecorder reset-prepare-xxx-END
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level2
    */
    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_1200', 0, async function (done) {
        let fileName = 'avRecorder_func_0'+ caseCount + FORMAT_MP4;
        await getRecorderFileFd(fileName, 'video');
        mySteps = new Array(CREATE_CALLBACK, PREPARE_CALLBACK, GETSURFACE_CALLBACK, START_CAMERA, START_CALLBACK, 
            RESET_CALLBACK, STOP_VIDEOOUTPUT, RELEASE_CAMERA, PREPARE_CALLBACK, GETSURFACE_CALLBACK, START_CAMERA, START_CALLBACK, 
            RESET_CALLBACK, RELEASE_CALLBACK, STOP_VIDEOOUTPUT, RELEASE_CAMERA, END);
        nextStep(done);
    })

    /* *
    * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_1300
    * @tc.name      : 01. test avRecorder prepare interface
    * @tc.desc      : test avRecorder stop-prepare-xxx-END
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level2
    */
    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_1300', 0, async function (done) {
        let fileName = 'avRecorder_func_0'+ caseCount + FORMAT_MP4;
        await getRecorderFileFd(fileName, 'video');
        mySteps = new Array(CREATE_PROMISE, PREPARE_PROMISE, GETSURFACE_PROMISE, START_CAMERA, START_PROMISE, 
            STOP_PROMISE, STOP_VIDEOOUTPUT, RELEASE_CAMERA, PREPARE_PROMISE, GETSURFACE_PROMISE, START_CAMERA, START_PROMISE, 
            RESET_PROMISE, RELEASE_PROMISE, STOP_VIDEOOUTPUT, RELEASE_CAMERA, END);
        nextStep(done);
    })

    /* *
    * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_1400
    * @tc.name      : 01. test avRecorder callback interface
    * @tc.desc      : test avRecorder stop-prepare-xxx-END
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level2
    */
    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_FUNC_1400', 0, async function (done) {
        let fileName = 'avRecorder_func_0'+ caseCount + FORMAT_MP4;
        await getRecorderFileFd(fileName, 'video');
        mySteps = new Array(CREATE_CALLBACK, PREPARE_CALLBACK, GETSURFACE_CALLBACK, START_CAMERA, START_CALLBACK, 
            STOP_CALLBACK, STOP_VIDEOOUTPUT, RELEASE_CAMERA, PREPARE_CALLBACK, GETSURFACE_CALLBACK, START_CAMERA, START_CALLBACK, 
            RESET_CALLBACK, STOP_VIDEOOUTPUT, RELEASE_CALLBACK, RELEASE_CAMERA, END);
        nextStep(done);
    })

    /* *
    * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVRECORDER_RESET_PROMISE_0100
    * @tc.name      : 01. test avRecorder basic function by promises
    * @tc.desc      : test avRecorder operation: any-reset(promise)
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level2
    */
    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_RESET_PROMISE_0100', 0, async function (done) {
        let fileName = 'avRecorder_reset_promise_0'+ caseCount + FORMAT_MP4;
        await getRecorderFileFd(fileName, 'video');
  
        let INFO1 = 'case current testcase is prepare-reset(promise)';
        let array1 = new Array(PRINT_INFO, INFO1, CREATE_PROMISE, PREPARE_PROMISE, RESET_PROMISE);

        let INFO2 = 'case current testcase is getInputSurface-reset(promise)';
        let array2 = new Array(PRINT_INFO, INFO2, PREPARE_PROMISE, GETSURFACE_PROMISE, RESET_PROMISE);

        let INFO3 = 'case current testcase is start-reset(promise)';
        let array3 = new Array(PRINT_INFO, INFO3, PREPARE_PROMISE, GETSURFACE_PROMISE, START_CAMERA, START_PROMISE, RESET_PROMISE, 
            STOP_VIDEOOUTPUT, RELEASE_CAMERA);

        let INFO4 = 'case current testcase is pause-reset(promise)';
        let array4 = new Array(PRINT_INFO, INFO4, PREPARE_PROMISE, GETSURFACE_PROMISE, START_CAMERA, START_PROMISE, PAUSE_PROMISE, 
            RESET_PROMISE, STOP_VIDEOOUTPUT, RELEASE_CAMERA);

        let INFO5 = 'case current testcase is resume-reset(promise)';
        let array5 = new Array(PRINT_INFO, INFO5, PREPARE_PROMISE, GETSURFACE_PROMISE, START_CAMERA, START_PROMISE, PAUSE_PROMISE, 
            RESUME_PROMISE, RESET_PROMISE, STOP_VIDEOOUTPUT, RELEASE_CAMERA);

        let INFO6 = 'case current testcase is stop-reset(promise)';
        let array6 = new Array(PRINT_INFO, INFO6, PREPARE_PROMISE, GETSURFACE_PROMISE, START_CAMERA, START_PROMISE, STOP_PROMISE, 
            RESET_PROMISE, STOP_VIDEOOUTPUT, RELEASE_CAMERA);

        mySteps = [...array1, ...array2, ...array3, ...array4, ...array5, ...array6, END];
        console.info('case mySteps is: ' + mySteps);
        nextStep(done);
    })

    /* *
    * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVRECORDER_RELEASE_PROMISE_0100
    * @tc.name      : 01. test avRecorder basic function by promises
    * @tc.desc      : test avRecorder operation: any-release(promise)
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level2
    */
    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_RELEASE_PROMISE_0100', 0, async function (done) {
        let fileName = 'avRecorder_release_promise_0'+ caseCount + FORMAT_MP4;
        await getRecorderFileFd(fileName, 'video');
    
        let INFO1 = 'case current testcase is create-release(promise)';
        let array1 = new Array(PRINT_INFO, INFO1, CREATE_PROMISE, RELEASE_PROMISE);
    
        let INFO2 = 'case current testcase is prepare-release(promise)';
        let array2 = new Array(PRINT_INFO, INFO2, CREATE_PROMISE, PREPARE_PROMISE, RELEASE_PROMISE);
    
        let INFO3 = 'case current testcase is getInputSurface-release(promise)';
        let array3 = new Array(PRINT_INFO, INFO3, CREATE_PROMISE, PREPARE_PROMISE, GETSURFACE_PROMISE, RELEASE_PROMISE);
    
        let INFO4 = 'case current testcase is start-release(promise)';
        let array4 = new Array(PRINT_INFO, INFO4, CREATE_PROMISE, PREPARE_PROMISE, GETSURFACE_PROMISE, START_CAMERA, START_PROMISE, 
            RELEASE_PROMISE, STOP_VIDEOOUTPUT, RELEASE_CAMERA);
    
        let INFO5 = 'case current testcase is pause-release(promise)';
        let array5 = new Array(PRINT_INFO, INFO5, CREATE_PROMISE, PREPARE_PROMISE, GETSURFACE_PROMISE, START_CAMERA, START_PROMISE, 
            PAUSE_PROMISE, RELEASE_PROMISE, STOP_VIDEOOUTPUT, RELEASE_CAMERA);
    
        let INFO6 = 'case current testcase is resume-release(promise)';
        let array6 = new Array(PRINT_INFO, INFO6, CREATE_PROMISE, PREPARE_PROMISE, GETSURFACE_PROMISE, START_CAMERA, START_PROMISE, 
            PAUSE_PROMISE, RESUME_PROMISE, RELEASE_PROMISE, STOP_VIDEOOUTPUT, RELEASE_CAMERA);
    
        let INFO7 = 'case current testcase is stop-release(promise)';
        let array7 = new Array(PRINT_INFO, INFO7, CREATE_PROMISE, PREPARE_PROMISE, GETSURFACE_PROMISE, START_CAMERA, START_PROMISE, 
            STOP_PROMISE, RELEASE_PROMISE, STOP_VIDEOOUTPUT, RELEASE_CAMERA);
    
        let INFO8 = 'case current testcase is reset-release(promise)';
        let array8 = new Array(PRINT_INFO, INFO8, CREATE_PROMISE, PREPARE_PROMISE, GETSURFACE_PROMISE, START_CAMERA, START_PROMISE, 
            RESET_PROMISE, RELEASE_PROMISE, STOP_VIDEOOUTPUT, RELEASE_CAMERA);
    
        mySteps = [...array1, ...array2, ...array3, ...array4, ...array5, ...array6, ...array7, ...array8, END];
        console.info('case mySteps is: ' + mySteps);
        nextStep(done);
    })

    /* *
    * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVRECORDER_SETCALLBACK_0100
    * @tc.name      : 01. test avRecorder setcallback on and off
    * @tc.desc      : test avRecorder setcallback on and off
    * @tc.size      : MediumTest
    * @tc.type      : Function
    * @tc.level     : Level2
    */
    it('SUB_MULTIMEDIA_MEDIA_AVRECORDER_SETCALLBACK_0100', 0, async function (done) {
        let fileName = 'avRecorder_callback_on_off_0'+ caseCount + FORMAT_MP4;
        await getRecorderFileFd(fileName, 'video');
        await createAVRecorderByPromise(done);
        CallbackOn(1);
        await prepareByPromise();
        CallbackOn(2);
        await getInputSurfaceByPromise(done);
        await startVideoOutput();
        await startByPromise();
        CallbackOff();
        await pauseByPromise();
        CallbackOn(3);
        await stopByPromise();
        CallbackOff();
        await releaseByPromise();
        done();
    })
})
}
