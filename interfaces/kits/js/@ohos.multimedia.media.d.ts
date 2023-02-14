/*
* Copyright (C) 2021 Huawei Device Co., Ltd.
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

import { ErrorCallback, AsyncCallback, Callback } from './basic';
import audio from "./@ohos.multimedia.audio";

/**
 * @name media
 * @since 6
 * @import import media from '@ohos.multimedia.media'
 */
declare namespace media {
  /**
   * Creates an AVPlayer instance.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.AVPlayer
   * @import import media from '@ohos.multimedia.media'
   * @return callback Callback used to return an AVPlayer instance if the operation is successful; returns null otherwise.
   */
   function createAVPlayer(callback: AsyncCallback<AVPlayer>): void;

  /**
   * Creates an AVPlayer instance.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.AVPlayer
   * @import import media from '@ohos.multimedia.media'
   * @return A Promise instance used to return an AVPlayer instance if the operation is successful; returns null otherwise.
   */
   function createAVPlayer() : Promise<AVPlayer>;

  /**
   * Creates an AudioPlayer instance.
   * @since 6
   * @syscap SystemCapability.Multimedia.Media.AudioPlayer
   * @import import media from '@ohos.multimedia.media'
   * @return Returns an AudioPlayer instance if the operation is successful; returns null otherwise.
   */
  function createAudioPlayer(): AudioPlayer;

  /**
   * Creates an AudioRecorder instance.
   * @since 6
   * @syscap SystemCapability.Multimedia.Media.AudioRecorder
   * @import import media from '@ohos.multimedia.media'
   * @return Returns an AudioRecorder instance if the operation is successful; returns null otherwise.
   */
  function createAudioRecorder(): AudioRecorder;

  /**
   * Creates an VideoPlayer instance.
   * @since 8
   * @syscap SystemCapability.Multimedia.Media.VideoPlayer
   * @import import media from '@ohos.multimedia.media'
   * @param callback Callback used to return AudioPlayer instance if the operation is successful; returns null otherwise.
   */
  function createVideoPlayer(callback: AsyncCallback<VideoPlayer>): void;
  /**
   * Creates an VideoPlayer instance.
   * @since 8
   * @syscap SystemCapability.Multimedia.Media.VideoPlayer
   * @import import media from '@ohos.multimedia.media'
   * @return A Promise instance used to return VideoPlayer instance if the operation is successful; returns null otherwise.
   */
  function createVideoPlayer() : Promise<VideoPlayer>;

  /**
   * Creates an VideoRecorder instance.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.VideoRecorder
   * @import import media from '@ohos.multimedia.media'
   * @param callback Callback used to return AudioPlayer instance if the operation is successful; returns null otherwise.
   * @throws { BusinessError } 5400101 - No memory. Return by callback.
   * @systemapi
   */
  function createVideoRecorder(callback: AsyncCallback<VideoRecorder>): void;
  /**
   * Creates an VideoRecorder instance.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.VideoRecorder
   * @import import media from '@ohos.multimedia.media'
   * @return A Promise instance used to return VideoRecorder instance if the operation is successful; returns null otherwise.
   * @throws { BusinessError } 5400101 - No memory. Return by promise.
   * @systemapi
   */
  function createVideoRecorder(): Promise<VideoRecorder>;

  /**
   * Enumerates state change reason.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   * @import import media from '@ohos.multimedia.media'
   */
   enum StateChangeReason {
    /**
     * state change by user operation.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    USER = 1,

    /**
     * state change by background action.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    BACKGROUND = 2,
  }

 /**
   * Enumerates ErrorCode types, return in BusinessError::code
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   * @import import media from '@ohos.multimedia.media'
   */
 enum AVErrorCode {
  /**
   * operation success.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_OK = 0,

  /**
   * permission denied.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_NO_PERMISSION = 201,

  /**
   * invalid parameter.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_INVALID_PARAMETER = 401,

  /**
   * the api is not supported in the current version
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_UNSUPPORT_CAPABILITY = 801,

  /**
   * the system memory is insufficient or the number of services reaches the upper limit
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_NO_MEMORY = 5400101,

  /**
   * current status does not allow or do not have permission to perform this operation
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_OPERATE_NOT_PERMIT = 5400102,

  /**
   * data flow exception information
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_IO = 5400103,

  /**
   * system or network response timeout.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_TIMEOUT = 5400104,

  /**
   * service process died.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_SERVICE_DIED = 5400105,

  /**
   * unsupported media format
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_UNSUPPORT_FORMAT = 5400106,
 }

  /**
   * Describes media playback states.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.AVPlayer
   * @import import media from '@ohos.multimedia.media'
   */
   type AVPlayerState = 'idle' | 'initialized' | 'prepared' | 'playing' | 'paused' | 'completed' | 'stopped' | 'released' | 'error';

   /**
    * Manages and plays media. Before calling an AVPlayer method, you must use createAVPlayer()
    * to create an AVPlayer instance.
    * @since 9
    * @syscap SystemCapability.Multimedia.Media.AVPlayer
    */
  interface AVPlayer {
    /**
     * prepare video playback, it will request resource for playing.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param callback A callback instance used to return when prepare completed.
     */
    prepare(callback: AsyncCallback<void>): void;

    /**
     * prepare video playback, it will request resource for playing.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @return A Promise instance used to return when prepare completed.
     */
    prepare(): Promise<void>;

    /**
     * Starts media playback.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    play(): void;
 
    /**
     * Pauses media playback.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    pause(): void;

    /**
     * Stops media playback.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    stop(): void;

    /**
     * reset AVPlayer, it will to idle state and can set src again.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param callback A callback instance used to return when reset completed.
     */
    reset(callback: AsyncCallback<void>): void;

    /**
     * reset AVPlayer, it will to idle state and can set src again.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @return A Promise instance used to return when reset completed.
     */
    reset(): Promise<void>;

    /**
     * Jumps to the specified playback position.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param timeMs Playback position to jump, should be in [0, 2147483647].
     * @param mode seek mode, see @SeekMode .
     */
    seek(timeMs: number, mode?:SeekMode): void;

    /**
     * Sets the volume.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param vol Relative volume. The value ranges from 0.00 to 1.00. The value 1 indicates the maximum volume (100%).
     */
    setVolume(volume: number): void;

    /**
     * Releases resources used for AVPlayer.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param callback A callback instance used to return when release completed.
     */
    release(callback: AsyncCallback<void>): void;

    /**
     * Releases resources used for AVPlayer.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @return A Promise instance used to return when release completed.
     */
    release(): Promise<void>;

    /**
     * get all track infos in MediaDescription, should be called after data loaded callback.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param callback async callback return track info in MediaDescription.
     */
    getTrackDescription(callback: AsyncCallback<Array<MediaDescription>>): void;

    /**
     * get all track infos in MediaDescription, should be called after data loaded callback..
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @return A Promise instance used to return the track info in MediaDescription.
     */
    getTrackDescription() : Promise<Array<MediaDescription>>;

    /**
     * Media URI. Mainstream media formats are supported.
     * network:http://xxx
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    url ?: string;

    /**
     * Media file descriptor. Mainstream media formats are supported.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    fdSrc ?: AVFileDescriptor;

    /**
     * DataSource descriptor. Mainstream media formats are supported.
     * @since 10
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    dataSrc ?: DataSrcDescriptor;

    /**
     * Whether to loop media playback. The value true means to loop playback.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    loop: boolean;

    /**
     * Describes audio interrupt mode, refer to {@link #audio.InterruptMode}. If it is not
     * set, the default mode will be used. Set it before calling the {@link #play()} in the
     * first time in order for the interrupt mode to become effective thereafter.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    audioInterruptMode ?: audio.InterruptMode;

    /**
     * Current playback position.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    readonly currentTime: number;

    /**
     * Playback duration, When the data source does not support seek, it returns - 1, such as a live broadcast scenario.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    readonly duration: number;

    /**
     * Playback state.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    readonly state: AVPlayerState;

    /**
     * SurfaceId surface id, video player will use this id get a surface instance.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    surfaceId ?: string;

    /**
     * video width, valid after prepared.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    readonly width: number;

    /**
     * video height, valid after prepared.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    readonly height: number;

    /**
     * video scale type. Defaultly, the {@link #VIDEO_SCALE_TYPE_FIT} will be used, for more
     * information, refer to {@link #VideoScaleType}
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    videoScaleType ?: VideoScaleType;

    /**
     * set payback speed.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param speed playback speed, see @PlaybackSpeed .
     */
    setSpeed(speed: number): void;

    /**
     * select a specified bitrate to playback, only valid for HLS protocal network stream. Defaulty, the
     * player will select the appropriate bitrate according to the network connection speed. The
     * available bitrates list reported by {@link #on('availableBitrates')}. Set it to select
     * a specified bitrate. If the specified bitrate is not in the list of available bitrates, the player
     * will select the minimal and closest one from the available bitrates list.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param bitrate the playback bitrate must be expressed in bits per second.
     */
    setBitrate(bitrate: number): void;

    /**
     * Listens for media playback events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback stateChange event.
     */
    on(type: 'stateChange', callback: (state: AVPlayerState, reason: StateChangeReason) => void): void;
    /**
     * Listens for media playback events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback volume event.
     */
    on(type: 'volumeChange', callback: Callback<number>): void;
    /**
     * Listens for media playback events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback end of stream
     */
    on(type: 'endOfStream', callback: Callback<void>): void;
    /**
     * Listens for media playback events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback seekDone event.
     */
    on(type: 'seekDone', callback: Callback<number>): void;
    /**
     * Listens for media playback events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback speedDone event.
     */
    on(type: 'speedDone', callback: Callback<number>): void;
    /**
     * Listens for media playback events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback setBitrateDone event.
     */
    on(type: 'bitrateDone', callback: Callback<number>): void;
    /**
     * Listens for media playback events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback timeUpdate event.
     */
    on(type: 'timeUpdate', callback: Callback<number>): void;
    /**
     * Listens for media playback events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback durationUpdate event.
     */
     on(type: 'durationUpdate', callback: Callback<number>): void;
    /**
     * Listens for video playback buffering events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback buffering update event to listen for.
     * @param callback Callback used to listen for the buffering update event, return BufferingInfoType and the value.
     */
    on(type: 'bufferingUpdate', callback: (infoType: BufferingInfoType, value: number) => void): void;
    /**
     * Listens for start render video frame events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return .
     */
    on(type: 'startRenderFrame', callback: Callback<void>): void;
    /**
     * Listens for video size change event.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return video size.
     */
    on(type: 'videoSizeChange', callback: (width: number, height: number) => void): void;
    /**
     * Listens for audio interrupt event, refer to {@link #audio.InterruptEvent}
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return audio interrupt info.
     */
    on(type: 'audioInterrupt', callback: (info: audio.InterruptEvent) => void): void;
    /**
     * Listens for available bitrates collect completed events for HLS protocal stream playback.
     * This event will be reported after the {@link #prepare} called.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return available bitrates.
     */
    on(type: 'availableBitrates', callback: (bitrates: Array<number>) => void): void;
    /**
     * Listens for playback error events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback error event to listen for.
     * @param callback Callback used to listen for the playback error event.
     */
    on(type: 'error', callback: ErrorCallback): void;
  }

  /**
   * Enumerates ErrorCode types, return in BusinessError::code
   * @since 8
   * @syscap SystemCapability.Multimedia.Media.Core
   * @import import media from '@ohos.multimedia.media'
   */
  enum MediaErrorCode {
    /**
     * operation success.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MSERR_OK = 0,

    /**
     * malloc or new memory failed. maybe system have no memory.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MSERR_NO_MEMORY = 1,

    /**
     * no permission for the operation.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MSERR_OPERATION_NOT_PERMIT = 2,

    /**
     * invalid argument.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MSERR_INVALID_VAL = 3,

    /**
     * an IO error occurred.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MSERR_IO = 4,

    /**
     * operation time out.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MSERR_TIMEOUT = 5,

    /**
     * unknown error.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MSERR_UNKNOWN = 6,

    /**
     * media service died.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MSERR_SERVICE_DIED = 7,

    /**
     * operation is not permit in current state.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MSERR_INVALID_STATE = 8,

    /**
     * operation is not supported in current version.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MSERR_UNSUPPORTED = 9,
  }

  /**
   * Enumerates buffering info type, for network playback.
   * @since 8
   * @syscap SystemCapability.Multimedia.Media.Core
   * @import import media from '@ohos.multimedia.media'
   */
  enum BufferingInfoType {
    /**
     * begin to buffering
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    BUFFERING_START = 1,

    /**
     * end to buffering
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    BUFFERING_END = 2,

    /**
     * buffering percent
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    BUFFERING_PERCENT = 3,

    /**
     * cached duration in milliseconds
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    CACHED_DURATION = 4,
  }

  interface AVFileDescriptor {
    /**
     * The file descriptor of audio or video source from file system. The caller
     * is responsible to close the file descriptor.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    fd: number

    /**
     * The offset into the file where the data to be readed, in bytes. Defaultly,
     * the offset is zero.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    offset?: number

    /**
     * The length in bytes of the data to be readed. Defaultly, the length is the
     * rest of bytes in the file from the offset.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    length?: number
  }

  /**
    * DataSource descriptor. The caller needs to ensure that the fileSize and 
    * callback is valid.
    * @since 10
    * @syscap SystemCapability.Multimedia.Media.Core
    */
  interface DataSrcDescriptor {
    /**
     * Size of the file, -1 indicates that the file size is unknown. If the fileSize is set to -1,
     * seek and setSpeed can't be executed, loop can't be set, and can't replay.
     * @since 10
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    fileSize: number;
    /**
     * Callback function implemented by users, which is used to fill data.
     * @param buffer The buffer need to fill.
     * @param length The stream length player want to get.
     * @param pos The stream position player want get start.
     * @return returns length of the data to be filled.
     * @since 10
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    callback: (buffer: ArrayBuffer, length: number, pos?: number) => int
  }

  /**
   * Describes audio playback states.
   * @since 6
   * @syscap SystemCapability.Multimedia.Media.AudioPlayer
   * @import import media from '@ohos.multimedia.media'
   */
  type AudioState = 'idle' | 'playing' | 'paused' | 'stopped' | 'error';

  /**
   * Manages and plays audio. Before calling an AudioPlayer method, you must use createAudioPlayer()
   * to create an AudioPlayer instance.
   * @since 6
   * @syscap SystemCapability.Multimedia.Media.AudioPlayer
   */
  interface AudioPlayer {
    /**
     * Starts audio playback.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     */
    play(): void;

    /**
     * Pauses audio playback.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     */
    pause(): void;

    /**
     * Stops audio playback.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     */
    stop(): void;

    /**
     * Resets audio playback.
     * @since 7
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     */
    reset(): void;

    /**
     * Jumps to the specified playback position.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @param timeMs Playback position to jump
     */
    seek(timeMs: number): void;

    /**
     * Sets the volume.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @param vol Relative volume. The value ranges from 0.00 to 1.00. The value 1 indicates the maximum volume (100%).
     */
    setVolume(vol: number): void;

    /**
     * Releases resources used for audio playback.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     */
    release(): void;
    /**
    * get all track infos in MediaDescription, should be called after data loaded callback.
    * @since 8
    * @syscap SystemCapability.Multimedia.Media.AudioPlayer
    * @param callback async callback return track info in MediaDescription.
    */
    getTrackDescription(callback: AsyncCallback<Array<MediaDescription>>): void;

    /**
    * get all track infos in MediaDescription, should be called after data loaded callback..
    * @since 8
    * @syscap SystemCapability.Multimedia.Media.AudioPlayer
    * @return A Promise instance used to return the track info in MediaDescription.
    */
    getTrackDescription() : Promise<Array<MediaDescription>>;

    /**
     * Listens for audio playback buffering events.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @param type Type of the playback buffering update event to listen for.
     * @param callback Callback used to listen for the buffering update event, return BufferingInfoType and the value.
     */
    on(type: 'bufferingUpdate', callback: (infoType: BufferingInfoType, value: number) => void): void;

    /**
     * Audio media URI. Mainstream audio formats are supported.
     * local:fd://XXX, file://XXX. network:http://xxx
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @permission ohos.permission.READ_MEDIA or ohos.permission.INTERNET
     */
    src: string;

    /**
     * Audio file descriptor. Mainstream audio formats are supported.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     */
    fdSrc: AVFileDescriptor;

    /**
     * Whether to loop audio playback. The value true means to loop playback.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     */
    loop: boolean;

    /**
     * Describes audio interrupt mode, refer to {@link #audio.InterruptMode}. If it is not
     * set, the default mode will be used. Set it before calling the {@link #play()} in the
     * first time in order for the interrupt mode to become effective thereafter.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     */
    audioInterruptMode ?: audio.InterruptMode;

    /**
     * Current playback position.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     */
    readonly currentTime: number;

    /**
     * Playback duration, When the data source does not support seek, it returns - 1, such as a live broadcast scenario.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     */
    readonly duration: number;

    /**
     * Playback state.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     */
    readonly state: AudioState;

    /**
     * Listens for audio playback events.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event.
     */
    on(type: 'play' | 'pause' | 'stop' | 'reset' | 'dataLoad' | 'finish' | 'volumeChange', callback: () => void): void;

    /**
     * Listens for audio playback events.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event.
     */
    on(type: 'timeUpdate', callback: Callback<number>): void;

    /**
     * Listens for audio interrupt event, refer to {@link #audio.InterruptEvent}
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return audio interrupt info.
     */
    on(type: 'audioInterrupt', callback: (info: audio.InterruptEvent) => void): void;

    /**
     * Listens for playback error events.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @param type Type of the playback error event to listen for.
     * @param callback Callback used to listen for the playback error event.
     */
    on(type: 'error', callback: ErrorCallback): void;
  }

  /**
   * Enumerates audio encoding formats, it will be deprecated after API8, use @CodecMimeType to replace.
   * @since 6
   * @syscap SystemCapability.Multimedia.Media.AudioRecorder
   * @import import media from '@ohos.multimedia.media'
   * @deprecated since 8
   */
  enum AudioEncoder {
    /**
     * Default audio encoding format, which is AMR-NB.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    DEFAULT = 0,

    /**
     * Indicates the AMR-NB audio encoding format.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    AMR_NB = 1,

    /**
     * Indicates the AMR-WB audio encoding format.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    AMR_WB = 2,

    /**
     * Advanced Audio Coding Low Complexity (AAC-LC).
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    AAC_LC = 3,

    /**
     * High-Efficiency Advanced Audio Coding (HE-AAC).
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    HE_AAC = 4
  }

  /**
   * Enumerates audio output formats, it will be deprecated after API8, use @ContainerFormatType to replace.
   * @since 6
   * @syscap SystemCapability.Multimedia.Media.AudioRecorder
   * @import import media from '@ohos.multimedia.media'
   * @deprecated since 8
   */
  enum AudioOutputFormat {
    /**
     * Default audio output format, which is Moving Pictures Expert Group 4 (MPEG-4).
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    DEFAULT = 0,

    /**
     * Indicates the Moving Picture Experts Group-4 (MPEG4) media format.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    MPEG_4 = 2,

    /**
     * Indicates the Adaptive Multi-Rate Narrowband (AMR-NB) media format.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    AMR_NB = 3,

    /**
     * Indicates the Adaptive Multi-Rate Wideband (AMR-WB) media format.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    AMR_WB = 4,

    /**
     * Audio Data Transport Stream (ADTS), a transmission stream format of Advanced Audio Coding (AAC) audio.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    AAC_ADTS = 6
  }

  /**
   * Provides the geographical location definitions for media resources.
   * @since 6
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  interface Location {
    /**
     * Latitude.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    latitude: number;

    /**
     * Longitude.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    longitude: number;
  }

  /**
   * Provides the audio recorder configuration definitions.
   * @since 6
   * @syscap SystemCapability.Multimedia.Media.AudioRecorder
   */
  interface AudioRecorderConfig {
    /**
     * Audio encoding format. The default value is DEFAULT, it will be deprecated after API8.
     * use "audioEncoderMime" instead.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @deprecated since 8
     */
    audioEncoder?: AudioEncoder;

    /**
     * Audio encoding bit rate.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    audioEncodeBitRate?: number;

    /**
     * Audio sampling rate.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    audioSampleRate?: number;

    /**
     * Number of audio channels.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    numberOfChannels?: number;

    /**
     * Audio output format. The default value is DEFAULT, it will be deprecated after API8.
     * it will be replaced with "fileFormat".
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @deprecated since 8
     */
    format?: AudioOutputFormat;

    /**
     * Audio output uri.support two kind of uri now.
     * format like: scheme + "://" + "context".
     * file:  file://path
     * fd:    fd://fd
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    uri: string;

    /**
     * Geographical location information.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    location?: Location;

    /**
     * audio encoding format MIME. it used to replace audioEncoder.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    audioEncoderMime?: CodecMimeType;
    /**
     * output file format. see @ContainerFormatType , it used to replace "format".
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    fileFormat?: ContainerFormatType;
  }

  /**
   * Manages and record audio. Before calling an AudioRecorder method, you must use createAudioRecorder()
   * to create an AudioRecorder instance.
   * @since 6
   * @syscap SystemCapability.Multimedia.Media.AudioRecorder
   */
  interface AudioRecorder {
    /**
     * Prepares for recording.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @param config Recording parameters.
     * @permission ohos.permission.MICROPHONE
     */
    prepare(config: AudioRecorderConfig): void;

    /**
     * Starts audio recording.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    start(): void;

    /**
     * Pauses audio recording.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    pause(): void;

    /**
     * Resumes audio recording.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    resume(): void;

    /**
     * Stops audio recording.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    stop(): void;

    /**
     * Releases resources used for audio recording.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    release(): void;

    /**
     * Resets audio recording.
     * Before resetting audio recording, you must call stop() to stop recording. After audio recording is reset,
     * you must call prepare() to set the recording configurations for another recording.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     */
    reset(): void;

    /**
     * Listens for audio recording events.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @param type Type of the audio recording event to listen for.
     * @param callback Callback used to listen for the audio recording event.
     */
    on(type: 'prepare' | 'start' | 'pause' | 'resume' | 'stop' | 'release' | 'reset', callback: () => void): void;

    /**
     * Listens for audio recording error events.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @param type Type of the audio recording error event to listen for.
     * @param callback Callback used to listen for the audio recording error event.
     */
    on(type: 'error', callback: ErrorCallback): void;
  }

  /**
  * Describes video recorder states.
  * @since 9
  * @syscap SystemCapability.Multimedia.Media.VideoRecorder
  * @systemapi
  */
  type VideoRecordState = 'idle' | 'prepared' | 'playing' | 'paused' | 'stopped' | 'error';

  /**
   * Manages and record video. Before calling an VideoRecorder method, you must use createVideoRecorder()
   * to create an VideoRecorder instance.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.VideoRecorder
   * @systemapi
   */
  interface VideoRecorder {
    /**
     * Prepares for recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @param config Recording parameters.
     * @param callback A callback instance used to return when prepare completed.
     * @permission ohos.permission.MICROPHONE
     * @throws { BusinessError } 201 - Permission denied. Return by callback.
     * @throws { BusinessError } 401 - Parameter error. Return by callback.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     * @systemapi
     */
    prepare(config: VideoRecorderConfig, callback: AsyncCallback<void>): void;
    /**
     * Prepares for recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @param config Recording parameters.
     * @return A Promise instance used to return when prepare completed.
     * @permission ohos.permission.MICROPHONE
     * @throws { BusinessError } 201 - Permission denied. Return by promise.
     * @throws { BusinessError } 401 - Parameter error. Return by promise.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by promise.
     * @throws { BusinessError } 5400105 - Service died. Return by promise.
     * @systemapi
     */
    prepare(config: VideoRecorderConfig): Promise<void>;
    /**
     * get input surface.it must be called between prepare completed and start.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @param callback Callback used to return the input surface id in string.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by callback.
     * @throws { BusinessError } 5400103 - IO error. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     * @systemapi
     */
    getInputSurface(callback: AsyncCallback<string>): void;
    /**
     * get input surface. it must be called between prepare completed and start.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @return A Promise instance used to return the input surface id in string.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by promise.
     * @throws { BusinessError } 5400103 - IO error. Return by promise.
     * @throws { BusinessError } 5400105 - Service died. Return by promise.
     * @systemapi
     */
    getInputSurface(): Promise<string>;
    /**
     * Starts video recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @param callback A callback instance used to return when start completed.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by callback.
     * @throws { BusinessError } 5400103 - IO error. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     * @systemapi
     */
    start(callback: AsyncCallback<void>): void;
    /**
     * Starts video recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @return A Promise instance used to return when start completed.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by promise.
     * @throws { BusinessError } 5400103 - IO error. Return by promise.
     * @throws { BusinessError } 5400105 - Service died. Return by promise.
     * @systemapi
     */
    start(): Promise<void>;
    /**
     * Pauses video recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @param callback A callback instance used to return when pause completed.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by callback.
     * @throws { BusinessError } 5400103 - IO error. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     * @systemapi
     */
    pause(callback: AsyncCallback<void>): void;
    /**
     * Pauses video recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @return A Promise instance used to return when pause completed.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by promise.
     * @throws { BusinessError } 5400103 - IO error. Return by promise.
     * @throws { BusinessError } 5400105 - Service died. Return by promise.
     * @systemapi
     */
    pause(): Promise<void>;
    /**
     * Resumes video recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @param callback A callback instance used to return when resume completed.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by callback.
     * @throws { BusinessError } 5400103 - IO error. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     * @systemapi
     */
    resume(callback: AsyncCallback<void>): void;
    /**
     * Resumes video recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @return A Promise instance used to return when resume completed.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by promise.
     * @throws { BusinessError } 5400103 - IO error. Return by promise.
     * @throws { BusinessError } 5400105 - Service died. Return by promise.
     * @systemapi
     */
    resume(): Promise<void>;
    /**
     * Stops video recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @param callback A callback instance used to return when stop completed.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by callback.
     * @throws { BusinessError } 5400103 - IO error. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     * @systemapi
     */
    stop(callback: AsyncCallback<void>): void;
    /**
     * Stops video recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @return A Promise instance used to return when stop completed.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by promise.
     * @throws { BusinessError } 5400103 - IO error. Return by promise.
     * @throws { BusinessError } 5400105 - Service died. Return by promise.
     * @systemapi
     */
    stop(): Promise<void>;
    /**
     * Releases resources used for video recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @param callback A callback instance used to return when release completed.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     * @systemapi
     */
    release(callback: AsyncCallback<void>): void;
    /**
      * Releases resources used for video recording.
      * @since 9
      * @syscap SystemCapability.Multimedia.Media.VideoRecorder
      * @return A Promise instance used to return when release completed.
      * @throws { BusinessError } 5400105 - Service died. Return by callback.
      * @systemapi
      */
    release(): Promise<void>;
    /**
     * Resets video recording.
     * Before resetting video recording, you must call stop() to stop recording. After video recording is reset,
     * you must call prepare() to set the recording configurations for another recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @param callback A callback instance used to return when reset completed.
     * @throws { BusinessError } 5400103 - IO error. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     * @systemapi
     */
    reset(callback: AsyncCallback<void>): void;
     /**
      * Resets video recording.
      * Before resetting video recording, you must call stop() to stop recording. After video recording is reset,
      * you must call prepare() to set the recording configurations for another recording.
      * @since 9
      * @syscap SystemCapability.Multimedia.Media.VideoRecorder
      * @return A Promise instance used to return when reset completed.
      * @throws { BusinessError } 5400103 - IO error. Return by promise.
      * @throws { BusinessError } 5400105 - Service died. Return by promise.
      * @systemapi
      */
    reset(): Promise<void>;
    /**
     * Listens for video recording error events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @param type Type of the video recording error event to listen for.
     * @param callback Callback used to listen for the video recording error event.
     * @throws { BusinessError } 5400103 - IO error. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     * @systemapi
     */
    on(type: 'error', callback: ErrorCallback): void;

    /**
     * video recorder state.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
     readonly state: VideoRecordState;
  }

  /**
   * Describes video playback states.
   * @since 8
   * @syscap SystemCapability.Multimedia.Media.VideoPlayer
   */
  type VideoPlayState = 'idle' | 'prepared' | 'playing' | 'paused' | 'stopped' | 'error';

  /**
   * Enumerates playback speed.
   * @since 8
   * @syscap SystemCapability.Multimedia.Media.VideoPlayer
   */
  enum PlaybackSpeed {
    /**
     * playback at 0.75x normal speed
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     */
    SPEED_FORWARD_0_75_X = 0,
    /**
     * playback at normal speed
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     */
    SPEED_FORWARD_1_00_X = 1,
    /**
     * playback at 1.25x normal speed
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     */
    SPEED_FORWARD_1_25_X = 2,
    /**
     * playback at 1.75x normal speed
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     */
    SPEED_FORWARD_1_75_X = 3,
    /**
     * playback at 2.0x normal speed
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     */
    SPEED_FORWARD_2_00_X = 4,
  }

  /**
   * Manages and plays video. Before calling an video method, you must use createVideoPlayer() to create an VideoPlayer
   * instance.
   * @since 8
   * @syscap SystemCapability.Multimedia.Media.VideoPlayer
   * @import import media from '@ohos.multimedia.media'
   */
  interface VideoPlayer {
    /**
     * set display surface.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param surfaceId surface id, video player will use this id get a surface instance.
     * @return A Promise instance used to return when release output buffer completed.
     */
    setDisplaySurface(surfaceId: string, callback: AsyncCallback<void>): void;
    /**
    * set display surface.
    * @since 8
    * @syscap SystemCapability.Multimedia.Media.VideoPlayer
    * @param surfaceId surface id, video player will use this id get a surface instance.
    * @return A Promise instance used to return when release output buffer completed.
    */
    setDisplaySurface(surfaceId: string): Promise<void>;
    /**
     * prepare video playback, it will request resource for playing.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param callback A callback instance used to return when prepare completed.
     */
    prepare(callback: AsyncCallback<void>): void;
     /**
      * prepare video playback, it will request resource for playing.
      * @since 8
      * @syscap SystemCapability.Multimedia.Media.VideoPlayer
      * @return A Promise instance used to return when prepare completed.
      */
    prepare(): Promise<void>;
    /**
     * Starts video playback.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param callback A callback instance used to return when start completed.
     */
    play(callback: AsyncCallback<void>): void;
     /**
      * Starts video playback.
      * @since 8
      * @syscap SystemCapability.Multimedia.Media.VideoPlayer
      * @return A Promise instance used to return when start completed.
      */
    play(): Promise<void>;
    /**
     * Pauses video playback.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param callback A callback instance used to return when pause completed.
     */
    pause(callback: AsyncCallback<void>): void;
     /**
      * Pauses video playback.
      * @since 8
      * @syscap SystemCapability.Multimedia.Media.VideoPlayer
      * @return A Promise instance used to return when pause completed.
      */
    pause(): Promise<void>;
    /**
     * Stops video playback.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param callback A callback instance used to return when stop completed.
     */
    stop(callback: AsyncCallback<void>): void;
     /**
      * Stops video playback.
      * @since 8
      * @syscap SystemCapability.Multimedia.Media.VideoPlayer
      * @return A Promise instance used to return when stop completed.
      */
    stop(): Promise<void>;
    /**
     * Resets video playback, it will release the resource.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param callback A callback instance used to return when reset completed.
     */
    reset(callback: AsyncCallback<void>): void;
     /**
      * Resets video playback, it will release the resource.
      * @since 8
      * @syscap SystemCapability.Multimedia.Media.VideoPlayer
      * @return A Promise instance used to return when reset completed.
      */
    reset(): Promise<void>;
    /**
     * Jumps to the specified playback position by default SeekMode(SEEK_CLOSEST),
     * the performance may be not the best.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param timeMs Playback position to jump
     * @param callback A callback instance used to return when seek completed
     * and return the seeking position result.
     */
    seek(timeMs: number, callback: AsyncCallback<number>): void;
    /**
     * Jumps to the specified playback position.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param timeMs Playback position to jump
     * @param mode seek mode, see @SeekMode .
     * @param callback A callback instance used to return when seek completed
     * and return the seeking position result.
     */
    seek(timeMs: number, mode:SeekMode, callback: AsyncCallback<number>): void;
     /**
      * Jumps to the specified playback position.
      * @since 8
      * @syscap SystemCapability.Multimedia.Media.VideoPlayer
      * @param timeMs Playback position to jump
      * @param mode seek mode, see @SeekMode .
      * @return A Promise instance used to return when seek completed
      * and return the seeking position result.
      */
    seek(timeMs: number, mode?:SeekMode): Promise<number>;
    /**
     * Sets the volume.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param vol Relative volume. The value ranges from 0.00 to 1.00. The value 1 indicates the maximum volume (100%).
     * @param callback A callback instance used to return when set volume completed.
     */
    setVolume(vol: number, callback: AsyncCallback<void>): void;
     /**
      * Sets the volume.
      * @since 8
      * @syscap SystemCapability.Multimedia.Media.VideoPlayer
      * @param vol Relative volume. The value ranges from 0.00 to 1.00. The value 1 indicates the maximum volume (100%).
      * @return A Promise instance used to return when set volume completed.
      */
    setVolume(vol: number): Promise<void>;
    /**
     * Releases resources used for video playback.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param callback A callback instance used to return when release completed.
     */
    release(callback: AsyncCallback<void>): void;
     /**
      * Releases resources used for video playback.
      * @since 8
      * @syscap SystemCapability.Multimedia.Media.VideoPlayer
      * @return A Promise instance used to return when release completed.
      */
    release(): Promise<void>;
    /**
    * get all track infos in MediaDescription, should be called after data loaded callback.
    * @since 8
    * @syscap SystemCapability.Multimedia.Media.VideoPlayer
    * @param callback async callback return track info in MediaDescription.
    */
    getTrackDescription(callback: AsyncCallback<Array<MediaDescription>>): void;

    /**
    * get all track infos in MediaDescription, should be called after data loaded callback..
    * @since 8
    * @syscap SystemCapability.Multimedia.Media.VideoPlayer
    * @return A Promise instance used to return the track info in MediaDescription.
    */
    getTrackDescription() : Promise<Array<MediaDescription>>;

    /**
     * media url. Mainstream video formats are supported.
     * local:fd://XXX, file://XXX. network:http://xxx
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     */
    url: string;

    /**
     * Video file descriptor. Mainstream video formats are supported.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     */
    fdSrc: AVFileDescriptor;

    /**
     * Whether to loop video playback. The value true means to loop playback.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     */
    loop: boolean;

    /**
     * Current playback position.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     */
    readonly currentTime: number;

    /**
     * Playback duration, if -1 means cannot seek.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     */
    readonly duration: number;

    /**
     * Playback state.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     */
    readonly state: VideoPlayState;

    /**
     * video width, valid after prepared.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     */
    readonly width: number;

    /**
     * video height, valid after prepared.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     */
    readonly height: number;

    /**
     * Describes audio interrupt mode, refer to {@link #audio.InterruptMode}. If it is not
     * set, the default mode will be used. Set it before calling the {@link #play()} in the
     * first time in order for the interrupt mode to become effective thereafter.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     */
    audioInterruptMode ?: audio.InterruptMode;

    /**
     * video scale type. Defaultly, the {@link #VIDEO_SCALE_TYPE_FIT} will be used, for more
     * information, refer to {@link #VideoScaleType}
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     */
    videoScaleType ?: VideoScaleType;

    /**
     * set payback speed.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param speed playback speed, see @PlaybackSpeed .
     * @param callback Callback used to return actually speed.
     */
    setSpeed(speed:number, callback: AsyncCallback<number>): void;
    /**
     * set output surface.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param speed playback speed, see @PlaybackSpeed .
     * @return A Promise instance used to return actually speed.
     */
    setSpeed(speed:number): Promise<number>;

    /**
     * Listens for video playback completed events.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return .
     */
    on(type: 'playbackCompleted', callback: Callback<void>): void;

    /**
     * Listens for video playback buffering events.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param type Type of the playback buffering update event to listen for.
     * @param callback Callback used to listen for the buffering update event, return BufferingInfoType and the value.
     */
    on(type: 'bufferingUpdate', callback: (infoType: BufferingInfoType, value: number) => void): void;

    /**
     * Listens for start render video frame events.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return .
     */
    on(type: 'startRenderFrame', callback: Callback<void>): void;

    /**
     * Listens for video size changed event.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return video size.
     */
    on(type: 'videoSizeChanged', callback: (width: number, height: number) => void): void;

    /**
     * Listens for audio interrupt event, refer to {@link #audio.InterruptEvent}
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return audio interrupt info.
     */
    on(type: 'audioInterrupt', callback: (info: audio.InterruptEvent) => void): void;

    /**
     * Listens for playback error events.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param type Type of the playback error event to listen for.
     * @param callback Callback used to listen for the playback error event.
     */
    on(type: 'error', callback: ErrorCallback): void;
  }

  /**
   * Enumerates video scale type.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.VideoPlayer
   * @import import media from '@ohos.multimedia.media'
   */
  enum VideoScaleType {
    /**
     * The content is stretched to the fit the display surface rendering area. When
     * the aspect ratio of the content is not same as the display surface, the aspect
     * of the content is not maintained. This is the default scale type.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     */
    VIDEO_SCALE_TYPE_FIT = 0,

    /**
     * The content is stretched to the fit the display surface rendering area. When
     * the aspect ratio of the content is not the same as the display surface, content's
     * aspect ratio is maintained and the content is cropped to fit the display surface.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     */
    VIDEO_SCALE_TYPE_FIT_CROP = 1,
  }

  /**
   * Enumerates container format type(The abbreviation for 'container format type' is CFT).
   * @since 8
   * @syscap SystemCapability.Multimedia.Media.Core
   * @import import media from '@ohos.multimedia.media'
   */
  enum ContainerFormatType {
    /**
     * A video container format type mp4.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    CFT_MPEG_4 = "mp4",

    /**
     * A audio container format type m4a.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    CFT_MPEG_4A = "m4a",
  }

  /**
   * Enumerates media data type.
   * @since 8
   * @syscap SystemCapability.Multimedia.Media.Core
   * @import import media from '@ohos.multimedia.media'
   */
  enum MediaType {
    /**
     * track is audio.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MEDIA_TYPE_AUD = 0,
    /**
     * track is video.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MEDIA_TYPE_VID = 1,
  }

  /**
   * Enumerates media description key.
   * @since 8
   * @syscap SystemCapability.Multimedia.Media.Core
   * @import import media from '@ohos.multimedia.media'
   */
  enum MediaDescriptionKey {
    /**
     * key for track index, value type is number.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MD_KEY_TRACK_INDEX = "track_index",

    /**
     * key for track type, value type is number, see @MediaType.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MD_KEY_TRACK_TYPE = "track_type",

    /**
     * key for codec mime type, value type is string.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MD_KEY_CODEC_MIME = "codec_mime",

    /**
     * key for duration, value type is number.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MD_KEY_DURATION = "duration",

    /**
     * key for bitrate, value type is number.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MD_KEY_BITRATE = "bitrate",

    /**
     * key for video width, value type is number.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MD_KEY_WIDTH = "width",

    /**
     * key for video height, value type is number.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MD_KEY_HEIGHT = "height",

    /**
     * key for video frame rate, value type is number.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MD_KEY_FRAME_RATE = "frame_rate",

    /**
     * key for audio channel count, value type is number
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MD_KEY_AUD_CHANNEL_COUNT = "channel_count",

    /**
     * key for audio sample rate, value type is number
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    MD_KEY_AUD_SAMPLE_RATE = "sample_rate",
  }

  /**
   * Provides the video recorder profile definitions.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.VideoRecorder
   * @systemapi
   */
  interface VideoRecorderProfile {
    /**
     * Indicates the audio bit rate.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    readonly audioBitrate: number;

    /**
     * Indicates the number of audio channels.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    readonly audioChannels: number;

    /**
     * Indicates the audio encoding format.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    readonly audioCodec: CodecMimeType;

    /**
     * Indicates the audio sampling rate.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    readonly audioSampleRate: number;

    /**
     * Indicates the output file format.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    readonly fileFormat: ContainerFormatType;

    /**
     * Indicates the video bit rate.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    readonly videoBitrate: number;

    /**
     * Indicates the video encoding format.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    readonly videoCodec: CodecMimeType;

    /**
     * Indicates the video width.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    readonly videoFrameWidth: number;

    /**
     * Indicates the video height.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    readonly videoFrameHeight: number;

    /**
     * Indicates the video frame rate.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    readonly videoFrameRate: number;
  }

  /**
   * Enumerates audio source type for recorder.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.VideoRecorder
   * @import import media from '@ohos.multimedia.media'
   * @systemapi
   */
  enum AudioSourceType {
    /**
     * default audio source type.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    AUDIO_SOURCE_TYPE_DEFAULT = 0,
    /**
     * source type mic.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    AUDIO_SOURCE_TYPE_MIC = 1,
  }

  /**
   * Enumerates video source type for recorder.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.VideoRecorder
   * @import import media from '@ohos.multimedia.media'
   * @systemapi
   */
  enum VideoSourceType {
    /**
     * surface raw data.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    VIDEO_SOURCE_TYPE_SURFACE_YUV = 0,
    /**
     * surface ES data.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    VIDEO_SOURCE_TYPE_SURFACE_ES = 1,
  }

  /**
   * Provides the video recorder configuration definitions.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.VideoRecorder
   * @systemapi
   */
  interface VideoRecorderConfig {
    /**
     * audio source type, details see @AudioSourceType .
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    audioSourceType?: AudioSourceType;
    /**
     * video source type, details see @VideoSourceType .
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    videoSourceType: VideoSourceType;
    /**
     * video recorder profile, can get by "getVideoRecorderProfile", details see @VideoRecorderProfile .
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    profile: VideoRecorderProfile;
    /**
     * video output uri.support two kind of uri now.
     * format like: scheme + "://" + "context".
     * fd:    fd://fd
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    url: string;
    /**
     * Sets the video rotation angle in output file, and for the file to playback. mp4 support.
     * the range of rotation angle should be {0, 90, 180, 270}, default is 0.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    rotation?: number;
    /**
     * geographical location information.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @systemapi
     */
    location?: Location;
  }

  /**
   * Provides the container definition for media description key-value pairs.
   * @since 8
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  interface MediaDescription {
    /**
     * key:value pair, key see @MediaDescriptionKey .
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    [key : string]: Object;
  }

  /**
   * Enumerates seek mode.
   * @since 8
   * @syscap SystemCapability.Multimedia.Media.Core
   * @import import media from '@ohos.multimedia.media'
   */
  enum SeekMode {
    /**
     * seek to the next sync frame of the given timestamp
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    SEEK_NEXT_SYNC = 0,
    /**
     * seek to the previous sync frame of the given timestamp
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    SEEK_PREV_SYNC = 1,
  }

  /**
   * Enumerates Codec MIME types.
   * @since 8
   * @syscap SystemCapability.Multimedia.Media.Core
   * @import import media from '@ohos.multimedia.media'
   */
   enum CodecMimeType {
    /**
     * H.263 codec MIME type.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    VIDEO_H263 = 'video/h263',
    /**
     * H.264 codec MIME type.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    VIDEO_AVC = 'video/avc',
    /**
     * MPEG2 codec MIME type.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    VIDEO_MPEG2 = 'video/mpeg2',
    /**
     * MPEG4 codec MIME type
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    VIDEO_MPEG4 = 'video/mp4v-es',

    /**
     * VP8 codec MIME type
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    VIDEO_VP8 = 'video/x-vnd.on2.vp8',

    /**
     * AAC codec MIME type.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    AUDIO_AAC = 'audio/mp4a-latm',

    /**
     * vorbis codec MIME type.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    AUDIO_VORBIS = 'audio/vorbis',

    /**
     * flac codec MIME type.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    AUDIO_FLAC = 'audio/flac',
  }
}
export default media;
