/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

import * as mediaTestBase from '../../../../../../MediaTestBase.js';
import media from '@ohos.multimedia.media'
import audio from '@ohos.multimedia.audio';
import { testAVPlayerFun, AV_PLAYER_STATE, setSource, testAVPlayerSeek } from '../../../../../../AVPlayerTestBase.js';
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from 'deccjsunit/index';

export default function AVPlayerLocalTestH() {
    describe('AVPlayerLocalTestH', function () {
        const VIDEO_SOURCE = 'H265_MP3_720x480_60fps.mp4';
        const AUDIO_SOURCE = '01.mp3';
        const VIDEO_NOAUDIO = 'H265_MP3_720x480_60fps_noaudio.mp4';
        const PLAY_TIME = 3000;
        const TAG = 'AVPlayerLocalTestH265:';
        let fileDescriptor = null;
        let fileDescriptor2 = null;
        let fileDescriptor3 = null;
        let avPlayer = null;
        let avPlayTest = {
            width: 0,
            height: 0,
            duration: -1,
        }

        const audioRenderInfoContent = [
            audio.ContentType.CONTENT_TYPE_UNKNOWN,
            audio.ContentType.CONTENT_TYPE_SPEECH,
            audio.ContentType.CONTENT_TYPE_MUSIC,
            audio.ContentType.CONTENT_TYPE_MOVIE,
            audio.ContentType.CONTENT_TYPE_SONIFICATION,
            audio.ContentType.CONTENT_TYPE_RINGTONE,
            audio.ContentType.CONTENT_TYPE_ULTRASONIC,
        ]

        const audioRenderInfoUsage = [
            audio.StreamUsage.STREAM_USAGE_UNKNOWN,
            audio.StreamUsage.STREAM_USAGE_MEDIA,
            audio.StreamUsage.STREAM_USAGE_VOICE_COMMUNICATION,
            audio.StreamUsage.STREAM_USAGE_VOICE_ASSISTANT,
            audio.StreamUsage.STREAM_USAGE_ALARM,
            audio.StreamUsage.STREAM_USAGE_NOTIFICATION_RINGTONE,
            audio.StreamUsage.STREAM_USAGE_ACCESSIBILITY,
            audio.StreamUsage.STREAM_USAGE_SYSTEM,
        ]

        beforeAll(async function() {
            console.info(TAG + 'beforeAll case');
            await mediaTestBase.getFileDescriptor(VIDEO_SOURCE).then((res) => {
                fileDescriptor = res;
            });
            await mediaTestBase.getFileDescriptor(AUDIO_SOURCE).then((res) => {
                fileDescriptor2 = res;
            });
            await mediaTestBase.getFileDescriptor(VIDEO_NOAUDIO).then((res) => {
                fileDescriptor3 = res;
            });
        })

        beforeEach(async function() {
            console.info(TAG + 'beforeEach case');
        })

        afterEach(async function() {
            if (avPlayer != null) {
                avPlayer.release().then(() => {
                }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
            }
            console.info(TAG + 'afterEach case');
        })

        afterAll(async function() {
            console.info(TAG + 'afterAll case');
            await mediaTestBase.closeFileDescriptor(VIDEO_SOURCE);
            await mediaTestBase.closeFileDescriptor(VIDEO_NOAUDIO);
        })

        function setAVPlayerTrackCb(avPlayer, descriptionKey, descriptionValue, done) {
            let arrayDescription;
            let surfaceID = globalThis.value;
            avPlayer.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AV_PLAYER_STATE.INITIALIZED:
                        avPlayer.surfaceId = surfaceID;
                        expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.INITIALIZED);
                        avPlayer.prepare();
                        break;
                    case AV_PLAYER_STATE.PREPARED:
                        await avPlayer.getTrackDescription().then((arrayList) => {
                            if (typeof (arrayList) != 'undefined') {
                                arrayDescription = arrayList;
                            } else {
                                console.info(TAG + 'case getTrackDescription is failed');
                                expect().assertFail();
                            }
                        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                        expect(descriptionKey.length).assertEqual(arrayDescription.length);
                        for (let i = 0; i < arrayDescription.length; i++) {
                            mediaTestBase.checkDescription(arrayDescription[i], descriptionKey[i], descriptionValue[i]);
                        }
                        avPlayer.getTrackDescription((error, arrayList) => {
                            if (error == null) {
                                for (let i = 0; i < arrayList.length; i++) {
                                    mediaTestBase.checkDescription(arrayList[i], descriptionKey[i], descriptionValue[i]);
                                }
                            } else {
                                console.info(TAG + 'getTrackDescription failed, message is:' + error.message);
                            }
                            avPlayer.release();
                        })
                        break;
                    case AV_PLAYER_STATE.RELEASED:
                        avPlayer = null;
                        done();
                        break;
                    case AV_PLAYER_STATE.ERROR:
                        expect().assertFail();
                        avPlayer.release().then(() => {
                        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                        avPlayer = null;
                        break;
                    default:
                        break; 
                }
            })
        }
        
        async function testCheckTrackDescription(src, avPlayer, descriptionKey, descriptionValue, done) {
            console.info(TAG + `case media source: ${src}`)
            media.createAVPlayer((err, video) => {
                if (typeof (video) != 'undefined') {
                    console.info(TAG + 'case createAVPlayer success');
                    avPlayer = video;
                    setAVPlayerTrackCb(avPlayer, descriptionKey, descriptionValue, done)
                    setSource(avPlayer, src);
                }
                if (err != null) {
                    console.error(TAG + `case createAVPlayer error, errMessage is ${err.message}`);
                    expect().assertFail();
                    done();
                }
            });
        }
        
        async function setAVPlayerScaleCb(avPlayer, done) {
            let surfaceID = globalThis.value;
            let count = 0;
            avPlayer.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AV_PLAYER_STATE.INITIALIZED:
                        avPlayer.surfaceId = surfaceID;
                        avPlayer.prepare();
                        break;
                    case AV_PLAYER_STATE.PREPARED:
                        avPlayer.loop = true;
                        expect(avPlayer.loop).assertEqual(true);
                        avPlayer.play();
                        break;
                    case AV_PLAYER_STATE.PLAYING:
                        for (let i = 0; i < 20; i++) {
                            if (count == 0) {
                                console.info(TAG + 'case set  videoScaleType : 1');
                                avPlayer.videoScaleType = media.VideoScaleType.VIDEO_SCALE_TYPE_FIT_CROP;
                                expect(avPlayer.videoScaleType).assertEqual(media.VideoScaleType.VIDEO_SCALE_TYPE_FIT_CROP);
                                count = 1;
                            } else {
                                console.info(TAG + 'case set  videoScaleType : 0');
                                avPlayer.videoScaleType = media.VideoScaleType.VIDEO_SCALE_TYPE_FIT;
                                expect(avPlayer.videoScaleType).assertEqual(media.VideoScaleType.VIDEO_SCALE_TYPE_FIT);
                                count = 0;
                            }
                            await mediaTestBase.msleepAsync(500); // play sleep 500ms
                        }
                        avPlayer.loop = false;
                        break;
                    case AV_PLAYER_STATE.COMPLETED:
                        expect(avPlayer.currentTime).assertEqual(avPlayer.duration);
                        expect(avPlayer.loop).assertEqual(false);
                        avPlayer.release();
                        break;
                    case AV_PLAYER_STATE.RELEASED:
                        avPlayer = null;
                        done();
                        break;
                    case AV_PLAYER_STATE.ERROR:
                        onsole.info(TAG + 'case now state is ERROR');
                        expect().assertFail();
                        avPlayer.release().then(() => {
                        }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
                        avPlayer = null;
                        break;
                    default:
                        break; 
                }
            })
        }
        
        async function testVideoScaleType(src, avPlayer, done) {
            console.info(TAG + `case media source: ${src}`)
            media.createAVPlayer((err, video) => {
                console.info(TAG + `case media err: ${err}`)
                if (typeof (video) != 'undefined') {
                    console.info(TAG + 'case createAVPlayer success');
                    avPlayer = video;
                    setAVPlayerScaleCb(avPlayer, done)
                    setSource(avPlayer, src);
                }
                if (err != null) {
                    console.error(TAG + `case createAVPlayer error, errMessage is ${err.message}`);
                    expect().assertFail();
                    done();
                }
            });
        }
        
        async function testAudioInterruptMode(audioSource, videoSource, done) {
            let testAVPlayer01 = await media.createAVPlayer();
            let testAVPlayer02 = await media.createAVPlayer();
            let surfaceID = globalThis.value;
            testAVPlayer01.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AV_PLAYER_STATE.INITIALIZED:
                        testAVPlayer01.prepare();
                        break;
                    case AV_PLAYER_STATE.PREPARED:
                        testAVPlayer01.audioInterruptMode = audio.InterruptMode.INDEPENDENT_MODE;
                        expect(testAVPlayer01.audioInterruptMode).assertEqual(audio.InterruptMode.INDEPENDENT_MODE);
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
                console.info(TAG + 'case audioInterrupt1 is called, info is :' + JSON.stringify(info));
                await testAVPlayer02.release();
                await testAVPlayer01.release().then(() => {
                    console.info(TAG + 'case release called!!');
                    done();
                }, mediaTestBase.failureCallback).catch(mediaTestBase.catchCallback);
            });
        
            testAVPlayer02.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AV_PLAYER_STATE.INITIALIZED:
                        expect(testAVPlayer02.state).assertEqual(AV_PLAYER_STATE.INITIALIZED);
                        testAVPlayer02.surfaceId = surfaceID;
                        testAVPlayer02.prepare();
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

        async function testAudioRendererInfo(src, avPlayer, done) {
            avPlayer = await media.createAVPlayer();
            let surfaceID = globalThis.value;
            avPlayer.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AV_PLAYER_STATE.INITIALIZED:
                        avPlayer.surfaceId = surfaceID;
                        expect(avPlayer.state).assertEqual(AV_PLAYER_STATE.INITIALIZED);
                        for (let i = 0; i < audioRenderInfoContent.length; i++) {
                            for (let j = 0; j < audioRenderInfoUsage.length; j++) {
                                let audioRenderInfo = {
                                    content: audioRenderInfoContent[i],
                                    usage: audioRenderInfoUsage[j],
                                    rendererFlags: 0,
                                }
                                avPlayer.audioRendererInfo = audioRenderInfo;
                                expect(avPlayer.audioRendererInfo.content).assertEqual(audioRenderInfoContent[i])
                                expect(avPlayer.audioRendererInfo.usage).assertEqual(audioRenderInfoUsage[j])
                            }
                        }
                        avPlayer.prepare((err) => {
                            if (err != null) {
                                console.error(TAG + `case prepare error, errMessage is ${err.message}`);
                                expect().assertFail();
                                done();
                            }
                        });
                        break;
                    case AV_PLAYER_STATE.PREPARED:
                        avPlayer.play();
                        break;
                    case AV_PLAYER_STATE.PLAYING:
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
            avPlayer.fdSrc = src;
        }

        async function setOnCallback(avPlayer, done) {
            let surfaceID = globalThis.value;
            let count = 0;
            let playCount = 0;
            avPlayer.on('endOfStream', () => {
                count++;
                avPlayer.off('endOfStream')
            })
            avPlayer.on('stateChange', async (state, reason) => {
                switch (state) {
                    case AV_PLAYER_STATE.INITIALIZED:
                        avPlayer.surfaceId = surfaceID;
                        avPlayer.prepare();
                        break;
                    case AV_PLAYER_STATE.PREPARED:
                        avPlayer.play();
                        break;
                    case AV_PLAYER_STATE.PLAYING:
                        if (playCount == 0) {
                            playCount++;
                            avPlayer.pause();
                        }
                        break;
                    case AV_PLAYER_STATE.PAUSED:
                        avPlayer.play();
                        break;
                    case AV_PLAYER_STATE.STOPPED:
                        expect(count).assertEqual(1);
                        avPlayer.release();
                        break;
                    case AV_PLAYER_STATE.COMPLETED:
                        expect(avPlayer.currentTime).assertEqual(avPlayer.duration);
                        if (playCount == 1) {
                            playCount++
                            avPlayer.play();
                        } else {
                            avPlayer.stop();
                        }
                        break;
                    case AV_PLAYER_STATE.RELEASED:
                        avPlayer = null;
                        done();
                        break;
                    case AV_PLAYER_STATE.ERROR:
                        expect().assertFail();
                        avPlayer.release();
                        break;
                    default:
                        break; 
                }
            })
        }

        async function testOffCallback(src, avPlayer, done) {
            console.info(TAG + `case media source: ${src}`)
            media.createAVPlayer((err, video) => {
                if (typeof (video) != 'undefined') {
                    console.info(TAG + 'case createAVPlayer success');
                    avPlayer = video;
                    setOnCallback(avPlayer, done)
                    setSource(avPlayer, src);
                }
                if (err != null) {
                    console.error(TAG + `case createAVPlayer error, errMessage is ${err.message}`);
                    expect().assertFail();
                    done();
                }
            });
        }
        
        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_h265_FDSRC_0100
            * @tc.name      : 001.H265_AAC - fdsrc
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level0
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_h265_FDSRC_0100', 0, async function (done) {
            avPlayTest = { width: 720, height: 480, duration: 10034 };
            testAVPlayerFun(fileDescriptor, avPlayer, avPlayTest, PLAY_TIME, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_h265_OFF_CALLBACK_0100
            * @tc.name      : 003.H265_AAC - off callback Function
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_h265_OFF_CALLBACK_0100', 0, async function (done) {
            testOffCallback(fileDescriptor, avPlayer, done);
        })
        
        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_h265_GETTRECKDESCRIPTION_0100
            * @tc.name      : 003.H265_AAC - getTrackDescription - video/audio
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_h265_GETTRECKDESCRIPTION_0100', 0, async function (done) {
            let videoTrackKey = new Array('codec_mime', 'frame_rate', 'height',
                                        'track_index', 'track_type', 'width');
            let audioTrackKey = new Array('bitrate', 'channel_count', 'codec_mime', 'sample_rate',
                                        'track_index', 'track_type');
            let videoTrackValue = new Array(3, 6000, 480, 0, 1, 720);
            let audioTrackValue = new Array(129674, 2, 4, 44100, 1, 0);
            let descriptionKey = new Array(videoTrackKey, audioTrackKey);
            let descriptionValue = new Array(videoTrackValue, audioTrackValue);
            testCheckTrackDescription(fileDescriptor, avPlayer, descriptionKey, descriptionValue, done)
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDI_AVPLAYER_h265_GETTRECKDESCRIPTION_0200
            * @tc.name      : 004.H265_AAC - getTrackDescription -pureVideo
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDI_AVPLAYER_h265_GETTRECKDESCRIPTION_0200', 0, async function (done) {
            let videoTrackKey = new Array('codec_mime', 'frame_rate', 'height',
                                        'track_index', 'track_type', 'width');
            let videoTrackValue = new Array(3, 6000, 480, 0, 1, 720);
            let descriptionKey = new Array(videoTrackKey);
            let descriptionValue = new Array(videoTrackValue);
            testCheckTrackDescription(fileDescriptor3, avPlayer, descriptionKey, descriptionValue, done)
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_h265_VIDEOSCALETYPE_0100
            * @tc.name      : 005.H265_AAC - videoScaleTpe
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_h265_VIDEOSCALETYPE_0100', 0, async function (done) {
            testVideoScaleType(fileDescriptor, avPlayer, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_h265_AUDIOINTERRUPTMODE_0100
            * @tc.name      : 006.H265_AAC - audioInterruptMode Function
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_h265_AUDIOINTERRUPTMODE_0100', 0, async function (done) {
            testAudioInterruptMode(fileDescriptor, fileDescriptor2, done);
        })

        /* *
            * @tc.number    : SUB_MULTIMEDIA_MEDIA_AVPLAYER_h265_AUDIORENDERERINFO_0100
            * @tc.name      : 007.H265_AAC - audioInterruptMode
            * @tc.desc      : Local Video playback control test
            * @tc.size      : MediumTest
            * @tc.type      : Function test
            * @tc.level     : Level1
        */
        it('SUB_MULTIMEDIA_MEDIA_AVPLAYER_h265_AUDIORENDERERINFO_0100', 0, async function (done) {
            testAudioRendererInfo(fileDescriptor, avPlayer, done);
        })
    })
}
