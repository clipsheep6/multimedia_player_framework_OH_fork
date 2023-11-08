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

/**
 * @addtogroup AVPlayer
 * @{
 *
 * @brief Provides APIs of Playback capability for Media Source.
 *
 * @Syscap SystemCapability.Multimedia.Media.AVPlayer
 * @since 11
 * @version 1.0
 */

/**
 * @file avplayer_base.h
 *
 * @brief Defines the structure and enumeration for Media AVPlayer.
 *
 * @library libmedia_player.so
 * @since 11
 * @version 1.0
 */

#ifndef MULTIMEDIA_PLAYER_FRAMEWORK_NATIVE_AVPLAYER_BASH_H
#define MULTIMEDIA_PLAYER_FRAMEWORK_NATIVE_AVPLAYER_BASH_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OH_AVPlayer OH_AVPlayer;
typedef struct NativeWindow OHNativeWindow;

/**
 * @brief Player States
 * @syscap SystemCapability.Multimedia.Media.AVPlayer
 * @since 11
 * @version 1.0
 */
typedef enum AVPlayerStates {
    /* error states */
    AV_STATE_ERROR = 0,
    /* idle states */
    AV_IDLE = 1,
    /* initialized states(Internal states) */
    AV_INITIALIZED = 2,
    /* preparing states(Internal states) */
    AV_PREPARING = 3,
    /* prepared states */
    AV_PREPARED = 4,
    /* started states */
    AV_STARTED = 5,
    /* paused states */
    AV_PAUSED = 6,
    /* stopped states */
    AV_STOPPED = 7,
    /* Play to the end states */
    AV_PLAYBACK_COMPLETE = 8,
    /* released states */
    AV_RELEASED = 9,
} AVPlayerStates;

/**
 * @brief Player Seek Mode
 * @syscap SystemCapability.Multimedia.Media.AVPlayer
 * @since 11
 * @version 1.0
 */
typedef enum AVPlayerSeekMode {
    /* sync to keyframes after the time point. */
    AV_SEEK_NEXT_SYNC = 0,
    /* sync to keyframes before the time point. */
    AV_SEEK_PREVIOUS_SYNC,
    /* sync to closest keyframes. */
    AV_SEEK_CLOSEST_SYNC,
    /* seek to frames closest the time point. */
    AV_SEEK_CLOSEST,
} AVPlayerSeekMode;

/**
 * @brief Playback Rate Mode
 * @syscap SystemCapability.Multimedia.Media.AVPlayer
 * @since 11
 * @version 1.0
 */
typedef enum AVPlaybackRateMode {
    /* Video playback at 0.75x normal speed */
    AV_SPEED_FORWARD_0_75_X,
    /* Video playback at normal speed */
    AV_SPEED_FORWARD_1_00_X,
    /* Video playback at 1.25x normal speed */
    AV_SPEED_FORWARD_1_25_X,
    /* Video playback at 1.75x normal speed */
    AV_SPEED_FORWARD_1_75_X,
    /* Video playback at 2.0x normal speed */
    AV_SPEED_FORWARD_2_00_X,
} AVPlaybackRateMode;

/**
 * @brief Player OnInfo Type
 * @syscap SystemCapability.Multimedia.Media.AVPlayer
 * @since 11
 * @version 1.0
 */
typedef enum AVPlayerOnInfoType {
    /* return the message when seeking done. */
    AV_INFO_TYPE_SEEKDONE = 1,
    /* return the message when speeding done. */
    AV_INFO_TYPE_SPEEDDONE,
    /* return the message when select bitrate done */
    AV_INFO_TYPE_BITRATEDONE,
    /* return the message when playback is end of steam. */
    AV_INFO_TYPE_EOS,
    /* return the message when PlayerStates changed. */
    AV_INFO_TYPE_STATE_CHANGE,
    /* return the current posion of playback automatically. */
    AV_INFO_TYPE_POSITION_UPDATE,
    /* return the playback message. */
    AV_INFO_TYPE_MESSAGE,
    /* return the message when volume changed. */
    AV_INFO_TYPE_VOLUME_CHANGE,
    /* return the message when video size is first known or updated. */
    AV_INFO_TYPE_RESOLUTION_CHANGE,
    /* return multiqueue buffering time. */
    AV_INFO_TYPE_BUFFERING_UPDATE,
    /* return hls bitrate.
       Bitrate is to convert data into uint8_t array storage,
       which needs to be forcibly converted to uint32_t through offset access. */
    AV_INFO_TYPE_BITRATE_COLLECT,
    /* return the message when audio focus changed. */
    AV_INFO_TYPE_INTERRUPT_EVENT,
    /* return the message when PlayerStates changed by audio. */
    AV_INFO_TYPE_STATE_CHANGE_BY_AUDIO,
    /* return the message with extra information in format. */
    AV_INFO_TYPE_EXTRA_FORMAT,
    /* return the duration of playback. */
    AV_INFO_TYPE_DURATION_UPDATE,
    /* return the playback is live stream. */
    AV_INFO_TYPE_IS_LIVE_STREAM,
    /* return the message when track changes. */
    AV_INFO_TYPE_TRACKCHANGE,
    /* return the default audio track. */
    INFO_TYPE_DEFAULTTRACK,
    /* return to the end of track processing. */
    AV_INFO_TYPE_TRACK_DONE,
    /* return error message to prompt the user. */
    AV_INFO_TYPE_ERROR_MSG,
    /* return the message when subtitle track num updated. */
    AV_INFO_TYPE_TRACK_NUM_UPDATE,
    /* return the message when subtitle track info updated. */
    AV_INFO_TYPE_TRACK_OH_INFO_UPDATE,
    /* return the subtitle of playback. */
    AV_INFO_TYPE_SUBTITLE_UPDATE,
    /* return to the end of adding subtitle processing. */
    AV_INFO_TYPE_ADD_SUBTITLE_DONE,
} AVPlayerOnInfoType;

/**
 * @brief Called when a player message or alarm is received.
 * @syscap SystemCapability.Multimedia.Media.AVPlayer
 * @param player The pointer to an OH_AVPlayer instance.
 * @param type Indicates the information type. For details, see {@link AVPlayerOnInfoType}.
 * @param extra Indicates other information, for example, the start time position of a playing file.
 * @since 11
 * @version 1.0
 */
typedef void (*OH_AVPlayerOnInfo)(OH_AVPlayer *player, AVPlayerOnInfoType type, int32_t extra);

/**
 * @brief Called when an error occurred for versions above api9
 * @syscap SystemCapability.Multimedia.Media.AVPlayer
 * @param player The pointer to an OH_AVPlayer instance.
 * @param errorCode Error code.
 * @param errorMsg Error message.
 * @since 11
 * @version 1.0
 */
typedef void (*OH_AVPlayerOnError)(OH_AVPlayer *player, int32_t errorCode, const char *errorMsg);

/**
 * @brief A collection of all callback function pointers in OH_AVPlayer. Register an instance of this
 * structure to the OH_AVPlayer instance, and process the information reported through the callback to ensure the
 * normal operation of OH_AVPlayer.
 * @syscap SystemCapability.Multimedia.Media.AVPlayer
 * @param onInfo Monitor OH_AVPlayer operation information, refer to {@link OH_AVPlayerOnInfo}
 * @param onError Monitor OH_AVPlayer operation errors, refer to {@link OH_AVPlayerOnError}
 * @since 11
 * @version 1.0
 */
typedef struct AVPlayerCallback {
    OH_AVPlayerOnInfo onInfo;
    OH_AVPlayerOnError onError;
} AVPlayerCallback;


#ifdef __cplusplus
}
#endif
#endif // MULTIMEDIA_PLAYER_FRAMEWORK_NATIVE_AVPLAYER_BASH_H
