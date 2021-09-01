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

#include "gst_meta_parser.h"
#include "media_errors.h"
#include "media_log.h"
#include "inner_meta_define.h"
#include "gst_utils.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstMetaParser"};
}

namespace OHOS {
namespace Media {
struct KeyToXMap {
    std::string_view keyName;
};

#define STRINGFY(key) #key
static const KeyToXMap KEY_TO_X_MAP[INNER_META_KEY_BUTT] = {
    { STRINGFY(INNER_META_KEY_ALBUM) },
    { STRINGFY(INNER_META_KEY_ALBUM_ARTIST) },
    { STRINGFY(INNER_META_KEY_ARTIST) },
    { STRINGFY(INNER_META_KEY_AUTHOR) },
    { STRINGFY(INNER_META_KEY_COMPOSER) },
    { STRINGFY(INNER_META_KEY_DURATION) },
    { STRINGFY(INNER_META_KEY_GENRE) },
    { STRINGFY(INNER_META_KEY_HAS_AUDIO) },
    { STRINGFY(INNER_META_KEY_HAS_VIDEO) },
    { STRINGFY(INNER_META_KEY_MIME_TYPE) },
    { STRINGFY(INNER_META_KEY_NUM_TRACKS) },
    { STRINGFY(INNER_META_KEY_SAMPLE_RATE) },
    { STRINGFY(INNER_META_KEY_TITLE) },
    { STRINGFY(INNER_META_KEY_VIDEO_HEIGHT) },
    { STRINGFY(INNER_META_KEY_VIDEO_WIDTH) },
};

static const std::unordered_map<std::string_view, int32_t> TAG_TO_KEY_MAPPING = {
    { GST_TAG_ALBUM, INNER_META_KEY_ALBUM },
    { GST_TAG_ALBUM_ARTIST, INNER_META_KEY_ALBUM_ARTIST },
    { GST_TAG_ARTIST, INNER_META_KEY_ARTIST },
    { GST_TAG_COMPOSER, INNER_META_KEY_COMPOSER },
    { GST_TAG_GENRE, INNER_META_KEY_GENRE },
    { GST_TAG_TRACK_COUNT, INNER_META_KEY_NUM_TRACKS },
    { GST_TAG_TITLE, INNER_META_KEY_TITLE }
};

static const std::unordered_map<std::string_view, int32_t> CAPS_FIELD_TO_KEY_MAPPING = {
    { "width", INNER_META_KEY_VIDEO_WIDTH },
    { "height", INNER_META_KEY_VIDEO_HEIGHT },
    { "rate", INNER_META_KEY_SAMPLE_RATE },
};

static const std::unordered_map<std::string_view, std::string_view> FILE_MIME_TYPE_MAPPING = {
    { "application/x-3gp", "video/mp4" }, // mp4
    { "video/quicktime", "video/mp4" },
    { "video/quicktime, variant=(string)apple", "video/mp4" },
    { "video/quicktime, variant=(string)iso", "video/mp4" },
    { "video/quicktime, variant=(string)3gpp", "video/mp4" },
    { "video/quicktime, variant=(string)3g2", "video/mp4" },
    { "audio/mpeg", "audio/mp4a-latm" }, // aac
    { "audio/m4a", "audio/mp4a-latm" }, // m4a
    { "application/x-id3", "audio/mpeg" }, // mp3
    { "application/ogg", "audio/ogg" }, // ogg
    { "audio/ogg", "audio/ogg" },
    { "audio/x-flac", "audio/flac" }, // flac
    { "audio/x-wav", "audio/x-wav" } // wav
};

enum TrackType : uint8_t {
    TRACK_TYPE_UNKNOWN,
    TRACK_TYPE_VIDEO,
    TRACK_TYPE_AUDIO,
    TRACK_TYPE_TEXT,
    TRACK_TYPE_IMAGE,
};

struct GstMetaParser::TrackMetaInfo {
    bool tagCatched = false;
    bool capsCatched = false;
    bool bufferCatched = false;
    uint8_t trackType = TRACK_TYPE_UNKNOWN;
};

struct GstMetaParser::FileMetaInfo {
    bool tagCatched = false; // global tag
    bool capsCatched = false; // demuxer's sinkpad caps
    int64_t duration = 0;
};

struct GstMetaParser::PadInfo {
    GstMetaParser::MetaSourceType srcType;
    GstPadDirection direction;
    gulong probeId;
    gulong blockId;
};

/**
 * Preferentially retrieve the cover page from GST_TAG_IMAGE, then GST_TAG_PREVIEW_IMAGE
 */

using GstMetaParserWrapper = ThizWrapper<GstMetaParser>;

void GstMetaParser::HaveTypeCallback(GstElement *elem, guint probability, GstCaps *caps, gpointer userdata)
{
    if (elem == nullptr || caps == nullptr || userdata == nullptr) {
        return;
    }

    auto thizStrong = GstMetaParserWrapper::TakeStrongThiz(userdata);
    if (thizStrong != nullptr) {
        return thizStrong->OnHaveType(*elem, probability, *caps);
    }
}

void GstMetaParser::PadAddedCallback(GstElement *elem, GstPad *pad, gpointer userdata)
{
    if (elem == nullptr || pad == nullptr || userdata == nullptr) {
        return;
    }

    auto thizStrong = GstMetaParserWrapper::TakeStrongThiz(userdata);
    if (thizStrong != nullptr) {
        return thizStrong->OnPadAdded(*elem, *pad);
    }
}

GstPadProbeReturn GstMetaParser::BlockCallback(GstPad *pad, GstPadProbeInfo *info, gpointer usrdata)
{
    if (pad == nullptr || info ==  nullptr || usrdata == nullptr) {
        MEDIA_LOGE("param is invalid");
        return GST_PAD_PROBE_PASS;
    }

    auto thizStrong = GstMetaParserWrapper::TakeStrongThiz(usrdata);
    CHECK_AND_RETURN_RET_LOG(thizStrong != nullptr, GST_PAD_PROBE_PASS, "thizStrong is nullptr");

    auto type = static_cast<unsigned int>(info->type);
    if (type & (GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_BUFFER_LIST)) {
        GstBuffer *buffer = gst_pad_probe_info_get_buffer (info);
        CHECK_AND_RETURN_RET_LOG(buffer != nullptr, GST_PAD_PROBE_PASS, "buffer is null");
        thizStrong->OnBufferProbe(*pad, *buffer);
        return GST_PAD_PROBE_OK;
    }

    return GST_PAD_PROBE_PASS;
}

GstPadProbeReturn GstMetaParser::ProbeCallback(GstPad *pad, GstPadProbeInfo *info, gpointer usrdata)
{
    if (pad == nullptr || info ==  nullptr || usrdata == nullptr) {
        MEDIA_LOGE("param is invalid");
        return GST_PAD_PROBE_OK;
    }

    auto thizStrong = GstMetaParserWrapper::TakeStrongThiz(usrdata);
    CHECK_AND_RETURN_RET_LOG(thizStrong != nullptr, GST_PAD_PROBE_OK, "thizStrong is nullptr");

    auto type = static_cast<unsigned int>(info->type);
    if (type & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) {
        GstEvent *event = gst_pad_probe_info_get_event (info);
        CHECK_AND_RETURN_RET_LOG(event != nullptr, GST_PAD_PROBE_OK, "event is null");
        thizStrong->OnEventProbe(*pad, *event);
    }

    if (type & (GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_BUFFER_LIST)) {
        GstBuffer *buffer = gst_pad_probe_info_get_buffer (info);
        CHECK_AND_RETURN_RET_LOG(buffer != nullptr, GST_PAD_PROBE_OK, "buffer is null");
        thizStrong->OnBufferProbe(*pad, *buffer);
    }

    return GST_PAD_PROBE_OK;
}

GstMetaParser::GstMetaParser()
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

GstMetaParser::~GstMetaParser()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    Reset();
}

void GstMetaParser::Start(bool needBlockBuffer)
{
    MEDIA_LOGD("enter, needBlockBuffer = %{public}d", needBlockBuffer);
    std::unique_lock<std::mutex> lock(mutex_);

    if (fileMetaInfo_ == nullptr) {
        fileMetaInfo_ = std::make_unique<FileMetaInfo>();
        (void)allMetaInfo_.emplace(INNER_META_KEY_NUM_TRACKS, "0");
    }

    collecting_ = true;
    if (!needBlockBuffer) {
        for(auto &[pad, padInfo] :  padInfos_) {
            if (padInfo.blockId != 0) {
                gst_pad_remove_probe(pad, padInfo.blockId);
                padInfo.blockId = 0;
            }
        }
    }
    needBlockBuffer_ = needBlockBuffer;
}

void GstMetaParser::AddMetaSource(GstElement &src, MetaSourceType type)
{
    MEDIA_LOGI("enter, elem: %{public}s, type: %{public}hhu", GST_ELEMENT_NAME(&src), type);
    std::unique_lock<std::mutex> lock(mutex_);

    if (type == MetaSourceType::META_SRC_DECODER) {
        AddDecoderMetaSource(src);
    } else if (type == MetaSourceType::META_SRC_PARSER) {
        AddParserMetaSource(src);
    } else if (type == MetaSourceType::META_SRC_DEMUXER) {
        AddDemuxerMetaSource(src);
    } else if (type == MetaSourceType::META_SRC_TYPEFIND) {
        AddTypeFindMetaSource(src);
    } else {
        MEDIA_LOGE("unsupported type: %{public}hhu", type);
    }
}

void GstMetaParser::AddDecoderMetaSource(GstElement &src)
{
    if (demuxer_ != nullptr) {
        MEDIA_LOGI("has decoder, remove the block probe at demuxer's srcpad");
        for (auto &[pad, padInfo] : padInfos_) {
            if (padInfo.srcType == MetaSourceType::META_SRC_DEMUXER && padInfo.blockId != 0) {
                gst_pad_remove_probe(pad, padInfo.blockId);
            }
        }
    }

    for (GList *padNode = g_list_first(src.sinkpads); padNode != nullptr; padNode = padNode->next) {
        if (padNode->data == nullptr) {
            continue;
        }

        GstPad *pad = reinterpret_cast<GstPad *>(padNode->data);
        int32_t ret = AddProbeToPad(*pad, MetaSourceType::META_SRC_DECODER);
        if (ret != MSERR_OK) {
            MEDIA_LOGE("add probe for decoder's sinkpad %{public}s failed", GST_PAD_NAME(pad));
        }
    }
}

void GstMetaParser::AddParserMetaSource(GstElement &src)
{
    if (demuxer_ != nullptr) {
        MEDIA_LOGI("has demuxer, ignore the parser");
        return;
    }

    for (GList *padNode = g_list_first(src.srcpads); padNode != nullptr; padNode = padNode->next) {
        if (padNode->data == nullptr) {
            continue;
        }

        GstPad *pad = reinterpret_cast<GstPad *>(padNode->data);
        int32_t ret = AddProbeToPad(*pad, MetaSourceType::META_SRC_PARSER);
        if (ret != MSERR_OK) {
            MEDIA_LOGE("add probe for parser's sinkpad %{public}s failed", GST_PAD_NAME(pad));
        }
    }
}

void GstMetaParser::AddDemuxerMetaSource(GstElement &src)
{
    if (demuxer_ != nullptr) {
        MEDIA_LOGI("has demuxer 0x%{public}06" PRIXPTR ", ignore the demuxer", FAKE_POINTER(demuxer_));
        return;
    }

    trackMetaInfos_.clear();
    demuxer_ = &src;

    for (GList *padNode = g_list_first(src.srcpads); padNode != nullptr; padNode = padNode->next) {
        if (padNode->data == nullptr) {
            continue;
        }

        GstPad *pad = reinterpret_cast<GstPad *>(padNode->data);
        int32_t ret = AddProbeToPad(*pad, MetaSourceType::META_SRC_PARSER);
        if (ret != MSERR_OK) {
            MEDIA_LOGE("add probe for parser's sinkpad %{public}s failed", GST_PAD_NAME(pad));
        }
    }

    auto wrapper = new (std::nothrow) GstMetaParserWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");

    (void)g_signal_connect_data(&src, "pad-added", G_CALLBACK(&GstMetaParser::PadAddedCallback),
        wrapper, (GClosureNotify)&GstMetaParserWrapper::OnDestory, static_cast<GConnectFlags>(0));
}

void GstMetaParser::AddTypeFindMetaSource(GstElement &src)
{
    auto wrapper = new (std::nothrow) GstMetaParserWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");

    (void)g_signal_connect_data(&src, "have-type", G_CALLBACK(&GstMetaParser::HaveTypeCallback),
        wrapper, (GClosureNotify)&GstMetaParserWrapper::OnDestory, static_cast<GConnectFlags>(0));
}

std::string GstMetaParser::GetMetadata(int32_t key)
{
    CHECK_AND_RETURN_RET_LOG(key < INNER_META_KEY_BUTT, "", "invalid key");

    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this]() { return !collecting_ || CheckCollectCompleted(); });

    if (allMetaInfo_.find(INNER_META_KEY_DURATION) == allMetaInfo_.end()) {
        int64_t duration = (fileMetaInfo_->duration + 500000) / 1000000; // ns -> ms
        (void)allMetaInfo_.emplace(INNER_META_KEY_DURATION, std::to_string(duration));
    }

    auto it = allMetaInfo_.find(key);
    if (it != allMetaInfo_.end()) {
        return it->second;
    }

    return "";
}

std::unordered_map<int32_t, std::string> GstMetaParser::GetMetadata()
{
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this]() { return !collecting_ || CheckCollectCompleted(); });

    if (allMetaInfo_.find(INNER_META_KEY_DURATION) == allMetaInfo_.end()) {
        int64_t duration = (fileMetaInfo_->duration + 500000) / 1000000; // ns -> ms
        (void)allMetaInfo_.emplace(INNER_META_KEY_DURATION, std::to_string(duration));
    }
    return allMetaInfo_;
}

bool GstMetaParser::CheckCollectCompleted() const
{
    if (!fileMetaInfo_->capsCatched) {
        return false;
    }

    if (trackMetaInfos_.size() == 0) {
        return false;
    }

    for (const auto &[pad, trackInfo] : trackMetaInfos_) {
        if (trackInfo.trackType == TRACK_TYPE_VIDEO ||
            trackInfo.trackType == TRACK_TYPE_AUDIO ||
            trackInfo.trackType == TRACK_TYPE_IMAGE) {
            if (!trackInfo.bufferCatched) {
                return false;
            }
        } else {
            if (!trackInfo.capsCatched) {
                return false;
            }
        }
    }

    return true;
}

void GstMetaParser::OnHaveType(const GstElement &elem, guint probability, const GstCaps &caps)
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("typefind [%{public}s] found type, probability: %{public}u", GST_ELEMENT_NAME(&elem), probability);

    guint capsSize = gst_caps_get_size(&caps);
    for (guint index = 0; index < capsSize; index++) {
        const GstStructure *struc = gst_caps_get_structure(&caps, index);
        FileCapsParse(*struc);
    }

    cond_.notify_all();
}

void GstMetaParser::OnPadAdded(const GstElement &src, GstPad &pad)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (demuxer_ != &src) {
        return;
    }

    MEDIA_LOGD("demuxer sinkpad %{public}s added", GST_PAD_NAME(&pad));

    int32_t ret = AddProbeToPad(pad, MetaSourceType::META_SRC_DEMUXER);
    CHECK_AND_RETURN_LOG(ret == MSERR_OK,  "add probe to demuxer sinkpad failed");
}

void GstMetaParser::OnEventProbe(GstPad &pad, GstEvent &event)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(padInfos_.count(const_cast<GstPad *>(&pad)) != 0, "unknown pad");

    if (GST_EVENT_TYPE(&event) == GST_EVENT_TAG) {
        GstTagList *tagList = nullptr;
        gst_event_parse_tag(&event, &tagList);
        CHECK_AND_RETURN_LOG(tagList != nullptr, "taglist is nullptr");

        TagEventParse(pad, *tagList);
    }

    if (GST_EVENT_TYPE(&event) == GST_EVENT_CAPS) {
        GstCaps *caps = nullptr;
        gst_event_parse_caps(&event, &caps);
        CHECK_AND_RETURN_LOG(caps != nullptr, "caps is nullptr");

        CapsEventParse(pad, *caps);
    }
}

void GstMetaParser::OnBufferProbe(GstPad &pad, GstBuffer &buffer)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(padInfos_.count(&pad) != 0, "unknown pad");

    auto it = trackMetaInfos_.find(&pad);
    if (it == trackMetaInfos_.end())  {
        return;
    }

    if (it->second.bufferCatched) {
        return;
    }

    it->second.bufferCatched = true;
    MEDIA_LOGI("catch buffer at pad %{public}s, trackType: %{public}hhu", GST_PAD_NAME(&pad), it->second.trackType);

    cond_.notify_all();
}

void GstMetaParser::TagVisitor(const GstTagList *list, const gchar *tag,  GstMetaParser *thiz)
{
    MEDIA_LOGD("visit tag: %{public}s", tag);
    auto tagToKeyIt = TAG_TO_KEY_MAPPING.find(tag);
    if (tagToKeyIt == TAG_TO_KEY_MAPPING.end()) {
        return;
    }

    CHECK_AND_RETURN_LOG(thiz != nullptr, "thiz is nullptr");

    const GValue *value = gst_tag_list_get_value_index(list, tag, 0);
    CHECK_AND_RETURN_LOG(value != nullptr, "get value for tag: %{public}s failed", tag);

    if (GST_VALUE_HOLDS_SAMPLE(value) || GST_VALUE_HOLDS_BUFFER(value)) {
        return; // not considering the sample and bufer currently.
    }

    gchar *str = gst_value_serialize(value);
    CHECK_AND_RETURN_LOG(str != nullptr, "serialize value failed");

    MEDIA_LOGI("got meta, key: %{public}s, value: %{public}s", KEY_TO_X_MAP[tagToKeyIt->second].keyName.data(), str);
    auto it = thiz->allMetaInfo_.find(tagToKeyIt->second);
    if (it != thiz->allMetaInfo_.end()) {
        MEDIA_LOGI("update the key: %{public}s, existed value: %{public}s",
            KEY_TO_X_MAP[tagToKeyIt->second].keyName.data(), it->second.c_str());
        it->second = str;
    } else {
        (void)thiz->allMetaInfo_.emplace(tagToKeyIt->second, std::string(str));
    }
    g_free(str);
}

void GstMetaParser::QueryDuration(GstPad &pad)
{
    GstQuery *query = gst_query_new_duration(GST_FORMAT_TIME);
    CHECK_AND_RETURN_LOG(query != nullptr, "query is failed");

    if (gst_pad_query(&pad, query)) {
        GstFormat format = GST_FORMAT_TIME;
        gint64 duration = 0;
        gst_query_parse_duration(query, &format, &duration);
        MEDIA_LOGI("duration = %{public}" PRIi64 "", duration);
        if (GST_CLOCK_TIME_IS_VALID(duration) && (duration > fileMetaInfo_->duration)) {
            fileMetaInfo_->duration = duration;
        }
    }

    gst_object_unref(query);
}

void GstMetaParser::TagEventParse(GstPad &pad, const GstTagList &tagList)
{
    if (trackMetaInfos_.count(&pad) == 0) {
        return;
    }

    GstTagScope scope = gst_tag_list_get_scope(&tagList);
    MEDIA_LOGI("catch tag %{public}s event", scope == GST_TAG_SCOPE_GLOBAL ? "global" : "stream");

    QueryDuration(pad);

    if (scope == GST_TAG_SCOPE_STREAM) {
        trackMetaInfos_.at(&pad).tagCatched = true;
    } else {
        if (fileMetaInfo_->tagCatched) {
            return;
        }
        fileMetaInfo_->tagCatched = true;
    }

    gst_tag_list_foreach(&tagList, (GstTagForeachFunc)TagVisitor, this);

    cond_.notify_all();
}

void GstMetaParser::ExpectedCapsParse(const GstStructure &structure, const std::vector<std::string_view> &fields)
{
    for (auto &field : fields) {
        if (!gst_structure_has_field(&structure, field.data())) {
            continue;
        }
        const GValue *val = gst_structure_get_value(&structure, field.data());
        if (val == nullptr) {
            MEDIA_LOGE("get %{public}s filed's value failed", field.data());
            continue;
        }
        gchar *str = gst_value_serialize(val);
        if (str == nullptr) {
            MEDIA_LOGE("serialize value for field %{public}s failed", field.data());
            continue;
        }

        MEDIA_LOGI("field %{public}s is %{public}s", field.data(), str);
        if (CAPS_FIELD_TO_KEY_MAPPING.count(field) != 0) {
            (void)allMetaInfo_.emplace(CAPS_FIELD_TO_KEY_MAPPING.at(field), str);
        }
        g_free(str);
    }
}

void GstMetaParser::VideoCapsParse(const GstPad &pad, const GstStructure &structure)
{
    MEDIA_LOGI("has video stream for pad: %{public}s", GST_PAD_NAME(&pad));
    (void)allMetaInfo_.emplace(INNER_META_KEY_HAS_VIDEO, "yes");

    std::string_view mimeType = gst_structure_get_name(&structure);
    MEDIA_LOGI("video codec type: %{public}s", mimeType.data());

    TrackMetaInfo &info = trackMetaInfos_.at(&pad);
    info.capsCatched = true;
    info.trackType = TRACK_TYPE_VIDEO;

    static const std::vector<std::string_view> targetFields = {
        "width", "height",
    };

    ExpectedCapsParse(structure, targetFields);
}

void GstMetaParser::AudioCapsParse(const GstPad &pad, const GstStructure &structure)
{
    MEDIA_LOGI("has audio stream for pad: %{public}s", GST_PAD_NAME(&pad));
    (void)allMetaInfo_.emplace(INNER_META_KEY_HAS_AUDIO, "yes");

    std::string_view mimeType = gst_structure_get_name(&structure);
    MEDIA_LOGI("audio codec type: %{public}s", mimeType.data());

    TrackMetaInfo &info = trackMetaInfos_.at(&pad);
    info.capsCatched = true;
    info.trackType = TRACK_TYPE_AUDIO;

    static const std::vector<std::string_view> targetFields = {
        "rate",
    };

    ExpectedCapsParse(structure, targetFields);
}

void GstMetaParser::ImageCapsParse(const GstPad &pad, const GstStructure &structure)
{
    MEDIA_LOGI("has image stream for pad: %{public}s", GST_PAD_NAME(&pad));

    std::string_view mimeType = gst_structure_get_name(&structure);
    MEDIA_LOGI("image codec type: %{public}s", mimeType.data());

    TrackMetaInfo &info = trackMetaInfos_.at(&pad);
    info.capsCatched = true;
    info.trackType = TRACK_TYPE_IMAGE;

    static const std::vector<std::string_view> targetFields = {
        "width", "height",
    };

    ExpectedCapsParse(structure, targetFields);
}

void GstMetaParser::TextCapsParse(const GstPad &pad, const GstStructure &structure)
{
    MEDIA_LOGI("has text stream for pad: %{public}s", GST_PAD_NAME(&pad));

    std::string_view mimeType = gst_structure_get_name(&structure);
    MEDIA_LOGI("text codec type: %{public}s", mimeType.data());

    TrackMetaInfo &info = trackMetaInfos_.at(&pad);
    info.capsCatched = true;
    info.trackType = TRACK_TYPE_TEXT;

    static const std::vector<std::string_view> targetFields = {
        "format"
    };

    ExpectedCapsParse(structure, targetFields);
}

void GstMetaParser::FileCapsParse(const GstStructure &structure)
{
    MEDIA_LOGI("file caps catched");
    std::string_view mimeType = gst_structure_get_name(&structure);
    fileMetaInfo_->capsCatched = true;

    const auto &it = FILE_MIME_TYPE_MAPPING.find(mimeType);
    if (it == FILE_MIME_TYPE_MAPPING.end()) {
        MEDIA_LOGE("unrecognized mimetype: %{public}s", mimeType.data());
        return;
    }

    MEDIA_LOGI("file caps mime type: %{public}s, mapping to %{public}s", mimeType.data(), it->second.data());
    (void)allMetaInfo_.emplace(INNER_META_KEY_MIME_TYPE, it->second);
}

void GstMetaParser::CapsEventParse(const GstPad &pad, const GstCaps &caps)
{
    guint capsSize = gst_caps_get_size(&caps);
    for (guint index = 0; index < capsSize; index++) {
        const GstStructure *struc = gst_caps_get_structure(&caps, index);
        std::string_view mimeType = gst_structure_get_name(struc);
        if (mimeType.find("video") == 0) {
            VideoCapsParse(pad, *struc);
        } else if (mimeType.find("audio") == 0) {
            AudioCapsParse(pad, *struc);
        } else if (mimeType.find("image") == 0) {
            ImageCapsParse(pad, *struc);
        } else if (mimeType.find("text") == 0) {
            TextCapsParse(pad, *struc);
        }
    }

    cond_.notify_all();
}

void GstMetaParser::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("enter");

    collecting_ = false;
    cond_.notify_all();
}

void GstMetaParser::Reset()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("enter");

    collecting_ = false;
    allMetaInfo_.clear();
    demuxer_ = nullptr;

    for (auto &[ pad, padInfo ] : padInfos_) {
        if (padInfo.probeId != 0) {
            gst_pad_remove_probe(pad, padInfo.probeId);
        }
        if (padInfo.blockId != 0) {
            gst_pad_remove_probe(pad, padInfo.blockId);
        }
        gst_object_unref(pad);
    }
    padInfos_.clear();
    trackMetaInfos_.clear();
    fileMetaInfo_ = nullptr;

    cond_.notify_all();

    MEDIA_LOGD("exit");
}

int32_t GstMetaParser::AddProbeToPad(GstPad &pad, MetaSourceType srcType)
{
    auto wrapper1 = new (std::nothrow) GstMetaParserWrapper(shared_from_this());
    CHECK_AND_RETURN_RET_LOG(wrapper1 != nullptr, MSERR_NO_MEMORY, "can not create this wrapper");

    GstPad *padRef = reinterpret_cast<GstPad *>(gst_object_ref(&pad));
    gulong probeId = 0;
    if (srcType != MetaSourceType::META_SRC_DECODER) {
        probeId = gst_pad_add_probe(padRef, GST_PAD_PROBE_TYPE_DATA_DOWNSTREAM,
            ProbeCallback, wrapper1, &GstMetaParserWrapper::OnDestory);
        (void)trackMetaInfos_.emplace(padRef, TrackMetaInfo {});
        allMetaInfo_.at(INNER_META_KEY_NUM_TRACKS) = std::to_string(trackMetaInfos_.size());
    }

    gulong blockId = 0;
    if (needBlockBuffer_)  {
        auto wrapper2 = new (std::nothrow) GstMetaParserWrapper(shared_from_this());
        CHECK_AND_RETURN_RET_LOG(wrapper2 != nullptr, MSERR_NO_MEMORY, "can not create this wrapper");

        blockId = gst_pad_add_probe(padRef, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
            BlockCallback, wrapper2, &GstMetaParserWrapper::OnDestory);
    }

    (void)padInfos_.emplace(padRef, PadInfo { srcType, GST_PAD_DIRECTION(padRef), probeId, blockId });
    return MSERR_OK;
}
}
}