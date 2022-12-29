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


const CODECMIMEVALUE = ['video/avc', 'audio/mp4a-latm', 'audio/mpeg']
// File operation
export async function getFileDescriptor(fileName) {
    let fileDescriptor = undefined;
    let mgr = globalThis.abilityContext.resourceManager
    await mgr.getRawFileDescriptor(fileName).then(value => {
        fileDescriptor = {fd: value.fd, offset: value.offset, length: value.length};
        console.log('case getRawFileDescriptor success fileName: ' + fileName);
    }).catch(error => {
        console.log('case getRawFileDescriptor err: ' + error);
    });
    return fileDescriptor;
}
export async function closeFileDescriptor(fileName) {
    await globalThis.abilityContext.resourceManager.closeRawFileDescriptor(fileName).then(()=> {
        console.log('case closeRawFileDescriptor ' + fileName);
    }).catch(error => {
        console.log('case closeRawFileDescriptor err: ' + error);
    });
}


// wait asynchronously
export async function msleepAsync(ms) {
    return new Promise((resolve) => setTimeout(resolve, ms));
}

// callback function for promise call back error
export function failureCallback(error) {
    expect().assertFail();
    console.info(`case failureCallback promise called,errMessage is ${error.message}`);
}

// callback function for promise catch error
export function catchCallback(error) {
    expect().assertFail();
    console.info(`case error catch called,errMessage is ${error.message}`);
}
export function checkDescription(actualDescription, descriptionKey, descriptionValue) {
    for (let i = 0; i < descriptionKey.length; i++) {
        let property = actualDescription[descriptionKey[i]];
        console.info('case key is  '+ descriptionKey[i]);
        console.info('case actual value is  '+ property);
        console.info('case hope value is  '+ descriptionValue[i]);
        if (descriptionKey[i] == 'codec_mime') {
            expect(property).assertEqual(CODECMIMEVALUE[descriptionValue[i]]);
        } else {
            expect(property).assertEqual(descriptionValue[i]);
        }
        
    }
}

