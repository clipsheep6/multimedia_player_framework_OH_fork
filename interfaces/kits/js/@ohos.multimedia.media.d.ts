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

import { ErrorCallback, AsyncCallback, Callback } from './@ohos.base';
import audio from "./@ohos.multimedia.audio";
import type image from './@ohos.multimedia.image';
import type drm from './@ohos.multimedia.drm';
/**
 * @name media
 * @since 6
 */
declare namespace media {
  /**
   * Creates an AVPlayer instance.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.AVPlayer
   * @param callback Callback used to return AVPlayer instance if the operation is successful; returns null otherwise.
   * @throws { BusinessError } 5400101 - No memory. Return by callback.
   */
  /**
   * Creates an AVPlayer instance.
   * @since 11
   * @syscap SystemCapability.Multimedia.Media.AVPlayer
   * @param callback Callback used to return AVPlayer instance if the operation is successful; returns null otherwise.
   * @atomicservice
   * @throws { BusinessError } 5400101 - No memory. Return by callback.
   */
   function createAVPlayer(callback: AsyncCallback<AVPlayer>): void;

  /**
   * Creates an AVPlayer instance.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.AVPlayer
   * @returns A Promise instance used to return AVPlayer instance if the operation is successful; returns null otherwise.
   * @throws { BusinessError } 5400101 - No memory. Return by promise.
   */
  /**
   * Creates an AVPlayer instance.
   * @since 11
   * @syscap SystemCapability.Multimedia.Media.AVPlayer
   * @returns A Promise instance used to return AVPlayer instance if the operation is successful; returns null otherwise.
   * @atomicservice
   * @throws { BusinessError } 5400101 - No memory. Return by promise.
   */
   function createAVPlayer() : Promise<AVPlayer>;

  /**
   * Creates an AVRecorder instance.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.AVRecorder
   * @param callback Callback used to return AVRecorder instance if the operation is successful; returns null otherwise.
   * @throws { BusinessError } 5400101 - No memory. Return by callback.
   */
  function createAVRecorder(callback: AsyncCallback<AVRecorder>): void;

   /**
    * Creates an AVRecorder instance.
    * @since 9
    * @syscap SystemCapability.Multimedia.Media.AVRecorder
    * @returns A Promise instance used to return AVRecorder instance if the operation is successful; returns null otherwise.
    * @throws { BusinessError } 5400101 - No memory. Return by promise.
    */
  function createAVRecorder() : Promise<AVRecorder>;

  /**
   * Creates an AudioPlayer instance.
   * @since 6
   * @syscap SystemCapability.Multimedia.Media.AudioPlayer
   * @returns Returns an AudioPlayer instance if the operation is successful; returns null otherwise.
   * @deprecated since 9
   * @useinstead ohos.multimedia.media/media#createAVPlayer
   */
  function createAudioPlayer(): AudioPlayer;

  /**
   * Creates an AudioRecorder instance.
   * @since 6
   * @syscap SystemCapability.Multimedia.Media.AudioRecorder
   * @returns Returns an AudioRecorder instance if the operation is successful; returns null otherwise.
   * @deprecated since 9
   * @useinstead ohos.multimedia.media/media#createAVRecorder
   */
  function createAudioRecorder(): AudioRecorder;

  /**
   * Creates an VideoPlayer instance.
   * @since 8
   * @syscap SystemCapability.Multimedia.Media.VideoPlayer
   * @param callback Callback used to return AudioPlayer instance if the operation is successful; returns null otherwise.
   * @deprecated since 9
   * @useinstead ohos.multimedia.media/media#createAVPlayer
   */
  function createVideoPlayer(callback: AsyncCallback<VideoPlayer>): void;

  /**
   * Creates an VideoPlayer instance.
   * @since 8
   * @syscap SystemCapability.Multimedia.Media.VideoPlayer
   * @returns A Promise instance used to return VideoPlayer instance if the operation is successful; returns null otherwise.
   * @deprecated since 9
   * @useinstead ohos.multimedia.media/media#createAVPlayer
   */
  function createVideoPlayer() : Promise<VideoPlayer>;

  /**
   * The maintenance of this interface has been stopped since version api 9. Please use AVRecorder
   * Creates an VideoRecorder instance.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.VideoRecorder
   * @param callback Callback used to return AudioPlayer instance if the operation is successful; returns null otherwise.
   * @throws { BusinessError } 5400101 - No memory. Return by callback.
   * @systemapi
   */
  function createVideoRecorder(callback: AsyncCallback<VideoRecorder>): void;

  /**
   * The maintenance of this interface has been stopped since version api 9. Please use AVRecorder
   * Creates an VideoRecorder instance.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.VideoRecorder
   * @returns A Promise instance used to return VideoRecorder instance if the operation is successful; returns null otherwise.
   * @throws { BusinessError } 5400101 - No memory. Return by promise.
   * @systemapi
   */
  function createVideoRecorder(): Promise<VideoRecorder>;

  /**
   * Creates an AVMetadataExtractor instance.
   * @returns { Promise<AVMetadataExtractor> } A Promise instance used to return AVMetadataExtractor instance
   * if the operation is successful; returns null otherwise.
   * @throws { BusinessError } 5400101 - No memory. Returned by promise.
   * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
   * @since 11
   */
  function createAVMetadataExtractor(): Promise<AVMetadataExtractor>;

  /**
   * Creates an AVMetadataExtractor instance.
   * @param { AsyncCallback<AVMetadataExtractor> } callback - Callback used to return AVMetadataExtractor instance
   * if the operation is successful; returns null otherwise.
   * @throws { BusinessError } 5400101 - No memory. Returned by callback.
   * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
   * @since 11
   */
  function createAVMetadataExtractor(callback: AsyncCallback<AVMetadataExtractor>): void;

  /**
   * Creates an AVImageGenerator instance.
   * @returns { Promise<AVImageGenerator> } A Promise instance used to return AVImageGenerator instance
   * if the operation is successful; returns null otherwise.
   * @throws { BusinessError } 5400101 - No memory. Returned by promise.
   * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
   * @systemapi
   * @since 11
   */
  function createAVImageGenerator(): Promise<AVImageGenerator>;

  /**
   * Creates an AVImageGenerator instance.
   * @param { AsyncCallback<AVImageGenerator> } callback - Callback used to return AVImageGenerator instance
   * if the operation is successful; returns null otherwise.
   * @throws { BusinessError } 5400101 - No memory. Returned by callback.
   * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
   * @systemapi
   * @since 11
   */
  function createAVImageGenerator(callback: AsyncCallback<AVImageGenerator>): void;

  /**
   * Fetch media meta data or audio art picture from source. Before calling an AVMetadataExtractor method,
   * you must use createAVMetadataExtractor() to create an AVMetadataExtractor instance.
   * @typedef AVMetadataExtractor
   * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
   * @since 11
   */
  interface AVMetadataExtractor {
    /**
     * Media file descriptor.
     * @type { ?AVFileDescriptor }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    fdSrc ?: AVFileDescriptor;

    /**
     * DataSource descriptor.
     * @type { ?DataSrcDescriptor }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    dataSrc ?: DataSrcDescriptor;

    /**
     * It will extract the resource to fetch media meta data info.
     * @param { AsyncCallback<AVMetadata> } callback - A callback instance used to return when fetchMetadata completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Returned by callback.
     * @throws { BusinessError } 5400106 - Unsupported format. Returned by callback.
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    fetchMetadata(callback: AsyncCallback<AVMetadata>): void;

    /**
     * It will extract the resource to fetch media meta data info.
     * @returns { Promise<AVMetadata> } A Promise instance used to return when fetchMetadata completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Returned by promise.
     * @throws { BusinessError } 5400106 - Unsupported format. Returned by promise.
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    fetchMetadata(): Promise<AVMetadata>;

    /**
     * It will extract the audio resource to fetch an album cover.
     * @param { AsyncCallback<image.PixelMap> } callback - A callback instance used
     * to return when fetchAlbumCover completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     * @throws { BusinessError } 5400106 - Unsupported format. Returned by callback.
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    fetchAlbumCover(callback: AsyncCallback<image.PixelMap>): void;

    /**
     * It will extract the audio resource to fetch an album cover.
     * @returns { Promise<image.PixelMap> } A Promise instance used to return when fetchAlbumCover completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Returned by promise.
     * @throws { BusinessError } 5400106 - Unsupported format. Returned by promise.
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    fetchAlbumCover(): Promise<image.PixelMap>;

    /**
     * Release resources used for AVMetadataExtractor.
     * @param { AsyncCallback<void> } callback - A callback instance used to return when release completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Returned by callback.
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    release(callback: AsyncCallback<void>): void;

    /**
     * Release resources used for AVMetadataExtractor.
     * @returns { Promise<void> } A Promise instance used to return when release completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Returned by promise.
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    release(): Promise<void>;
  }

  /**
   * Provides the container definition for media meta data.
   * @typedef AVMetadata
   * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
   * @since 11
   */
  interface AVMetadata {
    /**
     * The metadata to retrieve the information about the album title
     * of the media source.
     * @type { ?string }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    album?: string;

    /**
     * The metadata to retrieve the information about the performer or
     * artist associated with the media source.
     * @type { ?string }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    albumArtist?: string;

    /**
     * The metadata to retrieve the information about the artist of
     * the media source.
     * @type { ?string }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    artist?: string;

    /**
     * The metadata to retrieve the information about the author of
     * the media source.
     * @type { ?string }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    author?: string;

    /**
     * The metadata to retrieve the information about the created time of
     * the media source.
     * @type { ?string }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    dateTime?: string;

    /**
     * The metadata to retrieve the information about the created or modified time
     * with the specific date format of the media source.
     * @type { ?string }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    dateTimeFormat?: string;

    /**
     * The metadata to retrieve the information about the composer of
     * the media source.
     * @type { ?string }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    composer?: string;

    /**
     * The metadata to retrieve the playback duration of the media source.
     * @type { ?string }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    duration?: string;

    /**
     * The metadata to retrieve the content type or genre of the data
     * source.
     * @type { ?string }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    genre?: string;

    /**
     * If this value exists the media contains audio content.
     * @type { ?string }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    hasAudio?: string;

    /**
     * If this value exists the media contains video content.
     * @type { ?string }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    hasVideo?: string;

    /**
     * The metadata to retrieve the mime type of the media source. Some
     * example mime types include: "video/mp4", "audio/mp4", "audio/amr-wb",
     * @type { ?string }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    mimeType?: string;

    /**
     * The metadata to retrieve the number of tracks, such as audio, video,
     * text, in the media source, such as a mp4 or 3gpp file.
     * @type { ?string }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    trackCount?: string;

    /**
     * It is the audio sample rate, if available.
     * @type { ?string }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    sampleRate?: string;

    /**
     * The metadata to retrieve the media source title.
     * @type { ?string }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    title?: string;

    /**
     * If the media contains video, this key retrieves its height.
     * @type { ?string }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    videoHeight?: string;

    /**
     * If the media contains video, this key retrieves its width.
     * @type { ?string }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    videoWidth?: string;

    /**
     * The metadata to retrieve the information about the video
     * orientation.
     * @type { ?string }
     * @syscap SystemCapability.Multimedia.Media.AVMetadataExtractor
     * @since 11
     */
    videoOrientation?: string;
  }

  /**
   * Generate an image from a video resource with the specific time. Before calling an AVImageGenerator method,
   * you must use createAVImageGenerator() to create an AVImageGenerator instance.
   * @typedef AVImageGenerator
   * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
   * @systemapi
   * @since 11
   */
  interface AVImageGenerator {
    /**
     * Media file descriptor.
     * @type { ?AVFileDescriptor }
     * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
     * @systemapi
     * @since 11
     */
    fdSrc ?: AVFileDescriptor;

    /**
     * It will fetch a picture at @timeUs from the given video resource.
     * @param { number } timeUs - The time expected to fetch picture from the video resource.
     * The unit is microsecond(us).
     * @param { AVImageQueryOptions } options - The time options about the relationship
     * between the given timeUs and a key frame, see @AVImageQueryOptions .
     * @param { PixelMapParams } param - The output pixel map format params, see @PixelMapParams .
     * @param { AsyncCallback<image.PixelMap> } callback - A callback instance used
     * to return when fetchFrameByTime completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Returned by callback.
     * @throws { BusinessError } 5400106 - Unsupported format. Returned by callback.
     * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
     * @systemapi
     * @since 11
     */
    fetchFrameByTime(timeUs: number, options: AVImageQueryOptions, param: PixelMapParams,
      callback: AsyncCallback<image.PixelMap>): void;

    /**
     * It will decode the given video resource. Then fetch a picture
     * at @timeUs according the given @options and @param .
     * @param { number } timeUs - The time expected to fetch picture from the video resource.
     * The unit is microsecond(us).
     * @param { AVImageQueryOptions } options - The time options about the relationship
     * between the given timeUs and a key frame, see @AVImageQueryOptions .
     * @param { PixelMapParams } param - The output pixel map format params, see @PixelMapParams .
     * @returns { Promise<image.PixelMap> } A Promise instance used to return the pixel map
     * when fetchFrameByTime completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Returned by promise.
     * @throws { BusinessError } 5400106 - Unsupported format. Returned by promise.
     * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
     * @systemapi
     * @since 11
     */
    fetchFrameByTime(timeUs: number, options: AVImageQueryOptions, param: PixelMapParams): Promise<image.PixelMap>;

    /**
     * Release resources used for AVImageGenerator.
     * @param { AsyncCallback<void> } callback - A callback instance used to return when release completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Returned by callback.
     * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
     * @systemapi
     * @since 11
     */
    release(callback: AsyncCallback<void>): void;

    /**
     * Release resources used for AVImageGenerator.
     * @returns { Promise<void> } A Promise instance used to return when release completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Returned by promise.
     * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
     * @systemapi
     * @since 11
     */
    release(): Promise<void>;
  }

  /**
   * Enumerates options about the relationship between the given timeUs and a key frame.
   * @enum { number }
   * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
   * @systemapi
   * @since 11
   */
  enum AVImageQueryOptions {
    /**
     * This option is used to fetch a key frame from the given media
     * resource that is located right after or at the given time.
     * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
     * @systemapi
     * @since 11
     */
    AV_IMAGE_QUERY_NEXT_SYNC,

    /**
     * This option is used to fetch a key frame from the given media
     * resource that is located right before or at the given time.
     * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
     * @systemapi
     * @since 11
     */
    AV_IMAGE_QUERY_PREVIOUS_SYNC,

    /**
     * This option is used to fetch a key frame from the given media
     * resource that is located closest to or at the given time.
     * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
     * @systemapi
     * @since 11
     */
    AV_IMAGE_QUERY_CLOSEST_SYNC,

    /**
     * This option is used to fetch a frame (maybe not keyframe) from
     * the given media resource that is located closest to or at the given time.
     * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
     * @systemapi
     * @since 11
     */
    AV_IMAGE_QUERY_CLOSEST,
  }

  /**
   * Expected pixel map format for the fetched image from video resource.
   * @typedef PixelMapParams
   * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
   * @systemapi
   * @since 11
   */
  interface PixelMapParams {
    /**
     * Expected pixelmap's width, -1 means to keep consistent with the
     * original dimensions of the given video resource.
     * @type { ?number }
     * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
     * @systemapi
     * @since 11
     */
    width?: number;

    /**
     * Expected pixelmap's width, -1 means to keep consistent with the
     * original dimensions of the given video resource.
     * @type { ?number }
     * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
     * @systemapi
     * @since 11
     */
    height?: number;

    /**
     * Expected pixelmap's color format, see {@link PixelFormat}.
     * @type { ?PixelFormat }
     * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
     * @systemapi
     * @since 11
     */
    colorFormat?: PixelFormat;
  }

  /**
   * Enumerates options about the expected color options for the fetched image.
   * @enum { number }
   * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
   * @systemapi
   * @since 11
   */
  enum PixelFormat {
    /**
     * RGB_565 options.
     * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
     * @systemapi
     * @since 11
     */
    RGB_565 = 2,

    /**
     * RGBA_8888 options.
     * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
     * @systemapi
     * @since 11
     */
    RGBA_8888 = 3,

    /**
     * RGB_888 options.
     * @syscap SystemCapability.Multimedia.Media.AVImageGenerator
     * @systemapi
     * @since 11
     */
    RGB_888 = 5,
  }

  /**
   * Enumerates state change reason.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
   enum StateChangeReason {
    /**
     * State changed by user operation.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    USER = 1,

    /**
     * State changed by background action.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    BACKGROUND = 2,
  }

 /**
   * Enumerates ErrorCode types, return in BusinessError::code.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
 enum AVErrorCode {
  /**
   * Operation success.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_OK = 0,

  /**
   * Permission denied.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_NO_PERMISSION = 201,

  /**
   * Invalid parameter.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_INVALID_PARAMETER = 401,

  /**
   * The api is not supported in the current version.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_UNSUPPORT_CAPABILITY = 801,

  /**
   * The system memory is insufficient or the number of services reaches the upper limit.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_NO_MEMORY = 5400101,

  /**
   * Current status does not allow or do not have permission to perform this operation.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_OPERATE_NOT_PERMIT = 5400102,

  /**
   * Data flow exception information.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_IO = 5400103,

  /**
   * System or network response timeout.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_TIMEOUT = 5400104,

  /**
   * Service process died.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_SERVICE_DIED = 5400105,

  /**
   * Unsupported media format.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  AVERR_UNSUPPORT_FORMAT = 5400106,
 }

  enum VideoRecorderQualityLevel {
    /**
     * Quality level corresponding to the lowest available resolution.
     * @since 10
     */
    RECORDER_QUALITY_LOW = 0,

    /**
    * Quality level corresponding to the highest available resolution.
    * @since 10
    */
    RECORDER_QUALITY_HIGH = 1,

    /**
    * Quality level corresponding to the qcif (176 x 144) resolution.
    * @since 10
    */
    RECORDER_QUALITY_QCIF = 2,

    /**
    * Quality level corresponding to the cif (352 x 288) resolution.
    * @since 10
    */
    RECORDER_QUALITY_CIF = 3,

    /**
    * Quality level corresponding to the 480p (720 x 480) resolution.
    * @since 10
    */
    RECORDER_QUALITY_480P = 4,

    /**
    * Quality level corresponding to the 720P (1280 x 720) resolution.
    * @since 10
    */
    RECORDER_QUALITY_720P = 5,

    /**
    * Quality level corresponding to the 1080P (1920 x 1080) resolution.
    * @since 10
    */
    RECORDER_QUALITY_1080P = 6,

    /**
    * Quality level corresponding to the QVGA (320x240) resolution.
    * @since 10
    */
    RECORDER_QUALITY_QVGA = 7,

    /**
    * Quality level corresponding to the 2160p (3840x2160) resolution.
    * @since 10
    */
    RECORDER_QUALITY_2160P = 8,

    /**
    * Time lapse quality level corresponding to the lowest available resolution.
    * @since 10
    */
    RECORDER_QUALITY_TIME_LAPSE_LOW = 100,

    /**
    * Time lapse quality level corresponding to the highest available resolution.
    * @since 10
    */
    RECORDER_QUALITY_TIME_LAPSE_HIGH = 101,

    /**
    * Time lapse quality level corresponding to the qcif (176 x 144) resolution.
    * @since 10
    */
    RECORDER_QUALITY_TIME_LAPSE_QCIF = 102,

    /**
    * Time lapse quality level corresponding to the cif (352 x 288) resolution.
    * @since 10
    */
    RECORDER_QUALITY_TIME_LAPSE_CIF = 103,

    /**
    * Time lapse quality level corresponding to the 480p (720 x 480) resolution.
    * @since 10
    */
    RECORDER_QUALITY_TIME_LAPSE_480P = 104,

    /**
    * Time lapse quality level corresponding to the 720p (1280 x 720) resolution.
    * @since 10
    */
    RECORDER_QUALITY_TIME_LAPSE_720P = 105,

    /**
    * Time lapse quality level corresponding to the 1080p (1920 x 1088) resolution.
    * @since 10
    */
    RECORDER_QUALITY_TIME_LAPSE_1080P = 106,

    /**
    * Time lapse quality level corresponding to the QVGA (320 x 240) resolution.
    * @since 10
    */
    RECORDER_QUALITY_TIME_LAPSE_QVGA = 107,

    /**
    * Time lapse quality level corresponding to the 2160p (3840 x 2160) resolution.
    * @since 10
    */
    RECORDER_QUALITY_TIME_LAPSE_2160P = 108,

    /**
    * High speed ( >= 100fps) quality level corresponding to the lowest available resolution.
    * @since 10
    */
    RECORDER_QUALITY_HIGH_SPEED_LOW = 200,

    /**
    * High speed ( >= 100fps) quality level corresponding to the highest available resolution.
    * @since 10
    */
    RECORDER_QUALITY_HIGH_SPEED_HIGH = 201,

    /**
    * High speed ( >= 100fps) quality level corresponding to the 480p (720 x 480) resolution.
    * @since 10
    */
    RECORDER_QUALITY_HIGH_SPEED_480P = 202,

    /**
    * High speed ( >= 100fps) quality level corresponding to the 720p (1280 x 720) resolution.
    * @since 10
    */
    RECORDER_QUALITY_HIGH_SPEED_720P = 203,

    /**
    * High speed ( >= 100fps) quality level corresponding to the 1080p (1920 x 1080 or 1920x1088)
    * resolution.
    * @since 10
    */
    RECORDER_QUALITY_HIGH_SPEED_1080P = 204,
  }

  /**
   * Describes AVPlayer states.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.AVPlayer
   */
   type AVPlayerState = 'idle' | 'initialized' | 'prepared' | 'playing' | 'paused' | 'completed' | 'stopped' | 'released' | 'error';

   /**
    * Manages and plays media. Before calling an AVPlayer method, you must use createAVPlayer()
    * to create an AVPlayer instance.
    * @since 9
    * @syscap SystemCapability.Multimedia.Media.AVPlayer
    */
   /**
    * Manages and plays media. Before calling an AVPlayer method, you must use createAVPlayer()
    * to create an AVPlayer instance.
    * @since 11
    * @syscap SystemCapability.Multimedia.Media.AVPlayer
    * @atomicservice
    */
  interface AVPlayer {
    /**
     * Prepare audio/video playback, it will request resource for playing.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param callback A callback instance used to return when prepare completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     * @throws { BusinessError } 5400106 - Unsupport format. Return by callback.
     */
    /**
     * Prepare audio/video playback, it will request resource for playing.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param callback A callback instance used to return when prepare completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     * @throws { BusinessError } 5400106 - Unsupport format. Return by callback.
     * @atomicservice
     */
    prepare(callback: AsyncCallback<void>): void;

    /**
     * Prepare audio/video playback, it will request resource for playing.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @returns A Promise instance used to return when prepare completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by promise.
     * @throws { BusinessError } 5400106 - Unsupport format. Return by promise.
     */
    /**
     * Prepare audio/video playback, it will request resource for playing.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @returns A Promise instance used to return when prepare completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by promise.
     * @throws { BusinessError } 5400106 - Unsupport format. Return by promise.
     * @atomicservice
     */
    prepare(): Promise<void>;

    /**
     * Play audio/video playback.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param callback A callback instance used to return when play completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     */
    /**
     * Play audio/video playback.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param callback A callback instance used to return when play completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     * @atomicservice
     */
    play(callback: AsyncCallback<void>): void;

    /**
     * Play audio/video playback.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @returns A Promise instance used to return when play completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by promise.
     */
    /**
     * Play audio/video playback.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @returns A Promise instance used to return when play completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by promise.
     * @atomicservice
     */
    play(): Promise<void>;

    /**
     * Pause audio/video playback.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param callback A callback instance used to return when pause completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     */
    /**
     * Pause audio/video playback.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param callback A callback instance used to return when pause completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     * @atomicservice
     */
    pause(callback: AsyncCallback<void>): void;

    /**
     * Pause audio/video playback.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @returns A Promise instance used to return when pause completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by promise.
     */
    /**
     * Pause audio/video playback.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @returns A Promise instance used to return when pause completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by promise.
     * @atomicservice
     */
    pause(): Promise<void>;

    /**
     * Stop audio/video playback.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param callback A callback instance used to return when stop completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     */
    /**
     * Stop audio/video playback.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param callback A callback instance used to return when stop completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     * @atomicservice
     */
    stop(callback: AsyncCallback<void>): void;

     /**
      * Stop audio/video playback.
      * @since 9
      * @syscap SystemCapability.Multimedia.Media.AVPlayer
      * @returns A Promise instance used to return when stop completed.
      * @throws { BusinessError } 5400102 - Operation not allowed. Return by promise.
      */
     /**
      * Stop audio/video playback.
      * @since 11
      * @syscap SystemCapability.Multimedia.Media.AVPlayer
      * @returns A Promise instance used to return when stop completed.
      * @throws { BusinessError } 5400102 - Operation not allowed. Return by promise.
      * @atomicservice
      */
    stop(): Promise<void>;

    /**
     * Reset AVPlayer, it will to idle state and can set src again.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param callback A callback instance used to return when reset completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     */
    reset(callback: AsyncCallback<void>): void;

    /**
     * Reset AVPlayer, it will to idle state and can set src again.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @returns A Promise instance used to return when reset completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by promise.
     */
    reset(): Promise<void>;

    /**
     * Releases resources used for AVPlayer.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param callback A callback instance used to return when release completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     */
    /**
     * Releases resources used for AVPlayer.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param callback A callback instance used to return when release completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     * @atomicservice
     */
    release(callback: AsyncCallback<void>): void;

    /**
     * Releases resources used for AVPlayer.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @returns A Promise instance used to return when release completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by promise.
     */
    /**
     * Releases resources used for AVPlayer.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @returns A Promise instance used to return when release completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by promise.
     * @atomicservice
     */
    release(): Promise<void>;

    /**
     * Jumps to the specified playback position.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param timeMs Playback position to jump, should be in [0, duration].
     * @param mode See @SeekMode .
     */
    /**
     * Jumps to the specified playback position.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param timeMs Playback position to jump, should be in [0, duration].
     * @param mode See @SeekMode .
     * @atomicservice
     */
    seek(timeMs: number, mode?:SeekMode): void;

    /**
     * Sets the volume.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param volume Relative volume. The value ranges from 0.00 to 1.00. The value 1 indicates the maximum volume (100%).
     */
    setVolume(volume: number): void;

    /**
     * Get all track infos in MediaDescription, should be called after data loaded callback.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param callback Async callback return track info in MediaDescription.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     */
    /**
     * Get all track infos in MediaDescription, should be called after data loaded callback.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param callback Async callback return track info in MediaDescription.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     * @atomicservice
     */
    getTrackDescription(callback: AsyncCallback<Array<MediaDescription>>): void;

    /**
     * Get all track infos in MediaDescription, should be called after data loaded callback.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @returns A Promise instance used to return the track info in MediaDescription.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by promise.
     */
    /**
     * Get all track infos in MediaDescription, should be called after data loaded callback.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @returns A Promise instance used to return the track info in MediaDescription.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by promise.
     * @atomicservice
     */
    getTrackDescription() : Promise<Array<MediaDescription>>;

    /**
     * Media URI. Mainstream media formats are supported.
     * Network:http://xxx
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */

    /**
     * Set decryption session to codec module.
     * @param { drm.MediaKeySession } mediaKeySession - Handle of MediaKeySession to decrypt encrypted media.
     * @param { boolean } secureVideoPath - Secure video path required or not.
     * @throws { BusinessError } 401 - Invalid parameter.
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @since 11
     */
    setDecryptionConfig(mediaKeySession: drm.MediaKeySession, secureVideoPath: boolean): void;

    /**
     * Get media key system info from media source.
     * @returns { Array<drm.MediaKeySystemInfo> } MediaKeySystemInfo with PSSH.
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @since 11
     */
    getMediaKeySystemInfos(): Array<drm.MediaKeySystemInfo>;

    url ?: string;

    /**
     * Media file descriptor. Mainstream media formats are supported.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    /**
     * Media file descriptor. Mainstream media formats are supported.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @atomicservice
     */
    fdSrc ?: AVFileDescriptor;

    /**
     * DataSource descriptor. Mainstream media formats are supported.
     * @since 10
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    /**
     * DataSource descriptor. Mainstream media formats are supported.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @atomicservice
     */
    dataSrc ?: DataSrcDescriptor;

    /**
     * Whether to loop media playback.
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
     * Describes audio renderer info, refer to {@link #audio.AudioRendererInfo}. Set it before
     * calling the {@link #prepare()} in the first time in order for the audio renderer info to
     * become effective thereafter.
     * @since 10
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    audioRendererInfo ?: audio.AudioRendererInfo;

    /**
     * Obtains the current audio effect mode, refer to {@link #audio.AudioEffectMode}.
     * @since 10
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
     audioEffectMode ?: audio.AudioEffectMode;

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
     * Video player will use this id get a surface instance.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    /**
     * Video player will use this id get a surface instance.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @atomicservice
     */
    surfaceId ?: string;

    /**
     * Video width, valid after prepared.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    readonly width: number;

    /**
     * Video height, valid after prepared.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    readonly height: number;

    /**
     * Video scale type. By default, the {@link #VIDEO_SCALE_TYPE_FIT_CROP} will be used, for more
     * information, refer to {@link #VideoScaleType} .
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     */
    videoScaleType ?: VideoScaleType;

    /**
     * Set payback speed.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param speed playback speed, see @PlaybackSpeed .
     */
    setSpeed(speed: PlaybackSpeed): void;

    /**
     * select a specified bitrate to playback, only valid for HLS protocol network stream. By default, the
     * player will select the appropriate bitrate according to the network connection speed. The
     * available bitrate list reported by {@link #on('availableBitrates')}. Set it to select
     * a specified bitrate. If the specified bitrate is not in the list of available bitrate, the player
     * will select the minimal and closest one from the available bitrate list.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param bitrate the playback bitrate must be expressed in bits per second.
     */
    setBitrate(bitrate: number): void;

    /**
     * Register listens for mediaKeySystemInfoUpdate events.
     * @param { 'mediaKeySystemInfoUpdate' } type - Type of the event to listen for.
     * @param { function } callback - Callback used to listen for the mediaKeySystemInfoUpdate event.
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @since 11
     */
    on(type: 'mediaKeySystemInfoUpdate', callback: (mediaKeySystemInfo: Array<drm.MediaKeySystemInfo>) => void): void;

    /**
     * Unregister listens for mediaKeySystemInfoUpdate events.
     * @param { 'mediaKeySystemInfoUpdate' } type - Type of the event to listen for.
     * @param { function } callback - Callback for event.
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @since 11
     */
    off(type: 'mediaKeySystemInfoUpdate', callback?: (mediaKeySystemInfo: Array<drm.MediaKeySystemInfo>) => void): void;
    /**
     * Register or unregister listens for media playback events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback stateChange event.
     */
    /**
     * Register listens for media playback events.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback stateChange event.
     * @atomicservice
     */
    on(type: 'stateChange', callback: (state: AVPlayerState, reason: StateChangeReason) => void): void;
    /**
     * Unregister listens for media playback events.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @atomicservice
     */
    off(type: 'stateChange'): void;
    /**
     * Register or unregister listens for media playback events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback volume event.
     */
    on(type: 'volumeChange', callback: Callback<number>): void;
    off(type: 'volumeChange'): void;
    /**
     * Register or unregister listens for media playback events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback end of stream
     */
    on(type: 'endOfStream', callback: Callback<void>): void;
    off(type: 'endOfStream'): void;
    /**
     * Register or unregister  listens for media playback events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback seekDone event.
     */
    /**
     * Register listens for media playback events.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback seekDone event.
     * @atomicservice
     */
    on(type: 'seekDone', callback: Callback<number>): void;
    /**
     * Unregister listens for media playback events.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @atomicservice
     */
    off(type: 'seekDone'): void;
    /**
     * Register or unregister listens for media playback events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback speedDone event.
     */
    on(type: 'speedDone', callback: Callback<number>): void;
    off(type: 'speedDone'): void;
    /**
     * Register or unregister listens for media playback events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback setBitrateDone event.
     */
    on(type: 'bitrateDone', callback: Callback<number>): void;
    off(type: 'bitrateDone'): void;
    /**
     * Register or unregister listens for media playback events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback timeUpdate event.
     */
    /**
     * Register listens for media playback events.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback timeUpdate event.
     * @atomicservice
     */
    on(type: 'timeUpdate', callback: Callback<number>): void;
    /**
     * Unregister listens for media playback events.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback timeUpdate event.
     * @atomicservice
     */
    off(type: 'timeUpdate'): void;
    /**
     * Register or unregister listens for media playback events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback durationUpdate event.
     */
    on(type: 'durationUpdate', callback: Callback<number>): void;
    off(type: 'durationUpdate'): void;
    /**
     * Register or unregister listens for video playback buffering events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback buffering update event to listen for.
     * @param callback Callback used to listen for the buffering update event, return BufferingInfoType and the value.
     */
    on(type: 'bufferingUpdate', callback: (infoType: BufferingInfoType, value: number) => void): void;
    off(type: 'bufferingUpdate'): void;
    /**
     * Register or unregister listens for start render video frame events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return .
     */
    on(type: 'startRenderFrame', callback: Callback<void>): void;
    off(type: 'startRenderFrame'): void;
    /**
     * Register or unregister listens for video size change event.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return video size.
     */
    on(type: 'videoSizeChange', callback: (width: number, height: number) => void): void;
    off(type: 'videoSizeChange'): void;
    /**
     * Register or unregister listens for audio interrupt event, refer to {@link #audio.InterruptEvent}
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return audio interrupt info.
     */
    on(type: 'audioInterrupt', callback: (info: audio.InterruptEvent) => void): void;
    off(type: 'audioInterrupt'): void;
    /**
     * Register or unregister listens for available bitrate list collect completed events for HLS protocol stream playback.
     * This event will be reported after the {@link #prepare} called.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return available bitrate list.
     */
    on(type: 'availableBitrates', callback: (bitrates: Array<number>) => void): void;
    off(type: 'availableBitrates'): void;
    /**
     * Register or unregister listens for playback error events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback error event to listen for.
     * @param callback Callback used to listen for the playback error event.
     * @throws { BusinessError } 201 - Permission denied.
     * @throws { BusinessError } 401 - The parameter check failed.
     * @throws { BusinessError } 801 - Capability not supported.
     * @throws { BusinessError } 5400101 - No memory.
     * @throws { BusinessError } 5400102 - Operation not allowed.
     * @throws { BusinessError } 5400103 - I/O error.
     * @throws { BusinessError } 5400104 - Time out.
     * @throws { BusinessError } 5400105 - Service died.
     * @throws { BusinessError } 5400106 - Unsupport format.
     */
    /**
     * Register listens for playback error events.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback error event to listen for.
     * @param callback Callback used to listen for the playback error event.
     * @throws { BusinessError } 201 - Permission denied.
     * @throws { BusinessError } 401 - The parameter check failed.
     * @throws { BusinessError } 801 - Capability not supported.
     * @throws { BusinessError } 5400101 - No memory.
     * @throws { BusinessError } 5400102 - Operation not allowed.
     * @throws { BusinessError } 5400103 - I/O error.
     * @throws { BusinessError } 5400104 - Time out.
     * @throws { BusinessError } 5400105 - Service died.
     * @throws { BusinessError } 5400106 - Unsupport format.
     * @atomicservice
     */
    on(type: 'error', callback: ErrorCallback): void;
    /**
     * Unregister listens for playback error events.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.AVPlayer
     * @param type Type of the playback error event to listen for.
     * @atomicservice
     */
    off(type: 'error'): void;
  }

  /**
   * Enumerates ErrorCode types, return in BusinessError::code
   * @since 8
   * @syscap SystemCapability.Multimedia.Media.Core
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
     * an I/O error occurred.
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

  /**
   * Media file descriptor. The caller needs to ensure that the fd is valid and
   * the offset and length are correct.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  interface AVFileDescriptor {
    /**
     * The file descriptor of audio or video source from file system. The caller
     * is responsible to close the file descriptor.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    fd: number

    /**
     * The offset into the file where the data to be read, in bytes. By default,
     * the offset is zero.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    offset?: number

    /**
     * The length in bytes of the data to be read. By default, the length is the
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
   * @deprecated since 9
   * @useinstead ohos.multimedia.media/media.AVPlayerState
   */
  type AudioState = 'idle' | 'playing' | 'paused' | 'stopped' | 'error';

  /**
   * Manages and plays audio. Before calling an AudioPlayer method, you must use createAudioPlayer()
   * to create an AudioPlayer instance.
   * @since 6
   * @syscap SystemCapability.Multimedia.Media.AudioPlayer
   * @deprecated since 9
   * @useinstead ohos.multimedia.media/media.AVPlayer
   */
  interface AudioPlayer {
    /**
     * Starts audio playback.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#play
     */
    play(): void;

    /**
     * Pauses audio playback.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#pause
     */
    pause(): void;

    /**
     * Stops audio playback.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#stop
     */
    stop(): void;

    /**
     * Resets audio playback.
     * @since 7
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#reset
     */
    reset(): void;

    /**
     * Jumps to the specified playback position.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @param timeMs Playback position to jump
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#seek
     */
    seek(timeMs: number): void;

    /**
     * Sets the volume.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @param vol Relative volume. The value ranges from 0.00 to 1.00. The value 1 indicates the maximum volume (100%).
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#setVolume
     */
    setVolume(vol: number): void;

    /**
     * Releases resources used for audio playback.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#release
     */
    release(): void;

    /**
     * Get all track infos in MediaDescription, should be called after data loaded callback.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @param callback async callback return track info in MediaDescription.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#getTrackDescription
     */
    getTrackDescription(callback: AsyncCallback<Array<MediaDescription>>): void;

    /**
     * Get all track infos in MediaDescription, should be called after data loaded callback.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @returns A Promise instance used to return the track info in MediaDescription.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#getTrackDescription
     */
    getTrackDescription() : Promise<Array<MediaDescription>>;

    /**
     * Listens for audio playback buffering events.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @param type Type of the playback buffering update event to listen for.
     * @param callback Callback used to listen for the buffering update event, return BufferingInfoType and the value.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#event:bufferingUpdate
     */
    on(type: 'bufferingUpdate', callback: (infoType: BufferingInfoType, value: number) => void): void;

    /**
     * Audio media URI. Mainstream audio formats are supported.
     * local:fd://XXX, file://XXX. network:http://xxx
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @permission ohos.permission.READ_MEDIA or ohos.permission.INTERNET
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#url
     */
    src: string;

    /**
     * Audio file descriptor. Mainstream audio formats are supported.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#fdSrc
     */
    fdSrc: AVFileDescriptor;

    /**
     * Whether to loop audio playback. The value true means to loop playback.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#loop
     */
    loop: boolean;

    /**
     * Describes audio interrupt mode, refer to {@link #audio.InterruptMode}. If it is not
     * set, the default mode will be used. Set it before calling the {@link #play()} in the
     * first time in order for the interrupt mode to become effective thereafter.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#audioInterruptMode
     */
    audioInterruptMode ?: audio.InterruptMode;

    /**
     * Current playback position.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#currentTime
     */
    readonly currentTime: number;

    /**
     * Playback duration, When the data source does not support seek, it returns - 1, such as a live broadcast scenario.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#duration
     */
    readonly duration: number;

    /**
     * Playback state.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#state
     */
    readonly state: AudioState;

    /**
     * Listens for audio playback events.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#event:stateChange
     */
    on(type: 'play' | 'pause' | 'stop' | 'reset' | 'dataLoad' | 'finish' | 'volumeChange', callback: () => void): void;

    /**
     * Listens for audio playback events.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#event:timeUpdate
     */
    on(type: 'timeUpdate', callback: Callback<number>): void;

    /**
     * Listens for audio interrupt event, refer to {@link #audio.InterruptEvent}
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return audio interrupt info.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#event:audioInterrupt
     */
    on(type: 'audioInterrupt', callback: (info: audio.InterruptEvent) => void): void;

    /**
     * Listens for playback error events.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioPlayer
     * @param type Type of the playback error event to listen for.
     * @param callback Callback used to listen for the playback error event.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#event:error
     */
    on(type: 'error', callback: ErrorCallback): void;
  }

  /**
  * Describes media recorder states.
  * @since 9
  * @syscap SystemCapability.Multimedia.Media.AVRecorder
  */
  type AVRecorderState = 'idle' | 'prepared' | 'started' | 'paused' | 'stopped' | 'released' | 'error';

  /**
   * Manages and record audio/video. Before calling an AVRecorder method, you must use createAVRecorder()
   * to create an AVRecorder instance.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.AVRecorder
   */
  interface AVRecorder {
    /**
     * Prepares for recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @param config Recording parameters.
     * @param callback A callback instance used to return when prepare completed.
     * @permission ohos.permission.MICROPHONE
     * @throws { BusinessError } 201 - Permission denied. Return by callback.
     * @throws { BusinessError } 401 - Parameter error. Return by callback.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     * @throws { BusinessError } 5400106 - Unsupport format. Return by callback.
     */
    prepare(config: AVRecorderConfig, callback: AsyncCallback<void>): void;

    /**
     * Prepares for recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @param config Recording parameters.
     * @returns A Promise instance used to return when prepare completed.
     * @permission ohos.permission.MICROPHONE
     * @throws { BusinessError } 201 - Permission denied. Return by promise.
     * @throws { BusinessError } 401 - Parameter error. Return by promise.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by promise.
     * @throws { BusinessError } 5400105 - Service died. Return by promise.
     * @throws { BusinessError } 5400106 - Unsupport format. Return by promise.
     */
    prepare(config: AVRecorderConfig): Promise<void>;

    /**
     * Get input surface.it must be called between prepare completed and start.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @param callback Callback used to return the input surface id in string.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by callback.
     * @throws { BusinessError } 5400103 - IO error. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     */
    getInputSurface(callback: AsyncCallback<string>): void;

    /**
     * Get input surface. it must be called between prepare completed and start.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @returns A Promise instance used to return the input surface id in string.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by promise.
     * @throws { BusinessError } 5400103 - IO error. Return by promise.
     * @throws { BusinessError } 5400105 - Service died. Return by promise.
     */
    getInputSurface(): Promise<string>;
    
    /**
     * Start AVRecorder, it will to started state.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @param callback A callback instance used to return when start completed.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by callback.
     * @throws { BusinessError } 5400103 - IO error. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     */
    start(callback: AsyncCallback<void>): void;

    /**
     * Start AVRecorder, it will to started state.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @returns A Promise instance used to return when start completed.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by promise.
     * @throws { BusinessError } 5400103 - IO error. Return by promise.
     * @throws { BusinessError } 5400105 - Service died. Return by promise.
     */
    start(): Promise<void>;

    /**
     * Start AVRecorder, it will to paused state.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @param callback A callback instance used to return when pause completed.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by callback.
     * @throws { BusinessError } 5400103 - IO error. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     */
    pause(callback: AsyncCallback<void>): void;

    /**
     * Start AVRecorder, it will to paused state.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @returns A Promise instance used to return when pause completed.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by promise.
     * @throws { BusinessError } 5400103 - IO error. Return by promise.
     * @throws { BusinessError } 5400105 - Service died. Return by promise.
     */
    pause(): Promise<void>;

    /**
     * Resume AVRecorder, it will to started state.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @param callback A callback instance used to return when resume completed.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by callback.
     * @throws { BusinessError } 5400103 - IO error. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     */
    resume(callback: AsyncCallback<void>): void;

    /**
     * Resume AVRecorder, it will to started state.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @returns A Promise instance used to return when resume completed.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by promise.
     * @throws { BusinessError } 5400103 - IO error. Return by promise.
     * @throws { BusinessError } 5400105 - Service died. Return by promise.
     */
    resume(): Promise<void>;

    /**
     * Stop AVRecorder, it will to stopped state.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @param callback A callback instance used to return when stop completed.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by callback.
     * @throws { BusinessError } 5400103 - IO error. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     */
    stop(callback: AsyncCallback<void>): void;

    /**
     * Stop AVRecorder, it will to stopped state.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @returns A Promise instance used to return when stop completed.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by promise.
     * @throws { BusinessError } 5400103 - IO error. Return by promise.
     * @throws { BusinessError } 5400105 - Service died. Return by promise.
     */
    stop(): Promise<void>;

    /**
     * Reset AVRecorder, it will to idle state.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @param callback A callback instance used to return when reset completed.
     * @throws { BusinessError } 5400103 - IO error. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     */
    reset(callback: AsyncCallback<void>): void;

    /**
     * Reset AVRecorder, it will to idle state.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @returns A Promise instance used to return when reset completed.
     * @throws { BusinessError } 5400103 - IO error. Return by promise.
     * @throws { BusinessError } 5400105 - Service died. Return by promise.
     */
    reset(): Promise<void>;

    /**
     * Releases resources used for AVRecorder, it will to released state.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @param callback A callback instance used to return when release completed.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     */
    release(callback: AsyncCallback<void>): void;

    /**
     * Releases resources used for AVRecorder, it will to released state.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @returns A Promise instance used to return when release completed.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     */
    release(): Promise<void>;

    /**
     * getAVRecorderProfile for recording.
     * @since 10
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @param config Recording parameters.
     * @param callback A callback instance used to return when prepare completed.
     * @throws { BusinessError } 401 - Parameter error. Return by callback.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by callback.
     */
    getAVRecorderProfile(sourceId: number, qualityLevel: VideoRecorderQualityLevel): Promise<AVRecorderProfile>;

    /**
     * getAVRecorderProfile for recording.
     * @since 10
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @param config Recording parameters.
     * @param callback A callback instance used to return when prepare completed.
     * @throws { BusinessError } 401 - Parameter error. Return by callback.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by callback.
     */
    getAVRecorderProfile(sourceId: number, qualityLevel: VideoRecorderQualityLevel, callback: AsyncCallback<AVRecorderProfile>);

    /**
     * setAVRecorderConfig for recording.
     * @since 10
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @param config Recording parameters.
     * @param callback A callback instance used to return when prepare completed.
     * @permission ohos.permission.MICROPHONE
     * @throws { BusinessError } 401 - Parameter error. Return by callback.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by callback.
     */
    setAVRecorderConfig(config: AVRecorderConfig, callback: AsyncCallback<void>): void;

    /**
     * setAVRecorderConfig for recording.
     * @since 10
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @param config Recording parameters.
     * @returns A Promise instance used to return when prepare completed.
     * @permission ohos.permission.MICROPHONE
     * @throws { BusinessError } 401 - Parameter error. Return by promise.
     * @throws { BusinessError } 5400102 - Operate not permit. Return by promise.
     */
    setAVRecorderConfig(config: AVRecorderConfig): Promise<void>;

    /**
     * Recorder state.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    readonly state: AVRecorderState;

    /**
     * Listens for recording stateChange events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @param type Type of the recording event to listen for.
     * @param callback Callback used to listen for the recorder stateChange event.
     * @throws { BusinessError } 5400103 - IO error. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     */
    on(type: 'stateChange', callback: (state: AVRecorderState, reason: StateChangeReason) => void): void;

    /**
     * Listens for recording error events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @param type Type of the recording error event to listen for.
     * @param callback Callback used to listen for the recorder error event.
     * @throws { BusinessError } 201 - Permission denied.
     * @throws { BusinessError } 401 - The parameter check failed.
     * @throws { BusinessError } 801 - Capability not supported.
     * @throws { BusinessError } 5400101 - No memory.
     * @throws { BusinessError } 5400102 - Operation not allowed.
     * @throws { BusinessError } 5400103 - I/O error.
     * @throws { BusinessError } 5400104 - Time out.
     * @throws { BusinessError } 5400105 - Service died.
     * @throws { BusinessError } 5400106 - Unsupport format.
     */
    on(type: 'error', callback: ErrorCallback): void;

    /**
     * Cancel Listens for recording stateChange events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @param type Type of the recording stateChange event to listen for.
     */
    off(type: 'stateChange'): void;

    /**
     * Cancel Listens for recording error events.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     * @param type Type of the recording error event to listen for.
     */
    off(type: 'error'): void;
  }

  /**
   * Enumerates audio encoding formats, it will be deprecated after API8, use @CodecMimeType to replace.
   * @since 6
   * @syscap SystemCapability.Multimedia.Media.AudioRecorder
   * @deprecated since 8
   * @useinstead ohos.multimedia.media/media.CodecMimeType
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
   * @deprecated since 8
   * @useinstead ohos.multimedia.media/media.ContainerFormatType
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
   * @deprecated since 9
   * @useinstead ohos.multimedia.media/media.AVRecorderConfig
   */
  interface AudioRecorderConfig {
    /**
     * Audio encoding format. The default value is DEFAULT, it will be deprecated after API8.
     * use "audioEncoderMime" instead.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @deprecated since 8
     * @useinstead ohos.multimedia.media/media.AudioRecorderConfig.audioEncoderMime
     */
    audioEncoder?: AudioEncoder;

    /**
     * Audio encoding bit rate.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @deprecated since 9
     */
    audioEncodeBitRate?: number;

    /**
     * Audio sampling rate.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @deprecated since 9
     */
    audioSampleRate?: number;

    /**
     * Number of audio channels.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @deprecated since 9
     */
    numberOfChannels?: number;

    /**
     * Audio output format. The default value is DEFAULT, it will be deprecated after API8.
     * it will be replaced with "fileFormat".
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @deprecated since 8
     * @useinstead ohos.multimedia.media/media.AudioRecorderConfig.fileFormat
     */
    format?: AudioOutputFormat;

    /**
     * Audio output uri.support two kind of uri now.
     * format like: scheme + "://" + "context".
     * file:  file://path
     * fd:    fd://fd
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @deprecated since 9
     */
    uri: string;

    /**
     * Geographical location information.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @deprecated since 9
     */
    location?: Location;

    /**
     * audio encoding format MIME. it used to replace audioEncoder.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @deprecated since 9
     */
    audioEncoderMime?: CodecMimeType;
    /**
     * output file format. see @ContainerFormatType , it used to replace "format".
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @deprecated since 9
     */
    fileFormat?: ContainerFormatType;
  }

  /**
   * Manages and record audio. Before calling an AudioRecorder method, you must use createAudioRecorder()
   * to create an AudioRecorder instance.
   * @since 6
   * @syscap SystemCapability.Multimedia.Media.AudioRecorder
   * @deprecated since 9
   * @useinstead ohos.multimedia.media/media.AVRecorder
   */
  interface AudioRecorder {
    /**
     * Prepares for recording.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @param config Recording parameters.
     * @permission ohos.permission.MICROPHONE
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVRecorder#prepare
     */
    prepare(config: AudioRecorderConfig): void;

    /**
     * Starts audio recording.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVRecorder#start
     */
    start(): void;

    /**
     * Pauses audio recording.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVRecorder#pause
     */
    pause(): void;

    /**
     * Resumes audio recording.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVRecorder#resume
     */
    resume(): void;

    /**
     * Stops audio recording.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVRecorder#stop
     */
    stop(): void;

    /**
     * Releases resources used for audio recording.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVRecorder#release
     */
    release(): void;

    /**
     * Resets audio recording.
     * Before resetting audio recording, you must call stop() to stop recording. After audio recording is reset,
     * you must call prepare() to set the recording configurations for another recording.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVRecorder#reset
     */
    reset(): void;

    /**
     * Listens for audio recording events.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @param type Type of the audio recording event to listen for.
     * @param callback Callback used to listen for the audio recording event.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVRecorder#on
     */
    on(type: 'prepare' | 'start' | 'pause' | 'resume' | 'stop' | 'release' | 'reset', callback: () => void): void;

    /**
     * Listens for audio recording error events.
     * @since 6
     * @syscap SystemCapability.Multimedia.Media.AudioRecorder
     * @param type Type of the audio recording error event to listen for.
     * @param callback Callback used to listen for the audio recording error event.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVRecorder#on
     */
    on(type: 'error', callback: ErrorCallback): void;
  }

  /**
  * The maintenance of this interface has been stopped since version api 9. Please use AVRecorderState.
  * Describes video recorder states.
  * @since 9
  * @syscap SystemCapability.Multimedia.Media.VideoRecorder
  * @systemapi
  */
  type VideoRecordState = 'idle' | 'prepared' | 'playing' | 'paused' | 'stopped' | 'error';

  /**
   * The maintenance of this interface has been stopped since version api 9. Please use AVRecorder.
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
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     * @systemapi
     */
    prepare(config: VideoRecorderConfig, callback: AsyncCallback<void>): void;
    /**
     * Prepares for recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @param config Recording parameters.
     * @returns A Promise instance used to return when prepare completed.
     * @permission ohos.permission.MICROPHONE
     * @throws { BusinessError } 201 - Permission denied. Return by promise.
     * @throws { BusinessError } 401 - Parameter error. Return by promise.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by promise.
     * @throws { BusinessError } 5400105 - Service died. Return by promise.
     * @systemapi
     */
    prepare(config: VideoRecorderConfig): Promise<void>;
    /**
     * get input surface.it must be called between prepare completed and start.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @param callback Callback used to return the input surface id in string.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     * @throws { BusinessError } 5400103 - I/O error. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     * @systemapi
     */
    getInputSurface(callback: AsyncCallback<string>): void;
    /**
     * get input surface. it must be called between prepare completed and start.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @returns A Promise instance used to return the input surface id in string.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by promise.
     * @throws { BusinessError } 5400103 - I/O error. Return by promise.
     * @throws { BusinessError } 5400105 - Service died. Return by promise.
     * @systemapi
     */
    getInputSurface(): Promise<string>;
    /**
     * Starts video recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @param callback A callback instance used to return when start completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     * @throws { BusinessError } 5400103 - I/O error. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     * @systemapi
     */
    start(callback: AsyncCallback<void>): void;
    /**
     * Starts video recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @returns A Promise instance used to return when start completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by promise.
     * @throws { BusinessError } 5400103 - I/O error. Return by promise.
     * @throws { BusinessError } 5400105 - Service died. Return by promise.
     * @systemapi
     */
    start(): Promise<void>;
    /**
     * Pauses video recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @param callback A callback instance used to return when pause completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     * @throws { BusinessError } 5400103 - I/O error. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     * @systemapi
     */
    pause(callback: AsyncCallback<void>): void;
    /**
     * Pauses video recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @returns A Promise instance used to return when pause completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by promise.
     * @throws { BusinessError } 5400103 - I/O error. Return by promise.
     * @throws { BusinessError } 5400105 - Service died. Return by promise.
     * @systemapi
     */
    pause(): Promise<void>;
    /**
     * Resumes video recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @param callback A callback instance used to return when resume completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     * @throws { BusinessError } 5400103 - I/O error. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     * @systemapi
     */
    resume(callback: AsyncCallback<void>): void;
    /**
     * Resumes video recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @returns A Promise instance used to return when resume completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by promise.
     * @throws { BusinessError } 5400103 - I/O error. Return by promise.
     * @throws { BusinessError } 5400105 - Service died. Return by promise.
     * @systemapi
     */
    resume(): Promise<void>;
    /**
     * Stops video recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @param callback A callback instance used to return when stop completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by callback.
     * @throws { BusinessError } 5400103 - I/O error. Return by callback.
     * @throws { BusinessError } 5400105 - Service died. Return by callback.
     * @systemapi
     */
    stop(callback: AsyncCallback<void>): void;
    /**
     * Stops video recording.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoRecorder
     * @returns A Promise instance used to return when stop completed.
     * @throws { BusinessError } 5400102 - Operation not allowed. Return by promise.
     * @throws { BusinessError } 5400103 - I/O error. Return by promise.
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
      * @returns A Promise instance used to return when release completed.
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
     * @throws { BusinessError } 5400103 - I/O error. Return by callback.
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
      * @returns A Promise instance used to return when reset completed.
      * @throws { BusinessError } 5400103 - I/O error. Return by promise.
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
     * @throws { BusinessError } 5400103 - I/O error. Return by callback.
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
   * @deprecated since 9
   * @useinstead ohos.multimedia.media/media.AVPlayerState
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
   * @deprecated since 9
   * @useinstead ohos.multimedia.media/media.AVPlayer
   */
  interface VideoPlayer {
    /**
     * Set display surface.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param surfaceId surface id, video player will use this id get a surface instance.
     * @returns A Promise instance used to return when release output buffer completed.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#surfaceId
     */
    setDisplaySurface(surfaceId: string, callback: AsyncCallback<void>): void;
    /**
     * Set display surface.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param surfaceId surface id, video player will use this id get a surface instance.
     * @returns A Promise instance used to return when release output buffer completed.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#surfaceId
     */
    setDisplaySurface(surfaceId: string): Promise<void>;
    /**
     * Prepare video playback, it will request resource for playing.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param callback A callback instance used to return when prepare completed.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#prepare
     */
    prepare(callback: AsyncCallback<void>): void;
    /**
     * Prepare video playback, it will request resource for playing.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @returns A Promise instance used to return when prepare completed.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#prepare
     */
    prepare(): Promise<void>;
    /**
     * Starts video playback.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param callback A callback instance used to return when start completed.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#play
     */
    play(callback: AsyncCallback<void>): void;
    /**
     * Starts video playback.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @returns A Promise instance used to return when start completed.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#play
     */
    play(): Promise<void>;
    /**
     * Pauses video playback.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param callback A callback instance used to return when pause completed.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#pause
     */
    pause(callback: AsyncCallback<void>): void;
    /**
     * Pauses video playback.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @returns A Promise instance used to return when pause completed.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#pause
     */
    pause(): Promise<void>;
    /**
     * Stops video playback.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param callback A callback instance used to return when stop completed.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#stop
     */
    stop(callback: AsyncCallback<void>): void;
    /**
     * Stops video playback.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @returns A Promise instance used to return when stop completed.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#stop
     */
    stop(): Promise<void>;
    /**
     * Resets video playback, it will release the resource.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param callback A callback instance used to return when reset completed.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#reset
     */
    reset(callback: AsyncCallback<void>): void;
    /**
     * Resets video playback, it will release the resource.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @returns A Promise instance used to return when reset completed.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#reset
     */
    reset(): Promise<void>;
    /**
     * Jumps to the specified playback position by default SeekMode(SEEK_PREV_SYNC),
     * the performance may be not the best.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param timeMs Playback position to jump
     * @param callback A callback instance used to return when seek completed
     * and return the seeking position result.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#seek
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
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#seek
     */
    seek(timeMs: number, mode:SeekMode, callback: AsyncCallback<number>): void;
    /**
     * Jumps to the specified playback position.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param timeMs Playback position to jump
     * @param mode seek mode, see @SeekMode .
     * @returns A Promise instance used to return when seek completed
     * and return the seeking position result.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#seek
     */
    seek(timeMs: number, mode?:SeekMode): Promise<number>;
    /**
     * Sets the volume.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param vol Relative volume. The value ranges from 0.00 to 1.00. The value 1 indicates the maximum volume (100%).
     * @param callback A callback instance used to return when set volume completed.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#setVolume
     */
    setVolume(vol: number, callback: AsyncCallback<void>): void;
    /**
     * Sets the volume.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param vol Relative volume. The value ranges from 0.00 to 1.00. The value 1 indicates the maximum volume (100%).
     * @returns A Promise instance used to return when set volume completed.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#setVolume
     */
    setVolume(vol: number): Promise<void>;
    /**
     * Releases resources used for video playback.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param callback A callback instance used to return when release completed.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#release
     */
    release(callback: AsyncCallback<void>): void;
    /**
     * Releases resources used for video playback.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @returns A Promise instance used to return when release completed.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#release
     */
    release(): Promise<void>;
    /**
     * Get all track infos in MediaDescription, should be called after data loaded callback.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param callback async callback return track info in MediaDescription.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#getTrackDescription
     */
    getTrackDescription(callback: AsyncCallback<Array<MediaDescription>>): void;

    /**
     * Get all track infos in MediaDescription, should be called after data loaded callback.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @returns A Promise instance used to return the track info in MediaDescription.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#getTrackDescription
     */
    getTrackDescription() : Promise<Array<MediaDescription>>;

    /**
     * media url. Mainstream video formats are supported.
     * local:fd://XXX, file://XXX. network:http://xxx
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#url
     */
    url: string;

    /**
     * Video file descriptor. Mainstream video formats are supported.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#fdSrc
     */
    fdSrc: AVFileDescriptor;

    /**
     * Whether to loop video playback. The value true means to loop playback.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#loop
     */
    loop: boolean;

    /**
     * Current playback position.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#currentTime
     */
    readonly currentTime: number;

    /**
     * Playback duration, if -1 means cannot seek.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#duration
     */
    readonly duration: number;

    /**
     * Playback state.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#state
     */
    readonly state: VideoPlayState;

    /**
     * video width, valid after prepared.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#width
     */
    readonly width: number;

    /**
     * video height, valid after prepared.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#height
     */
    readonly height: number;

    /**
     * Describes audio interrupt mode, refer to {@link #audio.InterruptMode}. If it is not
     * set, the default mode will be used. Set it before calling the {@link #play()} in the
     * first time in order for the interrupt mode to become effective thereafter.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#audioInterruptMode
     */
    audioInterruptMode ?: audio.InterruptMode;

    /**
     * video scale type. By default, the {@link #VIDEO_SCALE_TYPE_FIT_CROP} will be used, for more
     * information, refer to {@link #VideoScaleType}
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#videoScaleType
     */
    videoScaleType ?: VideoScaleType;

    /**
     * set payback speed.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param speed playback speed, see @PlaybackSpeed .
     * @param callback Callback used to return actually speed.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#setSpeed
     */
    setSpeed(speed:number, callback: AsyncCallback<number>): void;
    /**
     * set output surface.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param speed playback speed, see @PlaybackSpeed .
     * @returns A Promise instance used to return actually speed.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#setSpeed
     */
    setSpeed(speed:number): Promise<number>;

    /**
     * Listens for video playback completed events.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return .
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#event:stateChange
     */
    on(type: 'playbackCompleted', callback: Callback<void>): void;

    /**
     * Listens for video playback buffering events.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param type Type of the playback buffering update event to listen for.
     * @param callback Callback used to listen for the buffering update event, return BufferingInfoType and the value.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#event:bufferingUpdate
     */
    on(type: 'bufferingUpdate', callback: (infoType: BufferingInfoType, value: number) => void): void;

    /**
     * Listens for start render video frame events.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#event:startRenderFrame
     */
    on(type: 'startRenderFrame', callback: Callback<void>): void;

    /**
     * Listens for video size changed event.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return video size.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#event:videoSizeChange
     */
    on(type: 'videoSizeChanged', callback: (width: number, height: number) => void): void;

    /**
     * Listens for audio interrupt event, refer to {@link #audio.InterruptEvent}
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return audio interrupt info.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#event:audioInterrupt
     */
    on(type: 'audioInterrupt', callback: (info: audio.InterruptEvent) => void): void;

    /**
     * Listens for playback error events.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.VideoPlayer
     * @param type Type of the playback error event to listen for.
     * @param callback Callback used to listen for the playback error event.
     * @deprecated since 9
     * @useinstead ohos.multimedia.media/media.AVPlayer#event:error
     */
    on(type: 'error', callback: ErrorCallback): void;
  }

  /**
   * Enumerates video scale type.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.VideoPlayer
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
   */
  /**
   * Enumerates media description key.
   * @since 11
   * @syscap SystemCapability.Multimedia.Media.Core
   * @atomicservice
   */
  enum MediaDescriptionKey {
    /**
     * key for track index, value type is number.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    /**
     * key for track index, value type is number.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.Core
     * @atomicservice
     */
    MD_KEY_TRACK_INDEX = "track_index",

    /**
     * key for track type, value type is number, see @MediaType.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    /**
     * key for track type, value type is number, see @MediaType.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.Core
     * @atomicservice
     */
    MD_KEY_TRACK_TYPE = "track_type",

    /**
     * key for codec mime type, value type is string.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    /**
     * key for codec mime type, value type is string.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.Core
     * @atomicservice
     */
    MD_KEY_CODEC_MIME = "codec_mime",

    /**
     * key for duration, value type is number.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    /**
     * key for duration, value type is number.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.Core
     * @atomicservice
     */
    MD_KEY_DURATION = "duration",

    /**
     * key for bitrate, value type is number.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    /**
     * key for bitrate, value type is number.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.Core
     * @atomicservice
     */
    MD_KEY_BITRATE = "bitrate",

    /**
     * key for video width, value type is number.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    /**
     * key for video width, value type is number.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.Core
     * @atomicservice
     */
    MD_KEY_WIDTH = "width",

    /**
     * key for video height, value type is number.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    /**
     * key for video height, value type is number.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.Core
     * @atomicservice
     */
    MD_KEY_HEIGHT = "height",

    /**
     * key for video frame rate, value type is number.
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    /**
     * key for video frame rate, value type is number.
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.Core
     * @atomicservice
     */
    MD_KEY_FRAME_RATE = "frame_rate",

    /**
     * key for audio channel count, value type is number
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    /**
     * key for audio channel count, value type is number
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.Core
     * @atomicservice
     */
    MD_KEY_AUD_CHANNEL_COUNT = "channel_count",

    /**
     * key for audio sample rate, value type is number
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    /**
     * key for audio sample rate, value type is number
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.Core
     * @atomicservice
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
   * @syscap SystemCapability.Multimedia.Media.AVRecorder
   */
  enum AudioSourceType {
    /**
     * Default audio source type.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    AUDIO_SOURCE_TYPE_DEFAULT = 0,
    /**
     * Source type mic.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    AUDIO_SOURCE_TYPE_MIC = 1,
  }

  /**
   * Enumerates video source type for recorder.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.AVRecorder
   */
  enum VideoSourceType {
    /**
     * Surface raw data.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    VIDEO_SOURCE_TYPE_SURFACE_YUV = 0,
    /**
     * Surface ES data.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
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
   * Provides the media recorder profile definitions.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.AVRecorder
   */
   interface AVRecorderProfile {
    /**
     * Indicates the audio bitrate.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    audioBitrate?: number;

    /**
     * Indicates the number of audio channels.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    audioChannels?: number;

    /**
     * Indicates the audio encoding format.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    audioCodec?: CodecMimeType;

    /**
     * Indicates the audio sampling rate.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    audioSampleRate?: number;

    /**
     * Indicates the output file format.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    fileFormat: ContainerFormatType;

    /**
     * Indicates the video bitrate.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    videoBitrate?: number;

    /**
     * Indicates the video encoding format.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    videoCodec?: CodecMimeType;

    /**
     * Indicates the video width.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    videoFrameWidth?: number;

    /**
     * Indicates the video height.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    videoFrameHeight?: number;

    /**
     * Indicates the video frame rate.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    videoFrameRate?: number;
  }

  /**
   * Provides the media recorder configuration definitions.
   * @since 9
   * @syscap SystemCapability.Multimedia.Media.AVRecorder
   */
  interface AVRecorderConfig {
    /**
     * Audio source type, details see @AudioSourceType .
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    audioSourceType?: AudioSourceType;
    /**
     * Video source type, details see @VideoSourceType .
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    videoSourceType?: VideoSourceType;
    /**
     * Video recorder profile, details see @AVRecorderProfile .
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    profile: AVRecorderProfile;
    /**
     * File output uri, support a kind of uri now.
     * format like: "fd://" + "context".
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    url: string;
    /**
     * Sets the video rotation angle in output file, and for the file to playback, mp4 support
     * the range of rotation angle should be {0, 90, 180, 270}, default is 0.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    rotation?: number;
    /**
     * Geographical location information.
     * @since 9
     * @syscap SystemCapability.Multimedia.Media.AVRecorder
     */
    location?: Location;
  }

  /**
   * Provides the container definition for media description key-value pairs.
   * @since 8
   * @syscap SystemCapability.Multimedia.Media.Core
   */
  /**
   * Provides the container definition for media description key-value pairs.
   * @since 11
   * @syscap SystemCapability.Multimedia.Media.Core
   * @atomicservice
   */
  interface MediaDescription {
    /**
     * key:value pair, key see @MediaDescriptionKey .
     * @since 8
     * @syscap SystemCapability.Multimedia.Media.Core
     */
    /**
     * key:value pair, key see @MediaDescriptionKey .
     * @since 11
     * @syscap SystemCapability.Multimedia.Media.Core
     * @atomicservice
     */
    [key : string]: Object;
  }

  /**
   * Enumerates seek mode.
   * @since 8
   * @syscap SystemCapability.Multimedia.Media.Core
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
