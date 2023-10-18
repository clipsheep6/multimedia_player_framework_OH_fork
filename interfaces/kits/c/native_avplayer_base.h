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

#ifndef MULTIMEDIA_PLAYER_FRAMEWORK_NATIVE_AVPLAYER_BASH_H
#define MULTIMEDIA_PLAYER_FRAMEWORK_NATIVE_AVPLAYER_BASH_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OH_AVPlayer OH_AVPlayer;
typedef struct NativeWindow OHNativeWindow;

typedef enum OH_PlayerStates {
    /* error states */
    OH_PLAYER_STATE_ERROR = 0,
    /* idle states */
    OH_PLAYER_IDLE = 1,
    /* initialized states(Internal states) */
    OH_PLAYER_INITIALIZED = 2,
    /* preparing states(Internal states) */
    OH_PLAYER_PREPARING = 3,
    /* prepared states */
    OH_PLAYER_PREPARED = 4,
    /* started states */
    OH_PLAYER_STARTED = 5,
    /* paused states */
    OH_PLAYER_PAUSED = 6,
    /* stopped states */
    OH_PLAYER_STOPPED = 7,
    /* Play to the end states */
    OH_PLAYER_PLAYBACK_COMPLETE = 8,
    /* released states */
    OH_PLAYER_RELEASED = 9,
} OH_PlayerStates;

typedef enum OH_PlayerSeekMode {
    /* sync to keyframes after the time point. */
    OH_SEEK_NEXT_SYNC = 0,
    /* sync to keyframes before the time point. */
    OH_SEEK_PREVIOUS_SYNC,
    /* sync to closest keyframes. */
    OH_SEEK_CLOSEST_SYNC,
    /* seek to frames closest the time point. */
    OH_SEEK_CLOSEST,
} OH_PlayerSeekMode;

typedef enum OH_PlaybackRateMode {
    /* Video playback at 0.75x normal speed */
    OH_SPEED_FORWARD_0_75_X,
    /* Video playback at normal speed */
    OH_SPEED_FORWARD_1_00_X,
    /* Video playback at 1.25x normal speed */
    OH_SPEED_FORWARD_1_25_X,
    /* Video playback at 1.75x normal speed */
    OH_SPEED_FORWARD_1_75_X,
    /* Video playback at 2.0x normal speed */
    OH_SPEED_FORWARD_2_00_X,
} OH_PlaybackRateMode;

typedef enum OH_PlayerOnInfoType {
    /* return the message when seeking done. */
    OH_INFO_TYPE_SEEKDONE = 1,
    /* return the message when speeding done. */
    OH_INFO_TYPE_SPEEDDONE,
    /* return the message when select bitrate done */
    OH_INFO_TYPE_BITRATEDONE,
    /* return the message when playback is end of steam. */
    OH_INFO_TYPE_EOS,
    /* return the message when PlayerStates changed. */
    OH_INFO_TYPE_STATE_CHANGE,
    /* return the current posion of playback automatically. */
    OH_INFO_TYPE_POSITION_UPDATE,
    /* return the playback message. */
    OH_INFO_TYPE_MESSAGE,
    /* return the message when volume changed. */
    OH_INFO_TYPE_VOLUME_CHANGE,
    /* return the message when video size is first known or updated. */
    OH_INFO_TYPE_RESOLUTION_CHANGE,
    /* return multiqueue buffering time. */
    OH_INFO_TYPE_BUFFERING_UPDATE,
    /* return hls bitrate.
       Bitrate is to convert data into uint8_t array storage,
       which needs to be forcibly converted to uint32_t through offset access. */
    OH_INFO_TYPE_BITRATE_COLLECT,
    /* return the message when audio focus changed. */
    OH_INFO_TYPE_INTERRUPT_EVENT,
    /* return the message when PlayerStates changed by audio. */
    OH_INFO_TYPE_STATE_CHANGE_BY_AUDIO,
    /* return the message with extra information in format. */
    OH_INFO_TYPE_EXTRA_FORMAT,
    /* return the duration of playback. */
    OH_INFO_TYPE_DURATION_UPDATE,
    /* return the playback is live stream. */
    OH_INFO_TYPE_IS_LIVE_STREAM,
    /* return the message when track changes. */
    OH_INFO_TYPE_TRACKCHANGE,
    /* return the default audio track. */
    OH_INFO_TYPE_DEFAULTTRACK,
    /* return to the end of track processing. */
    OH_INFO_TYPE_TRACK_DONE,
    /* return error message to prompt the user. */
    OH_INFO_TYPE_ERROR_MSG,
    /* return the message when subtitle track num updated. */
    OH_INFO_TYPE_TRACK_NUM_UPDATE,
    /* return the message when subtitle track info updated. */
    OH_INFO_TYPE_TRACK_OH_INFO_UPDATE,
    /* return the subtitle of playback. */
    OH_INFO_TYPE_SUBTITLE_UPDATE,
    /* return to the end of adding subtitle processing. */
    OH_INFO_TYPE_ADD_SUBTITLE_DONE,
} OH_PlayerOnInfoType;

/**
 * Called when a player message or alarm is received.
 *
 * @param player The pointer to an OH_AVPlayer instance.
 * @param type Indicates the information type. For details, see {@link PlayerOnInfoType}.
 * @param extra Indicates other information, for example, the start time position of a playing file.
 */
typedef void (*OH_AVPlayerOnInfo)(OH_AVPlayer *player, OH_PlayerOnInfoType type, int32_t extra);

/**
 * Called when an error occurred for versions above api9
 *
 * @param player The pointer to an OH_AVPlayer instance.
 * @param errorCode Error code.
 * @param errorMsg Error message.
 */
typedef void (*OH_AVPlayerOnError)(OH_AVPlayer *player, int32_t errorCode, const char *errorMsg);


typedef struct OH_AVPlayerCallback {
    OH_AVPlayerOnInfo onInfo;
    OH_AVPlayerOnError onError;
} OH_AVPlayerCallback;


#ifdef __cplusplus
}
#endif
#endif // MULTIMEDIA_PLAYER_FRAMEWORK_NATIVE_AVPLAYER_BASH_H
