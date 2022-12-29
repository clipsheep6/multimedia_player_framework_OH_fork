/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

import media from '@ohos.multimedia.media'
import * as mediaTestBase from './MediaTestBase.js';

export const AV_PLAYER_STATE = {
    IDLE : 'idle',
    INITIALIZED : 'initialized',
    PREPARED : 'prepared',
    PLAYING : 'playing',
    PAUSED : 'paused',
    COMPLETED : 'completed',
    STOPPED : 'stopped',
    RELEASED : 'released',
    ERROR : 'error',
}

let playTest = {
    width: 0,
    height: 0,
    duration: -1
}
export {playTest};

export function setSource(avPlayer, src) {
    if (typeof(avPlayer) == 'undefined') {
        console.error('case avPlayer is undefined');
        return;
    }
    if (typeof(src) == 'string') {
        console.info('case src test');
        avPlayer.url = src;
    } else {
        console.info('case fdsrc test');
        avPlayer.fdSrc = src;
    }
}

function checkPlayTest(avPlayer, playTest) {
    if (avPlayer == null) {
        return;
    }
    expect(Math.abs(avPlayer.duration - playTest.duration)).assertLess(500);
    if (playTest.width > 0) {
        expect(avPlayer.width).assertEqual(playTest.width);
        expect(avPlayer.height).assertEqual(playTest.height);
    }
}

function toPreparePromise(avPlayer, playTest) {
    if (typeof(avPlayer) == 'undefined') {
        return;
    }
    avPlayer.prepare().then(() => {
        console.info('case prepare called');
        console.info('case avPlayer.duration: ' + avPlayer.duration);
        checkPlayTest(avPlayer, playTest);
        expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.PREPARED);
        expect(avPlayer.currentTime).assertEqual(0);
    }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
}

function addCnt(countArr) {
    if (countArr != null) {
        countArr[0]++;
    }
}

function setCallback(avPlayer, type, countArr) {
    if (avPlayer == null) {
        return;
    }
    switch (type) {
        case 'volumeChange':
            avPlayer.on('volumeChange', (volume) => {
                console.info(`case volumeChange called, volume is ${volume}`);
                addCnt(countArr);
            });
            break;
        case 'endOfStream':
            avPlayer.on('endOfStream', () => {
                console.info(`case endOfStream called`);
                addCnt(countArr);
            });
            break;
        case 'speedDone':
            avPlayer.on('speedDone', (speedMode) => {
                console.info(`case speedDone called, speedMode is ${speedMode}`);
                addCnt(countArr);
            });
            break;
        case 'bitrateDone':
            avPlayer.on('bitrateDone', (bitrate) => {   
                console.info(`case bitrateDone called, bitrate is ${bitrate}`);
            });
            break;
        case 'timeUpdate':
            avPlayer.on('timeUpdate', (time) => {
                console.info('case timeUpdate callback, time:' + time);
            });
            break;
        case 'bufferingUpdate':
            avPlayer.on('bufferingUpdate', (infoType, value) => {
            });
            break;
        case  'durationUpdate':
            avPlayer.on('durationUpdate', (duration) => {
                console.info('case durationUpdate called,duration:' + duration);
                addCnt(countArr);
            });
            break;
        case 'startRenderFrame':
            avPlayer.on('startRenderFrame', () => {
                console.info('case startRenderFrame called');
                addCnt(countArr);
            });
            break;
        case 'videoSizeChange':
            avPlayer.on('videoSizeChange', (w, h) => {
                console.info(`case videoSizeChange called, weight is ${w}, height is ${h}`);
                addCnt(countArr);
            });
            break;
        case  'audioInterrupt':
            avPlayer.on('audioInterrupt', (info) => {
                console.info(`case audioInterrupt called, info is ${info}`);
            });
            break;
        case  'availableBitrates':
            avPlayer.on('availableBitrates', (bitrates) => {
                for (let i = 0; i < bitrates.length; i++) { 
                    console.info('case availableBitrates : '  + bitrates[i]);
                }
                addCnt(countArr);
            });
            break;
        default:
            break;
    }
}

export function setAVPlayerFunCb(src, avPlayer, playTest, playTime, done) {
    let volumeCnt = [0];
    let endOfStreamCnt = [0];
    let speedDoneCnt = [0];
    let videoSizeCnt = [0];
    let startRenderFrameCnt = [0];
    let durationUpdateCnt = [0];
    let seekDoneCnt = [0];
    let prepareCnt = 0;
    let playCnt = 0;
    let completedCnt = 0;
    let surfaceID = globalThis.value;
    console.info(`case setAVPlayerFunCb in, surfaceID is ${surfaceID}`);
    avPlayer.on('stateChange', async (state, reason) => {
        console.info(`case stateChange called, state is ${state}, reason is ${reason}`);
        if (reason == media.StateChangeReason.BACKGROUND && state != AV_PLAYER_STATE.COMPLETED) {
            console.info(`case media.StateChangeReason.BACKGROUND`);
            await avPlayer.release().then(() => {
            }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
        }
        console.info(`case state is ${state}`);
        switch (state) {
            case AV_PLAYER_STATE.INITIALIZED:
                expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.INITIALIZED);
                avPlayer.surfaceId = surfaceID;
                // step 1, 13: initialized -> prepared
                toPreparePromise(avPlayer, playTest);
                break;
            case AV_PLAYER_STATE.PREPARED:
                prepareCnt++;
                expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.PREPARED);
                checkPlayTest(avPlayer, playTest);
                expect(avPlayer.currentTime).assertEqual(0);
                if (prepareCnt == 1) {
                    // step 2: prepared -> playing
                    avPlayer.play();
                } else {
                    // step 14: prepared -> seek
                    avPlayer.seek(avPlayer.duration);
                }
                break;
            case AV_PLAYER_STATE.PLAYING:
                playCnt++;
                if (playCnt == 1) {
                    expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.PLAYING);
                    // step 3: playing -> seek duration/3
                    await mediaTestBase.msleepAsync(playTime);
                    avPlayer.seek(avPlayer.duration / 3);
                } else if (playCnt == 2) {
                    expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.PLAYING);
                    //  step 7: playing -> seek duration when loop true
                    avPlayer.seek(avPlayer.duration, media.SeekMode.SEEK_NEXT_SYNC);
                } else if (playCnt == 3) {
                    // step 10: playing -> stop
                    avPlayer.stop();
                }
                break;
            case AV_PLAYER_STATE.PAUSED:
                expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.PAUSED);
                // step 5: pause -> seek 0
                avPlayer.loop = true;
                avPlayer.seek(0, media.SeekMode.SEEK_NEXT_SYNC);
                break;
            case AV_PLAYER_STATE.COMPLETED:
                completedCnt++;
                expect(avPlayer.currentTime).assertEqual(avPlayer.duration);
                expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.COMPLETED);
                if (completedCnt == 1) {
                    // step 9: completed -> play
                    avPlayer.play();
                } else {
                    // step 16: completed -> reset
                    avPlayer.reset().then(() => {
                        expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.IDLE);
                        // step 17: reset -> release
                        avPlayer.release().then(() => {
                        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                    }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                }
                break;
            case AV_PLAYER_STATE.STOPPED:
                expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.STOPPED);
                // step 11: stop -> reset
                avPlayer.reset().then(() => {
                    expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.IDLE);
                    // step 12: reset -> initialized
                    setSource(avPlayer, src);
                }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                break;
            case AV_PLAYER_STATE.RELEASED:
                expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.RELEASED);
                // step 18: release -> done
                avPlayer = null;
                expect(volumeCnt[0]).assertLarger(0);
                expect(endOfStreamCnt[0]).assertLarger(0);
                expect(seekDoneCnt[0]).assertLarger(0);
                expect(speedDoneCnt[0]).assertLarger(0);
                expect(completedCnt).assertLarger(0);
                if (playTest.width != 0) {
                    expect(startRenderFrameCnt[0]).assertLarger(0);
                    expect(videoSizeCnt[0]).assertLarger(0);
                } else {
                    expect(startRenderFrameCnt[0]).assertEqual(0);
                    expect(videoSizeCnt[0]).assertEqual(0);
                }
                expect(durationUpdateCnt[0]).assertLarger(0)
                done();
                break;
            case AV_PLAYER_STATE.ERROR:
                expect().assertFail();
                avPlayer.release().then(() => {
                }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                break;
            default:
                break; 
        }
    });
    avPlayer.on('seekDone', async (seekDoneTime) => {
        seekDoneCnt[0]++;
        console.info(`case seekDone called, seekDoneCnt is ${seekDoneCnt}, seekDoneTime is ${seekDoneTime}`);
        switch (seekDoneCnt[0]) {
            case 2:
                // step 6: seek(paused) -> play
                expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.PAUSED);
                avPlayer.play();
                avPlayer.setSpeed(media.PlaybackSpeed.SPEED_FORWARD_2_00_X);
                avPlayer.setVolume(0.5);
                break;
            case 1:
                expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.PLAYING);
                // step 4: seek(playing) -> pause
                avPlayer.pause();
                break;
            case 3:
            case 4:
            case 5:
                let nowTime = avPlayer.currentTime;
                if (avPlayer.state == AV_PLAYER_STATE.PREPARED) {
                    // step 15: prepared -> play
                    avPlayer.play();
                }
                if (nowTime > avPlayer.duration / 2) {
                    avPlayer.setSpeed(media.PlaybackSpeed.SPEED_FORWARD_1_00_X);
                    await mediaTestBase.msleepAsync(avPlayer.duration - nowTime + playTime);
                }
                if (avPlayer.loop == true) {
                    // step 8: playing -> seek duration when loop false
                    expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.PLAYING);
                    avPlayer.loop = false;
                    avPlayer.seek(avPlayer.duration, media.SeekMode.SEEK_NEXT_SYNC);
                }
                break;
        }
    });
    setCallback(avPlayer, 'volumeChange', volumeCnt);
    setCallback(avPlayer, 'endOfStream', endOfStreamCnt);
    setCallback(avPlayer, 'speedDone', speedDoneCnt);
    setCallback(avPlayer, 'bitrateDone', null);
    setCallback(avPlayer, 'timeUpdate', null);
    setCallback(avPlayer, 'bufferingUpdate', null);
    setCallback(avPlayer, 'durationUpdate', durationUpdateCnt);
    setCallback(avPlayer, 'startRenderFrame', startRenderFrameCnt);
    setCallback(avPlayer, 'videoSizeChange', videoSizeCnt);
    setCallback(avPlayer, 'audioInterrupt', null);
    setCallback(avPlayer, 'availableBitrates', null);
    avPlayer.on('error', async (err) => {
        console.error(`case error called, errMessage is ${err.message}`);
        expect().assertFail();
        await avPlayer.release().then(() => {
            avPlayer = null;
            done();
        });
    });
}

export async function testAVPlayerFun(src, avPlayer, playTest, playTime, done) {
    console.info(`case media source: ${src}`)    
    await media.createAVPlayer().then((video) => {
        if (typeof(video) != 'undefined') {
            console.info('case createAVPlayer success');
            avPlayer = video;
        } else {
            console.error('case createAVPlayer failed');
            expect().assertFail();
            done();
        }
    }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
    setAVPlayerFunCb(src, avPlayer, playTest, playTime, done);
    setSource(avPlayer, src);
}

export function setAVPlayerSeekCb(src, avPlayer, playTest, playTime, done) {
    let volumeCnt = [0];
    let endOfStreamCnt = 0;
    let seekDoneCnt = 0;
    let speedDoneCnt = [0];
    let playCnt = 0;
    let surfaceID = globalThis.value;
    console.info(`case setCallback in, surfaceID is ${surfaceID}`);
    avPlayer.on('stateChange', async (state, reason) => {
        console.info(`case stateChange called, state is ${state}, reason is ${reason}`);
        if (reason == media.StateChangeReason.BACKGROUND && state != AV_PLAYER_STATE.COMPLETED) {
            avPlayer.release().then(() => {
            }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
        }
        switch (state) {
            case AV_PLAYER_STATE.INITIALIZED:
                expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.INITIALIZED);
                avPlayer.surfaceId = surfaceID;
                console.info('case initialized -> prepared');
                // step 1: initialized -> prepared
                avPlayer.prepare((err) => {
                    avPlayer.loop = true;
                    if (err != null) {
                        console.error(`case prepare error, errMessage is ${err.message}`);
                        expect().assertFail();
                        done();
                    } else {
                        checkPlayTest(avPlayer, playTest);
                        expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.PREPARED);
                        expect(avPlayer.currentTime).assertEqual(0);
                    }
                });
                break;
            case AV_PLAYER_STATE.PREPARED:
                expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.PREPARED);
                checkPlayTest(avPlayer, playTest);
                expect(avPlayer.currentTime).assertEqual(0);
                // step 2: prepared -> seek 0
                avPlayer.seek(0, media.SeekMode.SEEK_CLOSEST_SYNC);
                break;
            case AV_PLAYER_STATE.PLAYING:
                playCnt++;
                if (playCnt == 1) {
                    // step 4: seek + pause
                    expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.PLAYING);
                    avPlayer.seek(avPlayer.duration / 2, media.SeekMode.SEEK_PREV_SYNC);
                    avPlayer.pause();
                }
                break;
            case AV_PLAYER_STATE.RELEASED:
                expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.RELEASED);
                // step 18: release -> done
                avPlayer = null;
                expect(volumeCnt[0]).assertLarger(0);
                expect(endOfStreamCnt).assertLarger(0);
                expect(speedDoneCnt[0]).assertLarger(0);
                done();
                break;
            case AV_PLAYER_STATE.ERROR:
                expect().assertFail();
                avPlayer.release().then(() => {
                }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                break;
            default:
                break; 
        }
    });

    avPlayer.on('endOfStream', () => {
        console.info(`case endOfStream called`);
        endOfStreamCnt++;
        // step 9: seek + reset
        avPlayer.seek(avPlayer.duration / 2, media.SeekMode.SEEK_CLOSEST);
        avPlayer.reset((err) => {
            if (err == null) {
                expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.IDLE);
                console.info('case reset success!!');
                avPlayer.release((err) => {
                    if (err != null) {
                        mediaTestBase.assertErr('release', err, done);
                    }
                })
            }  else {
                mediaTestBase.assertErr('reset', err, done);
            }
        });
    });
    avPlayer.on('seekDone', async (seekDoneTime) => {
        seekDoneCnt++;
        console.info(`case seekDone called, seekDoneCnt is ${seekDoneCnt}, seekDoneTime is ${seekDoneTime}`);
        switch (seekDoneCnt) {
            case 1:
                expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.PREPARED);
                // step 3: seek(prepared) -> play
                avPlayer.play();
                break;
            case 2:
                // step 5: seek + play
                avPlayer.seek(avPlayer.duration / 2, media.SeekMode.SEEK_NEXT_SYNC);
                avPlayer.play();
                break;
            case 3:
                // step 6: seek + setSpeed + setVolume
                avPlayer.setSpeed(media.PlaybackSpeed.SPEED_FORWARD_1_75_X);
                avPlayer.setVolume(0.5);
                avPlayer.seek(avPlayer.duration / 2, media.SeekMode.SEEK_CLOSEST_SYNC);
                avPlayer.setSpeed(media.PlaybackSpeed.SPEED_FORWARD_2_00_X);
                avPlayer.play();
                break;
            case 4:
                // step 7: seek + seek
                avPlayer.seek(avPlayer.duration / 2);
                avPlayer.seek(avPlayer.duration, media.SeekMode.SEEK_NEXT_SYNC);
                avPlayer.play();
                break;
            case 5:
                // step 8: seek duration
                avPlayer.seek(avPlayer.duration, media.SeekMode.SEEK_PREV_SYNC);
            default:
                break;
        }
    });
    setCallback(avPlayer, 'volumeChange', volumeCnt);
    setCallback(avPlayer, 'speedDone', speedDoneCnt);
    setCallback(avPlayer, 'bitrateDone', null);
    setCallback(avPlayer, 'timeUpdate', null);
    setCallback(avPlayer, 'bufferingUpdate', null);
    setCallback(avPlayer, 'durationUpdate', null);
    setCallback(avPlayer, 'startRenderFrame', null);
    setCallback(avPlayer, 'videoSizeChange', null);
    setCallback(avPlayer, 'audioInterrupt', null);
    setCallback(avPlayer, 'availableBitrates', null);
    avPlayer.on('error', async (err) => {
        console.error(`case error called, errMessage is ${err.message}`);
    });
}

export async function testAVPlayerSeek(src, avPlayer, playTest, playTime, done) {
    console.info(`case media source: ${src}`)
    media.createAVPlayer((err, video) => {
        console.info(`case media err: ${err}`)
        if (typeof(video) != 'undefined') {
            console.info('case createAVPlayer success');
            avPlayer = video;
            setAVPlayerSeekCb(src, avPlayer, playTest, playTime, done);
            setSource(avPlayer, src);
        }
        if (err != null) {
            console.error(`case createAVPlayer error, errMessage is ${err.message}`);
            expect().assertFail();
            done();
        }
    });
}

export function setAVPlayerTrackCb(avPlayer, descriptionKey, descriptionValue, done) {
    let arrayDescription;
    let surfaceID = globalThis.value;
    avPlayer.on('stateChange', async (state, reason) => {
        switch (state) {
            case AV_PLAYER_STATE.INITIALIZED:
                console.info(`case AV_PLAYER_STATE.INITIALIZED`);
                avPlayer.surfaceId = surfaceID;
                expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.INITIALIZED);
                avPlayer.prepare((err) => {
                    console.info('case prepare called' + err);
                    if (err != null) {
                        console.error(`case prepare error, errMessage is ${err.message}`);
                        expect().assertFail();
                        done();
                    } else {
                        //console.info('case avPlayer state is:' + avPlayer.state)
                        console.info('case avPlayer.duration: ' + avPlayer.duration);
                    }
                });
                break;
            case AV_PLAYER_STATE.PREPARED:
                await avPlayer.getTrackDescription().then((arrayList) => {
                    console.info('case getTrackDescription called!!');
                    if (typeof (arrayList) != 'undefined') {
                        arrayDescription = arrayList;
                    } else {
                        console.info('case getTrackDescription is failed');
                        expect().assertFail();
                    }
                }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                for (let i = 0; i < arrayDescription.length; i++) {
                    mediaTestBase.checkDescription(arrayDescription[i], descriptionKey[i], descriptionValue[i]);
                }
                avPlayer.getTrackDescription((error, arrayList) => {
                    if (error == null) {
                        for (let i = 0; i < arrayList.length; i++) {
                            mediaTestBase.checkDescription(arrayList[i], descriptionKey[i], descriptionValue[i]);
                        }
                    } else {
                        console.info('getTrackDescription failed, message is:' + error.message);
                    }
                    avPlayer.release();
                })
                
                break;
            case AV_PLAYER_STATE.RELEASED:
                done();
                break;
            case AV_PLAYER_STATE.ERROR:
                expect().assertFail();
                avPlayer.release().then(() => {
                }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                break;
            default:
                break; 
        }
    })

}

export async function testCheckTrackDescription(src, avPlayer, descriptionKey, descriptionValue, done) {
    console.info(`case media source: ${src}`)
    media.createAVPlayer((err, video) => {
        console.info(`case media err: ${err}`)
        if (typeof (video) != 'undefined') {
            console.info('case createAVPlayer success');
            avPlayer = video;
            setAVPlayerTrackCb(avPlayer, descriptionKey, descriptionValue, done)
            setSource(avPlayer, src);
        }
        if (err != null) {
            console.error(`case createAVPlayer error, errMessage is ${err.message}`);
            expect().assertFail();
            done();
        }
    });
}

export async function setAVPlayerScaleCb(avPlayer, done) {
    let surfaceID = globalThis.value;
    let count = 0;
    avPlayer.on('stateChange', async (state, reason) => {
        switch (state) {
            case AV_PLAYER_STATE.INITIALIZED:
                console.info(`case AV_PLAYER_STATE.INITIALIZED`);
                expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.INITIALIZED);
                avPlayer.surfaceId = surfaceID;
                avPlayer.prepare((err) => {
                    console.info('case prepare called' + err);
                    avPlayer.loop = true;
                    if (err != null) {
                        console.error(`case prepare error, errMessage is ${err.message}`);
                        expect().assertFail();
                        done();
                    } else {
                        console.info('case avPlayer.duration: ' + avPlayer.duration);
                    }
                });
                break;
            case AV_PLAYER_STATE.PREPARED:
                console.info('case prepare called');
                avPlayer.play();
                break;
            case AV_PLAYER_STATE.PLAYING:
                console.info('case playing called');
                for (let i = 0; i < 20; i++) {
                    if (count == 0) {
                        console.info('case set  videoScaleType : 1');
                        avPlayer.videoScaleType = media.VideoScaleType.VIDEO_SCALE_TYPE_FIT_CROP;
                        count = 1;
                    } else {
                        console.info('case set  videoScaleType : 0');
                        avPlayer.videoScaleType = media.VideoScaleType.VIDEO_SCALE_TYPE_FIT;
                        count = 0;
                    }
                    mediaTestBase.msleep(500);
                }
                avPlayer.loop = false;
                break;
            case AV_PLAYER_STATE.COMPLETED:
                expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.COMPLETED);
                expect(avPlayer.currentTime).assertEqual(avPlayer.duration);
                avPlayer.release();
                break;
            case AV_PLAYER_STATE.RELEASED:
                done();
                break;
            case AV_PLAYER_STATE.ERROR:
                expect().assertFail();
                avPlayer.release().then(() => {
                }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                break;
            default:
                break; 
        }
    })
}

export async function testVideoScaleType(src, avPlayer, done) {
    console.info(`case media source: ${src}`)
    media.createAVPlayer((err, video) => {
        console.info(`case media err: ${err}`)
        if (typeof (video) != 'undefined') {
            console.info('case createAVPlayer success');
            avPlayer = video;
            setAVPlayerScaleCb(avPlayer, done)
            setSource(avPlayer, src);
        }
        if (err != null) {
            console.error(`case createAVPlayer error, errMessage is ${err.message}`);
            expect().assertFail();
            done();
        }
    });
}

export async function testAudioInterruptMode(audioSource, videoSource, done) {
    let testAVPlayer01 = await media.createAVPlayer();
    let testAVPlayer02 = await media.createAVPlayer();
    let surfaceID = globalThis.value;
    testAVPlayer01.on('stateChange', async (state, reason) => {
        switch (state) {
            case AV_PLAYER_STATE.INITIALIZED:
                console.info(`case AV_PLAYER_STATE.INITIALIZED`);
                expect(testAVPlayer01.state).assertEqual(AV_PLAYER_STATE.INITIALIZED);
                testAVPlayer01.prepare((err) => {
                    console.info('case prepare called' + err);
                    if (err != null) {
                        console.error(`case prepare error, errMessage is ${err.message}`);
                        expect().assertFail();
                        done();
                    } else {
                        console.info('case avPlayer.duration: ' + testAVPlayer01.duration);
                    }
                });
                break;
            case AV_PLAYER_STATE.PREPARED:
                testAVPlayer01.audioInterruptMode = 1;
                testAVPlayer01.play();             
                break;
            case AV_PLAYER_STATE.PLAYING:
                testAVPlayer02.fdSrc = videoSource;   
                break;
            case AV_PLAYER_STATE.RELEASED:
                break;
            case AV_PLAYER_STATE.ERROR:
                expect().assertFail();
                testAVPlayer01.release().then(() => {
                }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                break;
            default:
                break; 
        }
    })

    testAVPlayer01.on('audioInterrupt', async (info) => {
        console.info('case audioInterrupt1 is called, info is :' + JSON.stringify(info));
        await testAVPlayer02.release();
        await testAVPlayer01.release().then(() => {
            console.info('case release called!!');
            done();
        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
    });

    testAVPlayer02.on('stateChange', async (state, reason) => {
        switch (state) {
            case AV_PLAYER_STATE.INITIALIZED:
                console.info(`case AV_PLAYER_STATE.INITIALIZED`);
                expect(testAVPlayer02.state).assertEqual(AV_PLAYER_STATE.INITIALIZED);
                testAVPlayer02.surfaceId = surfaceID;
                testAVPlayer02.prepare((err) => {
                    console.info('case prepare called' + err);
                    if (err != null) {
                        console.error(`case prepare error, errMessage is ${err.message}`);
                        expect().assertFail();
                        done();
                    } else {
                        console.info('case avPlayer.duration: ' + testAVPlayer02.duration);
                    }
                });
                break;
            case AV_PLAYER_STATE.PREPARED:
                testAVPlayer02.play();             
                break;
            case AV_PLAYER_STATE.PLAYING:         
                break;
            case AV_PLAYER_STATE.RELEASED:
                break;
            case AV_PLAYER_STATE.ERROR:
                expect().assertFail();
                testAVPlayer02.release().then(() => {
                }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                break;
            default:
                break; 
        }
    })
    testAVPlayer01.fdSrc = audioSource;
}

export async function testErrorAudioInterruptMode(videoSource, done) {
    let testAVPlayer = await media.createAVPlayer();
    let surfaceID = globalThis.value;
    testAVPlayer.on('stateChange', async (state, reason) => {
        switch (state) {
            case AV_PLAYER_STATE.INITIALIZED:
                console.info(`case AV_PLAYER_STATE.INITIALIZED`);
                expect(testAVPlayer.state).assertEqual(AV_PLAYER_STATE.INITIALIZED);
                testAVPlayer.surfaceId = surfaceID;
                testAVPlayer.prepare((err) => {
                    console.info('case prepare called' + err);
                    if (err != null) {
                        console.error(`case prepare error, errMessage is ${err.message}`);
                        expect().assertFail();
                        done();
                    } else {
                        console.info('case avPlayer.duration: ' + testAVPlayer.duration);
                    }
                });
                break;
            case AV_PLAYER_STATE.PREPARED:
                testAVPlayer.audioInterruptMode = -1;           
                break;
            case AV_PLAYER_STATE.RELEASED:
                done();
                break;
            case AV_PLAYER_STATE.ERROR:
                expect().assertFail();
                testAVPlayer.release().then(() => {
                }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                break;
            default:
                break; 
        }
    })

    testAVPlayer.on('error', async (error) => {
        console.info('audioInterruptMode value is error, error message is :' + error.message)
        testAVPlayer.release();
    })
    testAVPlayer.fdSrc = videoSource;
}