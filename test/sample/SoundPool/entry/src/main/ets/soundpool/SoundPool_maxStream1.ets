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
import audio from '@ohos.multimedia.audio';
import fs from '@ohos.file.fs'
import featureAbility from '@ohos.ability.featureAbility';

@Entry
@Component
struct soundPool {
  @State returnMsg: string = ``;
  @State loadIndex: number = 0;
  @State playIndex: number = 0;
  @State selectedOffset: number = 0;
  @State leftVolume: number = 0.5;
  @State rightVolume: number = 0.5;
  @State minOffset: number = 0;
  @State maxOffset: number = 1;
  @State fileSize: number = 1;
  @State loop: number = 0;
  public uri1: string = "";
  public uri2: string = "";
  public FdList = [];
  public fd = -1;
  public FdSelectList: string[] = ['/test_01.mp3', '/test_02.mp3', '/test_03.mp3', '/test_04.mp3']
  @State selectedFdKey: string = "请选择"
  public selectedFd: string = String('test_01.mp3')

  public priorityList = []
  public prioritySelectList: number[] = [0, 1, 2, 3, 4, 5, 6]
  @State selectedPriorityKey: string = "请选择"
  public selectedPriority: number = 0

  public audioRendererRateList = []
  @State selectedAudioRendererRateKey: string = "请选择"
  public selectedAudioRendererRate: number = 0

  @State soundIdList: any[] = []
  @State soundIdSelectList: number[] = [];
  @State selectedSoundIdKey: string = "请选择"
  public selectedSoundId: number = this.soundIdSelectList[0];
  @State streamIdList: any[] = []
  @State streamIdSelectList: number[] = [];
  @State selectedStreamIdKey: string = "请选择"
  public selectedStreamId: number = this.streamIdSelectList[0];

  async aboutToAppear() {
    for (var i = 0; i < this.FdSelectList.length; i++) {
      var key = this.FdSelectList[i].toString();
      this.FdList.push({ value: key })
    }
    for (var i = 0; i < this.prioritySelectList.length; i++) {
      var key = this.prioritySelectList[i].toString();
      this.priorityList.push({ value: key });
    }
    for (let key in audio.AudioRendererRate) {
      this.audioRendererRateList.push({ value: key })
    }
    console.info('createSoundPool in');
    this.soundPool = await media.createSoundPool(1, this.audioRendererInfo);
    console.info('createSoundPool out');
    this.setSourceInfo();
  }

  async aboutToDisappear() {
    console.info('aboutToDisappear in');
    this.setOffCallback();
    for (var i = 0; i < this.soundIdSelectList.length; i++) {
      await this.soundPool.unload(this.soundIdSelectList[i]);
    }
    this.loadIndex = 0;
    this.playIndex = 0;
    await this.soundPool.release();
    console.info('aboutToDisappear out');
  }

  private soundPool: media.SoundPool;
  private audioRendererInfo = {
    content: audio.ContentType.CONTENT_TYPE_SPEECH,
    usage: audio.StreamUsage.STREAM_USAGE_MEDIA,
    rendererFlags: 1
  }
  private PlayParameters = {
    loop: this.loop,
    rate: audio.AudioRendererRate.RENDER_RATE_DOUBLE,
    leftVolume: 0.5,
    rightVolume: 0.5,
    priority: 0,
    parallelPlayFlag: true
  }

  async getFileUrl() {
    console.info('getFileUrl called');
    const context = getContext(this);
    const filePath = context.filesDir;
    console.info('filePath: '+ filePath);
    await fs.open(filePath + '/test_01.mp3', fs.OpenMode.READ_ONLY).then((file) => {
      console.info("file fd: " + file.fd);
      this.uri1 = 'fd://' + (file.fd).toString()
    }).catch((err) => {
      console.info("open file failed with error message: " + err.message + ", error code: " + err.code);
    });
    await fs.open(filePath + '/test_02.mp3', fs.OpenMode.READ_ONLY).then((file_) => {
      console.info("file fd: " + file_.fd);
      this.uri2 = 'fd://' + (file_.fd).toString()
    }).catch((err) => {
      console.info("open file failed with error message: " + err.message + ", error code: " + err.code);
    });
    console.info('uri1 is: ' + this.uri1);
    console.info('uri2 is: ' + this.uri2);
    console.info('getFileUrl done');
  }

  async loadSoundPool(uri) {
    console.info('loadSoundPool in');
    this.soundIdSelectList[this.loadIndex] = await this.soundPool.load(uri);
    var key = this.soundIdSelectList[this.loadIndex].toString();
    this.soundIdList.push({ value: key })
    this.loadIndex++;
    console.info('loadSoundPool out');
  }

  async loadFdSoundPool(fd, offset, size) {
    console.info('loadSoundPool in');
    this.soundIdSelectList[this.loadIndex] = await this.soundPool.load(fd, offset, size);
    var key = this.soundIdSelectList[this.loadIndex].toString();
    this.soundIdList.push({ value: key })
    this.loadIndex++;
    console.info('loadSoundPool out');
  }

  async playWithoutParam(soundId) {
    console.info('playWithoutParam in');
    this.streamIdSelectList[this.playIndex] = await this.soundPool.play(soundId);
    var key = this.streamIdSelectList[this.playIndex].toString();
    this.streamIdList.push({ value: key })
    this.playIndex++;
    console.info('playWithoutParam out');
  }

  async playWithParam(soundId) {
    console.info('playWithParam in');
    this.streamIdSelectList[this.playIndex] = await this.soundPool.play(soundId, this.PlayParameters);
    console.info(`streamId: ${this.streamIdSelectList[this.playIndex]}`);
    var key = this.streamIdSelectList[this.playIndex].toString();
    this.streamIdList.push({ value: key })
    this.playIndex++;
    console.info('playWithParam out');
  }

  setOffCallback() {
    this.soundPool.off('loadComplete');
    this.soundPool.off('playFinished');
    this.soundPool.off('error');
  }

  async setSourceInfo() {
    this.soundPool.on('loadComplete', (soundId) => {
      console.info('on load completed, soundId: ' + soundId);
    });
    this.soundPool.on('playFinished', () => {
      console.info('on play finished');
    });
    this.soundPool.on('error', (error) => {
      console.error('error happened,message is :' + error.message);
    })
  }

  async getOffsetRange(path) {
    const context = getContext(this);
    const filePath = context.filesDir;
    console.info('filePath: '+ filePath);
    fs.open(filePath + path, fs.OpenMode.READ_ONLY).then((file) => {
      console.info("file fd: " + file.fd);
      this.fd = file.fd;
    }).catch((err) => {
      console.info("open file failed with error message: " + err.message + ", error code: " + err.code);
    });
    let stat = await fs.stat(filePath + path);
    console.info(`stat: ${JSON.stringify(stat)}\n`);
    console.info(`size: ${stat.size}\n`);
    this.fileSize = stat.size;
    this.maxOffset = stat.size;
    this.returnMsg = "maxOffset:" + this.maxOffset;
  }

  build() {
    Column() {
      Row() {
        Column() {
          Text("返回数据：").fontWeight(FontWeight.Bolder).position({ x: 10, y: 5 }).fontSize(18)
          Text(this.returnMsg).position({ x: 10, y: 30 }).fontSize(14)
        }.width('98%').height(150).backgroundColor(Color.Orange).position({ x: '1%' })
      }.position({ x: 0, y: 0 }).width('100%').zIndex(999)

      Scroll() {
        Column() {
          Row() {
            Text('从指定路径加载audio').fontSize(22)
          }.margin({ left: 30 }).width('49%').justifyContent(FlexAlign.SpaceBetween)

          Row() {
            Button() {
              Text("load (uri 1)").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(async () => {
              await this.getFileUrl()
              this.loadSoundPool(this.uri1)
            })

            Button() {
              Text("load (uri 2)").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(async () => {
              await this.getFileUrl()
              this.loadSoundPool(this.uri2)
            })
          }.margin({ top: 5, bottom: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

          Divider().strokeWidth(10).color(Color.Blue)

          Row() {
            Text('从文件描述符加载audio').fontSize(22)
          }.margin({ top: 10, left: 20 }).width('49%').justifyContent(FlexAlign.SpaceBetween)

          Row() {
            Text("fd选择:").fontSize(20)
            Select(this.FdList).value(this.selectedFdKey)
              .onSelect((index, value) => {
                this.selectedFdKey = value
                console.log(`selectedFdKey: ${value}`)
                this.selectedFd = this.FdSelectList[index]
                console.log(`selectedFd: ${this.FdSelectList[index]}`)
                this.getOffsetRange(this.FdSelectList[index]);
              }).font({ size: 20 })
          }.margin({ bottom: 10 }).width('80%').justifyContent(FlexAlign.SpaceBetween)

          if (this.selectedFdKey !== "请选择") {
            Divider().strokeWidth(1).color(Color.Blue).margin({ bottom: 10 })
            Row() {
              Text("offset").fontSize(20)
              Slider({
                value: this.selectedOffset,
                min: this.minOffset,
                max: this.maxOffset - 100,
                step: 1,
                style: SliderStyle.InSet
              })
                .blockColor('#191970')
                .trackColor('#ADD8E6')
                .selectedColor('#4169E1')
                .showTips(true)
                .onChange((value: number, mode: SliderChangeMode) => {
                  this.selectedOffset = value;
                  console.info('value:' + value + '; mode:' + mode.toString());
                })
                .width('60%')
              Text(this.selectedOffset.toFixed(0)).fontSize(20)
            }
          }

          Row() {
            Button() {
              Text("load").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(() => {
              if (this.fd == -1) {
                this.returnMsg = "请选择fd"
                return
              }
              this.loadFdSoundPool(this.fd, this.selectedOffset, this.maxOffset)
            })
          }.margin({ bottom: 10, left: 260 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

          Divider().strokeWidth(10).color(Color.Blue)

          Row() {
            Text('音频池播放功能').fontSize(22)
          }.margin({ top: 10, left: 80 }).width('49%').justifyContent(FlexAlign.SpaceBetween)

          if (this.loadIndex !== 0) {
            Row() {
              Text("soundId:").fontSize(20)
              Select(this.soundIdList).value(this.selectedSoundIdKey)
                .onSelect((index, value) => {
                  this.selectedSoundIdKey = value
                  console.log(`selectedSoundId: ${value}`)
                  this.selectedSoundId = this.soundIdSelectList[index]
                  console.log(`soundId: ${this.soundIdSelectList[index]}`)
                })
            }.margin({ bottom: 5 })
          }

          Row() {
            Button() {
              Text("play").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(() => this.playWithoutParam(this.selectedSoundId))

            Button() {
              Text("play (PlayParameters)").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(() => this.playWithParam(this.selectedSoundId))
          }.margin({ top: 5, bottom: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

          if (this.playIndex !== 0) {
            Row() {
              Text("streamId:").fontSize(20)
              Select(this.streamIdList).value(this.selectedStreamIdKey)
                .onSelect((index, value) => {
                  this.selectedStreamIdKey = value
                  console.log(`selectedStreamId: ${value}`)
                  this.selectedStreamId = this.streamIdSelectList[index]
                  console.log(`streamId: ${this.streamIdSelectList[index]}`)
                })
            }.margin({ bottom: 5 })
          }

          Row() {
            Button() {
              Text("stop").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(() => this.soundPool.stop(this.selectedStreamId))
          }.margin({ bottom: 10, left: 260 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

          Divider().strokeWidth(10).color(Color.Blue)

          Row() {
            Text('音频池增强功能').fontSize(22)
          }.margin({ top: 10, left: 80 }).width('49%').justifyContent(FlexAlign.SpaceBetween)

          if (this.playIndex !== 0) {
            Row() {
              Text("streamId:").fontSize(20)
              Select(this.streamIdList).value(this.selectedStreamIdKey)
                .onSelect((index, value) => {
                  this.selectedStreamIdKey = value
                  console.log(`selectedStreamId: ${value}`)
                  this.selectedStreamId = this.streamIdSelectList[index]
                  console.log(`streamId: ${this.streamIdSelectList[index]}`)
                })
            }.margin({ bottom: 5 })
          }

          Row() {
            Text("loop:").fontSize(20)
            Radio({ value: 'loop0', group: "loopGroup" }).onChange((isChecked) => {
              if (isChecked) {
                this.loop = 2;
              } else {
                this.loop = 0;
              }
            }).checked(this.loop == 2)
            Text("2").fontSize(20)

            Radio({ value: 'loop1', group: "loopGroup" }).onChange((isChecked) => {
              if (isChecked) {
                this.loop = 0;
              } else {
                this.loop = -1;
              }
            }).checked(this.loop == 0)
            Text("0").fontSize(20)

            Radio({ value: 'loop2', group: "loopGroup" }).onChange((isChecked) => {
              if (isChecked) {
                this.loop = -2;
              } else {
                this.loop = 0;
              }
            }).checked(this.loop == -2)
            Text("-2").fontSize(20)
          }

          Row() {
            Text("循环数据：" + this.loop).fontSize(20)
          }

          Row() {
            Text("priority:").fontSize(20)
            Select(this.priorityList).value(this.selectedPriorityKey)
              .onSelect((index, value) => {
                this.selectedPriorityKey = value
                console.log(`selectedPriority: ${value}`)
                this.selectedPriority = this.prioritySelectList[index]
                console.log(`priority: ${this.prioritySelectList[index]}`)
              })
          }.margin({ top: 10 })

          Row() {
            Text("rate:").fontSize(20)
            Select(this.audioRendererRateList).value(this.selectedAudioRendererRateKey)
              .onSelect((index, value) => {
                this.selectedAudioRendererRateKey = value
                console.log(`selectedAudioRendererRate: ${value}`)
                this.selectedAudioRendererRate = audio.AudioRendererRate[value]
                console.log(`rate: ${audio.AudioRendererRate[value]}`)
              })
          }.margin({ top: 10 })

          Row() {
            Text("leftVolume").fontSize(20)
            Slider({
              value: this.leftVolume,
              min: -0.5,
              max: 1.5,
              step: 0.1,
              style: SliderStyle.InSet
            })
              .blockColor('#191970')
              .trackColor('#ADD8E6')
              .selectedColor('#4169E1')
              .showTips(true)
              .onChange((value: number, mode: SliderChangeMode) => {
                this.leftVolume = value;
                console.info('value:' + value + '; mode:' + mode.toString());
              })
              .width('60%')
            Text(this.leftVolume.toFixed(1)).fontSize(20)
          }

          Row() {
            Text("rightVolume").fontSize(20)
            Slider({
              value: this.rightVolume,
              min: -0.5,
              max: 1.5,
              step: 0.1,
              style: SliderStyle.InSet
            })
              .blockColor('#191970')
              .trackColor('#ADD8E6')
              .selectedColor('#4169E1')
              .showTips(true)
              .onChange((value: number, mode: SliderChangeMode) => {
                this.rightVolume = value;
                console.info('value:' + value + '; mode:' + mode.toString());
              })
              .width('60%')
            Text(this.rightVolume.toFixed(1)).fontSize(20)
          }

          Row() {
            Button() {
              Text("setLoop").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(() => this.soundPool.setLoop(this.selectedStreamId, this.loop))

            Button() {
              Text("setPriority").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(() => this.soundPool.setPriority(this.selectedStreamId, this.selectedPriority))
          }.margin({ bottom: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

          Row() {
            Button() {
              Text("setRate").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(() => this.soundPool.setRate(this.selectedStreamId, this.selectedAudioRendererRate))

            Button() {
              Text("setVolume").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(() => this.soundPool.setVolume(this.selectedStreamId, this.leftVolume, this.rightVolume))
          }.margin({ bottom: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)
        }.width('100%')
      }.margin({ top: 160 }).width('100%')
    }
  }
}