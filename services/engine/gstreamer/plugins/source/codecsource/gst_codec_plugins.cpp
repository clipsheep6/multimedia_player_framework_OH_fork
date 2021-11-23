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
#include "config.h"
#include "gst_encoder_surface_src.h"
#include "gst_codec_shmem_src.h"

static gboolean plugin_init(GstPlugin *plugin)
{
    gboolean ret = FALSE;
    if (gst_element_register(plugin, "encodersurfacesrc", GST_RANK_PRIMARY, GST_TYPE_ENCODER_SURFACE_SRC)) {
        ret = TRUE;
    } else {
        GST_WARNING_OBJECT(NULL, "register encodersurfacesrc failed");
    }
    if (gst_element_register(plugin, "codecshmemsrc", GST_RANK_PRIMARY, GST_TYPE_CODEC_SHMEM_SRC)) {
        ret = TRUE;
    } else {
        GST_WARNING_OBJECT(NULL, "register codecshmemsrc failed");
    }
    return ret;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    _codec_src,
    "GStreamer Codec Source",
    plugin_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
