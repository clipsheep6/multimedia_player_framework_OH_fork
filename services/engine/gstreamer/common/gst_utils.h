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

#ifndef GST_UTILS_H
#define GST_UTILS_H

#include <memory>
#include <vector>
#include <string>
#include <glib/glib.h>
#include <gst/gst.h>
#include "nocopyable.h"
#include "string_ex.h"

namespace OHOS {
namespace Media {
static bool MatchElementByKlassMeta(const GstElement &elem, const std::vector<std::string_view> &expectedMetaFields);
static GstPadProbeReturn BlockPadForBuffer(const GstPad *pad, const GstPadProbeInfo *info, const gpointer usrdata);

template <typename T>
class ThizWrapper {
public:
    ThizWrapper(std::weak_ptr<T> thiz) : thiz_(thiz) {}
    ~ThizWrapper() = default;

    DISALLOW_COPY_AND_MOVE(ThizWrapper);

    static std::shared_ptr<T> TakeStrongThiz(gpointer userdata)
    {
        if (userdata == nullptr) {
            return nullptr;
        }

        ThizWrapper<T> *wrapper = reinterpret_cast<ThizWrapper<T> *>(userdata);
        return wrapper->thiz_.lock();
    }

    static void OnDestory(gpointer wrapper, GClosure *unused)
    {
        (void)unused;
        if (wrapper == nullptr) {
            return;
        }
        delete reinterpret_cast<ThizWrapper<T> *>(wrapper);
    }

private:
    std::weak_ptr<T> thiz_;
};

static bool MatchElementByKlassMeta(const GstElement &elem, const std::vector<std::string_view> &expectedMetaFields)
{
    const gchar *metadata = gst_element_get_metadata(const_cast<GstElement *>(&elem), GST_ELEMENT_METADATA_KLASS);
    std::vector<std::string> klassDesc;
    SplitStr(metadata, "/", klassDesc, false, true);

    size_t matchCnt = 0;
    for (auto &expectedField : expectedMetaFields) {
        for (auto &item : klassDesc) {
            if (item.compare(expectedField) == 0) {
                matchCnt += 1;
                break;
            }
        }
    }

    if (matchCnt == expectedMetaFields.size()) {
        return true;
    }

    return false;
}

static GstPadProbeReturn BlockPadForBuffer(const GstPad *pad, const GstPadProbeInfo *info, const gpointer usrdata)
{
    (void)usrdata;
    (void)pad;

    if (static_cast<unsigned int>(info->type) & GST_PAD_PROBE_TYPE_BUFFER) {
        return GST_PAD_PROBE_OK;
    }

    if (static_cast<unsigned int>(info->type) & GST_PAD_PROBE_TYPE_BUFFER_LIST) {
        return GST_PAD_PROBE_OK;
    }

    return GST_PAD_PROBE_PASS;
}
}
}

#endif