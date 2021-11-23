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
#include "gst_encoder_surface_src.h"
#include <gst/video/video.h>

static GstStaticPadTemplate gst_src_template =
GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(GST_VIDEO_CAPS_MAKE(GST_VIDEO_FORMATS_ALL)));

#define gst_encoder_surface_src_parent_class parent_class
G_DEFINE_TYPE(GstEncoderSurfaceSrc, gst_encoder_surface_src, GST_TYPE_SURFACE_POOL_SRC);

static void gst_encoder_surface_src_class_init(GstEncoderSurfaceSrcClass *klass)
{
    g_return_if_fail(klass != nullptr);
    GstElementClass *gstelement_class = reinterpret_cast<GstElementClass *>(klass);

    gst_element_class_set_static_metadata(gstelement_class,
        "encoder surface source", "Source/Surface/Encoder",
        "Retrieve frame from surface buffer queue with raw data", "OpenHarmony");

    gst_element_class_add_static_pad_template(gstelement_class, &gst_src_template);
}

static void gst_encoder_surface_src_init(GstEncoderSurfaceSrc *self)
{
    (void)self;
}
