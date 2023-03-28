/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

import media from '@ohos.multimedia.media';
import { dateTime } from '../utils/DateTimeUtils';
import SaveCameraAsset from '../utils/SaveCameraAsset';

@Entry
@Component
struct audioRecording {
  private audioRecorder: media.AVRecorder;
  private fdPath: string = undefined;
  @State frameRate: number = 15;
  @State audioRecorderTimeText: string = '00:00:00';
  private seconds: number = 0;
  private timer: number = 0;
  private tag: string = '[Sample_AudioRecorder]';
  @State displayLog: string = undefined;
  private recorderState: string = 'free';
  @State isStartRecording: boolean = false;
  @State isPauseRecording: boolean = false;
  private mSaveCameraAsset: SaveCameraAsset = new SaveCameraAsset(this.tag);
  private mFileAssetId: number = 0;
  startRecordingBtn: any = $r('app.media.take_video_normal');
  stopRecordingBtn: any = $r('app.media.take_video_stop');
  pauseRecordingBtn: any = $r('app.media.ic_pause_white');
  resumeRecordingBtn: any = $r('app.media.take_video_resume');
  settingBtn: any = $r('app.media.setting');
  private avProfile = {
    audioBitrate: 48000, // set audioBitrate according to device ability
    audioChannels: 2, // set audioChannels, valid value 1-8
    audioCodec: media.CodecMimeType.AUDIO_AAC, // set audioCodec, AUDIO_AAC is the only choice
    audioSampleRate: 48000, // set audioSampleRate according to device ability
    fileFormat: media.ContainerFormatType.CFT_MPEG_4A, // set fileFormat, for video is m4a
  }
  private avConfig = {
    audioSourceType: media.AudioSourceType.AUDIO_SOURCE_TYPE_MIC,
    profile: this.avProfile,
    url: 'fd://'
  }

  aboutToAppear(): void {
    console.info(`${this.tag} aboutToAppear called`);
    console.info(`${this.tag} aboutToAppear done`);
  }

  aboutToDisappear(): void {
    console.info(`${this.tag} aboutToDisappear called`);
    console.info(`${this.tag} aboutToDisappear done`);
  }

  async failureCallback(error): Promise<void> {
    console.info(`case failureCallback called,errMessage is ${error.message}`);
  }

  async catchCallback(error): Promise<void> {
    console.info(`case catchCallback called,errMessage is ${error.message}`);
  }

  // show recording time
  getRecordTime(): void {
    this.timer = setInterval(() => {
      this.seconds += 1;
      this.audioRecorderTimeText = dateTime(this.seconds);
    }, 1000)
  }

  // create file fd
  async getFileFd(): Promise<void> {
    console.info(`${this.tag} getFileFd called`);
    this.mFileAssetId = await this.mSaveCameraAsset.createAudioFd();
    this.fdPath = 'fd://' + this.mFileAssetId.toString();
    this.avConfig.url = this.fdPath;
    console.info(`${this.tag} fdPath is: ${this.fdPath}`);
    console.info(`${this.tag} getFileFd done`);
  }

  async createAudioRecorder(): Promise<void> {
    await media.createAVRecorder().then((recorder) => {
      console.info(`${this.tag} case createAVRecorder called`);
      if (typeof (recorder) !== 'undefined') {
        this.audioRecorder = recorder;
      } else {
        console.info(`case create avRecorder failed!!!`);
        return
      }
    }, this.failureCallback).catch(this.catchCallback);
    this.setCallback();
  }

  // set callback on
  setCallback(): void {
    console.info(`case callback`);
    this.audioRecorder.on('stateChange', (state, reason) => {
      console.info(`case state has changed, new state is : ${state}`);
      switch (state) {
        case 'idle':
          this.recorderState = 'idle';
          break;
        case 'prepared':
          this.recorderState = 'prepared';
          break;
        case 'started':
          this.recorderState = 'started';
          this.getRecordTime();
          this.isStartRecording = true;
          this.isPauseRecording = false;
          break;
        case 'paused':
          this.recorderState = 'paused';
          clearInterval(this.timer);
          this.isPauseRecording = true;
          break;
        case 'stopped':
          this.recorderState = 'stopped';
          this.isStartRecording = false;
          break;
        case 'released':
          this.recorderState = 'released';
          break;
        case 'error':
          console.info(`case error state!!!`);
          this.recorderState = 'error';
          break;
        default:
          console.info(`case start is unknown`);
          break;
      }
    });
    this.audioRecorder.on('error', (err) => {
      console.info(`case avRecorder.on(error) called, errMessage is ${err.message}`);
    });
  }

  async prepareAudioRecorder(): Promise<void> {
    console.info(`${this.tag} case prepareAudioRecorder in`);
    await this.audioRecorder.prepare(this.avConfig).then(() => {
      console.info(`${this.tag} case prepare AVRecorder called`);
    }, this.failureCallback).catch(this.catchCallback);
    console.info(`${this.tag} case prepareAudioRecorder out`);
  }

  async startAudioRecording(): Promise<void> {
    console.info(`${this.tag} case startAudioRecording called`);
    await this.audioRecorder.start().then(() => {
      console.info(`${this.tag} case start AudioRecorder called`);
    }, this.failureCallback).catch(this.catchCallback);
  }

  async pauseAudioRecording(): Promise<void> {
    console.info(`${this.tag} case pauseAudioRecording called`);
    await this.audioRecorder.pause().then(() => {
      console.info(`${this.tag} case pause AudioRecorder called`);
    }, this.failureCallback).catch(this.catchCallback);
  }

  async resumeAudioRecording(): Promise<void> {
    console.info(`${this.tag} case resumeAudioRecording called`);
    await this.audioRecorder.resume().then(() => {
      console.info(`${this.tag} case resume AudioRecorder called`);
    }, this.failureCallback).catch(this.catchCallback);
  }

  async stopAudioRecording(): Promise<void> {
    console.info(`${this.tag} case stopAudioRecording called`);
    await this.audioRecorder.stop().then(() => {
      console.info(`${this.tag} case stop AudioRecorder called`);
    }, this.failureCallback).catch(this.catchCallback);
  }

  async resetAudioRecording(): Promise<void> {
    await this.audioRecorder.reset().then(() => {
      console.info(`${this.tag} case resetAudioRecording called`);
    }, this.failureCallback).catch(this.catchCallback);
  }

  async releaseAudioRecorder(): Promise<void> {
    if (this.audioRecorder) {
      this.audioRecorder.off('stateChange');
      this.audioRecorder.off('error');
      await this.audioRecorder.release().then(() => {
        console.info(`${this.tag} case releaseAudioRecorder called`);
      }, this.failureCallback).catch(this.catchCallback);
      this.audioRecorder = undefined;
    }
  }

  // close file fd
  async closeFd(): Promise<void> {
    console.info(`${this.tag} case closeFd called`);
    if (this.fdPath) {
      await this.mSaveCameraAsset.AudioPrepareFile.close(this.mFileAssetId);
      this.mFileAssetId = undefined;
      this.fdPath = undefined;
      console.info(`${this.tag} case closeFd done`);
    }
  }

  // start button process
  async startRecordingProcess(): Promise<void> {
    console.info(`${this.tag} startRecording called`);
    if (this.audioRecorder) {
      console.info(`${this.tag} audioRecorder exist,release it`);
      await this.audioRecorder.release();
    }
    await this.createAudioRecorder();
    await this.getFileFd();
    await this.prepareAudioRecorder();
    await this.startAudioRecording();
    console.info(`${this.tag} startRecording done`);
  }

  // pause button process
  async pauseRecordingProcess(): Promise<void> {
    console.info(`${this.tag} pauseRecording called`);
    if (this.recorderState === 'started') {
      console.info(`${this.tag} current state is started, to pause`);
      await this.pauseAudioRecording();
    }
    console.info(`${this.tag} pauseRecording done`);
  }

  // resume button process
  async resumeRecordingProcess(): Promise<void> {
    console.info(`${this.tag} resumeRecording called`);
    if (this.recorderState === 'paused') {
      console.info(`${this.tag} current state is paused, to resume`);
      await this.resumeAudioRecording();
    }
    console.info(`${this.tag} resumeRecording done`);
  }

  // stop button process
  async stopRecordingProcess(): Promise<void> {
    console.info(`${this.tag} stopRecording called`);
    if (this.recorderState === 'started' || this.recorderState === 'paused') {
      await this.stopAudioRecording();
    }
    await this.resetAudioRecording();
    await this.releaseAudioRecorder();
    await this.closeFd();
    clearInterval(this.timer);
    this.isStartRecording = false;
    this.isPauseRecording = false;
    this.seconds = 0;
    this.audioRecorderTimeText = '00:00:00';
    console.info(`${this.tag} stopRecording done`);
  }

  build() {
    Column() {
      Row() {
        Stack({ alignContent: Alignment.Bottom }) {
          Text(this.audioRecorderTimeText)
            .fontSize(20)
            .fontColor('white')
            .margin({ bottom: '95%' })
          Row({ space: 100 }) {
            if (!this.isStartRecording) {
              Image(this.startRecordingBtn)
                .width(50).height(50)
                .margin({ bottom: 10 })
                .onClick(() => this.startRecordingProcess())
            } else {
              Image(this.stopRecordingBtn)
                .width(50).height(50)
                .margin({ left: 10, bottom: 10 })
                .onClick(() => this.stopRecordingProcess())
              if (!this.isPauseRecording) {
                Image(this.pauseRecordingBtn)
                  .width(50).height(50)
                  .margin({ bottom: 10 })
                  .onClick(() => this.pauseRecordingProcess())
              } else {
                Image(this.resumeRecordingBtn)
                  .width(50).height(50)
                  .margin({ bottom: 10 })
                  .onClick(() => this.resumeRecordingProcess())
              }
            }
          }
        }.width('100%').height('100%')
      }.backgroundColor(Color.Black)
    }
  }
}
