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
#include <gst/gst.h>
#include <nocopyable.h>

namespace OHOS {
namespace Media {
class GstMetaParser {
public:
    GstMetaParser();
    ~GstMetaParser();

    int32_t Init();
    void AddMetaSource(GstElement &src);
    std::string GetMetadata(int32_t key);
    std::unordered_map<int32_t, std::string> GetMetadata();
    void Reset();

    DISALLOW_COPY_AND_MOVE(GstMetaParser);

private:
    bool CheckCollectCompleted() const;
    static GstPadProbeReturn EventProbe(const GstPad *pad, GstPadProbeInfo *info, GstMetaParser *thiz);
    void DoEventProbe(const GstPad &pad, GstEvent &event);
    void TagEventParse(const GstPad &pad, const GstTagList &tagList);
    static void TagVisitor(const GstTagList *list, const gchar *tag, GstMetaParser *thiz);
    void CapsEventParse(const GstPad &pad, const GstCaps &caps);
    void VideoCapsParse(const GstPad &pad, const GstStructure &structure);
    void AudioCapsParse(const GstPad &pad, const GstStructure &structure);
    void ExpectedCapsParse(const GstStructure &structure, const std::vector<std::string_view> &fields);

    std::mutex mutex_;
    std::condition_variable cond_;
    bool canceled_ = false;

    using PadProbeIdPair = std::pair<GstPad *, gulong>;
    std::unordered_map<const GstElement *, PadProbeIdPair> metaSrcs_;
    std::unordered_map<int32_t, std::string> allMetaInfo_;

    struct StreamMetaInfo;
    std::unordered_map<const GstPad *, StreamMetaInfo> streamInfos_;
    struct GlobalMetaInfo;
    std::unique_ptr<GlobalMetaInfo> globalInfos_;
};
}
}

#endif