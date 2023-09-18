/*
* Copyright (C) 2023 Huawei Device Co., Ltd.
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
import screencapturer from 'libscreencapturer.so';
import emitter from '@ohos.events.emitter'
import { DESTROY_FLOAT_WINDOW } from '../common/Constants'
import router from '@ohos.router'

const TAG: string = '[ScreenCapture]'

@Entry
@Component
export struct AVScreenCapture {
  textTimerController: TextTimerController = new TextTimerController()
  @State isIconHidden: boolean = false
  @State timerFormat: string = 'hh:mm:ss'
  private timeoutID: number = -1

  /* Red icon of the recording state to flash each 500ms */
  private flashTheRecordingIcon() {
    this.timeoutID = setTimeout(() => {
      this.isIconHidden = !this.isIconHidden
      this.flashTheRecordingIcon()
    }, 500)
  }

  /* Page life cycle after the build */
  onPageShow(): void {
      // Notes: controller must start after the build() func to init the TextTimer() now
      this.textTimerController.start()
      console.log(`onPageShow() textTimerController to start`);
      // enable this feature depend on the ArkTSPartialUpdate enable or TextTimer bugfix
      this.flashTheRecordingIcon()
  }

  onPageHide(): void {
    console.info(TAG, 'onPageHide()')
  }

  build() {
    Row() {
      Rect({ width: 10, height: 10, radius: 2 })
        .id('stopCapture')
        .fill(Color.Red)
        .visibility(this.isIconHidden ? Visibility.Hidden : Visibility.Visible)
        TextTimer({ controller: this.textTimerController })
        .format(this.timerFormat)
        .fontColor(0xe5000000)
        .fontSize(16)
        .margin({ left: 12 })
    }
    .onClick(() => {
      console.log(`onClick() stopScreenCapture to stop`);

      screencapturer.stopScreenCapture()
      this.textTimerController.pause()

      console.log(`onClick() stopScreenCapture stoped`);
      clearTimeout(this.timeoutID)
      this.isIconHidden = false
      emitter.emit({ eventId: DESTROY_FLOAT_WINDOW })
    })
    .borderRadius(15)
    .padding({ left: 20, right: 20 })
    .backgroundColor(0x55000000)
    .size({ width: '100%', height: '100%' })
  }
}
