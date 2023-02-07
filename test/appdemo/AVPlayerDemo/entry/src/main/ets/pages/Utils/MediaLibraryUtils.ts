/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
import mediaLibrary from '@ohos.multimedia.mediaLibrary';

const TAG = 'MediaDemo MediaLibraryUtils:'
export default class MediaLibraryUtils {
    private mediaLib: mediaLibrary.MediaLibrary = mediaLibrary.getMediaLibrary(globalThis.abilityContext)
    //根据文件id查寻文件对象
    async findFile(uri: string, displayName: string): Promise<mediaLibrary.FileAsset> {
        console.info(TAG + 'findFile start')
        let fileKeyObj = mediaLibrary.FileKey
        console.info(TAG + 'findFile running 1')
        const args = displayName.toString()
        console.info(TAG + 'findFile running 2, args is :' + args)
        const fetchOp = {
            selections: fileKeyObj.DISPLAY_NAME + '= ?',
            selectionArgs: [args],
        }
        console.info(TAG + 'findFile running 3')

        const fetchFileResult = await this.mediaLib.getFileAssets(fetchOp)
        console.info(TAG + 'findFile running 4')
        let fileAsset = null
        if (fetchFileResult.getCount() > 0) {
            console.info(TAG + 'findFile running 5')
            fileAsset = await fetchFileResult.getFirstObject()
        }
        console.info(TAG + 'findFile running 6')

        return fileAsset
    }

    getShowTime(ms): string {
        let seconds: any = Math.round(ms / 1000)
        let sec: any = seconds % 60
        let min: any = (seconds - sec) / 60
        if (sec < 10) {
            sec = '0' + sec
        }
        if (min < 10) {
            min = '0' + min
        }
        return min + ':' + sec
    }
}