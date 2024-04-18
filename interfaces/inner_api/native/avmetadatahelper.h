/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef AVMETADATAHELPER_H
#define AVMETADATAHELPER_H

#include <memory>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include "buffer/avsharedmemory.h"
#include "meta/format.h"
#include "meta/meta.h"
#include "media_data_source.h"
#include "nocopyable.h"
#include "pixel_map.h"

namespace OHOS {
namespace Media {
/**
 * @brief Enumerates avmetadata helper callback error type.
 */
enum HelperErrorType : int32_t {
    /* State error, current operation is invalid. */
    INVALID_OPERATION,
    /* Result error, current result is invalid. */
    INVALID_RESULT,
};

/**
 * @brief Enumerates avmetadata helper listener info type.
 */
enum HelperOnInfoType : int32_t {
    /* Current State changed, notify NAPI with onInfo callback. */
    HELPER_INFO_TYPE_STATE_CHANGE,
};

/**
 * @brief Enumerates avmetadata helper states.
 */
enum HelperStates : int32_t {
    /* error states */
    HELPER_STATE_ERROR = 0,
    /* idle states */
    HELPER_IDLE = 1,
    /* prepared states */
    HELPER_PREPARED = 2,
    /* call done states */
    HELPER_CALL_DONE = 3,
    /* released states */
    HELPER_RELEASED = 4,
};

/**
 * The meta data mappings from meta data enum keys to the string key.
 */
static const std::map<int32_t, const char*> g_MetadataCodeMap = {
    {0,     "album"},
    {1,     "albumArtist"},
    {2,     "artist"},
    {3,     "author"},
    {4,     "dateTime"},
    {5,     "dateTimeFormat"},
    {12,    "composer"},
    {15,    "duration"},
    {18,    "genre"},
    {19,    "hasAudio"},
    {21,    "hasVideo"},
    {29,    "mimeType"},
    {30,    "trackCount"},
    {31,    "sampleRate"},
    {33,    "title"},
    {35,    "videoHeight"},
    {37,    "videoWidth"},
    {38,    "videoOrientation"},
    {39,    "hdrType"},
};

/**
 * support metadata parameters
*/
static const std::vector<std::string> g_Metadata = {
    "album",
    "albumArtist",
    "artist",
    "author",
    "dateTime",
    "dateTimeFormat",
    "composer",
    "duration",
    "genre",
    "hasAudio",
    "hasVideo",
    "mimeType",
    "trackCount",
    "sampleRate",
    "title",
    "videoHeight",
    "videoWidth",
    "videoOrientation",
    "hdrType",
    "latitude",
    "longitude",
    "customInfo",
};

enum HdrType : int32_t {
    /**
     * This option is used to mark none HDR type.
     */
    AV_HDR_TYPE_NONE,
    /**
     * This option is used to mark HDR Vivid type.
     */
    AV_HDR_TYPE_VIVID,
};

/**
 * @brief Enumerates avmetadata usage.
 */
enum AVMetadataUsage : int32_t {
    /**
     * Indicates that the avmetadahelper's instance will only be used for resolving the
     * metadata from the given media resource.
     */
    AV_META_USAGE_META_ONLY,
    /**
     * Indicates that the avmetadahelper's instance will be used for fetching the video frame
     * and resolving metadata from the given media resource.
     */
    AV_META_USAGE_PIXEL_MAP,
};

/**
 * @brief Enumerates avmetadata's metadata key.
 */
enum AVMetadataCode : int32_t {
    /**
     * The metadata key to retrieve the information about the album title
     * of the media source.
     */
    AV_KEY_ALBUM = 0,
    /**
     * The metadata key to retrieve the information about the performers or
     * artist associated with the media source.
     */
    AV_KEY_ALBUM_ARTIST = 1,
    /**
     * The metadata key to retrieve the information about the artist of
     * the media source.
     */
    AV_KEY_ARTIST = 2,
    /**
     * The metadata key to retrieve the information about the author of
     * the media source.
     */
    AV_KEY_AUTHOR = 3,
    /**
     * The metadata key to retrieve the information about the created time of
     * the media source.
     */
    AV_KEY_DATE_TIME = 4,
    /**
     * The metadata key to retrieve the information about the created time of
     * the media source. This keyword is provided for the media library.
     */
    AV_KEY_DATE_TIME_FORMAT = 5,
    /**
     * The metadata key to retrieve the information about the composer of
     * the media source.
     */
    AV_KEY_COMPOSER = 12,
    /**
     * The metadata key to retrieve the playback duration of the media source.
     */
    AV_KEY_DURATION = 15,
    /**
     * The metadata key to retrieve the content type or genre of the data
     * source.
     */
    AV_KEY_GENRE = 18,
    /**
     * If this key exists the media contains audio content.
     */
    AV_KEY_HAS_AUDIO = 19,
    /**
     * If this key exists the media contains video content.
     */
    AV_KEY_HAS_VIDEO = 21,
    /**
     * The metadata key to retrieve the mime type of the media source. Some
     * example mime types include: "video/mp4", "audio/mp4", "audio/amr-wb",
     * etc.
     */
    AV_KEY_MIME_TYPE = 29,
    /**
     * The metadata key to retrieve the number of tracks, such as audio, video,
     * text, in the media source, such as a mp4 or 3gpp file.
     */
    AV_KEY_NUM_TRACKS = 30,
    /**
     * This key retrieves the sample rate, if available.
     */
    AV_KEY_SAMPLE_RATE = 31,
    /**
     * The metadata key to retrieve the media source title.
     */
    AV_KEY_TITLE = 33,
    /**
     * If the media contains video, this key retrieves its height.
     */
    AV_KEY_VIDEO_HEIGHT = 35,
    /**
     * If the media contains video, this key retrieves its width.
     */
    AV_KEY_VIDEO_WIDTH = 37,
    /**
     * The metadata key to retrieve the information about the video
     * orientation.
     */
    AV_KEY_VIDEO_ORIENTATION = 38,
    /**
     * The metadata key to retrieve the information about the video
     * is HDR or not.
     */
    AV_KEY_VIDEO_IS_HDR_VIVID = 39,

    /**
     * The metadata key to retrieve the information about the location longitude
    */
    AV_KEY_LOCATION_LONGITUDE = 40,

    /**
     * The metadata key to retrieve the information about the location latitude
    */
    AV_KEY_LOCATION_LATITUDE = 41,

    /**
     * Custom parameter key-value map
    */
    AV_KEY_CUSTOMINFO = 42,
};

/**
 * @brief Enumerates avmetadata's query option.
 */
enum AVMetadataQueryOption : int32_t {
    /**
     * This option is used to fetch a key frame from the given media
     * resource that is located right after or at the given time.
     */
    AV_META_QUERY_NEXT_SYNC,
    /**
     * This option is used to fetch a key frame from the given media
     * resource that is located right before or at the given time.
     */
    AV_META_QUERY_PREVIOUS_SYNC,
    /**
     * This option is used to fetch a key frame from the given media
     * resource that is located closest to or at the given time.
     */
    AV_META_QUERY_CLOSEST_SYNC,
    /**
     * This option is used to fetch a frame (maybe not keyframe) from
     * the given media resource that is located closest to or at the given time.
     */
    AV_META_QUERY_CLOSEST,
};

/**
 * @brief Provides the definition of the returned pixelmap's configuration
 */
struct PixelMapParams {
    /**
     * Expected pixelmap's width, -1 means to keep consistent with the
     * original dimensions of the given video resource.
     */
    int32_t dstWidth = -1;
    /**
     * Expected pixelmap's width, -1 means to keep consistent with the
     * original dimensions of the given video resource.
     */
    int32_t dstHeight = -1;
    /**
     * Expected pixelmap's color format, see {@link PixelFormat}. Currently,
     * RGB_565, RGB_888, RGBA_8888 are supported.
     */
    PixelFormat colorFormat = PixelFormat::RGB_565;
};

/**
 * @brief Provides the callback interfaces to notify client about errors or infos.
 */
class HelperCallback {
public:
    virtual ~HelperCallback() = default;
    /**
     * Called when a metadata helper message is notified.
     *
     * @param type Indicates the information type. For details, see {@link HelperOnInfoType}.
     * @param extra Indicates other information, for example, the current state in metadata helper server.
     * @param infoBody According to the info type, the information carrier passed. Is an optional parameter.
     */
    virtual void OnInfo(HelperOnInfoType type, int32_t extra, const Format &infoBody = {}) = 0;

    /**
     * Called when an error occurred
     *
     * @param errorCode Error code.
     * @param errorMsg Error message.
     */
    virtual void OnError(int32_t errorCode, const std::string &errorMsg) = 0;
};

/**
 * @brief Provides the interfaces to resolve metadata or fetch frame
 * from a given media resource.
 */
class AVMetadataHelper {
public:
    virtual ~AVMetadataHelper() = default;

    /**
     * Set the media source uri to resolve. Calling this method before the reset
     * of the methods in this class. This method maybe time consuming.
     * @param uri the URI of input media source.
     * @param usage indicates which scene the avmedatahelper's instance will
     * be used to, see {@link AVMetadataUsage}. If the usage need to be changed,
     * this method must be called again.
     * @return Returns {@link MSERR_OK} if the setting is successful; returns
     * an error code otherwise.
     */
    virtual int32_t SetSource(const std::string &uri, int32_t usage = AVMetadataUsage::AV_META_USAGE_PIXEL_MAP) = 0;

    /**
     * @brief Sets the media file descriptor source to resolve. Calling this method
     * before the reset of the methods in this class. This method maybe time consuming.
     * @param fd Indicates the file descriptor of media source.
     * @param offset Indicates the offset of media source in file descriptor.
     * @param size Indicates the size of media source.
     * @param usage Indicates which scene the avmedatahelper's instance will
     * be used to, see {@link AVMetadataUsage}. If the usage need to be changed,
     * this method must be called again.
     * @return Returns {@link MSERR_OK} if the setting is successful; returns
     * an error code otherwise.
     */
    virtual int32_t SetSource(int32_t fd, int64_t offset = 0, int64_t size = 0,
        int32_t usage = AVMetadataUsage::AV_META_USAGE_PIXEL_MAP) = 0;

    /**
     * @brief Sets the playback media data source for the meta data helper.
     *
     * @param dataSrc Indicates the media data source. in {@link media_data_source.h}
     * @return Returns {@link MSERR_OK} if the mediadatasource is set successfully; returns an error code defined
     * in {@link media_errors.h} otherwise.
     */
    virtual int32_t SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc) = 0;

    /**
     * Retrieve the meta data associated with the specified key. This method must be
     * called after the SetSource.
     * @param key One of the constants listed above at the definition of {@link AVMetadataCode}.
     * @return Returns the meta data value associate with the given key code on
     * success; empty string on failure.
     */
    virtual std::string ResolveMetadata(int32_t key) = 0;

    /**
     * Retrieve all meta data within the listed above at the definition of {@link AVMetadataCode}.
     * This method must be called after the SetSource.
     * @return Returns the meta data values on success; empty hash map on failure.
     */
    virtual std::unordered_map<int32_t, std::string> ResolveMetadata() = 0;

    /**
     * Fetch the album art picture associated with the data source. If there are
     * more than one pictures, the cover image will be returned preferably.
     * @return Returns the a chunk of shared memory containing a picture, which can be
     * null, if such a picture can not be fetched.
     */
    virtual std::shared_ptr<AVSharedMemory> FetchArtPicture() = 0;

    /**
     * Fetch a representative video frame near a given timestamp by considering the given
     * option if possible, and return a pixelmap with given parameters. This method must be
     * called after the SetSource.
     * @param timeUs The time position in microseconds where the frame will be fetched.
     * When fetching the frame at the given time position, there is no guarantee that
     * the video source has a frame located at the position. When this happens, a frame
     * nearby will be returned. If timeUs is negative, time position and option will ignored,
     * and any frame that the implementation considers as representative may be returned.
     * @param option the hint about how to fetch a frame, see {@link AVMetadataQueryOption}
     * @param param the desired configuration of returned pixelmap, see {@link PixelMapParams}.
     * @return Returns a pixelmap containing a scaled video frame, which can be null, if such a
     * frame cannot be fetched.
     */
    virtual std::shared_ptr<PixelMap> FetchFrameAtTime(int64_t timeUs, int32_t option, const PixelMapParams &param) = 0;

    /**
     * all meta data.
     * This method must be called after the SetSource.
     * @return Returns the meta data values on success; nullptr on failure.
     */
    virtual std::shared_ptr<Meta> GetAVMetadata() = 0;

    /**
     * Release the internel resource. After this method called, the avmetadatahelper instance
     * can not be used again.
     */
    virtual void Release() = 0;

    /**
     * @brief Method to set the meta data helper callback.
     *
     * @param callback object pointer.
     * @return Returns {@link MSERR_OK} if the meta data helper callback is set; returns an error code defined
     * in {@link media_errors.h} otherwise.
     */
    virtual int32_t SetHelperCallback(const std::shared_ptr<HelperCallback> &callback) = 0;
};

class __attribute__((visibility("default"))) AVMetadataHelperFactory {
public:
#ifdef UNSUPPORT_METADATA
    static std::shared_ptr<AVMetadataHelper> CreateAVMetadataHelper()
    {
        return nullptr;
    }
#else
    static std::shared_ptr<AVMetadataHelper> CreateAVMetadataHelper();
#endif
private:
    AVMetadataHelperFactory() = default;
    ~AVMetadataHelperFactory() = default;
};
} // namespace Media
} // namespace OHOS
#endif // AVMETADATAHELPER_H