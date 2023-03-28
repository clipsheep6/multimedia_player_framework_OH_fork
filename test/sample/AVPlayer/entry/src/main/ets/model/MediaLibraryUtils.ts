/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

import mediaLibrary from '@ohos.multimedia.mediaLibrary';
import media from '@ohos.multimedia.media';
import common from '@ohos.app.ability.common';
import Logger from '../model/Logger';

const TAG = 'MediaDemo MediaLibraryUtils:';

export default class MediaLibraryUtils {
  // @ts-ignore
  private context = getContext(this) as common.UIAbilityContext;
  private mediaLib: mediaLibrary.MediaLibrary = mediaLibrary.getMediaLibrary(this.context);
  //根据文件id查寻文件对象
  async findFile(uri: string, displayName: string): Promise<mediaLibrary.FileAsset> {
    let fileKeyObj = mediaLibrary.FileKey;
    const args = displayName.toString();
    const fetchOp = {
      selections: fileKeyObj.DISPLAY_NAME + '= ?',
      selectionArgs: [args],
    };
    const fetchFileResult = await this.mediaLib.getFileAssets(fetchOp);
    let fileAsset : mediaLibrary.FileAsset = null;
    if (fetchFileResult.getCount() > 0) {
      fileAsset = await fetchFileResult.getFirstObject();
    }
    return fileAsset;
  }

  async openFileDescriptor(name:string): Promise<media.AVFileDescriptor> {
    let fileDescriptor : media.AVFileDescriptor = null;
    await this.context.resourceManager.getRawFd(name).then(value => {
      fileDescriptor = {fd: value.fd, offset: value.offset, length: value.length};
      Logger.info(TAG, 'getRawFileDescriptor success fileName: ' + name);
    }).catch(error => {
      Logger.info(TAG, 'getRawFileDescriptor err: ' + error);
    });
    return fileDescriptor;
  }

  async closeFileDescriptor(name:string): Promise<void> {
    await this.context.resourceManager.closeRawFileDescriptor(name).then(()=> {
      Logger.info(TAG, 'case closeRawFileDescriptor ' + name);
    }).catch(error => {
      Logger.info(TAG, 'case closeRawFileDescriptor err: ' + error);
    });
  }

  getShowTime(ms): string {
    let seconds: any = Math.round(ms / 1000);
    let sec: any = seconds % 60;
    let min: any = (seconds - sec) / 60;
    if (sec < 10) {
      sec = '0' + sec;
    }
    if (min < 10) {
      min = '0' + min;
    }
    return min + ':' + sec;
  }
}