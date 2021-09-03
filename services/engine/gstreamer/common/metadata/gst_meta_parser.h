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

#ifndef GST_META_PARSER_H
#define GST_META_PARSER_H

#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <gst/gst.h>
#include <nocopyable.h>

namespace OHOS {
namespace Media {
template <typename T>
class ThizWrapper;

class GstMetaParser : public std::enable_shared_from_this<GstMetaParser> {
public:
    enum class MetaSourceType : uint8_t {
        META_SRC_UNKNOWN,
        META_SRC_DECODER,
        META_SRC_PARSER,
        META_SRC_DEMUXER,
        META_SRC_TYPEFIND,
    };

    GstMetaParser();
    ~GstMetaParser();

    void Start(bool needBlockBuffer = false);
    void BlockBuffer(bool needBlock);
    void AddMetaSource(GstElement &src, MetaSourceType type);
    std::string GetMetadata(int32_t key);
    std::unordered_map<int32_t, std::string> GetMetadata();
    void Stop();

    DISALLOW_COPY_AND_MOVE(GstMetaParser);

private:
    void Reset();
    void AddDecoderMetaSource(GstElement &src);
    void AddParserMetaSource(GstElement &src);
    void AddDemuxerMetaSource(GstElement &src);
    void AddTypeFindMetaSource(GstElement &src);
    bool CheckCollectCompleted() const;
    void TagEventParse(GstPad &pad, const GstTagList &tagList);
    void CapsEventParse(const GstPad &pad, const GstCaps &caps);
    void FileCapsParse(const GstStructure &structure);
    void VideoCapsParse(const GstPad &pad, const GstStructure &structure);
    void AudioCapsParse(const GstPad &pad, const GstStructure &structure);
    void ImageCapsParse(const GstPad &pad, const GstStructure &structure);
    void TextCapsParse(const GstPad &pad, const GstStructure &structure);
    void ExpectedCapsParse(const GstStructure &structure, const std::vector<std::string_view> &fields);
    static void PadAddedCallback(GstElement *elem, GstPad *pad, gpointer userdata);
    static GstPadProbeReturn ProbeCallback(GstPad *pad, GstPadProbeInfo *info, gpointer usrdata);
    static GstPadProbeReturn BlockCallback(GstPad *pad, GstPadProbeInfo *info, gpointer usrdata);
    static void HaveTypeCallback(GstElement *elem, guint probability, GstCaps *caps, gpointer userdata);
    static void TagVisitor(const GstTagList *list, const gchar *tag, GstMetaParser *thiz);
    void OnHaveType(const GstElement &elem, guint probability, const GstCaps &caps);
    void OnPadAdded(const GstElement &elem, GstPad &pad);
    void OnEventProbe(GstPad &pad, GstEvent &event);
    void OnBufferProbe(GstPad &pad, GstBuffer &buffer);
    int32_t AddProbeToPad(GstPad &pad, MetaSourceType srcType);
    void QueryDuration(GstPad &pad);

    std::mutex mutex_;
    std::condition_variable cond_;
    bool collecting_ = false;
    bool needBlockBuffer_ = false;

    struct PadInfo;
    std::unordered_map<GstPad *, PadInfo> padInfos_;
    GstElement *demuxer_ = nullptr;

    std::unordered_map<int32_t, std::string> allMetaInfo_;
    struct TrackMetaInfo;
    std::unordered_map<const GstPad *, TrackMetaInfo> trackMetaInfos_;
    struct FileMetaInfo;
    std::unique_ptr<FileMetaInfo> fileMetaInfo_;
};
}
}

#endif