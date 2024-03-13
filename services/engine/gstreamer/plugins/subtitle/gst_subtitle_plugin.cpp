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

#include "config.h"
#include "gst/gst.h"
#include "gst_subtitle_srt_parse.h"

typedef gboolean (*SubParserRegisterFunc)(GstPlugin *plugin);
typedef struct {
    const gchar *name;
    SubParserRegisterFunc func;
} SubtitleParserItem;

static SubtitleParserItem g_subtitle_parser[] = {
    { "srt", gst_subtitle_srt_parse_register },
    { nullptr, nullptr },
};

static gboolean plugin_init(GstPlugin *plugin)
{
    guint i = 0;
    gboolean reg_parser = FALSE;
    gboolean ret = FALSE;
    SubtitleParserItem *item = &g_subtitle_parser[i];

    while (item->name != nullptr && item->func != nullptr) {
        reg_parser = item->func(plugin);
        GST_INFO("subtitle: register %s parse %s", item->name, reg_parser ? "success" : "fail");
        ret = (ret | reg_parser);
        i++;
        item = &g_subtitle_parser[i];
    }

    return ret;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, _subtitle_parse_plugin, "GStreamer Subtitle Parser",
                  plugin_init, PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
