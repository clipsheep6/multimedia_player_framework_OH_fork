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
#include "gst_codec_shmem_src.h"
#include <gst/video/video.h>

static GstStaticPadTemplate gst_src_template =
GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

#define gst_codec_shemm_src_parent_class parent_class
G_DEFINE_TYPE(GstCodecShmemSrc, gst_codec_shmem_src, GST_TYPE_SHMEM_POOL_SRC);

static void gst_codec_shmem_src_class_init(GstCodecShmemSrcClass *klass)
{
    g_return_if_fail(klass != nullptr);
    GstElementClass *gstelement_class = reinterpret_cast<GstElementClass *>(klass);

    gst_element_class_set_static_metadata(gstelement_class,
        "codec shmem source", "Source/shmem/codec",
        "Retrieve frame from shmem pool with raw data", "OpenHarmony");

    gst_element_class_add_static_pad_template(gstelement_class, &gst_src_template);
}

static void gst_codec_shmem_src_init(GstCodecShmemSrc *self)
{
    (void)self;
}