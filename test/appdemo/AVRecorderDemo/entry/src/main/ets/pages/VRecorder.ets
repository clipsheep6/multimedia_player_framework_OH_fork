/*
 * Copyright (C) 2022-2023 Huawei Device Co., Ltd.
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


import router from '@ohos.router';
import camera from '@ohos.multimedia.camera'
import media from '@ohos.multimedia.media';
import {getShownTimer, dateTime} from '../Utils/DateTimeUtils';
import SaveCameraAsset from '../Utils/SaveCameraAsset';

@Entry
@Component

struct videoRecording {
  private surfaceId: string = '';
  xcomponentController: XComponentController = new XComponentController();
  private videoRecorder: media.AVRecorder;
  private cameraManager: camera.CameraManager;
  private cameras: any = undefined;
  private cameraCount: number = 0;
  private cameraInput: camera.CameraInput;
  private captureSession: camera.CaptureSession;
  private previewOutput: camera.PreviewOutput;
  private videoOutput: camera.VideoOutput;
  private videoOutSurfaceId: string = '';
  @State file_name: string = 'xxx.mp4';
  private fdPath: string = undefined;
  @State frameRate: number = 15;
  @State videoRecorderTime: number = 5;
  @State videoRecorderTimeText: string = '00:00:00,000';
  private ms: number =  0;
  private timer: number = 0;
  private cameraIndex: number = 0;
  @State RowObjText: string = '1080p';
  @State videoResolution: object = {'frameWidth':640, 'frameHeight':480}
  private tag: string = '[videoRecorder]';
  @State displayLog: string = undefined;
  private recorderState: string = 'free';
  @State isStartRecording: boolean = false;
  @State isPauseRecording: boolean = false;
  private mSaveCameraAsset: SaveCameraAsset = new SaveCameraAsset(this.tag);
  private mFileAssetId: number = 0;
  private cameraOutputCapability: camera.CameraOutputCapability = undefined; // 摄像头能力
  private videoProfiles: any = undefined; // 录制配置参数
  private previewProfiles: any = undefined; // 预览界面配置参数

  private avProfile = {
    audioBitrate : 48000,
    audioChannels : 2,
    audioCodec : media.CodecMimeType.AUDIO_AAC,
    audioSampleRate : 48000,
    fileFormat : media.ContainerFormatType.CFT_MPEG_4,
    videoBitrate : 48000,
    videoCodec : media.CodecMimeType.VIDEO_MPEG4,
    videoFrameWidth : 640,
    videoFrameHeight : 480,
    videoFrameRate : 30
  }

  private avConfig = {
    audioSourceType : media.AudioSourceType.AUDIO_SOURCE_TYPE_MIC,
    videoSourceType : media.VideoSourceType.VIDEO_SOURCE_TYPE_SURFACE_YUV,
    profile : this.avProfile,
    url : 'fd://',
    rotation : 0,
    location : { latitude : 30, longitude : 130 }
  }

  // 按钮图标
  startRecordingBtn: any = $r('app.media.take_video_normal');
  stopRecordingBtn: any = $r('app.media.take_video_stop');
  pauseRecordingBtn: any = $r('app.media.take_video_pause');
  resumeRecordingBtn: any = $r('app.media.take_video_resume');
  switchCameraBtn: any = $r('app.media.small_switch_camera');
  settingBtn: any = $r('app.media.setting');

  aboutToAppear(): void {
    console.info(this.tag + 'aboutToAppear called');
    console.info(this.tag + 'aboutToAppear done');
  }

  aboutToDisappear(): void {
    console.info(this.tag + 'aboutToDisappear called');
    this.releaseCamera();
    console.info(this.tag + 'aboutToDisappear done');
  }

  async failureCallback(error) {
    console.info(`case failureCallback called,errMessage is ${error.message}`);
  }

  async catchCallback(error) {
    console.info(`case catchCallback called,errMessage is ${error.message}`);
  }

  // 录制时长显示
  getRecordTime() {
    this.timer = setInterval(() => {
      this.ms += 100;
      this.videoRecorderTimeText = dateTime(this.ms);
    }, 100)
  }

  // 获取录制文件fd提供给配置参数
  async getFileFd() {
    console.info(this.tag + 'getFileFd called');
    this.mFileAssetId = await this.mSaveCameraAsset.createVideoFd();
    this.fdPath = 'fd://' + this.mFileAssetId.toString();
    this.avConfig.url = this.fdPath;
    console.info(this.tag + 'fdPath is: ' + this.fdPath);
    console.info(this.tag + 'getFileFd done');
  }

  // 获取设备摄像头能力
  async initCamera() {
    console.info(this.tag + 'init camera called');
    if (this.cameraManager) {
      console.info(this.tag + 'cameraManager already exits');
      return;
    }
    this.cameraManager = await camera.getCameraManager(globalThis.abilityContext);
    if (this.cameraManager) {
      console.info(this.tag + '[camera] case getCameraManager success');
    } else {
      console.info(this.tag + '[camera] case getCameraManager failed');
      return;
    }
    this.cameras = await this.cameraManager.getSupportedCameras();
    if (this.cameras) {
      console.info(this.tag + '[camera] case getCameras success');
    } else {
      console.info(this.tag + '[camera] case getCameras failed');
    }
    this.cameraOutputCapability = await this.cameraManager.getSupportedOutputCapability(this.cameras[this.cameraIndex]);
    this.videoProfiles = this.cameraOutputCapability.videoProfiles;
    this.cameraCount = this.cameras.length;
    console.info(this.tag + '[camera] case cameras.length is: ' + this.cameraCount);
    console.info(this.tag + 'init camera done');
  }

  // 构建相机出流
  async createCameraInput() {
    console.info(this.tag + 'createCameraInput called');
    this.cameraInput = await this.cameraManager.createCameraInput(this.cameras[this.cameraIndex]);
    if (this.cameraInput) {
      console.info(this.tag + '[camera] case createCameraInput success');
    } else {
      console.info(this.tag + '[camera] case createCameraInput failed');
      return;
    }
    await this.cameraInput.open((err) => {
      if(err){
        console.info(this.tag + '[camera] cameraInput open Failed');
        return
      }
      console.info(this.tag + '[camera] cameraInput open success');
    })
  }

  // 创建相机预览输出
  async createPreviewOutput() {
    console.info(this.tag + 'createPreviewOutput called');
    this.previewProfiles = this.cameraOutputCapability.previewProfiles;
    this.previewProfiles[0].size.height = this.videoResolution['frameHeight'];
    this.previewProfiles[0].size.width = this.videoResolution['frameWidth'];
    if (this.previewProfiles[0].format == camera.CameraFormat.CAMERA_FORMAT_YUV_420_SP) {
      console.info(this.tag + '[camera] case format is VIDEO_SOURCE_TYPE_SURFACE_YUV');
      this.avConfig.videoSourceType = media.VideoSourceType.VIDEO_SOURCE_TYPE_SURFACE_YUV;
    } else {
      console.info(this.tag + '[camera] case format is VIDEO_SOURCE_TYPE_SURFACE_ES');
      this.avConfig.videoSourceType = media.VideoSourceType.VIDEO_SOURCE_TYPE_SURFACE_ES;
    }
    this.previewOutput = await this.cameraManager.createPreviewOutput(this.previewProfiles[0], this.surfaceId);
    if (!this.previewOutput) {
      console.info(this.tag + 'create previewOutput failed!');
    }
    console.info(this.tag + 'createPreviewOutput done');
  }

  // 创建拍照会话类
  async createCaptureSession() {
    console.info(this.tag + 'createCaptureSession called');
    this.captureSession = await this.cameraManager.createCaptureSession();
    if (!this.captureSession) {
      console.info(this.tag + 'createCaptureSession failed!');
      return
    }
    await this.captureSession.beginConfig();
    await this.captureSession.addInput(this.cameraInput);
    await this.captureSession.addOutput(this.previewOutput);
    await this.captureSession.commitConfig();
  }

  // 启动CaptureSession
  async startCaptureSession() {
    console.info(this.tag + 'startCaptureSession called');
    if (!this.captureSession) {
      console.info(this.tag + 'CaptureSession does not exists!');
      return
    }
    await this.captureSession.start();
    console.info(this.tag + 'startCaptureSession done');
  }

  // 从CaptureSession移除上一个videoOutput
  async removeVideoOutput() {
    console.info(this.tag + 'removeVideoOutput called');
    if (this.videoOutput) {
      console.info(this.tag + 'videoOutput already exits, remove it');
      await this.captureSession.removeOutput(this.videoOutput);
      console.info(this.tag + 'remove videoOutput done');
    }
    console.info(this.tag + 'removeVideoOutput done');
  }

  // 创建videoOutput
  async createVideoOutput() {
    console.info(this.tag + 'createVideoOutput called');
    if (this.videoRecorder) {
      this.videoProfiles[this.cameraIndex].size.height = this.videoResolution['frameHeight'];
      this.videoProfiles[this.cameraIndex].size.width = this.videoResolution['frameWidth'];
      this.videoOutput = await this.cameraManager.createVideoOutput(this.videoProfiles[this.cameraIndex], this.videoOutSurfaceId);
      if (!this.videoOutput) {
        console.info('[camera] case create videoOutPut fail!!!');
        return;
      } else{
        console.info('[camera] case create videoOutPut done, videoOutPut is not null');
      }
    } else {
      console.info(this.tag + 'videoRecorder is null, createVideoOutput failed');
      return;
    }
  }

  // 将videoOutput加入CaptureSession，启动videoOutput
  async startVideoOutput() {
    console.info(this.tag + 'startVideoOutput called');
    await this.captureSession.stop();
    await this.captureSession.beginConfig();
    await this.removeVideoOutput();
    await this.createVideoOutput();
    await this.captureSession.addOutput(this.videoOutput);
    await this.captureSession.commitConfig();
    await this.startCaptureSession();
    if (this.videoOutput) {
      await this.videoOutput.start(async(err) => {
        if (err) {
          console.error(this.tag + 'start videoOutput failed, err is : ' + err);
          return
        }
        console.info(this.tag + 'start videoOutput success');
      })
    } else {
      console.info(this.tag + 'videoOutput is null!!!');
      return;
    }
  }

  // 停止videoOutput
  async stopVideoOutput() {
    console.info(this.tag + 'stopVideoOutput called');
    if (this.videoOutput) {
      await this.videoOutput.stop(async(err) => {
        if (err) {
          console.error(this.tag + 'stop videoOutput failed');
          return
        }
        console.info(this.tag + 'stop videoOutput success');
      })
    } else {
      console.info(this.tag + 'videoOutput is null!!!');
      return;
    }
  }

  // 释放相机输入流
  async releaseCameraInput() {
    console.info(this.tag + 'releaseCameraInput called');
    if (this.cameraInput) {
      await this.cameraInput.release();
      this.cameraInput = null;
    }
    console.info(this.tag + 'releaseCameraInput done');
  }

  // 释放预览流
  async releasePreviewOutput() {
    console.info(this.tag + 'releasePreviewOutput called');
    if (this.previewOutput) {
      await this.previewOutput.release();
      this.previewOutput = null;
    }
    console.info(this.tag + 'releasePreviewOutput done');
  }

  // 释放录制输出对象
  async releaseVideoOutput() {
    console.info(this.tag + 'releaseVideoOutput called');
    if (this.videoOutput) {
      await this.videoOutput.release();
      this.videoOutput = null;
    }
    console.info(this.tag + 'releaseVideoOutput done');
  }

  // 释放相机拍照会话
  async stopCaptureSession() {
    console.info(this.tag + 'stopCaptureSession called');
    if (this.captureSession) {
      await this.captureSession.stop();
    }
    console.info(this.tag + 'stopCaptureSession done');
  }

  // 释放相机拍照会话
  async releaseCaptureSession() {
    console.info(this.tag + 'releaseCaptureSession called');
    if (this.captureSession) {
      await this.captureSession.release();
      this.captureSession = null;
    }
    console.info(this.tag + 'releaseCaptureSession done');
  }

  // 清理工作，包含释放拍照会话实例、相机输入流、预览流、录像输出流、avRecorder实例
  async releaseCamera() {
    console.info(this.tag + 'releaseCamera called');
    await this.stopCaptureSession();
    await this.releaseVideoRecorder();
    await this.releaseCameraInput();
    await this.releasePreviewOutput();
    await this.releaseVideoOutput();
    await this.releaseCaptureSession();
    console.info(this.tag + 'releaseCamera done');
  }

  // 设置回调，获取状态变化
  setCallback() {
    console.info('case callback');
    this.videoRecorder.on('stateChange', (state, reason) => {
      console.info('case state has changed, new state is :' + state + ',and new reason is : ' + reason);
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
          console.info("case error state!!!");
          this.recorderState = 'error';
          break;
        default:
          console.info('case start is unknown');
      }
    });
    this.videoRecorder.on('error', (err) => {
      console.info('case avRecorder.on(error) called, errMessage is ' + err.message);
    });
  }

  async createVideoRecorder() {
    await media.createAVRecorder().then((recorder) => {
      console.info(this.tag + 'case createAVRecorder called');
      if (typeof (recorder) != 'undefined') {
        this.videoRecorder = recorder;
      } else {
        console.info('case create avRecorder failed!!!');
        return
      }
    }, this.failureCallback).catch(this.catchCallback);
    this.setCallback();
  }

  async prepareVideoRecorder() {
    console.info(this.tag + 'case prepareVideoRecorder in');
    await this.videoRecorder.prepare(this.avConfig).then(() => {
      console.info(this.tag + 'case prepare AVRecorder called');
    }, this.failureCallback).catch(this.catchCallback);
    console.info(this.tag + 'case prepareVideoRecorder out');
  }

  async getInputSurface() {
    console.info(this.tag + 'case getInputSurface in');
    await this.videoRecorder.getInputSurface().then((outPutSurface) => {
      console.info('case getInputSurface called');
      if (!outPutSurface) {
        console.error(this.tag + 'case getInputSurface ID is none');
        return
      }
      this.videoOutSurfaceId = outPutSurface;
      console.info('case outPutSurface surfaceID is: ' + this.videoOutSurfaceId);
    }, this.failureCallback).catch(this.catchCallback);
    console.info(this.tag + 'case getInputSurface out');
  }

  async startVideoRecording() {
    console.info(this.tag + 'case startVideoRecording called');
    await this.videoRecorder.start().then(() => {
      console.info(this.tag + 'case start AVRecorder called');
    }, this.failureCallback).catch(this.catchCallback);
  }

  async pauseVideoRecording() {
    console.info(this.tag + 'case pauseVideoRecording called');
    await this.videoRecorder.pause().then(() => {
      console.info(this.tag + 'case pause AVRecorder called');
    }, this.failureCallback).catch(this.catchCallback);
  }

  async resumeVideoRecording() {
    console.info(this.tag + 'case resumeVideoRecording called');
    await this.videoRecorder.resume().then(() => {
      console.info(this.tag + 'case resume AVRecorder called');
    }, this.failureCallback).catch(this.catchCallback);
  }

  async stopVideoRecording() {
    console.info(this.tag + 'case stopVideoRecording called');
    await this.videoRecorder.stop().then(() => {
      console.info(this.tag + 'case stop AVRecorder called');
    }, this.failureCallback).catch(this.catchCallback);
  }

  async resetVideoRecording() {
    await this.videoRecorder.reset().then(() => {
      console.info(this.tag + 'case resetVideoRecording called');
    }, this.failureCallback).catch(this.catchCallback);
  }

  async releaseVideoRecorder() {
    if (this.videoRecorder) {
      await this.videoRecorder.release().then(() => {
        console.info(this.tag + 'case releaseVideoRecorder called');
      }, this.failureCallback).catch(this.catchCallback);
      this.videoRecorder = undefined;
    }
  }

  async dialog() {
    // 相机录制流参数设置，暂未实现，参数默认
  }

  async switchCamera () {
    // 相机翻转，暂未实现，rk不支持相机反转
  }

  // 关闭录制文件fd
  async closeFd() {
    console.info(this.tag + 'case closeFd called');
    if (this.fdPath) {
      await this.mSaveCameraAsset.VideoPrepareFile.close(this.mFileAssetId);
      this.mFileAssetId = undefined;
      this.fdPath = undefined;
      console.info(this.tag + 'case closeFd done');
    }
  }

  // 初始化
  async enterInit() {
    console.info(this.tag + 'enterInit called');
    await this.initCamera(); // 获取设备相机信息
    await this.createCameraInput(); // 打开相机
    await this.createPreviewOutput(); // 预览流
    await this.createCaptureSession(); // 相机会话，保存一次相机运行需要的所有资源，并向相机设备社情完成相机功能
    await this.startCaptureSession();
    console.info(this.tag + 'exit enterInit');
  }

  // 开始录制button
  async startRecordingProcess() {
    console.info(this.tag + 'startRecording called');
    if (this.videoRecorder) {
      console.info(this.tag + 'videoRecorder exist,release it');
      await this.videoRecorder.release();
    }
    await this.createVideoRecorder();
    await this.getFileFd();
    await this.prepareVideoRecorder();
    await this.getInputSurface();
    await this.startVideoOutput();
    await this.startVideoRecording();
    console.info(this.tag + 'startRecording done');
  }

  // 暂停录制button
  async pauseRecordingProcess() {
    console.info(this.tag + 'pauseRecording called');
    if (this.recorderState == 'started') {
      console.info(this.tag + 'current state is started, to pause');
      this.videoRecorder.pause();
    }
    console.info(this.tag + 'pauseRecording done');
  }

  // 恢复录制button
  async resumeRecordingProcess() {
    console.info(this.tag + 'resumeRecording called');
    if (this.recorderState == 'paused') {
      console.info(this.tag + 'current state is paused, to resume');
      this.videoRecorder.resume();
    }
    console.info(this.tag + 'resumeRecording done');
  }

  // 停止录制button
  async stopRecordingProcess() {
    console.info(this.tag + 'stopRecording called');
    if (this.recorderState == 'started' || this.recorderState == 'paused' ) {
      await this.stopVideoRecording();
    }
    await this.resetVideoRecording();
    await this.releaseVideoRecorder();
    await this.stopVideoOutput();
    await this.closeFd();
    clearInterval(this.timer);
    this.isStartRecording = false;
    this.isPauseRecording = false;
    this.ms = 0;
    this.videoRecorderTimeText = '00:00:00,000';
    console.info(this.tag + 'stopRecording done');
  }

  // 应用页面布局
  build(){
    Column() {
      Row() {
        Stack({alignContent: Alignment.Bottom}) {
          XComponent({
            id: 'xcomponent1',
            type: 'surface',
            controller: this.xcomponentController
          })
            .onLoad(() => {
              this.xcomponentController.setXComponentSurfaceSize({ surfaceWidth: 1920, surfaceHeight: 1080 });
              this.surfaceId = this.xcomponentController.getXComponentSurfaceId();
              console.info(this.tag + '[VideoRecorder] surfaceId: ' + this.surfaceId);
              console.info(this.tag + 'XComponent1 loaded ');
              this.enterInit();
            })
            .width('100%')
            .height('100%')
          Text(this.videoRecorderTimeText).fontSize(24).fontColor('white').margin({bottom: 700})
          Row({space:100}) {
            if (!this.isStartRecording) {
              Image(this.settingBtn).width(35).height(35).margin({left: 10, bottom: 20})
                .onClick(() => this.dialog())
              Image(this.startRecordingBtn).width(65).height(65).margin({bottom: 10})
                .onClick(() => this.startRecordingProcess())
              Image(this.switchCameraBtn).width(50).height(50).margin({bottom: 10})
                .onClick(() => this.switchCamera())
            } else {
              Image(this.stopRecordingBtn).width(50).height(50).margin({ bottom: 10 })
                .onClick(() => this.stopRecordingProcess())
              if (!this.isPauseRecording) {
                Image(this.pauseRecordingBtn).width(50).height(50).margin({bottom: 20})
                  .onClick(() => this.pauseRecordingProcess())
              } else {
                Image(this.resumeRecordingBtn).width(50).height(50).margin({bottom: 10})
                  .onClick(() => this.resumeRecordingProcess())
              }
              Image(this.switchCameraBtn).width(50).height(50).margin({bottom: 10})
                .onClick(() => this.switchCamera())
            }
          }
        }
      }
    }
  }
}
