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

export async function getFd(pathName, fileType) {
    let fdObject = {
        fileAsset : null,
        fdNumber : null
    }
    let displayName = pathName;
    console.info('[mediaLibrary] fileType is ' + fileType);
    const mediaTest = mediaLibrary.getMediaLibrary(globalThis.abilityContext);
    let fileKeyObj = mediaLibrary.FileKey;
    let mediaType;
    let publicPath;
    if (fileType == 'audio') {
        mediaType = mediaLibrary.MediaType.AUDIO;
        publicPath = await mediaTest.getPublicDirectory(mediaLibrary.DirectoryType.DIR_AUDIO);
    } else {
        mediaType = mediaLibrary.MediaType.VIDEO;
        publicPath = await mediaTest.getPublicDirectory(mediaLibrary.DirectoryType.DIR_VIDEO);
    }
    console.info('[mediaLibrary] publicPath is ' + publicPath);
    let dataUri = await mediaTest.createAsset(mediaType, displayName, publicPath);
    if (dataUri != undefined) {
        let args = dataUri.id.toString();
        let fetchOp = {
            selections : fileKeyObj.ID + "=?",
            selectionArgs : [args],
        }
        let fetchFileResult = await mediaTest.getFileAssets(fetchOp);
        fdObject.fileAsset = await fetchFileResult.getAllObject();
        fdObject.fdNumber = await fdObject.fileAsset[0].open('rw');
        console.info('case getFd number is: ' + fdObject.fdNumber);
    }
    return fdObject;
}

export async function closeFd(fileAsset, fdNumber) {
    if (fileAsset != null) {
        await fileAsset[0].close(fdNumber).then(() => {
            console.info('[mediaLibrary] case close fd success');
        }).catch((err) => {
            console.info('[mediaLibrary] case close fd failed');
        });
    } else {
        console.info('[mediaLibrary] case fileAsset is null');
    }
}