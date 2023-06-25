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

import camera from '@ohos.multimedia.camera';
import media from '@ohos.multimedia.media';
import { dateTime } from '../utils/DateTimeUtils';
import SaveCameraAsset from '../utils/SaveCameraAsset';

@Entry
@Component
struct videoRecording {
  private surfaceId: string = '';
  xcomponentController: XComponentController = new XComponentController();
  private videoRecorder: media.AVRecorder;
  private cameraManager: camera.CameraManager;
  private cameras: any = undefined;
  private cameraInput: camera.CameraInput;
  private captureSession: camera.CaptureSession;
  private previewOutput: camera.PreviewOutput;
  private videoOutput: camera.VideoOutput;
  private videoOutSurfaceId: string = '';
  private fdPath: string = undefined;
  @State videoRecorderTimeText: string = '00:00:00';
  private seconds: number = 0;
  private timer: number = 0;
  private cameraIndex: number = 0;
  @State videoResolution: object = { 'frameWidth': 1280, 'frameHeight': 720 };
  @State framerate: number = 30; // set default video framerate 30
  @State videoCodecType: string = 'video/avc';
  private tag: string = '[Sample_VideoRecorder]';
  private recorderState: string = 'free';
  @State isStartRecording: boolean = false;
  @State isPauseRecording: boolean = false;
  private mSaveCameraAsset: SaveCameraAsset = new SaveCameraAsset(this.tag);
  private mFileAssetId: number = 0;
  private cameraOutputCapability: camera.CameraOutputCapability = undefined;
  private videoProfiles: any = undefined;
  private previewProfiles: any = undefined;

  // 按钮图标
  startRecordingBtn: any = $r('app.media.take_video_normal');
  stopRecordingBtn: any = $r('app.media.take_video_stop');
  pauseRecordingBtn: any = $r('app.media.ic_pause_white');
  resumeRecordingBtn: any = $r('app.media.take_video_resume');
  settingBtn: any = $r('app.media.setting');
  private avProfile = {
    audioBitrate: 200000, // set audioBitrate according to device ability
    audioChannels: 2, // set audioChannels, valid value 1-8
    audioCodec: media.CodecMimeType.AUDIO_AAC, // set audioCodec, AUDIO_AAC is the only choice
    audioSampleRate: 48000, // set audioSampleRate according to device ability
    fileFormat: media.ContainerFormatType.CFT_MPEG_4, // set fileFormat, for video is mp4
    videoBitrate: 300000, // set videoBitrate according to device ability
    videoCodec: media.CodecMimeType.VIDEO_AVC, // set videoCodec, avc or mpeg4 can be selected
    videoFrameWidth: 1280, // set videoFrameWidth according to device ability
    videoFrameHeight: 720, // set videoFrameHeight according to device ability
    videoFrameRate: 30 // set videoFrameRate according to device ability
  }
  private avConfig = {
    audioSourceType: media.AudioSourceType.AUDIO_SOURCE_TYPE_MIC,
    videoSourceType: media.VideoSourceType.VIDEO_SOURCE_TYPE_SURFACE_YUV,
    profile: this.avProfile,
    url: 'fd://',
    rotation: 90, // rotation angle, 0,90,180,270 can be selected
    location: { latitude: 30, longitude: 130 } // the location you recorde
  }

  aboutToAppear(): void {
    console.info(`${this.tag} aboutToAppear`);
  }

  aboutToDisappear(): void {
    console.info(`${this.tag} aboutToDisappear called`);
    this.releaseCamera();
  }

  onPageShow(): void {
    console.info(`${this.tag} onPageShow called`);
    this.enterInit();
  }

  onPageHide(): void {
    console.info(`${this.tag} onPageHide called`);
    this.stopRecordingProcess();
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
      this.videoRecorderTimeText = dateTime(this.seconds);
    }, 1000)
  }

  // get recording file fd
  async getFileFd(): Promise<void> {
    console.info(`${this.tag} getFileFd called`);
    this.mFileAssetId = await this.mSaveCameraAsset.createVideoFd();
    this.fdPath = 'fd://' + this.mFileAssetId.toString();
    this.avConfig.url = this.fdPath;
    console.info(`${this.tag} fdPath is: ${this.fdPath}`);
    console.info(`${this.tag} getFileFd done`);
  }

  // get camera ability
  async initCamera(): Promise<void> {
    console.info(`${this.tag} init camera called`);
    if (this.cameraManager) {
      console.info(`${this.tag} cameraManager already exits`);
      return;
    }
    console.info(`${this.tag} [camera] case to get cameraManager`);
    this.cameraManager = camera.getCameraManager(globalThis.abilityContext);
    if (this.cameraManager) {
      console.info(`${this.tag} [camera] case getCameraManager success`);
    } else {
      console.info(`${this.tag} [camera] case getCameraManager failed`);
      return;
    }
    this.cameras = this.cameraManager.getSupportedCameras();
    if (this.cameras) {
      console.info(`${this.tag} [camera] case getCameras success`);
    } else {
      console.info(`${this.tag} [camera] case getCameras failed`);
    }
    this.cameraOutputCapability = this.cameraManager.getSupportedOutputCapability(this.cameras[this.cameraIndex]);
    this.videoProfiles = this.cameraOutputCapability.videoProfiles;
    console.info(`${this.tag} init camera done`);
  }

  // create camera input
  async createCameraInput(): Promise<void> {
    console.info(`${this.tag} createCameraInput called`);
    this.cameraInput = this.cameraManager.createCameraInput(this.cameras[this.cameraIndex]);
    if (this.cameraInput) {
      console.info(`${this.tag} [camera] case createCameraInput success`);
    } else {
      console.info(`${this.tag} [camera] case createCameraInput failed`);
      return;
    }
    await this.cameraInput.open().then(() => {
      console.info(`[camera] case cameraInput.open() called`);
    }, this.failureCallback).catch(this.catchCallback);
  }

  // create camera preview
  async createPreviewOutput(): Promise<void> {
    console.info(`${this.tag} createPreviewOutput called`);
    this.previewProfiles = this.cameraOutputCapability.previewProfiles;
    this.previewProfiles[0].size.height = this.videoResolution['frameHeight'];
    this.previewProfiles[0].size.width = this.videoResolution['frameWidth'];
    if (this.previewProfiles[0].format === camera.CameraFormat.CAMERA_FORMAT_YUV_420_SP) {
      console.info(`${this.tag} [camera] case format is VIDEO_SOURCE_TYPE_SURFACE_YUV`);
      this.avConfig.videoSourceType = media.VideoSourceType.VIDEO_SOURCE_TYPE_SURFACE_YUV;
    } else {
      console.info(`${this.tag} [camera] case format is VIDEO_SOURCE_TYPE_SURFACE_ES`);
      this.avConfig.videoSourceType = media.VideoSourceType.VIDEO_SOURCE_TYPE_SURFACE_ES;
    }
    this.previewOutput = this.cameraManager.createPreviewOutput(this.previewProfiles[0], this.surfaceId);
    if (!this.previewOutput) {
      console.info(`${this.tag} create previewOutput failed!`);
    }
    console.info(`${this.tag} createPreviewOutput done`);
  }

  // create camera capture session
  async createCaptureSession(): Promise<void> {
    console.info(`${this.tag} createCaptureSession called`);
    this.captureSession = this.cameraManager.createCaptureSession();
    if (!this.captureSession) {
      console.info(`${this.tag} createCaptureSession failed!`);
      return
    }
    this.captureSession.beginConfig();
    this.captureSession.addInput(this.cameraInput);
    this.captureSession.addOutput(this.previewOutput);
    await this.captureSession.commitConfig().then(() => {
      console.info(`${this.tag} case commitConfig success`);
    }, this.failureCallback).catch(this.catchCallback);
  }

  // start captureSession
  async startCaptureSession(): Promise<void> {
    console.info(`${this.tag} startCaptureSession called`);
    if (!this.captureSession) {
      console.info(`${this.tag} CaptureSession does not exists!`);
      return
    }
    await this.captureSession.start().then(() => {
      console.info(`${this.tag} case start captureSession success`);
    }, this.failureCallback).catch(this.catchCallback);
  }

  // remove previous videoOutput
  async removeVideoOutput(): Promise<void> {
    console.info(`${this.tag} removeVideoOutput called`);
    if (this.videoOutput) {
      console.info(`${this.tag} videoOutput already exits, remove it`);
      this.captureSession.removeOutput(this.videoOutput);
      console.info(`${this.tag} remove videoOutput done`);
    }
    console.info(`${this.tag} removeVideoOutput done`);
  }

  // create video output
  async createVideoOutput(): Promise<void> {
    console.info(`${this.tag} createVideoOutput called`);
    if (this.videoRecorder) {
      this.videoProfiles[this.cameraIndex].size.height = this.videoResolution['frameHeight'];
      this.videoProfiles[this.cameraIndex].size.width = this.videoResolution['frameWidth'];
      this.videoOutput = this.cameraManager.createVideoOutput(this.videoProfiles[this.cameraIndex], this.videoOutSurfaceId);
      if (!this.videoOutput) {
        console.info(`[camera] case create videoOutPut fail!!!`);
        return;
      } else {
        console.info(`[camera] case create videoOutPut done, videoOutPut is not null`);
      }
    } else {
      console.info(`${this.tag} videoRecorder is null, createVideoOutput failed`);
      return;
    }
  }

  // add video output into CaptureSession
  async restartVideoOutput(): Promise<void> {
    console.info(`${this.tag} restartVideoOutput called`);
    await this.captureSession.stop().then(() => {
      console.info(`${this.tag} case stop captureSession success`);
    }, this.failureCallback).catch(this.catchCallback);
    this.captureSession.beginConfig();
    await this.removeVideoOutput();
    await this.createVideoOutput();
    this.captureSession.addOutput(this.videoOutput);
    await this.captureSession.commitConfig().then(() => {
      console.info(`${this.tag} case commitConfig success`);
    }, this.failureCallback).catch(this.catchCallback);
    await this.startCaptureSession();
    if (this.videoOutput) {
      await this.videoOutput.start().then(() => {
        console.info(`[camera] case videoOutput start success`);
      }, this.failureCallback).catch(this.catchCallback);
    } else {
      console.info(`${this.tag} videoOutput is null!!!`);
      return;
    }
  }

  // start videoOutput
  async startVideoOutput(): Promise<void> {
    console.info(`${this.tag} startVideoOutput called`);
    if (this.videoOutput) {
      await this.videoOutput.start().then(() => {
        console.info(`[camera] case videoOutput start success`);
      }, this.failureCallback).catch(this.catchCallback);
    } else {
      console.info(`${this.tag} videoOutput is null!!!`);
      return;
    }
  }

  // stop videoOutput
  async stopVideoOutput(): Promise<void> {
    console.info(`${this.tag} stopVideoOutput called`);
    if (this.videoOutput) {
      await this.videoOutput.stop().then(() => {
        console.info(`[camera] case videoOutput stop success`);
      }, this.failureCallback).catch(this.catchCallback);
    } else {
      console.info(`${this.tag} videoOutput is null!!!`);
      return;
    }
  }

  // release camera
  async releaseCameraInput(): Promise<void> {
    console.info(`${this.tag} releaseCameraInput called`);
    if (this.cameraInput) {
      this.cameraInput = null;
    }
    console.info(`${this.tag} releaseCameraInput done`);
  }

  // release preview
  async releasePreviewOutput(): Promise<void> {
    console.info(`${this.tag} releasePreviewOutput called`);
    if (this.previewOutput) {
      await this.previewOutput.release().then(() => {
        console.info(`[camera] case main previewOutput release called`);
      }, this.failureCallback).catch(this.catchCallback);
      this.previewOutput = null;
    }
    console.info(`${this.tag} releasePreviewOutput done`);
  }

  // release video output
  async releaseVideoOutput(): Promise<void> {
    console.info(`${this.tag} releaseVideoOutput called`);
    if (this.videoOutput) {
      await this.videoOutput.release().then(() => {
        console.info(`[camera] case main videoOutput release called`);
      }, this.failureCallback).catch(this.catchCallback);
      this.videoOutput = null;
    }
    console.info(`${this.tag} releaseVideoOutput done`);
  }

  // stop captureSession
  async stopCaptureSession(): Promise<void> {
    console.info(`${this.tag} stopCaptureSession called`);
    if (this.captureSession) {
      await this.captureSession.stop().then(() => {
        console.info(`[camera] case main captureSession stop success`);
      }, this.failureCallback).catch(this.catchCallback);
    }
    console.info(`${this.tag} stopCaptureSession done`);
  }

  // release captureSession
  async releaseCaptureSession(): Promise<void> {
    console.info(`${this.tag} releaseCaptureSession called`);
    if (this.captureSession) {
      await this.captureSession.release().then(() => {
        console.info(`[camera] case main captureSession release success`);
      }, this.failureCallback).catch(this.catchCallback);
      this.captureSession = null;
    }
    console.info(`${this.tag} releaseCaptureSession done`);
  }

  // clear camera environment
  async releaseCamera(): Promise<void> {
    console.info(`${this.tag} releaseCamera called`);
    await this.stopCaptureSession();
    await this.releaseCameraInput();
    await this.releasePreviewOutput();
    await this.releaseVideoOutput();
    await this.releaseCaptureSession();
    console.info(`${this.tag} releaseCamera done`);
  }

  // set callback on
  setCallback(): void {
    console.info(`case callback`);
    this.videoRecorder.on('stateChange', (state, reason) => {
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
    this.videoRecorder.on('error', (err) => {
      console.info(`case avRecorder.on(error) called, errMessage is ${err.message}`);
    });
  }

  async createVideoRecorder(): Promise<void> {
    await media.createAVRecorder().then((recorder) => {
      console.info(`${this.tag} case createAVRecorder called`);
      if (typeof (recorder) !== 'undefined') {
        this.videoRecorder = recorder;
      } else {
        console.info(`case create avRecorder failed!!!`);
        return
      }
    }, this.failureCallback).catch(this.catchCallback);
    this.setCallback();
  }

  async prepareVideoRecorder(): Promise<void> {
    console.info(`${this.tag} case prepareVideoRecorder in`);
    if (this.videoCodecType === 'video/mpeg4') {
      this.avConfig.profile.videoCodec = media.CodecMimeType.VIDEO_MPEG4;
    } else {
      this.avConfig.profile.videoCodec = media.CodecMimeType.VIDEO_AVC;
    }
    this.avConfig.profile.videoFrameHeight = this.videoResolution['frameHeight'];
    this.avConfig.profile.videoFrameWidth = this.videoResolution['frameWidth'];
    this.avConfig.profile.videoFrameRate = this.framerate;
    await this.videoRecorder.prepare(this.avConfig).then(() => {
      console.info(`${this.tag} case prepare AVRecorder called`);
    }, this.failureCallback).catch(this.catchCallback);
    console.info(`${this.tag} case prepareVideoRecorder out`);
  }

  async getInputSurface(): Promise<void> {
    console.info(`${this.tag} case getInputSurface in`);
    await this.videoRecorder.getInputSurface().then((outPutSurface) => {
      console.info(`case getInputSurface called`);
      if (!outPutSurface) {
        console.error(`${this.tag} case getInputSurface ID is none`);
        return
      }
      this.videoOutSurfaceId = outPutSurface;
      console.info(`case outPutSurface surfaceID is: ${this.videoOutSurfaceId}`);
    }, this.failureCallback).catch(this.catchCallback);
    console.info(`${this.tag} case getInputSurface out`);
  }

  async startVideoRecording(): Promise<void> {
    console.info(`${this.tag} case startVideoRecording called`);
    await this.videoRecorder.start().then(() => {
      console.info(`${this.tag} case start VideoRecorder called`);
    }, this.failureCallback).catch(this.catchCallback);
  }

  async pauseVideoRecording(): Promise<void> {
    console.info(`${this.tag} case pauseVideoRecording called`);
    await this.videoRecorder.pause().then(() => {
      console.info(`${this.tag} case pause VideoRecorder called`);
    }, this.failureCallback).catch(this.catchCallback);
  }

  async resumeVideoRecording(): Promise<void> {
    console.info(`${this.tag} case resumeVideoRecording called`);
    await this.videoRecorder.resume().then(() => {
      console.info(`${this.tag} case resume VideoRecorder called`);
    }, this.failureCallback).catch(this.catchCallback);
  }

  async stopVideoRecording(): Promise<void> {
    console.info(`${this.tag} case stopVideoRecording called`);
    await this.videoRecorder.stop().then(() => {
      console.info(`${this.tag} case stop VideoRecorder called`);
    }, this.failureCallback).catch(this.catchCallback);
  }

  async resetVideoRecording(): Promise<void> {
    await this.videoRecorder.reset().then(() => {
      console.info(`${this.tag} case resetVideoRecording called`);
    }, this.failureCallback).catch(this.catchCallback);
  }

  async releaseVideoRecorder(): Promise<void> {
    if (this.videoRecorder) {
      this.videoRecorder.off('stateChange');
      this.videoRecorder.off('error');
      await this.videoRecorder.release().then(() => {
        console.info(`${this.tag} case releaseVideoRecorder called`);
      }, this.failureCallback).catch(this.catchCallback);
      this.videoRecorder = undefined;
    }
  }

  // close file fd
  async closeFd(): Promise<void> {
    console.info(`${this.tag} case closeFd called`);
    if (this.fdPath) {
      await this.mSaveCameraAsset.VideoPrepareFile.close(this.mFileAssetId);
      this.mFileAssetId = undefined;
      this.fdPath = undefined;
      console.info(`${this.tag} case closeFd done`);
    }
  }

  // init camera
  async enterInit(): Promise<void> {
    console.info(`${this.tag} enterInit called`);
    await this.initCamera(); // 获取设备相机信息
    await this.createCameraInput(); // 打开相机
    await this.createPreviewOutput(); // 预览流
    await this.createCaptureSession(); // 会话，保存一次相机运行需要的所有资源，并向相机设备申请完成相机功能
    await this.startCaptureSession(); // 开启会话
    console.info(`${this.tag} exit enterInit`);
  }

  // start button process
  async startRecordingProcess(): Promise<void> {
    console.info(`${this.tag} startRecording called`);
    await this.createVideoRecorder();
    await this.getFileFd();
    await this.prepareVideoRecorder();
    await this.getInputSurface();
    await this.restartVideoOutput();
    await this.startVideoRecording();
    console.info(`${this.tag} startRecording done`);
  }

  // pause button process
  async pauseRecordingProcess(): Promise<void> {
    console.info(`${this.tag} pauseRecording called`);
    if (this.recorderState === 'started') {
      console.info(`${this.tag} current state is started, to pause`);
      await this.pauseVideoRecording();
      await this.stopVideoOutput();
    }
    console.info(`${this.tag} pauseRecording done`);
  }

  // resume button process
  async resumeRecordingProcess(): Promise<void> {
    console.info(`${this.tag} resumeRecording called`);
    if (this.recorderState === 'paused') {
      console.info(`${this.tag} current state is paused, to resume`);
      await this.startVideoOutput();
      await this.resumeVideoRecording();
    }
    console.info(`${this.tag} resumeRecording done`);
  }

  // stop button process
  async stopRecordingProcess(): Promise<void> {
    console.info(`${this.tag} stopRecording called`);
    if (this.recorderState === 'started' || this.recorderState === 'paused') {
      await this.stopVideoRecording();
    }
    await this.resetVideoRecording();
    await this.releaseVideoRecorder();
    await this.stopVideoOutput();
    this.videoOutput = undefined;
    await this.closeFd();
    clearInterval(this.timer);
    this.isStartRecording = false;
    this.isPauseRecording = false;
    this.seconds = 0;
    this.videoRecorderTimeText = '00:00:00';
    console.info(`${this.tag} stopRecording done`);
  }

  // detail button
  @Builder ProfileInfo() {
    Flex({ direction: FlexDirection.Column, alignItems: ItemAlign.Start, justifyContent: FlexAlign.Center }) {
      Flex({ direction: FlexDirection.Row }) {
        Text("分辨率：").fontSize(16)
        Text(this.videoResolution['frameWidth'] + '*' + this.videoResolution['frameHeight']).fontSize(16)
      }.margin('5%')

      Flex({ direction: FlexDirection.Row }) {
        Text("帧率：").fontSize(16)
        Text(this.framerate.toString()).fontSize(16)
      }.margin('5%')

      Flex({ direction: FlexDirection.Row }) {
        Text("编码格式：").fontSize(16)
        Text(this.videoCodecType).fontSize(16)
      }.margin('5%')
    }
    .width(200).height(200)
    .opacity(0.4)
  }

  build() {
    Column() {
      Row() {
        Stack({ alignContent: Alignment.Bottom }) {
          XComponent({
            id: 'xcomponent1',
            type: 'surface',
            controller: this.xcomponentController
          })
            .onLoad(() => {
              this.xcomponentController.setXComponentSurfaceSize({ surfaceWidth: 1920, surfaceHeight: 1080 })
              this.surfaceId = this.xcomponentController.getXComponentSurfaceId()
              console.info(`${this.tag} [VideoRecorder] surfaceId: ${this.surfaceId}`)
              console.info(`${this.tag} XComponent1 loaded`);
              this.enterInit();
            })
            .width('100%').height('100%')
          Text(this.videoRecorderTimeText)
            .fontSize(20)
            .fontColor('white')
            .margin({ bottom: '95%' })
          Button('i')
            .width('120px')
            .height('120px')
            .backgroundColor('white')
            .fontColor('white')
            .opacity(0.5)
            .margin({ bottom: '95%', left: '80%' })
            .bindContextMenu(this.ProfileInfo, ResponseType.LongPress)
          Row({ space: 100 }) {
            if (!this.isStartRecording) {
              Image(this.settingBtn)
                .width(50).height(50)
                .margin({ left: 10, bottom: 10 })
                .onClick(() => {
                  if (this.dialogController !== undefined) {
                    console.info(`${this.tag} to open setting dialog`);
                    this.dialogController.open()
                  }
                })
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
      }
    }
  }

  dialogController: CustomDialogController = new CustomDialogController({
    builder: CustomDialogExample({
      cancel: this.onCancel,
      confirm: this.onAccept,
      videoResolution: $videoResolution,
      videoFrameRate: $framerate,
      videoCodec: $videoCodecType,
    }),
    cancel: this.existApp,
    autoCancel: false,
  })

  onCancel() {
    console.info(`Callback when the cancel button is clicked`)
  }

  onAccept() {
    console.info(`Callback when the confirm button is clicked`)
  }

  existApp() {
    console.info(`Click the callback in the blank area`)
  }
}


@CustomDialog
struct CustomDialogExample {
  @Link videoResolution: Object
  @Link videoFrameRate: number
  @Link videoCodec: string
  controller: CustomDialogController
  cancel: () => void
  confirm: () => void
  @State current_videoResolution: Object = { 'frameWidth': 1280, 'frameHeight': 720 }
  @State current_videoResolutionText: string = '[16:9]720p'
  @State current_videoFramerate: number = 30
  @State current_videoCodec: string = 'video/avc'
  private init_videoResolution: Object = { 'frameWidth': 1280, 'frameHeight': 720 }
  private init_videoFramerate: number = 30
  private init_videoCodec: string = 'video/avc'
  @State videoResolutionList: Array<Object> = [
    { 'itemValue': '[16:9]720p', 'value': { 'frameWidth': 1280, 'frameHeight': 720 } },
    { 'itemValue': '[16:9]1080p', 'value': { 'frameWidth': 1920, 'frameHeight': 1080 } },
  ]
  @State videoFrameRateList: Array<Object> = [
    { 'itemValue': '30fps', 'value': 30 },
    { 'itemValue': '60fps', 'value': 60 },
  ]
  @State videoCodecList: Array<Object> = [
    { 'itemValue': 'video/avc', 'value': 'video/avc' },
    { 'itemValue': 'video/mpeg4', 'value': 'video/mpeg4' },
  ]

  build() {
    Column() {
      Text('视频分辨率： ' + this.current_videoResolutionText)
        .width('100%')
        .fontSize(18)
        .margin({ bottom: 10 })
      List() {
        ForEach(this.videoResolutionList, (item, index) => {
          ListItem() {
            Text(item.itemValue)
              .fontSize(16)
              .width('100%')
          }.height(48)
          .onClick(() => {
            console.info(`${item.itemValue} resolution is clicked`)
            this.current_videoResolution = item.value;
            this.current_videoResolutionText = item.itemValue;
            console.info(`${item.itemValue} resolution is clicked`)
          })
        })
      }.listDirection(Axis.Vertical)
      .divider({ strokeWidth: 0.5, color: '#33000000', startMargin: 0, endMargin: 12 })

      Text('视频帧率： ' + this.current_videoFramerate)
        .width('100%')
        .fontSize(18)
        .margin({ bottom: 10 })
      List() {
        ForEach(this.videoFrameRateList, (item) => {
          ListItem() {
            Text(item.itemValue)
              .fontSize(16)
              .width('100%')
          }.height(48)
          .onClick(() => {
            console.info(`${item.itemValue} framerate is clicked`)
            this.current_videoFramerate = item.value
            console.info(`${item.itemValue} framerate is clicked`)
          })
        })
      }.listDirection(Axis.Vertical)
      .divider({ strokeWidth: 0.5, color: '#33000000', startMargin: 0, endMargin: 12 })

      Text('视频编码格式： ' + this.current_videoCodec)
        .width('100%')
        .fontSize(18)
        .margin({ bottom: 10 })
      List() {
        ForEach(this.videoCodecList, (item) => {
          ListItem() {
            Text(item.itemValue)
              .fontSize(16)
              .width('100%')
          }.height(48)
          .onClick(() => {
            this.current_videoCodec = item.itemValue
            console.info(`${item.itemValue} is clicked`)
          })
        })
      }.listDirection(Axis.Vertical)
      .divider({ strokeWidth: 0.5, color: '#33000000', startMargin: 0, endMargin: 12 })

      Flex({ justifyContent: FlexAlign.SpaceAround }) {
        Button('取消')
          .onClick(() => {
            this.videoResolution = this.init_videoResolution
            this.videoFrameRate = this.init_videoFramerate
            this.videoCodec = this.init_videoCodec
            this.controller.close()
            this.cancel()
          }).backgroundColor(0xffffff)
          .fontColor(Color.Black)
        Button('确认')
          .onClick(() => {
            this.videoResolution = this.current_videoResolution
            this.videoFrameRate = this.current_videoFramerate
            this.videoCodec = this.current_videoCodec
            this.controller.close()
            this.confirm()
          }).backgroundColor(0xffffff)
          .fontColor(Color.Red)
      }.margin({ bottom: 10 })
    }.margin(24)
  }
}
