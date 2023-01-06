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
import router from '@ohos.router';
import { CommonConstants } from '../common/constants/CommonConstants';
import AVPlayerUtils from '../pages/Utils/AVPlayerUtils';
import MediaLibraryUtils from '../pages/Utils/MediaLibraryUtils'
import mediaLibrary from '@ohos.multimedia.mediaLibrary';
import media from '@ohos.multimedia.media';
import audio from '@ohos.multimedia.audio';

const TAG:string = 'MediaDemo AVPlayer:'
@Entry
@Component
struct AVPlayer {
  @State message: string = 'Hello World'
  @State ratio: number = 1.0
  @State mediaLibUtils:MediaLibraryUtils = new MediaLibraryUtils()
  @State isPlaying:boolean = true
  @State isShow: boolean = false
  @State currentTime:string = '0'
  @State durationTimeText:string = '100'
  @State setValue:number = 0
  @State isLoop: boolean = false
  @State videoName:string = ''
  @State avPlayer:AVPlayerUtils = new AVPlayerUtils()
  private duration: number = -1
  private playPath:string = globalThis.filesDir + '/H264_ACC.mp4'
  private fdPath:string = ''
  private fdHead:string = 'fd://'
  private surfaceID:string = ''
  private isPrepare:boolean = false
  private isNext = false
  private fileAsset:mediaLibrary.FileAsset
  private index:number = -1
  private fd:number
  private movingTime:number = -1
  private screenWidth:number = 1080
  private screenHeight:number = 1920
  private volumeValue:number = 1.00
  private mXcomponentController:XComponentController = new XComponentController()
  private isFinish:boolean = false
  // 界面初始化函数
  async aboutToAppear() {
    this.videoName = router.getParams()[CommonConstants.KEY_PARAM_DATA];
    this.index = CommonConstants.VIDEO_LISTS.map(item=>item).indexOf(this.videoName)
    console.info(TAG + 'aboutToAppear success, and play source is' + this.videoName)
  }
  // 界面销毁监听
  async aboutToDisappear() {
    console.info(TAG + 'aboutToDisappear success')
    await this.avPlayer.release()
  }
  // 设置播放时间上报监听
  async timeUpdate() {
    this.avPlayer.avPlayer.on('timeUpdate', (time: number) => {
      console.info(TAG + 'timeUpdate called: time is :' + time)
      this.currentTime = this.mediaLibUtils.getShowTime(time)
      this.setValue = Math.round((time/this.duration) * 100)
    })
  }
 // 设置错误监听
  async setErrorCallback() {
    this.avPlayer.avPlayer.on('error', (error) => {
      console.error(TAG + 'error happened,message is :' + error.message)
    })
  }

  // 视频信息上报函数
  async setVideoInfo() {
    // 音量变化回调函数
    this.avPlayer.avPlayer.on('volumeChange', (vol:number) => {
      console.info(TAG + 'volumeChange success,and new volume is :' + vol)
    })
    // 视频播放结束触发回调
    this.avPlayer.avPlayer.on('endOfStream', () => {
      console.info(TAG + 'endOfStream success')
    })
    // seek操作回调函数
    this.avPlayer.avPlayer.on('seekDone', (seekDoneTime:number) => {
      console.info(TAG + 'seekDone success,and seek time is:' + seekDoneTime)
    })
    // 设置倍速播放回调函数
    this.avPlayer.avPlayer.on('speedDone', (speed:number) => {
      console.info(TAG + 'speedDone success,and speed value is:' + speed)
    })
    // bitrate设置成功回调函数
    this.avPlayer.avPlayer.on('bitrateDone', (bitrate:number) => {
      console.info(TAG + 'bitrateDone success,and bitrate value is:' + bitrate)
    })
    // 缓冲上报回调函数
    this.avPlayer.avPlayer.on('bufferingUpdate', (infoType: media.BufferingInfoType, value: number) => {
      console.info(TAG + 'bufferingUpdate success,and infoType value is:' + infoType + ', value is :' + value)
    })
    // 首帧上报回调函数
    this.avPlayer.avPlayer.on('startRenderFrame', () => {
      console.info(TAG + 'startRenderFrame success')
    })
    // 视频宽高上报回调函数
    this.avPlayer.avPlayer.on('videoSizeChange', (width: number, height: number) => {
      console.info(TAG + 'videoSizeChange success,and width is:' + width + ', height is :' + height)
    })
    // 焦点上报回调函数
    this.avPlayer.avPlayer.on('audioInterrupt', (info: audio.InterruptEvent) => {
      console.info(TAG + 'audioInterrupt success,and InterruptEvent info is:' + info)
    })
    // HLS上报所有支持的比特率
    this.avPlayer.avPlayer.on('availableBitrates', (bitrates: Array<number>) => {
      console.info(TAG + 'availableBitrates success,and availableBitrates length is:' + bitrates.length)
    })
  }

  // 状态机上报回调函数
  async setStateChangeCallback() {
    this.avPlayer.avPlayer.on('stateChange', async (state, reason) => {
      switch (state) {
        case 'idle':
          console.info(TAG + 'state idle called')
          break;
        case 'initialized':
          console.info(TAG + 'state initialized called ')
          if (this.isNext) {
            await this.avPlayer.avPlayer.prepare()
          }
          break;
        case 'prepared':
          console.info(TAG + 'state prepared called')
          if (this.isNext) {
            this.play()
            this.isNext = false
          }
          break;
        case 'playing':
          console.info(TAG + 'state playing called')
          break;
        case 'paused':
          console.info(TAG + 'state paused called')
          break;
        case 'completed':
          console.info(TAG + 'state completed called')
          this.isPlaying = !this.isPlaying
          this.isFinish = true
          break;
        case 'stopped':
          console.info(TAG + 'state stopped called')
          break;
        case 'released':
          console.info(TAG + 'state released called')
          break;
        case 'error':
          console.info(TAG + 'state error called')
          break;
        default:
          console.info(TAG + 'unkown state :' + state)
          break;
      }
    })
  }

  // 播放文件打开函数
  async openMediaFile(fileName: string): Promise<mediaLibrary.FileAsset> {
    console.info(TAG + 'openMediaFile start')
    let fileAsset = await this.mediaLibUtils.findFile('0', fileName)
    console.info(TAG + 'openMediaFile end')
    return fileAsset;
  }
  // 初始化函数
  async initAVPlayer() {
    console.info(TAG + 'initAVPlayer success')
    this.fileAsset = await this.openMediaFile(this.videoName)
    this.duration = this.fileAsset.duration
    this.durationTimeText = this.mediaLibUtils.getShowTime(this.fileAsset.duration)
    this.fd = await this.fileAsset.open('r')
    this.fdPath = this.fdHead + this.fd
    console.info(TAG + 'file id is : ' + this.fdPath)
    this.surfaceID = this.mXcomponentController.getXComponentSurfaceId()
    console.info(TAG + 'surfaceID is : ' + this.surfaceID)
    await this.avPlayer.initAVPlayer(this.fdPath)
    this.setStateChangeCallback()
    this.setErrorCallback()
    this.timeUpdate()
    this.setVideoInfo()
  }
  // 调用播放接口
  async play() {
    if (!this.isPrepare) {
      // 设置surfaceID，当播放的问题纯音频时，无需设置
      this.avPlayer.setSurface(this.surfaceID)
      await this.avPlayer.prepare()
      this.isPrepare = !this.isPrepare
    }
    console.info(TAG + 'start to play')
    this.avPlayer.play()
  }
  // 暂停接口
  async pause() {
    console.info(TAG + 'start to pause')
    this.avPlayer.pause()
  }

  // 切换下个视频的接口
  async nextVideo() {
    // 调用reset函数，重置播放器状态至idle
    await this.avPlayer.reset()
    // 关闭当前已经打开的视频文件fd
    await this.fileAsset.close(this.fd)
    // 获取当前播放文件的下一个视频文件在列表中的下标
    if (this.index == (CommonConstants.VIDEO_LISTS.length - 1)) {
      this.index = 0
    } else {
      this.index = this.index + 1
    }
    // 打开新的视频文件，并获取相关duration 和 fd数值
    this.fileAsset = await this.openMediaFile(CommonConstants.VIDEO_LISTS[this.index])
    this.duration = this.fileAsset.duration
    this.durationTimeText = this.mediaLibUtils.getShowTime(this.fileAsset.duration)
    this.videoName = this.fileAsset.displayName
    this.fd = await this.fileAsset.open('r')
    this.fdPath = this.fdHead + this.fd
    console.info(TAG + 'file id is : ' + this.fdPath)
    // 开始进入下一个视频的播放
    if (this.getState() === 'idle') {
      this.avPlayer.avPlayer.url = this.fdPath
      // 将当前切视频状态置为true
      this.isNext = true
      this.isPlaying = false
    } else {
      console.info(TAG + 'nextVideo failed,state is not idle')
    }
  }
  // 切换上个视频的接口
  async preVideo() {
    await this.avPlayer.reset()
    await this.fileAsset.close(this.fd)
    if (this.index == (0)) {
      this.index = CommonConstants.VIDEO_LISTS.length - 1
    } else {
      this.index = this.index - 1
    }
    this.fileAsset = await this.openMediaFile(CommonConstants.VIDEO_LISTS[this.index])
    this.duration = this.fileAsset.duration
    this.durationTimeText = this.mediaLibUtils.getShowTime(this.fileAsset.duration)
    this.fd = await this.fileAsset.open('r')
    this.fdPath = this.fdHead + this.fd
    console.info(TAG + 'file id is : ' + this.fdPath)
    if (this.getState() === 'idle') {
      this.avPlayer.avPlayer.url = this.fdPath
      // 将当前切视频状态置为true
      this.isNext = true
      this.isPlaying = false
    } else {
      console.info(TAG + 'nextVideo failed,state is not idle')
    }
  }
  // 获取当前播放时间函数
  getCurrentTime(): number {
    return this.avPlayer.getCurrentTime()
  }
  // 获取当前播放状态函数
  getState(): string {
    return this.avPlayer.getState()
  }

  build() {
    Stack({ alignContent: Alignment.Bottom}) {
      Row() {
        XComponent({
          id: 'componentId',
          type: 'surface',
          controller: this.mXcomponentController
        })
        .onLoad(() => {
          // 加载完成后调用初始化播放器函数
          this.initAVPlayer()
        })
        .width('100%')
        .aspectRatio(this.ratio)
      }
      .height('100%')
      .width('100%')
      .justifyContent(FlexAlign.Center).position({x : 0, y : 0})
      .alignItems(VerticalAlign.Top)
      .onClick(() => {this.isShow = !this.isShow})
      .gesture(
        GestureGroup(GestureMode.Exclusive,
          PanGesture({direction:PanDirection.Horizontal})
            .onActionStart(async () => {
              console.info(TAG + 'onActionStart moving start')
            })
            .onActionUpdate((event: GestureEvent) => {
              // 左右滑动触发seek
              console.info(TAG + 'event.offsetX is :' + event.offsetX + ',screenWidth is :' + this.screenWidth)
              this.movingTime = parseInt(((event.offsetX/this.screenWidth)*this.duration).toString())
              console.info(TAG + 'onActionStart moving update, moving time is :' + this.movingTime)
              if (this.isFinish) {
                this.avPlayer.play()
                this.isPlaying = !this.isPlaying
                this.isFinish = false
              }
              this.avPlayer.avPlayer.seek(this.avPlayer.getCurrentTime() + this.movingTime, 3)
            })
            .onActionEnd(() => {
              console.info(TAG + 'onActionStart moving end')
            }),
          PanGesture({direction:PanDirection.Vertical})
            .onActionStart(() => {
              console.info(TAG + 'onActionStart  Vertical moving start')
            })
            .onActionUpdate((event: GestureEvent) => {
              // 上下滑动触发音量调节
              let volumeNum = parseFloat(((event.offsetY/this.screenHeight)*1.00).toString())
              this.volumeValue = this.volumeValue - volumeNum
              if (this.volumeValue < 0) {
                this.volumeValue = 0.00
              } else if (this.volumeValue > 1.00) {
                this.volumeValue = 1.00
              }
              console.info(TAG + 'onActionStart Vertical moving update, moving time is :' + this.volumeValue)
              this.avPlayer.avPlayer.setVolume(this.volumeValue)
            })
            .onActionEnd(() => {
              console.info(TAG + 'onActionStart  Vertical moving end')
            }))
      )
      Column() {
        Row() {
          Text(this.videoName).fontSize(15).width('30%').textAlign(TextAlign.Center)
        }
        Row() {
          Text(this.currentTime).fontSize(24).width('15%').textAlign(TextAlign.Center)
          Slider({
            value: this.setValue,
            min: 0,
            max: 100,
            step: 1,
            style: SliderStyle.OutSet
          }).blockColor(Color.Grey)
            .trackColor(Color.Orange)
            .trackColor(Color.Blue)
            .showSteps(true)
            .showTips(false)
            .width('65%')
            .onChange((value: number, mode: SliderChangeMode) => {
              // 拖动进度条触发seek
              if (this.isFinish) {
                this.avPlayer.play()
                this.isPlaying = !this.isPlaying
                this.isFinish = false
              }
              this.avPlayer.avPlayer.seek((value / 100) * this.duration, 3)
            })
          Text(this.durationTimeText).fontSize(24).width('15%').textAlign(TextAlign.Center)
        }
        .alignItems(VerticalAlign.Center)
        .visibility(this.isShow ? Visibility.Visible: Visibility.Hidden)
        Row(){
          // 点击执行上一个视频播放
          Image($r('app.media.previous'))
            .width('120px').height('120px')
            .margin({left: '10%'})
            .onClick(() => {
              this.preVideo();
            })
          // 根据状态不同设置播放暂停图标显示
          Image(this.isPlaying ? $r('app.media.pause') : $r('app.media.play'))
            .width('120px').height('120px')
            .margin({left: '10vp'})
            .onClick(() => {
              if (this.isPlaying) {
                this.play()
              } else {
                this.pause()
              }
              this.isPlaying = !this.isPlaying
            })
          // 点击执行下一个视频播放
          Image($r('app.media.next'))
            .width('120px').height('120px')
            .margin({left: '10vp'})
            .onClick(() => {
              this.nextVideo();
            })
          // 点击图标设置是否循环播放
          Image(this.isLoop ? $r('app.media.loop_true') : $r('app.media.loop_false'))
            .width('120px').height('120px')
            .margin({left: '10vp'})
            .onClick(() => {
              this.isLoop = !this.isLoop
              this.avPlayer.setLoop(this.isLoop)
            })
        }.alignItems(VerticalAlign.Center).margin({top: '10px', bottom: '10px'})
        .visibility(this.isShow ? Visibility.Visible: Visibility.Hidden)
      }
      .justifyContent(FlexAlign.Start)
    }
    .width('100%')
    .height('100%')
    .backgroundColor(Color.White)
  }
}