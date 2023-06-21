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


export async function initCaptureSession(cameraInput, videoOutPut, cameraManager, previewOutput) {
    await cameraInput.open().then(() => {
        console.info('[camera] case cameraInput.open() called');
    }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
    console.info('[camera] case cameraInput open success');
    let captureSession = cameraManager.createCaptureSession();
    console.info('[camera] case createCaptureSession success');
    captureSession.beginConfig();
    console.info('[camera] case beginConfig success');
    captureSession.addInput(cameraInput);
    console.info('[camera] case addInput(cameraInput) success');
    captureSession.addOutput(previewOutput);
    console.info('[camera] case addOutput(previewOutput) success');
    captureSession.addOutput(videoOutPut);
    console.info('[camera] case addOutput(videoOutPut) success');
    await captureSession.commitConfig().then(() => {
        console.info('[camera] case commitConfig');
    }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
    console.info('[camera] case commitConfig success');
    await captureSession.start().then(() => {
        console.info('[camera] case captureSession.start()');
    }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
    console.info('[camera] case captureSession.start() success');
    return captureSession;
}

export async function stopCaptureSession(captureSession) {
    await captureSession.stop().then(() => {
        console.info('[camera] case captureSession stop success');
    });
    await captureSession.release().then(() => {
        console.info('[camera] case captureSession release success');
    });
}