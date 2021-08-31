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

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstMetaParser"};
}

namespace OHOS {
namespace Media {
enum StreamType : uint8_t {
    STREAM_TYPE_UNKNOWN,
    STREAM_TYPE_GLOBAL,
    STREAM_TYPE_VIDEO,
    STREAM_TYPE_AUDIO,
};

struct KeyToXMap {
    std::string_view keyName;
    uint8_t streamType;
};

#define STRINGFY(key) #key
static const KeyToXMap KEY_TO_X_MAP[INNER_META_KEY_BUTT] = {
    { STRINGFY(INNER_META_KEY_ALBUM), STREAM_TYPE_GLOBAL },
    { STRINGFY(INNER_META_KEY_ALBUMARTIST), STREAM_TYPE_GLOBAL },
    { STRINGFY(INNER_META_KEY_ARTIST), STREAM_TYPE_GLOBAL },
    { STRINGFY(INNER_META_KEY_AUTHOR), STREAM_TYPE_GLOBAL },
    { STRINGFY(INNER_META_KEY_COMPOSER), STREAM_TYPE_GLOBAL },
    { STRINGFY(INNER_META_KEY_DURATION), STREAM_TYPE_GLOBAL },
    { STRINGFY(INNER_META_KEY_GENRE), STREAM_TYPE_GLOBAL },
    { STRINGFY(INNER_META_KEY_HAS_AUDIO), STREAM_TYPE_GLOBAL },
    { STRINGFY(INNER_META_KEY_HAS_VIDEO), STREAM_TYPE_GLOBAL },
    { STRINGFY(INNER_META_KEY_MIMETYPE), STREAM_TYPE_GLOBAL },
    { STRINGFY(INNER_META_KEY_NUM_TRACKS), STREAM_TYPE_GLOBAL },
    { STRINGFY(INNER_META_KEY_SAMPLERATE), STREAM_TYPE_AUDIO },
    { STRINGFY(INNER_META_KEY_TITLE), STREAM_TYPE_GLOBAL },
    { STRINGFY(INNER_META_KEY_VIDEO_HEIGHT), STREAM_TYPE_VIDEO },
    { STRINGFY(INNER_META_KEY_VIDEO_WIDTH), STREAM_TYPE_VIDEO },
};

static const std::unordered_map<std::string_view, int32_t> TAG_TO_KEY_MAPPING = {
    { GST_TAG_ALBUM, INNER_META_KEY_ALBUM },
    { GST_TAG_ALBUM_ARTIST, INNER_META_KEY_ALBUMARTIST },
    { GST_TAG_ARTIST, INNER_META_KEY_ARTIST },
    { GST_TAG_COMPOSER, INNER_META_KEY_COMPOSER },
    { GST_TAG_DURATION, INNER_META_KEY_DURATION },
    { GST_TAG_GENRE, INNER_META_KEY_GENRE },
    { GST_TAG_TRACK_COUNT, INNER_META_KEY_NUM_TRACKS },
    { GST_TAG_TITLE, INNER_META_KEY_TITLE }
};

static const std::unordered_map<std::string_view, int32_t> CAPS_FIELD_TO_KEY_MAPPING = {
    { "width", INNER_META_KEY_VIDEO_WIDTH },
    { "height", INNER_META_KEY_VIDEO_HEIGHT },
    { "rate", INNER_META_KEY_SAMPLERATE },
};

struct GstMetaParser::StreamMetaInfo {
    bool tagCatched = false;
    bool capsCatched = false;
    uint8_t streamType = STREAM_TYPE_UNKNOWN;
};

struct GstMetaParser::GlobalMetaInfo {
    bool tagCatched = false;
};

GstMetaParser::GstMetaParser()
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

GstMetaParser::~GstMetaParser()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

int32_t GstMetaParser::Init()
{
    globalInfos_ = std::make_unique<GlobalMetaInfo>();
    allMetaInfo_.emplace(INNER_META_KEY_NUM_TRACKS, "0");
    return MSERR_OK;
}

void GstMetaParser::AddMetaSource(GstElement &src)
{
    MEDIA_LOGD("enter");

    if (g_list_length(src.sinkpads) != 1) { // unsupport the multi-input's element
        MEDIA_LOGE("elem %{public}s has sinkpad count is not 1, ignored", GST_ELEMENT_NAME(&src));
        return;
    }

    GList *padNode = g_list_first(src.sinkpads);
    CHECK_AND_RETURN_LOG(padNode->data != nullptr, "padnode's data is nullptr");

    GstPad *pad = reinterpret_cast<GstPad *>(gst_object_ref(padNode->data));
    gulong probeId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
                                       (GstPadProbeCallback)EventProbe, this, nullptr);

    MEDIA_LOGD("pad: 0x%{public}06" PRIXPTR "", FAKE_POINTER(pad));

    std::unique_lock<std::mutex> lock(mutex_);
    (void)metaSrcs_.emplace(&src, PadProbeIdPair {pad, probeId});
    (void)streamInfos_.emplace(pad, StreamMetaInfo {});

    allMetaInfo_[INNER_META_KEY_NUM_TRACKS] = std::to_string(streamInfos_.size());
    MEDIA_LOGI("add meta probe for %{public}s's %{public}s", GST_ELEMENT_NAME(&src), GST_PAD_NAME(pad));
}

std::string GstMetaParser::GetMetadata(int32_t key)
{
    CHECK_AND_RETURN_RET_LOG(key < INNER_META_KEY_BUTT, "", "invalid key");

    std::unique_lock<std::mutex> lock(mutex_);
    if (key == INNER_META_KEY_NUM_TRACKS) {
        return allMetaInfo_[INNER_META_KEY_NUM_TRACKS];
    }

    cond_.wait(lock, [this]() { return canceled_ || CheckCollectCompleted(); });

    if (canceled_) {
        return "";
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
    cond_.wait(lock, [this]() { return canceled_ || CheckCollectCompleted(); });

    return allMetaInfo_;
}

bool GstMetaParser::CheckCollectCompleted() const
{
    if (streamInfos_.size() == 0) {
        return false;
    }

    for (auto &infoPair : streamInfos_) {
        if (!infoPair.second.tagCatched) {
            return false;
        }
        if (!infoPair.second.capsCatched) {
            return false;
        }
    }

    if ((streamInfos_.size() > 1) && !globalInfos_->tagCatched) {
        return false;
    }

    return true;
}

GstPadProbeReturn GstMetaParser::EventProbe(const GstPad *pad, GstPadProbeInfo *info,  GstMetaParser *thiz)
{
    CHECK_AND_RETURN_RET_LOG(thiz != nullptr, GST_PAD_PROBE_REMOVE, "thiz is nullptr");
    CHECK_AND_RETURN_RET_LOG(info != nullptr, GST_PAD_PROBE_OK, "info is null");

    GstEvent *event = gst_pad_probe_info_get_event (info);
    CHECK_AND_RETURN_RET_LOG(event != nullptr, GST_PAD_PROBE_OK, "event is null");

    MEDIA_LOGD("pad: 0x%{public}06" PRIXPTR "", FAKE_POINTER(pad));

    thiz->DoEventProbe(*pad, *event);

    return GST_PAD_PROBE_OK;
}

void GstMetaParser::DoEventProbe(const GstPad &pad, GstEvent &event)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(streamInfos_.count(&pad) != 0, "unknown pad");

    if (GST_EVENT_TYPE(&event) == GST_EVENT_TAG) {
        GstTagList *tagList = nullptr;
        gst_event_parse_tag(&event, &tagList);
        CHECK_AND_RETURN_LOG(tagList != nullptr, "taglist is nullptr");

        TagEventParse(pad, *tagList);
    }

    if (GST_EVENT_TYPE(&event) == GST_EVENT_CAPS) {
        GstCaps *caps;
        gst_event_parse_caps (&event, &caps);
        CHECK_AND_RETURN_LOG(caps != nullptr, "caps is nullptr");

        CapsEventParse(pad,  *caps);
    }
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
        return;
    }

    gchar *str = gst_value_serialize(value);
    CHECK_AND_RETURN_LOG(str != nullptr, "serialize value failed");

    MEDIA_LOGI("got meta, key: %{public}s, value: %{public}s", KEY_TO_X_MAP[tagToKeyIt->second].keyName.data(), str);
    (void)thiz->allMetaInfo_.emplace(tagToKeyIt->second, std::string(str));
    g_free(str);
}

void GstMetaParser::TagEventParse(const GstPad &pad, const GstTagList &tagList)
{
    GstTagScope scope = gst_tag_list_get_scope(&tagList);
    MEDIA_LOGI("catch tag %{public}s event", scope == GST_TAG_SCOPE_GLOBAL ? "global" : "stream");

    if (scope == GST_TAG_SCOPE_GLOBAL) {
        globalInfos_->tagCatched = true;
    } else {
        streamInfos_.at(&pad).tagCatched = true;
    }

    gst_tag_list_foreach(&tagList, (GstTagForeachFunc)TagVisitor, this);

    cond_.notify_one();
}

void GstMetaParser::ExpectedCapsParse(const GstStructure &structure, const std::vector<std::string_view> &fields)
{
    for (auto &field :  fields) {
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
        (void)allMetaInfo_.emplace(CAPS_FIELD_TO_KEY_MAPPING.at(field), str);
        g_free(str);
    }
}

void GstMetaParser::VideoCapsParse(const GstPad &pad, const GstStructure &structure)
{
    MEDIA_LOGI("has video stream");
    if (allMetaInfo_.count(INNER_META_KEY_HAS_VIDEO) != 0) {
        return;
    }

    (void)allMetaInfo_.emplace(INNER_META_KEY_HAS_VIDEO, "yes");
    std::string_view mimeType = gst_structure_get_name(&structure);
    allMetaInfo_[INNER_META_KEY_MIMETYPE] = mimeType;

    StreamMetaInfo &info = streamInfos_.at(&pad);
    info.capsCatched = true;
    info.streamType = STREAM_TYPE_VIDEO;

    static const std::vector<std::string_view> targetFields = {
        "width", "height",
    };

    ExpectedCapsParse(structure, targetFields);
}

void GstMetaParser::AudioCapsParse(const GstPad &pad, const GstStructure &structure)
{
    MEDIA_LOGI("has audio stream");
    if (allMetaInfo_.count(INNER_META_KEY_HAS_AUDIO) != 0) {
        return;
    }

    (void)allMetaInfo_.emplace(INNER_META_KEY_HAS_AUDIO, "yes");
    std::string_view mimeType = gst_structure_get_name(&structure);
    if (allMetaInfo_.count(INNER_META_KEY_HAS_VIDEO) == 0) {
        allMetaInfo_[INNER_META_KEY_MIMETYPE] = mimeType;
    }

    StreamMetaInfo &info = streamInfos_.at(&pad);
    info.capsCatched = true;
    info.streamType = STREAM_TYPE_AUDIO;

    static const std::vector<std::string_view> targetFields = {
        "rate",
    };

    ExpectedCapsParse(structure, targetFields);
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
        }
    }

    cond_.notify_one();
}

void GstMetaParser::Reset()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("enter");

    canceled_ = true;
    allMetaInfo_.clear();

    for (auto &elemPair : metaSrcs_) {
        auto &padProbeIdPair = elemPair.second;
        gst_pad_remove_probe(padProbeIdPair.first, padProbeIdPair.second);
        gst_object_unref(padProbeIdPair.first);
    }
    metaSrcs_.clear();
    streamInfos_.clear();
    if (globalInfos_ != nullptr) {
        globalInfos_->tagCatched = false;
        globalInfos_ = nullptr;
    }

    cond_.notify_all();
}
}
}