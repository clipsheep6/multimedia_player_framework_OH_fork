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

#ifndef GST_AVSPLITER_UTILS_H
#define GST_AVSPLITER_UTILS_H

#include <cinttypes>
#include <gst/gst.h>

#define CHECK_AND_BREAK_LOG(obj, cond, fmt, ...)       \
    if (1) {                                           \
        if (!(cond)) {                                 \
            GST_ERROR_OBJECT(obj, fmt, ##__VA_ARGS__); \
            break;                                     \
        }                                              \
    } else void (0)

#define CHECK_AND_BREAK(cond)                          \
    if (!(cond)) {                                     \
        break;                                         \
    }

#define CHECK_AND_CONTINUE(cond)                       \
    if (!(cond)) {                                     \
        continue;                                      \
    }

#define FREE_GST_OBJECT(obj)                           \
    if ((obj) != nullptr) {                            \
        gst_object_unref(obj);                         \
        (obj) = nullptr;                               \
    }

#define POINTER_MASK 0x00FFFFFF
#define FAKE_POINTER(addr) (POINTER_MASK & reinterpret_cast<uintptr_t>(addr))

#define FACTORY_NAME(factory) gst_plugin_feature_get_name(GST_PLUGIN_FEATURE_CAST(factory))

G_GNUC_INTERNAL GstCaps *get_pad_caps(GstPad *pad);
G_GNUC_INTERNAL gchar *error_message_to_string(GstMessage *msg);
G_GNUC_INTERNAL gboolean check_is_demuxer_element(GstElement *element);
G_GNUC_INTERNAL gboolean check_is_parser_converter(GstElement *element);
G_GNUC_INTERNAL gboolean check_is_parser(GstElement *element);
G_GNUC_INTERNAL gboolean check_is_simple_demuxer_factory(GstElementFactory *factory);
G_GNUC_INTERNAL gint sort_pads(GstPad *pada, GstPad *padb);
G_GNUC_INTERNAL gboolean print_sticky_event(GstPad *pad, GstEvent **event, gpointer userdata);
G_GNUC_INTERNAL GstStreamType guess_stream_type_from_caps(GstCaps *caps);
G_GNUC_INTERNAL gboolean check_caps_is_factory_subset(
    GstElement *elem, GstCaps *caps, GstElementFactory *factory);

#endif