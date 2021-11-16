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
#include "scope_guard.h"

namespace {
    constexpr gint DEFAULT_FRAME_RATE = 30;
}
static GstStaticPadTemplate gst_src_template =
GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(GST_VIDEO_CAPS_MAKE(GST_VIDEO_FORMATS_ALL)));

#define gst_encoder_surface_src_parent_class parent_class
G_DEFINE_TYPE(GstEncoderSurfaceSrc, gst_encoder_surface_src, GST_TYPE_SURFACE_POOL_SRC);

static GstCaps *gst_encoder_surface_src_fix_caps(GstEncoderSurfaceSrc *encoderSrc)
{
    g_return_val_if_fail(encoderSrc != nullptr, nullptr);
    GstSurfacePoolSrc *surfaceSrc = GST_SURFACE_POOL_SRC(encoderSrc);
    if (encoderSrc->newCaps != nullptr) {
        gst_caps_unref(encoderSrc->newCaps);
    }
    encoderSrc->newCaps = gst_caps_new_simple("video/x-raw",
        "format", G_TYPE_STRING, "I420",
        "framerate", GST_TYPE_FRACTION, DEFAULT_FRAME_RATE, 1,
        "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
        "width", G_TYPE_INT, surfaceSrc->videoWidth,
        "height", G_TYPE_INT, surfaceSrc->videoHeight,
        nullptr);
    return encoderSrc->newCaps;
}

static gboolean gst_encoder_surface_src_negotiate(GstBaseSrc *basesrc)
{
    g_return_val_if_fail(basesrc != nullptr, FALSE);
    GstEncoderSurfaceSrc *encoderSrc = GST_ENCODER_SURFACE_SRC(basesrc);
    return gst_base_src_set_caps(basesrc, gst_encoder_surface_src_fix_caps(encoderSrc));
}

static void gst_encoder_surface_src_finalize(GObject *object)
{
    GstEncoderSurfaceSrc *src = GST_ENCODER_SURFACE_SRC(object);
    g_return_if_fail(src != nullptr);

    if (src->newCaps != nullptr) {
        gst_caps_unref(src->newCaps);
        src->newCaps = nullptr;
    }
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_encoder_surface_src_class_init(GstEncoderSurfaceSrcClass *klass)
{
    g_return_if_fail(klass != nullptr);
    GObjectClass *gobject_class = reinterpret_cast<GObjectClass *>(klass);
    GstElementClass *gstelement_class = reinterpret_cast<GstElementClass *>(klass);
    GstBaseSrcClass *gstbasesrc_class = reinterpret_cast<GstBaseSrcClass *>(klass);

    gst_element_class_set_static_metadata(gstelement_class,
        "encoder surface source", "Source/Surface/Encoder",
        "Retrieve frame from surface buffer queue with raw data", "OpenHarmony");

    gst_element_class_add_static_pad_template(gstelement_class, &gst_src_template);

    gstbasesrc_class->negotiate = gst_encoder_surface_src_negotiate;
    gobject_class->finalize = gst_encoder_surface_src_finalize;
}

static void gst_encoder_surface_src_init(GstEncoderSurfaceSrc *self)
{
    (void)self;
}