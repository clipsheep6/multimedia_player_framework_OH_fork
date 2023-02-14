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


export async function initCaptureSession(videoOutPut, cameraManager, cameraDevice, previewOutput) {
    let cameraInput = await cameraManager.createCameraInput(cameraDevice);
    if (cameraInput != null) {
        console.info('[camera] case createCameraInput success');
    } else {
        console.info('[camera] case createCameraInput failed');
        return;
    }
    await cameraInput.open((err) => {
        if(err){
            console.info('[camera] cameraInput open Failed');
            return
        }
        console.info('[camera] cameraInput open success');
    })
    let captureSession = await cameraManager.createCaptureSession();
    await captureSession.beginConfig();
    await captureSession.addInput(cameraInput);
    await captureSession.addOutput(previewOutput);
    await captureSession.addOutput(videoOutPut);
    await captureSession.commitConfig();
    await captureSession.start();
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