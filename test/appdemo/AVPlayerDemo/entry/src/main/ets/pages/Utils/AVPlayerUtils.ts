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
import media from '@ohos.multimedia.media';

const TAG:string = 'MediaDemo AVPlayerUtils:'
const IDLE_STATE: string = 'idle';
const INIT_STATE: string = 'initialized';
const PREPARE_STATE: string = 'prepared';
const PLAY_STATE: string = 'playing';
const PAUSE_STATE: string = 'paused';
const COMPLETE_STATE: string = 'completed';
const STOP_STATE: string = 'stopped';
const RELEASE_STATE: string = 'released';
const ERROR_STATE: string = 'error';
const MEDIA_OK: number = 0;
const MEDIA_FAIL: number = -1;

export default class AVPlayerUtils{
    // avPlayer 实例对象
    public avPlayer = undefined
    // 显示画面ID
    private surfaceID = ''
    // 播放地址 fd://xxx http:// https://
    private playPath = ''
    private isNext = false
    private finishCallback
    // 初始化创建AVPlayer 实例对象 失败返回false
    async initAVPlayer(playPath) : Promise<boolean> {
        console.info(TAG + 'initAVPlayer start')
        let ret = false
        if (this.avPlayer != undefined) {
            await this.release()
            this.avPlayer = undefined
        }
        this.avPlayer = await media.createAVPlayer()
        if (this.avPlayer != undefined) {
            ret = true
        }
        this.avPlayer.url = playPath
        console.info(TAG + 'initAVPlayer end')
        return ret
    }

    async setSurface(surfaceID) {
        console.info(TAG + 'setSurface start')
        this.avPlayer.surfaceId = surfaceID
        this.surfaceID = surfaceID
        console.info(TAG + 'setSurface end')
    }
    async setFinishCallback(callback) {
        this.finishCallback = callback
    }


    async setLoop(loopValue: boolean) {
        this.avPlayer.loop = loopValue
    }

    // 播放初始化操作
    async prepare() : Promise<void>  {
        console.info(TAG + 'prepare start')
        await this.avPlayer.prepare().then(() => {
            console.info(TAG+ 'prepare success');
        }, (err) => {
            console.error(TAG + 'prepare filed,error message is :' + err.message)
        })
        console.info(TAG + 'prepare end')
    }
    // 播放操作
    play(): number {
        let state: string = this.getState()
        console.info(TAG+ 'play start, state is :' + state);
        if ((state == PREPARE_STATE) || (state == PAUSE_STATE) ||
            (state == COMPLETE_STATE)) {
            console.info(TAG+ 'play start 2, state is :' + state);
            this.avPlayer.play()
            return MEDIA_OK
        } else {
            console.info(TAG + 'this state:' + this.getState() + 'can not call play')
            return MEDIA_FAIL
        }
    }
    // 暂停操作
    async pause() {
        this.avPlayer.pause()
    }

    async reset() : Promise<void> {
        await this.avPlayer.reset().then(() => {
            console.info(TAG + 'reset success')
        }, (err) => {
            console.info(TAG + 'reset failed,error message is :' + err.message)
        })
    }

    async release() : Promise<void>  {
        await this.avPlayer.release().then(() => {
            console.info(TAG + 'release success')
        }, (err) => {
            console.info(TAG + 'release failed,error message is :' + err.message)
        })
        return
    }

    // 获取总时长操作
    async getDuration() : Promise<number> {
        return
    }
    // 获取当前时间操作
    getCurrentTime() : number {
        return this.avPlayer.currentTime
    }

    getState(): string {
        return this.avPlayer.state
    }

}