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
import audio from '@ohos.multimedia.audio';
import window from '@ohos.window'
import emitter from '@ohos.events.emitter'
import { DESTROY_FLOAT_WINDOW } from '../common/Constants'
import router from '@ohos.router'
import display from '@ohos.display'

const TAG: string = '[Index]'

@Entry
@Component
export struct Index {
  @State returnMsg: string = `hello`
  @State captureStart: boolean = false;
  public micSampleRateList = []
  @State selectedmicSampleRateKey: string = "请选择"

  public innerSampleRateList = []
  @State selectedinnerSampleRateKey: string = "请选择"

  public widthValueList = []
  public widthList: number[] = [640, 1280, 2560, 2716]
  @State selectedWidthKey: string = "请选择"
  public selectedWidth: number = Number(display.getDefaultDisplaySync().width)

  public heightValueList = []
  public heightList: number[] = [360, 720, 1440, 1524]
  @State selectedHeightKey: string = "请选择"
  public selectedHeight: number = Number(display.getDefaultDisplaySync().height)

  public micChannelValueList = []
  public micChannelList: number[] = [1, 2, 3, 4, 5, 6]
  @State selectedMicChannelKey: string = "请选择"
  public selectedMicChannel: number = 2

  public innerChannelValueList = []
  public innerChannelList: number[] = [1, 2, 3, 4, 5, 6]
  @State selectedInnerChannelKey: string = "请选择"
  public selectedInnerChannel: number = 0

  public isMicValueList = [];
  public isMicBoolList: boolean[] = [true ,false]
  @State selectedIsMicKey: string = "请选择"
  public selectedIsMic: boolean = false

  public CaptureConfig = {
    micSampleRate: audio.AudioSamplingRate.SAMPLE_RATE_16000,
    micChannel: this.selectedMicChannel,
    innerSampleRate: 0,
    innerChannel: this.selectedInnerChannel,
    width: this.selectedWidth,
    height: this.selectedHeight,
    isMic: this.selectedIsMic
  }

  aboutToAppear() {
    for (let key in audio.AudioSamplingRate) {
      this.micSampleRateList.push({ value: key })
    }
    for (let key in audio.AudioSamplingRate) {
      this.innerSampleRateList.push({ value: key })
    }
    for (var i = 0; i < this.heightList.length; i++) {
      var key = this.heightList[i].toString();
      this.heightValueList.push({ value: key });
    }
    for (var i = 0; i < this.widthList.length; i++) {
      var key = this.widthList[i].toString();
      this.widthValueList.push({ value: key });
    }
    for (var i = 0; i < this.micChannelList.length; i++) {
      var key = "mic_Chanel_" + this.micChannelList[i].toString();
      this.micChannelValueList.push({ value: key });
    }
    for (var i = 0; i < this.innerChannelList.length; i++) {
      var key = "inner_Chanel_" + this.innerChannelList[i].toString();
      this.innerChannelValueList.push({ value: key });
    }
    for (var i = 0; i < this.isMicBoolList.length; i++) {
      var key = "isMic_" + this.isMicBoolList[i].toString();
      this.isMicValueList.push({ value: key });
    }
  }

  startScreenCapture(CaptureConfig) {
    try {
      if (true) {
        let res = screencapturer.startScreenCapture(CaptureConfig);
        this.returnMsg = `startScreenCapture: 返回值 ${res}`
      } else {
        this.returnMsg = `screenCapture_ has started,please stop first`
      }
    } catch (error) {
      this.returnMsg = `startScreenCapture err: ${error}`
    }
  }

  getFrameRate() {
    try {
      let res = screencapturer.getFrameRate();
      this.returnMsg = `startScreenCapture: 返回值 audioframerate: ${res.audioframerate}, innerframerate: ${res.innerframerate}, videoframerate: ${res.videoframerate}`
    } catch (error) {
      this.returnMsg = `startScreenCapture err: ${error}`
    }
  }

  build() {
    Column() {
      Row() {
        Column() {
          Text("【屏幕录制-单实例-callback】返回数据: ")
            .fontWeight(FontWeight.Bolder)
            .position({ x: 10, y: 5 })
            .fontSize(18)
          Text(this.returnMsg).position({ x: 10, y: 30 }).fontSize(14)
        }.width('98%').height(150).backgroundColor(Color.Orange).position({ x: '1%' })
      }.position({ x: 0, y: 0 }).width('100%').zIndex(999)

      Scroll() {
        Column() {
          Row() {
            Text("micSampleRate:").fontSize(24)
            Select(this.micSampleRateList).value(this.selectedmicSampleRateKey)
              .onSelect((index, value) => {
                this.selectedmicSampleRateKey = value
                console.log(`selectedmicSampleRateKey: ${value}`)
                this.CaptureConfig.micSampleRate = audio.AudioSamplingRate[value]
                console.log(`micSampleRate: ${audio.AudioSamplingRate[value]}`)
              }).font({ size: 22 })
          }.margin({ top: 0 })

          Row() {
            Text("micChannel:").fontSize(24)
            Select(this.micChannelValueList).value(this.selectedMicChannelKey)
              .onSelect((index, value) => {
                this.selectedMicChannelKey = value
                console.log(`selectedChannelKey: ${value}`)
                this.CaptureConfig.micChannel = this.micChannelList[index]
                console.log(`micChannel: ${this.micChannelList[index]}`)
              })
          }.margin({ top: 10 })

          Row() {
            Text("innerSampleRate:").fontSize(24)
            Select(this.innerSampleRateList).value(this.selectedinnerSampleRateKey)
              .onSelect((index, value) => {
                this.selectedinnerSampleRateKey = value
                this.CaptureConfig.innerSampleRate = audio.AudioSamplingRate[value]
                console.log(`innerSampleRate: ${audio.AudioSamplingRate[value]}`)
              }).font({ size: 22 })
          }.margin({ top: 0 })

          Row() {
            Text("innerChannel:").fontSize(24)
            Select(this.innerChannelValueList).value(this.selectedInnerChannelKey)
              .onSelect((index, value) => {
                this.selectedInnerChannelKey = value
                console.log(`selectedInnerChannelKey: ${value}`)
                this.CaptureConfig.innerChannel = this.innerChannelList[index]
                console.log(`innerChannel: ${this.innerChannelList[index]}`)
              })
          }.margin({ top: 10 })

          Row() {
            Text("width:").fontSize(24)
            Select(this.widthValueList).value(this.selectedWidthKey)
              .onSelect((index, value) => {
                this.selectedWidthKey = value
                console.log(`selectedWidthKey: ${value}`)
                this.CaptureConfig.width = this.widthList[index]
                console.log(`width: ${this.widthList[index]}`)
              })

            Text("height:").fontSize(24)
            Select(this.heightValueList).value(this.selectedHeightKey)
              .onSelect((index, value) => {
                this.selectedHeightKey = value
                console.log(`selectedHeightKey: ${value}`)
                this.CaptureConfig.height = this.heightList[index]
                console.log(`height: ${this.heightList[index]}`)
              })
          }.margin({ top: 10 })

          Row() {
            Text("isMic:").fontSize(24)
            Select(this.isMicValueList).value(this.selectedIsMicKey)
              .onSelect((index, value) => {
                this.selectedIsMicKey = value
                console.log(`selectedIsMicKey: ${value}`)
                this.CaptureConfig.isMic = this.isMicBoolList[index]
                console.log(`isMic: ${this.isMicBoolList[index]}`)
              })
          }.margin({ top: 10 })

          Divider().strokeWidth(6).color(Color.Blue)

          Row() {
            Button() {
              Text("开始录制").fontSize(22).fontColor(Color.White)
            }.width('100%').height(60).onClick(() => {
              this.startScreenCapture(this.CaptureConfig)
              this.start()
            })
          }.margin({ top: 10 }).width('100%')

          Row() {
            Button() {
              Text("获取帧率").fontSize(22).fontColor(Color.White)
            }.width('100%').height(60).onClick(() => {
              this.getFrameRate()
            })
          }.margin({ top: 10 }).width('100%')

        }.justifyContent(FlexAlign.Center)
        .size({ width: '100%', height: '100%' })
      }
    }
  }

  start = () => {
    let windowClass
    //TODO: use the  new api "window.createWindow(config: Configuration, callback: AsyncCallback<Window>): void" instead
    let config = { name: "floatWindow", windowType: window.WindowType.TYPE_FLOAT, ctx: getContext(this) };
    window.createWindow(config, (err, data) => {
      if (err.code) {
        console.error('Failed to create the floatWindow. Cause: ' + JSON.stringify(err));
        return;
      }
      console.info('Succeeded in creating the floatWindow. Data: ' + JSON.stringify(data));
      windowClass = data;
      windowClass.moveWindowTo(50, 50, (err) => {
        if (err.code) {
          console.error('Failed to move the window. Cause:' + JSON.stringify(err));
          return;
        }
        console.info('Succeeded in moving the window.');
      });
      windowClass.resize(250, 80, (err) => {
        if (err.code) {
          console.error('Failed to change the window size. Cause:' + JSON.stringify(err));
          return;
        }
        console.info('Succeeded in changing the window size.');
      });
      windowClass.loadContent('pages/ScreenCapture', (err, data) => {
        if (err.code) {
          console.error(TAG, 'Failed to load ScreenRecorder page. Cause: ' + JSON.stringify(data))
          return
        }
        console.info(TAG, 'Succeeded in loading ScreenRecorder page. Data:' + JSON.stringify(data))

        windowClass.show((err, data) => {
          if (err.code) {
            console.error(TAG, 'Failed to show window. Case: ' + JSON.stringify(data))
            return
          }
          console.info(TAG, 'Succeeded in showing the window. Data:' + JSON.stringify(data))
        })
      })
      windowClass.setBackgroundColor(Color.Transparent)
    })
    globalThis.floatWindow = windowClass
    globalThis.mainWindow.hide()
    emitter.once({ eventId: DESTROY_FLOAT_WINDOW }, () => {
      console.info(TAG, 'emitter once')
      globalThis.mainWindow.show()
      windowClass.destroyWindow((err, data) => {
        if (err.code) {
          console.error(TAG, 'Failed to destroy the window. Cause:' + JSON.stringify(err))
          return
        }
        console.info(TAG, 'Succeeded to destroy the window. Data:' + JSON.stringify(data))
      })
    })
  }
}